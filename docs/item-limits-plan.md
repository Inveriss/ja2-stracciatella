# Item limits plan (branch `Item_Limits`)

Status: **implemented, stages 1-5 game-verified (2026-11-06).** Written 2026-11-06
from an analysis of `ja2master` 029b77395; line references below are from that commit.

## Implementation status

| Stage | Commit | Notes |
|-------|--------|-------|
| 1 - constants and loader | a027d4b37 | `MAXITEMS` 9000, `ItemIndexBlocks`, `registerItem()`, placeholders `__ITEM_<n>`; save 112 |
| (side fix) | c60d309d7, aaf04f75f | Manuel's face data; face eyes/mouth kept inside the face |
| 2 - sparse shop save (D9) | da592b1b6 | dealers and Bobby Ray store only non-empty entries; save 113 |
| 3 - Bobby Ray | 937e6ba73 | `BOBBY_R_USED_ORDER_FLAG` instead of +1000; texts from the data (D7); save 114 |
| (side change) | 66e15c8ea | Bobby Ray sells no used items: the hidden used stock is gone; save 115 |
| 4 - small fixes | 4992b39c9 | Alt+W weapon cycling, item description cycling, Quest Debug list skip placeholders |
| 5 - test data | (not in the repo) | mods in the game's `mods` folder, see below |

