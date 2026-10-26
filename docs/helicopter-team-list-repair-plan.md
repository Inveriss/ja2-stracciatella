# Plan: helicopter on the team list, with Repair

Status: **plan only**, not implemented (requested 2026-10-26 while the user
worked over Remote Control; the change is too big to make untested).
Decisions D1–D6 taken by the user on 2026-10-26 (section 5). Branch
`Vehicles`. Background: [vehicles-current-implementation.md](vehicles-current-implementation.md).

## 1. Goal

Make the helicopter behave like the Hummer in the map screen:

1. once Skyrider's helicopter is available, it shows on the team list in the
   vehicle rows (`newgoldpiece3_*`), with its condition;
2. a merc can take the **Repair** assignment and pick the helicopter, which
   restores its condition.

## 2. How it works today (the gap)

| | Hummer | Helicopter |
|---|---|---|
| `VEHICLETYPE` | yes | yes (`iHelicopterVehicleId`) |
| `SOLDIERTYPE` | yes (`SOLDIER_VEHICLE`, on `OUR_TEAM` after the first merc gets in) | **none** |
| team list row | yes (`AddCharacter` → from `FIRST_VEHICLE`) | **no** — the list holds `SOLDIERTYPE*` only |
| condition | `bLife` of its soldier | **`gubHelicopterHitsTaken`** (0–3), never goes down |
| damage | bullets / explosions in tactical | SAM hits in flight: hit 1 = quote, hit 2 = forced return to base, hit 3 = destroyed with everyone aboard |
| repair | `DoesVehicleNeedAnyRepairs` / `RepairVehicle` on `bLife` | explicitly **excluded** ("Skyrider isn't damagable/repairable") |

So in the original game helicopter damage is **permanent**: after two SAM hits
the next one is fatal, forever. Repair gives this a real meaning.

## 3. Recommended design (variant A): a "shell" soldier for the helicopter

Give the helicopter a `SOLDIERTYPE` that exists only for the strategic side,
like the Hummer's, but **never placed in tactical**.

- Created in `SetUpHelicopterForPlayer()` right after `AddVehicleToList(...,
  HELICOPTER)`: player team, profile `PROF_HELICOPTER` (163, already a
  `VEHICLE` profile), flag `SOLDIER_VEHICLE`, `bVehicleID = iHelicopterVehicleId`
  (`fUseGivenVehicle`, so no second vehicle slot is created), `bInSector =
  FALSE`, `sGridNo = NOWHERE`, condition `bLife = bLifeMax = 100`.
- Then the existing code gives most of the rest for free: team list row
  (`AddCharacter` → vehicle rows, scrolling already exists), status line
  "condition X/Y", Repair menu listing (`CFOR_EACH_VEHICLE`), repair points
  (`RepairVehicle`), save/load (saved with the team).
- `GetSoldierStructureForVehicle(helicopter)` starts to work; the existing
  `IsHelicopter` branches stay, since the helicopter still has no tactical body.

Variant B (rejected): a special non-soldier row in the team list. The team
list, info panel, assignment menus and repair menu all work on `SOLDIERTYPE*`;
a second kind of entry would touch far more code.

## 4. Changes by stage

### Stage 1 — shell soldier + team list row

- `Map_Screen_Helicopter.cc`: create the shell in `SetUpHelicopterForPlayer`;
  remove it in `SkyriderDestroyed` (and wherever the helicopter is removed).
- A body type is needed to create a soldier; use an existing vehicle body
  (e.g. `HUMVEE`) with `fUseGivenVehicle`; it is never rendered.
- Make sure nothing puts it into tactical: arrival code already skips the
  helicopter (`IsHelicopter` branch in `GroupArrivedAtSector`); check
  `UpdateMercInSector`, sector loading (`AddSoldierToSector` for the player
  team), tactical placement, autoresolve, squads (no squad for the helicopter,
  `AddCharacterToUniqueSquad` is only called for non-helicopters).
- Team list: the row's location/destination/assignment columns for the
  helicopter (sector of `GetHelicopter()`, destination of its path).
- Selecting the helicopter row and plotting a route: either forward to the
  existing Skyrider route plotting or block it (decision D5).
- Name and portrait: `zVehicleName[HELICOPTER]`, `giCarPortraits[HELICOPTER]`
  (check the portrait exists for index 5).

### Stage 2 — condition instead of hit count

- Replace `gubHelicopterHitsTaken` by the shell's `bLife` (D2): **each SAM hit
  −25 condition** (`HELI_SAM_HIT_DAMAGE` 25). From full condition:

  | Hit | Condition after | Effect |
  |---|---|---|
  | 1 | 75 | quote `HELI_TOOK_MINOR_DAMAGE` |
  | 2 | 50 | quote `HELI_TOOK_MINOR_DAMAGE` |
  | 3 | 25 | quote `HELI_TOOK_MAJOR_DAMAGE`, `MakeHeliReturnToBase()` |
  | 4 | 0 | the original crash: quote `HELI_GOING_DOWN`, crash sound, `SkyriderDestroyed()` — everyone aboard dies |

