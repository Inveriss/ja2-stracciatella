# Vehicles with 8 directions — plan

Status: **plan, not implemented, all decisions taken by the user (2026-11-04)** (written on
Remote Control). Work goes on the branch **`Tanks`**, by the user's decision. Everything rests on reading the code and the file headers. The only thing seen in
the game is the user's test: an APC under the Hummer's file stayed in one position.

## 1. Goal

The game's data holds vehicle graphics with 8 directions, which the code can't draw today. The
game should accept them on any vehicle: a file with 8 directions or with 32, without a change
in the code for each file.

Turning stays less smooth with 8 directions: the picture changes every 45°, not every 11.25°.
Nothing in this plan can make up for pictures that don't exist.

## 2. What exists today

### Files

From the headers in `Anims.slf`. Frames 0-7 of APC, VAN1 and 4X4 were also rendered and looked
at: 8 different directions, in the order the code expects (frame 0 facing up the screen).

| File | Frames | "Frames per direction" in the file | Largest frame | Used by the code |
|---|---|---|---|---|
| `HUMMER2.STI` | 32 | 1 | 141x104 | Hummer |
| `ICECRM.STI` | 32 | 1 | 152x154 | Ice cream truck |
| `HUMMER.STI` | 32 | 1 | 131x95 | Listed for Eldorado and Jeep, but see below |
| `ELDRDO.STI` | 32 | 1 | 159x110 | **No** |
| `JEEP.STI` | 32 | 1 | 111x92 | **No** |
| `TNK2_ROT.STI` | 32 | 1 | 248x170 | Tank |
| `HMMV.STI` | 8 | **8** | 130x90 | No |
| `4X4.STI` | 8 | **8** | 101x84 | No |
| `VAN1-4.STI` | 8 | **8** | 131x108 | No |
| `ATRUCK.STI` | 8 | **8** | 176x143 | No |
| `APC.STI` | 8 | **8** | 203x183 | No |
| `RED_CAR1.STI` | 8 | 1 | 144x84 | No |
| `HELI.sti` (the user's) | 8 | 1 | 226x181 | Helicopter, verified in the game |

The value 8 appears in the first frame's record only. It is what the tool wrote for one group
of 8 frames, as in the 8 frame firing animations of `TANK_SHT.STI`.

### Why the APC stood still

| Step | Where | What happens |
|---|---|---|
| Directions | `gAnimSurfaceDatabase`, `Animation_Data.cc:443` | Fixed per surface: `HUMVEE_BASIC` has 32, whatever the file holds |
| Frames per direction | `Animation_Data.cc:815` | Read from the file: 8 for the APC |
| Frame | `ConvertAniCodeToAniFrame`, `Soldier_Control.cc:4229` | Frames per direction x direction: 0, 8, 16... |
| Past the end | `Soldier_Control.cc:4232` | Frame 8 and above don't exist, so frame 0 and a warning in the log |

Writing 8 in the table is **not enough**: the file's value stays 8, and direction 1 still asks
for frame 8. It would be enough for `RED_CAR1.STI` alone.

### What already works for 8 directions

The helicopter proves the path, in the game:

- **The frame:** `Soldier_Control.cc:4188` starts from `OneCDirection(bDirection)`, and only the
  32 direction case replaces it.
- **Footprint:** every vehicle has 8 structures in its JSD, chosen by `OneCDirection`, whether
  the picture has 8 or 32 directions.
- **What hides the vehicle:** `AddZStripInfoToVObject` steps by frames / 8 structures, so 1 for
  an 8 frame file. `RenderWorld.cc:958` reads the same index.
- **Turning:** `TurnSoldier` walks the 32 steps for any vehicle and updates `bDirection` every
  fourth step (`Soldier_Control.cc:4460`). The picture would change exactly when the footprint
  does.
- **Routes:** a path has 8 directions, so a car with 8 pictures never drives sideways to its
  picture.

### Footprint and wreck belong to the body type, not the picture

| What | Where | Effect |
|---|---|---|
| Footprint | `gAnimStructureDatabase`, `Animation_Data.cc:524`: Hummer, Eldorado, truck and Jeep all use `HMMV.JSD` | An APC in the Hummer's file keeps the Hummer's footprint and its armour |
| Wreck | `HUMVEE_DIE`, `HUMMER_DEAD` | It becomes a Hummer's wreck |
| Portrait | `giCarPortraits`, `Interface_Utils.cc` | Hummer's portrait |

JSD files in the data (first structure read only):

| File | Tiles | Armour byte | Hit points byte | Flags |
|---|---|---|---|---|
| `STRUCTDATA/HMMV.JSD` | 13 | 0x16 | 0xFF | 0x8090 |
| `STRUCTDATA/TNK2_ROT.JSD` | 18 | 0x17 | 0xFF | 0x8090 |
| `VEHICLES/APC.JSD` | 21 | 0x17 | **0x00** | 0x8010, no `STRUCTURE_TRANSPARENT` |
| `VEHICLES/RED_CAR1.JSD` | 12 | 0x16 | **0x00** | 0x8010, no `STRUCTURE_TRANSPARENT` |

`APC.JSD` and `RED_CAR1.JSD` lie in the wrong folder for the code, which reads `STRUCTDATA`.
They differ from the JSDs in use in the hit points and the flag, and what that does hasn't been
checked. There is no JSD for 4X4, the vans or `ATRUCK`.

What may fit `HMMV.JSD`, judged by size only: 4X4, HMMV, the vans, the red car. The ice cream
truck already uses it at 152x154. `ATRUCK` is a little wider than that. The APC is not far
from the tank and needs more than 13 tiles.

### Found on the way: Jeep and Eldorado

`gubAnimSurfaceIndex` (`Animation_Control.cc:2386`, `2397`) gives both `HUMVEE_BASIC`, so they
would draw **the Hummer**. Their own surfaces `ELDORADO_BASIC` and `JEEP_BASIC` aren't assigned,
and point at `hummer.sti` anyway. `ELDRDO.STI` and `JEEP.STI` are never loaded. Stage 3 fixes
this (E3).

## 3. Decisions

All taken by the user, 2026-11-04.

| # | Question | Decision |
|---|---|---|
| E1 | When does the picture change during a 45° turn? | **At the end of the turn's 4 steps, with `bDirection`.** No extra code, and picture and footprint change together |
| E2 | New vehicles (APC, vans, 4X4...) as vehicles of their own? | **Yes, but as another stage of the project.** Nothing of it in this plan. It needs its own plan: a new body type touches about 15-20 files (tables, panels, profile, `vehicles.json`, map placement) |
| E3 | Jeep and Eldorado drawing their own files? | **Yes**: stage 3 redirects them to `jeep.sti` and `eldrdo.sti`. Both still wait for activation, so nothing visible changes today |
| E4 | Which branch? | **`Tanks`** |

Without a decision needed:

- No save format change: the direction is saved as today, and the frame is worked out when
  drawing. The no-compatibility rule isn't touched.
- Enemy tanks' and the player's tank's firing surfaces stay as they are: they aren't
  `ANIM_DATA_FLAG_NOFRAMES`.

## 4. Design

Only surfaces flagged `ANIM_DATA_FLAG_NOFRAMES` change: one picture per direction, so they are
the vehicles' driving surfaces. After loading (`Animation_Data.cc:815`):

```cpp
a->uiNumFramesPerDir = pAuxData->ubNumberOfFrames;
if (a->ubFlags & ANIM_DATA_FLAG_NOFRAMES)
{
	// one picture per direction, whatever the file's tool wrote; how many
	// directions the vehicle has comes from the file
	a->uiNumFramesPerDir = 1;
	a->uiNumDirections   = VehicleSurfaceDirections(a->hVideoObject->SubregionCount(), a->uiNumDirections);
}
```

```cpp
// 8 or 32 by the file; anything else keeps the table's value (the warning below stays)
UINT32 VehicleSurfaceDirections(UINT32 frames, UINT32 table_directions);
```

- `uiNumDirections` stops being `const` (`Animation_Data.h:533`).
- Move the block below `a->hVideoObject = hVObject.release()`, or use `hVObject` directly, so
  it runs before the mismatch check at line 838.
- `ConvertAniCodeToAniFrame` doesn't change: its `case 32` applies only to a surface with 32.
- Surfaces flagged today all have 1 frame per direction in their files, so forcing 1 changes
  nothing for them (checked in the table above, for all of them in use).
- PNG: the "animation" section goes the same way, so an 8 frame PNG works too. A note in
  `docs/png-images.md`.

## 5. Stages

### Stage 1: the code

- `Animation_Data.h/.cc`: as in section 4.
- A unit test for `VehicleSurfaceDirections`: 8, 32, and another count.
- Check in the game, with a copy of `APC.STI` as `Data\Anims\Vehicles\HUMMER2.STI`:
  - The Hummer drives and turns through all 8 directions, in tactical, turn-based and real time.
  - No "Wrong Number of frames" warning in the log.
  - Removing the copy: the normal Hummer, as smooth as before.
  - The ice cream truck, the helicopter, the enemy tanks and the player's tank unchanged.
  - Then `VAN1`, `4X4`, `RED_CAR1`, each under the same name.

### Stage 2: footprints, no code

Tests in the game, with copies in the user's `Data`. A JSD copied as
`Data\Anims\STRUCTDATA\HMMV.JSD` changes the footprint of all four cars, enough for a test.

- For each graphic worth keeping: does it sink into walls and trees, is it hidden correctly
  behind buildings and in front of them, do clicks hit it, do bullets?
- `APC.JSD` with the APC: does the hit points byte 0x00 or the missing `STRUCTURE_TRANSPARENT`
  change anything, such as line of sight past it?
- Results written here: which graphic fits which JSD. They are the input for the later plan of
  E2.

### Stage 3: Jeep and Eldorado's graphics (E3)

- `Animation_Data.cc`: `ELDORADO_BASIC` to `eldrdo.sti`, `JEEP_BASIC` to `jeep.sti`.
- `Animation_Control.cc`: `gubAnimSurfaceIndex[ELDORADO]` and `[JEEP]`, `STANDING` and `WALKING`,
  to these surfaces.
- Check: place each in a sector for a test and look at all directions. Footprint `HMMV.JSD`,
  to see whether it fits, above all the Eldorado at 159 wide.

## 6. Later, not in this plan

- New vehicles of their own (E2, another stage of the project): body type, footprint, wreck (`WRECK.STI` isn't used by anything
  and may serve), portrait, panels.
- The player's tank: `docs/tank-rotating-hull-plan.md` plans new art with 32 directions. After
  stage 1 `APC.STI` could stand in for the hull as a test, with only a new file. Firing would
  still show the tank's own firing frames.

## 7. Risks

| Risk | Note |
|---|---|
| A file with 8 frames and another layout | Only 8 and 32 count as directions. One with 8 frames that aren't directions would be drawn wrongly: none among these files |
| The order of the frames | Looked at for APC, VAN1, 4X4. The others are to be seen in stage 1 |
| A surface shared by bodies | `HUMVEE_BASIC` serves Hummer, Eldorado and Jeep until stage 3: a swapped file shows on all three |
| What hides a big vehicle | Depends on the JSD's tiles and the picture together. Only the game shows it: stage 2 |
| Shadows | Drawn in the frames themselves, as in the 32 direction files. To be seen in stage 1 |
