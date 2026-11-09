# Enemy equipment: nothing below the lowest tier (variant A, Weapons)

Status: plan only (2026-11-09, written in Remote Control mode), variant A chosen, B1-B6
decided as recommended (2026-11-09), not implemented.
Branch: `Weapons` (last commit 9c99cd64e).
Related: [enemy-equipment-progress-plan.md](enemy-equipment-progress-plan.md) (E12, stages 3-5).

## 1. Problem

Since stage 4, even admins at the very start of the game wear a Steel Helmet and a Flak
Jacket.

Cause:

1. `GenerateRandomEquipment` always asks admins and green militia for a helmet and a vest
   (`bHelmetClass = bVestClass = bRating`, class >= 1), as in the original game.
2. The original choice looked for `ubCoolness == class` and stepped down to 0; the weakest
   helmet and vest have coolness 2, so a class 1 admin found nothing and wore no armour. This
   was a side effect of the data, not an explicit rule.
3. The new tier rule E12 (`ChooseEnemyEquipmentTier`, `Inventory_Choosing.cc:2400`) never
   returns "nothing": when P is below the lowest tier it takes the lowest tier
   (decision d.1 of the user: "if no tier is that low, take the first higher tier"). Steel
   Helmet and Flak Jacket have `minEnemyProgress` 20 (base data), so an admin with P 0 gets
   them.

## 2. Goal (variant A)

E12 is split by category:

- **guns**: unchanged, P below the lowest tier takes the lowest tier: every soldier keeps a gun;
- **all other equipment** (helmet, vest, leggings, knives, gun attachments, kits, misc items,
  bombs, hand grenades): P below the lowest tier gives **nothing** of that category.

"P above the highest tier takes the highest tier" (d.2) stays for every category.

Effect with the base data: an admin (offset -20..-10) gets a Flak Jacket only once his P
reaches 20, i.e. from about 30-40% game progress; an army soldier (-10..+10) from about 10-30%;
an elite (0..+20) from 0-20%. Weak soldiers sometimes wear no armour again, as in the original.

## 3. Decisions (B1-B6 accepted as recommended, 2026-11-09)

| # | Question | Decision |
|---|---|---|
| B1 | Scope of "nothing below the lowest tier" | every category except guns (section 2) |
| B2 | Ammunition of a special weapon: grenade launcher grenades, mortar shells | keep "take the lowest tier": a soldier who was given a grenade launcher or a mortar always gets something to fire (they are only handed out at high ratings anyway) |
| B3 | First aid kit / medical kit (50% / 25% fixed picks in `ChooseKitsForSoldierCreateStruct`) | unchanged: given whenever they have `minEnemyProgress`, their tier is not compared with P (as in the original, "always useful"); the other kits follow B1 |
| B4 | Ceramic plates | unchanged (already only when the plates' tier <= the vest's P) |
| B5 | Stage 1 load warning (`MissingLowEnemyTiers`, `ItemModel.cc:357`) | keep, reword per category: guns "weaker soldiers get that tier", other categories "soldiers below it get none" |
| B6 | Switch | none: the original behaviour is what A restores; `enemy_equipment.by_progress: false` still gives the whole old system |

## 4. Code changes

`src/game/Tactical/Inventory_Choosing.cc` / `.h`:

1. `ChooseEnemyEquipmentTier(tiers, progress, bool lowestWhenBelow)`: with `false` it returns
   -1 when every tier is above `progress` (instead of the lowest tier). Callers:
   - `SelectEnemyGunByProgress` (`:2427`): `true` (B1);
   - `PickEnemyItemByProgress(candidates, progress, bool lowestWhenBelow = false)` (`:2477`):
     passes it on; -1 already gives `NOTHING`.
2. Grenades (`ChooseGrenadesForSoldierCreateStruct`, `:939-947`): `lowestWhenBelow` is `true`
   for the grenade launcher and the mortar (B2), `false` for hand grenades.
3. Armour (`ChooseArmourPieceByProgress`, `:1197`), knives (`:1414`), attachments (`:816`),
   other kits (`:1598`), misc (`:1685`), bombs (`:1750`): default `false`, no change at the
   call sites; `NOTHING` already means "nothing in this slot".
4. B5: the warning text in `MissingLowEnemyTiers` (`src/externalized/ItemModel.cc:357`).

No data change, no save format change, no new game needed.

## 5. Tests

`src/game/Tactical/Inventory_Choosing_unittest.cc`:

- `EnemyEquipmentTier.belowTheLowestTierTakesTheLowest`: run with `true` (unchanged result),
  add the `false` case (-1 for progress 0 and 25 with tiers 30, 50);
- `vestsByTiers`: P 0 gives `NOTHING` (Flak Jacket tier 20), P 20 gives Flak Jacket;
- `grenadesByTiers`: hand grenades at P 0 give `NOTHING` (Break Light tier 30); grenade
  launcher at P 0 still GL Stun (B2);
- `candidatesWithoutMinEnemyProgressNeverPicked`: kits at P 0 give `NOTHING` (Locksmith kit
  tier 30), at P 30 Locksmith kit;
- guns unchanged (`gunsOfTheChosenTierOnly`: P 0 still SW38 / Barracuda / Desert Eagle);
- `EnemyEquipmentProgressField.warnsAboutMissingLowTiers` (externalized): new texts (B5).

## 6. Game check (by the user)

Base data, no test mod (its lowest armour and grenade tiers are 0, so it does not show the
change), cheats on (CTRL+GABBY):

1. Progress 0, Experienced: ALT+A admins have no helmet, vest or leggings; ALT+B army soldiers
   mostly none (P 0-10 < 20); ALT+Q elites with P 20 a Flak Jacket / Steel Helmet.
2. Every soldier still has a gun.
3. Higher progress: armour appears once P reaches 20 (admin from about 30-40% progress).
4. Soldiers with a grenade launcher still carry GL grenades.

## 7. Stages

One stage: code + tests (sections 4-5), build, all unit tests, game check (section 6),
commit on `Weapons` after the user's check, plan status in
`enemy-equipment-progress-plan.md` (E12 amended: "below the lowest tier: guns and launcher
ammunition take the lowest tier, other equipment none").
