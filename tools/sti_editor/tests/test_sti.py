"""Tests for the STI reader/writer, drawing helpers and editor.

Run from the repository root:
    py -3 -m unittest discover -s tools/sti_editor/tests -v

The real-file tests read every .sti under assets/ and never write to it;
a checksum guard verifies that at the end.
"""

import hashlib
import io
import os
import random
import struct
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
TOOL_DIR = os.path.dirname(HERE)
REPO_ROOT = os.path.abspath(os.path.join(TOOL_DIR, "..", ".."))
sys.path.insert(0, TOOL_DIR)

from PIL import Image  # noqa: E402

import drawing  # noqa: E402
import palette_io  # noqa: E402
import sti  # noqa: E402
import sti_tool  # noqa: E402


def repo_sti_files():
    root = os.path.join(REPO_ROOT, "assets")
    found = []
    for dirpath, _, names in os.walk(root):
        for n in names:
            if n.lower().endswith(".sti"):
                found.append(os.path.join(dirpath, n))
    return sorted(found)


REAL_FILES = repo_sti_files()


def read_bytes(path):
    with open(path, "rb") as fh:
        return fh.read()


def sha256(path):
    return hashlib.sha256(read_bytes(path)).hexdigest()


def ja2_etrle_compress_row(src):
    """Straight port of STIConvert.cc ETRLECompress() (TCI == 0)."""
    out = bytearray()
    loc = 0
    n = len(src)
    while loc < n:
        cur, length = loc, 0
        if src[loc] == 0:
            while True:
                cur += 1
                length += 1
                if not (cur < n and src[cur] == 0 and length < 0x7F):
                    break
            out.append(length | 0x80)
        else:
            while True:
                cur += 1
                length += 1
                if not (cur < n and src[cur] != 0 and length < 0x7F):
                    break
            out.append(length)
            out += bytes(src[loc:loc + length])
        loc += length
    out.append(0)
    return bytes(out)


def make_file(width=8, height=4, frames=2, aux=False):
    img = Image.new("P", (width, height), 0)
    img.putpalette([i % 256 for i in range(768)])
    s = sti.STIFile.new_from_image(img)
    base = s.frames[0]
    s.frames = []
    for k in range(frames):
        f = sti.Frame(width, height, k, -k)
        for y in range(height):
            for x in range(width):
                if (x + y + k) % 3:
                    f.set(x, y, 1 + (x * 7 + y + k) % 250)
        if aux:
            f.aux = sti.AuxData(number_of_frames=frames if k == 0 else 0, current_frame=k,
                flags=sti.AUX_ANIMATED_TILE, tile_loc_index=300 + k)
        s.frames.append(f)
    del base
    return s


def rgb_file_bytes(width, height, pixels565):
    union = sti.HEADER_RGB.pack(0xF800, 0x07E0, 0x001F, 0, 5, 6, 5, 0)
    data = b"".join(struct.pack("<H", p) for p in pixels565)
    header = sti.HEADER.pack(sti.STCI_ID, len(data), len(data), 0, sti.STCI_RGB, height, width,
        union, 16, b"\0\0\0", 0, b"\0" * 12)
    return header + data


