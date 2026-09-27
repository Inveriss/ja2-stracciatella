"""Drawing primitives for the STI editor.

Every function returns a shape mask (bytes, one byte per pixel, non-zero =
pixel belongs to the shape) for a frame of the given size. Shapes are
rendered with Pillow in '1'-bit mode, so there is no anti-aliasing: an
indexed image must never receive blended in-between palette indices.
"""

from __future__ import annotations

from collections import deque
from typing import Iterable, Optional, Sequence, Tuple

from PIL import Image, ImageDraw, ImageFont

Point = Tuple[int, int]


def _canvas(width: int, height: int):
    img = Image.new("1", (width, height), 0)
    draw = ImageDraw.Draw(img)
    draw.fontmode = "1"
    return img, draw


def _bytes(img) -> bytes:
    return img.convert("L").tobytes()


def _box(p0: Point, p1: Point):
    (x0, y0), (x1, y1) = p0, p1
    return [min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1)]


def brush_mask(width: int, height: int, points: Sequence[Point], size: int = 1) -> bytes:
    """Square brush of `size` pixels stamped along a polyline (pencil/eraser).
    Consecutive points are joined so fast mouse moves leave no gaps."""
    img, draw = _canvas(width, height)
    half = (size - 1) // 2
    pts = list(points)
    stamps = []
    segments = list(zip(pts, pts[1:])) if len(pts) > 1 else [(pts[0], pts[0])]
    for a, b in segments:
        stamps.extend(bresenham(a, b))
    for x, y in stamps:
        draw.rectangle([x - half, y - half, x - half + size - 1, y - half + size - 1], fill=1)
    return _bytes(img)


def bresenham(p0: Point, p1: Point) -> Iterable[Point]:
    x0, y0 = p0
    x1, y1 = p1
    dx, dy = abs(x1 - x0), -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        yield x0, y0
        if x0 == x1 and y0 == y1:
            return
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def line_mask(width: int, height: int, p0: Point, p1: Point, size: int = 1) -> bytes:
    return brush_mask(width, height, [p0, p1], size)


def rect_mask(width: int, height: int, p0: Point, p1: Point, filled: bool = False) -> bytes:
    img, draw = _canvas(width, height)
    draw.rectangle(_box(p0, p1), outline=1, fill=1 if filled else None)
    return _bytes(img)


def ellipse_mask(width: int, height: int, p0: Point, p1: Point, filled: bool = False) -> bytes:
    img, draw = _canvas(width, height)
    draw.ellipse(_box(p0, p1), outline=1, fill=1 if filled else None)
    return _bytes(img)


def text_mask(width: int, height: int, origin: Point, text: str, size: int = 12,
        font_path: Optional[str] = None) -> bytes:
    img, draw = _canvas(width, height)
    try:
        font = ImageFont.truetype(font_path, size) if font_path else ImageFont.load_default(size=size)
    except (OSError, TypeError, AttributeError):
        font = ImageFont.load_default()
    draw.text(origin, text, fill=1, font=font)
    return _bytes(img)


def flood_fill_mask(pixels: Sequence[int], mask: Sequence[int], width: int, height: int,
        start: Point) -> bytes:
    """4-connected region of pixels equal to the start pixel -- same palette
    index and same transparency (all transparent pixels count as equal)."""
    sx, sy = start
    out = bytearray(width * height)
    if not (0 <= sx < width and 0 <= sy < height):
        return bytes(out)

    def key(i):
        return (pixels[i], True) if mask[i] else (None, False)

    target = key(sy * width + sx)
    queue = deque([(sx, sy)])
    out[sy * width + sx] = 1
    while queue:
        x, y = queue.popleft()
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < width and 0 <= ny < height:
                i = ny * width + nx
                if not out[i] and key(i) == target:
                    out[i] = 1
                    queue.append((nx, ny))
    return bytes(out)
