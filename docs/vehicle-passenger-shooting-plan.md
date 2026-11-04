# Shooting from inside ground vehicles: implementation plan

Status: plan only, nothing implemented (2026-11-01). Branch: `Vehicle_Inventories`.

## 1. Goal (user's requirements)

1. Passengers of ground vehicles can shoot from inside the vehicle.
   Excluded: the driver, the helicopter and the tank.
2. The driver drives and cannot shoot. Selecting the vehicle or the driver
   only allows driving it in tactical, by the current rules.
3. Passengers with a weapon shoot by the current procedures and code.
4. Only the bullets flying from the vehicle are shown (no visible shooter).
5. A passenger shooting from a vehicle counts as crouched, without the aiming bonus
   and with a penalty of 25 (`docs/vehicle-passenger-hit-penalty-plan.md`).
6. Turning to shoot in turn-based combat costs the crouched cost, 2 AP.
7. An empty weapon reloads from the vehicle's cargo as if the ammo were in the
   merc's own inventory.

## 2. What exists today

- `AddSoldierToVehicle` (`src/game/Tactical/Vehicles.cc`) takes the merc off the
  map grid (`RemoveSoldierFromGridNo`); the first one in is `SOLDIER_DRIVER`,
  the others `SOLDIER_PASSENGER`. Passengers stay in the sector and follow the
  vehicle's position (`UpdateAllVehiclePassengersGridNo`).
- Ground vehicle passengers already look around: `OppList.cc` excludes only the
  helicopter's passengers from sight.
- Selecting a driver or a passenger in the team panel selects the vehicle
  (`Interface_Panels.cc`, `gpSMCurrentMerc->uiStatusFlags & (SOLDIER_DRIVER |
  SOLDIER_PASSENGER)` → `HandleLocateSelectMerc(&vs)`).
- Passengers are skipped by `FindSoldier` (`Soldier_Find.cc`), by next-merc
  cycling (`Handle_UI.cc`) and by select-all (`Turn_Based_Input.cc`).
- In the team panel a passenger's hand slots show vehicle icons; the second one
  is the exit button.
- Entering a vehicle in combat is allowed for AP (`Vehicles.cc`,
  `AP_ENTER_EXIT_VEHICLE`).
- Limitation that stays: entering a battle sector from the map screen takes
  everyone out of the vehicles (PreBattle,
  `PutNonSquadMercsInBattleSectorOnSquads(TRUE)`). Shooting from a vehicle
  applies to mercs who get in, or drive, in the tactical sector.

## 3. Firing pipeline (unchanged, reused)

`HandleItem` (`Handle_Items.cc`) → AP check (`GetAPChargeForShootOrStabWRTGunRaises`,
`EnoughPoints`) → `SendBeginFireWeaponEvent` → `EVENT_FireSoldierWeapon`
(`Soldier_Control.cc`) → turn / ready / fire animation → animation frames call
`FireWeapon` → `UseGun` (`Weapons.cc`); bursts are driven by animation frames
(`Soldier_Ani.cc`, codes 430/448/449).

A passenger has no level node, so the animation runs but is never drawn: only
the bullets (plus sound and muzzle flash) are seen. This meets requirement 4
without a separate firing path.

## 4. Stages

### Stage 1: selecting a passenger

- New helper in `Vehicles.cc/.h`: `CanShootFromVehicle(SOLDIERTYPE const&)`.
  True for `SOLDIER_PASSENGER` (not `SOLDIER_DRIVER`) in a vehicle that is not
  the helicopter and not the tank (`VehicleTypeOfSoldier`).
- `Interface_Panels.cc`: clicking such a passenger's portrait selects the
  passenger; the driver still selects the vehicle.
- `Handle_UI.cc` next/previous merc: include such passengers.
- `Soldier_Find.cc`: unchanged (a click on the map hits the vehicle).
- Done when: a passenger can be selected and is shown as selected; the driver
  and the vehicle behave as before.

### Stage 2: crouched stance and blocks

- On entering a vehicle (`AddSoldierToVehicle`) a passenger who can shoot gets
  the `CROUCHING` state at once (no transition animation). On exit
  (`RemoveSoldierFromVehicle` / `ExitVehicle`) back to `STANDING`.
- Turning then costs `GetAPsToLook()` = `AP_LOOK_CROUCHED` (2), the ready and
  muzzle heights are the crouched ones: requirements 5 and 6 need no new
  AP code.
- `HandleMoveModeInteractiveClick`: a selected passenger does not walk; show a
  message (new string, all translations).
- Stance buttons and stance keys disabled for passengers.
- Done when: a selected passenger stays crouched, cannot walk or change stance,
  and turning in turn-based combat costs 2 AP.

### Stage 3: the shot

- Allow the attack for a passenger who can shoot (first hand slot shows the
  weapon in the team panel; second stays the exit button).
- Blocks: a passenger cannot shoot while the vehicle is moving; the vehicle
  cannot start moving while an attack is busy (`ubAttackBusyCount`).
- Check in the game that the invisible animation of a soldier without a level
  node runs without errors (similar guards exist from the militia real-time
  battle work).
- Done when: single shots, bursts and two-pistol shots work from the vehicle,
  only the bullets are seen, APs are charged by the current rules.

### Stage 4: the bullet must not hit its own vehicle

- The vehicle is a `STRUCTURE_PERSON` structure and the passenger's bullet
  starts at the vehicle's centre. LOS ignores only the firer and the target
  (`LOS.cc`, bullet movement and chance to get through); the own vehicle is
  only spared by the friendly-fire distance (`MIN_DIST_FOR_HIT_FRIENDS` 30,
  unaimed 15).
- In both places treat "the vehicle the firer is in" like the firer.
- Done when: shots and buckshot from any seat never hit the own vehicle, in any
  direction.

### Stage 5: ammo from the vehicle's cargo

- Today an empty gun turns the cursor into "reload" and a click or the R key
  calls `AutoReload`.
- `FindAmmoToReload` returns a slot index of the merc. Add a variant returning
  the ammo object: the merc's pockets first, then the vehicle's cargo slots
  (`IsVehicleCargoSlot`, not the fuel slots).
- Use it in `AutoReload` (`Items.cc`), `GetAPsToAutoReload` (`Points.cc`) and
  the two cursor checks (`UI_Cursors.cc`). `ReloadGun` / `AttachObject` work on
  any object, so a magazine from the vehicle's slot is accepted. Refresh the
  vehicle's panel after reloading. AP cost as today.
- Done when: a passenger with no ammo of his own reloads from the vehicle's
  cargo; the vehicle's stack goes down.

### Stage 6: checks and translations

- Enemy AI shooting at a passenger they see: the shot at the passenger's tile
  should hit the vehicle; passengers take no damage until the vehicle is
  destroyed (current rule).
- Interrupts after a passenger's shot work as for any merc.
- New strings in all 9 `translation-*.json` files.

## 5. Not affected

- No savegame format change.
- The helicopter and the tank: no change.
- The driver: no change.
