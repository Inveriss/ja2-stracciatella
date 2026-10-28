# Analysis: driving vehicles on the tactical map

Status: **analysis and plan only**, not implemented (2026-10-28, written over
Remote Control, no code changed). Branch `Vehicles`. To be implemented after
the plans written before it:
[helicopter-passengers-tactical-plan.md](helicopter-passengers-tactical-plan.md)
and [skyrider-at-refuel-sites-plan.md](skyrider-at-refuel-sites-plan.md).
Decisions D1–D13 **taken 2026-10-28, all as proposed** (D2 / D13 made
concrete, section 4); implementation started (section 8). Background:
[vehicles-current-implementation.md](vehicles-current-implementation.md) (section 8).

The original developers ran out of time and money for driving vehicles in
tactical. The engine still has a large part of the work, blocked at one place.
Scope here: the ground vehicles — Hummer, ice cream truck — and the tank. The
helicopter is out of scope (it never drives).

## 1. What exists in the code today

| Piece | Where | State |
|---|---|---|
| **The block** | `HandleMoveModeInteractiveClick()` (`Turn_Based_Input.cc`): a selected vehicle gets `VEHICLE_CANT_MOVE_IN_TACTICAL` | the only entry point that stops driving |
| Multi-tile pathing | `PathAI.cc`, `#define VEHICLE` (line 75) is on; blocks at ~470, 650–700, 816, 884, 1192: the vehicle's footprint (its JSD structure) is checked on every tile and every turn, continuous turning for vehicles / monsters / animals (`fContinuousTurnNeeded`) | compiled, used today only by multi-tile creatures (queen, etc.); untested for vehicles since 1999 |
| Footprint checks while moving / turning | `Overhead.cc` (~3068, 3125: `SOLDIER_MULTITILE`), `Soldier_Control.cc` (~3818, 7790: `OkayToAddStructureToWorld` per direction), `UpdateMercStructureInfo()` | in place |
| 32-direction turning | `Soldier_Control.cc`: `ubHiResDirection`, `Dir2ExtDir()`, `QuickestDirection(..., 16)` (~3935), the vehicle branch of the turning step (~4423), sprite frame by `ExtOneCDirection()` for 32-direction surfaces (`ConvertAniCodeToAniFrame()`) | in place, but see 2.2 |
| Movement speed | `gubAnimWalkSpeeds[]` (`Animation_Control.cc`): `{-10, 4.0f}` for HUMVEE, TANK_NW/NE, ELDORADO, ICECREAMTRUCK, JEEP; `WALKING` maps to the vehicles' standing surface (the sprite slides, no wheel frames) | in place |
| Engine sound | `HandleVehicleMovementSound()` (`Vehicles.cc`), started / stopped in `Soldier_Control.cc` (~1296, 1306); `moveSound` per vehicle in `vehicles.json` | in place |
| Passengers follow | `EVENT_InternalSetSoldierPosition()` → `UpdateAllVehiclePassengersGridNo()` (`Soldier_Control.cc` ~1846, `Vehicles.cc`) | in place |
| Cursors | `MOVE_VEHICLE_UICURSOR` (`CURSOR_DRIVEV`), `CONFIRM_MOVE_VEHICLE_UICURSOR`, `ALL_MOVE_VEHICLE_UICURSOR`, the tile marker `VEHICLEMOVE1` (`Interface_Cursors.cc`, `Handle_UI.cc` ~3666, 3721, 3750) | in place |
| Path plotting | vehicle footstep graphics in the plotted path (`PathAI.cc` ~2239) | in place |
| Popup menu | the movement popup shows "Drive" for a vehicle (`DRIVE_POPUPTEXT`, `Interface.cc` ~461); the action button is "N/A — until we get mounted weapons" | in place |
| AP | vehicles +35 AP (`CalcActionPoints`), cap `AP_VEHICLE_MAXIMUM` 50; the driver's AP shown is the vehicle's | in place |
| Stance | vehicles can only "stand" (`IsValidStance`) | in place |
| Graphics | Hummer `hummer2.sti`, ice cream truck `icecrm.sti`: 32 directions, one frame each; tank `tank_rot.sti` / `tnk2_rot.sti` (32 directions), `tank_sht.sti` / `tnk2_sht.sti` (shooting, 8 directions); wrecks `hm_wrek.sti`, `tk_wrek.sti`, `tk2_wrek.sti`; corpses `HUMMER_DEAD`, `ICECREAM_DEAD`, `TANK1/2_DEAD`; structure `anims/STRUCTDATA/HMMV.JSD` (8 directions) | in place |
| Fuel | `sBreathRed` of the vehicle's soldier = fuel (10000 = full); spent only by strategic travel (`SpendVehicleFuel()`, `Strategic_Movement.cc`); gas can refuelling in tactical (`AddFuelToVehicle()`) | strategic only |

## 2. What is missing or wrong — (a) the code to write

### 2.1 Open the entry point
`HandleMoveModeInteractiveClick()`: let a vehicle move when it may (2.4), with
the normal path / AP / confirm flow; keep the message for the cases it may not.

