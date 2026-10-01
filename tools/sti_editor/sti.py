"""Reader/writer for Sir-Tech's Crazy Image (STCI, ``.sti``) files.

The layout mirrors the engine's own definitions -- see ``src/sgp/ImgFmt.h``
(STCIHeader / STCISubImage / STCIPaletteElement), ``src/sgp/STCI.cc`` (the
loader), ``src/sgp/HImage.h`` (AuxObjectData) and
``src/game/Utils/STIConvert.cc`` (the ETRLE writer). The ETRLE decoder follows
the blitters in ``src/sgp/VObject_Blitters.cc``.

File layout (all little-endian, C struct alignment):

    STCIHeader              64 bytes
    palette                 uiNumberOfColours * 3 bytes   (STCI_INDEXED only)
    STCISubImage[]          usNumberOfSubImages * 16 bytes (STCI_ETRLE_COMPRESSED only)
    image data              uiStoredSize bytes
    application data        uiAppDataSize bytes (AuxObjectData[] -- 16 bytes per subimage)

Transparency: ETRLE stores it as runs, not as a palette index. A pixel is
either inside a transparent run or inside a literal run; a literal pixel is
always drawn with its palette colour, even palette index 0. The engine's own
writer treats index 0 as transparent (TCI == 0), so in files it wrote the two
notions coincide. This module keeps an explicit per-pixel mask so that files
written by other tools (with opaque index 0 pixels) survive a round trip.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from typing import List, Optional, Sequence, Tuple

# fFlags (ImgFmt.h)
STCI_TRANSPARENT = 0x0001
STCI_ALPHA = 0x0002
STCI_RGB = 0x0004
STCI_INDEXED = 0x0008
STCI_ZLIB_COMPRESSED = 0x0010
STCI_ETRLE_COMPRESSED = 0x0020

FLAG_NAMES = [
    (STCI_TRANSPARENT, "TRANSPARENT"),
    (STCI_ALPHA, "ALPHA"),
    (STCI_RGB, "RGB"),
    (STCI_INDEXED, "INDEXED"),
    (STCI_ZLIB_COMPRESSED, "ZLIB"),
    (STCI_ETRLE_COMPRESSED, "ETRLE"),
]

# ETRLE run bytes (ImgFmt.h)
COMPRESS_TRANSPARENT = 0x80
COMPRESS_RUN_LIMIT = 0x7F

# AuxObjectData.fFlags (HImage.h)
AUX_FULL_TILE = 0x01
AUX_ANIMATED_TILE = 0x02
AUX_DYNAMIC_TILE = 0x04
AUX_INTERACTIVE_TILE = 0x08
AUX_IGNORES_HEIGHT = 0x10
AUX_USES_LAND_Z = 0x20

AUX_FLAG_NAMES = [
    (AUX_FULL_TILE, "FULL_TILE"),
    (AUX_ANIMATED_TILE, "ANIMATED_TILE"),
    (AUX_DYNAMIC_TILE, "DYNAMIC_TILE"),
    (AUX_INTERACTIVE_TILE, "INTERACTIVE_TILE"),
    (AUX_IGNORES_HEIGHT, "IGNORES_HEIGHT"),
    (AUX_USES_LAND_Z, "USES_LAND_Z"),
]

STCI_ID = b"STCI"
PALETTE_SIZE = 256

# STCIHeader: cID, uiOriginalSize, uiStoredSize, uiTransparentValue, fFlags,
# usHeight, usWidth, <20-byte union>, ubDepth, <3 padding bytes>,
# uiAppDataSize, cUnused[12]. The padding bytes are kept verbatim.
HEADER = struct.Struct("<4sIIIIHH20sB3sI12s")
# union, STCI_INDEXED variant: uiNumberOfColours, usNumberOfSubImages,
# ubRed/Green/BlueDepth, cIndexedUnused[11]
HEADER_INDEXED = struct.Struct("<IHBBB11s")
# union, STCI_RGB variant: uiRed/Green/Blue/AlphaMask, ubRed/Green/Blue/AlphaDepth
HEADER_RGB = struct.Struct("<IIIIBBBB")
# STCISubImage: uiDataOffset, uiDataLength, sOffsetX, sOffsetY, usHeight, usWidth
SUBIMAGE = struct.Struct("<IIhhHH")
# AuxObjectData: ubWallOrientation, ubNumberOfTiles, usTileLocIndex,
# ubUnused1[3], ubCurrentFrame, ubNumberOfFrames, fFlags, ubUnused[6]
AUX = struct.Struct("<BBH3sBBB6s")

assert HEADER.size == 64 and HEADER_INDEXED.size == 20 and HEADER_RGB.size == 20
assert SUBIMAGE.size == 16 and AUX.size == 16


class STIError(Exception):
    pass


def flag_names(flags: int, table=FLAG_NAMES) -> str:
    names = [name for bit, name in table if flags & bit]
    unknown = flags & ~sum(bit for bit, _ in table)
    if unknown:
        names.append(hex(unknown))
    return "|".join(names) if names else "0"


# --------------------------------------------------------------------------
# ETRLE
# --------------------------------------------------------------------------

def etrle_decode(data: bytes, width: int, height: int) -> Tuple[bytearray, bytearray, List[str]]:
    """Decode one ETRLE subimage.

    Returns (indices, mask, warnings); mask is 255 for opaque, 0 for
    transparent pixels. Each scanline is a series of run bytes ended by a
    0 byte: bit 7 set = that many transparent pixels, otherwise that many
    literal palette indices follow.
    """
    pixels = bytearray(width * height)
    mask = bytearray(width * height)
    warnings: List[str] = []
    pos = 0
    n = len(data)
    for y in range(height):
        x = 0
        row = y * width
        while True:
            if pos >= n:
                raise STIError(f"ETRLE data ends inside scanline {y}")
            b = data[pos]
            pos += 1
            if b == 0:
                break
            count = b & COMPRESS_RUN_LIMIT
            if x + count > width:
                raise STIError(f"ETRLE run overflows scanline {y} (x={x}, run={count}, width={width})")
            if b & COMPRESS_TRANSPARENT:
                x += count
            else:
                if pos + count > n:
                    raise STIError(f"ETRLE literal run past end of data in scanline {y}")
                pixels[row + x:row + x + count] = data[pos:pos + count]
                mask[row + x:row + x + count] = b"\xff" * count
                pos += count
                x += count
        if x != width:
            warnings.append(f"scanline {y} covers {x} of {width} pixels")
    if pos != n:
        warnings.append(f"{n - pos} unused byte(s) after the last scanline")
    return pixels, mask, warnings


def etrle_encode(pixels: Sequence[int], mask: Sequence[int], width: int, height: int) -> bytes:
    """Encode one subimage the way STIConvert.cc's ETRLECompress() does,
    except that runs follow the mask instead of 'index == 0'. For images
    whose transparent pixels are exactly the index 0 pixels the output is
    identical to the engine's writer."""
    out = bytearray()
    for y in range(height):
        row = y * width
        x = 0
        while x < width:
            opaque = mask[row + x] != 0
            start = x
            while x < width and (mask[row + x] != 0) == opaque and x - start < COMPRESS_RUN_LIMIT:
                x += 1
            count = x - start
            if opaque:
                out.append(count)
                out += bytes(pixels[row + start:row + x])
            else:
                out.append(count | COMPRESS_TRANSPARENT)
        out.append(0)
    return bytes(out)


