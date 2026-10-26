"""Weapon redraw prototype: finds the weapon in the frames of a JA2 soldier
animation, erases it and draws another weapon shape along the same axis, then
writes comparison pictures (see README.md).

    python -I -B preview.py --data E:/JA2_Release1/Data --out out
"""
import argparse
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image, ImageDraw  # noqa: E402

from weapon import (SHADOW, axis, draw_weapon, erase, grip, read_palreps,  # noqa: E402
                    read_sti, slf_file, weapon_mask)

OUTFIT = ['BROWNHEAD', 'PINKSKIN', 'greyVEST', 'GREENPANTS']


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--data', default=r'E:/JA2_Release1/Data', help='game Data directory (Anims.slf, Binarydata.slf)')
    ap.add_argument('--anim', default='s_merc/s_r_std.sti', help='animation inside Anims.slf')
    ap.add_argument('--out', default='out', help='output directory')
    ap.add_argument('--zoom', type=int, default=4)
    ap.add_argument('--behind', default='0,7', help='directions where the weapon is behind the body')
    ap.add_argument('--anim-dirs', default='2,5', help='directions shown in the animated GIF')
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    behind = {int(d) for d in args.behind.split(',') if d}
    Z = args.zoom

    base, frames = read_sti(slf_file(os.path.join(args.data, 'Anims.slf'), args.anim))
    ranges, reps = read_palreps(args.data)
    pal = list(base)
    for r in OUTFIT:
        t, rgb = reps[r]
        s, e = ranges[t]
        for k in range(e - s + 1):
            pal[s + k] = rgb[k]
    per = len(frames) // 8

    def to_img(px, w, h, bg=(150, 140, 120)):
        im = Image.new('RGB', (w, h), bg)
        for y in range(h):
            for x in range(w):
                v = px[y][x]
                if v == SHADOW:
                    im.putpixel((x, y), tuple(int(c * 0.55) for c in bg))
                elif v > 0:
                    im.putpixel((x, y), pal[v])
        return im.resize((w * Z, h * Z), Image.NEAREST)

    def process(f, d):
        m = weapon_mask(f)
        er = erase(f, m)
        new, *_ = draw_weapon(f, er, m, d in behind)
        return m, er, new

    def grid(tiles, ncol, gap=10):
        W = max(t.width for t in tiles)
        H = max(t.height for t in tiles)
        sheet = Image.new('RGB', (W * ncol + gap * (ncol - 1), H * ((len(tiles) + ncol - 1) // ncol)), (235, 235, 235))
        for i, t in enumerate(tiles):
            sheet.paste(t, ((i % ncol) * (W + gap), (i // ncol) * H))
        return sheet

    # 1. the weapon mask (magenta), the hand pixels (yellow) and the axis (cyan)
    tiles = []
    for d in range(8):
        f = frames[d * per]
        m = weapon_mask(f)
        (cx, cy), (ux, uy), (u0, u1), _ = axis(m)
        im = to_img(f.px, f.w, f.h)
        ov = im.copy()
        dr = ImageDraw.Draw(ov)
        for (x, y) in m:
            dr.rectangle([x * Z, y * Z, x * Z + Z - 1, y * Z + Z - 1], fill=(255, 0, 255))
        for (x, y) in grip(f, m):
            dr.rectangle([x * Z, y * Z, x * Z + Z - 1, y * Z + Z - 1], fill=(255, 255, 0))
        dr.line([((cx + ux * u0) * Z + Z / 2, (cy + uy * u0) * Z + Z / 2),
                 ((cx + ux * u1) * Z + Z / 2, (cy + uy * u1) * Z + Z / 2)], fill=(0, 255, 255), width=2)
        t = Image.new('RGB', (im.width * 2 + 6, im.height + 16), (235, 235, 235))
        t.paste(im, (0, 16))
        t.paste(ov, (im.width + 6, 16))
        ImageDraw.Draw(t).text((2, 2), f'dir {d}: {len(m)} px, angle {math.degrees(math.atan2(uy, ux)):.0f}, '
                                       f'length {u1 - u0:.1f}', fill=(0, 0, 0))
        tiles.append(t)
    grid(tiles, 4).save(os.path.join(args.out, 'masks.png'))

    # 2. per direction: original | weapon erased | new weapon
    tiles = []
    for d in range(8):
        f = frames[d * per]
        _, er, new = process(f, d)
        ims = [to_img(p, f.w, f.h) for p in (f.px, er, new)]
        t = Image.new('RGB', (ims[0].width * 3 + 8, ims[0].height + 14), (235, 235, 235))
        for i, im in enumerate(ims):
            t.paste(im, (i * (ims[0].width + 4), 14))
        ImageDraw.Draw(t).text((2, 1), f'dir {d}: original | erased | new weapon', fill=(0, 0, 0))
        tiles.append(t)
    grid(tiles, 2).save(os.path.join(args.out, 'compare_dirs.png'))

    # 3. animation: every frame of some directions, original next to new
    dirs = [int(d) for d in args.anim_dirs.split(',') if d]
    gif = []
    for k in range(per):
        row = []
        for d in dirs:
            f = frames[d * per + k]
            _, _, new = process(f, d)
            row += [to_img(f.px, f.w, f.h), to_img(new, f.w, f.h)]
        fr = Image.new('RGB', (sum(im.width for im in row) + 10 * (len(row) - 1), max(im.height for im in row)),
                       (235, 235, 235))
        x = 0
        for im in row:
            fr.paste(im, (x, 0))
            x += im.width + 10
        gif.append(fr)
    gif[0].save(os.path.join(args.out, 'compare_anim.gif'), save_all=True, append_images=gif[1:], duration=150, loop=0)
    print('written to', os.path.abspath(args.out))


if __name__ == '__main__':
    main()
