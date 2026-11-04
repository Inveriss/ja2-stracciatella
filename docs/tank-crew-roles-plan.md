# Player's tank: a crew of four with fixed roles — plan

Status: **stages 1-3 done and verified in the game, stages 4-5 not implemented; all decisions
taken by the user (2026-11-04)**
(written on Remote Control, branch `Tanks`). Changes
decisions G1 and F6 (the gunner) of `docs/tank-cannon-firing-plan.md` and
`docs/tank-firing-while-moving-plan.md`.

## 1. Goal (user's requirements)

The tank is run by a crew, as closely to the real thing as the game allows.

1. At most 4 people aboard.
2. Four roles in a fixed order: **Commander, Driver, Gunner, Loader**. The first four merc
   slots of the tank's squad in the Bottom Bar panels always belong to them, in this order.
3. The role's name stands in the slot's first hand window. An item in that hand is not
   shown there; the merc's inventory panel shows it as usual.
4. Nobody fires a personal weapon from inside the tank (as today).
5. In tactical, those getting in take the next role in the fixed order. On the strategic
   screen they get in by the current rules.
6. Moving needs at least Commander and Driver, in tactical and on the strategic map.
7. The cannon and the machine gun need all four. With fewer, a question mark replaces the
   firing cursor. Turn-based and real time alike.

The earlier requirement that those behind move up when someone gets out is removed from the
plan (user, 2026-11-04). R8 says where a newcomer sits instead.

## 2. What exists today

Checked in the code; nothing here has been tried in the game.

| What | Where | Today | Needed |
|---|---|---|---|
| Seats | `vehicles.json`, `seats` | 6 | 4 |
| Seat order | `pPassengers[]`, `AddSoldierToVehicle` | Behind the last one aboard; when the last seat is taken, everyone moves up to close the gaps | The first empty seat, nobody moves (R8) |
| Getting out | `RemoveSoldierFromVehicle` | The seat stays empty | No change |
| Driver flag | `SOLDIER_DRIVER` | The first aboard; passes to the next seat when he leaves | The Driver's seat only |
| Team panel | `AddPlayerToInterfaceTeamSlot` | Takes the **first free slot**, and skips the empty seats | Slot number = seat number |
| First hand | `RenderSoldierTeamInv` | Steering wheel for the driver, the item for a passenger | The role's name |
| Moving, tactical | `CanVehicleDriveInTactical` | Anyone able aboard | Commander and Driver |
| Moving, strategic | `CanCharacterMoveInStrategic` | Not empty (`ME_VEHICLE_EMPTY`) | Commander and Driver |
| Sleeping | `SoldierMustDriveVehicle` | The only one able to drive may not sleep | R3 |
| Who aims | `GetTankGunner` | Best marksman aboard | The Gunner's seat |
| Firing | `GetActionModeCursor`, `HandleItem` | Refuses with no gunner, by a message | Question mark with fewer than four |

The question mark exists: `INVALIDCURS` gives `INVALID_ACTION_UICURSOR`, drawn from
`cursors/que.sti` (the graphic was checked). The tank itself takes no slot in `gTeamPanel`,
so slots 0-3 are free for the crew; the 1366x768 panel shows its portrait in a window of
its own, before the slots.

## 3. Decisions

All taken by the user, 2026-11-04.

| # | Question | Decision |
|---|---|---|
| R1 | Does an unconscious, sleeping or collapsed merc fill his role? | **No** |
| R2 | An escorted character (EPC) or the robot aboard? | **Fills no role.** May get in and holds a seat, so with one aboard there is no full crew |
| R3 | Sleeping | **A merc must be active to fill a role; sleep excludes him, in any of the four roles.** See section 4 for a tank on the road |
| R4 | Roles on empty slots | **All four names always show**, even with only a Commander aboard. They show while the tank's squad is the current one; another squad's panel is as usual |
| R5 | Choosing who does what | **By boarding order, then by the arrows** of `docs/team-panel-merc-order-plan.md`, the next change after this plan (section 6) |
| R6 | Do the roles use a merc's skills? | **Not in this plan.** Skill traits (Heavy Weapons, Night Ops and the like) by role are another stage of the project. The Gunner's numbers stay as they are today, read from the Gunner's seat |
| R7 | The right click on the first hand window of a crew member | **Off.** The inventory panel still swaps his hands |
| R8 | Where does a newcomer sit after someone has left? | **In the first empty role, and nobody ever changes seats.** B leaves, N becomes Driver. A small tank branch in `AddSoldierToVehicle`, in place of today's rule, which seats him behind the last one aboard and closes the gaps when the last seat is taken |

