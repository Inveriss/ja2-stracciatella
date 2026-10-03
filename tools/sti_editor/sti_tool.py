"""Command-line companion of the STI editor: info, export, analysis, round trip.

    py -3 tools/sti_editor/sti_tool.py info      FILE.sti [--frames]
    py -3 tools/sti_editor/sti_tool.py export    FILE.sti OUT_DIR [--frame N] [--mask]
    py -3 tools/sti_editor/sti_tool.py analyze   FILE.sti [--frame N]
    py -3 tools/sti_editor/sti_tool.py pixel     FILE.sti X Y [--frame N]
    py -3 tools/sti_editor/sti_tool.py roundtrip FILE.sti [...]
    py -3 tools/sti_editor/sti_tool.py palette-export FILE.sti OUT.pal [--format jasc|riff|act|gpl|png]
    py -3 tools/sti_editor/sti_tool.py palette-import FILE.sti PALETTE OUT.sti
    py -3 tools/sti_editor/sti_tool.py png-sheet FILE.sti OUT.png [--max-width N] [--duration MS] [--mask]
    py -3 tools/sti_editor/sti_tool.py png-assemble FRAME_DIR --out OUT.png [--duration MS] [--durations MS,MS,...]

`roundtrip` never writes to disk: it re-serialises in memory, checks the
bytes are identical, then forces a full ETRLE re-encode and checks every
decoded pixel, mask, offset and the palette are unchanged.
"""

from __future__ import annotations

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import palette_io  # noqa: E402
import sti  # noqa: E402


def cmd_info(args):
    s = sti.STIFile.load(args.file)
    print(args.file)
    print(s.describe())
    if args.frames and not s.is_rgb:
        print(f"\n{'nr':>5} {'szer.':>6} {'wys.':>6} {'X':>6} {'Y':>6} {'ETRLE B':>8}  aux")
        for i, f in enumerate(s.frames):
            aux = ""
            if f.aux:
                aux = (f"frames={f.aux.number_of_frames} cur={f.aux.current_frame} "
                       f"flags={sti.flag_names(f.aux.flags, sti.AUX_FLAG_NAMES)}")
            print(f"{i:5d} {f.width:6d} {f.height:6d} {f.offset_x:6d} {f.offset_y:6d} {len(f.encoded()):8d}  {aux}")


def cmd_export(args):
    s = sti.STIFile.load(args.file)
    stem = os.path.splitext(os.path.basename(args.file))[0]
    os.makedirs(args.out_dir, exist_ok=True)
    frames = [args.frame] if args.frame is not None else range(s.frame_count)
    for i in frames:
        p = os.path.join(args.out_dir, f"{stem}_{i:03d}.png")
        s.export_png(i, p)
        print(p)
        if args.mask and not s.is_rgb:
            pm = os.path.join(args.out_dir, f"{stem}_{i:03d}_mask.png")
            s.frames[i].mask_image().save(pm)
            print(pm)


def cmd_analyze(args):
    s = sti.STIFile.load(args.file)
    frames = [args.frame] if args.frame is not None else range(s.frame_count)
    for i in frames:
        if s.is_rgb:
            img = s.frame_rgba(0)
            print(f"RGB {img.width}x{img.height} bbox(alpha)={img.getbbox()}")
            continue
        st = s.frames[i].stats(s.palette)
        print(f"frame {i}: size={st['size'][0]}x{st['size'][1]} offset={st['offset']} "
              f"opaque={st['opaque']} transparent={st['transparent']} bbox={st['opaque_bbox']} "
              f"indices={st['unique_indices']} opaque_index0={st['opaque_index0']}")
        for idx, count, rgb in st["top_indices"][:5]:
            print(f"    index {idx:3d}: {count:6d} px  RGB{rgb}")


def cmd_pixel(args):
    s = sti.STIFile.load(args.file)
    i = args.frame or 0
    if s.is_rgb:
        print(s.frame_rgba(0).getpixel((args.x, args.y)))
        return
    f = s.frames[i]
    idx, opaque = f.get(args.x, args.y)
    print(f"frame {i} ({args.x},{args.y}): index {idx} RGB{s.palette[idx]} "
          f"{'opaque' if opaque else 'transparent'}")


def cmd_palette_export(args):
    s = sti.STIFile.load(args.file)
    if not s.is_indexed:
        raise sti.STIError("RGB file has no palette")
    used = palette_io.save_palette(args.out, s.palette[:s.number_of_colours], args.format)
    print(f"{args.out} ({palette_io.FORMATS[used]})")


def cmd_palette_import(args):
    if os.path.abspath(args.out) == os.path.abspath(args.file):
        raise sti.STIError("refusing to overwrite the input file -- choose a different OUT.sti")
    s = sti.STIFile.load(args.file)
    if not s.is_indexed:
        raise sti.STIError("RGB file has no palette")
    new = palette_io.load_palette(args.palette)
    changed = palette_io.apply_palette(s.palette, new)
    s.save(args.out)
    print(f"{args.out}: {len(new)} colours read, {changed} changed")