class FormatTests(unittest.TestCase):
    def test_struct_sizes_match_engine(self):
        # STCI.cc unit test: sizeof(STCIHeader) == 64, STCISubImage == 16, palette entry == 3
        self.assertEqual(sti.HEADER.size, 64)
        self.assertEqual(sti.SUBIMAGE.size, 16)
        self.assertEqual(sti.AUX.size, 16)

    def test_decode_known_bytes(self):
        # width 5: 2 transparent, literal [7, 8], 1 transparent, end of line
        px, mk, warn = sti.etrle_decode(bytes([0x82, 0x02, 7, 8, 0x81, 0x00]), 5, 1)
        self.assertEqual(list(px), [0, 0, 7, 8, 0])
        self.assertEqual(list(mk), [0, 0, 255, 255, 0])
        self.assertEqual(warn, [])

    def test_decode_errors(self):
        with self.assertRaises(sti.STIError):
            sti.etrle_decode(bytes([0x03, 1, 2, 3]), 3, 1)          # missing end-of-line
        with self.assertRaises(sti.STIError):
            sti.etrle_decode(bytes([0x85, 0x00]), 3, 1)             # run past the width

    def test_encoder_matches_engine_writer(self):
        rnd = random.Random(1)
        for _ in range(200):
            w = rnd.randint(1, 300)
            row = bytes(rnd.choice([0, 0, 0, rnd.randint(1, 255)]) for _ in range(w))
            mask = bytes(255 if v else 0 for v in row)
            self.assertEqual(sti.etrle_encode(row, mask, w, 1), ja2_etrle_compress_row(row))

    def test_long_runs_split_at_127(self):
        w = 300
        px = bytes([5] * 200 + [0] * 100)
        mk = bytes([255] * 200 + [0] * 100)
        enc = sti.etrle_encode(px, mk, w, 1)
        self.assertEqual(enc[0], 127)
        p2, m2, _ = sti.etrle_decode(enc, w, 1)
        self.assertEqual((bytes(p2), bytes(m2)), (px, mk))

    def test_opaque_index_zero_survives(self):
        px = bytes([0, 3, 0, 0])
        mk = bytes([255, 255, 0, 255])
        enc = sti.etrle_encode(px, mk, 4, 1)
        p2, m2, _ = sti.etrle_decode(enc, 4, 1)
        self.assertEqual((bytes(p2), bytes(m2)), (px, mk))

    def test_rejects_invalid_files(self):
        with self.assertRaises(sti.STIError):
            sti.STIFile.from_bytes(b"XXXX" + bytes(60))
        s = make_file()
        data = bytearray(s.to_bytes())
        struct.pack_into("<I", data, 16, sti.STCI_INDEXED | sti.STCI_ZLIB_COMPRESSED)
        with self.assertRaises(sti.STIError):
            sti.STIFile.from_bytes(bytes(data))

    def test_synthetic_roundtrip_with_aux(self):
        s = make_file(frames=3, aux=True)
        data = s.to_bytes()
        s2 = sti.STIFile.from_bytes(data)
        self.assertEqual(s2.to_bytes(), data)
        self.assertEqual(s2.app_data_size, 3 * 16)
        self.assertEqual([f.aux.pack() for f in s2.frames], [f.aux.pack() for f in s.frames])
        self.assertEqual([(f.offset_x, f.offset_y) for f in s2.frames], [(0, 0), (1, -1), (2, -2)])
        s2.frames[1].aux.number_of_frames = 9
        s3 = sti.STIFile.from_bytes(s2.to_bytes())
        self.assertEqual(s3.frames[1].aux.number_of_frames, 9)
        self.assertEqual(s3.frames[1].aux.tile_loc_index, 301)

    def test_rgb_file_view_and_roundtrip(self):
        data = rgb_file_bytes(2, 1, [0xF800, 0x07E0])
        s = sti.STIFile.from_bytes(data)
        self.assertTrue(s.is_rgb)
        self.assertFalse(s.editable)
        self.assertEqual(s.to_bytes(), data)
        img = s.frame_rgba(0)
        self.assertEqual(img.getpixel((0, 0)), (255, 0, 0, 255))
        self.assertEqual(img.getpixel((1, 0)), (0, 255, 0, 255))

    def test_uncompressed_indexed_roundtrip(self):
        pal = bytes(range(256)) * 3
        px = bytes([1, 2, 3, 4, 5, 6])
        union = sti.HEADER_INDEXED.pack(256, 0, 8, 8, 8, b"\0" * 11)
        header = sti.HEADER.pack(sti.STCI_ID, 6, 6, 0, sti.STCI_INDEXED, 2, 3, union, 8, b"\0" * 3, 0, b"\0" * 12)
        data = header + pal + px
        s = sti.STIFile.from_bytes(data)
        self.assertEqual(bytes(s.frames[0].pixels), px)
        self.assertEqual(s.to_bytes(), data)

    def test_single_frame_header_follows_frame_size(self):
        img = Image.new("RGBA", (4, 3), (255, 0, 0, 255))
        s = sti.STIFile.from_bytes(sti.STIFile.new_from_image(img).to_bytes())
        f = s.frames[0]
        f.replace_pixels(bytes(10 * 2), bytes([255] * 20), 10, 2)
        s2 = sti.STIFile.from_bytes(s.to_bytes())
        self.assertEqual((s2.width, s2.height, s2.original_size), (10, 2, 20))


