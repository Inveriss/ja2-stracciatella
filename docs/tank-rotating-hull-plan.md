# Player's tank: a hull that rotates while driving — plan

Status: **plan, not implemented, all decisions taken** (written 2026-11-03 on Remote Control, branch
`Tanks`). Read `docs/tactical-vehicle-driving-analysis.md` first for the driving
rules; the player's tank already drives by them (`c6e771cd8`).

## 1. The problem

The player's tank (profile 232, `PROF_TANK`, body `TANK_NE`) drives in every
direction in one unchanged hull position, because the original graphics rotate
**only the turret**. The Hummer and the ice cream truck rotate as a whole.

Goal: the whole tank rotates in 32 steps, so driving and turning look smooth.

## 2. What exists today

### 2.1 The two tank bodies

`TANK_NW` and `TANK_NE` are two body types of one vehicle type (`TANK_CAR`). All
game logic treats them alike (`TANK()` macro); they differ in data only:

| | `TANK_NW` | `TANK_NE` |
|---|---|---|
| Rotation (32 turret positions) | `TANK_ROT.STI` | `TNK2_ROT.STI` |
| Shot (64 frames) | `TANK_SHT.STI` | `TNK2_SHT.STI` |
| Wreck | `TK_WREK.STI` | `TK2_WREK.STI` |
| Structure | `TNK_SHT.JSD` | `TNK2_ROT.JSD` |
| Corpse type | `TANK1_DEAD` | `TANK2_DEAD` |

### 2.2 What already follows the direction

- **The picture.** `Animation_Data.cc` gives the rotation surfaces 32 directions
  and `ANIM_DATA_FLAG_NOFRAMES`; `Soldier_Control.cc` picks the frame by
  `ExtOneCDirection(ubHiResDirection)`. Frame `f` shows high resolution direction
  `(f + 28) % 32`, so frame 0 is `NORTHWEST`, frame 4 is `NORTH`, and every fourth
  frame is one of the 8 world directions. `hummer2.sti` has the same order.
- **Turning.** A vehicle turns through `ubHiResDirection` and always ends on one
  of the 8 world directions (`bDirection`), so the other 24 frames only appear
  during a turn.
- **Footprint.** Pathing (`PathAI.cc`), `VehicleFootprintTerrainOK()`, the turning
  check (`Soldier_Control.cc`) and `Overhead.cc` all take the structure by
  `pDBStructureRef[OneCDirection(direction)]`.

### 2.3 What does not

**The footprint never changes.** Parsed from `Anims.slf`:

| File | Tiles per direction | Different footprints of 8 | Extent |
|---|---|---|---|
| `HMMV.JSD` | 13-17 | 6 | varies, roughly centred on the base tile |
| `TNK2_ROT.JSD` | 18 | **1** | x 0..2, y -2..3 |
| `TNK_SHT.JSD` | 18 | **1** | x -3..2, y -1..1 |

New frames alone would show a turning tank that still blocks the same 3x6 tiles.
The footprint is also off-centre, so it could not stay as it is.

**The files are shared with the enemy.** Replacing `TNK2_ROT.STI` or its JSD also
changes the enemy's `TANK_NE` (its turret rotation would become a hull rotation).
Whether campaign maps place such tanks has not been checked.

**The shot and the wreck show the old hull position.** `TNK2_SHT.STI` has it in
all 64 frames. The wreck's direction is fixed at 7 for tanks
(`Rotting_Corpses.cc`), while the Hummer and the truck have 2.

**The sound.** `TurnSoldier` plays `TURRET_MOVE` / `TURRET_STOP` for both tank
bodies, also when the hull turns.

## 3. Decisions

| # | Question | Decision | State |
|---|---|---|---|
| D1 | The turret | **Variant A:** fixed to the hull; the tank turns as a whole towards its target. Independent turret (pre-rendered, or as two drawn layers) only if the project later moves to a higher graphics level | **decided** (user, 2026-11-03) |
| D2 | Enemy tanks | Stay exactly as they are: a new body type for the player's tank, with its own files; `TANK_NW` / `TANK_NE` untouched | **decided** (user, 2026-11-03) |
| D3 | File format | STI or PNG, both load through the same surface (`docs/png-images.md`: 32 directions supported); new surfaces listed as `FullColour` in `GetAnimationSurfaceColours()` | **decided** (user, 2026-11-03) |
| D4 | Footprint | Keep 18 tiles / 3x6 on the straight directions, centred on the base tile; diagonals modelled on `HMMV.JSD`; see section 5 | **decided** (user, 2026-11-03) |
| D5 | Wreck | First the existing `tk2_wrek.sti` in its one position; later 2 directions, as the Hummer | **decided** (user, 2026-11-03) |
| D6 | Turning sound | No turret sounds for the new body; the driving sound stays as it is | **decided** (user, 2026-11-03) |
| D7 | Firing | Not in this plan: the player's tank has no firing yet. Only the art format is fixed here; the rules (ammunition, AP, who fires, interface) get their own plan | **decided** (user, 2026-11-03) |

Save compatibility: not kept, as in the whole project; test with a new game.

## 4. Art needed

With variant A, per tank model:

| Graphic | Frames | Notes |
|---|---|---|
| Hull rotation | **32** | single frame per direction; order as in `hummer2.sti` |
| Shot | **64** | 8 directions x 8 frames, one per world direction; order as in `TNK2_SHT.STI` |
| Wreck | 1, later 2 | D5 |

No turret rotation frames are needed, and no shot frames for the 24 in-between
directions, since a tank never stands or fires in those.

