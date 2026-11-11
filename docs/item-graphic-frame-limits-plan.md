# Item graphic frame limits

Status: decisions D1–D4 decided 2026-11-11: all as recommended (A).
Implemented on the Weapons branch:

| Stage | Commit | Status |
|---|---|---|
| 1 - ground tile counts | 0f472f403 | game-verified |
| 2 - inventory picture index | 31fac9f0e | game-verified |
| 3 - range check and game tests | this commit | game-verified |

Item graphic frame limits COMPLETE (2026-11-11).

Note for data: `tileGraphic.subIndex` counts pictures from 1,
`inventoryGraphics.*.subImageIndex` from 0. The same picture number in
`smguns.sti` and `mdguns.sti` is therefore subIndex = subImageIndex + 1
(original guns: Glock 17 subImageIndex 1, subIndex 2).

Raise the number of frames the game can use from the item graphic files, so
new items can get their own pictures:

- `Interface/MDGUNS.STI` (and the other small inventory graphics): the item
  picture in the inventory;
- `Tilesets/0/SMGUNS.STI` (and `SMP1ITEMS`, `SMP2ITEMS`, `SMP3ITEMS`): the
  item lying on the ground.

The limits come from the code, not from the file format: a STI file and a
PNG file can both hold up to 65,535 frames (`src/sgp/PNG.cc`), so exporting
to PNG does not change them.

## 1. Current state (code facts)

### 1.1 Inventory pictures (`MDGUNS.STI` and co.)

- An item refers to its picture by `inventoryGraphics.small.subImageIndex`
  (and `.big`).
- `GraphicModel::subImageIndex` is `uint8_t` (`src/externalized/GraphicModel.h`),
  and so is the schema (`rust/stracciatella/src/schemas/yaml/types/graphic.schema.yaml`,
  `types/uint8.schema.yaml`). **Limit: frames 0–255.**
- `GetSmallInventoryGraphicForItem` / `GetBigInventoryGraphicForItem`
  (`src/game/Tactical/Interface_Items.cc`) return the index as `UINT8`. An index
  beyond the file's frames already logs an error and uses a fallback picture.
- The user's `MDGUNS.STI` has 102 frames: all usable today.

### 1.2 Ground pictures (`SMGUNS.STI` and co.)

- An item refers to its ground picture by `tileGraphic` = tile type + `subIndex`
  (`uint16` in the schema `types/tileset-tile-index.schema.yaml`).
- Each tile type has a fixed number of tiles in the tile enum
  (`src/game/TileEngine/TileDat.h`) and in `gNumTilesPerType`
  (`TileDat.cc`):

  | Tile type | File | Tiles | Original file |
  |---|---|---|---|
  | `GUNS` | `smguns.sti` | 60 (`GUN1`…`GUN60`) | 50 frames |
  | `P1ITEMS` | `smp1items.sti` | 149 | 138 frames |
  | `P2ITEMS` | `smp2items.sti` | 45 | 43 frames |
  | `P3ITEMS` | `smp3items.sti` | 16 | 12 frames |

- `CreateTileDatabase` (`TileDef.cc`) takes only that many frames from the
  file; frames beyond it are ignored. A file with fewer frames is padded with
  filler tiles (frame 0), so the tile database index of every tile always
  equals its enum value.
- The item's tile is computed at run time: `GetTileGraphicForItem` =
  `GetTileIndexFromTypeSubIndex(type, subIndex)` = start of the type +
  subIndex − 1. There is **no range check**: a `subIndex` above the type's
  count shows a tile of the **next** type (for `GUNS` 61+: a `P1ITEMS` tile).
- The user's `SMGUNS.STI` has 102 frames: frames 61–102 are never loaded.

### 1.3 Is it safe to add tiles to a type?

- Adding tiles to `GUNS` moves the database index of every later tile (all
  types after `GUNS`). That is safe only if nothing stores raw tile indices.
- Maps store tiles as **type + subindex** (`WorldDef.cc`, `LoadWorld`:
  `ubType`, `ubSubIndex` → `GetTileIndexFromTypeSubIndex`).
- Map changes in the temp files store **type + subindex**
  (`SaveLoadMap.h`: `usImageType`, `usSubImageIndex`).
- Items on the ground are stored as items; their tiles are rebuilt from the
  item data when the sector loads.
- Code that names single item tiles: only the counts in `gNumTilesPerType`
  (`GUN60 - GUN1 + 1` …); no `GUNxx` / `P1ITEMxx` is used anywhere else.
- The tile database has 3,125 tiles; the index is `UINT16` (65,535), so there
  is plenty of room.
- The overhead map's small tile database (`Overhead_Map.cc`) uses the same
  counts. Item types have no small `t/` version, the default file stands in;
  nothing changes there.
- Saved games: a new game is used anyway after such a change (project rule:
  no save compatibility). Stage 3 checks a new game.

## 2. Decisions

- **D1 – New number of ground tiles for `GUNS`.** (A, recommended) 2000, the
  whole weapon block of item indices (1000–2999), so every possible weapon can
  have its own ground picture. (B) 1000. (C) 500.
- **D2 – The other item tile types.** (A, recommended) raise them too, to the
  size of their item blocks: `P1ITEMS` 2000, `P2ITEMS` 1000, `P3ITEMS` 1000.
  (B) only `GUNS` for now.
- **D3 – Inventory picture index.** (A, recommended) `subImageIndex` becomes
  `uint16` (0–65,535) for the small and the big inventory pictures, in the
  code and in the schema. (B) keep 0–255.
- **D4 – A ground picture beyond the file's frames.** (A, recommended) at
  load, an item whose `tileGraphic.subIndex` is above the frames of its file
  logs a warning and shows frame 1 of its type, like the inventory pictures
  already do. (B) keep the current behaviour (a tile of the next type).

## 3. Implementation (after D1–D4)

### Stage 1 – Ground tile counts

- `TileDat.h`: replace the listed `GUN1`…`GUN60` entries by a range
  (`GUN1`, `GUN_LAST = GUN1 + GUN_TILES - 1`), the same for `P1ITEM`,
  `P2ITEM`, `P3ITEM` (D2). The counts become constants (`GUN_TILES` …).
- `TileDat.cc`: `gNumTilesPerType` from those constants.
- Unit test: the counts and the start index of the type after `P3ITEMS`.

### Stage 2 – Inventory picture index (D3)

- `GraphicModel`: `uint16_t subImageIndex`, getter and constructor.
- Schema `graphic.schema.yaml`: `uint16`.
- `GetSmallInventoryGraphicForItem`, `GetBigInventoryGraphicForItem` and
  their callers: `UINT16` index.
- Unit test: an item with `subImageIndex` 300 is read and kept.

### Stage 3 – Range check and game tests (D4)

- After the tile database is built, check every item's `tileGraphic` against
  the frames of its file; log a warning and use frame 1 for one beyond it.
  Done in `GetTileGraphicForItem` (`Interface_Items.cc`), which every ground
  picture goes through: `ItemGroundSubIndex` keeps a subIndex from 1 to the
  frames the tile database took from the type's file (at most the type's
  count), else 1; the warning is logged once per item.
- Game tests: a new game. A weapon with ground picture `GUNS` 61+ and
  inventory picture 101 in the user's 102 frame files, dropped on the ground,
  shown in the inventory, in the sector inventory, and in the editor. Other
  items on the ground and map objects (later tile types: windows, pointers,
  footprints, wireframes) look as before.
