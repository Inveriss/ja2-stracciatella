# Gun Nut only - plan

Status: plan written 2026-11-16, decisions G1-G5 decided 2026-11-16 (all A, as
recommended). Implemented on the Weapons branch. Gun Nut only COMPLETE (2026-11-16).

| Stage | Commit | State |
|---|---|---|
| 1 - code | 104911ea7 | game-verified |
| 2 - data | this commit | game-verified |

Remove the "Normal" (reduced) gun mode for good: every weapon of `weapons.json` always
takes part in the game, and the data no longer needs `standardReplacement` or
`bBigGunList`.

## 1. Current state (code facts)

- The choice is hidden and Gun Nut is always on since commit d669b0106:
  `GameSettings.cc` (defaults), `GameInitOptionsScreen.cc` (new game, the check boxes
  are in `#if 0`), `Init.cc` (editor) set `gGameOptions.fGunNut = TRUE`. The editor
  (`EditScreen.cc`) also forces it on while it runs.
- So all the Normal mode code below never runs today, but it is still there:
  - `ItemIsLegal` (`Items.cc`): without Gun Nut, guns and magazines with
    `bBigGunList` are not sold (Bobby Ray `BobbyR.cc`, dealers `ArmsDealerInvInit.cc`).
  - `ReplaceExtendedGuns` (`Inventory_Choosing.cc`), called from `Soldier_Create.cc`:
    a big-list gun of a map-placed soldier becomes its `standardReplacement` (civilians)
    or a gun of the same class from `SelectStandardArmyGun`.
  - `SelectStandardArmyGun`: uses `army-gun-choice-normal.json` without Gun Nut, else
    `army-gun-choice-extended.json`.
  - `Soldier_Profile.cc`: big-list guns of the merc profiles are replaced by their
    `standardReplacement`, with the ammo.
  - `World_Items.cc`: big-list guns and magazines lying on the maps are replaced by
    their `standardReplacement`.
  - `Interface_Items.cc`: without Gun Nut the calibre is added to a gun's name.
  - `SaveLoadScreen.cc`: the save's tool tip shows "normal guns / additional guns".
  - `SaveLoadGame.cc`: the option is one byte of the saved game options, and it changes
    the rotation set of the save (`+= BASE_NUMBER_OF_ROTATION_ARRAYS * 6`).
  - `ScriptingExtensionsLua.cc`: Lua scripts can read and **write** `fGunNut`. A script
    that sets it to false would switch the Normal code on, and with the data fields
    removed `getWeaponByName("")` would stop the game.
- Data: `standardReplacement` in 40 entries of `weapons.json` and 12 of
  `magazines.json` (with `bBigGunList`). Both fields are optional for the loader and
  the schemas; the schemas accept unknown keys.
- The enemy gun choice by progress (`SelectEnemyGunByProgress`) already takes every
  gun with `minEnemyProgress`, whatever `bBigGunList` says.
- Science Fiction mode: weapons have no Sci-Fi flag. Sci-Fi switches the creatures
  (`Creature_Spreading.cc`), some NPC placements and the map items the map editor
  marked "Sci-Fi only" / "realistic only". It is not part of this plan.

## 2. Decisions

- **G1 - the Normal mode code.** (A, recommended) remove it: `ItemIsLegal` loses the
  big-list check (the function stays for the callers, always true; or is removed with
  its two calls), `ReplaceExtendedGuns` and its call, the replacements in
  `Soldier_Profile.cc` and `World_Items.cc`, the calibre-in-name branch of
  `Interface_Items.cc`, the hidden check boxes and their callback in
  `GameInitOptionsScreen.cc`. (B) keep it.
- **G2 - `army-gun-choice-normal.json`.** (A, recommended) remove the file, its loader
  and `getNormalGunChoice`; `SelectStandardArmyGun` always uses the extended table.
  (B) keep the file unused.
- **G3 - the data fields in the code.** (A, recommended) remove `standardReplacement`
  from `WeaponModel` and `MagazineModel` (field, getter, read, write), the
  `ITEM_BIGGUNLIST` flag with `isInBigGunList`, and both fields from the schemas
  (`weapons`, `magazines`, `base-item`). An old mod that still has them loads, the keys
  are ignored. (B) keep the code, remove the data only.
- **G4 - `fGunNut` in the game options.** (A, recommended) remove the field from
  `GAME_OPTIONS`, from Lua and from the save tool tip. The save keeps its layout: the
  byte is written as 1 and skipped when read; the rotation set always adds the Gun Nut
  part (every save of this project has had it since d669b0106). The vanilla save header
  tests drop their `fGunNut` checks. (B) keep the field, always true, and only make it
  read-only for Lua.
- **G5 - the texts.** (A, recommended) keep the unused strings (`GIO_GUN_NUT_TEXT`,
  `GIO_REDUCED_GUNS_TEXT`, `SLG_NORMAL_GUNS`, `SLG_ADDITIONAL_GUNS` ...): the text
  tables are indexed by these enums in all languages, removing entries would shift
  them. (B) remove them from every language.

## 3. Stages (after G1-G5)

### Stage 1 - code

- G1, G2, G4 in the code; G3 in the models and schemas.
- Unit tests: the save header tests read the options after the kept byte (their
  `fGunNut` checks removed); the data load test covers the fields being ignored (stage
  1, data still has them) and gone (stage 2). No shop test: `ItemIsLegal` is removed.
- Done: `ItemIsLegal` and its two calls, `ReplaceExtendedGuns`, the profile and map item
  replacements, `FindReplacementMagazineIfNecessary` (only used there), the calibre in
  the gun name, the hidden gun check boxes, the "gun option" line of the game settings
  message, `getNormalGunChoice`, `standardReplacement`, `isInBigGunList`,
  `ITEM_BIGGUNLIST`, the schema entries, `fGunNut` (options, Lua, editor, save tool tip;
  the save writes 1 / skips its byte).

### Stage 2 - data

- Remove `standardReplacement` and `bBigGunList` from `weapons.json` and
  `magazines.json` (repo and `E:\Release1\externalized`); remove
  `army-gun-choice-normal.json` (both places).
- Game test (new game): TEC-9, Calico, Glock 18 and the other former big-list guns are
  sold at Bobby Ray's and by the dealers, enemies carry them, merc profiles keep their
  guns, map items are not replaced; a save loads and shows its tool tip.