## 4. Design

**Role by seat.** `pPassengers[0..3]` are the roles, so nothing new goes in the save:

```cpp
enum TankRole { TANK_COMMANDER, TANK_DRIVER, TANK_GUNNER, TANK_LOADER, NUM_TANK_ROLES };
SOLDIERTYPE* GetTankCrewMember(VEHICLETYPE const&, TankRole); // who sits there, or NULL
bool IsTankRoleManned(VEHICLETYPE const&, TankRole);          // and can do it (R1-R3)
bool TankCanMove(VEHICLETYPE const&);                         // Commander and Driver
bool TankCanFire(VEHICLETYPE const&);                         // all four
```

`IsTankRoleManned` asks for: `bLife >= OKLIFE`, awake, not collapsed, `bBreathMax` above
`BREATHMAX_ABSOLUTE_MINIMUM`, not an EPC, not the robot. The Gunner also needs a
marksmanship above 0, as `GetTankGunner` does today.

**The driver's flag follows the seat.** `SOLDIER_DRIVER` for seat 1, `SOLDIER_PASSENGER`
for the others, set by one function (`UpdateTankCrewFlags`) after each change. It replaces
the hand-over in `RemoveSoldierFromVehicle` for the tank. As in the Hummer, clicking the
Driver's portrait selects the tank, and the others select themselves. With the Driver's seat
empty nobody has the flag; the tank's own portrait still selects it, and `TalkerFor` already
falls back on anyone aboard.

**Panel: slot number = seat number.** A gap stays a gap, and shows its role's name (R4).
`CheckForAndAddMercToTeamPanel` goes through `CFOR_EACH_PASSENGER`, which skips the empty
seats, and then takes the first free slot: both would move a merc to the wrong role. The tank
writes `gTeamPanel[seat]` directly instead. On the 1366x768 panel, empty slots must keep
drawing their window, and `BigTeamPanelMaxFirstSlot` must count four slots.

**Sleep on the road (R3).** Asleep means out of the role, in all four. A merc asleep in a
standing tank is allowed; the tank then can't move or fire. Two existing rules stay, through
`SoldierMustDriveVehicle`, which becomes true for the Commander's and Driver's seats on a
tank:

- While the tank has a route, these two are refused sleep, with the existing message: if they
  could sleep the tank would stop between sectors. An exhausted one stops it at the next
  sector (`PlayerSoldierTooTiredToTravel`).
- Giving a route wakes these two if they can be woken
  (`WakeUpAnySleepingSelectedMercsOnFootOrDriving`), as for a driver today. If not, the route
  is refused.

Gunner and Loader may sleep on the road; the tank then can't fire on arrival, unless the
battle wakes them (`WakeUpAllMercsInSectorUnderAttack`, through the pre-battle screen).

## 5. Stages

### Stage 1: seats and order

- `vehicles.json`: the tank's `seats` 4.
- `Vehicles.h/.cc`: the enum and functions above; `UpdateTankCrewFlags`.
- `AddSoldierToVehicle`: the first empty seat (R8); flags through `UpdateTankCrewFlags`.
- `RemoveSoldierFromVehicle`: flags through `UpdateTankCrewFlags`. Reselect the tank if the
  merc selected becomes the Driver (the existing case).
- `Interface_Panels.cc`: slot number = seat number for the tank's squad.

