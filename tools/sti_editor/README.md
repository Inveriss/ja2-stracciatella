# STI Editor

A developer tool (Python + Pillow + Tk) to read, view, analyse and edit the
`.sti` files (STCI -- Sir-Tech's Crazy Image) used by JA2 Stracciatella. It is
independent of the engine: it compiles nothing, doesn't modify the JA2 code
and is not part of the build.

| File | Role |
|---|---|
| `sti.py` | STI format parser/writer, ETRLE, export to Pillow/PNG |
| `palette_io.py` | palette import/export (JASC-PAL, RIFF PAL, ACT, GIMP, STI, PNG) |
| `drawing.py` | shape masks (pencil, line, rectangle, ellipse, fill, text) |
| `sti_editor.py` | desktop editor (GUI, Tk) |
| `sti_tool.py` | command line version: info, export, analysis, pixel, round trip |
| `tests/test_sti.py` | tests (`unittest`) |

## Requirements

- Python 3.9+ with Tk (the standard Python installer for Windows includes Tk),
- Pillow (tested with Pillow 12.3, Python 3.14, Tk 9.0).

No other libraries are needed.

## Running

From the repository root (on Windows: the `py` launcher):

```
py -3 tools/sti_editor/sti_editor.py
py -3 tools/sti_editor/sti_editor.py path/to/file.sti
```

Command line:

```
py -3 tools/sti_editor/sti_tool.py info      file.sti [--frames]
py -3 tools/sti_editor/sti_tool.py analyze   file.sti [--frame N]
py -3 tools/sti_editor/sti_tool.py pixel     file.sti X Y [--frame N]
py -3 tools/sti_editor/sti_tool.py export    file.sti DIRECTORY [--frame N] [--mask]
py -3 tools/sti_editor/sti_tool.py roundtrip file1.sti [file2.sti ...]
py -3 tools/sti_editor/sti_tool.py palette-export file.sti palette.pal [--format jasc|riff|act|gpl|png]
py -3 tools/sti_editor/sti_tool.py palette-import file.sti palette.pal result.sti
py -3 tools/sti_editor/sti_tool.py png-sheet file.sti result.png [--max-width N] [--duration MS] [--mask]
py -3 tools/sti_editor/sti_tool.py png-assemble FRAMES_DIRECTORY --out result.png [--max-width N] [--duration MS] [--durations MS,MS,...]
```

`roundtrip` writes nothing to disk: it serialises the file in memory and
checks that the bytes are identical, then forces recompression of all frames
and compares every pixel, mask, position, palette and aux data.

`png-sheet` writes all frames of a palettised STI as one palettised PNG, which
the game loads instead of the STI (`src/sgp/PNG.cc`). The palette indices stay
unchanged, and transparent pixels get index 0, which the game always treats as
transparent. The frames are laid out in rows `--max-width` wide (1024 by
default) with a 1 pixel gap. The frame positions and offsets go to
`result.png.json`; with a single frame without an offset that file isn't
created. Opaque pixels with index 0 are reported as a warning, since they
would become transparent in the game. `--duration` writes the animation frame
duration in milliseconds (`frameDuration`, described in `docs/png-images.md`).
For animated STIs (aux data that is only animation frame counts: characters,
explosions, cursors) an `"animation": { "framesPerDirection": N }` section is
written, and every direction goes to its own row of the sheet. Other aux data
(e.g. of tilesets) is not exported – a warning is shown then. `--mask` also
writes the colour mask `result.mask.png` (the same layout, the indices of
pixels from the people colour replacement ranges, 0 elsewhere), needed by full
colour versions of characters (`docs/png-images.md`).

`png-assemble` puts separate frame files (a directory of PNG files in name
order, or a list of files) together into one sheet `result.png` with
`result.png.json`. When all frames are PNGs with the same palette, the sheet
has a palette too (indices unchanged, index 0 transparent), otherwise it is
RGBA. `--duration` is the duration of all frames, `--durations` the durations
of the successive frames (0 = default). The frame offsets are 0.

Tests:

```
py -3 -m unittest discover -s tools/sti_editor/tests -v
```

The tests read all `.sti` files from `assets/` (read only -- at the end they
check SHA-256 sums that none was changed). Test writes go to temporary
directories.

## Editor features

The editor's user interface is in Polish; menu and button names are quoted
below as shown.

- **File** (menu *Plik*): Open, Save, Save as, New from PNG, export of the
  current frame to PNG, export of all frames to PNG, export of the frame mask,
  import of a PNG into the current frame. Before saving, the file is verified
  by reading it back.
- **Frames (multi-page):** ⏮ / ◀ *Poprz.* (previous) / *Nast.* ▶ (next) / ⏭,
  the *Nr obr.* field (frame number, Enter), PageUp/PageDown, arrow keys ←/→;
  duplicating, a new empty frame, deleting, clearing a frame.
- **Information:** size of the current frame, X/Y position (editable,
  `sOffsetX/sOffsetY`), flags, size from the header, number of frames, aux
  data.
- **Playback:** ▶ Play / ⏸ Pause (space) with an adjustable frame duration --
  preview in the editor only, see the limitations.
- **Palette:** 256 colours; click = pick a colour, double click = edit a
  palette colour, the *Przezroczysty* (transparent) button. Hovering shows the
  index and RGB.
- **Palette import / export** (menu *Paleta* or the buttons under the
  palette): export to JASC-PAL (`.pal`), Microsoft RIFF PAL (`.pal`), Adobe ACT
  (`.act`), GIMP (`.gpl`) or a 16×16 PNG swatch (one cell = one index). Import
  recognises the format by the file's content (not by its extension) and also
  accepts the palette of another `.sti` file or of a palettised PNG/BMP/GIF.
  Import changes the colours, not the pixel indices (that is how an STI
  palette works); it can be undone (Ctrl+Z). A palette shorter than 256
  colours replaces only the first indices.
- **Tools:** pencil (P), line (L), rectangle (R), ellipse (E) -- outline or
  filled, fill (F), text (T), eyedropper (I, also the right mouse button with
  any tool), eraser/transparency (X); brush size 1--16, font size.
- **View:** zoom 1×--32× (Ctrl+/Ctrl-, Ctrl+wheel), background: checkerboard /
  black / magenta / white, transparency mask preview, pixel grid (zoom ≥ 6).
- **Undo/Redo:** Ctrl+Z / Ctrl+Y, up to 100 steps (pixels, positions, aux,
  palette, adding/removing frames). Undoing all changes restores the original
  bytes of the file.
- **Analysis:** frame statistics (transparent/opaque pixels, bbox, number of
  used indices, most frequent colours, ETRLE size), file information, a status
  bar with x/y, index and RGB of the pixel under the cursor.

## The STI format in JA2 (source: the engine code)

Based on `src/sgp/ImgFmt.h`, `src/sgp/STCI.cc`, `src/sgp/HImage.h`,
`src/game/Utils/STIConvert.cc` and `src/sgp/VObject_Blitters.cc`.
Little-endian, structure alignment as in C.

```
STCIHeader            64 B
palette               uiNumberOfColours × 3 B (RGB)          -- STCI_INDEXED only
STCISubImage[]        usNumberOfSubImages × 16 B             -- STCI_ETRLE_COMPRESSED only
image data            uiStoredSize B
application data      uiAppDataSize B (AuxObjectData[], 16 B per frame)
```

- `STCIHeader`: `cID "STCI"`, `uiOriginalSize`, `uiStoredSize`,
  `uiTransparentValue`, `fFlags`, `usHeight`, `usWidth`, a 20 B union (indexed:
  `uiNumberOfColours`, `usNumberOfSubImages`, R/G/B depths; RGB: channel masks
  and depths), `ubDepth` (offset 44), 3 B alignment, `uiAppDataSize` (offset
  48), 12 B unused. The alignment and unused bytes are kept verbatim.
- Flags: `TRANSPARENT 0x01`, `ALPHA 0x02`, `RGB 0x04`, `INDEXED 0x08`,
  `ZLIB 0x10`, `ETRLE 0x20`.
- `STCISubImage` (frame): `uiDataOffset`, `uiDataLength`, `sOffsetX`,
  `sOffsetY` (INT16, added to the blit position), `usHeight`, `usWidth`.
- **ETRLE:** every line is a sequence of control bytes ended by a `0x00` byte;
  bit 7 = 1 → that many transparent pixels (1--127), bit 7 = 0 → that many
  palette indices following it.
- `AuxObjectData`: `ubWallOrientation`, `ubNumberOfTiles`, `usTileLocIndex`,
  3 B, `ubCurrentFrame`, `ubNumberOfFrames`, `fFlags` (`FULL_TILE`,
  `ANIMATED_TILE`, `DYNAMIC_TILE`, `INTERACTIVE_TILE`, `IGNORES_HEIGHT`,
  `USES_LAND_Z`), 6 B.

### Lossless saving

- Frames that weren't changed are written with their original ETRLE bytes; if
  nothing was changed, the file is identical byte for byte (checked on all 31
  `.sti` files in `assets/`, 163,524 frames).
- Changed frames are compressed with the algorithm from `STIConvert.cc`
  (`ETRLECompress`). On the files in the repository, recompressing every frame
  gives exactly the original bytes.
- Transparency is kept as a separate mask, not as "index 0". The engine draws
  every pixel of a literal run, also those with index 0 -- and such opaque
  pixels with index 0 occur in real files (e.g.
  `inventory-graphic-not-found-*.sti`). The editor keeps them.
- The palette, X/Y positions, `AuxObjectData`, unrecognised application data
  and any bytes past the end of the file are kept.

## Limitations (deliberately not simulated)

- **Key frame:** the STI format has no such information. The only animation
  data is `AuxObjectData` (`ubNumberOfFrames`, `ubCurrentFrame`, the
  `ANIMATED_TILE` flag) -- the editor shows it and lets you change it when the
  file has it.
- **Frame duration / pause:** STI doesn't store the frame duration (the game
  code sets the animation pace). Playback and its speed are only a preview in
  the editor and are not saved.
