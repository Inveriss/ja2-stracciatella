# Enemy drops limited by game progress - plan

Status: **COMPLETE - stages 1-4 implemented and game-verified (2026-11-07); D1-D12 decided.**
Written 2026-11-07 from an analysis of branch `Item_Limits` at 63390995c.

| Stage | Commit | State |
|---|---|---|
| 1 - data, switch, check | 83abcf774 | game-verified |
| 2 - deaths, auto-resolve, knocked-out items, D10 | b5328ccb4 | game-verified |
| 3 - barrel extenders, thrown knives, pickups (D9) | ebcb32104 | game-verified |
| 4 - test mod and status | this commit | test mod `item-limits-test` 1.5.0 (outside the repository) |

Stage 1 notes: `minDropProgress` is read in `DefaultContentManager::registerItem()`, so
every item file (items, weapons, magazines, armours, explosives) has it; the item part of
the check is `ApplyDropProgressLimit(o, progress)`, the soldier part
`ApplyEnemyDropProgressLimit(s, o)`. Bit 0x02 (`OBJECT_PICKED_UP`) was never set by any
code, so map data can't carry it. A gun's loaded magazine is not limited (only the gun
and its attachments, D4).

Stage 2 notes: path A (`Rotting_Corpses.cc`), path B (`Tactical_Save.cc`) and path C
(`Soldier_Control.cc`, the hand item stays in the hand) call `ApplyEnemyDropProgressLimit()`;
D10 uses `EnemyDropBlockedByProgress(usItem)` in `Inventory_Choosing.cc`. The test mod
`item-limits-test` already has the stage 4 thresholds: `LASERSCOPE` 80, `SILENCER` 50,
`GLOCK_17` 30.

Stage 3 notes: a falling barrel extender (`Weapons.cc`) and a thrown knife on the ground
(`Weapons.cc`, `LOS.cc` window hits) or in the target's pocket (`LOS.cc`) go through
`ApplyEnemyDropProgressLimit()` with the shooter/thrower. `SoldierGetItemFromWorld()` calls
`MarkObjectPickedUpBy()`: a non-player soldier sets `OBJECT_PICKED_UP`, a merc clears it; a
falling extender inherits the gun's mark. A thrown knife is created anew, so a knife an
enemy picked up and threw again is limited. Test mod: `GUN_BARREL_EXTENDER` 60,
`THROWING_KNIFE` / `BLOODY_THROWING_KNIFE` 40.

Related: [item-limits-plan.md](item-limits-plan.md),
[weapon-attachments-plan.md](weapon-attachments-plan.md).

## Goal

Every item can have a minimum game progress for enemy drops, e.g. `LASERSCOPE` at 80 %.
Below that progress an enemy never leaves the item behind: not when killed (tactical or
auto-resolve), not when it is knocked out of his hands by a hit or an explosion, not as a
part of another item (an attachment on a dropped gun). Stealing the item straight from an
enemy's hands stays allowed.

## 1. How the game computes progress

`Campaign.cc` ~1189, `CurrentPlayerProgressPercentage()`, 0-100:

| part | formula | weight (game.json `progress`) |
|---|---|---|
| income | `PredictIncomeFromPlayerMines() * portion / CalcMaxPlayerIncomeFromMines()` | `weight_income` 50 |
| kills | `usPlayerKills / kills_per_point_{easy,medium,hard}` (7 / 10 / 15), capped at the portion | `weight_kills` 25 |
| control | `CalcImportantSectorControl()` (~1401): owned sectors of the mining towns + the 3 wilderness SAMs (counted double), capped at the portion | `weight_control` 25 |

Each portion = 100 x weight / (sum of the weights). Special cases:

- returns 0 in the map editor;
- a Lua script can override the result (`OnCalcPlayerProgress` observable);
- current progress **can go down**: a lost mine or town sector lowers income and control;
- kills come from tactical deaths (`Soldier_Ani.cc` ~3006), auto-resolve
  (`Auto_Resolve.cc` ~3694, ~3838) and the "enemies wiped out" shortcuts
  (`PreBattle_Interface.cc` ~1130/1165, `Turn_Based_Input.cc` ~2073 debug).

`HighestPlayerProgressPercentage()` (~1278) returns `gStrategicStatus.ubHighestProgress`,
the record. `HourlyProgressUpdate()` (~1290) raises it once per game hour and fires the
Madlab / Mike / Iggy events at `event_*_min`. The record never goes down and is saved with
the game. The enemies' equipment quality already uses it (`Inventory_Choosing.cc`), as do
dealers, the queen's AI phases and quest facts.

So the player raises progress by: taking and keeping mines (half of it), killing enemies,
taking town sectors and SAM sites. Losing mines/sectors lowers only the current value.

## 2. How `drop_everything` works today

