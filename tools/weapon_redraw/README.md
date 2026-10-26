# Weapon redraw prototype

A developer experiment (Python + Pillow): it tries to change the shape of the
weapon a soldier holds in an animation, automatically. In JA2 the weapon is not
a separate graphic: it is painted into every frame of the soldier animation
together with the arms and the body. The tool finds the weapon in each frame,
measures where it is and how it is held, erases it and draws another weapon
shape in its place. It is independent of the engine and not part of the build.

Status: prototype, tried on `s_merc/s_r_std.sti` (regular male standing with a
rifle, 8 directions x 8 frames). It writes preview pictures only; it does not
write game files yet.

## Usage

    python -I -B preview.py --data E:/JA2_Release1/Data --out out

| Option | Default | Meaning |
|---|---|---|
| `--data` | `E:/JA2_Release1/Data` | game data directory with `Anims.slf` and `Binarydata.slf` |
| `--anim` | `s_merc/s_r_std.sti` | the animation inside `Anims.slf` |
| `--out` | `out` | output directory |
| `--zoom` | `4` | nearest-neighbour zoom of the pictures |
| `--behind` | `0,7` | directions where the weapon is carried behind the body |
| `--anim-dirs` | `2,5` | directions shown in the animated GIF |

Output:

- `masks.png` - the first frame of every direction: the weapon pixels found
  (magenta), the pixels touching the hands (yellow) and the weapon's axis
  (cyan);
- `compare_dirs.png` - per direction: original | weapon erased | new weapon;
- `compare_anim.gif` - every frame of `--anim-dirs`, original next to new.

The soldier is drawn with a fixed outfit (`BROWNHEAD`, `PINKSKIN`, `greyVEST`,
`GREENPANTS`), without the game's lighting.

## How it works

Files:

- `ja2sprite.py` - readers for SLF archives, 8-bit ETRLE STCI sprites and the
  palette replacements of `ja2pal.dat`;
- `weapon.py` - weapon detection, erasing and drawing;
- `preview.py` - the command line tool.

The soldier sprites use fixed palette index ranges, which makes the weapon
separable from the body:

| Indices | Use |
|---|---|
| 245-250 | hair (replaced) |
| 235-244 | skin (replaced) |
| 220-234 | vest (replaced) |
| 205-219 | pants (replaced) |
| 173-187 | boots (brown ramp) |
| 197-202 | the weapon (grey ramp) |
| 203 | darkest grey: the weapon's edge, also the outline of the body |
| 254 | shadow |

Steps, per frame:

1. **Mask.** The blobs of grey indices 197-202. The weapon is the biggest group
   of blobs lying within 5 pixels of each other (a hand over the gun splits
   it), plus blobs further away on the weapon's line (a cut-off muzzle), plus
   the index-203 pixels touching them.
2. **Axis.** The principal axis of the mask (PCA): centre, angle, length. The
   muzzle is the end away from the centre of the vest and pants pixels. The
   grip is where the mask touches skin pixels.
3. **Erase.** Weapon pixels mostly surrounded by body pixels (at least 4 of 8
   neighbours) take the commonest body index around them, the body seen behind
   the gun; the others become transparent.
4. **Draw.** The new shape is defined in the weapon's own frame (along the
   barrel and across it, in pixels) and rasterised with 4 samples per pixel,
   from the old stock end, at the old angle, 1.35 times the old length (the
   isometric foreshortening comes with the old length). The prototype draws a
   long rifle with a scope and a magazine, shaded with the same grey indices.
   Skin and hair stay over it (the hands hold it). In the `--behind`
   directions it shows only where nothing else is drawn.

## Known limits

- The ground shadow still has the old weapon's shape.
- In the back views (directions 0 and 7) only a small part of the weapon is
  visible, so its axis is less reliable; which directions are "behind" is
  given by hand.
- Erasing guesses the body behind the gun from its neighbours; larger hidden
  areas (an arm partly behind the gun) are not rebuilt.
- The shape is drawn flat, without the weapon's own 3D turn in the isometric
  directions, and the outlines are simpler than hand-made pixel art.
- Animations where the weapon moves fast (firing, reloading) and other body
  types have not been tried.
