# Item field order: minEnemyProgress before minDropProgress (Weapons)

Status: IMPLEMENTED (2026-11-09), game-verified, commit 8db792378 on `Weapons`: 83 swaps
(weapons 38, armours 10, explosives 19, items 16), values unchanged, schema order (F2) changed
too, all 278 unit tests pass. F1-F3 decided as recommended (2026-11-09). Rule F3 stays in force
for new items. Note: `git diff --stat` counts each swap as 1 + / 1 - (83 / 83), not 2 / 2 as
section 4 expected.
Branch: `Weapons` (last commit 4fd20e8ea).
Related: [enemy-equipment-progress-plan.md](enemy-equipment-progress-plan.md) (stage 1 added
`minEnemyProgress`), [enemy-drop-progress-plan.md](enemy-drop-progress-plan.md) (`minDropProgress`).

## 1. Goal

In the item files every entry that has both fields lists them as

```json
"minDropProgress": 20,
"minEnemyProgress": 20,
```

because stage 1 of the enemy equipment plan inserted `minEnemyProgress` on the line right
after `minDropProgress`. The wanted order is the other way round:

```json
"minEnemyProgress": 20,
"minDropProgress": 20,
```

This is a readability change only. JSON object keys have no order for the game: the content
manager reads every field by name (`ApplyMinEnemyProgress`, `ApplyMinDropProgress` in
`ItemModel.cc`), so loading, validation and behaviour stay exactly the same.

## 2. What is affected

Counted in `assets/externalized` on 2026-11-09:

| File | entries with `minEnemyProgress` | of them directly after `minDropProgress` |
|---|---|---|
| weapons.json | 38 | 38 |
| armours.json | 10 | 10 |
| explosives.json | 19 | 19 |
| items.json | 16 | 16 |
| magazines.json | 0 | - |

83 entries in total (86 from stage 1 minus TSHIRT, TSHIRT_DEIDRANNA and LEATHER_JACKET, which
lost the field in stage 4). In every one of them `minEnemyProgress` is the line directly below
`minDropProgress`, with the same indentation, so the change is a swap of two adjacent lines.

Entries with only `minDropProgress` (the other items and all magazines) are not touched.

Not affected:

- the mod `E:\Release1\mods\enemy-equipment-test` and `item-limits-test`: their JSON patches
  add, replace and remove fields by path and name (`/1/minEnemyProgress`), never by position;
- the C++ code and the unit tests: nothing depends on the order;
- the schemas (`types/base-item.schema.yaml`, `magazines.schema.yaml`): properties are
  unordered. Optionally the `minEnemyProgress` block in `base-item.schema.yaml` is moved above
  `minDropProgress` so the documentation shows the same order (cosmetic);
- the build copies (`_bin/Release/externalized`, `E:\Release1\externalized`): the build copies
  the files from `assets/externalized`.

## 3. Decisions (F1-F3 accepted as recommended, 2026-11-09)

| # | Question | Decision |
|---|---|---|
| F1 | Scope | the 4 item files of `assets/externalized` (83 swaps) |
| F2 | Schema documentation order | move the `minEnemyProgress` block above `minDropProgress` in `base-item.schema.yaml` too |
| F3 | Future entries | rule for new items and tools: `minEnemyProgress` always directly above `minDropProgress`; the stage 1 generator in the scratchpad is not kept, nothing else writes these fields |

## 4. Implementation (one step)

1. Script (Python, run with `-I`) over the 4 files: for each line `"minDropProgress": …,`
   directly followed by a line `"minEnemyProgress": …,` swap the two lines; keep every other
   byte of the file (indentation, LF line endings, comments) unchanged. Report the count per
   file and stop with an error when a `minEnemyProgress` line is not directly below a
   `minDropProgress` line (then the file is edited by hand).
2. Check: the counts match section 2; `git diff --stat` shows exactly 2 changed lines per entry
   (166 + / 166 − in total, without F2); a parse of each file still succeeds; every item keeps
   the same values (compare the parsed objects before and after: equal as dictionaries).
3. F2: move the `minEnemyProgress` property block in `base-item.schema.yaml`.
4. Build and run the unit tests (expect all 278 to pass unchanged, including
   `EnemyEquipmentProgress.baseDataForTodaysEnemyItems` and `DropProgressTest.*`).
5. Game check by the user: a new game starts without a data error; the enemy-equipment-test
   mod still loads.
6. Commit on `Weapons` after the user's game check (project rule), e.g.
   `Item files: minEnemyProgress before minDropProgress`.

No save format change, no new game needed.
