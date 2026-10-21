# Sprite palettes preview

A developer experiment (Python + Pillow) comparing clothing colours of soldier
sprites: the hand-made palette replacements of `ja2pal.dat` (1999) against
colour ramps computed in the OKLab colour space. It is independent of the
engine and not part of the build. The game file readers come from
`tools/weapon_redraw/ja2sprite.py`.

## Usage

    python -I -B render_pants.py --data E:/JA2_Release1/Data --out pants_compare.png

| Option | Default | Meaning |
|---|---|---|
| `--data` | `E:/JA2_Release1/Data` | game data directory with `Anims.slf` and `Binarydata.slf` |
| `--anim` | `s_merc/s_r_std.sti` | the animation inside `Anims.slf` |
| `--direction` | `4` | direction 0-7 (4 faces the viewer); the first frame is drawn |
| `--zoom` | `6` | nearest-neighbour zoom |
| `--out` | `pants_compare.png` | the picture written |

The picture has three rows:

- the same soldier with three trousers: `GREENPANTS` (1999), `OKGREENPANTS` and
  `OKARMYPANTS` (computed);
- the outfits as tried in the game: an administrator (`YELLOWVEST` +
  `GREENPANTS`), green militia (`GREENVEST` + `OKGREENPANTS`), a soldier
  (`REDVEST` + `OKARMYPANTS`);
- the 15 shades of each trouser palette.

Each pixel is the colour the game would use, without lighting. The screen
of the game is 16-bit (RGB565), so in the game the shades are also rounded to
32 levels of red and blue and 64 of green.

## Palettes

A palette replacement is a ramp of shades for one index range of the sprite
(trousers: indices 205-219, 15 shades). The sprite's pixel art decides which
pixel gets which shade; only the colours change.

`GREENPANTS` of `ja2pal.dat` scales the RGB channels down evenly to black. The
computed ramps (`code_ramp()`) go from a top colour down to the lightness of a
bottom colour in the OKLab colour space:

- even steps of perceived lightness;
- the chroma fading out towards the shadows (`1 - t^1.6`);
- the lights 4 degrees of hue towards yellow, the shadows 10 degrees towards
  cyan.

| Palette | Top colour | Bottom | Chroma | Hue |
|---|---|---|---|---|
| `OKGREENPANTS` | 79, 90, 45 (`GREENPANTS`' top) | 2, 2, 2 | x1.0 | +0 |
| `OKARMYPANTS` | 79, 90, 45 | 2, 2, 2 | x1.35 | +14 degrees (greener) |

## The in-game experiment

The same ramps were tried in the game (2026-10-21, `Militia` branch): code in
`Soldier_Control.cc` added them to the palette replacements after
`LoadPaletteData()`, the green militia wore `OKGREENPANTS` and the enemy army
`OKARMYPANTS`. It was postponed and the original colours restored; that code
is kept here as `code_drawn_palettes.patch` (palettes only: the game code
that gives them to the soldiers, in `Soldier_Create.cc`, is not in it). To try
it again, from the repository root:

    git apply tools/sprite_palettes/code_drawn_palettes.patch

then set `PantsPal = "OKGREENPANTS"` / `"OKARMYPANTS"` for the wanted soldier
classes in `GeneratePaletteForSoldier()` (`Soldier_Create.cc`).