`game.json` `"drop_everything": true` -> `GamePolicy::f_drop_everything`
(`DefaultGamePolicy.cc` 23). It is read in **one place only**.

### Item flags decide what drops

When an enemy is created, `Inventory_Choosing.cc` marks his items `OBJECT_UNDROPPABLE`
and then `RandomlyChooseWhichItemsAreDroppable()` (~1698) clears the flag on a few:
ammo 50 %, grenades 25 %, other equipment 15 %, and a weapon type never dropped before
in this game always drops (`WasWeaponAlreadyDropped`). Not during auto-resolve.
`CreateItem()` (Items.cc ~3085) also sets the flag for items with
`ITEM_DEFAULT_UNDROPPABLE`.

### The drop paths of an enemy

| # | Where | When | Uses `drop_everything` |
|---|---|---|---|
| A | `Rotting_Corpses.cc` ~781 `TurnSoldierIntoCorpse()` | killed in a loaded sector | **yes**: drops an item when it is droppable, OR the soldier is ours, OR `f_drop_everything`; never `ITEM_DEFAULT_UNDROPPABLE`; ammo cut to 1..n magazines by `ReduceAmmoDroppedByNonPlayerSoldiers()` |
| B | `Tactical_Save.cc` ~831 `AddDeadSoldierToUnLoadedSector()` | killed in auto-resolve (`Auto_Resolve.cc` ~2337) or in an unloaded sector | **no**: 75 % of the items are kept anyway, `OBJECT_UNDROPPABLE` never drops |
| C | `Soldier_Control.cc` ~5808 in `SoldierTakeDamage()` | any damage except bleeding, **explosions included** | **no**: chance = 2 x max(life lost, breath lost / 100) - strength (+20 for bloodcats); -30 for an undroppable hand item, and an undroppable hand item never falls anyway. Only `HANDPOS` |
| D | `Weapons.cc` ~2552 | a worn barrel extender falls off the gun | no |
| E | `Weapons.cc` ~2013, `LOS.cc` ~1850 | a thrown knife lands on the ground / in the target's pocket | no |
| F | `Keys.cc` ~1279 `DropKeysInKeyRing()` | after A and B | no |
| - | militia | never drop anything (project change) | - |

With `drop_everything` true, path A drops everything; path C still never drops a hand
item that was not chosen as droppable. Explosions don't have their own code: an
explosion calls `SoldierTakeDamage()` with `TAKE_DAMAGE_EXPLOSION`, so it is path C,
and an enemy killed by it is path A. So the paths **don't** share one check: A has the
policy, B has its own 75 % rule, C only looks at the flag.

### Stealing

`Weapons.cc` ~1474 (`UseHandToHand()`, `fStealing`): a good steal moves the enemy's hand
item to the thief (`AutoPlaceObject`), or to the thief's tile when he has no room; a
half-good steal drops the item on the thief's or the target's tile. This doesn't go
through A-F.

### Other drops that are NOT enemy drops

Player drops, throws and item swaps (`Handle_Items.cc`, `Interface_Items.cc`, map
inventory), NPC dialogue gifts (`Interface_Dialogue.cc`), deliveries, the captured
player's items (`Queen_Command.cc` ~1185), dealers. They stay untouched.

## 3. Design

One helper decides what an enemy may leave behind:

```cpp
// Items.h
// true: the object may drop (blocked attachments were removed);
// false: the whole object must not drop
bool ApplyEnemyDropProgressLimit(SOLDIERTYPE const& s, OBJECTTYPE& o);
```

- Applies only to the teams of D5; returns true for everyone else.
- An object marked as picked up from the world (D9) is never limited.
- `UINT8 progress = HighestPlayerProgressPercentage()` (D2).
- The object's item has `minDropProgress > progress` -> false.
- Each attachment with `minDropProgress > progress` is removed (lost) from the object
  (D4).

Item data: a new optional field `"minDropProgress": 0-100` on every item (base item
schema, `ItemModel`), default 0 = no limit. Out of range -> `DataError` at load.

## 4. Decisions

All decisions were taken by the user on 2026-11-07, D1-D12 as recommended.

