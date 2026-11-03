# Player's tank: firing while driving in real time — plan

Status: **plan, not implemented, all decisions taken** (written 2026-11-03 on Remote
Control, branch `Tanks`). Related: `docs/tank-rotating-hull-plan.md` (the hull,
variant A), `docs/tactical-vehicle-driving-analysis.md` (driving rules).

## 1. Goal

The player's tank fires its cannon while it keeps driving, in real time only.
Turn-based combat does not change: there a tank fires standing, by its own rules.
Enemy tanks do not change.

## 2. What exists today

Checked in the code; nothing here has been tried in the game.

### 2.1 Why a moving tank cannot fire

- **One animation state.** `EVENT_FireSoldierWeapon` (`Soldier_Control.cc`) readies
  the weapon, waits for the turn to the target (`fTurningToShoot`) and replaces the
  animation with `SelectFireAnimation()`: for a tank `TANK_SHOOT`, flagged stationary
  and ending in `ANIM_STAND`. So a shot through this path ends the drive.
- **The player's tank has no firing.** It holds no cannon (`EquipTank` in
  `Inventory_Choosing.cc` only equips generated enemies), `CanShootFromVehicle`
  excludes it, and a click with a vehicle selected only means driving.
- **The attack count.** A shot raises `ubAttackBusyCount`; while it is above zero a
  vehicle takes no new drive order (`Turn_Based_Input.cc`).

### 2.2 What helps

- **The shot does not need the animation.** The animation only times the call of
  `FireWeapon()` → `UseGun()`. `Handle_Items.cc` already has a path that raises the
  attack count and calls `FireWeapon()` directly.
- **Attacks are detached from movement.** Bullets, sound (`tank_cannon.wav`), noise
  and hits all come from `UseGun()` and need no state change on the shooter.
- **The cannon exists:** `TANK_CANNON` in `weapons.json`, `TANK_SHELL` calibre.
- **Short overlays exist:** `CreateAnimationTile()` (`Tile_Animation.h`).

### 2.3 Why real time ends quickly

With `turnbased_starts_when_spotted: true` (`game.json`):

- **The attack itself starts the turns.** After an attack by our team that was
  noticed, on a target, `ReduceAttackBusyCount` (`Overhead.cc`) takes
  `ProcessImplicationsOfPCAttack()` and calls `EnterCombatMode()`. The first shot at
  an enemy therefore ends real time, unless this plan changes it (F1).
- **An enemy seeing a merc starts the turns** (`AnEnemySeesAMerc()`, `OppList.cc`).
  Its loop counts every soldier of our team in the sector, with no neutral check.
  Whether the crew and the tank (neutral since `4e15ce057`) actually get seen this
  way is not settled: to check in stage 1.
- **A merc seeing an enemy in real time stops** (`StopMercOnSpotting`, `OppList.cc`).
  What that does to a tank whose crew spots an enemy is also to check in stage 1: a
  tank that stops on every sighting would rarely fire while moving.

## 3. Dependencies

- `docs/tank-rotating-hull-plan.md`: the `TANK_PLAYER` body and the 32 hull frames.
  The mechanics of this plan can be built before the art, but they aim by the hull's
  direction, which only looks right with those frames.
- **Firing a tank at all:** `docs/tank-cannon-firing-plan.md`. Stages 2 and 3 of that
  plan replace stage 2 here. F6 and F7 below are shared with it.

## 4. Decisions

All decided as proposed (user, 2026-11-03). F1 may be revisited after stage 1.

| # | Question | Decision |
|---|---|---|
| F1 | Does a shot from a moving tank start the turns? | **No.** The tank's real time shot doesn't call `EnterCombatMode()` by itself; turns start the usual way, when an enemy sees a merc. Otherwise the feature is one shot long. A whole real time tank battle, as `IsRealtimeFight()` does for militia fights, would be a much larger, separate decision |
| F2 | Where can it aim? | The hull's front only (variant A, the turret is fixed): the direction to the target equals the tank's `bDirection`, an arc of about 45 degrees; the tank keeps its course |
| F3 | During a turn? | Not while the hull is between world directions (`ubHiResDirection % 4 != 0`): it is short, and the flash has only 8 directions |
| F4 | How does the shot look? | An overlay at the muzzle: flash and smoke, no recoil; the hull keeps driving (section 5) |
| F5 | Rate of fire | No AP in real time, so a reload timer: a constant, first 3 seconds, tuned in the game; shown by a refusal message when clicked too early |
| F6 | Who aims? | The tank's own stats are almost all 1 or 0 (`mercs-profile-info.json`), so the chance to hit is taken from the best marksman aboard. Nobody able aboard: no shot. Shared with stationary firing |
| F7 | Ammunition | `TANK_SHELL` from the tank's cargo slots, as a passenger's reload takes ammunition from cargo; the cannon sits in `HANDPOS` (not a cargo slot) and can't be dropped. Today shells are not deducted for tanks by the AI's rule, so `Points.cc` must take the player's tank apart. Shared with stationary firing |
| F8 | Controls | The existing action cursor toggle, for a selected tank in real time: a left click on a target fires without touching the path; a move click stays a move click |
| F9 | Machine gun | Not in this plan |

