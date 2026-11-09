# Item names and descriptions in JSON (strings/item-texts-<language>.json)

Status: plan only (2026-11-09, written in Remote Control mode), T1-T8 decided by the user
(2026-11-09), export language English, not implemented.
Branch: `Weapons` (last commit 3105624d1); the work may also get its own branch from
`ja2master` (workflow rule) - to be decided at the desk.
Related: the earlier migrations of the same kind: `mercs-profile-names-<language>.json`
(96614e5d4), `mercs-dialogue-<language>.json`, `strings/email-text-<language>.json`
(316be3ac9, `tools/export-email-text.ps1`).

## 1. Today

| Text | Source | Size (characters) |
|---|---|---|
| short name, name, description | `BINARYDATA/itemdesc.edt` (inside `Binarydata.slf`), row = `itemIndex`, read once by `BinaryData::deserialize` (`src/externalized/strings/ItemStrings.cc:61`) | 80 / 80 / 240 (`Text.h:13-15`) |
| Bobby Ray catalogue name and description | `BINARYDATA/braydesc.edt`, read on demand by `GetBobbyRItemName` / `GetBobbyRItemDescription` (`src/game/Laptop/BobbyRGuns.cc:1292-1311`), only for `itemIndex < VANILLA_MAXITEMS`; empty -> the item's own name/description | 80 / 320 (`BobbyRGuns.h:10-12`) |
| one item's override | optional `"shortName"`, `"name"`, `"description"` in the item file; `deserializeHelper` (`ItemModel.cc:9`) uses them, else `itemdesc.edt`. Used today only by the test M14 copy in `weapons.json` | no limit |

The EDT files hold the language of the installed game; the user's game is **English**.

## 2. Goal

All item texts of the game language come from **`strings/item-texts-<language>.json`**, keyed by
`internalName`. The English file is exported once from the user's English `itemdesc.edt` and
`braydesc.edt`; from then on the texts are edited in JSON. The EDT files stay as a fallback.

## 3. Decisions (T1-T8, accepted 2026-11-09)

| # | Question | Decision |
|---|---|---|
| T1 | Scope | short name, name, description from `itemdesc.edt` **and** the Bobby Ray texts from `braydesc.edt` |
| T2 | Files | one file per language in `strings/`, entries keyed by `internalName` (as the merc files) |
| T3 | Languages | export English only (`item-texts-eng.json`); other languages keep the EDT until someone adds their file |
| T4 | EDT fallback | kept: the game works without a language file |
| T5 | Order | language file -> field in the item file -> EDT |
| T6 | Length | no hard limit; a load warning when a text is longer than the old EDT size (80 / 80 / 240, Bobby Ray 80 / 320), because the description boxes may cut it |
| T7 | Export tool | a one-time export that uses the game's own EDT decoder (section 6); the generated file goes into the repository |
| T8 | Empty EDT rows (unused slots, `NOTHING`) | skipped |

## 4. File format

`assets/externalized/strings/item-texts-eng.json`, a list in `itemIndex` order (readability only;
the key is `internalName`):

```json
[
    {
        "internalName": "GLOCK_17",
        "shortName": "Glock 17",
        "name": "Glock 17 9mm",
        "description": "…",
        "bobbyRayName": "…",
        "bobbyRayDescription": "…"
    }
]
```

- `internalName` required; every other field optional: a field left out falls back (T5), so an
  entry can override only the description, for example.
- Schema `rust/stracciatella/src/schemas/yaml/item-texts.schema.yaml` (strings, `internalName`
  required, `additionalProperties: false`); path rule in `rust/stracciatella/src/schemas/mod.rs`:
  `strings/item-texts-` -> `item-texts`, placed **before** the generic `strings/` rule.
- Mods change it with the usual JSON patches (by index or by replacing the file).

## 5. Loading (game code)

`src/externalized/DefaultContentManager.cc`:

1. `ItemTextsFile(version)` = `"strings/item-texts" + L10n::GetSuffix(version, true) + ".json"`
   (like `MercProfileNamesFile`, `:1165`).
