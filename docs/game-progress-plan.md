# Game progress for enemy drops (Game_Progress)

Status: P1–P6 decided as recommended (2026-11-07); stages 1–2 implemented, build + 249 unit
tests pass, game-verified 2026-11-07. Stage 3 (dealer stock) implemented, 250 unit tests pass, game-verified 2026-11-07.
Branch: `Game_Progress` (from ja2master dbcb72f50).
Related: [enemy-drop-progress-plan.md](enemy-drop-progress-plan.md) (`minDropProgress`, `enemy_drop_progress_limits`).

## 1. Goal

A new, simple game progress value that the "drops all items" limit
(`minDropProgress`) is checked against. It depends on two things only:

- **territorial expansion**: first visit (tactical screen) of each town sector
  and SAM site sector, +2% each;
- **completed quests**: +2% per quest completed.

The sum is capped at 100%. The maximum is deliberately above 100%, so that the
player reaches 100% (all items can drop) before the final attack on Meduna
without visiting everything or finishing every quest.

## 2. How progress works today

### 2.1 Calculation — `CurrentPlayerProgressPercentage()` ([Campaign.cc:1189](../src/game/Tactical/Campaign.cc))

Returns 0–100, computed on demand from the **current** situation, from three
parts whose weights come from `game.json` (`progress_weight_kills`,
`progress_weight_control`, `progress_weight_income`; normalised to 100):

| Part | Source | Can go down? |
|---|---|---|
| income | `PredictIncomeFromPlayerMines()` / `CalcMaxPlayerIncomeFromMines()` | yes (mine lost) |
| kills | `gStrategicStatus.usPlayerKills / kills_per_point_<difficulty>` (7/10/15), capped at its portion | no |
| control | `CalcImportantSectorControl()`: player-controlled sectors where militia training is allowed, SAM sectors count double; capped at its portion | yes (sector lost) |

At the end a Lua hook `OnCalcPlayerProgress` may override the value
(no mod in `assets` or `E:\Release1\mods` uses it).

### 2.2 Record — `HighestPlayerProgressPercentage()` ([Campaign.cc:1278](../src/game/Tactical/Campaign.cc))

Returns `gStrategicStatus.ubHighestProgress` (saved in the savegame).
It is raised only in `HourlyProgressUpdate()` ([Campaign.cc:1288](../src/game/Tactical/Campaign.cc)),
called from [Hourly_Update.cc:57](../src/game/Strategic/Hourly_Update.cc) — so a new record
appears **up to one game hour late**. When a threshold is crossed for the first
time it also fires story events: Madlab scientist meanwhile
(`progress_event_madlab_min`), Mike available to the army
(`progress_event_mike_min`), Iggy placed in C5 (`progress_event_iggy_min`).

### 2.3 Who uses it today

Current value (`CurrentPlayerProgressPercentage`):

| Place | Use |
|---|---|
| [Strategic_AI.cc:2508](../src/game/Strategic/Strategic_AI.cc) | queen priority phase (progress / 10) — army garrison strength |
| [Strategic_Status.cc:246](../src/game/Strategic/Strategic_Status.cc) | Enrico emails: progress, minor/major setback (needs a value that can drop) |
| [Quests.cc:861-869](../src/game/Strategic/Quests.cc) | facts PLAYER_DOING_POORLY (<20) / WELL (>50) / VERY_WELL (>80) — NPC dialogue |
| [Morale.cc:255](../src/game/Tactical/Morale.cc) | merc morale + progress / 5 |
| [Strategic_Movement.cc:3394](../src/game/Strategic/Strategic_Movement.cc) | max bloodcats in ambush |
| [PreBattle_Interface.cc:502,510](../src/game/Strategic/PreBattle_Interface.cc) | ambush chance |
| [MapScreen.cc:9140](../src/game/Strategic/MapScreen.cc) | Skyrider "things are going well" quote |
| [GameSettings.cc:223](../src/game/GameSettings.cc) | cheat debug message (current / highest) |

