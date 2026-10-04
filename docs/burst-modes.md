# Burst modes

Guns that can fire bursts have three burst lengths instead of one: **Short**,
**Long** and **Full**. AP costs are calculated by the original JA2 system.

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

AP costs use the original JA2 system without changes: `ubShotsPer4Turns` in
`weapons.json` keeps its original meaning and values, the original formula in
`BaseAPsToShootOrStab()` (`Points.cc`) with its rounding is unchanged. The
value is a parameter of that formula, not an AP cost.

### Burst costs

A burst costs the single shot plus the original burst surcharge
(`CalcAPsToBurst()`): about 5 AP for a merc with 25 AP per turn, at least 3.
The Spring and Bolt Upgrade lowers it as before. **All three burst modes cost
the same** for now, and the G11 no longer has its special 1 AP burst.

Fields for a later stage, `null` in the vanilla data for every weapon that can
burst:

| Mode  | JSON field           |
|-------|----------------------|
| Short | `ubAPsPerShortBurst` |
| Long  | `ubAPsPerLongBurst`  |
| Full  | `ubAPsPerFullBurst`  |

Despite the name these are **not AP costs**. Like `ubShotsPer4Turns`, a value
is a parameter (shots per 4 turns) of the original AP formula: when a weapon has
a number there, that burst mode costs what the original formula gives for it
instead of `ubShotsPer4Turns` (the single shot's cost is replaced, not added
to), so the cost still depends on the merc's AP and aim skill. The Spring and
Bolt Upgrade works through the formula like for a single shot. `null` or a
missing field means the original burst cost.

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