class ImageTests(unittest.TestCase):
    def test_export_png_matches_pixels_and_mask(self):
        s = make_file(width=6, height=5, frames=2)
        with tempfile.TemporaryDirectory() as tmp:
            paths = s.export_all_png(tmp, "test")
            self.assertEqual(len(paths), 2)
            for i, p in enumerate(paths):
                img = Image.open(p)
                self.assertEqual(img.mode, "RGBA")
                self.assertEqual(img.size, (6, 5))
                f = s.frames[i]
                for y in range(5):
                    for x in range(6):
                        idx, opaque = f.get(x, y)
                        r, g, b, a = img.getpixel((x, y))
                        self.assertEqual(a, 255 if opaque else 0)
                        if opaque:
                            self.assertEqual((r, g, b), s.palette[idx])

    def test_export_indexed_sheet_keeps_indices_and_frames(self):
        s = make_file(width=6, height=5, frames=3)
        s.frames[1].offset_x, s.frames[1].offset_y = -4, 7
        s.frames[2].set(0, 0, 0)                       # opaque index 0 -> warning
        s.frames[2].set(1, 0, 254)
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "sheet.png")
            json_path, warnings = s.export_indexed_sheet(out, max_width=13)
            self.assertEqual(json_path, out + ".json")
            self.assertEqual(len(warnings), 1)
            self.assertIn("frame 2", warnings[0])

            import json
            with open(json_path, encoding="utf-8") as fh:
                frames = json.load(fh)["frames"]
            # two frames fit into 13 pixels (6 + 1 + 6), the third wraps
            self.assertEqual([(f["x"], f["y"]) for f in frames], [(0, 0), (7, 0), (0, 6)])
            self.assertEqual((frames[1]["offsetX"], frames[1]["offsetY"]), (-4, 7))

            img = Image.open(out)
            self.assertEqual(img.mode, "P")
            self.assertEqual(img.size, (13, 11))
            self.assertEqual(img.info.get("transparency"), 0)
            pal = img.getpalette()
            self.assertEqual([tuple(pal[i * 3:i * 3 + 3]) for i in range(256)], s.palette)
            for f, meta in zip(s.frames, frames):
                for y in range(f.height):
                    for x in range(f.width):
                        idx, opaque = f.get(x, y)
                        self.assertEqual(img.getpixel((meta["x"] + x, meta["y"] + y)), idx if opaque else 0)

    def test_export_indexed_sheet_single_frame_has_no_metadata(self):
        s = make_file(width=4, height=3, frames=1)
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "one.png")
            json_path, _ = s.export_indexed_sheet(out)
            self.assertIsNone(json_path)
            self.assertFalse(os.path.exists(out + ".json"))
            self.assertEqual(Image.open(out).size, (4, 3))

    def test_new_from_palette_image_keeps_indices(self):
        img = Image.new("P", (3, 1))
        img.putpalette([0, 0, 0, 10, 20, 30, 40, 50, 60] + [0] * (768 - 9))
        img.putdata([0, 1, 2])
        img.info["transparency"] = 0
        s = sti.STIFile.new_from_image(img)
        f = s.frames[0]
        self.assertEqual(list(f.pixels), [0, 1, 2])
        self.assertEqual(list(f.mask), [0, 255, 255])
        self.assertEqual(s.palette[2], (40, 50, 60))

    def test_new_from_rgba_reserves_index_zero(self):
        img = Image.new("RGBA", (4, 1))
        img.putdata([(0, 0, 0, 0), (0, 0, 0, 255), (200, 10, 10, 255), (10, 200, 10, 255)])
        s = sti.STIFile.new_from_image(img)
        f = s.frames[0]
        self.assertEqual(f.mask[0], 0)
        self.assertTrue(all(f.pixels[i] != 0 for i in range(1, 4)))
        rgba = s.frame_rgba(0)
        self.assertEqual(rgba.getpixel((2, 0)), (200, 10, 10, 255))

    def test_import_maps_exact_palette_colours(self):
        pal = [(i, 255 - i, i // 2) for i in range(256)]
        img = Image.new("RGBA", (3, 1))
        img.putdata([pal[5] + (255,), pal[77] + (255,), (0, 0, 0, 0)])
        px, mk = sti.image_to_frame_data(img, pal)
        self.assertEqual(list(px[:2]), [5, 77])
        self.assertEqual(list(mk), [255, 255, 0])


class DrawingTests(unittest.TestCase):
    def test_line_rect_ellipse(self):
        m = drawing.line_mask(5, 5, (0, 0), (4, 4))
        self.assertEqual([i for i, v in enumerate(m) if v], [0, 6, 12, 18, 24])
        r = drawing.rect_mask(4, 4, (3, 3), (0, 0))
        self.assertEqual(sum(1 for v in r if v), 12)
        rf = drawing.rect_mask(4, 4, (0, 0), (3, 3), filled=True)
        self.assertEqual(sum(1 for v in rf if v), 16)
        e = drawing.ellipse_mask(9, 9, (0, 0), (8, 8), filled=True)
        self.assertTrue(e[4 * 9 + 4])
        self.assertFalse(e[0])

    def test_brush_joins_points(self):
        m = drawing.brush_mask(10, 1, [(0, 0), (9, 0)])
        self.assertTrue(all(m))

    def test_flood_fill_respects_index_and_transparency(self):
        px = bytes([1, 1, 2, 0, 0])
        mk = bytes([255, 255, 255, 0, 0])
        self.assertEqual(list(drawing.flood_fill_mask(px, mk, 5, 1, (0, 0))), [1, 1, 0, 0, 0])
        self.assertEqual(list(drawing.flood_fill_mask(px, mk, 5, 1, (4, 0))), [0, 0, 0, 1, 1])

    def test_text_is_not_antialiased(self):
        m = drawing.text_mask(60, 20, (1, 1), "Ab", 12)
        self.assertTrue(any(m))
        self.assertTrue(set(m) <= {0, 255})

    def test_paint_counts_changes(self):
        f = sti.Frame(3, 1)
        self.assertEqual(f.paint(bytes([1, 1, 0]), 9), 2)
        self.assertEqual(list(f.pixels), [9, 9, 0])
        self.assertEqual(f.paint(bytes([1, 0, 0]), None), 1)
        self.assertEqual(list(f.mask), [0, 255, 0])


class PaletteTests(unittest.TestCase):
    PAL = [(i, (i * 7) % 256, 255 - i) for i in range(256)]

    def test_export_import_every_format(self):
        with tempfile.TemporaryDirectory() as tmp:
            for fmt, ext in (("jasc", ".pal"), ("riff", ".pal"), ("act", ".act"), ("gpl", ".gpl"), ("png", ".png")):
                with self.subTest(fmt=fmt):
                    p = os.path.join(tmp, f"p_{fmt}{ext}")
                    self.assertEqual(palette_io.save_palette(p, self.PAL, fmt), fmt)
                    self.assertEqual(palette_io.load_palette(p), self.PAL)

    def test_format_detection_does_not_trust_extension(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = os.path.join(tmp, "riff_named.act")
            palette_io.save_palette(p, self.PAL, "riff")
            self.assertEqual(palette_io.load_palette(p), self.PAL)

    def test_known_riff_and_jasc_bytes(self):
        body = struct.pack("<HH", 0x0300, 2) + bytes([1, 2, 3, 0, 4, 5, 6, 0])
        riff = b"RIFF" + struct.pack("<I", 4 + 8 + len(body)) + b"PAL " + b"data" + struct.pack("<I", len(body)) + body
        jasc = b"JASC-PAL\r\n0100\r\n2\r\n1 2 3\r\n4 5 6\r\n"
        with tempfile.TemporaryDirectory() as tmp:
            for name, data in (("a.pal", riff), ("b.pal", jasc)):
                p = os.path.join(tmp, name)
                with open(p, "wb") as fh:
                    fh.write(data)
                self.assertEqual(palette_io.load_palette(p), [(1, 2, 3), (4, 5, 6)])

    def test_invalid_palettes_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            bad = os.path.join(tmp, "bad.pal")
            with open(bad, "wb") as fh:
                fh.write(b"JASC-PAL\r\n0100\r\n1\r\n300 0 0\r\n")
            with self.assertRaises(palette_io.PaletteError):
                palette_io.load_palette(bad)
            rgb = os.path.join(tmp, "rgb.png")
            Image.new("RGB", (2, 2)).save(rgb)
            with self.assertRaises(palette_io.PaletteError):
                palette_io.load_palette(rgb)

    def test_short_palette_replaces_only_first_entries(self):
        target = list(self.PAL)
        changed = palette_io.apply_palette(target, [(9, 9, 9), self.PAL[1]])
        self.assertEqual(changed, 1)
        self.assertEqual(target[0], (9, 9, 9))
        self.assertEqual(target[2:], self.PAL[2:])

    def test_import_recolours_without_touching_pixels(self):
        s = sti.STIFile.from_bytes(make_file(frames=2).to_bytes())
        raws = [f.encoded() for f in s.frames]
        palette_io.apply_palette(s.palette, self.PAL)
        s2 = sti.STIFile.from_bytes(s.to_bytes())
        self.assertEqual(s2.palette, self.PAL)
        self.assertEqual([f.encoded() for f in s2.frames], raws)
        idx, _ = s2.frames[0].get(1, 0)
        self.assertEqual(s2.frame_rgba(0).getpixel((1, 0))[:3], self.PAL[idx])

    @unittest.skipUnless(REAL_FILES, "no .sti files found under assets/")
    def test_palette_from_sti_file(self):
        self.assertEqual(palette_io.load_palette(REAL_FILES[0]), sti.STIFile.load(REAL_FILES[0]).palette)

    def test_cli_palette_roundtrip_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, "src.sti")
            make_file().save(src)
            pal = os.path.join(tmp, "x.gpl")
            palette_io.save_palette(pal, self.PAL)
            out = os.path.join(tmp, "out.sti")
            self.assertEqual(sti_tool.main(["palette-import", src, pal, out]), 0)
            self.assertEqual(sti.STIFile.load(out).palette, self.PAL)
            before = read_bytes(src)
            self.assertEqual(sti_tool.main(["palette-import", src, pal, src]), 1)
            self.assertEqual(read_bytes(src), before)
            exp = os.path.join(tmp, "e.pal")
            self.assertEqual(sti_tool.main(["palette-export", out, exp]), 0)
            self.assertEqual(palette_io.load_palette(exp), self.PAL)


@unittest.skipUnless(REAL_FILES, "no .sti files found under assets/")
class RealFileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hashes = {p: sha256(p) for p in REAL_FILES}

    @classmethod
    def tearDownClass(cls):
        for p, h in cls.hashes.items():
            assert sha256(p) == h, f"test modified {p}"

    def test_every_file_roundtrips(self):
        for p in REAL_FILES:
            with self.subTest(file=os.path.relpath(p, REPO_ROOT)):
                ok, msg = sti_tool.roundtrip_check(p)
                self.assertTrue(ok, msg)

    def test_reencode_is_byte_identical_to_original_frames(self):
        for p in REAL_FILES:
            s = sti.STIFile.load(p)
            bad = [i for i, f in enumerate(s.frames)
                if sti.etrle_encode(f.pixels, f.mask, f.width, f.height) != f.encoded()]
            self.assertEqual(bad, [], os.path.basename(p))

    def test_edit_one_frame_keeps_the_rest(self):
        multi = [p for p in REAL_FILES if 1 < len(sti.STIFile.load(p).frames) < 100]
        self.assertTrue(multi)
        path = multi[0]
        s = sti.STIFile.load(path)
        originals = [f.encoded() for f in s.frames]
        target = s.frames[1]
        idx, _ = target.get(0, 0)
        new_idx = 1 + (idx % 254)
        target.set(0, 0, new_idx)
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "edited.sti")
            s.save(out)
            s2 = sti.STIFile.load(out)
        self.assertEqual(s2.frames[1].get(0, 0), (new_idx, True))
        self.assertEqual(s2.palette, s.palette)
        for i, f in enumerate(s2.frames):
            if i != 1:
                self.assertEqual(f.encoded(), originals[i], f"frame {i} bytes changed")
                self.assertEqual((f.offset_x, f.offset_y), (s.frames[i].offset_x, s.frames[i].offset_y))

    def test_export_real_file(self):
        path = REAL_FILES[0]
        s = sti.STIFile.load(path)
        with tempfile.TemporaryDirectory() as tmp:
            paths = s.export_all_png(tmp, "real")
            self.assertEqual(len(paths), s.frame_count)
            img = Image.open(paths[0])
            self.assertEqual(img.size, (s.frames[0].width, s.frames[0].height))
            alpha = img.getchannel("A").tobytes()
            self.assertEqual(alpha, bytes(s.frames[0].mask))


class GuiSmokeTest(unittest.TestCase):
    def setUp(self):
        try:
            import tkinter as tk
            self.root = tk.Tk()
        except Exception as exc:  # noqa: BLE001 -- no display / no Tk
            self.skipTest(f"Tk unavailable: {exc}")
        self.root.withdraw()
        import sti_editor
        self.app = sti_editor.STIEditor(self.root)

    def tearDown(self):
        self.app.dirty = False
        self.root.destroy()

    @unittest.skipUnless(REAL_FILES, "no .sti files found under assets/")
    def test_edit_undo_redo_and_frames(self):
        path = next(p for p in REAL_FILES if len(sti.STIFile.load(p).frames) > 1)
        original = read_bytes(path)
        app = self.app
        app.open_file(path)
        self.assertEqual(app.frame_count, len(sti.STIFile.load(path).frames))
        app.goto_frame(1)
        self.assertEqual(app.frame_idx, 1)
        app.goto_frame(10**6)
        self.assertEqual(app.frame_idx, app.frame_count - 1)
        app.goto_frame(0)
        f = app.frame
        app.select_color(3)
        app._push_frame_undo()
        app._paint(drawing.rect_mask(f.width, f.height, (0, 0), (2, 2), filled=True))
        self.assertEqual(f.get(1, 1), (3, True))
        self.assertTrue(app.dirty)
        app.undo()
        self.assertEqual(app.sti.to_bytes(), original)     # undo restores byte-exact state
        app.redo()
        self.assertEqual(app.frame.get(1, 1), (3, True))
        app.tool.set("eraser")
        app._push_frame_undo()
        app._paint(drawing.brush_mask(f.width, f.height, [(1, 1)]))
        self.assertEqual(app.frame.get(1, 1), (0, False))
        app.duplicate_frame()
        self.assertEqual(app.frame_idx, 1)
        app.undo()
        app.undo()
        app.undo()
        self.assertEqual(app.sti.to_bytes(), original)
        for z in (1, 8, 32):
            app.zoom = z
            app.show_grid.set(True)
            app.render()
        app.show_mask.set(True)
        app.render()
        app.analyze_frame()
        with tempfile.TemporaryDirectory() as tmp:
            pal_path = os.path.join(tmp, "p.pal")
            app.export_palette(pal_path, "jasc")
            self.assertEqual(palette_io.load_palette(pal_path), app.sti.palette)
            inverted = [(255 - r, 255 - g, 255 - b) for r, g, b in app.sti.palette]
            palette_io.save_palette(pal_path, inverted, "riff")
            app.import_palette(pal_path)
            self.assertEqual(app.sti.palette, inverted)
            app.undo()
        self.assertEqual(app.sti.to_bytes(), original)
        self.assertEqual(read_bytes(path), original)


if __name__ == "__main__":
    unittest.main()
