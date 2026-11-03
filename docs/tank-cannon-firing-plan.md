# Player's tank: firing the cannon — plan

Status: **plan, not implemented, all decisions taken** (written 2026-11-03 on Remote
Control, branch `Tanks`). Scope: profile 232 (`PROF_TANK`) fires its cannon while
standing, in turn-based combat and in real time.

Related: `docs/tank-rotating-hull-plan.md` (D7 sends firing here),
`docs/tank-firing-while-moving-plan.md` (firing on the move builds on this plan;
its decisions F6 and F7 apply here too).

## 1. What exists today

Checked in the code; nothing here has been tried in the game.

### 1.1 What already works, as enemy tanks use it

- **The firing pipeline.** `HandleItem` (`Handle_Items.cc`) → `EVENT_FireSoldierWeapon`
  → turn to the target (`TurnSoldier` allows tanks; `fTurningToShoot` also accepts
  them without a ready animation) → `SelectFireAnimation()` gives `TANK_SHOOT` for a
  cannon → `FireWeapon()` → `UseGun()`.
- **The shell.** `TANK_CANNON` flies as a bullet (`BULLET_FLAG_TANK_CANNON`) and
  explodes on impact as `TANK_SHELL` (`Weapons.cc`, blast 60 damage, radius 6).
- **Sound and art.** `tank_cannon.wav`, `TNK2_SHT.STI`.
- **Animations for our body.** `gubAnimSurfaceIndex[TANK_NE][TANK_SHOOT]` is set. The
  current graphics fit the current driving: the turret turns and the hull stays. No
  new art is needed for this plan; see section 4.
- **Combat.** A tank on our team already has its 25 AP and its turn.

### 1.2 What stops the player's tank

| What | Where | Effect |
|---|---|---|
| No cannon | `EquipTank` only runs for generated enemies; the tank comes from a profile placement | `HANDPOS` is empty |
| No target cursor | `GetActionModeCursor` (`UI_Cursors.cc`) returns `INVALIDCURS` for every vehicle | The selected tank can't aim |
| Clicks mean driving | `Handle_UI.cc`, `Turn_Based_Input.cc` take vehicles out of the attack paths in several places | A click on an enemy never reaches `HandleItem` |
| Marksmanship 0 | `CalcChanceToHitGun` returns `MINCHANCETOHIT` at once for `bMarksmanship == 0`; the tank's profile has 0 | Always the minimum chance |
| Ammunition | `EnoughAmmo` (`Points.cc`) wants a bomb attached to the cannon; `DeductAmmo` does nothing for a cannon (the enemy has endless shells) | No shell can be fired, and none would be used up |
| No shells to get | `TANK_SHELL` is `bNotBuyable` and `bNotEditor` | The player can't obtain any |
| Neutral | Our tank is `bNeutral` (since `4e15ce057`, as the Hummer); `OppList.cc` skips neutrals (`CONSIDERED_NEUTRAL`) in sight and noise | A firing tank could be ignored by enemies: to check, G5 |

### 1.3 The AP cost by the existing formula

`BaseAPsToShootOrStab` with the tank's 25 AP and the cannon's `ubShotsPer4Turns` 8:

| Aim skill | AP per shot |
|---|---|
| 0 | 25 |
| 80 | 14 |
| 100 | 13 |

Aiming and turning come on top. With a gunner of skill 80 that leaves about 11 AP for
driving, so one shot a turn and a short move. That reads as right for a tank.

## 2. Decisions

G1, G2 follow F6, F7 of the firing-while-moving plan. Details may be refined after
the full implementation (user, 2026-11-03).

| # | Question | Decision | State |
|---|---|---|---|
| G1 | Who aims? | The best marksman aboard (`F6`): his marksmanship, dexterity, experience; experience gain goes to him. Nobody able aboard: no shot | decided (F6) |
| G2 | Ammunition | `TANK_SHELL` from the tank's cargo slots (`F7`), one for each shot, no loading step. Out of shells: refusal with the driver's out of ammo quote | decided (F7) |
| G3 | The cannon | Created in `HANDPOS` when the tank is created for a non-enemy team, `OBJECT_UNDROPPABLE`. The slot stays hidden on both panels, no repairs, no wear, no jams; a damaged tank fires as usual | **decided** (user, 2026-11-03) |
| G4 | Who pays AP? | The tank's own 25 AP, as for driving, with the existing formula and the gunner's aim skill | **decided** (user, 2026-11-03) |
| G5 | Neutrality | With anyone aboard the tank is not neutral (`SetSoldierNonNeutral`); empty it is neutral again. Stage 1 confirms that enemies really ignore a neutral tank | **decided** (user, 2026-11-03) |
| G6 | Where do shells come from? | For now only the test switch (below). Whether Bobby Ray's, a dealer or finds sell them is a later campaign decision. The tank itself has no campaign source yet either | **decided** (user, 2026-11-03) |
| G7 | Passengers | `CanShootFromVehicle` stays closed: nobody shoots small arms from inside a tank. They are selected and crouched as in the Hummer (`IsSelectablePassenger`) | **decided** (user, 2026-11-03) |
| G8 | While driving | Refused in turn-based and in real time here (`IsVehicleMoving`). Firing on the move is its own plan | **decided** (user, 2026-11-03) |
| G9 | Close range | Keep the original penalties (`Weapons.cc`, under `MIN_TANK_RANGE` and the max visible distance), so a very close target is close to 0%. Tune in the game | **decided** (user, 2026-11-03) |
| G10 | Real time | The usual rule: the first noticed shot starts the turns. The exception (F1) belongs to shots on the move only | **decided** (user, 2026-11-03) |
| G11 | Machine gun | Not in this plan | **decided** (user, 2026-11-03) |