Record (`HighestPlayerProgressPercentage` / `ubHighestProgress`):

| Place | Use |
|---|---|
| [Items.cc:4105](../src/game/Tactical/Items.cc) `EnemyDropBlockedByProgress` | **drop limit** — item not given to enemies as a guaranteed drop (Inventory_Choosing.cc:1823) |
| [Items.cc:4133](../src/game/Tactical/Items.cc) `ApplyEnemyDropProgressLimit` | **drop limit** — every drop path (corpse, unloaded sector, hand drop on hit/explosion, LOS/Weapons fall-off, thrown knives) |
| [Strategic_AI.cc:2808,3300](../src/game/Strategic/Strategic_AI.cc) | attack group size, reinforcement priority |
| [Strategic_Status.cc:234,238](../src/game/Strategic/Strategic_Status.cc) | Enrico lack-of-progress tolerance |
| [ArmsDealerInvInit.cc:145](../src/game/Tactical/ArmsDealerInvInit.cc) | dealer stock coolness |
| [Inventory_Choosing.cc:816,823](../src/game/Tactical/Inventory_Choosing.cc) | condition of enemy/militia equipment |
| [Soldier_Create.cc:938,1192](../src/game/Tactical/Soldier_Create.cc) | Night Ops trait chance, enemy difficulty modifier (equipment rating) |

**Important:** the drop limit uses the old value in exactly two functions in
`Items.cc`. Every drop path goes through them, so switching the drop system to
a new progress value is a change in one place.

## 3. Can the new system be built from existing data? — Yes

### 3.1 Visited sectors

- `SF_ALREADY_VISITED` (sector flag in `SectorInfo[].uiFlags`) **cannot be
  used as it is**. Besides entering tactical ([StrategicMap.cc:819](../src/game/Strategic/StrategicMap.cc),
  `EnterSector`) it is also set by:
  - a group arriving in a sector on the strategic map without entering tactical
    ([Strategic_Movement.cc:1342](../src/game/Strategic/Strategic_Movement.cc));
  - the start sector at campaign start (`reveal_start_sector`, [Campaign_Init.cc:119](../src/game/Strategic/Campaign_Init.cc));
  - the "reveal whole map" testing code ([StrategicMap_Secrets.cc:116](../src/game/Strategic/StrategicMap_Secrets.cc)).
- **Solution:** a new sector flag `SF_PROGRESS_VISITED = 0x00001000` (free bit),
  set only in `EnterSector` next to `SF_ALREADY_VISITED` (not in meanwhile).
  `SectorInfo[].uiFlags` is already saved/loaded with the game
  ([StrategicMap.cc:2343/2366](../src/game/Strategic/StrategicMap.cc)) — **no save format change**.
  The flag is never cleared, so "only the first visit counts" holds by itself.

### 3.2 Counted sectors

Taken from data, not hard-coded:

- town sectors: `strategic-map-towns.json` (12 towns, 36 sectors, via `GCM` towns);
- SAM sites: `strategic-map-sam-sites.json` (D2, D15, I8, N4).

**N4 is both a Meduna town sector and a SAM site**, so there are **39 different
sectors**, not 40 → decision P1.

### 3.3 Completed quests

- `gubQuest[MAX_QUESTS]` (saved with the game) already holds `QUESTDONE` for
  each finished quest — the same state that turns the red History entry normal.
- History has **23 quests** (IDs 0–22, texts in `BinaryData/quests.edt`).
  IDs 23, 24, 26–29 are empty; 25 (Kill Deidranna) has no text and is never started.
- IDs 8 (Held in Alma) and 9 (Interrogation) are the capture scenario, not
  normal quests; 8 can also be set DONE silently ([Queen_Command.cc:1255](../src/game/Strategic/Queen_Command.cc)).
- Your count is 22 → decision P2.

### 3.4 Calculation

Computed on demand, nothing cached:

