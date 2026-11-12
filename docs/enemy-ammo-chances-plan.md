# Enemy ammo by chance table - plan

Status: decisions A1-A9 decided 2026-11-12 (all as recommended; A2 as an interim rule,
see below); plan accepted 2026-11-12. Implemented on the Weapons branch.

| Stage | Commit | State |
|---|---|---|
| 1 - chance table in game.json, new magazine choice | c5012c0db | unit tests (no game effect yet) |
| 2 - enemies and militia use it, old system removed | this commit | game-verified |

Enemy ammo by chance table COMPLETE (2026-11-12).

Replace the current choice of the ammo type for an enemy's gun by a plain chance table:
the row is chosen by the soldier's progress, the columns are the four standard ammo
types. Shotgun and rocket magazines get fixed shares.

## 1. Current state (code facts)

- `ChooseWeaponForSoldierCreateStruct` (`src/game/Tactical/Inventory_Choosing.cc`)
  picks the gun, then the spare magazines with `RandomMagazine(gun, chance)`
  (`src/game/Tactical/Items.cc`):
  - the magazines that fit the gun: same calibre and capacity (`WeaponModel::matches`),
    in `magazines.json` order;
  - with `Random(100) < chance` the first of them (the "standard" one), else one of the
    others, all equally likely;
  - `chance` = 80 when the gun's default magazine is AP / SUPER_AP, else
    100 - 9 x weapon class; for guns pre-set on the map (negative class)
    100 - 9 x |class|.
- Two call sites: generated guns, and guns pre-set on the map that get ammo only.
  Enemies and militia both come through `GenerateRandomEquipment`.
- All spare magazines of one soldier are of the one chosen type; their number depends
  on the soldier class (admin 1-2, army 2-3, elite 3-4).
- The gun itself is created loaded with its default magazine (`CreateItem` ->
  `DefaultMagazine`), whatever the spare magazines are.
- The soldier's equipment progress P (`RollEnemyEquipmentProgress`: game progress +
  offset of the soldier's level or type + difficulty offset, 0-100) already chooses the
  gun by `minEnemyProgress`. It is -1 when `enemy_equipment.by_progress` is false and
  for civilians.
- Not part of this: the Elite Heavy's backup Minimi gets its default magazine
  (`EquipEliteHeavyMilitia`), grenades, mortar shells and LAWs are no magazines.

## 2. Decisions

- **A1 - progress of the row.** The soldier's equipment progress P (the same P that
  chooses his gun; it already includes his class and the difficulty). When P is not
  rolled (-1), the game progress percentage is used.
- **A2 - a type of the table missing for the calibre.** For now (later every standard
  gun will have all four types): the chance of a missing type goes to the nearest
  weaker type that exists (SUPER_AP -> AP -> HP -> REGULAR), and if there is none, to
  the nearest stronger one.
- **A3 - shotguns.** When a magazine of type BUCKSHOT fits the gun: 50 % BUCKSHOT,
  50 % the others (equally). AMMO_FLECHETTE counts as buckshot (no flechette magazine
  yet; it comes later). In the data: 12G slug / buckshot 50/50, CAWS SUPER_AP /
  flechette (typed BUCKSHOT) 50/50.
- **A4 - rockets.** When an HE or HEAT magazine fits: HE 70 %, HEAT 30 %, the other
  magazines of the calibre (`CLIPROCKET_AP`, SUPER_AP) 0 %. With only one of HE / HEAT,
  that one.
- **A5 - the loaded magazine.** The gun is loaded with the chosen magazine too.
- **A6 - one roll per soldier.** All his magazines (loaded and spare) are of one type.
- **A7 - where.** Everywhere `RandomMagazine` is used now: enemies, militia, guns pre-set
  on the map.
- **A8 - old system.** Removed (`RandomMagazine`'s standard chance, the weapon-class
  formula, the 80 % AP rule); no switch.
- **A9 - data.** The table and the two shares are in `game.json`.

## 3. The table (game.json)

```
"enemy_ammo": {
  // progress 0-9, 10-19, ..., 80-89, 90-100: REGULAR, HP, AP, SUPER_AP (each row 100)
  "type_chances": [
    [80, 20,  0,  0],
    [40, 60,  0,  0],
    [20, 80,  0,  0],
    [10, 40, 50,  0],
    [ 0, 30, 70,  0],
    [ 0, 20, 80,  0],
    [ 0, 10, 80, 10],
    [ 0,  0, 70, 30],
    [ 0,  0, 50, 50],
    [ 0,  0, 30, 70]
  ],
  "buckshot_chance": 50,  // shotguns: buckshot / flechette, the rest to the others
  "he_chance": 70         // rockets: HE, the rest to HEAT
}
```

10 rows of 4 values; every row must add up to 100, both shares 0-100, else the game
stops with a data error naming the row.

## 4. The choice (one function)

`EnemyMagazineForGun(gun, progress)` in `Items.cc`, replacing `RandomMagazine`:

1. The magazines that fit the gun (same calibre and capacity, as now). None: no ammo.
   One: that one.
2. A BUCKSHOT (or FLECHETTE) magazine among them: `buckshot_chance` % one of the
   buckshot ones, else one of the others (A3).
3. An HE or HEAT magazine among them: `he_chance` % HE, else HEAT (A4).
4. Else the row of `type_chances` for the progress (`min(progress / 10, 9)`); types
   with no magazine pass their chance on (A2); roll the type; several magazines of the
   type (e.g. two 9 mm REGULAR of one capacity): equally likely.
5. Other types only (sleep darts, monster...): the first fitting magazine.

## 5. Stages

### Stage 1 - chance table and the new choice

- `GamePolicy`: `enemy_ammo_type_chances[10][4]`, `enemy_ammo_buckshot_chance`,
  `enemy_ammo_he_chance`; read in `DefaultGamePolicy.cc` with the checks of section 3;
  defaults = the table above. `assets/externalized/game.json` (and the user's
  `E:\Release1\externalized\game.json`): the `enemy_ammo` section with a comment.
- `EnemyMagazineForGun(gun, progress)` in `Items.cc` (section 4).
- Unit tests: row choice (0, 9, 10, 95, 100), A2 hand-over (.38 at progress 85 gives AP),
  12G 50/50, rockets never `CLIPROCKET_AP`, a gun with one magazine, the data checks.

### Stage 2 - enemies and militia use it, old system removed

- `ChooseWeaponForSoldierCreateStruct`: both call sites use
  `EnemyMagazineForGun(gun, P or game progress)`; the gun in the hand (generated or
  pre-set) is loaded with the chosen magazine (A5, A6).
- `RandomMagazine` and `ubChanceStandardAmmo` removed (A8).
- Unit test: a generated soldier's loaded and spare magazines are of one type.
- Game test (new game): enemies at low progress carry REGULAR / HP, AP later; shotgun
  enemies about half buckshot; the loaded magazine matches the spare ones; militia too.
