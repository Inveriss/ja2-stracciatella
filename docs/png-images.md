# PNG images

The game also loads images from PNG files, next to STI and PCX. This document
describes how to prepare such files and where to put them. Code: `src/sgp/PNG.cc`,
`src/sgp/HImage.cc` (`CreateImage()`), `src/sgp/VObject*.cc`.

## How to replace an image

**A file next to the original.** A file `name.png` in the same directory as
`name.sti` or `name.pcx` is loaded instead of it. No JSON or code changes are
needed. The PNG can be in a mod directory (`mods/<mod>/data/...`), loose in the
game's `Data` directory or in an SLF.

The PNG wins only when it is in the same or a higher VFS layer than the
original. Layer order (highest first): home directory, mods, `externalized`,
`Data`, SLF files. Examples:

- a PNG in a mod replaces an STI from `Interface.slf`;
- a loose PNG in `Data/Interface` replaces an STI from `Interface.slf`;
- a PNG in `Data` does **not** replace an STI provided by a mod.

**A path in JSON.** Where an image is named in the data (`items.json`,
`weapons.json`, `loading-screens.json` etc.), a path to a `.png` file can be
used.

Character animations (`anims/`), tile cache animations (`tilecache/`, e.g.
explosions) and cursors can be replaced by palettised PNGs with an `animation`
section in the metadata (see "Animations in the game world" below). Tileset
tiles are not replaced by PNGs.

### When a PNG can't be used

If a PNG next to the original is broken or doesn't fit the place it is used
in, the game log gets the entry
`Cannot use <file>.png instead of <file>.sti, loading the original: <reason>`
and the game loads the original. A PNG named directly in JSON has no original,
so the error stops loading that image.

### Turning the replacement off

In `ja2.json` (e.g. `%APPDATA%\JA2\ja2.json`):

```json
"image_png_override": false
```

turns off loading PNGs next to the originals. The game then looks exactly as
without the PNG files. `.png` files named directly in JSON are still loaded.

## Kinds of PNG

### Palettised PNG (recommended for game images)

Equivalent to a palettised STI, 1:1. The palette indices are kept, so all of
the game's effects work: shading, lighting, the grey faces of the dead, font
colours.

- **Index 0 is always transparent**, as in STI.
- An index with alpha below 128 in the `tRNS` chunk is transparent too. Alpha
  from 128 to 254 is drawn as opaque, with a warning in the log.
- **Index 254** means the same as in STI: in item images the outline pixels
  (e.g. of compatible ammo), in tiles and characters the shadow.
- Depth of 1, 2, 4 or 8 bits, interlaced or not. The palette can have fewer
  than 256 entries (the rest is black).
- The image editor **must not reorder or remove palette entries** (many
  editors do that when saving, e.g. when "optimising the palette"). The safest
  way is to export from the STI with `tools/sti_editor/sti_tool.py png-sheet`.

### Full colour PNG (RGBA, RGB, greyscale)

- As an **object** (icons, interface elements): drawn with alpha blending.
  Alpha 0 is skipped, 255 drawn as is, values in between blended with the
  background. Pure black stays visible.
- The **outline** (e.g. of a compatible item) is drawn automatically around the
  opaque pixels (alpha at least 128): on the transparent pixels touching them
  on the left, right, top or bottom. The outline must fit in the frame, so
  leave a 1 pixel transparent margin. Turning the outline off:
  `"outline": false` in the metadata (below).
- There is no palette, so effects based on the palette don't work. In these
  places the game uses the original instead of an RGBA PNG (with a log entry):
  - portraits of mercs and IMP characters (shading, grey faces of the dead),
  - fonts,
  - the radar and the person markers on the tactical map,
  - the generic button graphic `DEFAULT_GENERIC_BUTTON_OFF` (button text
    colour).

  Palettised PNGs can be used in these places.
- 16 bits per channel are cut down to 8 bits. The game screen has 16 bits
  (RGB565), so smooth colour gradients can show visible banding.

### Backgrounds (loading screens, laptop desktop, strategic map, window backgrounds)

- A palettised PNG gives an 8 bit background with the same indices (`tRNS` is
  ignored, as in PCX).