2. Before the item files are read: when the file exists, load it into
   `std::map<ST::string, ItemTexts> m_itemTexts` (internalName -> five optional strings). Errors:
   an unknown `internalName` (no such item after loading) and a duplicate entry -> load error with
   the name (as other data errors).
3. `ItemModel::InitData` (`ItemModel.h`) gets a pointer to the entry of this item (or null);
   `deserializeHelper` (`ItemModel.cc:9`) order: entry field -> item file field -> `BinaryData`
   (T5). The same for `shortName`, `name`, `description`.
4. Bobby Ray: `ItemModel` gets `getBobbyRayName()` / `getBobbyRayDescription()` (empty when the
   entry has none). `GetBobbyRItemName` / `GetBobbyRItemDescription` (`BobbyRGuns.cc:1292`): entry
   -> `braydesc.edt` (vanilla indices) -> the item's name/description (unchanged rest).
5. T6: after loading, one warning per text longer than its old EDT size, e.g.
   `item-texts-eng.json: GLOCK_17 description has 251 characters (the item box shows about 240)`.
6. Log line (info): `item texts: N items from strings/item-texts-eng.json`.

No save format change, no new game needed (texts are not saved).

## 6. Export (T7)

A developer switch, like the existing `JA2_USE_PROF_DAT`: starting the game with the environment
variable **`JA2_EXPORT_ITEM_TEXTS=<output file>`** writes the file and quits after the content is
loaded (before the menu):

- source: `BinaryData` as the game decodes it (`itemdesc.edt` from `Binarydata.slf`, with the
  game's own decryption and code page handling) and `braydesc.edt` via
  `GCM->loadEncryptedString` (rows of 400: name 80, description 320);
- the language file is **ignored** during the export (pure EDT texts);
- one entry per item that has an `ItemModel` and at least one non-empty text (T8); rows of
  `itemdesc.edt` without an item, and items beyond `VANILLA_MAXITEMS` with no EDT row, are skipped;
- Bobby Ray fields only when the `braydesc.edt` row is not empty; a Bobby Ray text equal to the
  item text is still written (the two files are independent);
- JSON written with 4-space indentation, UTF-8 without BOM, LF, like the other data files;
- report on the console/log: number of entries, how many with Bobby Ray texts, how many skipped.

The variable is kept after the migration (a way to export another language's EDT files later).

Command at the desk (English game data in `E:\JA2_Release1`):

```bash
JA2_EXPORT_ITEM_TEXTS=assets/externalized/strings/item-texts-eng.json _bin/Release/ja2.exe
```

## 7. Tests

`src/externalized/` unit tests (DefaultContentManagerUT, externalized folder):

- an entry overrides only the fields it has (T5): entry with `description` only -> name from the
  item file / EDT stub, description from the entry;
- item-file fields win over the EDT, entry fields win over the item file;
- unknown `internalName` and a duplicate entry -> load error;
- length warning above 80 / 80 / 240 / 80 / 320 (T6);
- `item-texts-eng.json` of the repository: loads without errors, every `internalName` exists,
  `GLOCK_17` / `MORTAR` / `FIRSTAIDKIT` have a name and a description;
- the export writer: a small fake `BinaryData` -> expected JSON (empty rows skipped, T8).

## 8. Steps

1. Code: sections 4-5 (schema, path rule, loading, Bobby Ray, warnings) + tests; build, all unit
   tests.
2. Export: section 6 switch, run it once on the English game data, check the report and a few
   texts by hand (a gun, an armour, a kit, an item with a CP437 character), commit the file
   together with the code.
3. Check that the export is exact: start the game **without** the variable; in the log
   `item texts: N items …`, no length warnings for the exported file (the EDT texts fit their
   sizes by construction).
4. Game check by the user: item description boxes (tactical, map inventory), Bobby Ray guns /
   armour / misc / used pages and the order form show the same texts as before; change one
   description in `item-texts-eng.json` and see it in the game; a mod item without an entry still
   shows its item-file texts.
5. Commit after the user's game check (project rule).
