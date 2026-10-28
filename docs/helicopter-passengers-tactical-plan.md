# Plan: the helicopter's passengers on the tactical team panel (variant B)

Status: **plan only**, not implemented (2026-10-28, written over Remote Control,
no code changed). Decisions B1–B6 taken; checks C1 / C2 (section 6) come
first. Branch `Vehicles`. Follows the vehicle inventory plan
[vehicle-inventory-plan.md](vehicle-inventory-plan.md), stages 4c–4e.

## 1. Goal

On the 1366x768 team panel a left click on a ground vehicle's portrait shows
the mercs aboard it (stage 4d, "left click on a vehicle's portrait"). The same
should work for the helicopter: a left click on its portrait shows its
passengers, so that their inventories can be opened and items passed with the
rules already in place (P7, stage 4d), while the helicopter is in tactical
(stage 4e).

Variant B: **without giving the helicopter a squad**. Its strategic handling
(Skyrider's routes, landing, hovering, SAM hits) stays untouched.

## 2. Findings (code analysis)

How it works for the Hummer / ice cream truck:

- When the first merc gets into an empty ground vehicle, the vehicle's soldier
  is put on a new squad of its own (`AddSoldierToVehicle()`, `Vehicles.cc`,
  `AddCharacterToUniqueSquad(vs)`); its passengers get the assignment `VEHICLE`
  and its id (`iVehicleId`).
- `SetCurrentSquad()` (`Squads.cc`) clears the team panel slots and calls
  `CheckForAndAddMercToTeamPanel()` for every member of the squad; for the
  vehicle's soldier that function adds the vehicle's passengers
  (`CFOR_EACH_PASSENGER`) instead of the vehicle itself
  (`Interface_Panels.cc`).
- The left click on a vehicle window (`BigTeamVehicleFaceCallbackPrimary()`)
  calls `SetCurrentSquad(v->bAssignment)` when someone is aboard.

Why it doesn't work for the helicopter:

1. `AddSoldierToVehicle()` skips the helicopter on purpose
   (`if (!IsHelicopter(v))`): in the original it had no soldier. Our
   helicopter soldier (created by `CreateHelicopterSoldier()`) keeps the
   assignment `ASSIGNMENT_EMPTY`, so `SetCurrentSquad()` gets an invalid squad
   number and only the camera moves.
2. The helicopter's passengers are treated as airborne
   (`InHelicopter()`, `SoldierAboardAirborneHeli()`,
   `Map_Screen_Helicopter.h/.cc`) and are most likely **not put into the
   loaded sector** the way ground vehicles' passengers are
   (`InternalAddSoldierToSector()`: drivers/passengers only get a merc slot
   and `bInSector`, no place on the map). To be checked on the PC (section 6,
   check C1).

## 3. Design

### 3.1 A "helicopter passengers" view of the team panel

- A new team panel mode next to the squads: "the passengers of vehicle X"
  (only the helicopter uses it). It is kept in the team panel code
  (`Interface_Panels.cc`), e.g. a `gTeamPanelVehicleView` pointer to the
  helicopter's soldier, `NULL` = the normal squad view.
- Entering it: left click on the helicopter's window (when someone is aboard)
  clears the slots (`RemoveAllPlayersFromSlot()`) and fills them with the
  helicopter's passengers (`pPassengers`, in seat order), the way
  `SetCurrentSquad()` fills them from a squad.
- `iCurrentTacticalSquad` is **not** changed, so all squad logic (movement,
  selection, saving the current squad) stays as it is.
- Leaving it: Change Squad (button / Shift+Space), a click on another vehicle's
  window, any `SetCurrentSquad()` call (sector change, a merc selected from
  another squad), the helicopter leaving tactical (`UpdateHelicopterInTactical()`
  takes it out), or the helicopter becoming empty. Leaving restores the
  current squad's slots (`SetCurrentSquad(iCurrentTacticalSquad, TRUE)` /
  `RebuildCurrentSquad()`).
- Slot numbers and keys 1–9 in this view: the passengers' slots, as in a
  squad (decision B3).
- **No new panels or graphics:** the view uses the existing 1366x768 team
  panels (`BOTTOM_BAR_BIG_1366x768.png`, `BOTTOM_BAR_LOW_BIG_1366x768.png`);
  only who fills the slots (`gTeamPanel`) changes. Drawing, the vehicle
  windows, portraits, bars, hands, numbers, scrolling and the low panel work
  with the code they use now.