- The effect is chosen from the **condition after the hit** (D2 update,
  2026-10-26), so a repaired or partly repaired helicopter follows the same
  rules:

  | Condition after the hit | Effect |
  |---|---|
  | 100–50 | minor damage quote |
  | 49–25 | major damage quote + return to base |
  | 24 and below | the original crash, everyone aboard dies |

  Examples: 82 → 57 minor quote; 70 → 45 major quote + return to base;
  35 → 10 crash. Condition never goes below 0. This replaces the original
  hit-count rule (quote, return, crash on hits 1, 2, 3).
- The vehicle damage path (`VehicleTakeDamage`, which sets life < `OKLIFE` to 0)
  is not used for SAM hits; the helicopter logic changes `bLife` itself.
- Save: `gubHelicopterHitsTaken` stays in the save record (unused or derived);
  per the no-save-compatibility rule a new game may be needed.

### Stage 3 — Repair

- `DoesVehicleNeedAnyRepairs`: drop the "helicopter is never damaged" early
  return (use `bLife < bLifeMax`).
- `CanCharacterRepairVehicle` / `IsThisVehicleAccessibleToSoldier`: for the
  helicopter, `OKUseVehicle` means "can fly" (`CanHelicopterFly`), which is
  false e.g. when Skyrider is owed money or the heli is returning to base.
  For repair use its own rule (decision D3): merc in the helicopter's sector,
  helicopter **landed** (`!fHelicopterIsAirBorne`), not between sectors,
  not destroyed.
- Repair speed (D4): the helicopter is slower to repair — **20 repair points =
  1 condition point** (new `HELICOPTER_REPAIR_POINTS_DIVISOR` 20; the Hummer
  keeps `VEHICLE_REPAIR_POINTS_DIVISOR` 10). `RepairVehicle` picks the divisor
  by `IsHelicopter(v)`.
- `Faces.cc` rounds the shown repair points to a multiple of
  `VEHICLE_REPAIR_POINTS_DIVISOR`; it must use the same per-vehicle divisor.
- Flight lock (D6): **below 25 condition the helicopter can't take off** until
  repaired — a new check in `CanHelicopterFly()` / `CanHelicopterTakeOff()`
  with a Skyrider message why. Note: with the D2 update a hit that leaves
  1–24 destroys the helicopter, so a flying-capable helicopter never has less
  than 25 after a SAM hit; the lock is a safety net only (e.g. for a condition
  set some other way).

### Stage 4 — limits, texts, tests

- Team slot: the shell takes one `PLAYER_TEAM_SIZE` slot like the Hummer
  (`PlayerTeamFull` counts vehicles) — decision D1.
- Status line: condition only, no fuel (the helicopter has no fuel model) —
  like the robot's line.
- Translations: only if new messages are added (e.g. "the helicopter must be
  landed to be repaired").
- Tests in the game: get Skyrider, check the row; fly over a SAM sector to take
  hits (75 / 50 / 25 and the quotes, return to base after the 3rd); land;
  repair with a mechanic (20 points per condition point); take-off lock below
  25; destroy the heli (4th hit from full) and check the row disappears;
  save/load.

## 5. Decisions (taken 2026-10-26)

| # | Question | Decision |
|---|---|---|
| D1 | Does the helicopter take a team slot (like the Hummer)? | **yes**, same as Hummer (as recommended) |
| D2 | Damage per SAM hit and effects | **−25 per hit**; effect by condition after the hit: **100–50** minor damage quote, **49–25** major damage quote + return to base, **24 and below** the original crash, everyone dies. From full condition: hits 1–2 minor, hit 3 major + return, hit 4 crash (details in stage 2) |
| D3 | Where can it be repaired? | **only landed, merc in the same sector** (any sector, not only a refuel site), as recommended |
| D4 | Repair speed | **20 repair points = 1 condition point** (slower than the Hummer's 10, because it is a helicopter) |
| D5 | Clicking the helicopter row to plot a route | **open the normal Skyrider route plotting**, as recommended |
| D6 | Does damage also stop flying? | **yes, below 25 condition** the helicopter can't take off until repaired |

## 6. Size and risk

Big: a new kind of soldier on the player team that must never enter tactical,
a changed damage model, repair rules, save content. Many loops over
`OUR_TEAM` / `SOLDIER_VEHICLE` must be checked for the helicopter case.
Estimate 4 stages, each to be built and tested in the game before the next.
