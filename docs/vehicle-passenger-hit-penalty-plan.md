# Passengers shooting from vehicles: a hit chance penalty — plan

Status: **plan, not implemented, all decisions taken** (written 2026-11-03 on Remote
Control). Changes the rules of `docs/vehicle-passenger-shooting-plan.md`.

## 1. Goal (user's requirements)

For passengers who can shoot from inside a vehicle:

1. No bonus for the crouched stance.
2. A penalty of 30 to the chance to hit.
3. The penalty is part of the formula of the **Base** field in the weapon boxes
   (`Infobox_...` and `ItemInfoC_...`).

## 2. What exists today

Checked in the code; nothing here has been tried in the game.

- **Who:** `CanShootFromVehicle()` (`Vehicles.cc`): `SOLDIER_PASSENGER`, not the driver,
  in any ground vehicle except the tank. The helicopter is excluded too. This covers the
  Hummer, the ice cream truck, the Eldorado and the Jeep.
- **The real chance:** `CalcChanceToHitGun()` (`Weapons.cc`). It has no rule for
  passengers. A passenger is kept crouched (`CrouchShootingPassenger`), so the crouched
  stance gives `iRange / 2`, capped at `AIM_BONUS_CROUCHING` (5). In practice that is +5
  from the first tile on.
- **The cursor:** `UI_Cursors.cc` shows the same `CalcChanceToHitGun()`, so a change there
  shows at once.
- **The Base field:** `RenderItemDescriptionBox` (`Interface_Items.cc`):
  `GunLaserScopeBonus(obj) + GunCrouchStanceBonus(gpItemDescSoldier) +
  GunRoofBonus(gpItemDescSoldier)`, always printed with a sign. It is a display only, a
  simplified copy of the real rules: `GunCrouchStanceBonus()` returns a fixed
  `CROUCH_STANCE_DISPLAY_BONUS` (5) for a crouched soldier.
- **Which boxes:** that block runs for `!in_map || IsMapWeaponBox()`, so one change covers
  all boxes with a Base field, including the big ones. The old shared map box of the other
  resolutions has no Base field, and stays so.

## 3. Rules

| | Today | After |
|---|---|---|
| Passenger, real chance | +5 for crouching | no bonus, -30 |
| Passenger, Base field | +5 for crouching | no bonus, -30 |
| Everyone else | | no change |

"-30" means 30 percentage points off the chance, as all the penalties here: 70% becomes
40%. It does not mean 30% of the chance. The chance stays within the usual limits
(`MINCHANCETOHIT`), so a poor shot from a vehicle ends at the minimum.

Unchanged: the passenger stays crouched. For someone sitting in a vehicle the stance is
above all the bullet's height and the cost of turning, so the code can keep using the
existing mechanisms (user, 2026-11-03). Only the aiming bonus goes.

Where the penalty is taken in the formula: **at the end, right before the limits**. Taken
at the stance bonus, later steps (wounds, fatigue, range halving) would scale it, and it
would no longer be the 30 the Base field shows.

The value goes in one place, used by both the real chance and the display:
`AIM_PENALTY_VEHICLE_PASSENGER` (30) in `Weapons.h`.

## 4. Decision

| # | Question | Decision |
|---|---|---|
| P1 | The **Prone** field stands next to Base; its formula does not contain the crouch bonus, and the requirements name only Base. A passenger can't lie down, so it would show a chance he can never use. Add the penalty there too? | **No**, Prone doesn't change (user, 2026-11-03) |

## 5. Stages

One stage, about 15 lines.

`Weapons.h`:
- `#define AIM_PENALTY_VEHICLE_PASSENGER 30`
- `INT8 GunVehiclePassengerPenalty(SOLDIERTYPE const*);`

`Weapons.cc`:
- `GunVehiclePassengerPenalty(s)`: `s && CanShootFromVehicle(*s) ?
  -AIM_PENALTY_VEHICLE_PASSENGER : 0`. (`Vehicles.h` is already included.)
- `GunCrouchStanceBonus(s)`: 0 when `CanShootFromVehicle(*s)`.
- `CalcChanceToHitGun()`:
  - the crouched bonus only when `!CanShootFromVehicle(*pSoldier)`;
  - `iChance += GunVehiclePassengerPenalty(pSoldier);` right before "MAKE SURE CHANCE TO
    HIT IS WITHIN DEFINED LIMITS".

`Interface_Items.cc`:
- `baseBonus` adds `GunVehiclePassengerPenalty(gpItemDescSoldier)`.
- The comments of the Base field (both stat tables and the block itself) name it.

`docs/vehicle-passenger-shooting-plan.md`: a note at requirement 5, "counts as crouched,
without the aiming bonus, and with the penalty".

No new strings, assets or save changes.

## 6. Tests in the game

- A passenger in the Hummer and in the ice cream truck, in a battle: the cursor's chance is
  about 35 lower than the same merc crouched on the ground (5 + 30), at the same distance.
- The Base field, tactical and map, normal and big boxes (`Show Large Icons`):
  - no scope, aboard: `-30%`, on the ground crouched: `+5%`, standing: `+0%`;
  - with a laser scope: its bonus is added;
  - out of the vehicle the same merc's field returns to normal, with the box reopened.
- A driver, the tank (no passenger shots) and the helicopter: no change.
- Very low chance: the minimum holds, no negative values.
- Aimed shots and bursts from a vehicle fire normally.
- Enemies and militia: no change; they are never passengers.

## 7. Risks

| Risk | Note |
|---|---|
| The Base field is opened for a merc other than the one aboard | It uses `gpItemDescSoldier`, the box's own merc, as the crouch bonus does |
| An open box does not refresh when the merc gets out | The same as the crouch bonus, read when the box is drawn. Check in the test above |
| Too harsh or too soft | The one constant makes tuning simple |