- **Option O1 (to be decided at implementation):** mark the view on the panel
  without new graphics — a frame around the helicopter's window in the
  selected merc's gold colour (`BIG_TM_SELECTED_COLOR`, as
  `RenderBigTeamSlotBorder()` draws it), while the passengers view is shown.

### 3.2 The passengers in the loaded sector

Only needed if check C1 shows they aren't there:

- While the helicopter's soldier is in tactical (stage 4e, both modes), its
  passengers get what ground vehicle passengers get: a merc slot, `bInSector`,
  their position = the helicopter's (`EVENT_SetSoldierPositionXY` as in
  `AddSoldierToVehicle()`), no place on the map, no movement.
- They are taken out together with the helicopter
  (`TakeHelicopterOffTactical()` / `ResetHelicopterInTactical()`).
- Strategic effects to keep away: they must not count as being "in the
  sector" for battles, retreat, militia, sector inventory or the
  "everyone in sector" checks while the helicopter only hovers — the same
  question the original answers with `SoldierAboardAirborneHeli()`; every use
  of `bInSector` for our team has to be checked (decision B4).

### 3.3 What then works with no extra code

Opening a passenger's inventory (right click on the portrait, stage 4d), the
helicopter's cargo panel, passing items between passengers, to the helicopter
and to mercs within 3 tiles of it (`CanPassViaVehicle()`, P7), Previous / Next
merc through the team panel order (P6).

### 3.4 Getting out of the helicopter in tactical (seat belts)

Added 2026-10-28. Today getting out of the helicopter only works on the map
screen: a new assignment in the team list; the merc then lands at the map's
centre entry point, in the Drassen delivery sector at gridno 10125
(`SetSoldierExitHelicopterInsertionData()`), never in a sector with enemies
(`IsSoldierInHelicopterInHostileSector()`, `AC_IN_HELI_IN_HOSTILE_SECTOR`,
`Assignments.cc`) and not while the helicopter is going down. In tactical the
passengers' single seat belts can't show (no passengers view yet), and stage 4e
hides the "everyone out" belt on the helicopter's window, because the ground
vehicles' `ExitVehicle()` would place a helicopter passenger twice (once by the
helicopter branch of `RemoveSoldierFromVehicle()` → `UpdateMercInSector()`,
once next to the vehicle).

With the passengers view (3.1):

- **Single seat belt** in each passenger's second hand window (the low panel:
  the first's), as for the ground vehicles' passengers: that merc gets out.
