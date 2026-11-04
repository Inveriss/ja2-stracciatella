# Player's tank: the machine gun — plan

Status: **plan, not implemented, all decisions taken**
(written 2026-11-03, branch `Tanks`). Builds on `docs/tank-cannon-firing-plan.md`,
whose stages 1-4 are done. Scope: profile 232 fires a machine gun from the upper
Armament slot, standing, in turn-based combat and in real time.

## 1. What exists today

Checked in the code; nothing here has been tried in the game.

### 1.1 What already works

- **The animation.** `SelectFireAnimation()` gives `TANK_BURST` when a tank has anything
  but the cannon in `HANDPOS`, and `gubAnimSurfaceIndex` maps it to `TNK2_SHT.STI` too.
- **The weapon.** `EquipTank` gives enemy tanks a `MINIMI` in a pocket.
- **The bullet.** A machine gun bullet is an ordinary one; only `TANK_CANNON` explodes.
- **Bursts.** `IsGunBurstCapable`, `CalcAPsToBurst` and the Short / Long / Full modes
  read the gun in `HANDPOS`, so they need no tank case.
- **The gunner.** `CalcChanceToHitGun`, `CalcAimSkill`, experience, kills and statistics
  already go through `GetTankGunner` and `ShooterFor`.
- **Ammunition search.** `FindAmmoToReload` looks through the whole inventory, and the
  cargo is the tank's own inventory, so `AutoReload` should find magazines there.
- **Second hand.** `IsValidSecondHandShot` stays false with the cannon and the MINIMI in
  the two hands: the MINIMI is two handed, the cannon never has `ubGunShotsLeft`.

### 1.2 What stops it

| What | Where | Effect |
|---|---|---|
| No gun | `EquipPlayerTank` | Only the cannon is created |
| One active weapon | All of the firing code reads `HANDPOS` | The gun that fires must be in `HANDPOS`, so the two must swap |
| Swap is off | `HandleTBSwapHands`, `TMClickFirstHandInvCallbackSecondary` and the second hand's, since cannon stage 2 | No way to choose a weapon |
| Fixed slot | `m_invSlotPosition*Tank[HANDPOS]` is the lower slot | After a swap the MINIMI would show in the cannon's place |
| Second hand hidden | `m_invSlotPosition*Tank[SECONDHANDPOS]` | The upper slot shows nothing |
| Endless ammunition | `DeductAmmo` returns early for `TANK` and any weapon but the cannon | Bullets are never used up |
| Tank penalties | `CalcChanceToHitGun`: `TANK()` at max visible distance (2 per tile), under `MIN_TANK_RANGE` (13 or 25 per tile) | Written for the cannon; a machine gun is close to useless near the tank |
| Skill traits | `CalcChanceToHitGun`: `NUM_SKILL_TRAITS(pSoldier, AUTO_WEAPS)` | Reads the tank, not the gunner |
| Jams | `CheckForGunJam` skips `EXPLOSIVE_GUN` only | The MINIMI would jam, and nobody could reach it to clear or repair it |
| Pickup guard | Primary click handlers return for `HANDPOS` only | The gun in `SECONDHANDPOS` could be picked up |
| Wreck | `Rotting_Corpses.cc` keeps only `HANDPOS` | The other weapon would drop |
| Burst button | `Interface_Panels.cc`: `burst && CanShootFromVehicle(s)` | The `B` key is blocked while the SM panel is open |
| Ready cost | `GetAPChargeForShootOrStabWRTGunRaises`: a tank is never in a ready animation | `ubReadyTime` 3 is paid for every attack |

### 1.3 AP by the existing formula

25 AP. The MINIMI has `ubShotsPer4Turns` 13; the burst adds `AP_BURST` 5.

| Aim skill | Cannon | MG, one shot | MG, burst |
|---|---|---|---|
| 0 | 25 | 15 | 20 |
| 80 | 14 | 9 | 14 |
| 100 | 13 | 8 | 13 |

Reloading costs `AP_RELOAD_GUN` 5, plus any aiming and turning. Without M6 add 3.

### 1.4 Not known yet

`TANK_BURST` uses the same 64 frames as the cannon. Whether its frames show the
cannon's flash, recoil or smoke cannot be read from the code; the scripts are in the
data files. Long and Full bursts also rely on the loop in `Soldier_Ani.cc` (codes 448,
449), which was written for mercs.

## 2. Decisions

All decided as proposed (user, 2026-11-03).

| # | Question | Decision |
|---|---|---|
| M1 | Which gun? | The `MINIMI`, as the enemy's tanks have: 5.56 mm, magazines of 30, sold in the game. A dedicated tank gun (T-55: 7.62 mm belt) would need a new item and art, later if wanted |
| M2 | Choosing the weapon | The swap again: `Ctrl+Q`, and a right click on the tank's hand window in the team panel. The action cursor stays as it is |
| M3 | Seeing the active weapon | The team panel shows the active weapon in the first hand window and the other in the second, as for a merc. This changes cannon decision G3, which hides it. The panels keep each gun in its own slot |
| M4 | Who aims? | The same gunner as the cannon (G1). One shooter, one pool of AP |
| M5 | Ammunition | Magazines from the cargo, as a merc: the usual reload cursor and click. Dropping a magazine on the upper slot reloads as well. Rounds left show on the slot. Out of ammo: the driver's quote |
| M6 | Ready cost | None. A mounted gun needs no raising, so the player's tank pays no ready time; a turn then costs the usual `AP_LOOK_STANDING` |
| M7 | Wear and jams | None, as for the cannon: nobody can reach the gun to repair it |
| M8 | Tank penalties | The two close range penalties only for the cannon on the player's tank. Enemy tanks do not change |
| M9 | Picking up, wreck | As the cannon: can't be picked up, right click opens the description, stays with the wreck |

