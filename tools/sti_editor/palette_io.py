"""Palette import/export for the STI editor.

An indexed STI stores one 256-entry RGB palette (STCIPaletteElement,
ImgFmt.h); pixels are indices into it. Importing a palette therefore
recolours the image without touching pixel indices.

Supported formats (the format is detected from the file contents on import):

    JASC-PAL        text, "JASC-PAL / 0100 / count / r g b"   (*.pal)
    Microsoft RIFF  binary "RIFF....PAL data"                  (*.pal)
    Adobe ACT       768 raw RGB bytes (+ optional 4-byte count) (*.act)
    GIMP            text, "GIMP Palette"                       (*.gpl)
    STI             the palette of another .sti file           (*.sti, import only)
    image           palette of a palettised PNG/BMP/GIF        (import only)
    PNG swatch      16x16 grid image, one cell per index       (*.png, export only)
"""

from __future__ import annotations

import os
import struct
from typing import List, Sequence, Tuple

RGB = Tuple[int, int, int]
PALETTE_SIZE = 256

FORMATS = {
    "jasc": "JASC-PAL (*.pal)",
    "riff": "Microsoft RIFF PAL (*.pal)",
    "act": "Adobe ACT (*.act)",
    "gpl": "GIMP (*.gpl)",
    "png": "PNG -- próbnik 16×16 (*.png)",
}


class PaletteError(Exception):
    pass


def _clamp_entries(entries: Sequence[Sequence[int]]) -> List[RGB]:
    out = []
    for e in entries[:PALETTE_SIZE]:
        r, g, b = (int(v) for v in e[:3])
        if not all(0 <= v <= 255 for v in (r, g, b)):
            raise PaletteError(f"colour value out of range 0..255: {(r, g, b)}")
        out.append((r, g, b))
    if not out:
        raise PaletteError("palette has no colours")
    return out


# ---------------------------------------------------------------- import
def load_palette(path) -> List[RGB]:
    """Read a palette; returns 1..256 colours (a shorter palette replaces
    only the first entries)."""
    ext = os.path.splitext(str(path))[1].lower()
    with open(path, "rb") as fh:
        data = fh.read()
    if data[:4] == b"STCI":
        return _from_sti(data)
    if data[:4] == b"RIFF" and data[8:12] == b"PAL ":
        return _from_riff(data)
    head = data[:16].lstrip(b"\xef\xbb\xbf")
    if head.startswith(b"JASC-PAL"):
        return _from_jasc(data)
    if head.startswith(b"GIMP Palette"):
        return _from_gpl(data)
    if ext == ".act" or len(data) in (768, 772):
        return _from_act(data)
    try:
        from PIL import Image
        with Image.open(path) as img:
            if img.mode != "P":
                raise PaletteError("image is not palettised (mode P) -- no palette to import")
            pal = img.getpalette() or []
    except PaletteError:
        raise
    except Exception as exc:  # noqa: BLE001 -- anything Pillow can't read
        raise PaletteError(f"unrecognised palette format: {exc}") from None
    return _clamp_entries([pal[i:i + 3] for i in range(0, len(pal) - 2, 3)])


def _from_sti(data: bytes) -> List[RGB]:
    import sti
    s = sti.STIFile.from_bytes(data)
    if not s.is_indexed:
        raise PaletteError("this STI file is RGB and has no palette")
    return list(s.palette[:s.number_of_colours])


def _from_riff(data: bytes) -> List[RGB]:
    pos = 12
    while pos + 8 <= len(data):
        cid, size = data[pos:pos + 4], struct.unpack_from("<I", data, pos + 4)[0]
        if cid == b"data":
            _version, count = struct.unpack_from("<HH", data, pos + 8)
            base = pos + 12
            if base + count * 4 > len(data):
                raise PaletteError("RIFF palette is truncated")
            return _clamp_entries([data[base + i * 4:base + i * 4 + 3] for i in range(count)])
        pos += 8 + size + (size & 1)
    raise PaletteError("RIFF palette has no 'data' chunk")


def _text_lines(data: bytes) -> List[str]:
    return data.decode("utf-8-sig", errors="replace").splitlines()