- **"Everyone out" seat belt** on the helicopter's window again: everyone
  aboard gets out (as the ground vehicles' one, stage 4d).
- **Its own way out** for the helicopter, built on the map screen's: take him
  out (`RemoveSoldierFromHelicopter()`), put him on a squad, then put him on the
  map once (no `UpdateMercInSector()` through the helicopter branch plus a
  second placement); select him and switch to his squad, as `ExitVehicle()`
  does. When nobody is left aboard the passengers view ends (3.1).

### 3.5 Not in this plan

- Giving the helicopter a squad (variant A).

## 4. Decisions (taken 2026-10-28, all as proposed)

| # | Question | Decision |
|---|---|---|
| B1 | What does the left click on the helicopter's window do with nobody aboard? | only moves the view, as now |
| B2 | Which view shows the helicopter's own window then? | unchanged: the vehicle windows stay first in their fixed order (P1) in every view |
| B3 | Slot numbers / keys 1–9 in the passengers view | the passengers' slots 1, 2, 3… (as a squad) |
| B4 | Passengers "in the sector" while the helicopter only hovers (3.2) | only for the team panel and passing items; excluded from battles, retreat and sector checks as now |
| B5 | In combat | as Q1 of stage 4d: the view can be shown, but no passing with those aboard |
| B6 | Landed at a refuel site (Drassen, Estoni) with passengers aboard | same as hovering: the view works the same way |

Getting out in tactical (3.4), taken 2026-10-28, all as proposed:

| # | Question | Decision |
|---|---|---|
| E1 | Where do they stand? | next to the helicopter: by its picture when hovering (its body), by the map's scenery helicopter at a refuel site (its soldier's gridno) — a free spot found around it |
| E2 | A sector with enemies | blocked, as on the map screen: the belts are inactive, a click says why |
| E3 | Squad when everyone gets out | one common squad (the first one out's), as with the Hummer; one who doesn't fit keeps his own |
| E4 | The helicopter going down | no getting out, as on the map screen |

## 5. Stages

1. **Check C1 and C2** on the PC (section 6) — decides whether 3.2 is needed.
2. **The passengers view** (3.1): enter / leave, slots, selection, the
   vehicle windows, redraws. Test: helicopter hovering with 3 passengers,
   left click on its window, Change Squad back, another vehicle's window.
3. **The passengers in the sector** (3.2), if C1 says so. Test: their
   inventories, passing to the helicopter's cargo and to a merc next to it,
   battle start in the sector, save / load, the helicopter leaving.
4. **Getting out in tactical** (3.4, E1–E4): the single and the "everyone
   out" seat belts, the helicopter's own way out. Test: one passenger out,
   everyone out (one squad), hovering and landed at Drassen, a sector with
   enemies (blocked), the helicopter leaving right after.

Size: stage 2 small–medium; stage 3 medium (the `bInSector` audit); stage 4
small–medium.

## 6. Implementation (from 2026-10-28, on the PC)

Order changed: the passengers in the sector (3.2) first, then the view (3.1),
then getting out (3.4).

- **C1 / C2 answered from the code:** those aboard went into the loaded sector
  only when it was loaded with the helicopter there (`UpdateMercsInSector()`
  → `InternalAddSoldierToSector()`: a merc slot, `bInSector`, no place on the
  map, as the ground vehicles' passengers); nothing updated them when it
  came or left later. The original already keeps them out of sightings
  (`IsSoldierValidForSightings()`).
- **Stage 1 (3.2):** `UpdateHelicopterPassengersInTactical()` (from
  `UpdateHelicopterInTactical()`, every tactical frame): those aboard are in
  the sector exactly while the helicopter is, at its position.
  `SetSoldierExitHelicopterInsertionData()` now only skips a merc already on
  the map (`pLevelNode`), so getting out on the map screen still lands at the
  usual spot. Kept out (B4): the losing end of a battle
  (`CheckForLosingEndOfBattle()`), blasts and gas
  (`DamageSoldierFromBlast()`, `DishOutGasDamage()`), the warp to another
  level (`DoneFadeOutWarpCallback()`).
- **Stage 2 (3.1, O1):** `Interface_Panels.cc` — a left click on the
  helicopter's window with someone aboard in the sector fills the slots with
  them (`ShowHelicopterPassengersView()`), the current squad unchanged; the
  helicopter's window gets the gold frame meanwhile. `SetCurrentSquad()` (then
  always refilling) and `RebuildCurrentSquad()` end it; every tactical frame
  `UpdateHelicopterPassengersView()` goes back to the squad when the
  helicopter left or is empty, and refills the slots when someone got in or
  out. A click on a ground vehicle's window leaves it, also for the current
  squad's vehicle.
- **Going to tactical (added 2026-10-28):** with nobody of ours on the ground,
  the map screen's Tactical button only worked where the helicopter had landed
  (Drassen, Estoni): `CanGoToTacticalInSector()` skips those aboard it in the
  air. Now those aboard a helicopter hovering over the sector count too, if
  the sector has no enemies (`IsHelicopterHoveringOverSafeSector()`) — a
  battle with nobody on the ground would be lost at once (B4).
- **Entering with nobody on the ground (added 2026-10-28):** as with a ground
  vehicle's squad, the first tactical frame of a new world shows the
  helicopter's passengers view at once (its window framed, those aboard in
  the slots, the screen on it) when nobody of ours is on the ground
  (`ShowVehicleOnSectorEntry()`; 1366x768). With several vehicles, all
  everyone in them, the first with someone aboard in the windows' order is
  shown: the helicopter, the Hummer, the ice cream truck, the others (a
  ground vehicle: its squad).

## 7. Checks on the PC before implementing

- **C1:** with the helicopter hovering over the loaded sector and mercs
  aboard, do those mercs have `bInSector` set, a merc slot, a position?
  (debugger or a temporary log in `UpdateHelicopterInTactical()`.)
- **C2:** the same with the helicopter landed at Drassen (B13).
