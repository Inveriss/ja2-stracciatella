# Vehicles — current (original) implementation

Reference analysis of how vehicles work in the code on branch `Vehicles`
(state of `ja2master` d696d6a5a, 2026-10-26). Nothing here is a change; it is
the starting point for the vehicles work.

## 1. Overview

A vehicle exists in **two linked forms**:

| Form | Where | What it holds |
|---|---|---|
| `VEHICLETYPE` | `pVehicleList` (`src/game/Tactical/Vehicles.cc`) | strategic data: type, sector, movement group, strategic path, passengers, destroyed flag |
| `SOLDIERTYPE` with `SOLDIER_VEHICLE` | the normal soldier array | tactical body: position, animation, **condition = `bLife`**, **fuel = `sBreathRed`/`bBreath`**, team, AP |

Link: `SOLDIERTYPE::bVehicleID` → index in `pVehicleList`;
`GetSoldierStructureForVehicle(v)` searches the soldier array the other way.

**Exception — the helicopter (Skyrider):** it is a `VEHICLETYPE` only
(`iHelicopterVehicleId`), with **no** `SOLDIERTYPE`. Many functions check
`IsHelicopter(v)` / `InHelicopter(s)` and branch.

Passengers are ordinary mercs with `bAssignment == VEHICLE` and
`iVehicleId` = vehicle index; the first one in gets `SOLDIER_DRIVER`, the others
`SOLDIER_PASSENGER`.

## 2. Files

| File | Role |
|---|---|
| `src/game/Tactical/Vehicles.h/.cc` | core: list, add/remove, enter/exit, damage, repair, fuel init, driver rules, save/load |
| `src/game/Tactical/LoadSaveVehicleType.cc` | 128-byte save record of `VEHICLETYPE` |
| `src/externalized/VehicleModel.h/.cc` | `vehicles.json` model (sounds, profile, movement type, armour, seats) |
| `assets/externalized/vehicles.json` | the 6 vehicle types (data) |
| `src/game/Strategic/Strategic_Movement.cc` | vehicle movement groups, travel time per terrain, fuel use, out-of-gas, gas can refuel |
| `src/game/Strategic/Strategic_Pathing.cc` | vehicle path lives in `VEHICLETYPE::pMercPath` |
| `src/game/Strategic/Assignments.cc` | "Vehicle" assignment menu, repair menu (vehicles), sleep/driver rules |
| `src/game/Strategic/Map_Screen_Interface.cc` | move errors (empty / damaged / no gas), status line (condition/fuel) |
| `src/game/Strategic/MapScreen.cc` | vehicles in the team list (own rows from `FIRST_VEHICLE`) |
| `src/game/Strategic/Map_Screen_Helicopter.cc` | the helicopter: Skyrider, refuel sites, SAM danger, costs |
| `src/game/Strategic/PreBattle_Interface.cc` | battle entry: passengers leave vehicles; retreat drives away |
| `src/game/Tactical/Soldier_Create.cc` | body type → vehicle type, `AddVehicleToList` when a vehicle soldier is created |
| `src/game/Tactical/Soldier_Control.cc` | damage scaling, enter event, refuel event, AP, stance/turning rules |
| `src/game/Tactical/Turn_Based_Input.cc`, `Handle_UI.cc`, `UI_Cursors.cc` | click on vehicle → walk up and enter; cursors; "vehicle can't move" |
| `src/game/Tactical/Interface_Panels.cc`, `Interface.cc`, `Interface_Utils.cc` | team panel (exit button in hand slots), vehicle portrait, popup with Drive button |
| `src/game/Tactical/Weapons.cc`, `LOS.cc`, `OppList.cc` | armour vs bullets, hit location, sight |
| `src/game/Tactical/Squads.cc` | vehicle as a squad member, squad rules |

## 3. Data model

### 3.1 Vehicle types (`Vehicles.h`)

```
ELDORADO_CAR = 0, HUMMER, ICE_CREAM_TRUCK, JEEP_CAR, TANK_CAR, HELICOPTER
```