Test switch: while `TANK_IN_A9_FOR_TESTING` is on, the tank starts with a stack of
shells in a cargo slot.

## 3. Stages

### Stage 1: checks

The code reading is done (2026-11-03). One game test is still open.

**1. Enemies and a tank with a crew (G5).**

- A neutral vehicle is not a target: `CalcBestShot` and its neighbours in `Attacks.cc`
  skip it through `CONSIDERED_NEUTRAL`, and no opponent is counted in `ManSeesMan`. So the
  driving analysis, 2d, is wrong that the AI "doesn't leave vehicles out".
- The crew is a target: `IsSoldierValidForSightings` only leaves out the helicopter's
  crew, those aboard stand on the vehicle's tile, and they are not neutral. By the code,
  enemies see the crew and shoot at that tile. The shot should then hit the vehicle, as
  the crew has no place on the map. **Not proven by the code: to test in the game.**
- `AnEnemySeesAMerc` has no neutral check and counts vehicles on our team, so an empty
  tank that was boarded once can also start the turns when seen.
- The neutral flag matters for the cannon itself: `HearNoise` handles interrupts only
  between mutual hostiles (`OppList.cc`), so a neutral tank firing gives no such
  interrupts. G5 stays.
- For G5: the flip needs `RecalculateOppCntsDueToNoLongerNeutral` and
  `RecalculateOppCntsDueToBecomingNeutral`, not only the setters. A tank's flag survives
  a load (`TacticalCreateSoldierFromExisting` forces neutral only on the Hummer, the
  truck and the helicopter).

Game test, no code needed: drive a crewed tank up to enemies. Do they shoot it, does it
lose health, does the crew stay unhurt, do the turns start?

**2. `SOLDIER_PC`: yes.** Boarding goes through `ChangeSoldierTeam` →
`TacticalCreateSoldier`, which sets the flag for `OUR_TEAM`, and nothing removes it. So
`InternalReduceAttackBusyCount` takes `UnSetUIBusy`, which only acts for the selected
soldier, and that is the tank. There is no difficulty modifier on the chance to hit
either (`CalcChanceToHitGun`). `ChangeSoldierTeam` also copies the inventory, as stage 2
expects. An attack that starts while another soldier is selected is the case to try in
stage 5.

**3. The cursor: `TARGETCURS`.** The cannon's model passes it in its constructor
(`WeaponModels.cc`), so `weapons.json` needs no entry. Only the vehicle exit in
`GetActionModeCursor` stands in the way.

**4. The turning AP.** A tank stands in a non ready animation, so
`GetAPChargeForShootOrStabWRTGunRaises` charges the raise and `MinAPsToShootOrStab`
drops the turn. The cannon's `ubReadyTime` is 0, so the shown cost has no turn or raise
in it. The real turn may still take `AP_LOOK_STANDING` (1 AP) at its start
(`Soldier_Control.cc`), by the animation's flags. To compare in stage 5: the AP shown
against the AP taken, with a turn.

**Also found: no jam work is needed.** `CheckForGunJam` already leaves out
`EXPLOSIVE_GUN`, which includes the cannon. Stage 2 only has to stop the wear.

### Stage 2: the cannon and the shells

- `Soldier_Create.cc`: the cannon (G3), at the tank case that already sets `bNeutral`.
  `ReloadSoldier` copies the inventory when the team changes, so it survives boarding.
- `Vehicles.cc`: `IsPlayerTank()`, and `GetTankGunner()` returning the best marksman
  aboard or nothing.
- `Points.cc`, `EnoughAmmo`: for the player's tank a shell in the cargo
  (`FindLaunchable`), not an attachment. `DeductAmmo`: takes one shell from that stack
  and refreshes the panel; enemy behaviour stays.