# --------------------------------------------------------------------------
# Data model
# --------------------------------------------------------------------------

@dataclass
class AuxData:
    """AuxObjectData (HImage.h) -- per-subimage application data used by
    tilesets/cursors (animation frame counts, wall orientation, ...)."""
    wall_orientation: int = 0
    number_of_tiles: int = 0
    tile_loc_index: int = 0
    unused1: bytes = b"\0" * 3
    current_frame: int = 0
    number_of_frames: int = 0
    flags: int = 0
    unused: bytes = b"\0" * 6

    @classmethod
    def unpack(cls, raw: bytes) -> "AuxData":
        return cls(*AUX.unpack(raw))

    def pack(self) -> bytes:
        return AUX.pack(self.wall_orientation, self.number_of_tiles, self.tile_loc_index,
            self.unused1, self.current_frame, self.number_of_frames, self.flags, self.unused)

    def copy(self) -> "AuxData":
        return AuxData.unpack(self.pack())


class Frame:
    """One subimage ("page"/"frame") of an STI file.

    For ETRLE files the compressed bytes read from disk are kept until the
    frame's pixels are modified, so untouched frames are written back
    byte-for-byte.
    """

    def __init__(self, width: int, height: int, offset_x: int = 0, offset_y: int = 0,
            pixels: Optional[bytearray] = None, mask: Optional[bytearray] = None):
        self.width = width
        self.height = height
        self.offset_x = offset_x
        self.offset_y = offset_y
        self._pixels = pixels if pixels is not None else bytearray(width * height)
        self._mask = mask if mask is not None else bytearray(width * height)
        self._raw: Optional[bytes] = None
        self.orig_data_offset: Optional[int] = None
        self.decode_warnings: List[str] = []
        self.aux: Optional[AuxData] = None

    @classmethod
    def from_etrle(cls, raw: bytes, width: int, height: int, offset_x: int, offset_y: int,
            data_offset: Optional[int] = None) -> "Frame":
        f = cls.__new__(cls)
        f.width, f.height, f.offset_x, f.offset_y = width, height, offset_x, offset_y
        f._pixels = None
        f._mask = None
        f._raw = bytes(raw)
        f.orig_data_offset = data_offset
        f.decode_warnings = []
        f.aux = None
        return f

    # -- lazy decode -------------------------------------------------------
    def _ensure_decoded(self) -> None:
        if self._pixels is None:
            self._pixels, self._mask, self.decode_warnings = etrle_decode(self._raw, self.width, self.height)

    @property
    def pixels(self) -> bytearray:
        self._ensure_decoded()
        return self._pixels

    @property
    def mask(self) -> bytearray:
        self._ensure_decoded()
        return self._mask

    @property
    def is_pristine(self) -> bool:
        """True while the original compressed bytes are still valid."""
        return self._raw is not None

    def mark_modified(self) -> None:
        self._ensure_decoded()
        self._raw = None

    def encoded(self) -> bytes:
        if self._raw is not None:
            return self._raw
        return etrle_encode(self._pixels, self._mask, self.width, self.height)

    # -- pixel access ------------------------------------------------------
    def get(self, x: int, y: int) -> Tuple[int, bool]:
        i = y * self.width + x
        return self.pixels[i], self.mask[i] != 0

    def set(self, x: int, y: int, index: Optional[int]) -> None:
        """Set a pixel; index None makes it transparent (stored as index 0,
        like STIConvert.cc's TCI)."""
        if not (0 <= x < self.width and 0 <= y < self.height):
            return
        self.mark_modified()
        i = y * self.width + x
        if index is None:
            self._pixels[i] = 0
            self._mask[i] = 0
        else:
            self._pixels[i] = index
            self._mask[i] = 255

    def replace_pixels(self, pixels: bytes, mask: bytes, width: Optional[int] = None, height: Optional[int] = None) -> None:
        if width is not None:
            self.width = width
        if height is not None:
            self.height = height
        if len(pixels) != self.width * self.height or len(mask) != self.width * self.height:
            raise STIError("pixel/mask buffer size does not match frame size")
        self._pixels = bytearray(pixels)
        self._mask = bytearray(mask)
        self._raw = None

    def paint(self, shape_mask: bytes, index: Optional[int]) -> int:
        """Set every pixel where shape_mask is non-zero to index (None =
        transparent). Returns the number of pixels changed."""
        self._ensure_decoded()
        px, mk = self._pixels, self._mask
        value = 0 if index is None else index
        alpha = 0 if index is None else 255
        changed = 0
        for i, m in enumerate(shape_mask):
            if m and (px[i] != value or mk[i] != alpha):
                px[i] = value
                mk[i] = alpha
                changed += 1
        if changed:
            self._raw = None
        return changed

    def restore_from(self, other: "Frame") -> None:
        """Copy another frame's state into this object (keeps identity, so
        undoing every edit makes the frame byte-exact pristine again)."""
        self.width, self.height = other.width, other.height
        self.offset_x, self.offset_y = other.offset_x, other.offset_y
        self._raw = other._raw
        self._pixels = bytearray(other._pixels) if other._pixels is not None else None
        self._mask = bytearray(other._mask) if other._mask is not None else None
        self.decode_warnings = list(other.decode_warnings)
        self.aux = other.aux.copy() if other.aux else None

    def copy(self) -> "Frame":
        f = Frame.__new__(Frame)
        f.width, f.height, f.offset_x, f.offset_y = self.width, self.height, self.offset_x, self.offset_y
        f._raw = self._raw
        f._pixels = bytearray(self._pixels) if self._pixels is not None else None
        f._mask = bytearray(self._mask) if self._mask is not None else None
        f.orig_data_offset = self.orig_data_offset
        f.decode_warnings = list(self.decode_warnings)
        f.aux = self.aux.copy() if self.aux else None
        return f

    # -- Pillow ------------------------------------------------------------
    def index_image(self):
        """Palette indices as a Pillow 'L' image."""
        from PIL import Image
        return Image.frombytes("L", (self.width, self.height), bytes(self.pixels))

    def mask_image(self):
        """Transparency mask as a Pillow 'L' image (255 = opaque)."""
        from PIL import Image
        return Image.frombytes("L", (self.width, self.height), bytes(self.mask))

    def to_rgba(self, palette: Sequence[Tuple[int, int, int]]):
        """Frame as a Pillow RGBA image; transparent pixels get alpha 0."""
        from PIL import Image
        img = Image.frombytes("P", (self.width, self.height), bytes(self.pixels))
        img.putpalette(palette_bytes(palette))
        rgba = img.convert("RGBA")
        rgba.putalpha(self.mask_image())
        return rgba

    def stats(self, palette: Optional[Sequence[Tuple[int, int, int]]] = None) -> dict:
        px, mk = self.pixels, self.mask
        opaque = [px[i] for i in range(len(px)) if mk[i]]
        hist = {}
        for v in opaque:
            hist[v] = hist.get(v, 0) + 1
        bbox = self.mask_image().getbbox()
        top = sorted(hist.items(), key=lambda kv: -kv[1])[:10]
        return {
            "size": (self.width, self.height),
            "offset": (self.offset_x, self.offset_y),
            "pixels": self.width * self.height,
            "opaque": len(opaque),
            "transparent": self.width * self.height - len(opaque),
            "opaque_bbox": bbox,
            "unique_indices": len(hist),
            "opaque_index0": hist.get(0, 0),
            "top_indices": [(i, c, tuple(palette[i]) if palette else None) for i, c in top],
        }