- A full colour PNG gives a 16 bit background. Pixels with alpha below 128 are
  transparent where the game uses background transparency; black pixels stay
  visible.
- The **strategic map** (`interface/b_map_*.pcx`) must be a palettised PNG,
  since the game draws it scaled down and shaded through the palette. A full
  colour PNG ends with a clear error.

### Main menu and initial options backgrounds in the game resolution

The original backgrounds of these screens are 640×480 and drawn in the middle
of the screen. A full screen background for the current game resolution is a
file in the same directory, with the resolution in its name:

| Screen | Original | Background for 1280×720 | Background for 1366×768 |
|---|---|---|---|
| main menu | `loadscreens/mainmenubackground.sti` (`Loadscreens.slf`) | `loadscreens/mainmenubackground_1280x720.png` | `loadscreens/mainmenubackground_1366x768.png` |
| initial options (new game) | `interface/optionsscreenbackground.sti` (`Interface.slf`) | `interface/optionsscreenbackground_1280x720.png` | `interface/optionsscreenbackground_1366x768.png` |

- The name is made from the current resolution. The launcher offers two base
  resolutions, 1280×720 and 1366×768 (the Auto mode picks one of them), so one
  background per base resolution covers every player. A custom resolution set
  with `res` in `ja2.json` (Manual mode) or with `--res` works the same way.
  Letter case doesn't matter. The file can be in a mod or loose in `Data`.
- `.png`, `.sti` and `.pcx` are looked for, in this order (the STI must be 16
  bit RGB or 8 bit without ETRLE).
- An image of another size than the screen is stretched to the full screen
  (nearest pixel, the aspect ratio can change), with a warning in the log.
- Without a file for the current resolution (or when it can't be loaded, with
  an error in the log) the screen draws the original 640×480 as before.
- The buttons, logo, texts and the darkened rectangle of the options screen
  stay in the 640×480 area in the middle of the screen: at resolution W×H it
  starts at ((W-640)/2, (H-480)/2). Design the background with that in mind.
- The background is 16 bit like the whole game screen, so a full colour PNG is
  shown in 65,536 colours. Transparency doesn't matter here (pixels with alpha
  below 128 are black).
- Memory: 2 bytes per screen pixel (e.g. 1280×720: 1.8 MB, 1366×768: 2 MB),
  while the screen is shown.

## Metadata: `name.png.json`

An optional file next to the PNG. Without it the whole image is one frame with
no offset and with an outline.

A list of frames (like STI subimages, with a drawing offset):

```json
{
  "frames": [
    { "x": 0,  "y": 0, "w": 56, "h": 17, "offsetX": 1, "offsetY": 1 },
    { "x": 57, "y": 0, "w": 26, "h": 20, "offsetX": 1, "offsetY": 2 }
  ]
}
```

A grid of equal cells, left to right and top to bottom:

```json
{
  "grid": { "w": 32, "h": 24, "count": 10 },
  "offsets": [ [0, 0], [-1, 2], ... ]
}
```

- `offsetX`/`offsetY` are optional (0 by default).
- `count` is optional (all cells by default).
- `offsets` is optional, and if present must have one entry per frame.
- `"outline": false` turns off the outline of a full colour image (no effect
  on palettised PNGs). It can stand on its own: `{ "outline": false }`.

The frame order must match the order of the subimages in the STI the PNG
replaces (the game refers to them by number).

### Animation frame durations

How long a frame is shown, in milliseconds (an integer 0–65535):

```json
{
  "frameDuration": 100,
  "frames": [
    { "x": 0,   "y": 0, "w": 248, "h": 110, "duration": 400 },
    { "x": 249, "y": 0, "w": 248, "h": 110 }
  ]
}
```

```json
{ "grid": { "w": 64, "h": 64, "count": 4 }, "durations": [80, 80, 120, 200] }
```

- `duration` in a frame of the `frames` list, `durations` (one entry per
  frame) next to `grid`;
- `frameDuration` applies to frames without their own duration (also to a
  single frame without `frames`/`grid`);
- `0` or nothing means "no change": the animation uses its usual duration from
  the game.

Durations only work in animations that support them. Elsewhere they are
ignored and the animation keeps the game's pace (as with STIs, which have no
frame durations). Animations supporting durations from PNG:

