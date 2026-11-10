# Dual burst: burst fire with two guns at once

Status: PLAN, decisions D1–D12 decided 2026-11-10: all as recommended (A).
Nothing implemented.

## 1. Current state (code facts)

- Two-gun fire works only for single shots. `IsValidSecondHandShot()`
  (`Soldier_Control.cc`) requires a one-handed gun (`bTwoHanded: false`) in
  `SECONDHANDPOS`, a gun in `HANDPOS` **and `!s->bDoBurst`**. Picking a burst
  mode with two pistols in hand fires a normal burst from the right-hand gun
  only.
- `IsValidSecondHandShotForReloadingPurposes()` also has `!bDoBurst`, so in
  burst mode only the main gun is reloaded.
- Dual single shot = animation states `SHOOT_DUAL_STAND/CROUCH/PRONE`. The
  scripts come from `BINARYDATA/ja2bin.dat` (`gusAnimInst`, 320 x 100 codes):

  ```
  SHOOT_DUAL_STAND : 757 459 470 441 430 17 18 477 19 20 458 470 441 430 21 22 477 23 24 753 473 699 999
  SHOOT_DUAL_PRONE : 757 5 459 470 441 430 6 7 477 8 9 458 470 441 430 10 11 477 12 753 473 981 999
  STANDING_BURST   : 757 441 448 430 19 20 477  (x6)  21 20 18 449 753 473 612 999
  ```

  Code 459 = attack with the first hand, 458 = attack with the second hand,
  430 = fire (event -> `UseGun`), 448 = burst check (length, ammo, jam),
  449 = burst end, 441 = muzzle flash, 477 = shot end. Frames 17–20 are the
  right-hand shot and 21–24 the left-hand shot (prone: 6–9 and 10–12).
- Burst logic uses **one** counter (`bDoBurst`, incremented in `UseGun`), one
  length (`ubBurstLength = CalcBurstLength(s, inv[ubAttackingHand])`) and one
  anchor for the extended burst (`usBurstFireAniCodeAnchor`, cases 448/449/477).
- AP: dual single shot charges the higher of the two shot costs
  (`CalcTotalAPsToAttack`); a burst charges `CalcAPsToBurst(HANDPOS gun, mode)`
  once, at the first shot (`UseGun`, `bDoBurst == 1`).
- Hit chance: dual = −`AIM_PENALTY_DUAL_PISTOLS` (20); burst = the weapon's
  `ubBurstPenalty × (bDoBurst − 1)` per following shot.
- `NUMANIMATIONSTATES` = 311 of `MAX_ANIMATIONS` 320, so 9 free animation slots.

## 2. Answers

1. **Is it possible?** Yes. Nothing in the engine blocks it. Today the
   mechanic is switched off in one condition (`!bDoBurst`). The burst
   bookkeeping only knows one gun, so it has to learn about two.
2. **What changes?** See section 4: the validity condition, burst length and
   counters per hand, three new animation states with scripts built in code,
   AP cost, hit chance, sound, reload, burst end and unjam states, and the UI
   labels. Optionally AI.
3. **New graphics?** **Not required.** The dual-shot graphics already have
   separate right-hand and left-hand firing frames. A burst can loop them
   alternately (R, L, R, L ...) just as the normal burst loops frames 19–20.
   The dual graphics exist for every body type that can dual-wield: regular
   male (also used by stocky), big male and female, standing, crouching, prone
   and in water:

   | Body | Stand | Crouch | Prone | Water |
   |---|---|---|---|---|
   | S_MERC | `s_dblsht.sti` (25 frames/dir) | `sc_dblsh.sti` (25) | `s_db_prn.sti` (12) | `sw_dblsht.sti` (25) |
   | M_MERC | `m_dblsht.sti` | `mc_dblsh.sti` | `m_db_prn.sti` | `mw_dbl.sti` |
   | F_MERC | `f_dblsht.sti` | `fc_dblsh.sti` | `f_db_prn.sti` | `fw_dbl.sti` |

   Optional, only if the look is not good enough after stage 2: new frames
   showing both guns firing at the same moment (both muzzles flashing), with
   8 directions per frame, added to the 12 files above or as 12 new files.
   This would be a later stage and is not needed for the mechanic.

## 3. Decisions

- **D1 – When is it a dual burst?** (A, recommended) both guns are one-handed
  and both have the active burst mode (Short/Long/Full). (B) the second gun
  without that mode fires single shots in between.
- **D2 – Shot order.** (A, recommended) alternate R, L, R, L, like the dual
  single shot. (B) all right-hand shots first, then all left.
- **D3 – Length.** (A, recommended) each gun fires its own length for the mode
  (`CalcBurstLength` per gun, attachments included), so the total is the sum.
  (B) the main gun's length, split between the hands.
- **D4 – A gun runs out, jams or finishes earlier.** (A, recommended) its turns
  are skipped and the other gun goes on. (B) the whole burst stops. The jam check
  stays per gun, at its first shot.
- **D5 – AP cost.** (A, recommended) the higher of the two burst costs (same
  rule as the dual single shot). (B) the sum of both costs. (C) the higher cost
  plus a fixed extra.
