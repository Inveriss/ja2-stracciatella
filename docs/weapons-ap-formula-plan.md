# Fixed weapon AP cost: plan

Status: **implemented and game-verified** (2026-11-05), branch `Weapons_AP_Formula` (from
`ja2master` 50ae80da9). Stages 1-4 done, see section 8. The plan was written in Remote Control
mode; sections 2-7 keep its reasoning, and the rules in section 3 are the ones in the game.

## 1. Goal

A weapon has one fixed AP cost per shot, for everyone, in every situation.

- A number in `weapons.json` is the number the player sees in the game.
- Wounds, fatigue and weight still lower the AP a merc has per turn. They no longer change
  the price of a shot.
- Marksmanship no longer changes the AP cost. It still affects the chance to hit.

## 2. Before this change

`BaseAPsToShootOrStab` (`src/game/Tactical/Points.cc`):

```
bottom = ((50 + AIM/2) * S4T) / 4          // then Spring and Bolt
base   = round(100 * AP_MAX / bottom)
```

`AP_MAX` is `CalcActionPoints()`, computed live, so wounds, breath and weight lower it. It is
in the numerator, so a weaker merc pays less. An RPK-74 (S4T 13) at marksmanship 80:

| Merc | AP_MAX | Shot cost | Shots per turn |
|---|---:|---:|---:|
| Healthy | 20 | 7 | 2 |
| 20/80 life, not bandaged | 10 | 3 | 3 |
| 40/80 life, bandaged | 10 | 3 | 3 |

The burst surcharge has the same flaw: `max(3, (5 * AP_MAX + 24) / 25)`.

`CalcActionPoints` stays: turn AP, soldier creation, throws and AI threat rating all use it.
`ubShotsPer4Turns` has no other use: the only read in game logic is `Points.cc:1050`.

## 3. New rules

```
single shot = ubAPCostPerSingleShot        - Spring and Bolt
burst       = ubAPCostPer<Mode>Burst - Spring and Bolt
```

Both are direct AP values. A burst field is the full cost of that burst, not a surcharge on
top of the single shot.

**No exceptions.** Mercs, enemy soldiers, militia, civilians, the robot, the player's tank,
enemy tanks, bloodcats and monsters all use these rules, with no other formula.

### Spring and Bolt

- **-1 AP** to the single shot and **-1 AP** to the burst, in every burst mode.
- A fixed value: the attachment's status no longer changes it.
- A cost never falls below 1 AP.
- Each cost drops once. A burst is not reduced twice.

| Weapon | Single | With S&B | Burst | With S&B |
|---|---:|---:|---:|---:|
| AK-74 | 5 | 4 | 9 | 8 |
| RPK-74 | 6 | 5 | 10 | 9 |
| M24 (no burst) | 10 | 9 | - | - |

### Melee

Blades and throwing knives use `ubAPCostPerSingleShot`. Brass knuckles and the crowbar are `IC_PUNCH`:
`MinAPsToPunch` starts from a flat 4 AP and never read this field. They stay as they are; their
field is unused and the schema description says so.

### What stays on top of the cost

The weapon's cost is fixed. These are separate actions, so they are still paid when they happen:
aiming clicks (0 to 4 AP, +10% each), ready time, turning, the 1 AP for a new target, standing
up for a thrown knife or LAW, reloading. Throws have their own formula and are out of scope.

The 1 AP for a new target is charged only with the weapon already raised. Raising the weapon is
part of aiming at the target. FN Minimi (`ubReadyTime` 3, single shot 6):

| Situation | Formula | AP |
|---|---|---:|
| Weapon lowered | `ubReadyTime + ubAPCostPerSingleShot` | 9 |
| Weapon raised, same target | `ubAPCostPerSingleShot` | 6 |
| Weapon raised, another target | `ubAPCostPerSingleShot + AP_CHANGE_TARGET` | 7 |

Turning and raising the weapon are both paid, in every stance (the original game paid only one
of them: standing dropped the turn, crouched and prone dropped the raise). Weapon lowered and the
merc facing away: standing 3 + 6 + 1 = 10, crouched 3 + 6 + 2 = 11, prone 3 + 6 + 6 = 15 (a prone
merc gets up to a crouch, turns and lies down again: 2 + 2 + 2, see `GetAPsToLook`).

A tank never pays the 1 AP for a new target. Turning its turret costs `AP_LOOK_STANDING` (1 AP), and
the player's tank can turn it with the L key (the Look button).

The target is remembered by its tile, so a raised weapon also pays this 1 AP for an enemy who
has moved. `SOLDIERTYPE::fRaisingWeaponForAttack`, not saved, carries the state to the moment
the gun fires, when the weapon is already up.

## 4. Starting data

These are the current values converted with `round(80 / S4T)`, to be tuned freely later.