| Animation | File | Notes |
|---|---|---|
| florist ad (AIM) | `laptop/flowerad_16.png` | 16 frames; the frame duration sets how long it is shown before the next one (150 ms by default); the final display of frame 0 with the text lasts as before |
| "Your ad here" ad (AIM) | `laptop/yourad_13.png` | 13 frames, 150 ms by default |
| insurance ad (AIM) | `laptop/insurancead_10.png` | 10 frames, 150 ms by default |
| funeral home ad (AIM) | `laptop/funeralad_9.png` | 9 frames, 250 ms by default |
| Bobby Ray's ad (AIM) | `laptop/bobbyrayad_21.png` | 21 frames, played as 0–6, 0–6, 7–20; 300 ms by default |
| tile cache animations (explosions, smoke and other effects) | e.g. `tilecache/zgrav_d.png` | the frame duration replaces the animation delay (for explosions `blastSpeed` from `explosion-animations.json`); frames without a duration last as before |

In AIM ads with frame durations every frame is drawn on the page background
saved when it was drawn, not on the previous frame, so frames can have
transparency and translucency. A frame without its own duration lasts the
ad's default duration from the table. Without durations in the PNG (and for
STI) the ads work exactly as before, including the longer shown first and last
frames; with durations only `png.json` decides how long each frame lasts. The
ad files of other language versions have other names (e.g.
`german/yourad_13_german`).

### Animations in the game world (characters, explosions, cursors)

Character animations (`anims/`, e.g. `anims/s_merc/s_r_std` – a standing merc),
tile cache animations (`tilecache/`, e.g. explosions) and cursors need
auxiliary data: the number of animation frames. A PNG can replace them when:

- it is a **palettised PNG** – and for tile cache effects, characters (people
  with a colour mask) and corpses also a full colour PNG (below); adult
  creatures, their corpses and cursors must have a palette, since the game
  changes their colours through the palette,
- its metadata has an `animation` section:

```json
{
  "animation": { "framesPerDirection": 21 },
  "frameDuration": 80,
  "frames": [ ... ]
}
```

`framesPerDirection` (1–255) is the number of frames of one animation; with
several directions the frames of the next directions follow one another
(direction 0: frames 0…N−1, direction 1: N…2N−1, …). The game builds from this
the same data the original STIs have: the first frame of every direction gets
the number of frames and the animation flag. A static cursor has
`"framesPerDirection": 1`.

Without the `animation` section, or for a full colour PNG where a palette is
needed, the game loads the original and logs an error. The number of frames and
their sizes must match the `.jsd` file of the same name, if there is one.
Tileset tiles are not replaced.

Characters:

- the frames are ordered by direction: the number of directions of the
  animation (1, 2, 3, 4, 8 or 32, depending on the animation) ×
  `framesPerDirection` must equal the number of frames of the original,
  otherwise the game logs a mismatch;
- the order and sizes of the frames must match the original, since the
  animation scripts (`binarydata/ja2bin.dat`) refer to frames by number, and
  the structure data (`anims/structdata/*.jsd`) relates to their sizes;
- **the palette must stay unchanged**: the game changes the hair, skin, vest
  and pants colours through palette entries (character palettes), and lighting
  and shadow use the palette as well;
- the character's shadow is part of its frames: pixels with index 254 darken
  the background.

Full colour (RGBA) characters with nothing else are possible where the game
doesn't change colours:

| Character | Animations |
|---|---|
| cow | `anims/animals/c_breath`, `c_walk`, `c_die`, `c_eat` |
| crow | `anims/animals/cr_walk`, `cr_fly`, `cr_eat`, `cr_die` |
| bloodcat | `anims/animals/ct_breath`, `ct_walk`, `ct_run`, `ct_ready`, `ct_hit`, `ct_die`, `ct_swipe`, `ct_bite` |
| robot | `anims/civs/j_r_bret`, `j_r_walk`, `j_r_hit`, `j_r_die`, `j_r_shot` |
| vehicles | `anims/vehicles/hummer`, `hummer2`, `icecrm`, `hm_wrek`, `tank_rot`, `tank_sht`, `tk_wrek`, `tnk2_rot`, `tnk2_sht`, `tk2_wrek` |
| queen | `anims/monsters/qmn_breat`, `q_ready`, `q_spit_sw`, `q_spit_e`, `q_spit_ne`, `q_spit_s`, `q_spit_se`, `q_die`, `q_swipe` |
| larvae and young creatures | `anims/monsters/l_breath`, `l_die`, `l_walk`, `i_breath`, `i_walk`, `i_die`, `i_eat`, `i_attack` |