- **D6 – Hit chance.** (A, recommended) −20 dual penalty on every shot, plus
  the burst penalty of the firing gun times **that gun's** shot index (each
  hand has its own recoil). (B) the shared index of all shots (harsher).
- **D7 – Sound.** (A, recommended) every shot plays its own gun's single-shot
  sound (or silenced sound). (B) the main gun's burst sample once.
- **D8 – Spread burst (fDoSpread over several targets).** (A, recommended) not
  for dual bursts in the first version: a dual burst goes at one target.
  (B) supported from the start.
- **D9 – AI.** (A, recommended) enemies and militia with two one-handed guns
  may use it, with the same cost rules. (B) player only.
- **D10 – Experience.** (A, recommended) one marksmanship gain per burst, cut to
  2/3 as for the dual single shot.
- **D11 – Graphics.** (A, recommended) the existing frames (section 2.3).
  (B) new frames as a later stage.
- **D12 – Labels.** (A, recommended) the burst cursor and the weapon box show
  the total shots and "x2" for a dual burst.

## 4. Implementation (by stages, after D1–D12)

### Stage 1 – Rules and costs (no visible burst yet)

- `Soldier_Control.cc`: split the conditions.
  - `IsValidSecondHandShot()` stays for single shots.
  - New `IsValidDualBurst(s)` (D1): both guns one-handed and usable, with
    rounds, and the active mode valid for both.
  - `IsValidSecondHandShotForReloadingPurposes()` drops `!bDoBurst`, so both
    guns reload in burst mode as well.
- `Weapons.cc`: new `CalcDualBurstLength()`, which gives the shots per hand (D3).
- `Points.cc` `CalcTotalAPsToAttack`: the dual burst cost (D5).
  `Interface_Items.cc` (burst AP lines) uses the same function.
- Unit tests: validity (two pistols, pistol + rifle, mode missing on one gun),
  lengths, AP.

### Stage 2 – Animations

- `Animation_Control.h`: append `DUAL_BURST_STAND`, `DUAL_BURST_CROUCH` and
  `DUAL_BURST_PRONE` before `NUMANIMATIONSTATES` (311 -> 314).
- `Animation_Control.cc`:
  - `gAnimControl` rows copied from `SHOOT_DUAL_*`;
  - `gubAnimSurfaceIndex` for REGMALE, BIGMALE, STOCKYMALE and REGFEMALE, set
    to the same surfaces as `SHOOT_DUAL_*`;
  - `gubAnimSurfaceMidWaterSubIndex` for standing in water.
- Scripts **built in code** after `LoadAnimationStateInstructions()`, so
  `ja2bin.dat` stays untouched and no new data file is needed. The scripts are
  hand blocks, repeated for both hands:
  `459 441 448 430 17 18 477` and `458 441 448 430 21 22 477`
  (prone: frames 6 7 / 10 11), repeated, then `449 753 473 <aim state> 999`.
- A new animation code (a free number, e.g. 488, to be confirmed) at the start
  of each block chooses the next hand per D2/D4. It jumps to that hand's block
  or ends the burst. Hand switching is then driven by code, and the repeat count
  of the table no longer limits the burst. The extended-burst anchor logic is
  reused.
- `SelectFireAnimation()`: `IsValidDualBurst` -> the new states.
- End and recovery states:
  - burst end goes to `AIM_DUAL_*`, not `AIM_RIFLE_*` (case 448 stop, 449);
  - unjam mapping in `Soldier_Ani.cc` (~3345) maps the new states to the
    `*_SHOOT_DWEL_UNJAM` states;
  - `CheckForImproperFireGunEnd`.

### Stage 3 – Firing logic

- `SOLDIERTYPE`: transient per-hand counters (shots fired per hand, length per
  hand), set in `EVENT_FireSoldierWeapon` and cleared at burst end. They are
  not saved: a burst never spans a save. If they still go into the save file,
  that means a new game, without compatibility code.
- `EVENT_FireSoldierWeapon`: `bBulletsLeft` = the sum of both hands (buckshot
  pellets included).
- Case 448 / the new code: length, ammo and jam per `ubAttackingHand` (D4).
  The burst ends when both hands are done.
- `UseGun`:
  - AP deducted once, at the first shot of the burst (D5);
  - sound per shot (D7);
  - ammo and wear per hand (already per `ubAttackingHand`);
  - experience (D10).
- `CalcChanceToHitGun`: the dual penalty plus the per-hand burst penalty (D6).
- Spread burst off for dual bursts (D8).

### Stage 4 – Interface

- Cursor burst labels (Short/Long/Full) and the weapon box AP/shots lines (D12).
- Weapon mode switching allows burst modes when `IsValidDualBurst` holds.

### Stage 5 – AI (if D9 = A)

- `Attacks.cc` / `DecideAction.cc`: the burst AP cost and the expected hits use
  the dual functions when the soldier holds two guns.

### Stage 6 – Test mod and game tests

- A test merc with two Glocks, two SMGs (one-handed), pistol + SMG with
  different modes, and one gun almost empty.
- Check standing, crouching, prone, in water, a jam on one gun, Full mode,
  and a big male and a female body.
