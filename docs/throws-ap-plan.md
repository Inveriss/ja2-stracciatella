# Fixed AP cost for throws: plan

Status: **plan, not implemented, all decisions taken** (T1-T10, user, 2026-11-05).
Branch `Weapons_AP_Formula`. A follow-up of `docs/weapons-ap-formula-plan.md`: the same goal
for grenades and thrown items. All numbers come from the code and the data; none were checked
in the game.

## 1. Goal

A throw has one fixed AP cost for everyone, in every situation, like a weapon shot.

- The number in the data is the number the player sees in the game.
- Wounds, fatigue and weight lower the AP a merc has per turn, not the price of a throw.
- Dexterity no longer changes the AP cost. It still affects where the throw lands.

## 2. Today

### 2.1 The formula

`MinAPsToThrow` (`src/game/Tactical/Points.cc`):

```
cost   = AP_CHANGE_TARGET (1, for a new target)
       + standing up (0 / 2 / 4 AP: standing / crouched / prone)
       + round(100 * AP_MAX / bottom)
bottom = TOSSES_PER_10TURNS (18) * (50 + dexterity / 2) / 10
AP_MAX = CalcActionPoints(), lowered by wounds, breath and weight
```

The result is clamped to 1..`AP_MAX`. The throw alone (no new target, no standing up):

| AP_MAX \ dexterity | 0 | 50 | 80 | 100 |
|---|---:|---:|---:|---:|
| 25 | 25 (cap) | 19 | 15 | 14 |
| 20 | 20 (cap) | 15 | 12 | 11 |
| 15 | 15 (cap) | 11 | 9 | 8 |
| 10 | 10 (cap) | 7 | 6 | 6 |

A wounded merc with 10 AP throws for 6 AP, a healthy one with 20 AP for 12: the same flaw the
weapons had, with dexterity in place of marksmanship.

### 2.2 What is thrown

| Item | File | Class |
|---|---|---|
| Hand grenade, stun, tear gas, mustard, mini, smoke grenade, break light, creature cocktail, big tear gas (`cursor` `TOSS`) | `explosives.json` | `IC_GRENADE` |
| Rock, rock 2 | `weapons.json` (type `THROWN`, no AP field) | `IC_THROWN` |

Out of scope: the throwing knife (already a weapon with `ubAPCostPerSingleShot`), launchers and
the mortar (weapons), placed bombs and mines (`AP_DROP_BOMB` 3), and tossing an unarmed item to a
teammate (a flat `AP_TOSS_ITEM` 8).

### 2.3 Where the cost is used

| Place | Use |
|---|---|
| `UI_Cursors.cc:827` | The cost on the throw cursor |
| `Handle_Items.cc` (`IC_GRENADE` / `IC_THROWN` path) | `MinAPsToAttack` for the AP check, then `FireWeapon` → `UseThrown` |
| `Soldier_Ani.cc:911` | AP taken when an armed item leaves the hand (codes 460/461) |
| `Weapons.cc:1654` (`UseLauncher`) | A non-launcher branch that calls `MinAPsToThrow`; launchers take the other branch |
| `Attacks.cc:861` (`CalcBestThrow`) | The AI's rating of a throw, also of mortar and grenade launcher fire |

### 2.4 Details found

- **Turning is free.** `add_turning_cost` is never read, and `HandleItem` sets
  `fDontChargeTurningAPs` for every `TOSS` cursor (`Handle_Items.cc`, "Don't charge turning").
- **Standing up** is shown by the formula and taken by the stance animations. When the item
  leaves the hand the merc already stands, so it isn't taken twice.
- **The AI misjudges the mortar and the grenade launcher.** `CalcBestThrow` rates their fire
  with `MinAPsToThrow` (`ubRawAPCost`, hit rate only), while firing takes their fixed weapon
  cost (16 AP). Whether the AI can afford it is checked with `MinAPsToAttack`, which is right.
- **The AI plans aiming time for throws** (`ubAPCost = ubMinAPcost + ubMaxPossibleAimTime`),
  but a throw takes only `MinAPsToThrow`. To check before changing anything around it.
- **`CalcTotalAPsToAttack`** has a hard-coded 5 AP for `IC_GRENADE` / `IC_BOMB`. No caller
  reaches it with a grenade in hand that I could find.
- `TOSSES_PER_10TURNS` is defined twice (`Points.cc`, `AIInternals.h`).

## 3. Proposed rules

```
throw = ubAPCostPerThrow (+ standing up, + turning, see T4-T5)
```

