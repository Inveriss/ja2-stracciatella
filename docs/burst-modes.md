# Burst modes

Guns that can fire bursts have three burst lengths instead of one: **Short**,
**Long** and **Full**. Each mode has its own fixed AP cost, see
`docs/weapons-ap-formula-plan.md`.

Code: `src/game/Tactical/Weapons.cc` (modes, burst length), `Points.cc` (AP
costs), `Soldier_Ani.cc` (burst animation loop), `Soldier_Control.cc` (burst
start), `UI_Cursors.cc` and `src/game/Utils/Cursors.cc` (cursor label),
`src/externalized/WeaponModels.cc` (JSON).

## Weapon modes

The B key and the burst button in the inventory bottom panel switch modes the
same way:

Single → Short → Long → Full → underslung grenade launcher → Single

Modes the weapon in hand can't use are skipped: a gun without burst goes
straight from Single to the grenade launcher (if it has one), a gun without
launcher goes from Full back to Single.

When the burst cursor is shown, a label above it names the mode: `Short`,
`Long` or `Full` (`TacticalStr` in `translation-*.json`, English in every
language for now). The cursor graphics and the panel button icon are the same
for all three burst modes.

Aiming at an enemy, the burst cursor also names the body part under it (head,
torso, legs) 9 pixels below the burst image, with the chance to hit below
that, like the single shot cursor. Bursts always aimed at that body part, only
the label was missing.

## Burst length

| Mode  | JSON field             | Default          |
|-------|------------------------|------------------|
| Short | `ubShotsPerShortBurst` | 3                |
| Long  | `ubShotsPerLongBurst`  | 6                |
| Full  | `ubShotsPerFullBurst`  | `ubMagSize`      |

- A burst never fires more bullets than the magazine holds when it starts.
- **Full** fires `min(ubShotsPerFullBurst, bullets in the magazine)`. The field
  is the factory magazine size, so a larger magazine (e.g. a future C-MAG with
  100 rounds in an M16) does not make a Full burst longer than 30 shots. The
  length is taken once, when the burst starts.
- A burst that ends because it fired all its shots no longer shows the
  "burst fire depleted the clip" message, even if the magazine is now empty.
  The message is shown only when the magazine runs out before the burst is
  complete.
- A gun can burst when at least one of the three fields is above 0. A mode
  whose field is 0 is skipped when switching modes.
- The Burst Extender attachment has no effect for now. Burst length
  attachments (a later stage, e.g. Burst Extender Elite: +3 Short, +4 Long)
  never change Full.
- The burst animation loop supports up to 100 shots
  (`ENABLE_EXTENDED_BURST_FIRE` in `Soldier_Control.h` must stay `true`).

The old `ubShotsPerBurst` field is no longer used by the game data. If a
weapon (e.g. from a mod) still has it and none of the new fields, a value above
0 gives the defaults 3 / 6 / magazine size, 0 means no burst.

## AP costs

Since branch `Weapons_AP_Formula` every cost is a direct AP value in `weapons.json`, the same
for every merc whatever his AP, wounds or marksmanship (full rules:
`docs/weapons-ap-formula-plan.md`):

| Shot  | JSON field              |
|-------|-------------------------|
| Single | `ubAPCostPerSingleShot` |
| Short | `ubAPCostPerShortBurst` |
| Long  | `ubAPCostPerLongBurst`  |
| Full  | `ubAPCostPerFullBurst`  |

- A burst field is the **full cost of the burst**, not a surcharge on top of the single shot.
  `CalcAPsToBurst()` (`Points.cc`) returns the difference for its callers.
- The three modes can differ. In the vanilla data they all start at the single shot + 4.
- A weapon that can burst but has no value for a mode (`null` or missing, e.g. from a mod)
  pays the single shot + 4 for it.
- The Spring and Bolt Upgrade takes a fixed 1 AP off the single shot and off every burst
  mode, whatever its status; a cost never falls below 1 AP.
- Ready time, turning, the 1 AP for a new target and aiming clicks come on top, as for a
  single shot. Bursts aren't aimed.

## Who uses which mode

- **Player mercs:** the mode chosen with B or the panel button.
- **Enemy AI:** Long (enemies don't keep a mode, the AI only decides to burst).
- **Psycho mercs:** sometimes switch from a single shot to Full, only when they
  have enough AP for it.
- **A merc or soldier woken up by an attack** who fires back switches to Short.

## Saved games

The mode is stored in the existing `bWeaponMode` byte. The new modes are added
after the old ones (`WM_NORMAL` 0, `WM_BURST_SHORT` 1 = the old burst mode,
`WM_ATTACHED` 2, `WM_BURST_LONG` 3, `WM_BURST_FULL` 4), so older saves load
with their mode unchanged. The save format did not change.

## Not part of this stage

Item description and info box values, burst sounds (the per-count sound files
only exist for short bursts; longer bursts fall back to the generic burst
sound), translations of the labels, larger magazines (C-MAG, Auto Rocket Rifle
with 6 rockets — until then its Full burst is 5) and burst length attachments.