```
visited = number of counted sector entries with SF_PROGRESS_VISITED (P1, 40 max)
done    = number of counted quests with their "progress given" bit   (P2, 21 max)
progress = min(100, visited * 2 + done * 2)
```

**Permanent progress (user rule, 2026-11-07):** progress once gained never goes
down, like the original record. And **a completed quest gives progress only
once**: if it is set back (`InternalStartQuest` sets even a done quest to
"in progress"; the quest debug screen can set any state) and completed again,
it gives nothing more.

- Visits: `SF_PROGRESS_VISITED` is never cleared → can only grow.
- Quests: `gStrategicStatus.ubGameProgressQuestsDone[4]` — one bit per quest that
  has already given its progress. Set in `InternalEndQuest` (every path that ends
  a quest) and, as a safety net, for every quest found `QUESTDONE` when progress
  is computed. Bits are never cleared. The 4 bytes come from `bPadding`
  (70 → 66), so `sizeof(STRATEGIC_STATUS)` stays 192 — no save format change.

A single "highest value" record was rejected: after a quest was set back, a new
sector visit would only bring the sum back to the record and be lost.

## 4. Decisions (all taken as recommended, 2026-11-07)

**P1 — N4 (Meduna town + SAM).**
- A: 39 unique sectors × 2% = 78%.
- **B (recommended):** count the two lists separately — N4 visit gives 2% as a
  town sector and 2% as a SAM = 40 entries, 80%, as in your calculation.
- C: add one more sector of your choice to get 40 unique.

**P2 — which quests count.**
- A: all 23 from History (46%).
- **B (recommended):** 21, without 8 and 9 (being captured is not an
  achievement; 42%).
- C: you name the one quest to exclude (22, 44%).

**P3 — scope of the new progress.**
- **A (recommended):** new independent function `GameProgressPercentage()`,
  used only by the drop limit (`EnemyDropBlockedByProgress`,
  `ApplyEnemyDropProgressLimit`). The old progress stays for AI, Enrico,
  morale, NPC facts, story events, dealers, enemy equipment.