**People** (mercs, army, militia, civilians: `anims/s_merc/`, `m_merc/`,
`f_merc/`, `civs/` and the other people animations) can be full colour only
with a **colour mask**, since the game changes the colours of their hair,
skin, vest and pants (and camouflage). The mask is a file `name.mask.png` next
to `name.png`:

- a palettised PNG of the same size as the sheet (the same frames from
  `png.json`); the mask's palette doesn't matter, the indices do;
- index **0**: the pixel has the colour from the full colour PNG;
- an index from a replacement range (hair 245–250, pants 205–219, skin
  235–244, vest 220–234, according to `binarydata/ja2pal.dat`): the game finds
  the pixel's brightness on the colours of that range in the original's
  palette (from light to dark) and gives the pixel the colour from the same
  place of the range in the character's palette, smoothly between neighbouring
  colours; so paint these parts in the original's colours, and the game turns
  them into the character's colours keeping brightness and detail;
- any other index: the pixel gets the exact colour of that index from the
  character's palette;
- black shadow pixels (alpha below 255) are not changed.

Without a mask the game skips a person's full colour PNG and loads the original
(with a log entry). A mask matching the original is written by
`png-sheet --mask` (below). Adult creatures (palette from `.COL` files) only
have palettised PNGs.

An RGBA character:

- is lit and highlighted the same way as with a palette (light levels and
  their colour, the red highlight of an enemy, the grey of an unseen enemy, the
  white flash): the game changes the pixel colours with the same formulas it
  builds the shading palettes with;
- the **shadow** in a frame is a black pixel with partial transparency (alpha
  below 255): it darkens the background through blending and is neither lit
  nor highlighted; an alpha of about 100–127 is recommended, since pixels with
  alpha at least 128 write Z;
- the Z buffer works as in STI; pixels with alpha at least 128 write Z; a
  character behind a structure is drawn as a checkerboard as with a palette;
  characters on several tiles (cow, bloodcat, vehicles, queen) have Z values
  from the structure data in the successive vertical strips of the frame, the
  same as with a palette;
