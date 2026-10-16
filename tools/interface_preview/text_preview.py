#!/usr/bin/env python3
"""Interface text preview for JA2 Stracciatella.

Draws texts onto an interface graphic exactly the way the game prints them
(sgp/Font.cc, MPrint() + Blt8BPPDataTo16BPPBufferMonoShadowClip()), with the
game's own fonts, and reports where the ink of every text actually lands:
its bounding box, the margins to the walls of its box and the gaps between
the lines. Meant for laying out texts on new interface panels before the
game is built.

    py -3 tools/interface_preview/text_preview.py render LAYOUT.json --data-dir GAME/Data --out preview.png
    py -3 tools/interface_preview/text_preview.py metrics FONT12ARIAL --data-dir GAME/Data [--text "Kills"]

See README.md for the layout file.
"""

import argparse
import json
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "tools", "sti_editor"))
import sti  # noqa: E402  (tools/sti_editor/sti.py)

DEFAULT_SHADOW = 2  # sgp/Font.h


class Glyph:
    def __init__(self, width, height, offset_x, offset_y, pixels, mask):
        self.width, self.height = width, height
        self.offset_x, self.offset_y = offset_x, offset_y
        self.pixels, self.mask = pixels, mask  # palette indices, 1 = opaque

    @property
    def advance(self):
        # GetWidth(), sgp/Font.cc
        return self.width + self.offset_x


class Font:
    def __init__(self, name, path, palette, glyphs):
        self.name, self.path, self.palette, self.glyphs = name, path, palette, glyphs

    @property
    def height(self):
        # GetFontHeight() = GetHeight(font, 0), sgp/Font.cc
        g = self.glyphs[0]
        return g.height + g.offset_y


def load_sti_font(name, path):
    f = sti.STIFile.load(path)
    glyphs = [Glyph(fr.width, fr.height, fr.offset_x, fr.offset_y, bytes(fr.pixels), bytes(fr.mask))
              for fr in f.frames]
    return Font(name, path, list(f.palette), glyphs)


def load_png_font(name, path):
    # PNG_Files: a palettised PNG plus PATH.json with its frames
    with open(path + ".json", encoding="utf-8") as fp:
        frames = json.load(fp)["frames"]
    im = Image.open(path)
    if im.mode != "P":
        raise SystemExit(f"{path}: a font PNG must be palettised")
    raw = im.getpalette() or []
    palette = [tuple(raw[i:i + 3]) for i in range(0, min(len(raw), 768), 3)]
    palette += [(0, 0, 0)] * (256 - len(palette))
    trns = im.info.get("transparency")
    alpha = [255] * 256
    if isinstance(trns, (bytes, bytearray)):
        for i, a in enumerate(trns):
            alpha[i] = a
    elif isinstance(trns, int):
        alpha[trns] = 0
    data = im.load()
    glyphs = []
    for fr in frames:
        w, h = fr["w"], fr["h"]
        px = bytearray(w * h)
        mk = bytearray(w * h)
        for yy in range(h):
            for xx in range(w):
                i = data[fr["x"] + xx, fr["y"] + yy]
                px[yy * w + xx] = i
                mk[yy * w + xx] = 1 if alpha[i] >= 128 else 0
        glyphs.append(Glyph(w, h, fr.get("offsetX", 0), fr.get("offsetY", 0), bytes(px), bytes(mk)))
    return Font(name, path, palette, glyphs)


def find_font(name, font_dirs):
    for d in font_dirs:
        if not os.path.isdir(d):
            continue
        files = {f.lower(): f for f in os.listdir(d)}
        for ext, loader in ((".png", load_png_font), (".sti", load_sti_font)):
            f = files.get(name.lower() + ext)
            if f and (ext != ".png" or (f.lower() + ".json") in files):
                return loader(name, os.path.join(d, f))
    raise SystemExit(f"font {name} not found in: {', '.join(font_dirs)}")


class Fonts:
    def __init__(self, font_dirs, table_path):
        self.font_dirs = font_dirs
        with open(table_path, encoding="utf-8") as fp:
            self.table = {k: v for k, v in json.load(fp).items()}
        self.cache = {}

    def get(self, name):
        if name not in self.cache:
            self.cache[name] = find_font(name, self.font_dirs)
        return self.cache[name]

    def glyph_index(self, font, c):
        # GetGlyphIndex(), sgp/Font.cc: unknown characters become '?'
        i = self.table.get(c, self.table.get("?"))
        return i if i < len(font.glyphs) else self.table.get("?")

    def text_width(self, font, text):
        # StringPixLength()
        return sum(font.glyphs[self.glyph_index(font, c)].advance for c in text)


