# Enemy equipment by game progress (Weapons)

Status: E1–E14 and O1–O3 decided (2026-11-08). Stage 1 implemented (field, validation,
base data for 86 items, load warning, 5 unit tests), game-verified, committed e4d2461ff.
Stage 2 implemented (game.json enemy_equipment, P per soldier, sentinel level, O1, tier
rule helper, 9 unit tests), game-verified, committed 384ec629f.
Stage 3 implemented (guns by tiers, trade-off = 10 points, condition from P, first drop
per gun, lowest gun tier marked at start; no save change needed, the dropped set was
already per gun; 3 unit tests; build + 269 tests pass), not yet game-verified.
Branch: `Weapons` (from ja2master dfa36b2ce).
Related: [game-progress-plan.md](game-progress-plan.md) (`GameProgressPercentage`),
[enemy-drop-progress-plan.md](enemy-drop-progress-plan.md) (`minDropProgress`).

## 1. Goal

Today two progress systems work side by side:

- the **old progress** (`HighestPlayerProgressPercentage`: kills, sector control,
  mine income) chooses what enemies and militia carry;
- the **new game progress** (`GameProgressPercentage`: town/SAM visits, quests)
  decides whether a killed enemy may drop an item (`minDropProgress`).

The two values can differ a lot, so enemies may carry weapons that never drop.
The goal: the choice of enemy and militia equipment follows the new game
progress, controlled per item by one data field, without long weapon lists.

## 2. How the choice works today

`GenerateRandomEquipment` ([Inventory_Choosing.cc:109](../src/game/Tactical/Inventory_Choosing.cc)):

```
class = base(soldier type) + relative equipment level (0-4)
      + CalcDifficultyModifier() / 10 - 5           (clamped to 1..11)
```

- base: admin / green militia 1, army / regular militia 3, elite / elite militia 5;
- relative equipment level: from the map editor, or rolled by
  `RandomizeRelativeLevel` from the distance to the palace (strategic soldiers);
- `CalcDifficultyModifier` ([Soldier_Create.cc:1149](../src/game/Tactical/Soldier_Create.cc)):
  difficulty + old progress × 50% + distance from the palace + income/progress
  bumps;
- random ±1 trade-offs (gun vs vest etc.).

What the class picks, per category:

| Category | Today |
|---|---|
| guns | row `class - 1` of `army-gun-choice-extended.json` (Gun Nut is always on) |
| armour, knives, attachments | `ubCoolness == class`, falling back to lower values |
| kits, misc, bombs | `ubCoolness <= class` |
| grenades | hard-coded types bought from a point budget (`bGrenades * class`) |
| face gear, location gear | fixed items |
| GL / LAW / mortar | hard-coded items, chance gated by the rating |

The class also drives counts and chances: magazines, grenades, attachment
chance, special weapons, AP ammo chance (`100 - class * 9`), gun condition.

## 3. Decisions

| # | Decision |
|---|---|
| E1 | One item field **`minEnemyProgress`** (0–100), in every item file, stored on `ItemModel` (like `minDropProgress`). No per-category fields; per-category settings, if ever needed, go to `game.json`. |
| E2 | `minEnemyProgress` and `minDropProgress` are **independent** (variant B). |
| E3 | An item **without** `minEnemyProgress` is never given to enemies or militia by the random choice (fixed editor inventories are untouched). |
| E4 | Values are **multiples of 5** (0, 5, 10 … 100); anything else is a data error at load time. |
| E5 | Progress source: `GameProgressPercentage()`. Enemy stats, AI level, queen phases, story events keep the old progress (separate topic). |
| E6 | Effective progress per soldier: `P = GameProgress + offset + difficulty offset`, rolled **once per soldier** and used for every category. |
| E7 | Editor-placed soldier (has an editor equipment level): offset by level only — Bad −20..−10, Poor −10..0, Average −10..+10, Good 0..+10, Great +10..+20 (uniform random, inclusive). |
| E8 | Strategically created soldier (reinforcements, counterattacks, autoresolve, `TacticalCreateEnemySoldier` / `TacticalCreateMilitia`): offset by type only — Admin −20..−10, Army −10..+10, Elite 0..+20. No distance-from-palace roll. E7 and E8 never add up. |
| E9 | Difficulty offset: Easy −10, Experienced 0, Expert +10 — **enemies only**. |
| E10 | Militia: green = Admin, regular = Army, elite = Elite (E8); militia on enemy map placements use E7; no difficulty offset. Elite Heavy keeps its fixed kit. |
| E11 | Location (distance from the palace, Orta/Tixa/Alma) no longer affects equipment. |
| E12 | Tier rule: a tier is a `minEnemyProgress` value present among the eligible items of a category. Take the **highest tier ≤ P**; if P is below the lowest tier, take the **lowest tier**. Pick uniformly among the items of exactly that tier. |
| E13 | Counts and chances keep the old rules, fed with `class = P / 10 + 1` (P clamped to 0..100, so 1..11). |
| E14 | `army-gun-choice-*.json` stay; a `game.json` switch selects the old table system (extended table) or the new one. Grenades become data-driven in their own stage. |

