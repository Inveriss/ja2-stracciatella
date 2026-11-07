# Weapon attachments plan (variant C: rules instead of lists)

Status: **implemented, stages 1-5 game-verified (2026-11-07).** Written 2026-11-06 from
an analysis of `ja2master` 029b77395 (branch `Item_Limits`); line numbers below are
from that commit.

## Implementation status

| Stage | Commit | Notes |
|-------|--------|-------|
| 1 - data model and compatibility | 6229b2e9c | `weapon-attachment-families.json`, allow/deny, startup checks, skill check from the family |
| 2 - one level per family, exclusions | 27cfcaf8d | `FindConflictingAttachment()`; the existing messages, no new strings |
| 3 - effects from data | 5abacf245 | `AttachmentEffectValue()`: the data value, else the family default (original constants / game.json policies) |
| 4 - interface | 5f4776e48 | effects line under the description (`pWeaponAttachmentStrings`), editor: one button per family |
| 5 - test data | (not in the repo) | the `item-limits-test` mod, see below |

Differences from the plan as written below:

- W1/W7: `bAttachment` is not required for an attachment with a family, because
  SPRING_AND_BOLT_UPGRADE, GUN_BARREL_EXTENDER and UNDER_GLAUNCHER of the original
  data don't have it; it stays required in the 7000 block (item limits D5).
- W5: the `attachmentEffects` values are optional; a value the data leaves out is the
  family's default (the original constants and the game.json policies
  `range_bonus_barrel_extender`, `range_penalty_silencer`, `aim_bonus_laserscope`,
  `aim_bonus_sniperscope` keep working). Each effect keeps the original condition rule
  (`scalesWithCondition` was dropped): a range bonus scales, a range penalty doesn't,
  aim/scope/bipod use the 50% bonus/penalty rule, noise scales, AP and burst don't.