### 2.2 Turning
`TurnSoldier()` (`Soldier_Control.cc` ~4239) returns at once for every vehicle
except the tanks ("If we are a vehicle... DON'T TURN!"), so a car would never
finish a turn on its path. That return must go for driving vehicles; the
32-direction turning step behind it is already written.

### 2.3 Fuel instead of breath
- Movement costs breath (`TerrainBreathPoints()`, `DeductPoints()` in
  `Points.cc`). For a vehicle `sBreathRed` is the **fuel**, with the opposite
  meaning (high = full) — breath costs would change it the wrong way. A vehicle
  branch: no breath cost, a fuel cost per tile instead (`SpendVehicleFuel()`
  style), and no move when the tank is empty (message).
- Its bars already show fuel (team panel, map screen).

### 2.4 Who may drive, selection, AP
- A vehicle moves only with a driver aboard (`SOLDIER_DRIVER`) who can drive
  (`OKUseVehicle` profile facts, not an EPC, `bLife >= OKLIFE`, not too tired —
  the rules of `SoldierMustDriveVehicle()` / the driver check in `Vehicles.cc`).
- Selecting the vehicle: clicking the driver's portrait already selects it
  (`HandleLocateSelectMerc(&vs)`); our team panel vehicle windows (stage 4c)
  could select it too.
- AP: the vehicle's own (+35, max 50) or the driver's (decision D3).

### 2.5 Terrain rules
`TerrainActionPoints()` / the path cost functions treat a vehicle like a
person. A vehicle cost table is needed: roads, dirt roads, flat and grass
yes; bushes / rubble slow or no; water (shore, knee-deep, deep), fences,
stairs, roofs, doors (inside buildings) never. The footprint check already
stops it at walls, trees, other vehicles and people.

### 2.6 Sight while driving
Our vehicles have no sight (`OppList.cc` ~1401, 3614), and the passengers are
off the map — today a moving vehicle would drive blind (fog, no enemies
spotted, no interrupts for them). Needed: the occupants look from the
vehicle's position (`HandleSight()` for the passengers after each tile), or
the vehicle looks with the best occupant's sight (decision D5).

### 2.7 Noise, interrupts, enemies
- Engine noise for the enemies to hear (`MakeNoise()` per tile, a loud
  vehicle noise).
- Interrupts: the enemies may interrupt a moving vehicle like a moving merc;
  the passengers as observers (D5).
- Enemies shooting at it: the vehicle damage code exists
  (`SoldierTakeDamage()` → `VehicleTakeDamage()`); the occupants' damage /
  bailing out are decisions (D9).

### 2.8 On the way
Mines, traps, items and corpses on the path, doors, other people blocking
(no running over — D8), the camera following the vehicle, stopping the move.

### 2.9 Combat and battles
- Turn-based movement through the normal AP flow.
- Today entering a battle sector takes everyone out of the vehicles
  (`PutNonSquadMercsInBattleSectorOnSquads(TRUE)`, pre-battle interface) and
  the vehicle doesn't take part (`PlayerMercInvolvedInThisCombat`). Driving in
  combat changes that (D6).

### 2.10 Leaving the sector by driving
Driving to a map edge / exit: the strategic side only knows road travel for
cars (`CAR`). Either only at road edges into a road-connected sector, or not
at all (D7). Needs the normal sector-exit flow for a group with a vehicle.

### 2.11 Save / load
A vehicle saved mid-move or mid-turn (`ubHiResDirection`, path, passengers'
gridnos) — the soldier save already holds these fields; to be tested.

### 2.12 The tank
- Tanks are enemy-only today (editor / debug, `TANK_NW` / `TANK_NE` body
  types, `tank_rot.sti` 32-direction rotation, shooting 8 directions), the AI
  treats them as stationary gun platforms.
- A moving tank means AI movement decisions for a multi-tile unit (paths,
  cover, not blocking roads) — the biggest part. A player tank would also need
  a way to get one (the campaign has none) and mounted weapons.

## 3. Rules — (b) proposed behaviour

1. Only Hummer and ice cream truck (and the tank per D10) drive; never the
   helicopter.
2. Only with a driver aboard who can drive; passengers ride along.
3. Out of combat freely (real time); in combat per D6.
4. Road, dirt road, flat, grass: yes; water, fences, stairs, roofs, inside
   buildings: no; bushes / rubble per D4.
5. Every tile costs fuel; an empty tank stops the vehicle (refuel with a gas
   can, as now).
6. Occupants see from the vehicle (D5); the engine is loud (enemies hear it).
7. People, other vehicles, walls and trees block it (no running over, D8).
8. Mines on its path explode under it (D8).
9. Leaving the sector by driving per D7.

## 4. Decisions — (c)