- One direct AP value per throwable item, the same for every merc.
- No `AP_MAX` and no dexterity in the cost (T3). The upper cap at the merc's AP goes (as D10),
  the lower limit of 1 AP stays.
- Spring and Bolt doesn't apply (it is a gun attachment).

## 4. Decisions

All decided by the user on 2026-11-05: T1 as below, the rest as recommended.

| # | Question | Decision |
|---|---|---|
| T1 | Where the value lives | **Every throwable item has its own entry** `ubAPCostPerThrow`: the 9 `TOSS` grenades in `explosives.json`, the 2 rocks in `weapons.json`. A value missing from the data (e.g. a mod) falls back to T2's 11 AP, with a warning in the log |
| T2 | Starting value | **11 AP** for every throwable item: today's cost at 20 AP and dexterity 100, the same conversion the weapons used (their `round(80 / S4T)` is today's cost at 20 AP and skill 100). For comparison: AK-74 5, M24 10, LAW 16. Can be tuned per item later |
| T3 | Dexterity | Out of the cost, as marksmanship for weapons (D4). It still drives the hit chance (`CalcThrownChanceToHit`) |
| T4 | 1 AP for a new target | **Never** for a throw: like a lowered weapon, every throw is aimed from scratch, so the aim is part of it |
| T5 | Turning | **Paid**, as for weapons since `d2f1e234f`: the merc throws standing, so a standing turn (1 AP) after standing up. `HandleItem` stops setting `fDontChargeTurningAPs` for throws |
| T6 | Cap at the merc's AP | Removed, as D10. A merc with too few AP saves AP or can't throw |
| T7 | AI rating of mortar and grenade launcher | Rate them with `MinAPsToShootOrStab` (their fixed cost) instead of `MinAPsToThrow` |
| T8 | `CalcTotalAPsToAttack` for grenades | Return the throw cost instead of the hard-coded 5, so a future caller can't get a wrong value |
| T9 | Item boxes | Out of scope; showing the throw cost in a grenade's box is a later, separate change |
| T10 | Unarmed item toss (`AP_TOSS_ITEM` 8), bomb placing (3) | Unchanged |

**Balance with T2 = 11:** a healthy merc with good dexterity (20 AP, dex 80) pays 11 instead of
12, a wounded one with 10 AP pays 11 instead of 6, a merc with 25 AP and dex 50 pays 11
instead of 19. Each throw is still one of the most expensive attacks, as grenades are strong.

## 5. Code changes

- `ExplosiveModel.cc/.h`, `explosives.schema.yaml`: `ubAPCostPerThrow` (read, write, schema).
- `WeaponModels.cc`: the `THROWN` (`NoWeapon`) branch reads `ubAPCostPerThrow`;
  `weapons.schema.yaml` describes it.
- `explosives.json`, `weapons.json`: an entry for each of the 9 grenades and the 2 rocks (T1, T2).
- `Points.cc`: `MinAPsToThrow` = standing up + turning (T5) + `ubAPCostPerThrow`, at least 1;
  remove the formula, `TOSSES_PER_10TURNS` and the cap. Also `CalcTotalAPsToAttack` (T8).
- `Handle_Items.cc`: no `fDontChargeTurningAPs` for throws (T5).
- `Attacks.cc`: T7; remove the unused `TOSSES_PER_10TURNS` from `AIInternals.h`.
- Unit test: the value from the data, the fallback, no cap.

No save format change: item data isn't stored in saves.

## 6. Stages

Each stage: build and tests, then game verification by the user, then commit.

1. **Fixed throw cost:** T1-T3, T6, data, loading, schema, `MinAPsToThrow`, T8, unit test.
2. **New target and turning:** T4, T5.
3. **AI:** T7, the duplicate constant; a look at the aiming time the AI plans for throws.
4. **Docs:** this plan's status, a line in `docs/weapons-ap-formula-plan.md`.

### Game checks

- The throw cursor shows the same cost for a wounded and a healthy merc, and for low and high
  dexterity.
- AP shown = AP taken: standing, crouched and prone (standing up), with and without a turn, a
  second throw at the same and at another target.
- Hand grenade, stun, smoke, break light and a rock.
- A merc with fewer AP than the throw: no throw and the usual message.
- One battle with soldiers who throw grenades and fire the mortar or grenade launcher: no
  crash, no AI lockup.

## 7. Open points

- How a crouched or prone merc gets up for the throw (`HandleSoldierThrowItem`): the turn is
  expected after standing up, at the standing cost. To confirm in the game in stage 2.
- The AI's planned aiming time for throws (2.4).