- Stage 2: the refusals reuse STR_ATTACHMENT_ALREADY and STR_CANT_USE_TWO_ITEMS.
- Stage 4 (user's decision): the effects are one generated line under the description
  ("Level 2 | Range +0 | Noise 0% | AP -1"), not separate rows; the editor has one
  button per family that steps none, level 1, 2, 3, none.
- **Deferred:** the underbarrel grenade launcher stays item-specific (its weapon mode
  uses the launcher's own weapon stats); Bobby Ray's attachment sub-pages, the sector
  inventory filter and the dealers' item categories still sort the original attachments
  by item index.

Stage 5 test data: the development mod `item-limits-test` (JSON patches in the game's
`mods` folder): family TEST_GRIP (SMG and assault rifles, excludes BIPOD) with levels
7001-7003 (aim +5; aim +10, range +20; aim +15, range +40, AP -1, burst +1), AK74
denies it, GLOCK_17 allows it; the 7000 silencer is SILENCER level 2 without the range
penalty; the SILENCER family gets the rule PISTOL/M_PISTOL/SMG and excludes LASERSCOPE.

Related: [item-limits-plan.md](item-limits-plan.md) (attachment block 7000-7999, D5:
items there need `ITEM_ATTACHMENT`). This is one more item limit, so it belongs to the
branch `Item_Limits`.

## Goal

About 20 new attachment kinds, each in 3 quality levels (about 60 items), usable on
hundreds of weapons **without** a per-weapon entry for each attachment and **without**
C++ changes for each new attachment.

## Current state (analysis)

A new attachment today needs all of this:

1. `WeaponModels.cc`: a `bool attachXxx` field, its JSON key `attachment_Xxx`
   (de)serialized by hand, and a line in `WeaponModel::canBeAttached()`
   (`attachXxx && attachmentID == XXX`). 9 such keys today.
2. `Items.cc`: a row in `AttachmentInfo[]` (target item class + skill check).
3. `weapons.json`: `"attachment_Xxx": true` on every weapon that takes it (default is
   false). Vanilla data: e.g. LaserScope 38, SpringAndBolt 38, SniperScope 24 of 72
   weapons.
4. The effect is code keyed by the item index, `FindAttachment(obj, XXX)`, in about
   27 places:

| Attachment | Effect today | Where |
|---|---|---|
| SILENCER | range -`SILENCER_RANGE_PENALTY`, attack volume by condition, "silenced" shot | Weapons.cc `GunRange`, `GunAttackVolume`, ~807, ~891 |
| GUN_BARREL_EXTENDER | range +`GUN_BARREL_RANGE_BONUS` by condition, wear and fall-off | Weapons.cc `GunRange`, ~2397 |
| LASERSCOPE | aim bonus `LASERSCOPE_BONUS` by condition | Weapons.cc `GunLaserScopeBonus` |
| SNIPERSCOPE | aimed-shot bonus by range and aim time | Weapons.cc ~2457, display bonus |
| BIPOD | prone bonus | Weapons.cc ~2268, display bonus |
| SPRING_AND_BOLT_UPGRADE | -1 AP per shot | Points.cc ~863 |
| DUCKBILL | buckshot spread | LOS.cc ~3359 |
| UNDER_GLAUNCHER | second weapon (grenades): fire mode, AP, cursor, panels | Points.cc, UI_Cursors.cc, Interface_Panels.cc, Weapons.cc ~3925 |
| BURST_EXTENDER_ATTACHMENT | no effect for now | - |

5. `ValidItemAttachment()` (Items.cc ~757) only blocks the **same item** twice, so a
   level 1 and a level 3 silencer could both be attached.
6. Editor: `g_weapon_attachment[]` (Editor/Item_Statistics.cc ~141) is a fixed list of
   6 vanilla attachments for the item buttons.

`OBJECTTYPE` keeps attachments as item indices (`usAttachItem[MAX_ATTACHMENTS]`, 20
slots), so nothing in the save format depends on how compatibility is decided.

## Design

### Family and level on the attachment item

Each weapon attachment (vanilla and new) gets in `items.json`:

```json
"attachmentFamily": "SILENCER",
"attachmentLevel": 2
```

- **Family** = what the attachment is. All levels of a family share compatibility and
  behaviour; levels differ only in their numbers (effects, price, weight, ...).
- **Level** 1-3: for display, sorting and validation only (no hidden rules).

### Family rules, defined once

New file `weapon-attachment-families.json`, one entry per family:

```json
{
  "family": "SILENCER",
  "weaponTypes": ["PISTOL", "M_PISTOL", "SMG", "ASRIFLE"],
  "calibres": [],
  "excludes": [],
  "skillCheck": "NONE"
}
```

- `weaponTypes`: the weapon `internalType` values that take this family
  (PISTOL, M_PISTOL, SMG, RIFLE, SN_RIFLE, ASRIFLE, SHOTGUN, LMG, ...).
- `calibres` (optional): restrict further, empty = any calibre.
- `excludes` (optional): families that cannot be on the same weapon together with this
  one (see "Free placement" below); empty by default.
- `skillCheck`: replaces the `AttachmentInfo[]` row (NONE / SPECIAL / ...).

### Per-weapon exceptions, only where needed

In `weapons.json`, both directions, by family name:

```json
"attachmentsAllow": ["BIPOD"],
"attachmentsDeny":  ["SILENCER"]
```

A family fits a weapon when:
`(rule matches weapon type and calibre AND family not in attachmentsDeny)
 OR family in attachmentsAllow OR the legacy attachment_Xxx flag is true`.

A typical weapon needs **no** attachment entries at all.

### Free placement, one level per family (W4 = variant B)

The player has full freedom: any compatible attachment goes into any of the 20
attachment positions of the item (`usAttachItem[MAX_ATTACHMENTS]`), and any
combination of **different** families is allowed. The position has no effect; the
effects of all attached items are summed. There are no mounting points (muzzle, optic,
...) on the weapon.

Only two restrictions:

1. **One level per family**: a second attachment of a family already on the weapon is
   refused (e.g. level 1 and level 3 silencer together), so effects of one family never
   stack. This replaces today's "same item twice" check, which would not catch two
   different levels.
2. **Optional exclusions**: a family rule may list `excludes` (e.g. SILENCER excludes
   DUCKBILL). The check is symmetric: either rule listing the other is enough. Empty by
   default, so vanilla combinations stay possible.

Rejected alternatives: mounting points with one attachment each (variant A, limits the
player); no restriction at all (variant C, levels of one family would stack).

### Effects from data

Numeric effects live on the attachment item, so each level carries its own numbers:

```json
"attachmentEffects": {
  "rangeBonus": 20,
  "aimBonus": 5,
  "aimedShotBonus": 0,
  "proneBonus": 0,
  "attackVolumePercent": 40,
  "apPerShotChange": -1,
  "burstShotsBonus": 0,
  "scalesWithCondition": true
}
```

- The game sums the effects of all attached items (at most one level per family).
- `scalesWithCondition`: the existing formula (status above 50 % = bonus, below =
  penalty) is applied per attachment.
- Vanilla attachments get numbers equal to today's constants, so their behaviour does
  not change.

**Special behaviour stays code, but keyed by family, not by item index.** Things that
are not a number (underbarrel grenade launcher as a second weapon, duckbill spread,
silenced shot sound/flag, barrel extender wear and fall-off) are written once per
family: `FindAttachmentOfFamily(obj, "SILENCER")` instead of
`FindAttachment(obj, SILENCER)`. Every level of that family then works automatically.
A new family with a new kind of behaviour still needs code once; a new family that is
only numbers needs none.

## Decisions

| ID | Decision | Status |
|----|----------|--------|
| W1 | `attachmentFamily` + `attachmentLevel` (1-3) on attachment items | decided by the user 2026-11-06 |
| W2 | Family rules in `weapon-attachment-families.json` (weapon types, optional calibres, optional excludes, skill check) | decided by the user 2026-11-06 |
| W3 | Per-weapon `attachmentsAllow` / `attachmentsDeny` by family name | decided by the user 2026-11-06 |
| W4 | Variant B: free placement in any of the 20 positions, any combination of different families; only one level per family, plus optional symmetric `excludes` between families; no mounting points | decided by the user 2026-11-06 |
| W5 | Numeric effects on the item (`attachmentEffects`), summed; special behaviour by family | decided by the user 2026-11-06 |
| W6 | Legacy `attachment_Xxx` flags keep working as "allow" (vanilla data unchanged); new data uses rules | decided by the user 2026-11-06 |
| W7 | Startup validation: unknown family or weapon type (also in `excludes`, allow/deny), attachment without family, item in block 7000-7999 without `ITEM_ATTACHMENT`, level outside 1-3, two families with the same name -> clear error | decided by the user 2026-11-06 |
| W8 | Editor attachment buttons built from the data (all families) instead of the fixed list of 6 | decided by the user 2026-11-06 |
| W9 | No save format change: attachments stay item indices | follows from the design |

## Implementation stages

### Stage 1 - data model and compatibility

- `ItemModel`: optional `attachmentFamily`, `attachmentLevel`, `attachmentEffects`
  (only read for `ITEM_ATTACHMENT` items).
- New model + loader for `weapon-attachment-families.json`, with W7 checks and a Rust
  schema (`rust/stracciatella/src/schemas/yaml/`).
- `WeaponModel`: `attachmentsAllow` / `attachmentsDeny`; `canBeAttached()` uses the
  rule from the design; the 9 legacy flags stay (W6).
- `AttachmentInfo[]` rows for gun attachments come from the family `skillCheck`
  (rows for bombs, armour, misc stay).
- Vanilla `items.json`: family and level 1 for the 9 vanilla attachments.
- Unit tests: rule match by weapon type and calibre, allow/deny, legacy flags, every
  vanilla weapon accepts exactly the same attachments as before.

### Stage 2 - one level per family, exclusions

- `ValidItemAttachment()`: refuse when an attachment of the same family is already on
  the item, or when a family on the item and the new one exclude each other (message
  naming the attached item); the DETONATOR / REMDETONATOR rule stays. The position the
  player picks among the 20 is not restricted.
- New message string(s) in the translation files.
- Unit tests: two levels of one family refused; an `excludes` pair refused in both
  orders; vanilla combinations still allowed.

### Stage 3 - effects from data

- Helpers in Weapons.cc: `AttachmentEffectSum(obj, effect)` and
  `FindAttachmentOfFamily(obj, family)`.
- Replace the item-index checks (table above) with family lookups and effect sums:
  `GunRange`, `GunAttackVolume`, laser / sniper scope / bipod bonuses, the AP reduction
  in Points.cc, burst bonus in `GunShotsPerBurst`, silenced flag, duckbill, underbarrel
  launcher, barrel extender wear.
- Vanilla effect numbers equal to today's constants; unit tests compare old and new
  values for every vanilla attachment.

### Stage 4 - interface

- Item description box: attachment effects shown from data (per level).
- Editor buttons from data (W8).

### Stage 5 - test data and game verification

- One test family with 3 levels (numbers only) in the user's game data, block 7000.
- Checks in the game: attaches only to the right weapon types; a deny exception and an
  allow exception work; the second level of the same family is refused; an attachment
  goes into any of the 20 positions; an `excludes` pair is refused; effects of each
  level are visible in the description box and in play (range, aim, noise, AP);
  vanilla attachments behave exactly as before; save and load keep attachments.

## Open questions for later

- Attachments for non-guns (armour plates, head items) stay on their current rules
  (`ArmourModel::canBeAttached()`); moving them to families is a possible later step.
- AI use of new attachments (enemies / militia choosing attachments by family).
- Dealer and Bobby Ray stock for the new families.