## 5. Art

No new art is strictly needed for the shot to work. For it to look right:

| Graphic | Frames | Notes |
|---|---|---|
| Muzzle flash and smoke | about 32-64 (8 directions x 4-8 frames, an estimate) | one overlay, the same for any hull colour; STI or PNG |

Also needed in numbers, not in pictures: the muzzle position per world direction, a
table of 8 pixel offsets read off the final hull frames. The overlay stays where it
started; it is too short for the tank's movement to show, which is to judge in the
game.

The alternatives were rejected for now: playing the 64 shot frames of the hull plan
on the move needs a new moving animation state, and an independent turret (variants
B and C) is left for a higher graphics level.

## 6. Stages

### Stage 1: checks, no behaviour

Answer the open points of 2.3 by reading and with debug output: does an enemy see a
tank's crew or the tank; what does `StopMercOnSpotting` do to a driving tank; and
does anything at the end of an attack reset the shooter's animation (`FreeUpAttacker`,
`ReduceAttackBusyCount`, `UnSetUIBusy`). Revisit F1 if the answers change it.

### Stage 2: the cannon aboard

`TANK_CANNON` in `HANDPOS` of the player's tank (`OBJECT_UNDROPPABLE`); shells from
cargo (F7); the gunner (F6). The chance to hit reads the attacker's stats, so
`CalcChanceToHitGun` and friends need the gunner's values for a tank. Experience
gain and kill credit go to the gunner.

### Stage 3: the shot while moving

A new function in `Vehicles.cc`, used only out of combat (`!(uiFlags & INCOMBAT)`):

1. refuse when: not the player's tank, in combat, off the arc (F2, F3), reloading
   (F5), no gunner or shell;
2. set `sTargetGridNo`, `bTargetLevel`, `target`, `usAttackingWeapon`,
   `ubAttackingHand = HANDPOS`, no burst, no aiming, `bBulletsLeft = 1`;
3. raise the attack count and call `FireWeapon()` directly, as `Handle_Items.cc` does;
4. no call of `EVENT_FireSoldierWeapon`, `SoldierReadyWeapon` or any animation.

Check the attack count: a shell in flight must not block the drive already under way,
and a move order given during the flight must not simply be lost (`Turn_Based_Input.cc`
refuses it today).

### Stage 4: the real time rule (F1)

In `ReduceAttackBusyCount`: no `EnterCombatMode()` for such a shot. The target must
still react: under fire, hostile, its noise heard. If combat starts mid-drive, the
existing driving rules take over (`docs/tactical-vehicle-driving-analysis.md`, 2a);
a shot under way must finish cleanly.

### Stage 5: controls (F8)

Action cursor for the tank, target cursor, the refusal messages. New strings in all
translations, as usual.

### Stage 6: the look

The overlay and the muzzle offsets (section 5), tuned on the final hull frames.

## 7. Risks

| Risk | Note |
|---|---|
| The shell hits the tank itself | It starts inside a multi-tile footprint; `LOS.cc` has tank cases, but they were written for standing enemy tanks. Test in all 8 directions, with the new footprint |
| Attack bookkeeping | The attacker's attack is usually tied to its animation; a leftover count locks the interface. The heaviest test of stage 3: shoot, drive, get interrupted, die, leave the sector |
| Shots at neutrals or friends | A shot at civilians or militia must follow the usual consequences, also with F1 |
| Too strong | A tank firing freely before turns start can kill unaware enemies from afar. `usRange` 800 and `ubDeadliness` 80 were set for the AI; F1, F5 and F2 are the brakes. Balance in the game |
| Little use | If 2.3 shows that enemies see the tank at once, real time lasts seconds. Stage 1 exists to find that before the work |
