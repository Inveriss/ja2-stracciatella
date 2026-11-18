# Reload AP by weapon type - plan

Status: plan written 2026-11-18, decisions R1-R7 decided 2026-11-18 (all A; R3 with
15 AP instead of 5). Implemented on the Weapons branch. Reload AP by weapon type COMPLETE
(2026-11-18).

| Stage | Commit | State |
|---|---|---|
| 1 - data and the lookup | this commit | game-verified |
| 2 - the game uses it | this commit | game-verified |

Give every weapon its own AP cost to reload: by default from a table of weapon types
in `game.json`, and a weapon of `weapons.json` can set its own value that wins over
the table.

## 1. Current state (code facts)

- The reload cost does not depend on the calibre: it is one constant,
  `AP_RELOAD_GUN` = 5 (`Points.h`).
- `GetAPsToReloadGunWithAmmo(gun, ammo)` (`Points.cc`) is the one place that prices a
  reload:
  - launchers (`IC_LAUNCHER`: grenade launcher, mortar, under-barrel GL): 5;
  - a magazine of the gun's size: 5;
  - a magazine of another size (`isSameMagCapacity` false): 5 + 5 = 10.
- It is used by `ReloadGun` (`Items.cc`, the player, the AI through `TryToReload`, the
  sector inventory and vehicle cargo paths) and by `GetAPsToAutoReload` (the cursor's
  reload cost, both guns of a two-gun soldier added up).
- The description box (`Interface_Items.cc`, the "reload" line) repeats the same
  rule by hand.
- `AP_RELOAD_GUN` is also the cost to put on / take off an attachment in the
  description box (`Interface_Items.cc`, `DoAttachment` path). That is not a reload.
- Weapon types (`internalType` in `weapons.json`): PISTOL 17, M_PISTOL 4, SMG 13,
  SHOTGUN 4, ASRIFLE 15, RIFLE 3, SN_RIFLE 2, LMG 3; plus LAUNCHER, LAW, CANNON, blades,
  thrown and the creature attacks, which never reload a magazine.
- The heavy weapons (`IsHeavyWeapon`, sorted as launchers) carry an ordinary type:
  rocket rifle RIFLE, auto rocket rifle ASRIFLE, flamethrower SHOTGUN; the dart gun is
  a PISTOL.
- The AP costs of a weapon are already direct values in `weapons.json`
  (`ubAPCostPerSingleShot`, `ubAPCostPerShortBurst` ..., docs/weapons-ap-formula-plan.md).

## 2. The proposal

```
// game.json
//  AP to reload a weapon by its type (docs/weapon-reload-ap-plan.md); a weapon of
//  weapons.json with "ubAPCostPerReload" uses that value instead. A magazine of another
//  size than the weapon's costs double.
"weapon_reload_ap": {
  "PISTOL": 2,
  "M_PISTOL": 3,
  "SMG": 5,
  "SHOTGUN": 10,
  "ASRIFLE": 8,
  "RIFLE": 10,
  "SN_RIFLE": 12,
  "LMG": 15,
  "HEAVY_WEAPON": 15
}
```

```
// weapons.json, only where a weapon differs from its type
"ubAPCostPerReload": 4,
```

`GetAPsToReloadGunWithAmmo` becomes: the weapon's own `ubAPCostPerReload`, else the
table value of its type (heavy weapons and launchers: `HEAVY_WEAPON`), doubled for a
magazine of another size. One function `WeaponReloadAPs(weapon)` gives the base value
to the reload, the cursor and the description box.

This is the same idea as `weapon-attachment-families.json` (a rule by weapon type,
an exception per weapon), but a single number per type fits better in `game.json` next
to the other tables than in a file of its own.

## 3. Decisions

- **R1 - name of the weapon field.** (A, recommended) `ubAPCostPerReload`, like the
  other AP fields (`ubAPCostPerSingleShot`, `ubAPCostPerShortBurst` ...). (B)
  `ubAPCostReload`. (C) `ubReload`.
- **R2 - where the table lives.** (A, recommended) `game.json`, section
  `weapon_reload_ap` (one value per type, like `enemy_ammo`). (B) a new file
  `weapon-reload-ap.json`. (C) inside `weapon-attachment-families.json` (mixes two
  systems).
- **R3 - heavy weapons and launchers.** (A, recommended) one entry `HEAVY_WEAPON` for
  every weapon `IsHeavyWeapon` names and every launcher (grenade launcher, mortar,
  rocket rifles, flamethrower), 15 AP (decided: 15, not the 5 of now); else the rocket rifle would cost 10 as a
  RIFLE and the flamethrower 10 as a SHOTGUN. (B) by their `internalType`, exceptions
  through `ubAPCostPerReload`.
- **R4 - a type missing from the table.** (A, recommended) the game stops with a data
  error naming the type, when a weapon that reloads (a gun or launcher with a magazine
  or a launcher) has a type that is not in the table and no `ubAPCostPerReload`.
  (B) fall back to 5 silently.
- **R5 - a magazine of another size.** (A, recommended) double the weapon's cost, as
  now (5 -> 10 becomes e.g. 8 -> 16 for an assault rifle). (B) + 5 as now, whatever the
  weapon. (C) no surcharge.
- **R6 - putting on / taking off an attachment.** (A, recommended) stays 5 AP
  (`AP_RELOAD_GUN`, renamed `AP_ATTACH_ITEM` so it is not mistaken for the reload).
  (B) the weapon's reload cost.
- **R7 - attachments and skills.** (A, recommended) no effect on the reload cost
  (the spring and bolt upgrade changes shots only); a reload cost could later become an
  attachment effect like `apPerShotChange`. (B) add a `reloadApChange` effect now.

## 4. Stages (after R1-R7)

### Stage 1 - data and the lookup

- `GamePolicy`: `weapon_reload_ap` (type name -> AP), read in `DefaultGamePolicy.cc`
  with checks (0-255 each); defaults = the table above. `game.json` (repo and
  `E:\Release1\externalized`) with a comment; the schema `game.schema.yaml`.
- `WeaponModel`: optional `ubAPCostPerReload` (schema `weapons.schema.yaml`).
- `WeaponReloadAPs(usItem)`: own value, else the table (`HEAVY_WEAPON` per R3); the
  load check of R4.
- Unit tests: each type gives its table value; a weapon with `ubAPCostPerReload` gives
  its own; heavy weapons and launchers give `HEAVY_WEAPON`; a missing type stops the
  load (R4).

### Stage 2 - the game uses it

- `GetAPsToReloadGunWithAmmo`: `WeaponReloadAPs` (doubled per R5 for another magazine
  size); so `ReloadGun`, the AI and `GetAPsToAutoReload` follow.
- The description box's reload line calls the same function.
- R6: the attachment cost renamed, value unchanged.
- Unit tests: reload of a pistol 2, an assault rifle 8, the same rifle with a magazine
  of another size 16, a weapon with its own value; two guns add up on the cursor.
- Game test: the reload cost on the cursor and in the description box for a pistol,
  SMG, rifle, sniper rifle, LMG, shotgun and a rocket rifle; AP taken by a reload; a
  weapon with `ubAPCostPerReload`; the enemies reload.