The order is hard-coded: `vehicles.json` must have at least these 6 entries in
this order (`VehicleModel::validateData`); HELICOPTER must be index 5.

### 3.2 `vehicles.json` fields → `VehicleModel`

| Field | Use |
|---|---|
| `enterSound` | played on enter/exit (`AddSoldierToVehicle`, `ExitVehicle`) |
| `moveSound` | looped while moving in tactical (`HandleVehicleMovementSound`) |
| `profile` | profile of the vehicle (must be type `VEHICLE` or `NOT_USED`); used for name/face, `OKUseVehicle` |
| `movementType` | `FOOT`/`CAR`/`TRUCK`/`TRACKED`/`AIR` → `GROUP::ubTransportationMask` |
| `armourType` | armour item used as the vehicle's protection against bullets |
| `seats` | max passengers, ≤ `MAX_PASSENGERS_IN_VEHICLE` (10) |

Current data: all 5 ground vehicles are `CAR` with 6 seats; Hummer and Tank use
`SPECTRA_VEST` armour, the others `KEVLAR_VEST`; Jeep and Tank point to the
placeholder profile `NPC164`. Only Hummer, Ice cream truck and Helicopter are
used in the vanilla campaign.

Vehicle profiles (`mercs-profile-info.json`, type `VEHICLE`): 160
`PROF_HUMMER`, 161 `PROF_ELDERODO`, 162 `PROF_ICECREAM`, 163 `PROF_HELICOPTER`.

### 3.3 `VEHICLETYPE`

```
PathSt*   pMercPath        // strategic path (vehicle owns the group's path)
UINT8     ubMovementGroup  // strategic movement group
UINT8     ubVehicleType    // index into vehicles.json
SGPSector sSector
BOOLEAN   fBetweenSectors
INT16     sGridNo          // tactical position (set at creation)
SOLDIERTYPE* pPassengers[10]
BOOLEAN   fDestroyed
UINT32    uiMovementSoundID
BOOLEAN   fValid           // slot in use
```

Limits: `MAX_VEHICLES` 10 (pre-created movement groups),
`MAX_PASSENGERS_IN_VEHICLE` 10.

### 3.4 Vehicle stats stored in `SOLDIERTYPE`

| Meaning | Field | Notes |
|---|---|---|
| condition | `bLife` / `bLifeMax` | < `OKLIFE` → can't travel; 0 → destroyed |
| fuel | `sBreathRed` 0..10000, `bBreath` 0..100 | 10000 = full tank |
| kind | `ubWhatKindOfMercAmI = MERC_TYPE__VEHICLE`, flag `SOLDIER_VEHICLE` | |
| body | `ubBodyType` HUMVEE, ELDORADO, ICECREAMTRUCK, JEEP, TANK_NW, TANK_NE | |

## 4. Lifecycle

### 4.1 Creation

- **New game:** `InitVehicles()` creates 10 persistent vehicle movement groups
  (`gubVehicleMovementGroups`), all with `ubTransportationMask = CAR`.
- **Ground vehicles come from maps:** a vehicle is placed in a sector map as a
  soldier with a vehicle body type (civilian team). When that soldier is created
  (`Soldier_Create.cc`), the body type picks the vehicle type,
  `AddVehicleToList()` gets a slot (reusing the slot's pre-made group and setting
  its transport mask from `movementType`), and `SetVehicleValuesIntoSoldierType()`
  sets name, profile, full fuel and `MERC_TYPE__VEHICLE`. Hummer, Eldorado and
  Ice cream truck are created `bNeutral`.
- **Helicopter:** `SetUpHelicopterForPlayer()` → `AddVehicleToList(..., HELICOPTER)`
  when Skyrider is set up.
- **Debug:** a cheat key in `Turn_Based_Input.cc` turns the selected soldier into
  a tank body with a MINIMI, registered as a HUMMER vehicle.

### 4.2 Ownership / permission to use

`OKUseVehicle(profile)`:

- Hummer → fact `FACT_OK_USE_HUMMER` (set by NPC action
  `NPC_ACTION_TIMER_FOR_VEHICLE`, scheduled for the next day from NPC dialogue);