def palette_bytes(palette: Sequence[Tuple[int, int, int]]) -> bytes:
    out = bytearray()
    for r, g, b in palette:
        out += bytes((r, g, b))
    return bytes(out)


class RGBImage:
    """Uncompressed STCI_RGB image (single frame, usually 16 bpp 565).

    Read, preview and export only -- see README. The raw pixel data is
    kept and written back unchanged."""

    def __init__(self, width: int, height: int, depth: int, masks: Tuple[int, int, int, int], raw: bytes):
        self.width, self.height, self.depth, self.masks, self.raw = width, height, depth, masks, raw

    def to_rgba(self):
        from PIL import Image
        w, h = self.width, self.height
        if self.depth not in (16, 32) and self.depth != 24:
            raise STIError(f"unsupported RGB depth {self.depth}")
        bpp = self.depth // 8
        out = bytearray(w * h * 4)

        def chan(mask):
            if not mask:
                return 0, 0
            shift = (mask & -mask).bit_length() - 1
            return shift, mask >> shift

        chans = [chan(m) for m in self.masks]
        for i in range(w * h):
            v = int.from_bytes(self.raw[i * bpp:(i + 1) * bpp], "little")
            for c, (shift, maxv) in enumerate(chans[:3]):
                out[i * 4 + c] = ((v >> shift) & maxv) * 255 // maxv if maxv else 0
            if chans[3][1]:
                out[i * 4 + 3] = ((v >> chans[3][0]) & chans[3][1]) * 255 // chans[3][1]
            else:
                out[i * 4 + 3] = 255
        return Image.frombytes("RGBA", (w, h), bytes(out))