| `ubAPCostPerSingleShot` | Burst fields | Weapons |
|---:|---:|---|
| 5 | 9 | MP5K, Commando, AKS-74U, P90, C7, FAMAS, AK-74, AKM, FN FAL |
| 5 | - | Beretta 92F |
| 6 | 10 | Glock 18, Beretta 93R, MAC-10, AUG, G41, M14, M14 copy, G3A3, G11, Minimi, RPK-74, HK21E |
| 6 | - | Glock 17, M1911, SKS, Mini-14, dart gun |
| 7 | 11 | MP53, Auto Rocket Rifle |
| 7 | - | SW38, Barracuda, Desert Eagle, Dragunov, combat knife, throwing knife, bloody throwing knife |
| 8 | 12 | Thompson, Type-85, CAWS |
| 8 | - | SPAS-15, Rocket Rifle, flamethrower |
| 9 | - | Automag III, machete |
| 10 | - | M24 |
| 11 | - | M870, underslung grenade launcher |
| 13 | - | Tank cannon (kept close to today, 1 shot per 25 AP turn) |
| 16 | - | LAW, mortar, grenade launcher |

Monster and bloodcat weapons, converted the same way, **to be tuned at a later project stage**:

| `ubAPCostPerSingleShot` | Weapons |
|---:|---|
| 6 | Bloodcat claw, infant spit |
| 8 | Creature claws (4), queen tentacles, queen spit, young and old male spit, bloodcat bite |

- A burst field starts as `ubAPCostPerSingleShot + 4`, which matches today's surcharge for a 20 AP merc.
- All three modes start equal. They can now differ, for example 8 / 10 / 13.
- A weapon with no such burst mode leaves its field `null`.
- A weapon that has a burst mode but no value (for example from a mod) falls back to
  `ubAPCostPerSingleShot + 4`.
- The tank's Minimi is the same weapon as the merc's: 6 AP, burst 10 AP.

## 5. Decisions

All decided by the user on 2026-11-05.

| # | Decision |
|---|---|
| D1 | `AP_MAX`, aim skill and the formula are removed; the cost is a direct value in `weapons.json` |
| D2 | The new field is `ubAPCostPerSingleShot`, also for blades and throwing knives |
| D3 | `ubShotsPer4Turns` is removed completely |
| D4 | No marksmanship add-on: a weapon has one fixed cost (the earlier band proposal is cancelled) |
| D5 | Burst fields hold the full burst cost as direct AP values |
| D6 | Spring and Bolt: fixed -1 AP to the single shot, -1 AP to the burst in every mode |
| D7 | No exceptions: monsters, bloodcats and enemy tanks use `ubAPCostPerSingleShot` too, tuned later |
| D8 | The tank's Minimi has the same values as the merc's |
| D9 | The tank cannon starts at 13 AP |
| D10 | The upper cap at the merc's maximum AP is removed; the lower limit of 1 AP stays |
| D11 | Brass knuckles and the crowbar stay out; their field is unused and documented |
| D12 | Item box thresholds: `BAD_AP_COST` 10, `EXCEPTIONAL_AP_COST` 6 |
| D13 | The earlier rejection of AP units on `Burst_Modes` no longer applies |

**D10 in practice.** A merc with too few AP must save them from the previous turn (up to 5 carry
over) or cannot use that weapon. A LAW costs 16 AP, so a merc with `AP_MAX` 10 (at most 15 AP
available) can't fire it at all.

## 6. Code changes

### `WeaponModels.cc`, `WeaponModels.h`, `weapons.schema.yaml`

- New `ubAPCostPerSingleShot`: read, written, in the constructors and in the schema.
- `ubShotsPer4Turns`: removed from all of these.
- `ubAPCostPerShortBurst`, `ubAPCostPerLongBurst`, `ubAPCostPerFullBurst`: names kept, and they now mean
  AP. Remove the "not an AP cost" comments and schema descriptions.

### `weapons.json`

New values per section 4, and `ubShotsPer4Turns` removed. Generate with a script and review the
diff.

### `Points.cc` and `Points.h`

- `BaseAPsToShootOrStab(OBJECTTYPE const&)`: `ubAPCostPerSingleShot` minus Spring and Bolt, at least 1.
  Both of today's overloads go away, with the `bAPs` and `bAimSkill` parameters.
- `CalcAimSkill`: removed, since only the AP code uses it.
- `MinAPsToShootOrStab`: no `full_aps` and no aim skill. The upper clamp goes (D10), the lower
  limit of 1 AP stays. Dual pistols still pay the higher of the two costs.
- `CalcAPsToBurst(OBJECTTYPE const&, WeaponModes)`: keeps its meaning for the callers, what a
  burst costs on top of a single shot, that is `burst field - ubAPCostPerSingleShot`. Both sides drop by
  1 with Spring and Bolt, so the total drops once. A burst field below the base gives 0, so a
  burst then costs what a single shot does. The rate path and the overload taking a soldier go
  away.