def is_drawn(rgb, index):
    # usShadow/usBackground == 0 means "don't draw" (Get16BPPColor() of a
    # near-black colour is 0); SetFontShadow() turns a non-zero index that
    # maps to 0 into 1, i.e. drawn
    return index != 0 or any(c >= 8 for c in rgb)


def draw_text(img, fonts, font, text, x, y, fg, shadow, bg):
    """MPrint(): returns the bounding boxes (x0, y0, x1, y1) of the letters
    alone and of everything drawn (letters and their shadow), or None."""
    px = img.load()
    W, H = img.size
    fg_rgb = font.palette[fg]
    sh_rgb = font.palette[shadow] if is_drawn(font.palette[shadow], shadow) else None
    bg_rgb = font.palette[bg] if bg and is_drawn(font.palette[bg], bg) else None
    ink = None
    drawn = None
    for c in text:
        g = font.glyphs[fonts.glyph_index(font, c)]
        gx, gy = x + g.offset_x, y + g.offset_y
        for yy in range(g.height):
            for xx in range(g.width):
                i = yy * g.width + xx
                dx, dy = gx + xx, gy + yy
                if not (0 <= dx < W and 0 <= dy < H):
                    continue
                if not g.mask[i]:
                    if bg_rgb:
                        px[dx, dy] = bg_rgb + (255,)
                    continue
                v = g.pixels[i]
                col = (bg_rgb if v == 0 else sh_rgb if v == 1 else fg_rgb)
                if col is None:
                    continue
                px[dx, dy] = col + (255,)
                drawn = (dx, dy, dx, dy) if drawn is None else (
                    min(drawn[0], dx), min(drawn[1], dy), max(drawn[2], dx), max(drawn[3], dy))
                if v >= 2:  # the letters themselves, not their shadow
                    ink = (dx, dy, dx, dy) if ink is None else (
                        min(ink[0], dx), min(ink[1], dy), max(ink[2], dx), max(ink[3], dy))
        x += g.advance
    return ink, drawn


def resolve_text(item, strings):
    if "text" in item:
        return item["text"]
    key = item["string"]  # "gzInfoboxStatsStrings[3]" or "gzSomething"
    if key.endswith("]"):
        name, idx = key[:-1].split("[")
        return strings[name][int(idx)]
    return strings[key]


def place(fonts, font, text, item):
    """The print position, after the alignment FindFont*Coordinates() does."""
    x, y = item["x"], item["y"]
    align = item.get("align", "left")
    w = fonts.text_width(font, text)
    if align == "right":
        x = x + item["w"] - w  # FindFontRightCoordinates()
    elif align == "center":
        x = x + (item["w"] - w) // 2  # FindFontCenterCoordinates()
    if item.get("valign") == "center":
        y = y + (item["h"] - font.height) // 2
    return x, y


