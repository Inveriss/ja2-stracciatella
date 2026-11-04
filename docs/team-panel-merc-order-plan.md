# Team panel: moving a merc between slots with two arrows — plan

Status: **plan, not implemented, all decisions taken by the user (2026-11-04)**
(written on Remote Control). Work goes on the branch **`Tanks`**, by
the user's decision. Above all, this is **how a tank's crew chooses its roles**: it is the
"next change" of `docs/tank-crew-roles-plan.md`. A merc in the wrong role, Loader instead of
Gunner, is put right with one click.

Implement after stages 1 and 2 of the crew plan, which make seat the role and show the roles
on the panel.

## 1. Goal (user's requirements)

1. Two more arrows on the Bottom Bar panel, left and right, with the same graphics as
   Next / Previous Merc. A placeholder position for now; the final one is found later.
2. Left arrow: the merc swaps places with the one in the slot before him (3rd goes to 2nd,
   2nd to 3rd).
3. Right arrow: the merc swaps places with the one in the slot after him (6th goes to 7th,
   7th to 6th).
4. The arrows loop. The last merc reaches the first place with the right arrow, and the first
   the last place with the left arrow; all the others shift by one.
5. The order chosen is remembered by the game for good.
6. It is reset when the merc changes squad.
7. It works in vehicles too: the helicopter, and above all the tank.

## 2. What exists today

Checked in the code; nothing here has been tried in the game.

| What | Where | Today | Effect on this change |
|---|---|---|---|
| Squad order | `Squad[n]`, a list of members | The order they joined; gaps squeezed by `RebuildSquad` | The order to change, on foot |
| Vehicle order | `pPassengers[]`, seats | The panel shows them, skipping empty seats | The order to change, aboard |
| Saving | `SaveSquadInfoToSavedGameFile`; `pPassengers` in `LoadSaveVehicleType.cc` | Both written by position | **Requirement 5 needs no new save data**, no new format |
| Changing squad | `RemoveCharacterFromSquads`, `AddCharacterToSquad` | Out of the list, then the first free place of the new one | **Requirement 6 needs no new code** |
| Panel slots | `gTeamPanel[i].merc` | Filled by `AddPlayerToInterfaceTeamSlot`, first free slot | The slot follows the list only by chance |
| Filled from the squad | `SetCurrentSquad`, `RebuildCurrentSquad` | In `Squad` order | Good |
| Filled from elsewhere | `UpdateTeamPanelAssignments` (`FOR_EACH_IN_TEAM`: a contract ending, a death, profile changes), `AddSoldierToSector` | **In team order**, or in order of arrival | Would undo the player's order: must be fixed |
| A gap on the panel | `RemovePlayerFromTeamSlot` | The slot stays empty, while `Squad` squeezes | A normal slot now (P3) |
| Dead mercs | `sDeadMercs` | Out of `Squad`, but keep a panel slot until it is rebuilt | A normal slot now (P3) |
| What follows the slots | Slot numbers, `HandleSelectMercSlot`, Space, the big panel's Next / Previous | All read `gTeamPanel` | Follow the new order with no change |
| Scrolling | `giBigTeamPanelFirstSlot`, squads of up to 100 in 7 windows | Scrolls only when the **selection** changes | A move keeps the selection, so it must ask for this itself |
| Next / Previous buttons | `CreateTEAMPanelButtons`, `giBigTeamPanelMercButtons` | `inventory_buttons.sti` frames 20-23, 1366x768 only | The pattern to copy |

**The laptop does the same.** `MoveBookmarkUp` (`Laptop.cc`), the arrow beside the WWW
bookmarks: a swap with the neighbour, and from the first place a rotation of the whole list.
Requirements 2 to 4 are the same logic, in both directions.

**Three facts about selection in a vehicle** (`IsSelectablePassenger`, `IsShownSelected`,
`MercFacePanelCallbackPrimary`):

- A passenger in a Hummer or a tank is selected himself.
- **The driver can't be selected.** A click on him selects the vehicle, and his slot then
  shows as selected.