- B: replace the old progress everywhere (`CurrentPlayerProgressPercentage`
  returns the new value). Consequences: Enrico setback emails never fire
  (progress can't drop), army strength / queen phase / Madlab (35%) / Mike
  (50%) / Iggy (70%) / NPC "doing well" lines change timing, morale bonus changes.
- C: A + also item-related uses: dealer stock coolness (ArmsDealerInvInit) and
  enemy equipment condition (Inventory_Choosing) follow the new progress.

**P4 — watched militia battle (spectator mode, `gfWatchingMilitiaBattle`).**
It loads the tactical map without mercs.
- **A (recommended):** does not count.
- B: counts like a normal visit.

**P5 — configuration in `game.json`.**
- **A (recommended):** `game_progress_sector_percent: 2`,
  `game_progress_quest_percent: 2` (cap stays 100); quest and sector lists from
  code/data as above.
- B: hard-coded constants.

**P6 — showing the value to the player.**
- **A (recommended):** only the existing cheat debug message
  (GameSettings.cc:223) gets the new value added.
- B: also a visible line, e.g. in the strategic map or laptop (separate stage).

## 5. Changes (assuming recommended P1–P6)

| File | Change |
|---|---|
| `src/game/Strategic/Campaign_Types.h` | `SF_PROGRESS_VISITED = 0x00001000` |
| `src/game/Strategic/StrategicMap.cc` (`EnterSector`, ~819) | set `SF_PROGRESS_VISITED` when not in meanwhile and not `gfWatchingMilitiaBattle` (P4) |
| `src/game/Tactical/Campaign.cc/.h` | `UINT8 GameProgressPercentage()` (+ helpers `GameProgressVisitedEntries()`, `GameProgressDoneQuests()`), returns 0 in edit mode |
| `src/game/Strategic/Strategic_Status.h` | `ubGameProgressQuestsDone[4]` from `bPadding` (70 → 66) |
| `src/game/Strategic/Quests.cc` (`InternalEndQuest`) | `GameProgressNoteQuestDone(ubQuest)` |
| `src/game/Tactical/Campaign.cc` | `IsGameProgressQuest()` (0–22 without 8, 9), `GameProgressNoteQuestDone()` |
| `src/game/Tactical/Items.cc` (4105, 4133) | `HighestPlayerProgressPercentage()` → `GameProgressPercentage()` |
| `src/externalized/policy/GamePolicy.h`, `DefaultGamePolicy.cc`, `assets/externalized/game.json` | `game_progress_sector_percent`, `game_progress_quest_percent` (P5) |
| `src/game/GameSettings.cc:223` | line `Drop progress: N% (city sectors X, quests Y)` under V, always shown (changed from cheat-only on user request, 2026-11-07; not translated; city sectors = 40 town + SAM entries) |
| `src/game/Tactical/Items_unittest.cc` (DropProgressTest) | `SetGameProgress(p)` sets real visit flags instead of `ubHighestProgress` (49 → 48: only even values) |
| new unit test `gameProgressFromVisitsAndQuests` | A1/SF_ALREADY_VISITED not counted, N4 double, quests 8/9 excluded, set-back quest keeps progress, re-completion gives nothing, new visit still adds, 40 + 21 → cap 100 |

Save: no format change (flag in already-saved `uiFlags`, quest bits in the
already-saved padding of `STRATEGIC_STATUS`). Old saves simply start with 0 visited sectors — per the project
rule, a new game is required anyway.

Not changed (P3 = A): every other progress consumer in 2.3, `HourlyProgressUpdate`,
`ubHighestProgress`, `OnCalcPlayerProgress`.

## 6. Stages

Game test (stages 1–2): new game, cheat mode, read the "Drop progress" line of the
game-options message (V on the map screen; cheat mode must be on) after entering A9, A10, a wilderness sector (no change),
a watched militia battle (no change), and after finishing "Deliver Letter".

1. **Visit flag + calculation:** (implemented) `SF_PROGRESS_VISITED`, `GameProgressPercentage()`,
   policy keys, debug line, unit tests. Game test: enter towns/SAMs, read the
   debug value.
2. **Drop integration:** (implemented) switch `Items.cc` to the new value, update
   DropProgressTest. Game test with the test mod (LASERSCOPE 80, SILENCER 50,
   GLOCK_17 30).
3. **Dealer stock (user request 2026-11-07):** see section 7. Enemy equipment
   still on the old progress.
4. Plan status + docs.

## 7. Stage 3 — dealer stock follows minDropProgress

User request (2026-11-07): only *whether an item appears* at **Tony, Devin and
Bobby Ray** (new and used) follows `minDropProgress`; everything else stays,
including the coolness windows and their shifts (Tony +1, Devin at least 3–4).

Decisions:
- **S1 = replace the cut-off:** an item is stockable only when
  `minDropProgress <= GameProgressPercentage()`. The old "coolness above the
  window → never" cut-off is gone for these dealers: such an item (already
  allowed by `minDropProgress`) gets suitability HIGH. Inside / below the window
  the old HIGH / MEDIUM / LOW frequencies are unchanged (still from the old
  progress record).
- **S2 = staple items limited too:** common magazines, first aid / medical /
  tool / locksmith kits, canteen, crowbar and jars appear only from their
  `minDropProgress` on (with the placeholder data coolness × 10 they may be
  missing early on).
- Unchanged: coolness 0 → never sold; `ITEM_NOT_BUYABLE`, illegal guns; all other
  dealers; sell-off chances; prices.

How it works: `GetCurrentSuitabilityForItem()` ([ArmsDealerInvInit.cc](../src/game/Tactical/ArmsDealerInvInit.cc))
returns NONE while `DealerStockBlockedByProgress()` is true, before the staple
list. NONE gives a 0% restock chance, so the item is never marked
`fPreviouslyEligible` and Bobby Ray's site doesn't list it. The first restock
after it unlocks stocks it instantly (existing "first eligible day" rule) — Tony
and Devin restock daily, Bobby Ray on his usual schedule. Progress never goes
down, so an unlocked item never locks again.