Check: 4 get in, a fifth is refused, in tactical and on the map; someone from each seat gets
out and the others stay in their slots; a newcomer takes that role.

### Stage 2: roles on the panels

- New strings `pTankCrewStrings` (4): COMMANDER, DRIVER, GUNNER, LOADER (capitals, user); `Localization.h`
  and all translations, in English as the rest of these arrays.
- `RenderSoldierTeamInv`: for a tank crew member the role, centred in the first hand window,
  in place of the wheel or the item. Render with `tools/interface_preview` first.
- Empty slots: the role's name too (R4).
- The first hand's help text: the role; the right click off (R7).

Check: on the big and the low team panel, and in other resolutions with a smaller slot; an
item in the merc's hand doesn't show there and does in his inventory.

### Stage 3: moving

- `CanVehicleDriveInTactical`: `TankCanMove` for the tank.
- `HandleMoveModeInteractiveClick`: a new message, as a car out of fuel has: "%s needs an
  active commander and driver." (`pVehiclePassengerStrings[5]`).
- `CanCharacterMoveInStrategic`: for the tank, `ME_CUSTOM` with that message, counting a merc
  who can be woken (section 4).
- `RemoveSoldierFromVehicle`: without a Commander or a Driver, a planned route is cancelled,
  as today only an empty vehicle's is. Getting out between sectors is already refused.
- `SoldierMustDriveVehicle`: the two seats on a tank (section 4).

Check: tactical in turns and real time, the map, with one and two aboard; a route planned,
and the Driver gets out; Commander asleep when the route is given; an EPC in the Driver's seat.

### Stage 4: firing

- `GetActionModeCursor`: `INVALIDCURS`, the question mark, for a tank without
  `TankCanFire`. This also covers the machine gun and `HandleUIReloading`.
- `GetTankGunner`: the Gunner's seat, not the best marksman (G1, F6), if
  `IsTankRoleManned`, else `NULL`. The guard for a shot credited after the wreck's vehicle
  is gone stays. Note both changes in the older plans.
- `HandleItem`: keep a refusal as the safety net; the cursor no longer lets a click through,
  so "nobody aboard able to aim" should now never show.

Check: 1, 2, 3 and 4 aboard with both weapons, in turns and real time; a member knocked out
or put to sleep turns the cursor back to the question mark; experience and kills to the
third seat's merc, whatever the others' skill.

### Stage 5: the whole in play

Everything together: a full battle; the crew changing in combat with its AP costs (D15);
the tank destroyed (`KillAllInVehicle`, gaps while it empties); getting out while it moves
(D13 forbids it); Hummer and truck unchanged, including their seat rules; save and load
with a crew of each size; a new game, because a save may hold more than four. Then balance.

## 6. Next change

**Choosing a role** (R5): `docs/team-panel-merc-order-plan.md`. Two arrows on the team panel
move the selected merc to the next or previous slot, so to the next or previous role.
Role by seat suits it: a move changes the seat, then `UpdateTankCrewFlags` and the panel. It
starts after stages 1 and 2 of this plan.

## 7. Later, not in this plan

- Firing or moving with a crew one short at a penalty, instead of not at all.
- Skill traits by role (R6): another stage of the project, with its own plan.

## 8. Risks

| Risk | Note |
|---|---|
| Code assuming the first aboard drives | With the Driver's seat empty nobody has `SOLDIER_DRIVER`. The uses were listed by search: panels, `TalkerFor`, `GetUIApsToDisplay`. Repeat the search when implementing |
| Other vehicles change | Each new rule goes through the tank's type |
| A crew stuck by someone unconscious | R1, R2: he has to be taken out. To judge in the game whether this is too harsh |
| The wrong merc as Gunner | R5: one poor shot in the third seat makes the cannon weak, until the arrows come |
| The team panel's order | Written directly into `gTeamPanel`; must agree with `RebuildCurrentSquad` and the keys 1-9 |
| A squad of four slots on a smaller panel | Other resolutions must still show four; stage 2 |
| Text width | "Commander" must fit the small windows of other resolutions; stage 2 |