def cmd_render(a):
    with open(a.layout, encoding="utf-8") as fp:
        layout = json.load(fp)
    data = a.data_dir
    font_dirs = a.font_dir or [os.path.join(data, "Fonts"), os.path.join(data, "UNSLF", "fonts")]
    table = a.table or os.path.join(REPO, "assets", "externalized", "translation_tables", f"translation-table-{a.table_lang}.json")
    fonts = Fonts(font_dirs, table)
    strings = {}
    if a.strings or a.lang:
        path = a.strings or os.path.join(REPO, "assets", "externalized", "strings", f"translation-{a.lang}.json")
        with open(path, encoding="utf-8") as fp:
            strings = json.load(fp)

    bg_path = layout["background"]
    if not os.path.isabs(bg_path):
        bg_path = os.path.join(data, bg_path)
    img = Image.open(bg_path).convert("RGBA")
    defaults = layout.get("defaults", {})
    boxes = layout.get("boxes", [])

    report = []
    for raw in layout["items"]:
        item = dict(defaults, **raw)
        text = resolve_text(item, strings)
        font = fonts.get(item["font"])
        x, y = place(fonts, font, text, item)
        ink, drawn = draw_text(img, fonts, font, text, x, y, item.get("colour", 2),
                               item.get("shadow", DEFAULT_SHADOW), item.get("background", 0))
        box = next((b for b in boxes if b["x"] <= item["x"] < b["x"] + b["w"] and b["y"] <= item["y"] < b["y"] + b["h"]), None)
        report.append((item, text, font, x, y, ink, drawn, box))

    # "visible" = the letters with their shadow: what is seen on the screen
    print(f"{'text':<26} {'print x,y':>10} {'visible x':>10} {'visible y':>10}  box margins (visible): left right top bottom")
    for item, text, font, x, y, ink, drawn, box in report:
        if drawn is None:
            print(f"{text[:26]:<26} {x:>4},{y:<5}  (nothing drawn)")
            continue
        v = drawn
        line = f"{text[:26]:<26} {x:>4},{y:<5} {v[0]:>4}-{v[2]:<5} {v[1]:>4}-{v[3]:<5}"
        if box:
            line += (f"  {box['name']}: {v[0] - box['x']} {box['x'] + box['w'] - 1 - v[2]}"
                     f" {v[1] - box['y']} {box['y'] + box['h'] - 1 - v[3]}")
        print(line)

    # gaps between the lines of a box: the free rows between what is drawn of
    # one line (with its shadow) and the next (different texts reach
    # different rows -- capitals, descenders); also between the letters
    # alone, without the shadow
    def line_rows(b, idx):
        rows = sorted({(r[idx][1], r[idx][3]) for r in report if r[7] is b and r[idx]})
        merged = []
        for top, bottom in rows:
            if merged and top <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(merged[-1][1], bottom))
            else:
                merged.append((top, bottom))
        return merged, [merged[i + 1][0] - merged[i][1] - 1 for i in range(len(merged) - 1)]

    for b in boxes:
        rows, gaps = line_rows(b, 6)
        if len(rows) > 1:
            _, letter_gaps = line_rows(b, 5)
            print(f"{b['name']}: visible rows {rows}, free rows between lines {gaps} (letters alone: {letter_gaps})")

    if a.scale > 1:
        img = img.resize((img.width * a.scale, img.height * a.scale), Image.NEAREST)
    img.save(a.out)
    print(f"written {a.out}")


def cmd_metrics(a):
    data = a.data_dir
    font_dirs = a.font_dir or [os.path.join(data, "Fonts"), os.path.join(data, "UNSLF", "fonts")]
    table = a.table or os.path.join(REPO, "assets", "externalized", "translation_tables", f"translation-table-{a.table_lang}.json")
    fonts = Fonts(font_dirs, table)
    font = fonts.get(a.font)

    def rows(c):
        g = font.glyphs[fonts.glyph_index(font, c)]
        ys = [g.offset_y + yy for yy in range(g.height) for xx in range(g.width)
              if g.mask[yy * g.width + xx] and g.pixels[yy * g.width + xx] >= 2]
        return (min(ys), max(ys)) if ys else None

    print(f"{font.name}: {font.path}")
    print(f"GetFontHeight: {font.height}")
    for label, c in (("capital (H)", "H"), ("x-height (x)", "x"), ("descender (g)", "g"), ("accent (E-acute)", "É")):
        r = rows(c)
        if r:
            print(f"{label:<15} ink rows {r[0]}..{r[1]} ({r[1] - r[0] + 1} px)")
    if a.text:
        print(f"width of {a.text!r}: {fonts.text_width(font, a.text)} px")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    def common(sp):
        sp.add_argument("--data-dir", required=True, help="the game's Data directory")
        sp.add_argument("--font-dir", action="append", help="where to look for fonts, in order (default: DATA/Fonts, DATA/UNSLF/fonts)")
        sp.add_argument("--table", help="glyph translation table (default: assets/externalized/translation_tables)")
        sp.add_argument("--table-lang", default="eng", help="translation table suffix (eng, fr, rus, rus-gold, chs)")

    r = sub.add_parser("render", help="draw a layout's texts on its graphic and report their ink")
    r.add_argument("layout")
    common(r)
    r.add_argument("--out", required=True)
    r.add_argument("--scale", type=int, default=1)
    r.add_argument("--lang", default="eng", help="strings for 'string' items: assets/externalized/strings/translation-LANG.json")
    r.add_argument("--strings", help="a strings file instead of --lang")
    r.set_defaults(func=cmd_render)

    m = sub.add_parser("metrics", help="a font's height and ink rows")
    m.add_argument("font")
    common(m)
    m.add_argument("--text")
    m.set_defaults(func=cmd_metrics)

    a = p.parse_args()
    a.func(a)


if __name__ == "__main__":
    main()