Out of this plan: firing while driving (`docs/tank-firing-while-moving-plan.md`), and a
second shooter firing both weapons in one turn.

Test switch: while `TANK_IN_A9_FOR_TESTING` is on, the tank also starts with magazines
in a cargo slot.

## 3. Stages

### Stage 1: the gun aboard

- `EquipPlayerTank`: the MINIMI in `SECONDHANDPOS`, loaded, `OBJECT_UNDROPPABLE`; test
  magazines in `BIGPOCK2POS`.
- `UILayout.cc`: `m_invSlotPosition*Tank[SECONDHANDPOS]`: tactical (453, tacHeliY + 99),
  strategic (47, mapHeliY + 338).
- `LayoutTankCannonSlot()` becomes the layout of both weapons, width 157, by the item:
  the cannon always low, the MINIMI always high, whichever hand holds it.
- The click handlers, the wreck: both hands (M9).

Check: both graphics show, in the right slots, on both panels; no pickup; the
description opens for each.

### Stage 2: choosing and firing

- `HandleTBSwapHands` and the team panel callbacks open again for the tank (M2), and
  `RenderSoldierTeamInv` and its help text show the weapons (M3). The burst mode resets
  on a swap; `ReLoadSoldierAnimationDueToHandItemChange` already does this for a gun
  that can't burst. Refresh the slot layout after a swap.
- `Interface_Panels.cc`: the burst button also for the player's tank.
- `CalcChanceToHitGun`: `AUTO_WEAPS` from the gunner; the tank penalties (M8).
- `GetAPChargeForShootOrStabWRTGunRaises`: no raise for the player's tank (M6).
- `CheckForGunJam`: not for the player's tank (M7).

Check: one shot, Short, Long, Full; the AP against the table in 1.3; the look of
`TANK_BURST` (1.4); no interface lock; the second hand never fires.

Implemented and game-verified (2026-11-03). `SwapTankWeapons()` does the swap:
`SwapHandItems()` would move the cannon out for a two handed gun, and
`ReLoadSoldierAnimationDueToHandItemChange()` would play the raise and lower rifle
animations. It refuses while the tank drives or shoots. The two refusal messages no
longer name the cannon.

Found in the game, and what answers 1.4:
- `TANK_BURST` repeats until `bDoBurst` passes the burst length, so a single shot never
  ended. A single shot uses `TANK_SHOOT` now.
- Both animations use the cannon's frames, so for the machine gun
  `DetermineSoldierAnimationSurface` gives the standing picture; the script only times
  the shots. The gun has no muzzle flash of its own yet.
- `TANK_SHOOT` moves the hull by its script (`ja2bin.dat`, codes 412 and 413). They are
  skipped for the machine gun.
- `IsValidSecondHandShotForReloadingPurposes` took the cannon for a second gun to reload
  and crashed on its shell: false for the player's tank.
- The burst button works on the tank's panel, with `B`.

### Stage 3: ammunition

- `DeductAmmo`: the early return for enemy tanks only. Done with stage 2.
- `FindAmmoObjToReload`, `GetAPsToAutoReload`, `AutoReload`: confirm for a vehicle, with
  the panel refreshed.
- Primary click handlers: a magazine on the cursor reloads the gun in the slot (M5).
  The old magazine goes to the cursor as usual.

Implemented and game-verified (2026-11-04): `IsTankGunReload()`, through
`CanItemFitInPosition`.

Check: bullets run out, the reload cursors, reloading in combat and out, a partial
magazine, the wrong calibre refused, no magazines left.

### Stage 4: the whole in play

All directions; a target next to the tank and far away; a wall, a roof; a neutral or
friend; a burst interrupted; the tank destroyed in a burst; the gunner changing between
shots; experience and kills to the gunner; save and load; a new game. Then the balance.

## 4. Risks

| Risk | Note |
|---|---|
| Enemy tanks change by accident | `TANK()` covers them: each change goes through `IsPlayerTank()` |
| The look of the burst | 1.4; only the game can show it |
| The bullet hits the tank | It starts inside the footprint. Enemy tanks fire this way, but test all directions, with passengers too |
| Interface lock | A burst has more attack bookkeeping than one shell; stage 2 |
| Too strong | 25 AP gives two or three bursts with skill 80. Ammunition and the gunner's skill are the brakes; tune in stage 4 |
| The hull plan | `docs/tank-rotating-hull-plan.md`: `TANK_PLAYER` needs `TANK_BURST` as well; it is already on its list |