- has no outline (the game doesn't build an outline mask for characters);
- the colour mask takes 1 byte per pixel (5 bytes together with RGBA);
- can be mixed with STI animations of the same character (e.g. only `cr_fly`
  in RGBA): the game then takes the shading palettes for the STIs from the
  original the PNG replaces.

Tile cache effects (`tilecache/`: explosions, smoke, splashes and others) can
be **full colour PNGs** (RGBA):

- alpha is blended with the background as in RGBA objects, so smoke and fire
  can have soft, translucent edges;
- the Z buffer works as in STI: the effect hides behind walls and characters in
  front of it; effects writing Z do so only for pixels with alpha at least 128;
- translucent effects in the game (e.g. smoke, the end of an explosion) draw
  the PNG with its alpha halved instead of the STI's pixel grid;
- the shadow layer draws pixels with alpha at least 128 as shadow;
- effects are not lit (like the STIs of these effects); the PNG colours are
  shown as they are;
- `"outline": false` is recommended, since effects don't use the outline, and
  the outline mask takes memory;
- **corpses** (`anims/corpses/`) are drawn like characters on several tiles,
  with lighting. They can be full colour PNGs when the game doesn't change
  their colours: animal corpses (`ct_dead`, `cw_dead1`), vehicle wrecks
  (`tk_wrek`, `tk2_wrek`, `hm_wrek`, `ic_wrek`), the queen (`qn_dead`), the
  robot (`j_dead`) and corpses in the late stage of decay (`p_decomp2`), as well
  as larvae and young creatures (`l_dead1`, `i_dead1`). **Corpses of people**
  (also camouflaged; `s_d_*`, `m_d_*`, `f_d_*`, civilian corpses and their
  bloodless `_nb` versions) can be full colour only with a **colour mask**
  `name.mask.png`, like the people animations: corpses keep the colours of the
  character they come from. Corpses of adult creatures have a palette: a full
  colour PNG is skipped and the game loads the original.

Frame durations (`duration`) don't apply to characters or cursors: the game
sets their pace. The easiest start is exporting the original with the
`png-sheet` command (below), which writes the `animation` section for
animations itself and puts every direction in its own row of the sheet.

## Tools

Export of an STI to a palettised PNG, keeping indices, frames and offsets:

```
py -3 tools/sti_editor/sti_tool.py png-sheet file.sti file.png [--max-width N] [--duration MS] [--mask]
```

Creates `file.png` and, with several frames, offsets or `--duration`,
`file.png.json` (`--duration` writes `frameDuration`). For animated STIs
(characters, explosions, cursors) it also writes the `animation` section and
puts every direction in its own row. STIs with other auxiliary data
(tilesets) give a warning, since the game can't use such a PNG. `--mask` also
writes the colour mask `file.mask.png` for full colour versions of people: the
indices of pixels from the colour replacement ranges, 0 elsewhere.

Assembling separate frame files (e.g. `explosion/000.png`, `001.png`, … in name
order) into one sheet with metadata:

```
py -3 tools/sti_editor/sti_tool.py png-assemble explosion --out explosion.png [--duration MS] [--durations 80,80,120]
```

If all frames are PNGs with the same palette, the sheet has a palette too
(indices unchanged); otherwise it is RGBA. The frame offsets are 0 and can be
changed later in `explosion.png.json`. The game always loads the sheet, not
the separate frame files. Details in `tools/sti_editor/README.md`.

## Limitations

- Image dimensions up to 65535 pixels in each direction and up to 2^26 pixels
  in total.
- Tileset tiles still need STI; character animations, tile cache effects and
  cursors – palettised PNGs only (full colour in the game world is not
  supported yet).
- A palettised PNG image takes as much memory as an STI, a full colour image
  about 5 bytes per pixel (RGBA and the outline mask).

## Performance

Measurement (RelWithDebInfo build, one load from the file cache, 2026-10-02)
on game images, test `PNGLoadTest.DISABLED_benchmark`:

| Image | Use | Original | PNG | Memory original | Memory PNG |
|---|---|---:|---:|---:|---:|
| `b_map_1024` (1460×1190, palettised) | 8 bpp background | 9.0 ms | 20.2 ms | 1697 KiB | 1697 KiB |
| `ls_dayomerta` (640×480, RGB) | 16 bpp background | 0.2 ms | 10.7 ms | 600 KiB | 600 KiB |
| `gun01` (1 frame, palettised) | object | 0.13 ms | 0.45 ms | 1.3 KiB | 1.3 KiB |
| `mdguns` (50 frames, palettised) | object | 0.14 ms | 1.35 ms | 25 KiB | 25 KiB |
| `mdguns` (50 frames, RGBA) | RGBA object | 0.14 ms | 1.87 ms | 25 KiB | 207 KiB |

Checking whether a PNG lies next to an image: about 7 µs on the first load of
a given path (walking the VFS layers), then 0.5 µs (the result is remembered).

Conclusions:

- A PNG loads slower than an STI/PCX (zlib decompression, ETRLE encoding or
  conversion to 16 bits), but that is milliseconds per image, paid when a
  screen is loaded, not on every drawn frame.
- Drawing palettised images is identical to STI. RGBA images are blended pixel
  by pixel, which only matters for large images drawn every frame.
- A palettised PNG takes as much memory as an STI; RGBA about 8 times more
  than ETRLE.
- When there is no PNG, the cost is one check per path: with a few hundred
  images on a screen that is fractions of a millisecond on the first load.

## Tests

- Unit tests: `ja2 -unittests` (groups `PNG`, `ETRLE`, `PNGLoadTest`). The test
  files are created by `tools/generate_png_unittest_data.py`.
- Time and memory measurement (off by default): pairs of `name.png` and
  `name.sti`/`name.pcx` in `<build directory>/unittests/data/pngbench/`, then

  ```
  GTEST_ALSO_RUN_DISABLED_TESTS=1 GTEST_FILTER=PNGLoadTest.DISABLED_benchmark ja2 -unittests
  ```