- `Weapons.cc`: no wear for this cannon (`uiDepreciateTest`); jams already skip it.

Implemented and game-verified (2026-11-03), with these differences and
additions:
- Shells are found with `FindObj`, not `FindLaunchable`: nothing but the cargo can hold
  them in a player's tank.
- `UseGun` no longer queues the out of ammo quote after each cannon shot, because
  `ubGunShotsLeft` is always 0 on a cannon.
- `Rotting_Corpses.cc`: the cannon doesn't drop with the wreck. `OUR_TEAM` soldiers drop
  undroppable items too, so the flag alone isn't enough.
- The cannon is hidden where the panels show a hand: the team panel with its help text,
  and the map's equipment row. The hand swap (click, key) is off for the tank.
- G5 has no stage of its own yet: it goes in with the aiming, stage 4.

### Stage 3: the gunner's numbers

- `CalcChanceToHitGun`: the early return for marksmanship 0 and the marksmanship read
  look at the gunner. The robot's controller next to it is the existing model. Dexterity
  and experience reads further down use the same gunner; go through all of them.
- `CalcAimSkill` (`Points.cc`): the gunner's skill, for the AP.
- `UseGun`: experience (`StatChange` ignores vehicles) and kill credit to the gunner.
- Quotes through `TalkerFor` (the driver speaks for the vehicle).

Implemented and game-verified with stage 4 (2026-11-03).
`ShooterFor()` gives the credit for a shot. `CalcChanceToHitGun` reads the gunner's skills
and condition, also wounds and breath, so a damaged tank fires as usual (G3). Also
credited to the gunner: shot statistics (`LOS.cc`), kills and assists (`Soldier_Ani.cc`),
morale and experience of a kill (`Overhead.cc`), the kill quote and laugh.

### Stage 4: aiming

- `GetActionModeCursor`: the player's tank with a gunner gets its target cursor, with
  the chance to hit and AP shown as for a merc.
- `Handle_UI.cc`: the vehicle exits on the way to action mode. Move and action mode must
  swap as they do for a merc: a click on open ground drives, the target cursor fires.
  Getting into the tank must stay impossible for its own crew, and other vehicles change
  nothing. `IsDrivingVehicleBody` and `CanVehicleDriveInTactical` stay separate from it.
- `Turn_Based_Input.cc`: the same for the clicks, including the checks that exclude
  vehicle targets, which remain about targets, not shooters.
- Refusal messages (no gunner, driving, no shells): new strings in all translations.

Implemented and game-verified (2026-11-03). The mode swap already works by right
click, so only the vehicle exit in `GetActionModeCursor` had to go. The cursor shows for
every player's tank, also with no gunner: `HandleItem` refuses with a message, which a
cursor without a click could not give. Out of shells: the driver's quote, no new string.
G5 is in `AddSoldierToVehicle` and `RemoveSoldierFromVehicle`.

### Stage 5: the shot in play

Fire through `HandleItem` as for any gun. Test: all directions; turn, AP, aiming clicks;
hit, miss, wall, roof; a shot at a neutral or friend; interrupted by an enemy; tank
destroyed with the shell in flight; out of shells; the shell's blast near the tank, with
the crew safe by D9 of the driving plan; save and load; a new game.

### Stage 6: tuning

Hit chance by range (G9), AP (G4), blast and enemy reaction, in real battles.

## 4. Art and the other plans

No new art is needed: `TNK2_SHT.STI` works with today's hull. When the rotating hull
arrives (`docs/tank-rotating-hull-plan.md`, stage 3) the 64 shot frames must be redrawn
in the same format, because they show the old hull position. Its `TANK_PLAYER` body also
needs `gubAnimSurfaceIndex[TANK_PLAYER][TANK_SHOOT]` and `TANK_BURST`.

This plan does not depend on that one, in either order. If the hull comes first,
turning to a target needs room for the footprint: a tank without room must refuse the
shot with a message, not hang. Firing on the move depends on this plan's stages 2 and 3.

## 5. Risks

| Risk | Note |
|---|---|
| Enemy behaviour changes by accident | `TANK()` covers enemy tanks too: each change goes through `IsPlayerTank()` or the team |
| Interface lock | Attack counts and `SetUIBusy` for a vehicle shooter have not been used before on our side; stage 1 and stage 5 |
| A tank that shoots but is never shot at | G5 |
| Too strong | Blast 60 in radius 6 on 25 AP, values set for the AI; shells are the brake (G6) |
| Volatile cargo | Shells are damageable (`volatility` 2): destroyed tanks may blast harder, which fits, but test it (`CheckEquipmentForDamage`) |
| Vehicle exits in the interface | The paths in `Handle_UI.cc` are many and were written for vehicles that never attack: other vehicles must behave as before |