- **Transparency:** binary only (a pixel is transparent or not); an indexed STI
  has no alpha channel or translucency.
- **RGB files (16 bpp, `STCI_RGB`):** preview, analysis and PNG export only;
  saving keeps the data unchanged. There are no such files in the repository.
- **Indexed without ETRLE:** reading, preview and export; editing is off (the
  engine creates video objects only from ETRLE files).
- **ZLIB:** not supported -- as in the engine (`STCI.cc` rejects such files).
- **STI-Edit palette format:** it wasn't established in which format STI-Edit
  saves its palette files, so import supports the popular formats and
  recognises them by content. If a palette file from STI-Edit isn't
  recognised, import reports an error instead of guessing.
- **Palette import without remapping:** pixels keep their indices, so the
  image takes the colours of the new palette under the same numbers.
- **Editing the palette** changes the colour in all frames (the palette is
  shared by the whole file).
- **Size from the header** (`usWidth/usHeight`): in multi-frame files it
  describes the source sheet, not a frame, so it isn't changed. In single-frame
  files where it matched the frame, it is updated together with it (as
  `WriteSTIFile` writes it).
- **PNG import / New from PNG:** colours are mapped to the nearest palette
  colour (exact matches stay exact); pixels with alpha < 128 become
  transparent. On import, opaque pixels don't get index 0, since the JA2
  writing tools (`STIConvert.cc`) treat it as transparent.
- **Text** is drawn with Pillow's default font (no antialiasing), not with the
  game's fonts.
- **Zoom** is limited so that the zoomed image doesn't exceed about 24 million
  pixels (memory).
