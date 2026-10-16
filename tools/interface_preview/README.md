# Interface text preview

A developer tool (Python + Pillow) that draws texts onto an interface graphic
the way the game prints them, with the game's own fonts, and reports where
their ink lands. It is for laying out texts on new interface panels
(positions, margins, line spacing) before the game is built and tested. It is
independent of the engine and not part of the build.

It reuses `tools/sti_editor/sti.py` to read `.sti` fonts and also reads the
palettised PNG fonts of the PNG_Files feature (`NAME.png` + `NAME.png.json`).

## What it reproduces

- `MPrint()` / `Blt8BPPDataTo16BPPBufferMonoShadowClip()` (sgp): glyph
  offsets, the advance (`width + offsetX`), glyph pixel 1 = shadow colour,
  2+ = foreground colour, 0 = background colour, a near-black shadow or
  background colour of index 0 is not drawn.
- `GetFontHeight()`, `StringPixLength()`, `FindFontRightCoordinates()`,
  `FindFontCenterCoordinates()`.
- The glyph translation table (`assets/externalized/translation_tables`).

Font lookup, in order: `DATA/Fonts`, then `DATA/UNSLF/fonts` (an extracted
`Fonts.slf`), PNG before STI. Override it with `--font-dir` (repeatable).

## Usage

From the repository root:

```
py -3 tools/interface_preview/text_preview.py metrics FONT12ARIAL --data-dir "GAME/Data" --text "Kills"
py -3 tools/interface_preview/text_preview.py render tools/interface_preview/layouts/stats_big_1366x768.json --data-dir "GAME/Data" --out preview.png [--scale 2]
```

`metrics` prints the font height and the ink rows of a capital, an `x`, a
descender (`g`), and the width of `--text`.

`render` writes the preview and, for every text, its print position, its
ink bounding box and the margins of that ink to the walls of its box; then,
per box, the gaps between the ink of its lines. The ink is the letters only,
without their shadow.

## Layout file

```json
{
  "background": "Interface/ITEMINFOC_STATS_BIG_1366x768.png",
  "defaults": { "font": "FONT12ARIAL", "colour": 126, "shadow": 2 },
  "boxes": [ { "name": "kills", "x": 425, "y": 46, "w": 130, "h": 40 } ],
  "items": [
    { "string": "gzInfoboxStatsStrings[1]", "x": 430, "y": 51 },
    { "text": "0", "x": 430, "y": 51, "w": 121, "align": "right", "colour": 130 }
  ]
}
```

- `background`: relative to `--data-dir`, or absolute.
- `boxes`: the inside of the graphic's boxes (without their frame); a text
  belongs to the box holding its `x`, `y`.
- `items`: `text`, or `string` taken from
  `assets/externalized/strings/translation-LANG.json` (`--lang`, default
  `eng`); `x`, `y` as in the code; `align` `left` (default), `right` or
  `center` within `w`; `valign` `center` within `h`; `font`, `colour`,
  `shadow`, `background` as palette indices of the font (`SetFontForeground()`
  and the like).

The coordinates in a layout are copied from the code by hand; keep them in
step with it.

## Notes on FONT12ARIAL

Height 11: capitals and digits take rows 0-8 (9 px), descenders reach row
10. Every glyph has a blank column on its right, so a right-aligned value
ends 1 px before its field does.