All numbers of E7–E9 live in `game.json`, the values above are the defaults.

## 4. Further decisions (O1–O3, accepted as recommended 2026-11-08)

- **O1** — Sectors the player once liberated strip detailed placements and
  re-roll the editor level by location ([Soldier_Init_List.cc:1053, 1869](../src/game/Tactical/Soldier_Init_List.cc)).
  Decided: in the new system keep the stored editor level (E7), no re-roll.
- **O2** — GL / LAW / mortar stay hard-coded special weapons, chance gated by
  the E13 class (as today). Decided: yes; making them data-driven is a
  later topic.
- **O3** — "First drop of a new gun" bonus: today it marks a whole gun class as
  dropped (`notUsedGunIndeces[11]` in the save). Decided: mark single
  guns only; the save format changes, so a new game is required (no migration,
  project rule).

## 5. Data and settings

Item files (example):

```json
{ "internalName": "FNFAL", "minDropProgress": 60, "minEnemyProgress": 55, ... }
```

`game.json`:

```json
"enemy_equipment": {
  "by_progress": true,
  "level_offsets":      { "bad": [-20, -10], "poor": [-10, 0], "average": [-10, 10], "good": [0, 10], "great": [10, 20] },
  "type_offsets":       { "admin": [-20, -10], "army": [-10, 10], "elite": [0, 20] },
  "difficulty_offsets": { "easy": -10, "experienced": 0, "expert": 10 }
}
```

Base data, first version: items that enemies can get today receive
`minEnemyProgress = minDropProgress` (the `ubCoolness × 10` placeholder rounded
to a multiple of 5): guns from `army-gun-choice-extended.json`, and armour,
knives, attachments, kits, misc, bombs, grenades with `ubCoolness > 0`.
Values are tuned later.

## 6. Stages

1. **Field and data** — read/validate `minEnemyProgress` (0–100, multiple of 5)
   in `registerItem`, getter on `ItemModel`, base data in the item files, load
   warning when a category has no tier at 0–20; unit tests.
2. **Effective progress** — `game.json` settings, `P` per soldier (E6–E11);
   strategic soldiers marked "no editor level" (sentinel `-1` in
   `bRelativeEquipmentLevel` instead of `RandomizeRelativeLevel` when
   `by_progress` is on); class for counts from `P` (E13); unit tests for the
   offsets and the tier rule.
3. **Guns** — tier choice from the catalogue (E12) instead of the table;
   AP-ammo chance and condition from `P`; "first drop" per gun (O3, save
   version bump).
4. **Armour, knives, attachments, kits, misc, bombs** — same tier choice per
   category (attachments still filtered by `ValidAttachment`).
5. **Grenades** — choose from `IC_GRENADE` items by tier, count from E13;
   GL grenades and mortar shells by the same rule.
6. **Test mod + status** — a mod with distinct tiers, debug log line per soldier
   (`P`, offset, chosen tiers), game verification, plan status.