def cmd_png_sheet(args):
    s = sti.STIFile.load(args.file)
    ranges = sti.CHARACTER_PALETTE_RANGES if args.mask else None
    json_path, warnings = s.export_indexed_sheet(args.out, args.max_width, args.duration, ranges)
    print(args.out + (f" + {json_path}" if json_path else "") +
          (f" + {sti.colour_mask_path(args.out)}" if args.mask else ""))
    for w in warnings:
        print(f"warning: {w}")


def cmd_png_assemble(args):
    if os.path.isdir(args.frames[0]) and len(args.frames) == 1:
        directory = args.frames[0]
        paths = sorted(os.path.join(directory, n) for n in os.listdir(directory) if n.lower().endswith(".png"))
    else:
        paths = list(args.frames)
    durations = None
    if args.durations:
        durations = [int(d) for d in args.durations.split(",")]
    json_path, kind = sti.assemble_png_sheet(paths, args.out, args.max_width, args.duration, durations)
    print(f"{args.out} + {json_path}: {len(paths)} frames, {kind}")


def roundtrip_check(path) -> tuple[bool, str]:
    with open(path, "rb") as fh:
        data = fh.read()
    s = sti.STIFile.from_bytes(data)
    if s.to_bytes() != data:
        return False, "re-serialised bytes differ"
    if s.is_rgb:
        return True, "byte-identical (RGB)"
    for f in s.frames:
        f.mark_modified()
    s2 = sti.STIFile.from_bytes(s.to_bytes())
    if s2.palette != s.palette or len(s2.frames) != len(s.frames):
        return False, "palette/frame count differ after re-encode"
    for i, (a, b) in enumerate(zip(s.frames, s2.frames)):
        if (a.width, a.height, a.offset_x, a.offset_y) != (b.width, b.height, b.offset_x, b.offset_y):
            return False, f"frame {i} geometry differs after re-encode"
        if a.pixels != b.pixels or a.mask != b.mask:
            return False, f"frame {i} pixels differ after re-encode"
        if (a.aux and a.aux.pack()) != (b.aux and b.aux.pack()):
            return False, f"frame {i} aux data differs after re-encode"
    return True, f"byte-identical; re-encode lossless ({len(s.frames)} frames)"


def cmd_roundtrip(args):
    ok_all = True
    for p in args.files:
        try:
            ok, msg = roundtrip_check(p)
        except (OSError, sti.STIError) as exc:
            ok, msg = False, str(exc)
        ok_all &= ok
        print(f"{'OK  ' if ok else 'FAIL'} {p}: {msg}")
    return 0 if ok_all else 1


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("info")
    p.add_argument("file")
    p.add_argument("--frames", action="store_true", help="list every frame")
    p.set_defaults(func=cmd_info)
    p = sub.add_parser("export")
    p.add_argument("file")
    p.add_argument("out_dir")
    p.add_argument("--frame", type=int)
    p.add_argument("--mask", action="store_true", help="also write the transparency mask")
    p.set_defaults(func=cmd_export)
    p = sub.add_parser("analyze")
    p.add_argument("file")
    p.add_argument("--frame", type=int)
    p.set_defaults(func=cmd_analyze)
    p = sub.add_parser("pixel")
    p.add_argument("file")
    p.add_argument("x", type=int)
    p.add_argument("y", type=int)
    p.add_argument("--frame", type=int)
    p.set_defaults(func=cmd_pixel)
    p = sub.add_parser("palette-export")
    p.add_argument("file")
    p.add_argument("out")
    p.add_argument("--format", choices=sorted(palette_io.FORMATS), help="default: from the extension (.pal = JASC)")
    p.set_defaults(func=cmd_palette_export)
    p = sub.add_parser("palette-import")
    p.add_argument("file")
    p.add_argument("palette")
    p.add_argument("out")
    p.set_defaults(func=cmd_palette_import)
    p = sub.add_parser("png-sheet", help="palettised PNG (+ frame metadata) for the game's PNG loader")
    p.add_argument("file")
    p.add_argument("out")
    p.add_argument("--max-width", type=int, default=1024, help="wrap frames to rows of this width")
    p.add_argument("--duration", type=int, default=0, help="frame duration in milliseconds (\"frameDuration\")")
    p.add_argument("--mask", action="store_true",
                   help="also write the colour mask (<name>.mask.png) for a full colour version")
    p.set_defaults(func=cmd_png_sheet)
    p = sub.add_parser("png-assemble", help="one PNG sheet (+ frame metadata) from separate frame images")
    p.add_argument("frames", nargs="+", help="a directory of frame PNGs (in name order) or the frame files")
    p.add_argument("--out", required=True, help="the sheet to write (.png)")
    p.add_argument("--max-width", type=int, default=1024, help="wrap frames to rows of this width")
    p.add_argument("--duration", type=int, default=0, help="frame duration in milliseconds (\"frameDuration\")")
    p.add_argument("--durations", help="comma separated durations, one per frame, e.g. 80,80,120")
    p.set_defaults(func=cmd_png_assemble)
    p = sub.add_parser("roundtrip")
    p.add_argument("files", nargs="+")
    p.set_defaults(func=cmd_roundtrip)
    args = ap.parse_args(argv)
    try:
        return args.func(args) or 0
    except (OSError, sti.STIError, palette_io.PaletteError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
