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

D6 taken 2026-10-29: **vehicles may drive in a battle sector, and the
pre-battle "everyone out of the vehicles" rule is dropped** (for the vehicles
that can drive in tactical).

Stage 2 decisions, taken 2026-10-29 as proposed:

| # | Question | Decision |
|---|---|---|
| D14 | AP per tile, turn-based | road 2 AP, off the road 3 AP (with 50 AP: about 25 / 16 tiles a turn) — **changed 2026-10-29 (variant A):** the Hummer and the ice cream truck get a merc's maximum, 25 AP (also its cap, `gubMaxActionPoints`, so the up to 5 carried over don't make 30), at 1 AP a tile on a road and 2 off it (25 / about 12 tiles a turn) |
| D15 | Getting in / out in combat | allowed on the merc's turn, at the AP getting in costs today; without the AP the usual message — **changed 2026-10-29:** getting in or out costs 5 AP (`AP_ENTER_EXIT_VEHICLE`), getting in also the walk up to it in his movement mode; the enter-vehicle cursor shows that total (`GetAPsToEnterVehicle()`, `ShowAPsToEnterVehicle()`) |
| D16 | The placement screen when arriving in a vehicle | those aboard skip it, the vehicle comes in at its map edge by itself; with everyone aboard the screen isn't shown |
| D17 | Interrupts while driving | those aboard look out with interrupts: the vehicle stops when they spot an enemy, like a walking merc, and enemies may interrupt it |

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
- **1a fix — the move cursor:** the original showed a red X for every selected
  vehicle and never plotted its path (`Handle_UI.cc`, "just show an X"), so a
  click used a stale path cost ("Path Blocked" at random); a vehicle that may
  drive now gets the usual path and the vehicle move cursor.
- **1b — terrain (D4):** `FindBestPath()` (`PathAI.cc`) already keeps
  non-person bodies out of water and away from fences, and the vehicle's
  footprint (`OkayToAddStructureToWorld()`) stops it at walls, trees, fences,
  people and other vehicles. Added: `VehicleFootprintTerrainOK()` — no tile of
  the footprint in water (shore, knee-deep, deep) or under a roof (inside a
  building). Bushes, rubble and grass stay allowed at their usual higher path
  costs, so the path prefers roads. AP costs per tile (`TerrainActionPoints()`)
  wait for turn-based driving (stage 2).
- **1c — fuel (D11):** `HandleGotoNewGridNo()` (`Overhead.cc`): each tile a
  vehicle drives into costs `VehicleTacticalFuelCost()` — `tacticalFuelPerTile`
  of `vehicles.json` (default 2 of 10000, a full tank; the strategic rate is
  about 2 a tile too) on a paved / dirt road, twice that off the road — spent
  by `SpendVehicleFuelInTactical()` (the fuel bar follows). With too little
  for the next tile it stops there and says it is out of fuel (`STR_LATE_62`,
  "The vehicle has run out of fuel and cannot continue driving.", also when
  told to drive with an empty tank); a gas can
  refuels it as before, and `CanVehicleDriveInTactical()` keeps an empty one
  from starting.
- **1d — sight, noise, camera (D5):** `HandleAtNewGridNo()` (`Overhead.cc`),
  for our driving vehicle on each new tile: an engine noise of 25
  (`VEHICLE_ENGINE_NOISE`; footsteps are 9 at most) for the enemies to hear,
  instead of footsteps; those aboard (conscious) turn as it faces and look out
  with their own sight (`HandleSight()`, their position is the vehicle's) —
  the vehicle itself still has none. The screen doesn't follow it (a follow
  when it left the screen was tried and switched off at the user's request,
  2026-10-29).
- **1e — mines, people, blocks while moving (D8, D9, D13):**
  - mines: `HandleAtNewGridNo()` sets off the pressure mines under every tile
    of its footprint (`SetOffBombsUnderVehicle()`), not only its base tile;
    the blast damages the vehicle (`VehicleTakeDamage()`) and stops it;
  - those aboard any vehicle aren't hit by blasts while it holds
    (`DamageSoldierFromBlast()`, D9; its destruction kills them as before);
  - people: already — the path goes around them and its footprint can't
    enter their tiles;
  - D13: `IsVehicleMoving()` (`Vehicles.cc`: away from its final destination
    or in a moving animation) — no getting in (`EVENT_SoldierEnterVehicle()`,
    the click on it), no getting out (the seat belts), no passing items, also
    inside it, and no cargo panel by reach (`CanPassViaVehicle()`,
    `IsVehicleInMercReach()`) while it drives;
  - save / load mid-move: verified in the game (2026-10-29).

Then stages 2–5 of section 6.

Stage 2 (turn-based driving and battles) in sub-stages:

- **2a — driving in a running battle (D3, D14, D17):** `CanVehicleDriveInTactical()`
  lets it drive in combat on our turn, for its own AP (since the D14 change:
  25, `CalcActionPoints()`); `TerrainActionPoints()` (`Points.cc`) gives a
  vehicle 1 AP a tile on a paved / dirt road and 2 off it (first 2 / 3 with
  50 AP) — used alike by the plotted path's AP and by
  the move (`ActionPointCost()`); in combat those aboard look out with
  interrupts (`HandleSight(... SIGHT_INTERRUPT)` in `HandleAtNewGridNo()`), so
  an enemy they spot stops it.