- Ice cream truck → `FACT_OK_USE_ICECREAM` (set by NPC scripts; Hamous and the
  truck move together between sectors until the player has the truck or Hamous);
- Helicopter → `CanHelicopterFly()`;
- everything else → allowed.

The vehicle soldier stays on the civilian team until the first merc gets in:
`AddSoldierToVehicle` then calls `ChangeSoldierTeam(vs, OUR_TEAM)`. From then on
it occupies a **player team slot** (`PlayerTeamFull` counts vehicles;
`NumberOfMercsOnPlayerTeam` does not).

### 4.3 Entering

Paths into a vehicle, all ending in `PutSoldierInVehicle` → `AddSoldierToVehicle`:

- **Map screen:** Assignment → Vehicle → pick a vehicle (`DisplayVehicleMenu`).
  Only in map mode, vehicle in the same sector and not between sectors, usable
  (`OKUseVehicle`), no battle in the loaded sector, merc not EPC-restricted /
  mechanical (`CanCharacterVehicle`). Full vehicles are shaded.
- **Tactical:** clicking a vehicle (`CheckForAndHandleHandleVehicleInteractiveClick`)
  walks the merc next to it (pending action `MERC_ENTER_VEHICLE`), then
  `EVENT_SoldierEnterVehicle`. Only `OK_ENTERABLE_VEHICLE` = not a tank and
  condition ≥ `OKLIFE`; costs the walking AP only.

What `AddSoldierToVehicle` does:

1. (not helicopter) switch the vehicle to the player team; if the vehicle was
   empty, put the vehicle soldier into its own new squad
   (`AddCharacterToUniqueSquad`) and into the vehicle movement group;
2. put the merc in the first free seat; take him out of any other vehicle,
   squad or group;
3. assignment → `VEHICLE`, `iVehicleId` set, merc added to the vehicle group;
4. first passenger → `SOLDIER_DRIVER`, others → `SOLDIER_PASSENGER`;
5. merc removed from the map grid and moved onto the vehicle's position; enter
   sound; in tactical the vehicle's squad becomes current.

So the **vehicle is a squad member** and its passengers belong to that squad
indirectly (`IsSoldierInThisVehicleSquad`). A squad that contains a vehicle
accepts only drivers/passengers/the vehicle itself (`Squads.cc`).

### 4.4 Exiting

- **Tactical:** clicking the second hand slot of a passenger in the team panel
  (`TMClickSecondHandInvCallbackPrimary` → `ExitVehicle`). The merc is placed on
  a free tile near the vehicle (sweet-spot search radius 5, then 20), put in a
  new unique squad and selected.
- **Elsewhere:** `TakeSoldierOutOfVehicle` → `RemoveSoldierFromVehicle` (no
  tactical placement). Used when assigning to a squad, other assignments,
  contract end, capture, etc.
- When the **last** passenger leaves a ground vehicle: its path is cancelled; if
  it was between sectors it is **teleported to the nearer of the two sectors**
  (`TeleportVehicleToItsClosestSector`); it is removed from its squad, put back
  into its vehicle group and its assignment is cleared.
- Leaving the helicopter marks the sector as visited and sets insertion data
  (Drassen airport uses a fixed gridno).

## 5. Strategic movement (map screen)

- The vehicle and passengers form **one movement group** (`fVehicle = TRUE`); the
  path belongs to the vehicle (`MoveCharactersPathToVehicle`,
  `SetUpMvtGroupForVehicle`).
- **Can it move?** (`CanCharacterMoveInStrategic`):
  - empty → `ME_VEHICLE_EMPTY` (needs a driver),
  - condition < `OKLIFE` → `ME_VEHICLE_DAMAGED`,
  - fuel 0 → `ME_VEHICLE_NO_GAS`,
  - plus all normal checks (combat, enemies in sector, robot without its
    controller in the vehicle, EPC without a non-EPC in the vehicle, too tired…).