- **In the helicopter nobody aboard can be selected.** A click selects the helicopter: P7.

Button grid of the 1366x768 panel: `BIG_TM_BUTTON_X/Y`. Row 0 holds End Turn, Map, Squad; row
1 Previous, Next, the low panel toggle. By the code, **row 2, columns 0 and 1 are free** on
both forms, down to the clock at y 163 (full) and y 113 (low): y 73 and y 69. That is the
placeholder. Whether it looks right on the panel's picture can only be seen in the game.

## 3. Decisions

All taken by the user, 2026-11-04.

| # | Question | Decision |
|---|---|---|
| P1 | Which merc do the arrows move? | **Only the one selected now.** He stays selected. With a vehicle selected, that is its driver, whose slot shows as selected |
| P2 | The loop: what happens to the others? | **All the others shift by one**, as in the laptop's bookmarks. Between neighbours it stays a swap |
| P3 | A dead merc's slot, an empty slot | **A normal slot**: the merc swaps with it |
| P4 | Vehicles | **Yes**: the vehicles and the helicopter, above all the tank |
| P5 | Which panels? | **Only the Bottom Bar**: the 1366x768 team panel, full and low. Not the map screen, and not the old panels. Not the SM panel either, which shows no slots |
| P6 | When is the merc's place lost? | **When he changes squad.** Nothing extra to write. Boarding or leaving a vehicle is a change of squad in the game: a vehicle has a squad of its own |
| P7 | Who moves in the helicopter, where nobody can be selected? | **A click on a passenger's portrait marks him** with the selected border, for the arrows only. The helicopter stays the selected soldier, so no order can go to someone in the air |
| P8 | Hummer and other cars: does the driver change when slots change? | **Yes: the first merc aboard drives**, as the tank has a fixed seat. It is the one way to choose the driver, and it matters, as the driver can't shoot from inside. The helicopter isn't affected, as Skyrider flies it |

Also, by default and with no decision needed:

- No AP cost, in combat and out, while the panel's buttons are enabled.
- Refused while the vehicle drives or anyone shoots (as `SwapTankWeapons`), since a shot
  already fired credits its gunner.

## 4. Design

**The range.** The slots an arrow works on:

- The tank: always its 4 roles, even if empty.
- Everyone else: from the first slot to the last one occupied, dead mercs included.

**The move**, on the panel's slots, in a function of its own:

```cpp
// Interface_Panels.h: moves him among the slots, then writes the order back
bool MoveMercOnTeamPanel(SOLDIERTYPE const&, bool right);
```

1. Find his slot `i` and the range `n`. With a range under 2, or the refusals above: false.
2. Between neighbours, swap slots `i` and `i ± 1`. At an end, `std::rotate` the range.
3. Write the order back, then mark the panel dirty and scroll the merc into view.

**Writing back:**

- **On foot:** `Squad[]` gets the living members in the panel's order. The dead aren't in it,
  and a gap doesn't exist there.
- **In a vehicle:** `pPassengers[slot]` = the slot's merc: **a slot number is a seat number,
  in every vehicle**, as the crew plan already has for the tank. Then the flags: the tank
  through `UpdateTankCrewFlags`, the cars by P8.
- **Selection:** if the selected merc became the driver, the vehicle gets selected, as in
  `RemoveSoldierFromVehicle`; if the merc shown as selected stopped being the driver, he gets
  selected himself. So the same merc can be walked along with repeated clicks.

**What lasts.** The order of the living on foot and every seat aboard, gaps included, are
saved. The place of a dead merc or of a gap on foot is not: it lasts until the panel is
rebuilt, by a sector change or a load, when the living close up and the dead go last, as
today.

**The panel follows the lists.** `SyncTeamPanelOrderWithSquad()`, called at the end of
`CheckForAndAddMercToTeamPanel`, which every filling path goes through:

- On foot: the slots held by the squad's members, rewritten in `Squad` order. Dead mercs and
  gaps stay put. Only the `merc` pointers move: not `RebuildCurrentSquad`, which also picks a
  squad, sets `fUICloseMerc` on everyone and turns all faces off.