| ID | Question | Decision |
|----|----------|----------------|
| D1 | Where the threshold lives | per item `minDropProgress` in the item JSON (items.json, weapons.json, magazines.json, armours.json ... via the base item schema), default 0 |
| D2 | Which progress | **highest** (the record): never goes down, already used for the enemies' gear; lags by at most one game hour. Alternative: current (falls when mines are lost) |
| D3 | What happens with a blocked item | it is **lost** (stays with the corpse, not on the ground) |
| D4 | Blocked attachment on an allowed gun / blocked gun | attachment removed and lost, the gun drops; a blocked gun is lost **with** its attachments and ammo |
| D5 | Whose drops | **ENEMY_TEAM** only (the army). Alternative: also creatures and hostile civilians (quest NPCs would then lose scripted items) |
| D6 | Steal exemption | the whole steal action is exempt: a good steal and the half-good steal that drops the item on the ground |
| D7 | Auto-resolve / unloaded sector (path B) | gets the limit; its own 75 % rule and its not reading `drop_everything` stay as they are (separate question) |
| D8 | Barrel extender falling off (D), thrown knives (E) | limit both (an enemy's item reaching the player); keys (F) never limited |
| D9 | Items an enemy picked up from the ground (possibly the player's own) | **exempt**: new object flag `OBJECT_PICKED_UP` (bit 0x02, free) set when a non-player soldier picks an item up; the limit never destroys a player's item |
| D10 | "A never-dropped weapon type always drops" | a weapon blocked by progress is **not** recorded as dropped, so its first real drop later still counts as the first |
| D11 | Switch | game.json `"enemy_drop_progress_limits": true` (false = old behaviour) |
| D12 | Blocked hand item knocked out by a hit/explosion (path C) | **stays in the hand** (the enemy keeps it, it is not dropped). Alternative: it is destroyed |

## 5. Code changes

| File | Change |
|---|---|
| `ItemModel.h/.cc`, `base-item.schema.yaml` (+ weapons/magazines/... schemas if they repeat fields) | `minDropProgress` read/validated |
| `GamePolicy.h`, `DefaultGamePolicy.cc`, `game.json` | `enemy_drop_progress_limits` (D11) |
| `Items.h/.cc` | `ApplyEnemyDropProgressLimit()`; `OBJECT_PICKED_UP` in `Item_Types.h` |
| `Rotting_Corpses.cc` ~792 (path A) | `if (!ApplyEnemyDropProgressLimit(s, *pObj)) continue;` before `AddItemToPool` |
| `Tactical_Save.cc` ~876 (path B) | same before `AddItemsToUnLoadedSector` |
| `Soldier_Control.cc` ~5840 (path C, hits and explosions) | blocked -> the item is lost (`DeleteObj`) or, alternative, stays in the hand; recommendation: stays in the hand, so the enemy keeps fighting with it and the item is simply not dropped |
| `Weapons.cc` ~2552 (D), ~2013 and `LOS.cc` ~1850 (E) | limit the created object; the thrower/owner must be known at that point (check) |
| `Handle_Items.cc` `SoldierGetItemFromWorld()` ~1199 | set `OBJECT_PICKED_UP` for non-player soldiers (D9); cleared when a merc takes the item |
| `Inventory_Choosing.cc` ~1820 | D10: don't mark a progress-blocked weapon as already dropped |
| `Weapons.cc` ~1474 (steal) | **no change** (D6) |

Save games: no format change (the progress record is saved already; `fFlags` exists).
New flag meaning -> per project rule a new game is expected anyway.

## 6. Risks and side effects

- Fewer weapons and attachments early: with `drop_everything` true most of the loot
  comes from corpses, so thresholds reshape the economy; dealers and Bobby Ray are not
  limited by this plan.
- Enemies still **carry and use** blocked items; only the drop is suppressed. The
  enemies' equipment already scales with progress, so this mostly hits special
  placements (map-placed elites, Meduna, scripted soldiers).
- Hourly lag of the record (D2): crossing a threshold takes effect within an hour.
- Lua override of progress affects the thresholds too (intended).
- A weapon lost with its ammo may look odd if the magazines in pockets do drop;
  thresholds for ammo are separate per item.
- Path C with "stays in the hand": a heavily hit enemy no longer drops that item, so the
  explosion "disarm" effect is weaker for high-tier weapons early on.

## 7. Implementation stages

1. Data: `minDropProgress`, schema, policy switch, validation, unit tests for the helper
   (thresholds, attachments, teams, picked-up flag).
2. Paths A, B, C + D10.
3. Paths D, E + D9 (pickup flag).
4. Test mod `item-limits-test`: `LASERSCOPE` `minDropProgress` 80, a cheap gun at 30,
   an attachment on a gun at 50; debug check of the current/highest progress.

## 8. Testing

- Unit: helper with fixed progress values, below/at/above threshold, attachments
  removed, other teams untouched, picked-up objects untouched.
- Game, progress below 80 %: kill an enemy with a laser scope (tactical, auto-resolve,
  explosion), knock the gun out of his hands with a grenade -> no scope; the gun drops
  without the scope; steal the scoped gun -> the thief gets it with the scope.
- Raise progress above 80 % (debug) -> next hour the scope drops.
- Drop own scope, let an enemy pick it up, kill him -> the scope drops (D9).
- `enemy_drop_progress_limits` false -> old behaviour.