- **Who drives?** `SoldierMustDriveVehicle`: when the vehicle is travelling and
  exactly one passenger can drive (not the helicopter, not mechanical, not EPC,
  life ≥ `OKLIFE`, breath max above minimum), that merc can't sleep. With several
  possible drivers, nobody is forced.
- **Travel time** (`GetSectorMvtTimeForGroup`, minutes per sector):

  | Mask | Base | Allowed terrain (speed %) |
  |---|---|---|
  | FOOT | 89 | road 100, plains 85, sparse 70, dense 60, sand 50, hills 50, swamp 35, water/rivers 25 |
  | **CAR** | **30** | **road only** |
  | TRUCK | 32 | road 100, plains 75, sparse 60, hills 50 |
  | TRACKED | 46 | road/plains 100, sand 70, sparse 60, hills 60, rivers 20, water 10 |
  | AIR | 10 | everywhere, same time (also over towns) |

  Town sectors: 5 minutes for everything except AIR. A group uses the slowest of
  its masks. All current ground vehicles are `CAR` → roads only.
- **Fuel use:** on arrival in each sector (not the helicopter)
  `fuel -= traverse_time × 6` (`SpendVehicleFuel`). Example: a road sector (30
  min) costs 180 of 10000 → about 55 road sectors per full tank. At 0 the
  "out of gas" message is shown and the path is cleared.
- **No travel experience** is given to mercs riding in a vehicle (only walking
  gives it).
- On arrival the vehicle soldier and the passengers get the sector, insertion
  direction and code; if the sector is loaded they are put into tactical.

## 6. Fuel

- Full tank = `sBreathRed` 10000 (`bBreath` 100). Shown on the map screen as
  "fuel X/100".
- **Gas can (`GAS_CAN`) in tactical:** merc with a gas can in hand, cursor
  `REFUELCURS` over the vehicle, cost = crouch AP + `AP_REFUEL_VEHICLE` (10),
  animation `REFUEL_VEHICLE`, then `AddFuelToVehicle`: each status point of the
  can = 50 fuel (a 100 % can = half a tank); the can loses status, an empty can
  is deleted.
- **NPC refuel:** `NPC_ACTION_FILL_UP_CAR` fills the Hummer (or the ice cream
  truck if the Hummer is full / missing) to 100 %.
- Saves older than version 74 get full fuel on load (`AddVehicleFuelToSave`).

## 7. Condition, damage and repair

- **Damage** (`SoldierTakeDamage` for a vehicle soldier):
  - gunfire damage ÷ 3, explosion damage above 50 × 2 (tanks: unchanged);
  - hand-to-hand and gas do nothing;
  - armour: bullets are reduced by the armour item from `armourType`
    (`Weapons.cc`, `GetVehicleArmour`);
  - `VehicleTakeDamage` → `HandleCriticalHitForVehicleInLocation`: life drops, the
    damage number is shown; life < `OKLIFE` is set to 0.
- **Destroyed** (life 0): `fDestroyed`, a `GREAT_BIG_EXPLOSION` at the vehicle,
  death handling (`VEHICLE_DIE` animation, single-direction vehicle corpse) and
  **every passenger is killed** (`KillAllInVehicle`, 100 damage each).
- **Battle sounds of a vehicle** (e.g. "OK" on a command) are spoken by a random
  passenger (`PickRandomPassengerFromVehicle`); passengers take no damage from
  hits on the vehicle until it is destroyed.
- **Repair:** assignment Repair → vehicle (map screen) or toolkit click on the
  vehicle in tactical (sets the same assignment). Each 10 repair points
  (`VEHICLE_REPAIR_POINTS_DIVISOR`) restore 1 condition point. Allowed when the
  vehicle is not destroyed, damaged, accessible to the merc. The helicopter can't
  be damaged or repaired this way.

## 8. Tactical behaviour

- **Vehicles cannot be driven in tactical.** Moving a selected vehicle gives the
  message `VEHICLE_CANT_MOVE_IN_TACTICAL` (`HandleMoveModeInteractiveClick`).
  There is leftover driving support (Drive button text, vehicle move cursors,
  continuous turning in `PathAI.cc`, 32-direction turning, movement sound), but
  the entry point is blocked.