Requirements for the 32 frames:
- one rotation axis, kept at the same place on the base tile in all frames (the
  frames' offsets must carry this, or the tank drifts while turning);
- the sprite must match the footprint of section 5 on the 8 world directions;
- temporary art is fine: section 6, stage 2 uses a placeholder until it exists.

Check the direction of each frame against `hummer2.sti`; note that the game's
`NORTH` is not straight up on the screen.

## 5. The new structure file

A new `Data\Anims\STRUCTDATA\TNK3_ROT.JSD`, generated by a script, as was done
for `HELI.JSD`:

- format: 16 byte header, then per direction a 16 byte `DB_STRUCTURE` and a 32 byte
  `DB_STRUCTURE_TILE` per tile (`Structure_Internals.h`);
- 8 structures in the order `OneCDirection()` indexes them: `NORTHWEST` first, then
  `NORTH`, `NORTHEAST`, and so on;
- tile shapes (`Shape`), `fFlags` and `ubVehicleHitLocation` copied from the tiles
  of `TNK2_ROT.JSD`, positions new; `sPosRelToBase` recomputed from x and y;
- the base tile keeps position (0, 0) in every direction.

Proposed extents, to be tuned in the game: north-south x -1..1, y -3..2;
east-west x -3..2, y -1..1; diagonals derived from `HMMV.JSD` and grown by about
one tile along the hull. A larger diagonal footprint will refuse more tight
passages, which is the main thing to judge in the game.

A tank can also fail to turn on the spot: the turning check tests the footprints
on the way to the desired direction (`Soldier_Control.cc`). See the risks.

## 6. Stages

### Stage 1: a new body type, nothing looks different

Append `TANK_PLAYER` right before `TOTALBODYTYPES` in `Animation_Data.h`. It must
go last, because map files store body type numbers. Temporarily it points at the
`TNK2_*` files, so the game should look and play exactly as now.

- `Animation_Data.h`: surfaces `TANKPL_READY`, `TANKPL_SHOOT`, `TANKPL_DIE`.
- `Animation_Data.cc`: the surfaces, `gAnimStructureDatabase` row,
  `GetAnimationSurfaceColours()`.
- `Animation_Control.cc`: walking, running, swat and crawl speed rows,
  `gubAnimSurfaceIndex`, `gubAnimSurfaceCorpseID`, the empty-string switch.
- `Soldier_Control.cc`: `gubMaxActionPoints` row (`AP_MAXIMUM`).
- `Soldier_Macros.h`: `TANK()` includes the new body.
- Direct `TANK_NW` / `TANK_NE` checks that must include it: `Soldier_Control.cc`
  (fire animations, `TurnSoldier`, the turn sounds), `Soldier_Create.cc` (2 places),
  `Soldier_Ani.cc`, `Animation_Control.cc`.
- `IsDrivingVehicleBody()` and `OK_ENTERABLE_VEHICLE`: no change needed, they use
  `TANK()` with the team.
- `enums.cc`: both directions of the name conversion.
- `Interface_Items.cc`: `gMercPreviewFrames` row, if its initialiser needs one.
- `mercs-profile-info.json`: `PROF_TANK` gets `"bodyType": "TANK_PLAYER"`.
- `scripts/enums.lua`: already outdated (it lacks `HELICOPTERBODY`, says 28 types);
  bring it up to date.
- Editor: `EditorMercs.cc` names the body; it stays out of the enemy list.

Check in the game: the A9 tank appears (the placement comes from profile 232),
can be boarded, driven and repaired, its panels open, and a wreck is left. Check
that the placement really takes the profile's body and not one stored in the map.
Enemy `TANK_NE` in the editor or a test sector stays as before.

### Stage 2: the new footprint

Generate `TNK3_ROT.JSD` (section 5) and point the new body's row at it. For a
truthful test the rotation surface may temporarily use `hummer2.sti`, whose frames
do rotate. Tune the extents by driving: straight and diagonal, along buildings,
trees and fences, near water, past other soldiers, turning on the spot.

### Stage 3: the hull graphics

Point `TANKPL_READY` at the 32 new frames (`tnk3_rot`). Verify all 8 world
directions against the footprint: mercs must not stand inside the picture, and
the picture must not stand in walls. Check the turn for drift. Drop the turret
sounds (D6).

### Stage 4: the wreck

First stays as in D5. With new art: a `TANK3_DEAD` corpse type with its entries in
both `Rotting_Corpses.cc` file lists, and the 2 direction handling already used by
`HUMMER_DEAD` and `ICECREAM_DEAD`. Corpse JSDs are read from `Data\Tilecache`.

### Stage 5: firing (own plan, D7)

Shot frames (section 4) plus the firing rules. A hull that has to turn to shoot
meets the footprint check, so the plan must say what happens when it has no room.

## 7. Before merging

- `TANK_IN_A9_FOR_TESTING` off (`Vehicles.h`).
- Build, unit tests (`gubMaxActionPoints` size test covers one of the new rows;
  the arrays sized by `TOTALBODYTYPES` do not fail on a missing row, so check each).

## 8. Risks

| Risk | Note |
|---|---|
| A missing row in a `TOTALBODYTYPES` table | Compiles silently, zero values; walk the list in stage 1 |
| Tank stuck in narrow places | Bigger diagonal footprint; turning needs room. Tune in stage 2; a tank that cannot turn must still be able to back out or let its crew out |
| Footprint and picture disagree | Only visible in the game, on every direction; stage 3 check |
| Map placements store a body type | The A9 placement could keep `TANK_NE`; stage 1 check, re-saving A9 in the editor if needed |
| Passenger and cargo code that names `TANK_NE` directly | Covered by `TANK()`; the list above comes from a search, repeat it when implementing |