- `DEFAULT_APS`, `DEFAULT_AIMSKILL`: removed.

`CalcActionPoints` keeps every other caller, and `MinAPsToThrow` is untouched.

### `Interface_Items.cc`

The item boxes show the weapon's own values.

- Lines 1780, 1879, 4071, 4084, 4681, 4692: call the new functions.
- Thresholds (D12):

| Constant | Today | Weapons today | New | Effect |
|---|---:|---|---:|---|
| `BAD_AP_COST` | 11 | S4T 8 or lower | **10** | The same weapons |
| `EXCEPTIONAL_AP_COST` | 7 | S4T 12 or higher | **6** | S4T 12 weapons lose the highlight (MP53, Auto Rocket Rifle, knives) |

### AI

- `Attacks.cc:104`, `DecideAction.cc:3257`, `DecideAction.cc:3293`: the new `CalcAPsToBurst`.
- Costs otherwise come from the same functions. Test that AI behaviour still looks sane.

### Saves

Weapon data isn't stored in saves, so the format doesn't change and no new game is required.

## 7. Balance effects

| Merc | Weapon | Today | New | Shots per turn |
|---|---|---:|---:|---|
| 25 AP, skill 90 | RPK-74 | 8 | 6 | 3 to 4 |
| 20 AP, skill 80 | RPK-74 | 7 | 6 | 2 to 3 |
| 20 AP, skill 50 | RPK-74 | 8 | 6 | 2 to 3 |
| 15 AP, skill 80 | RPK-74 | 5 | 6 | 3 to 2 |
| 10 AP, skill 80 | RPK-74 | 3 | 6 | 3 to 1 |
| 20 AP, skill 30 | AK-74 | 7 | 5 | 2 to 4 |
| 20 AP, skill 30 | LAW | 25 | 16 | Fireable by more mercs |

- **Maximum AP become truly valuable.** Agile, experienced mercs fire more often than before.
- **Wounds hurt far more,** as intended. Enemy soldiers are affected the same way.
- **Poor shooters fire faster than before,** since marksmanship no longer slows them. They still
  hit less often.
- **Until tuned, monsters, bloodcats and enemy tanks are much more dangerous.** Their AP are far
  above a merc's and no longer raise their costs. An adult monster with 40 AP goes from about 2
  claw attacks per turn to 5. This is expected (D7) and left for the later tuning stage, so
  combat against them is not representative until then.

## 8. Stages

Each stage: build and tests, then game verification by the user, then commit. All done.

1. **Fixed cost**, single shot and burst together (they couldn't be separated without breaking
   the build): `ba1a5e15a`. Then, within stage 1:
   - names and order of the fields (`ubAPCostPer...`): `25cf13f35`;
   - 1 AP for a new target only with the weapon raised: `4cec6b304`;
   - turning and raising the weapon both paid in every stance: `d2f1e234f`;
   - the plan's prone turn (6 AP): `350030caf`.
2. **Item boxes.** The two thresholds: `5a7b467f3`.
3. **Battles** against soldiers, creatures, bloodcats and an enemy tank, and the player's tank:
   game-verified; for the tank, no new target AP and the L key turning the turret: `a28fd4fe7`.
4. **Docs.** `docs/burst-modes.md` and the AP sections of `docs/tank-cannon-firing-plan.md`
   and `docs/tank-machine-gun-plan.md`.

### Game checks

- The cursor shows the same cost before and after a merc is wounded, tired or overloaded.
- Two mercs with different marksmanship and AP pay the same for the same weapon.
- The value in `weapons.json` matches the item box and the cursor, once ready, turn, new target
  and aiming clicks are accounted for.
- The AP shown equal the AP taken: single shot, aimed, each burst mode, with a turn, with a raise.
- Spring and Bolt: -1 to single and -1 to each burst mode, at low and at high status.
- Dual pistols, underslung launcher, mortar, LAW, knife stab, thrown knife.
- A merc with few AP and a heavy weapon: no shot and the usual message, per D10.
- Tank cannon at 13 AP; tank Minimi at the merc's values.
- One battle each against soldiers, creatures, bloodcats and an enemy tank: no crash or AI lockup.

## 9. Later stages and follow-ups

- **Tune monsters, bloodcats and enemy tanks** (planned by the user).
- **Tune weapon and burst values**, including different costs per burst mode.
- **Throws.** `MinAPsToThrow` has the same flaw (`AP_MAX` in the numerator, dexterity).
- **The AI burst estimate.** `DecideAction.cc` sets `ubAimTime = BURSTING` (5) just before
  subtracting it from the cost, so it always subtracts 5, not the real aim time.
- **`ubReadyTime` on launchers.** It is read only for `IC_GUN`, so a launcher's value is ignored.