def _from_jasc(data: bytes) -> List[RGB]:
    lines = [l.strip() for l in _text_lines(data) if l.strip()]
    if len(lines) < 3:
        raise PaletteError("JASC-PAL file is too short")
    try:
        count = int(lines[2])
        entries = [list(map(int, l.split()[:3])) for l in lines[3:3 + count]]
    except ValueError as exc:
        raise PaletteError(f"invalid JASC-PAL line: {exc}") from None
    if len(entries) < count:
        raise PaletteError("JASC-PAL file has fewer colours than declared")
    return _clamp_entries(entries)


def _from_gpl(data: bytes) -> List[RGB]:
    entries = []
    for line in _text_lines(data)[1:]:
        s = line.strip()
        if not s or s.startswith("#") or ":" in s.split()[0]:
            continue  # comments, "Name: ...", "Columns: ..."
        parts = s.split()
        try:
            entries.append([int(p) for p in parts[:3]])
        except ValueError:
            continue
    return _clamp_entries(entries)


def _from_act(data: bytes) -> List[RGB]:
    if len(data) < 768:
        raise PaletteError("ACT palette must be at least 768 bytes")
    count = PALETTE_SIZE
    if len(data) >= 772:
        n = struct.unpack_from(">H", data, 768)[0]
        if 0 < n <= PALETTE_SIZE:
            count = n
    return _clamp_entries([data[i * 3:i * 3 + 3] for i in range(count)])


# ---------------------------------------------------------------- export
def save_palette(path, palette: Sequence[RGB], fmt: str | None = None) -> str:
    """Write the palette; fmt is one of FORMATS (default: from extension,
    '.pal' -> JASC-PAL). Returns the format used."""
    if fmt is None:
        fmt = guess_format(path)
    pal = list(palette[:PALETTE_SIZE])
    if fmt == "jasc":
        text = "JASC-PAL\r\n0100\r\n%d\r\n" % len(pal) + "".join("%d %d %d\r\n" % c for c in pal)
        data = text.encode("ascii")
    elif fmt == "riff":
        body = struct.pack("<HH", 0x0300, len(pal)) + b"".join(bytes((r, g, b, 0)) for r, g, b in pal)
        chunk = b"data" + struct.pack("<I", len(body)) + body
        data = b"RIFF" + struct.pack("<I", 4 + len(chunk)) + b"PAL " + chunk
    elif fmt == "act":
        raw = b"".join(bytes(c) for c in pal) + bytes(3 * (PALETTE_SIZE - len(pal)))
        data = raw + struct.pack(">HH", len(pal), 0xFFFF)
    elif fmt == "gpl":
        name = os.path.splitext(os.path.basename(str(path)))[0]
        lines = ["GIMP Palette", f"Name: {name}", "Columns: 16", "#"]
        lines += ["%3d %3d %3d\tindex %d" % (r, g, b, i) for i, (r, g, b) in enumerate(pal)]
        data = ("\n".join(lines) + "\n").encode("utf-8")
    elif fmt == "png":
        swatch_image(pal).save(path, "PNG")
        return fmt
    else:
        raise PaletteError(f"unknown palette format '{fmt}'")
    with open(path, "wb") as fh:
        fh.write(data)
    return fmt


def guess_format(path) -> str:
    ext = os.path.splitext(str(path))[1].lower()
    return {".act": "act", ".gpl": "gpl", ".png": "png"}.get(ext, "jasc")


def swatch_image(palette: Sequence[RGB], cell: int = 16):
    """16x16 grid, one cell per palette index (index = row * 16 + column).
    Saved as a palettised PNG, so it can be imported back exactly."""
    from PIL import Image
    pal = list(palette[:PALETTE_SIZE])
    img = Image.new("P", (16 * cell, 16 * cell), 0)
    flat = [v for c in pal for v in c]
    img.putpalette(flat + [0] * (768 - len(flat)))
    px = img.load()
    for i in range(len(pal)):
        x0, y0 = (i % 16) * cell, (i // 16) * cell
        for y in range(y0, y0 + cell):
            for x in range(x0, x0 + cell):
                px[x, y] = i
    return img


def apply_palette(target: List[RGB], new: Sequence[RGB]) -> int:
    """Replace the first len(new) entries of target in place; returns how
    many colours actually changed."""
    changed = 0
    for i, c in enumerate(new[:len(target)]):
        if target[i] != tuple(c):
            target[i] = tuple(c)
            changed += 1
    return changed