Switch: `game.json` `"dealer_stock_progress_limits": true` (false = original
behaviour). Test: `DropProgressTest.dealerStockFollowsMinDropProgress`.

**Lead for Tony and Devin (user request 2026-11-07):** Tony and Devin stock
items whose `minDropProgress` is up to **progress + 10** (progress 50 → up to
60). Bobby Ray stays at the progress itself. `game.json`
`"dealer_stock_progress_lead": 10`. The coolness windows (their shifts Tony +1,
Devin at least 3–4) still only set the frequency and still use `ubCoolness` with
the old progress record.

## 8. Progress messages in the message history

User request (2026-11-07): each time the game progress reaches a full step of
10, 20, …, 100% a message `You have reached N% game progress.` goes to the
message history (`ScreenMsg`, `MSG_INTERFACE`: tactical scroll + the strategic
map message log). Not translated, like the V line.

- `GameProgressNewMilestones()` returns the full 10% steps reached since the
  last call; `gStrategicStatus.ubGameProgressAnnounced` (from `bPadding`,
  66 → 65, size stays 192, no save format change) remembers the highest step
  announced, so each step is announced **once**, also after loading a save.
  A jump over several steps announces all of them in order.
- `HandleGameProgressMilestones()` is called where progress can grow: after
  `SF_PROGRESS_VISITED` is set in `EnterSector`, after `GameProgressNoteQuestDone`
  in `InternalEndQuest`, and in `HourlyProgressUpdate` as a safety net (e.g. a
  quest set to done from the quest debug screen).
- Test: `DropProgressTest.gameProgressMilestonesOnce`.

## 9. All dealers follow minDropProgress, no guaranteed deliveries

User request (2026-11-07): the stock of **every** dealer depends on
`minDropProgress`; guaranteed deliveries are switched off; everything else of the
dealers stays as it was.

- `DealerStockBlockedByProgress()` now covers all dealers (switch
  `dealer_stock_progress_limits`). Lead: Tony and Devin +10
  (`dealer_stock_progress_lead`), all others 0, like Bobby Ray. Staple items are
  limited for every dealer. Other dealers keep their constant frequency (HIGH,
  staples ALWAYS once allowed); coolness 0 still means no normal stock.
- `game.json` `"dealer_guaranteed_deliveries": false` switches off, in
  `AdjustCertainDealersInventory()` and `SetFactTrue()`: Tony's guaranteed big gun,
  the bars' minimum alcohol, Sam's jar, Franz's video camera, Jake's gas cans.
  Tony's maximum limits (2 big guns, 3 handguns, 8 ammo) are not guarantees and stay.
  The Lua function `GuaranteeAtLeastXItemsOfIndex` for mods stays.
- **Jake's gas cans:** `"GAS_CAN": 6` added to `dealer-inventory-jake.json`; for a
  dealer with `SELLS_FUEL` they are normal stock (coolness 0 does not block them),
  only once `FACT_ESTONI_REFUELLING_POSSIBLE` is set (Shank brought to Estoni) and
  their `minDropProgress` is reached; suitability HIGH like his other items. Gas cans
  are liquids, so always sold full. First stock comes with the next daily restock
  after the quest (the "first eligible day" rule stocks it instantly), not at the
  moment of the quest any more.
- Consequences to tune later (user, 2026-11-07): Franz's **video camera** (Madlab's
  robot quest) appears only from its `minDropProgress` (placeholder 60) and by chance;
  bar alcohol, Sam's / Jake's jars from 10 (placeholder data).
- Tests: `dealerStockFollowsMinDropProgress` (Keith/Franz limited, no lead),
  `jakeGasCansAfterTheShankQuest`.