class STIFile:
    def __init__(self):
        self.original_size = 0
        self.stored_size = 0
        self.transparent_value = 0
        self.flags = STCI_INDEXED | STCI_ETRLE_COMPRESSED
        self.height = 0
        self.width = 0
        self.depth = 8
        self.app_data_size = 0
        self.header_pad = b"\0" * 3
        self.header_unused = b"\0" * 12
        # indexed union
        self.number_of_colours = PALETTE_SIZE
        self.red_depth = self.green_depth = self.blue_depth = 8
        self.indexed_unused = b"\0" * 11
        # rgb union
        self.rgb_masks = (0, 0, 0, 0)
        self.rgb_depths = (0, 0, 0, 0)
        self.union_raw = b"\0" * 20

        self.palette: List[Tuple[int, int, int]] = [(0, 0, 0)] * PALETTE_SIZE
        self.frames: List[Frame] = []
        self.rgb: Optional[RGBImage] = None
        self.app_data_raw = b""       # used only when app data is not AuxObjectData[]
        self.trailing = b""           # bytes after the declared end of the file
        self.warnings: List[str] = []

        # pristine-layout bookkeeping for byte-exact saves
        self._orig_data: Optional[bytes] = None
        self._orig_frames: Optional[list] = None

    # ----------------------------------------------------------------------
    @property
    def is_indexed(self) -> bool:
        return bool(self.flags & STCI_INDEXED) and not self.flags & STCI_RGB

    @property
    def is_etrle(self) -> bool:
        return bool(self.flags & STCI_ETRLE_COMPRESSED)

    @property
    def is_rgb(self) -> bool:
        return bool(self.flags & STCI_RGB)

    @property
    def editable(self) -> bool:
        return self.is_indexed and self.is_etrle

    @property
    def has_aux(self) -> bool:
        return any(f.aux is not None for f in self.frames)

    # ----------------------------------------------------------------------
    @classmethod
    def load(cls, path) -> "STIFile":
        with open(path, "rb") as fh:
            return cls.from_bytes(fh.read())

    @classmethod
    def from_bytes(cls, data: bytes) -> "STIFile":
        if len(data) < HEADER.size:
            raise STIError("file is shorter than the 64-byte STCI header")
        s = cls()
        (cid, s.original_size, s.stored_size, s.transparent_value, s.flags, s.height, s.width,
            s.union_raw, s.depth, s.header_pad, s.app_data_size, s.header_unused) = HEADER.unpack_from(data, 0)
        if cid != STCI_ID:
            raise STIError("STCI file has invalid header")  # same message as STCI.cc
        if s.flags & STCI_ZLIB_COMPRESSED:
            raise STIError("Cannot handle zlib compressed STCI files")  # the engine can't either
        if not s.flags & (STCI_RGB | STCI_INDEXED):
            raise STIError("Unknown data organization in STCI file.")
        pos = HEADER.size

        if s.is_rgb:
            s.rgb_masks = HEADER_RGB.unpack(s.union_raw)[:4]
            s.rgb_depths = HEADER_RGB.unpack(s.union_raw)[4:]
            raw = data[pos:pos + s.stored_size]
            if len(raw) != s.stored_size:
                raise STIError("image data is truncated")
            pos += s.stored_size
            s.rgb = RGBImage(s.width, s.height, s.depth, s.rgb_masks, raw)
        else:
            (s.number_of_colours, n_sub, s.red_depth, s.green_depth, s.blue_depth,
                s.indexed_unused) = HEADER_INDEXED.unpack(s.union_raw)
            pal_len = s.number_of_colours * 3
            if pos + pal_len > len(data):
                raise STIError("palette is truncated")
            pal = data[pos:pos + pal_len]
            pos += pal_len
            if s.number_of_colours != PALETTE_SIZE:
                s.warnings.append(f"palette has {s.number_of_colours} colours; the engine requires 256")
            s.palette = [tuple(pal[i * 3:i * 3 + 3]) for i in range(s.number_of_colours)]
            s.palette += [(0, 0, 0)] * (PALETTE_SIZE - len(s.palette))

            if s.is_etrle:
                subs = []
                for i in range(n_sub):
                    if pos + SUBIMAGE.size > len(data):
                        raise STIError("subimage table is truncated")
                    subs.append(SUBIMAGE.unpack_from(data, pos))
                    pos += SUBIMAGE.size
                blob = data[pos:pos + s.stored_size]
                if len(blob) != s.stored_size:
                    raise STIError("image data is truncated")
                pos += s.stored_size
                for i, (off, length, ox, oy, h, w) in enumerate(subs):
                    if off + length > len(blob):
                        raise STIError(f"subimage {i} data lies outside the image data")
                    s.frames.append(Frame.from_etrle(blob[off:off + length], w, h, ox, oy, off))
                s._orig_data = blob
                s._orig_frames = [(f, f.orig_data_offset, len(f._raw), f.width, f.height, f.offset_x, f.offset_y)
                    for f in s.frames]
            else:
                # Uncompressed indexed image: one frame, raw indices, no transparency runs.
                raw = data[pos:pos + s.stored_size]
                if len(raw) < s.width * s.height:
                    raise STIError("image data is truncated")
                pos += s.stored_size
                f = Frame(s.width, s.height, 0, 0, bytearray(raw[:s.width * s.height]),
                    bytearray(b"\xff" * (s.width * s.height)))
                s.frames.append(f)
                s._orig_data = raw

        app = data[pos:pos + s.app_data_size]
        if len(app) != s.app_data_size:
            raise STIError("application data is truncated")
        pos += s.app_data_size
        if s.app_data_size and s.frames and s.app_data_size == AUX.size * len(s.frames):
            for i, f in enumerate(s.frames):
                f.aux = AuxData.unpack(app[i * AUX.size:(i + 1) * AUX.size])
        else:
            s.app_data_raw = app
            if s.app_data_size:
                s.warnings.append(f"{s.app_data_size} bytes of application data kept as raw bytes")
        s.trailing = data[pos:]
        if s.trailing:
            s.warnings.append(f"{len(s.trailing)} byte(s) after the declared end of file (kept)")
        return s

    @classmethod
    def new_from_image(cls, img, alpha_threshold: int = 128) -> "STIFile":
        """Build a new single-frame indexed ETRLE STI from a Pillow image.

        A palettised ('P') image keeps its own palette and indices exactly;
        its transparent index/alpha (if any) becomes the ETRLE transparency.
        Any other image is quantised to 255 colours placed at indices 1..255,
        with index 0 left black and used for transparency (STIConvert.cc's TCI).
        """
        from PIL import Image
        s = cls()
        w, h = img.size
        s.width, s.height = w, h
        s.original_size = w * h
        if img.mode == "P":
            pal = img.getpalette() or []
            pal = (pal + [0] * 768)[:768]
            s.palette = [tuple(pal[i * 3:i * 3 + 3]) for i in range(PALETTE_SIZE)]
            pixels = bytearray(img.tobytes())
            trans = img.info.get("transparency")
            if isinstance(trans, int):
                mask = bytearray(0 if p == trans else 255 for p in pixels)
            elif isinstance(trans, (bytes, bytearray)):
                alpha = list(trans) + [255] * (PALETTE_SIZE - len(trans))
                mask = bytearray(255 if alpha[p] >= alpha_threshold else 0 for p in pixels)
            else:
                mask = bytearray(b"\xff" * (w * h))
            for i in range(w * h):
                if not mask[i]:
                    pixels[i] = 0
        else:
            rgba = img.convert("RGBA")
            alpha = rgba.getchannel("A").tobytes()
            q = rgba.convert("RGB").quantize(colors=255, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
            qpal = (q.getpalette() or []) + [0] * 768
            s.palette = [(0, 0, 0)] + [tuple(qpal[i * 3:i * 3 + 3]) for i in range(255)]
            qpx = q.tobytes()
            pixels = bytearray(w * h)
            mask = bytearray(w * h)
            for i in range(w * h):
                if alpha[i] >= alpha_threshold:
                    pixels[i] = qpx[i] + 1
                    mask[i] = 255
        s.frames = [Frame(w, h, 0, 0, pixels, mask)]
        return s

    # ----------------------------------------------------------------------
    def _layout_is_pristine(self) -> bool:
        if self._orig_frames is None or len(self._orig_frames) != len(self.frames):
            return False
        for f, (of, off, length, w, h, ox, oy) in zip(self.frames, self._orig_frames):
            if f is not of or not f.is_pristine or (f.width, f.height, f.offset_x, f.offset_y) != (w, h, ox, oy):
                return False
        return True

    def to_bytes(self) -> bytes:
        n = len(self.frames)
        pal_and_table = b""
        if self.is_rgb:
            body = self.rgb.raw
        elif self.is_etrle:
            if self._layout_is_pristine():
                body = self._orig_data
                table = [(off, length) for _, off, length, *_ in self._orig_frames]
            else:
                chunks, table, off = [], [], 0
                for f in self.frames:
                    enc = f.encoded()
                    table.append((off, len(enc)))
                    chunks.append(enc)
                    off += len(enc)
                body = b"".join(chunks)
                self._sync_single_frame_header()
            pal_and_table = palette_bytes(self.palette[:self.number_of_colours])
            for f, (off, length) in zip(self.frames, table):
                pal_and_table += SUBIMAGE.pack(off, length, f.offset_x, f.offset_y, f.height, f.width)
        else:
            f = self.frames[0]
            body = bytes(f.pixels) + self._orig_data[len(f.pixels):] if self._orig_data else bytes(f.pixels)
            pal_and_table = palette_bytes(self.palette[:self.number_of_colours])

        if self.has_aux:
            app = b"".join((f.aux or AuxData()).pack() for f in self.frames)
        else:
            app = self.app_data_raw
        self.stored_size = len(body)
        self.app_data_size = len(app)

        if self.is_rgb:
            union = self.union_raw
        else:
            union = HEADER_INDEXED.pack(self.number_of_colours, n if self.is_etrle else
                HEADER_INDEXED.unpack(self.union_raw)[1], self.red_depth, self.green_depth,
                self.blue_depth, self.indexed_unused)
        header = HEADER.pack(STCI_ID, self.original_size, self.stored_size, self.transparent_value,
            self.flags, self.height, self.width, union, self.depth, self.header_pad,
            self.app_data_size, self.header_unused)
        return header + pal_and_table + body + app + self.trailing

    def _sync_single_frame_header(self) -> None:
        """For single-frame files whose header described exactly that frame,
        keep usWidth/usHeight/uiOriginalSize in step with it (as
        STIConvert.cc's WriteSTIFile() writes them). Multi-frame headers are
        left alone -- they describe the source sheet, not any one frame."""
        if len(self.frames) != 1 or not self._orig_frames or len(self._orig_frames) != 1:
            return
        _, _, _, w0, h0, _, _ = self._orig_frames[0]
        if (self.width, self.height) == (w0, h0):
            f = self.frames[0]
            if self.original_size == w0 * h0 * (self.depth // 8):
                self.original_size = f.width * f.height * (self.depth // 8)
            self.width, self.height = f.width, f.height

    def save(self, path) -> None:
        data = self.to_bytes()
        with open(path, "wb") as fh:
            fh.write(data)

    # ----------------------------------------------------------------------
    def frame_rgba(self, index: int):
        if self.is_rgb:
            return self.rgb.to_rgba()
        return self.frames[index].to_rgba(self.palette)

    @property
    def frame_count(self) -> int:
        return 1 if self.is_rgb else len(self.frames)

    def frame_geometry(self, index: int) -> Tuple[int, int, int, int]:
        if self.is_rgb:
            return self.width, self.height, 0, 0
        f = self.frames[index]
        return f.width, f.height, f.offset_x, f.offset_y

    def export_png(self, index: int, path) -> None:
        self.frame_rgba(index).save(path, "PNG")

    def export_all_png(self, directory, stem: str) -> List[str]:
        import os
        os.makedirs(directory, exist_ok=True)
        paths = []
        digits = max(3, len(str(self.frame_count - 1)))
        for i in range(self.frame_count):
            p = os.path.join(directory, f"{stem}_{i:0{digits}d}.png")
            self.export_png(i, p)
            paths.append(p)
        return paths

    def describe(self) -> str:
        lines = [
            f"flags          : {flag_names(self.flags)} ({self.flags:#06x})",
            f"header size    : {self.width}x{self.height}, depth {self.depth} bpp",
            f"original/stored: {self.original_size} / {self.stored_size} bytes",
            f"transparent val: {self.transparent_value}",
        ]
        if self.is_rgb:
            lines.append("masks          : " + " ".join(f"{m:#06x}" for m in self.rgb_masks))
        else:
            lines.append(f"palette        : {self.number_of_colours} colours")
            lines.append(f"frames         : {len(self.frames)}")
        lines.append(f"app data       : {self.app_data_size} bytes" + (" (AuxObjectData per frame)" if self.has_aux else ""))
        for w in self.warnings:
            lines.append(f"warning        : {w}")
        return "\n".join(lines)


def nearest_palette_index(palette: Sequence[Tuple[int, int, int]], rgb: Tuple[int, int, int],
        skip_zero: bool = True, cache: Optional[dict] = None) -> int:
    if cache is not None and rgb in cache:
        return cache[rgb]
    best, best_d = 0, None
    r, g, b = rgb
    for i, (pr, pg, pb) in enumerate(palette):
        if skip_zero and i == 0:
            continue
        d = (pr - r) ** 2 + (pg - g) ** 2 + (pb - b) ** 2
        if best_d is None or d < best_d:
            best, best_d = i, d
            if d == 0:
                break
    if cache is not None:
        cache[rgb] = best
    return best


def image_to_frame_data(img, palette: Sequence[Tuple[int, int, int]], alpha_threshold: int = 128,
        skip_zero: bool = True) -> Tuple[bytearray, bytearray]:
    """Map a Pillow image onto the file's palette (nearest colour, exact
    matches preserved). Pixels with alpha < alpha_threshold become
    transparent. Palette index 0 is avoided for opaque pixels by default,
    because the engine's own writer treats index 0 as transparent."""
    rgba = img.convert("RGBA")
    w, h = rgba.size
    data = rgba.tobytes()
    pixels = bytearray(w * h)
    mask = bytearray(w * h)
    cache: dict = {}
    for i in range(w * h):
        r, g, b, a = data[i * 4:i * 4 + 4]
        if a < alpha_threshold:
            continue
        pixels[i] = nearest_palette_index(palette, (r, g, b), skip_zero, cache)
        mask[i] = 255
    return pixels, mask
