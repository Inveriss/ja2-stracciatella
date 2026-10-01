#!/usr/bin/env python3
"""Generates the PNG files used by src/sgp/PNG_unittest.cc.

The files are written by hand (zlib + struct, no imaging library), so every
detail that the decoder has to handle is under control: bit depth, palette
size, tRNS, row filter types and Adam7 interlacing.

The pixel values follow simple formulas that PNG_unittest.cc repeats; if you
change a formula here, change it there as well.

It also writes pairs of an STI file (made with tools/sti_editor) and a PNG
(+ frame metadata) with the same content into assets/unittests/data/pngtest/.
PNG_unittest.cc loads both through the VFS and expects identical images.

Usage: python tools/generate_png_unittest_data.py
       (writes into assets/unittests/png/ and assets/unittests/data/pngtest/)
"""

import json
import os
import struct
import sys
import zlib

ASSETS_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "unittests")
OUT_DIR = os.path.join(ASSETS_DIR, "png")
PAIR_DIR = os.path.join(ASSETS_DIR, "data", "pngtest")

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "sti_editor"))
import sti  # noqa: E402

ADAM7 = [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4),
         (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)]


# --- formulas shared with PNG_unittest.cc ----------------------------------

def palette_entry(i):
    # Pairs of entries (2n, 2n+1) have the same colour on purpose: indices must
    # survive decoding even when the palette has duplicate colours.
    j = i & ~1
    return (j, 255 - j, (j * 7) & 0xFF)


def idx8_value(x, y):
    if (x, y) == (1, 0):
        return 254
    if (x, y) == (2, 0):
        return 255
    return (x * 37 + y * 101) % 256


def idx_low_value(x, y, depth):
    return (x + 2 * y) % (1 << depth)


def interlaced_value(x, y):
    return (x * 37 + y * 101) % 256


def rgb_value(x, y):
    return (x * 60, y * 100, (x + y) * 30)


def alpha_value(x, y):
    return (x * 80 + y * 20) % 256


def grey_value(x, y):
    return x * 60 + y * 20


def rgb16_value(x, y):
    return (x * 100, y * 100, 50)


# --- PNG writer -------------------------------------------------------------

def chunk(kind, data):
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def ihdr(width, height, depth, colour_type, interlace=0):
    return chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, depth, colour_type, 0, 0, interlace))


def pack_row(values, depth):
    if depth == 8:
        return bytes(values)
    out = bytearray()
    acc = 0
    bits = 0
    for v in values:
        acc = (acc << depth) | v
        bits += depth
        if bits == 8:
            out.append(acc)
            acc = 0
            bits = 0
    if bits:
        out.append(acc << (8 - bits))
    return bytes(out)


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def filter_row(filter_type, row, prev, bpp):
    out = bytearray([filter_type])
    for i, x in enumerate(row):
        a = row[i - bpp] if i >= bpp else 0
        b = prev[i]
        c = prev[i - bpp] if i >= bpp else 0
        pred = [0, a, b, (a + b) >> 1, paeth(a, b, c)][filter_type]
        out.append((x - pred) & 0xFF)
    return bytes(out)


def filtered_image(rows, bpp, filter_for_row):
    """rows: list of packed rows (bytes) of one (sub)image."""
    out = bytearray()
    prev = bytes(len(rows[0])) if rows else b""
    for y, row in enumerate(rows):
        out += filter_row(filter_for_row(y), row, prev, bpp)
        prev = row
    return bytes(out)