| # | Question | Proposal |
|---|---|---|
| D1 | Which vehicles | Hummer and ice cream truck first; the tank as a later stage (D10) |
| D2 | Real time only, or turn-based too? | real time first (stage 1), turn-based next (stage 2) |
| D3 | Whose AP in turn-based | the vehicle's own (+35, max 50), as the engine has it; the driver's shown |
| D4 | Bushes / rubble / sand | allowed, slower (higher AP / fuel cost); water, fences, stairs, buildings never |
| D5 | Sight | the occupants look from the vehicle's position after each tile (their own sight, so a wounded / tired passenger sees less) |
| D6 | Driving in a battle sector | allowed once turn-based works; then the pre-battle "everyone out" is dropped for vehicles that can drive — a separate decision at stage 2 |
| D7 | Leaving the sector by driving | only at map edges with a road into a road-connected sector (a later stage); otherwise blocked with a message |
| D8 | People / mines on the path | people block (the path goes around them); mines explode under it and damage the vehicle |
| D9 | Occupants when it is shot | no damage to them while the vehicle holds (as now); when it is destroyed, the existing destruction rules |
| D10 | The tank | separate later stage: enemy tanks moving (AI) first; a player tank only with mounted weapons |
| D11 | Fuel cost per tile | ~ the strategic rate per tile, tuned in `vehicles.json` (e.g. `tacticalFuelPerTile`) |
| D12 | Sound | the existing `moveSound` loop; engine start / stop sounds as an option with new assets |
| D13 | A vehicle on the move (added 2026-10-28) | nobody gets in or out and no items are passed while it moves; once it stops, as now |

All taken 2026-10-28 as proposed. D2 in practice: in combat the vehicle
stays where it is (the message) until stage 2.

## 5. Assets — (d)

**Already there (enough for stage 1):** 32-direction sprites of the Hummer and
the ice cream truck (one frame per direction — the vehicle slides while
driving, as many games of that time), the 8-direction JSD `HMMV.JSD`, wrecks
and corpses, the driving cursors (`CURSOR_DRIVEV`, `VEHICLEMOVE1`), the path
footsteps, the movement sound per vehicle, the tank's 32-direction rotation
and 8-direction shooting sprites.

**Optional, for a better look (later, an animation stage):**
- driving animation frames per direction (wheels turning, body bouncing): e.g.
  4–8 frames × 32 directions per vehicle, as a new surface (`HUMVEE_DRIVE`)
  instead of sliding the standing sprite;
- dust / exhaust effects behind a moving vehicle (tile cache animations, like
  smoke);
- smoke from a damaged vehicle;
- engine start / stop / idle sounds (one-shot WAVs per vehicle);
- the tank: a hull driving animation if the turret should turn separately
  (today hull and turret are one sprite per direction);
- high-resolution PNG versions (the PNG loader supports frame offsets,
  `docs/png-images.md`).

## 6. Proposed stages

1. **Driving out of combat** (D1, D2, D4, D5, D8, D11, D12): open the entry
   point, turning, fuel, terrain table, driver rule, occupants' sight, noise,
   camera. Size: medium–big (mostly testing the 1999 multi-tile code).
2. **Turn-based driving and battles** (D3, D6, D9): AP flow, interrupts, the
   pre-battle rule. Size: medium.
3. **Leaving the sector by driving** (D7). Size: medium.
4. **The tank** (D10): AI movement for enemy tanks. Size: big.
5. **Look and sound** (assets of section 5). Size: depends on the assets.

## 7. Risks

- The multi-tile movement code was never used for cars: footprint updates per
  tile, the 32-direction sprites against the 8-direction JSD (z-strip index =
  direction / 4) and narrow roads may show problems only in the game.
- Fuel lives in the breath fields: every breath-related code path that can
  reach a vehicle must be checked (stamina regeneration, collapse, gas).
- Our earlier stages assume a parked vehicle (cargo reach 3 tiles from its
  gridno, passengers' anchor = the vehicle): they keep working while it moves,
  but passing items during a move should be blocked.

## 8. Implementation plan (2026-10-28)

Stage 1 (driving out of combat) in sub-stages, each verified in the game:

- **1a — opening it:** `HandleMoveModeInteractiveClick()`
  (`Turn_Based_Input.cc`) lets a Hummer / ice cream truck move out of combat
  with a driver aboard who can drive (`OKUseVehicle()`, conscious); otherwise,
  in combat and for other vehicles the message as before. `TurnSoldier()`
  (`Soldier_Control.cc`) lets them turn (the 32-direction step is there).
  `DeductPoints()` (`Points.cc`) charges a vehicle no breath — its breath
  field is the fuel.
- **1b — terrain (D4):** a vehicle cost table in `TerrainActionPoints()` and
  the path cost (`PathAI.cc`).
- **1c — fuel (D11):** the cost per tile, the stop with an empty tank (a new
  message), `tacticalFuelPerTile` in `vehicles.json`.
- **1d — sight, noise, camera (D5):** those aboard look from its position
  after each tile, engine noise for the enemies, the camera follows it.
- **1e — mines, people, blocks while moving (D8, D13)**, save / load mid-move.

Then stages 2–5 of section 6.