- Vehicle soldiers: only stance "stand", no turning by command, no attack
  (popup action button is "N/A — until we get mounted weapons"), no sight for
  player vehicles (`OppList.cc`), +35 AP and a 50 AP cap (`AP_VEHICLE_MAXIMUM`),
  no breath regeneration, no lights for passengers.
- Passengers are hidden from the map grid and follow the vehicle's position
  (`UpdateAllVehiclePassengersGridNo`). In the team panel their hand slots show
  vehicle icons (`inventor.sti`): the second one is the **exit** button; a
  driver's AP shown is the vehicle's AP.
- The vehicle's portrait is drawn from `giCarPortraits[type]`.
- AI ignores empty vehicles; enemies can shoot vehicles; enemy vehicles (jeep,
  tank) exist only as body types (editor / debug), not in the campaign.

## 9. Battles

- **Entering a battle sector:** the pre-battle interface takes everyone out of
  the vehicles (`PutNonSquadMercsInBattleSectorOnSquads(TRUE)`); each vehicle's
  passengers go to one new empty squad (needed because simultaneous groups may
  arrive from different edges). The vehicle itself does not take part
  (`PlayerMercInvolvedInThisCombat` excludes `SOLDIER_VEHICLE`); in tactical
  placement vehicles are skipped.
- **Retreat:** passengers stay inside and the vehicle drives back
  (`RetreatAllInvolvedPlayerGroups`, `fExitVehicles = FALSE`); empty vehicle
  groups don't retreat.
- **Auto-resolve:** vehicles are not combatants.
- **Capture:** a vehicle soldier is never captured; captured mercs are taken out
  of their vehicle first.
- Enemy forces ignore vehicle groups when counting player groups in a sector.

## 10. Map screen UI

- Team list: vehicles have their own rows starting at `FIRST_VEHICLE`
  (= `PLAYER_TEAM_SIZE`), at most 10.
- Status line for a vehicle: condition X/Y, fuel X/Y.
- Assignment menu: "Vehicle" line (shaded if not possible) → popup with the
  accessible vehicles; Repair menu lists vehicles in the sector (shaded if not
  repairable).
- Moving vehicles are listed in the "moving" box with `*name*` when moving.

## 11. Save / load

- `pVehicleList`: count (UINT8) + per slot `fValid`, then a 128-byte record
  (`LoadSaveVehicleType.cc`: path pointer placeholder, group, type, sector,
  between-sectors flag, gridno, 10 passenger **profile IDs**, destroyed, sound
  id, valid) + the strategic path.
- `gubVehicleMovementGroups` (10 × INT8).
- Vehicle condition/fuel are saved with the vehicle's `SOLDIERTYPE`.
- On load, a passenger whose assignment is not `VEHICLE` is repaired or dropped
  (workaround for issue 1885).

## 12. Hard-coded points to keep in mind for changes

1. Vehicle type enum order and `HELICOPTER == 5` (`vehicles.json` order).
2. Body type → vehicle type mapping only in `Soldier_Create.cc`.
3. Vehicle profiles 160–163 and `OKUseVehicle` facts per profile.
4. Fuel and condition live in the breath/life fields of `SOLDIERTYPE`; any new
   "breath"/"life" logic must skip vehicles (many places already check
   `SOLDIER_VEHICLE`).
5. All ground vehicles are `CAR` → road-only travel; `TRUCK`/`TRACKED` exist but
   are unused.
6. `MAX_VEHICLES` 10 (pre-made groups, team list rows) and
   `MAX_PASSENGERS_IN_VEHICLE` 10 (struct array, save record).
7. Vehicles take player team slots (`PLAYER_TEAM_SIZE`).
8. Tactical driving is blocked at one place (`HandleMoveModeInteractiveClick`);
   the old driving code behind it is untested.
9. The helicopter has no `SOLDIERTYPE`; every generic vehicle function needs its
   `IsHelicopter` branch.