Stage 5 test data are three development mods with JSON patches (`<file>.patch.json`
next to the data files, applied by the VFS) in `E:\Release1\mods\`:

- `item-limits-test`: one item in every block, copied from an original item
  (1000 / 2000 pistols, 3000 9mm magazine, 4000 grenade, 5000 vest, 6000 first aid kit,
  7000 attachment, 8000 key), sold by Bobby Ray and Tony. The 7000 attachment cannot be
  attached to guns yet (see [weapon-attachments-plan.md](weapon-attachments-plan.md)).
- `item-limits-test-wrong-block`: an armour at 1500; the game must stop at start.
- `item-limits-test-duplicate`: a second item at 201; the game must stop at start.

### Adding real items

- Add them to the data files in the repo (`assets/externalized/*.json`); the copy in the
  game's `externalized` folder is overwritten by every build.
- Use an index from the block of the file, a unique `internalName`, and give `name`,
  `shortName` and `description` in the JSON (the original text files only cover 0-350).
- Unused indices need no entries: the loader fills them with placeholders.
- Indices of the test mod collide with real items at the same numbers: disable the mod or
  start real items one higher (e.g. 1001, 3001).

## Goal

Allow many more items in every category than today. The item index space grows from
351 (`MAXITEMS`) to **9000**, split into fixed blocks per category (2000 for weapons,
1000 for every other category), as in the original 1999 layout with `FIRST_WEAPON`,
`FIRST_AMMO`, and so on. The save game stores shop stock sparsely (D9), so its size
depends on the items actually stocked, not on the ceiling, and a later change of the
ceiling does not break saves.

## Current state (analysis)

### Vanilla layout is preserved

`src/game/Tactical/Item_Types.h` keeps every vanilla item on its 1999 index:

| Range   | Category (vanilla)                | Placeholders today          |
|---------|-----------------------------------|-----------------------------|
| 1-65    | weapons (`MAX_WEAPONS` = 70)      | 66-70 `unused`              |
| 71-115  | ammo                              | 116-130 `CLIP_NOTHING`      |
| 131-160 | explosives                        | -                           |
| 161-198 | armour                            | 199-200 `NOTHING`           |
| 201-269 | kits / misc                       | 270 `NOTHING`               |
| 271-302 | keys                              | -                           |
| 303-350 | additional items                  | 328-350 `UNUSED_*` (some used) |

Differences from 1999:

- `FIRST_WEAPON` ... `FIRST_KEY` and `FIRST_HELMET` ... `LAST_HEAD_ITEM` no longer exist.
  The category of an item comes from the JSON file it is defined in (`weapons.json`,
  `magazines.json`, `explosives.json`, `armours.json`, `items.json`) and from
  `usItemClass` / `armourClass`, not from the index.
- Only remnant of index ranges: `loadItems()` rejects `items.json` entries with
  index <= `MAX_WEAPONS` (70).
- Enum naming slip: `__ITEM_302` actually has index 303 (harmless).

### Hard limits found

1. `MAXITEMS` is the last value of `enum ITEMDEFINE` (351).
2. `DefaultContentManager::loadGameData()` does `m_items.resize(MAXITEMS)`. Every index
   must be defined in JSON: a missing one leaves `nullptr`, which crashes the
   `m_itemMap` loop right after loading (`item->getInternalName()`), and later every
   `GCM->getItems()` loop.
3. Loader range checks are inconsistent:
   - `loadWeapons()` / `loadArmours()`: `index >= m_items.size()` -> error (OK).
   - `loadItems()`: `index > MAXITEMS` -> off by one, 351 is accepted and written past
     the vector.
   - `loadMagazines()` / `loadExplosives()`: no check at all -> out-of-bounds write.
   - No duplicate-index detection (a later file silently overwrites an earlier item).
4. Save game, laptop (`Laptop.cc` `SaveLaptopInfoToSavedGame` / `LoadLaptopInfoFromSavedGame`):
   fixed `BYTE data[7445]`, which contains `BobbyRayInventory[MAXITEMS]` and
   `BobbyRayUsedInventory[MAXITEMS]` at 8 bytes each (2 x 351 x 8 = 5616 bytes,
   1829 bytes of other fields).
5. Save game, dealers (`Arms_Dealer_Init.cc`): `gArmsDealersInventory[NUM_ARMS_DEALERS][MAXITEMS]`,
   16 bytes per entry. Scales automatically with `MAXITEMS`.
6. Bobby Ray used items (`BobbyR.cc`): a used-item reorder is encoded as
   `item + BOBBY_R_USED_PURCHASE_OFFSET` (1000), cast to `INT16`, and decoded in
   `AddFreshBobbyRayInventory()` with `>= 1000`. Collides with any item index >= 1000.
7. Bobby Ray names and descriptions (`BobbyRGuns.cc`, `BobbyRMailOrder.cc`) are read from
   `braydesc.edt` at `400 * itemIndex`. Items past the vanilla file get an empty string
   (the read only logs a warning); there is no JSON fallback.
8. Item names from `itemdesc.edt` (`ItemStrings.cc`, `VANILLA_MAX_ITEMS` 351): already
   fine. Items past the file must give `name` / `shortName` / `description` in JSON.
9. Stack arrays sized by `MAXITEMS` in `Arms_Dealer_Init.cc` (`usAvailableItem`,
   `ubNumberOfAvailableItem`, `ubChanceForAvailableItem`).
10. Minor: `gfBobbyRNotifyEverShown[MAXITEMS]` (BobbyRGuns.cc), Quest Debug item list
    (`gItemListBox.usMaxArrayIndex = MAXITEMS`), debug `CycleSelectedMercsItem()`
    (`> MAX_WEAPONS`), unit test `VanillaWeapons_unittests.cc` (`MAXITEMS - 1`, loop
    from `UINT16_MAX`).

### Not limited by the index (no work needed)

- `OBJECTTYPE::usItem`, attachments, merc inventories, map files and the JSON schema
  (`types/uint16.schema.yaml`) all use 16-bit item numbers (up to 65535).
- Editor item lists, Bobby Ray stock building, enemy/militia gun choice and dealers
  iterate `GCM->getItems()` or use item names.
- `fWeaponDroppedAlready[MAX_WEAPONS]` is legacy only; the real list lives in
  `g_gameStates` (no index limit).
- Inventory graphics are path + `subImageIndex` (max 256 images per file, unlimited files).

### Related limits outside this plan

- Keys: `OBJECTTYPE::ubKeyID` and `ItemModel::ubClassIndex` are 8-bit and the lock table
  stores a key ID; the editor draws keys as `KEY_1 + usKeyItem`. In practice at most 256
  key IDs. The 8000 block reserves the numbers; raising the key-ID limit is a separate task.
- World tile graphics of items come from the tileset types GUNS / P1ITEMS / P2ITEMS /
  P3ITEMS, which have a fixed number of tiles. New items can reuse existing tiles; new
  tiles are a graphics task.
- Weapon attachment permissions are hand-written per attachment in `WeaponModels.cc`
  (`attachment_Xxx` flags). Many new attachments will need a data-driven mechanism; a
  separate task.

## Decisions

| ID | Decision | Status |
|----|----------|--------|
All decisions were taken by the user on 2026-11-06 (D3-D7 as recommended).

| ID | Decision |
|----|----------|
| D1 | Block layout (below); weapons get two blocks (1000-2999), the rest one block each |
| D2 | `MAXITEMS` = 9000 |
| D3 | Loader rejects an item outside its block with a clear error naming file, item and allowed range |
| D4 | 351-999 is a reserve: rejected by the loader until assigned to something |
| D5 | 7000-7999 (weapon attachments) are `items.json` entries that must have the `ITEM_ATTACHMENT` flag; 6000-6999 takes the other `items.json` items except keys; 8000-8999 takes `IC_KEY` items |
| D6 | Unused indices are filled automatically with a placeholder item (`IC_NONE`, `ITEM_NOT_BUYABLE`, `ITEM_NOT_EDITOR`, internal name `__ITEM_<n>`); the vanilla `NOTHING` / `UNUSED_*` JSON entries may stay |
| D7 | Bobby Ray name / description: JSON `name` / `description` first, `braydesc.edt` only for vanilla indices < 351 |
| D8 | Save format changes -> `SAVE_GAME_VERSION` bump, new game required, no migration (project rule) |
| D9 | Sparse shop save: dealers and Bobby Ray (new + used) write only non-empty entries, each with its item index and a count prefix; size follows the stocked items, and a later `MAXITEMS` change keeps saves loadable (see stage 2) |

### Block layout (D1, D2)

| Range     | Category                         | Defined in          |
|-----------|----------------------------------|---------------------|
| 0-350     | vanilla items, unchanged         | all files (as today) |
| 351-999   | reserve                          | -                   |
| 1000-2999 | weapons                          | `weapons.json`      |
| 3000-3999 | ammo (magazines)                 | `magazines.json`    |
| 4000-4999 | explosives                       | `explosives.json`   |
| 5000-5999 | armour                           | `armours.json`      |
| 6000-6999 | misc (kits, misc, quest items)   | `items.json`        |
| 7000-7999 | weapon attachments               | `items.json` (`ITEM_ATTACHMENT`) |
| 8000-8999 | keys                             | `items.json` (`IC_KEY`) |

`MAXITEMS` = 9000. Vanilla indices 0-350 keep their current rules (any category, as
today), so vanilla maps and the existing placeholders keep working.

### Rules for later changes

- **An item number, once given, never changes.** Item numbers are stored in saves
  (inventories, sector items, shops) and in maps saved by the editor; renumbering an
  existing item breaks both.
- Block borders are only loader rules (not stored anywhere), so they may be moved later
  as long as no existing item ends up outside its block.
- With D9, raising `MAXITEMS` later keeps saves loadable. Lowering it is only safe when
  no saved item has an index >= the new value; the loader of the save must detect that
  and report an error instead of reading out of range.

## Implementation stages

### Stage 1 - constants and loader

- `Item_Types.h`: end the enum at the last vanilla item; add
  `constexpr UINT16 MAXITEMS = 9000;` (or keep it as a separate `enum` value) plus
  block constants, e.g. `FIRST_NEW_WEAPON = 1000`, `FIRST_NEW_AMMO = 3000` ...
  `FIRST_NEW_KEY = 8000`,
  `VANILLA_MAXITEMS = 351`, and a small helper
  `ItemBlockOf(index)` / `IsIndexAllowed(index, category)`.
- `DefaultContentManager`:
  - one shared `registerItem(ItemModel*, category, fileName)` used by all five loaders:
    range check (`< MAXITEMS`), block check (D3/D4/D5), duplicate check;
  - fix the missing checks in `loadMagazines()` / `loadExplosives()` and the
    off-by-one in `loadItems()`;
  - after loading, fill every `nullptr` slot with a placeholder (D6) before the
    `m_itemMap` loop;
  - placeholders must not be inserted into `m_itemMap` under a shared name
    (use `__ITEM_<n>` so names stay unique).
- Unit tests: block checks, duplicate detection, placeholder fill, `getItem(MAXITEMS - 1)`.

### Stage 2 - per-item arrays and the save game

- Sparse shop save (D9):
  - `Laptop.cc`: the fixed 7445-byte block keeps only the 1829 bytes of other fields.
    `BobbyRayInventory` / `BobbyRayUsedInventory` are written after it as
    `UINT16 count` + `count` records of 8 bytes (the record already holds
    `usItemIndex`). Only the first `usInventoryListLength[...]` entries are in use
    (the stock list is compacted and sorted by `SetupStoreInventory()`), so exactly
    those are written; the rest is reset on load.
  - `Arms_Dealer_Init.cc`: per dealer, `UINT16 count` + `count` records of
    `UINT16 itemIndex` + the existing 16 bytes, written only for entries that are not
    empty (`ubTotalItems`, `ubQtyOnOrder`, `uiOrderArrivalTime`, `fPreviouslyEligible`
    or special items set). Special items keep following in the same item order.
  - Load: reset all tables first, then fill from the records. A record with
    `itemIndex >= MAXITEMS` or a placeholder item is a load error (clear message).
- `Arms_Dealer_Init.cc`: stack arrays -> `std::vector` (keeps the stack safe); daily
  loops over `MAXITEMS` stay (19 dealers x 9000).
- In-memory tables stay full-size arrays (fast lookup by item index): dealers about
  19 x 9000 x ~40 bytes = ~7 MB RAM, Bobby Ray ~150 KB. Acceptable.
- Save size then follows the stock: with today's data it is smaller than now.
- `SAVE_GAME_VERSION` bump (D8).

### Stage 3 - Bobby Ray

- Replace the `+ 1000` used-item encoding: pass a separate used flag (e.g. a high bit
  of the 32-bit strategic event parameter, `0x10000`) and change
  `AddFreshBobbyRayInventory()` to take `(UINT16 item, bool used)`.
- Name / description fallback (D7) in `BobbyRGuns.cc` and `BobbyRMailOrder.cc`.
- `gfBobbyRNotifyEverShown` scales with `MAXITEMS` (no change needed beyond the constant).

### Stage 4 - small fixes

- Debug `CycleSelectedMercsItem()`: cycle through real weapons (`isWeapon()`) instead of
  `> MAX_WEAPONS`.
- Quest Debug item list: skip placeholders.
- `VanillaWeapons_unittests.cc`: loops adjusted to the new `MAXITEMS`.
- Check that the editor item lists and the Bobby Ray / dealer stock builders skip
  placeholders (they should, through `ITEM_NOT_EDITOR` / `ITEM_NOT_BUYABLE`).

### Stage 5 - test data and game verification

- Temporary test items (one per block: 1000, 2000 (second weapon block), 3000, 4000,
  5000, 6000, 7000, 8000) in the user's game data, not in the repo assets.
- Checks in the game: start a new game; items appear in the editor lists; buy at Bobby
  Ray (new and used) and receive the delivery; dealers buy and sell them; save and load;
  a weapon at 1000+ / 2000+ fires with ammo at 3000+; an attachment at 7000+ attaches.
- Save checks (D9): save size with empty and with filled shops; a save made with
  `MAXITEMS` 9000 loads after a test build with a higher ceiling.
- Error checks: an item placed in the wrong block or with a duplicate index gives a
  clear startup error.

## Related plan on this branch

- [magazine-size-variants-plan.md](magazine-size-variants-plan.md): magazines of
  different sizes are interchangeable only when their item indices are consecutive
  (`ReloadGun()` guesses `±1`); replaced by a lookup by calibre, size and ammo type.
- [weapon-attachments-plan.md](weapon-attachments-plan.md): attachment compatibility by
  family rules (weapon types, exceptions, one level per family) and effects from data, instead of a
  hand-written flag per attachment on every weapon.

## Open questions for later

- Raising the key-ID limit above 256 (would need `ubKeyID` widening and the lock table).
- Data-driven attachment permissions for the 7000 block.
- New world tile graphics for new items.