- **2b — getting in / out in combat (D15):** getting in already costs the
  walk up to the vehicle (as before); two fixes on the way: the UI lock is
  lifted before getting in selects the vehicle (`EVENT_SoldierEnterVehicle()`),
  and a click without the APs to walk up ends with the message instead of a
  search of the vehicle's tile. Getting out (`MayExitVehicleInTactical()`,
  `ChargeVehicleExit()`, `Vehicles.cc`): in combat only on our turn, with the
  merc's interrupt, for the AP of one step on flat ground (2,
  `AP_EXIT_VEHICLE`), else "Not enough action points"; for the passengers'
  seat belts and the "everyone out" belts of the ground vehicles and the
  helicopter (the ones without the APs stay aboard). **Q1 lifted
  (2026-10-29, user's request):** in combat, too, the inventories of those
  aboard and the vehicle's cargo panel open, and items pass with them and
  between them at the usual AP costs (`CanPassViaVehicle()`,
  `VehicleCargoPanelOwner()`, the portrait click, Previous / Next); D13 still
  blocks it while the vehicle drives. Fix: `IsVehicleMoving()` only counts
  its moving animation, not held by `fNoAPToFinishMove` — a vehicle out of APs
  keeps the rest of its path for the next turn, and its passengers (with
  their own APs) couldn't get out.
- **2c — arriving in a battle without getting out (D6, D16):**
  `PutNonSquadMercsInPlayerGroupOnSquads()` (`PreBattle_Interface.cc`) keeps
  those aboard a vehicle that drives in tactical (`VehicleDrivesInTactical()`:
  Hummer, ice cream truck) in it when entering the battle; the placement
  screen (`IsPlacedInTacticalPlacement()`, `Tactical_Placement_GUI.cc`) leaves
  out the vehicles and those aboard, who come in with the vehicle at its map
  edge; with nobody left to place (`AnyMercForTacticalPlacement()`) the screen
  isn't shown. Retreat already left them aboard; auto-resolve counts them as
  before (it never took them out).
- **Leaving the sector by driving (part of stage 3 / D7, 2026-10-29):** the
  exit dialog offered a driving vehicle only "Selected Merc" — its squad
  seemed empty (`NumberOfPlayerControllableMercsInSquad()` left out vehicles)
  — and leaving alone put the vehicle on a squad of its own, out of its
  movement group with those aboard, which broke the vehicle and its group.
  Now a vehicle that drives in tactical counts as a member of its squad, so
  "All Mercs in Squad" (and the go-to-sector choice, as for any squad) moves
  its movement group — the vehicle and those aboard; "Selected Merc" is
  disabled for a vehicle. The strategic movement decides where it may go (a
  car only by road; otherwise the usual "can't go" message).
  `JumpIntoAdjacentSector()` fixes: the round that takes the uncontrollable
  off their squads skips those aboard (they are in the vehicle's group, on no
  squad: it looped forever, the game hung on "OK"); the vehicle doesn't walk
  off — it leaves from where it stands at the edge (its footprint can't go
  off the map, and the edge-tile search for it was slow).
- **Stage 3 completed (2026-10-29):** besides the above, a vehicle never
  leaves through an exit grid (stairs, cave and mine entrances,
  underground): `OKForSectorExit()` refuses it, and a click there says
  "Vehicles cannot drive into mines." (`STR_LATE_63`, all nine translations;
  the game's generic "... is too far away." came up before). Where a car may go stays the strategic rule (a road
  between the two sectors, `GetSectorMvtTimeForGroup()`), not the exact tile
  of the edge; arriving, it comes in at the edge it entered from, those
  aboard with it, also into a battle (2c).
  Leaving a battle with everyone of ours aboard: the exit dialog took those
  aboard the selected vehicle for another squad (`fMultipleSquadsInSector`),
  so the go-to choice was off and "OK" left an empty tactical screen. They
  are now left out of that count, and the dialog goes to the map screen
  ("Go to Map", checked; `SetAdjacentJumpToMapScreen()` makes
  `AllMercsHaveWalkedOffSector()` take the map screen case also for a short
  way).
- **2d — fighting a vehicle:** checked in the code (2026-10-29), nothing to
  add: enemies see our vehicle (`OtherTeamsLookForMan()` — only its own sight
  is off) and may interrupt it and shoot it, the AI doesn't leave vehicles out
  as targets (`Attacks.cc`); gunfire costs it a third, explosions over 50
  double (`SoldierTakeDamage()` → `VehicleTakeDamage()`); those aboard have no
  place on the map, so shots can't reach them, and blasts don't (D9);
  destroyed, it explodes, its cargo drops damaged, those aboard die
  (`KillAllInVehicle()`), the wreck stays; those aboard count as fighting
  for the end of a battle (they can get out and fight, it can drive), so a
  battle is lost only when they are gone too. To be verified in the game.
