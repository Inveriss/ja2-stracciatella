# Magazine size variants plan

Status: **implemented and game-verified (2026-11-06).** Stage 1 (lookup instead of
`±1`): a2e8ab482; stage 2 (duplicate warning): c41c6aac1; stage 3 (game
checks) done with the original 7.62 mm pairs and the 3000 test magazine of the
`item-limits-test` mod. Written 2026-11-06 from an analysis of `ja2master` 029b77395
(branch `Item_Limits` at 56cbdedf1); line numbers below are from that commit.

Related: [item-limits-plan.md](item-limits-plan.md) (new ammo block 3000-3999). The
adjacent-index rule is one more item limit, so this plan belongs to the same branch,
`Item_Limits` (M5). It does not depend on the item limits stages and can be done
before or after them.

## Problem

A gun can be loaded with a magazine of the same calibre but a different size, e.g.
`CLIP762N_20_AP` into a gun with a 5-round magazine, or `CLIP762N_5_AP` into a gun with
a 20-round magazine. This only works correctly when the two magazines have
**consecutive item indices, the smaller one first** (vanilla: 100 = `CLIP762N_5_AP`,
101 = `CLIP762N_20_AP`).

### Where the rule comes from

- Whether a magazine may be loaded at all checks the calibre only:
  `CompatibleAmmoForGun()` (Interface_Items.cc ~2315), `ValidAmmoType()`
  (Items.cc ~836), `FindAmmoToReload()` (Items.cc ~1619, prefers the same size, then
  any size). A different size costs double AP (`GetAPsToReloadGunWithAmmo()`,
  Points.cc ~1445).
- `ReloadGun()` (Items.cc ~1325) must store in the gun which magazine item it now holds
  (`usGunAmmoItem`). For a different size it does not search, it guesses:
  - magazine larger than the gun: `usNewAmmoItem = pAmmo->usItem - 1;` (~1442)
  - magazine smaller than the gun: `usNewAmmoItem = pAmmo->usItem + 1;` (~1454)

### What goes wrong without adjacent indices

Shooting is fine (damage uses `ubGunAmmoType`, taken from the real magazine), but the
gun remembers the wrong item, which comes back as a real item:

- unloading: `EmptyWeaponMagazine()` creates `usGunAmmoItem` (Items.cc ~1580);
- swapping magazines: the old magazine is `usGunAmmoItem` (Items.cc ~1383);
- selling a loaded gun: the shop splits it into gun + `usGunAmmoItem`
  (ShopKeeper_Interface.cc ~5375);
- auto-reload looks first for `usGunAmmoItem` in the inventory (Items.cc ~1637).

The neighbour may be another calibre, another ammo type, another size, or not a
magazine at all (possible crash in `asAmmo()`). Also only pairs work: with three sizes
(10/20/30) a 30 loaded into a 10-round gun becomes the 20.

### Existing helper

`FindReplacementMagazine(calibre, mag_size, ammo_type)` (Items.cc ~2835) already
searches all magazines by calibre, size and ammo type. It is used when a gun is
replaced by another gun. When no magazine has the requested ammo type it falls back to
any magazine of that calibre and size, which would silently change AP into regular
ammo, so it cannot be used as is for reloading.

## Decisions

| ID | Decision | Status |
|----|----------|--------|
| M1 | Replace the `±1` guess with a lookup: a magazine with the gun's calibre, the gun's magazine size and **the same ammo type** as the loaded magazine. Item indices and their order no longer matter; any number of sizes works | decided by the user 2026-11-06 (variant A) |
| M2 | When no such magazine exists, the gun keeps the loaded magazine item itself (`usGunAmmoItem = pAmmo->usItem`); unloading returns that magazine with the remaining rounds (rejected alternative: refuse to load a different size without a matching variant) | decided by the user 2026-11-06 |
| M3 | Data check at startup: a warning when two magazines share calibre, capacity and ammo type (the lookup then takes the one with the lowest index) | decided by the user 2026-11-06 |
| M4 | No save format change: `usGunAmmoItem` keeps its meaning, so no new game is needed | follows from M1 |
| M5 | Done on the existing branch `Item_Limits`, not on a new branch: it removes another item limit | decided by the user 2026-11-06 |

## Implementation stages

### Stage 1 - lookup instead of `±1`

- Items.cc: new function, e.g.
  `UINT16 FindMagazineVariant(const WeaponModel* gun, const MagazineModel* loaded)`:
  - same capacity as the gun -> `loaded` itself;
  - otherwise iterate `GCM->getMagazines()` for calibre == gun calibre,
    capacity == `gun->ubMagSize`, `ammoType` == `loaded->ammoType`; lowest index wins;
  - nothing found -> M2 result (`loaded` itself).
- `ReloadGun()`: both branches (~1442, ~1454) use the new function. The bullet counts
  (`ubBulletsToMove`, capped by `ubMagSize`) stay as they are.
- `FindReplacementMagazine()` and its current callers stay unchanged.
- Unit tests (VanillaWeapons_unittests.cc or a new file):
  - vanilla pairs: 20 into a 5-round gun -> the 5 of the same type, and back;
  - AP / HP keep their type;
  - a magazine with no variant -> the loaded item itself (M2);
  - a test set with three sizes and non-adjacent indices.

### Stage 2 - data check (M3)

- After `loadMagazines()`: log a warning for every duplicate
  (calibre, capacity, ammo type), naming both items.

### Stage 3 - game verification

- 20-round AP magazine into a 5-round gun and the reverse: reload, unload, the unloaded
  magazine is the right item with the right number of rounds.
- Swap magazines in combat and outside combat (old magazine returned correctly).
- Sell a loaded gun at a shop: the split-off magazine is the right item.
- Auto-reload picks the expected magazine.
- Double AP for a different size is unchanged.
- A test magazine with a non-adjacent index (e.g. in the 3000 block once
  `Item_Limits` is in) works the same way.