def write_png(name, header, extra_chunks, raw, directory=OUT_DIR):
    data = (b"\x89PNG\r\n\x1a\n" + header + b"".join(extra_chunks)
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    with open(os.path.join(directory, name), "wb") as f:
        f.write(data)


def plte(count):
    return chunk(b"PLTE", b"".join(bytes(palette_entry(i)) for i in range(count)))


# --- test files --------------------------------------------------------------

def indexed(name, width, height, depth, value, palette_size, extra=()):
    rows = [pack_row([value(x, y) for x in range(width)], depth) for y in range(height)]
    raw = filtered_image(rows, 1, lambda y: y % 5)
    write_png(name, ihdr(width, height, depth, 3), [plte(palette_size), *extra], raw)


def indexed_interlaced(name, width, height):
    raw = bytearray()
    for p, (x0, y0, dx, dy) in enumerate(ADAM7):
        xs = range(x0, width, dx)
        ys = range(y0, height, dy)
        if not xs or not ys:
            continue
        rows = [pack_row([interlaced_value(x, y) for x in xs], 8) for y in ys]
        raw += filtered_image(rows, 1, lambda y, p=p: (p + y) % 5)
    write_png(name, ihdr(width, height, 8, 3, interlace=1), [plte(256)], bytes(raw))


# --- STI / PNG pairs ----------------------------------------------------------

def pair_palette(count=256):
    return [((i * 5) & 0xFF, (i * 11) & 0xFF, (255 - i) & 0xFF) for i in range(count)]


def write_pair(stem, palette, frames, sheet_w, sheet_h, meta=None, trns=None):
    """frames: list of (x, y, w, h, offset_x, offset_y, pixels); pixels is a
    w*h list of palette indices, None for transparent. The STI gets the frames
    as subimages, the PNG a sheet with each frame at x, y (index 0 elsewhere)."""
    sheet = [0] * (sheet_w * sheet_h)
    s = sti.STIFile()
    s.palette = list(palette) + [(0, 0, 0)] * (256 - len(palette))
    transparent = {i for i, a in enumerate(trns or []) if a < 128} | {0}
    for x, y, w, h, ox, oy, pixels in frames:
        idx = bytearray(0 if p is None else p for p in pixels)
        mask = bytearray(0 if p is None or p in transparent else 255 for p in pixels)
        s.frames.append(sti.Frame(w, h, ox, oy, idx, mask))
        for j in range(h):
            for i in range(w):
                sheet[(y + j) * sheet_w + x + i] = pixels[j * w + i] or 0
    s.width, s.height = sheet_w, sheet_h
    s.save(os.path.join(PAIR_DIR, stem + ".sti"))

    rows = [bytes(sheet[r * sheet_w:(r + 1) * sheet_w]) for r in range(sheet_h)]
    extra = [chunk(b"PLTE", b"".join(bytes(c) for c in palette))]
    if trns:
        extra.append(chunk(b"tRNS", bytes(trns)))
    write_png(stem + ".png", ihdr(sheet_w, sheet_h, 8, 3), extra,
              filtered_image(rows, 1, lambda r: r % 5), PAIR_DIR)
    if meta is not None:
        with open(os.path.join(PAIR_DIR, stem + ".png.json"), "w", encoding="utf-8") as f:
            json.dump(meta, f, indent=2)
            f.write("\n")


def frame_pixels(w, h, seed):
    """Indices with transparent (None) and index 0 pixels, 254 and 255, and
    runs longer than 127 pixels in wide frames."""
    out = []
    for y in range(h):
        for x in range(w):
            v = (x * 7 + y * 13 + seed) % 23
            if v < 6:
                out.append(None)
            elif v == 6:
                out.append(254)
            elif v == 7:
                out.append(255)
            elif x >= 140:
                out.append(None)         # long transparent run at the end
            elif 5 <= x < 140 and y == 1:
                out.append(9)            # long opaque run
            else:
                out.append((x * 3 + y * 5 + seed) % 250 + 1)
    return out


def write_pairs():
    os.makedirs(PAIR_DIR, exist_ok=True)
    pal = pair_palette()

    # one frame, no metadata: rows longer than 127 pixels
    write_pair("single", pal, [(0, 0, 150, 4, 0, 0, frame_pixels(150, 4, 1))], 150, 4)

    # explicit frames of different sizes and offsets on a sheet
    frames = [(0, 0, 10, 6, -3, 2, frame_pixels(10, 6, 2)),
              (12, 0, 4, 9, 5, -7, frame_pixels(4, 9, 3)),
              (0, 9, 17, 3, 0, 0, frame_pixels(17, 3, 4))]
    write_pair("frames", pal, frames, 20, 12, meta={"frames": [
        {"x": x, "y": y, "w": w, "h": h, "offsetX": ox, "offsetY": oy}
        for x, y, w, h, ox, oy, _ in frames]})

    # a 3x2 grid of 5x4 cells, 5 of them used, with offsets
    offsets = [(0, 0), (-1, 1), (2, -2), (3, 3), (-4, 0)]
    frames = [(i % 3 * 5, i // 3 * 4, 5, 4, ox, oy, frame_pixels(5, 4, 10 + i))
              for i, (ox, oy) in enumerate(offsets)]
    write_pair("grid", pal, frames, 15, 8,
               meta={"grid": {"w": 5, "h": 4, "count": 5}, "offsets": [list(o) for o in offsets]})

    # 4 colour palette; tRNS makes index 2 transparent, index 3 half transparent
    pixels = [(x + y) % 4 for y in range(3) for x in range(6)]
    write_pair("trns", pal[:4], [(0, 0, 6, 3, 0, 0, pixels)], 6, 3, trns=[255, 255, 0, 200])

    # RGBA image for the video surface path (16 bpp): pure colours, black,
    # alpha just below and at the 128 threshold, a red too dark for RGB565
    rgba = [[(255, 0, 0, 255), (0, 255, 0, 255), (0, 0, 255, 255), (0, 0, 0, 255)],
            [(255, 255, 255, 0), (10, 20, 30, 127), (10, 20, 30, 128), (4, 0, 0, 255)]]
    rows = [b"".join(bytes(p) for p in row) for row in rgba]
    write_png("rgba_surface.png", ihdr(4, 2, 8, 6), [], filtered_image(rows, 4, lambda r: r % 5), PAIR_DIR)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    write_pairs()

    indexed("indexed8.png", 7, 5, 8, idx8_value, 256)
    for depth in (1, 2, 4):
        indexed(f"indexed{depth}.png", 5, 3, depth,
                lambda x, y, d=depth: idx_low_value(x, y, d), 1 << depth)

    indexed_interlaced("indexed8_interlaced.png", 9, 9)
    indexed_interlaced("indexed8_interlaced_1x1.png", 1, 1)

    trns = chunk(b"tRNS", bytes([0, 128]))
    indexed("indexed8_trns.png", 3, 2, 8, lambda x, y: (x + y) % 4, 4, [trns])

    indexed("indexed8_out_of_range.png", 2, 2, 8, lambda x, y: 7 if (x, y) == (1, 1) else 0, 4)

    w, h = 4, 3
    rgb = [b"".join(bytes(rgb_value(x, y)) for x in range(w)) for y in range(h)]
    write_png("rgb8.png", ihdr(w, h, 8, 2), [], filtered_image(rgb, 3, lambda y: y % 5))

    rgba = [b"".join(bytes(rgb_value(x, y) + (alpha_value(x, y),)) for x in range(w)) for y in range(h)]
    write_png("rgba8.png", ihdr(w, h, 8, 6), [], filtered_image(rgba, 4, lambda y: y % 5))

    grey = [bytes(grey_value(x, y) for x in range(w)) for y in range(h)]
    write_png("grey8.png", ihdr(w, h, 8, 0), [], filtered_image(grey, 1, lambda y: 0))

    # 16 bits per channel; the low byte must be dropped by the decoder.
    rgb16 = [b"".join(struct.pack(">HHH", *((v << 8) | 0x7F for v in rgb16_value(x, y))) for x in range(2))
             for y in range(2)]
    write_png("rgb16.png", ihdr(2, 2, 16, 2), [], filtered_image(rgb16, 6, lambda y: 0))


if __name__ == "__main__":
    main()