- In a vehicle: `gTeamPanel[seat]` directly. The helicopter's passengers' view likewise,
  skipping those not in the sector.

## 5. Stages

### Stage 1: the order

- `Interface_Panels.h/.cc`: `MoveMercOnTeamPanel`, `SyncTeamPanelOrderWithSquad`; slot = seat
  for all vehicles; the scroll made callable (the check on `last_selected` in the function
  around line 4490).
- `Squads.h/.cc`: writing the order into `Squad[]`.
- Faces: `RemovePlayerFromInterfaceTeamSlot` calls `SetAutoFaceInActive`, and an active face
  holds its position. Do the same for those moved, if a talking face stays in the old slot.

Check: nothing changes for a player who never uses the arrows, apart from the panel always
following the squad's order, and a vehicle's empty seats showing as gaps.

### Stage 2: the arrows

- `Interface_Panels.cc`: `giBigTeamPanelOrderButtons[2]`, in row 2 (section 2), images from
  `inventory_buttons.sti` frames 20-23 by `UseLoadedButtonImage`. The position in one place,
  so the final one is a small change.
- The same five places as `giBigTeamPanelMercButtons`: created, removed, hidden on the watched
  battle's bar, disabled and enabled, marked dirty.
- Disabled while nothing can move: nobody selected as in P1, a range under 2, the tank with
  nobody in the Driver's seat while the tank itself is selected.
- New strings for the help text, "Move merc left" and "Move merc right"; all translations, in
  English as is usual here.

Check: requirements 2 to 4 on both forms of the panel; the selection stays; the slot number
stays with the slot; a click on an arrow doesn't reach the map beneath.

### Stage 3: vehicles

- The driver's flag by P8; reselecting (section 4).
- The helicopter by P7.
- The tank: its range of 4; `UpdateTankCrewFlags`; a role name follows the slot, not the merc.

Check, the tank above all: Loader to Gunner with one click, then firing with him; a merc into
an empty role; Commander left to Loader and Loader right to Commander, the others shifting;
the Driver moved from the tank selected, then clicked further; `TankCanMove` and `TankCanFire`
turning at once, with the question mark; experience to the new Gunner.

### Stage 4: the whole in play

- Save, load, and the order is there, gaps in vehicles too. Leave the sector and return.
  Change the current squad and back.
- A merc changes squad and comes back: he is last, the others keep their order. A squad
  boards a Hummer and gets out. A merc's contract ends; a merc dies, in combat and out; a merc
  arrives by helicopter.
- Moving with a dead merc beside him, and with a gap, on foot and aboard.
- A squad bigger than 7: wrap from the last slot to the first, and the panel scrolls.
- In combat: on our turn, and disabled in the enemy's. Talking faces during a move.
- Refused while the vehicle drives or a shot is in the air.

## 6. Risks

| Risk | Note |
|---|---|
| A fill path missed | All of them end in `CheckForAndAddMercToTeamPanel` |
| `Squad[n][0]` has meaning | The default selected merc (`SetCurrentSquad`), and read for a location in `SaveLoadGame.cc` and `Assignments.cc`. Any member fits: same sector and squad |
| Members not on the panel | One not in the sector yet (arrival by helicopter) keeps his place in the list |
| Foot mercs in a vehicle's squad | Not expected, but `AddCharacterToSquad` wasn't traced for it. Check in stage 1; if it can happen, the arrows stay off for that squad |
| A car's `AddSoldierToVehicle` | Its rule closes the gaps when the last seat is taken: the slots follow, as slot = seat |
| Marked merc in the helicopter (P7) | A new small state, not saved: to be cleared when the view ends |
| Old panels | Only the sync runs there, and slot = seat: to be seen in another resolution |
| The map screen | Its character list has its own sort: not touched |
| Too easy in combat | A free change of Gunner at any moment on our turn. Experience and kills go to whoever fires. An AP cost could come later |
