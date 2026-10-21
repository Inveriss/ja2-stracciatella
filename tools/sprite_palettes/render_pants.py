"""Renders a JA2 soldier sprite with three trouser palettes side by side: the
original GREENPANTS of ja2pal.dat and two ramps computed in the OKLab colour
space, without the game's lighting (see README.md).

    python -I -B render_pants.py --data E:/JA2_Release1/Data --out pants_compare.png
"""
import argparse
import math
import os
import sys

# the game file readers of the weapon redraw prototype
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'weapon_redraw'))
from PIL import Image, ImageDraw  # noqa: E402

from ja2sprite import read_palreps, read_sti, slf_file  # noqa: E402

SHADOW = 254


# --- OKLab (Bjorn Ottosson, 2020) ---------------------------------------------

def srgb_to_linear(c):
    c /= 255
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def linear_to_srgb(c):
    c = max(0.0, min(1.0, c))
    v = 12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055
    return round(v * 255)


def to_oklab(c):
    r, g, b = [srgb_to_linear(x) for x in c]
    l = (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b) ** (1 / 3)
    m = (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b) ** (1 / 3)
    s = (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b) ** (1 / 3)
    return (0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s)


def from_oklab(L, a, b):
    l = (L + 0.3963377774 * a + 0.2158037573 * b) ** 3
    m = (L - 0.1055613458 * a - 0.0638541728 * b) ** 3
    s = (L - 0.0894841775 * a - 1.2914855480 * b) ** 3
    return (linear_to_srgb(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
            linear_to_srgb(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
            linear_to_srgb(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s))


def code_ramp(top, bottom, chroma_mul, hue_shift, n=15):
    """n shades from top down to bottom's lightness: even steps of perceived
    lightness, the colour fading out towards the shadows, lights 4 degrees
    towards yellow, shadows 10 degrees towards cyan."""
    L0, a, b = to_oklab(top)
    L1 = to_oklab(bottom)[0]
    C = math.hypot(a, b) * chroma_mul
    H = math.atan2(b, a) + math.radians(hue_shift)
    out = []
    for i in range(n):
        t = i / (n - 1)
        c = C * (1 - t ** 1.6)
        h = H + math.radians(-4 * (1 - t) + 10 * t)
        out.append(from_oklab(L0 + (L1 - L0) * t, c * math.cos(h), c * math.sin(h)))
    return out


# --- rendering ------------------------------------------------------------------

def render(base_pal, frame, ranges, reps, outfit, zoom):
    pal = list(base_pal)
    for rep in outfit:
        t, rgb = reps[rep]
        s, e = ranges[t]
        for k in range(e - s + 1):
            pal[s + k] = rgb[k]
    im = Image.new('RGBA', (frame.w, frame.h), (0, 0, 0, 0))
    for y, row in enumerate(frame.px):
        for x, v in enumerate(row):
            if v == 0:
                continue
            im.putpixel((x, y), (0, 0, 0, 110) if v == SHADOW else pal[v] + (255,))
    return im.resize((frame.w * zoom, frame.h * zoom), Image.NEAREST)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--data', default=r'E:/JA2_Release1/Data', help='game Data directory (Anims.slf, Binarydata.slf)')
    ap.add_argument('--anim', default='s_merc/s_r_std.sti', help='animation inside Anims.slf')
    ap.add_argument('--direction', type=int, default=4, help='0-7; 4 faces the viewer')
    ap.add_argument('--zoom', type=int, default=6)
    ap.add_argument('--out', default='pants_compare.png')
    args = ap.parse_args()

    base_pal, frames = read_sti(slf_file(os.path.join(args.data, 'Anims.slf'), args.anim))
    ranges, reps = read_palreps(args.data)
    # the ramps of the postponed in-game experiment (git stash "Code-drawn OKLab
    # trouser palettes"): GREENPANTS' olive, and a livelier military green
    reps['OKGREENPANTS'] = (1, code_ramp((79, 90, 45), (2, 2, 2), 1.0, 0))
    reps['OKARMYPANTS'] = (1, code_ramp((79, 90, 45), (2, 2, 2), 1.35, 14))
    frame = frames[args.direction * (len(frames) // 8)]
    Z = args.zoom

    cols = [('GREENPANTS (1999)', 'GREENPANTS'), ('OKGREENPANTS (code)', 'OKGREENPANTS'),
            ('OKARMYPANTS (code)', 'OKARMYPANTS')]
    rows = [
        ('Same outfit, only the trousers differ', (128, 128, 128),
         [(label, ['BROWNHEAD', 'PINKSKIN', 'greyVEST', key]) for label, key in cols]),
        ('As in the game', (96, 80, 56),
         [('administrator - GREENPANTS', ['BLACKHEAD', 'DARKSKIN', 'YELLOWVEST', 'GREENPANTS']),
          ('green militia - OKGREENPANTS', ['BLACKHEAD', 'DARKSKIN', 'GREENVEST', 'OKGREENPANTS']),
          ('soldier - OKARMYPANTS', ['BLONDHEAD', 'PINKSKIN', 'REDVEST', 'OKARMYPANTS'])]),
    ]
    cw, ch = frame.w * Z + 40, frame.h * Z + 40
    W = max(3 * cw, 3 * (15 * 14 + 20)) + 20
    img = Image.new('RGB', (W, 2 * (ch + 30) + 110), (235, 235, 235))
    d = ImageDraw.Draw(img)
    y = 10
    for title, bg, items in rows:
        d.text((10, y), title, fill=(0, 0, 0))
        y += 18
        for i, (label, outfit) in enumerate(items):
            x = 10 + i * (W - 20) // 3
            d.rectangle([x, y, x + cw - 1, y + ch - 1], fill=bg)
            spr = render(base_pal, frame, ranges, reps, outfit, Z)
            img.paste(spr, (x + 20, y + 20), spr)
            d.text((x, y + ch + 2), label, fill=(0, 0, 0))
        y += ch + 30
    d.text((10, y), 'The 15 trouser shades (lightest first)', fill=(0, 0, 0))
    y += 16
    for i, (label, key) in enumerate(cols):
        x = 10 + i * (W - 20) // 3
        for k, c in enumerate(reps[key][1]):
            d.rectangle([x + k * 14, y, x + k * 14 + 12, y + 30], fill=c)
        d.text((x, y + 34), label, fill=(0, 0, 0))
    img.save(args.out)
    print('saved', os.path.abspath(args.out))


if __name__ == '__main__':
    main()
