# Enemy Elite Heavy soldiers (Weapons)

Status: plan only (2026-11-09, written in Remote Control mode), H1-H8, N1-N10 and O1-O2 decided
by the user (2026-11-09), not implemented.
Branch: `Weapons` (last commit 4463fec38).
Related: Elite Heavy militia (docs of the Militia branch, `EquipEliteHeavyMilitia` in
`Inventory_Choosing.cc:583`), [enemy-equipment-progress-plan.md](enemy-equipment-progress-plan.md),
[enemy-drop-progress-plan.md](enemy-drop-progress-plan.md).

## 1. Goal

A fourth enemy type next to Admins, Army and Elite: the **Elite Heavy** enemy, the counterpart of
the Elite Heavy militia. He carries a fixed heavy kit (LAWs or a mortar), a FN Minimi as the
backup weapon and fixed Spectra armour.

## 2. Decisions

| # | Topic | Decision |
|---|---|---|
| H1 | Approach | a **subtype of Elite**: the strategic AI still counts only admins/troops/elites (`ubNumElites`); some elites become Elite Heavy when the soldiers are created (tactical and autoresolve). No strategic or save-structure change for the counts. |
| H2 | Count | from the number of elites in the sector, no progress threshold, difficulty modifier (section 3) |
| H3 | Heavy weapon | fixed: **3 x LAW** or **1 mortar + 3 mortar shells**; the k-th Elite Heavy of the sector: odd k LAW, even k mortar (LAW, mortar, LAW, mortar, ...) |
| H4 | Armour | fixed: Spectra helmet, Spectra vest with ceramic plates, Spectra leggings, gas mask, UV goggles |
| H5 | Backup weapon | FN Minimi (`MINIMI`) + 2 magazines; used only once the heavy weapon is spent (all 3 LAWs fired, or all 3 shells fired) - section 6 |
| H6 | Regular elites | unchanged, including their random special weapons (`ChooseSpecialWeapons…`, the 1-mortar-per-team roll) |
| H7 | Uniform | grey vest + black trousers (`greyVEST` + `BLACKPANTS`) |
| H8 | Stats, traits | as the Elite Heavy militia: always BIGMALE, experience base 8, strength 100, agility 94, traits Heavy Weapons + Night Ops |
| N1 | Difficulty levels | the game has 3: Novice (`DIF_LEVEL_EASY`), Experienced (`DIF_LEVEL_MEDIUM`), Expert (`DIF_LEVEL_HARD`) - modifier -1 / 0 / +1 |
| N2 | Formula | section 3 table; never below 0, the modifier counts only from 5 elites on |
| N3 | Drops | `drop_everything` (game.json, on): everything he carries can drop, limited only by each item's `minDropProgress` / `"blocked"` (`Rotting_Corpses.cc:789`) - no special code |
| N4 | Underground | every Elite Heavy gets 3 x LAW (no mortar underground) |
| N5 | Mortar limit | not concerned: the fixed kit never goes through the random special-weapon roll (`MAX_MORTARS_PER_TEAM`) |
| N6 | Other AI | unchanged; the only AI change is section 6 |
| N7 | Elite Heavy militia AI rules | apply to the enemy too: no item pickup while the heavy weapon can fire; LAWs/shells not saved for groups of targets |
| N8 | Reinforcements | the count is taken from all elites in the sector (present + arriving); arrivals convert only the missing difference |
| N9 | Settings | `game.json` |
| N10 | Autoresolve | the same rules: Elite Heavy enemies are created in autoresolve too and fire their heavy weapon there |

## 3. Count (H2, N1, N2, N8)

`E` = elites in the sector (regular elites + Elite Heavy, already present and arriving),
`m` = difficulty modifier (Novice -1, Experienced 0, Expert +1).

```
heavies(E) = 0                                   if E < elites_per_heavy (5)
heavies(E) = clamp(E / 5 + m, 0, E)              otherwise (integer division)
```

| Elites in the sector | Novice | Experienced | Expert |
|---|---|---|---|
| 0-4 | 0 | 0 | 0 |
| 5-9 | 0 | 1 | 2 |
| 10-14 | 1 | 2 | 3 |
| 15-19 | 2 | 3 | 4 |
| 20-24 | 3 | 4 | 5 |

`game.json`:

```json
//  Elite Heavy enemies (docs/enemy-elite-heavy-plan.md): one elite in every
//  "elites_per_heavy" becomes an Elite Heavy (LAWs or a mortar, a Minimi, Spectra armour);
//  "difficulty_modifiers" adds to that count once the sector has at least "elites_per_heavy"
//  elites; never below 0, never more than the elites.
//  vanilla setting: enabled false (new)
"enemy_elite_heavy": {
  "enabled": true,
  "elites_per_heavy": 5,
  "difficulty_modifiers": { "easy": -1, "experienced": 0, "expert": 1 }
},
```

## 4. Where the conversion happens

One decision function, e.g. `SoldierClass ChooseEnemyEliteClass(UINT8 elitesSoFar, UINT8 heaviesSoFar)`
in `Soldier_Create.cc`: it is called for every enemy elite about to be created and returns
`SOLDIER_CLASS_ELITE_HEAVY` while `heaviesSoFar < heavies(elitesSoFar + 1)`, else
`SOLDIER_CLASS_ELITE`. It also returns which kit the new heavy gets (index k = heaviesSoFar + 1, H3/N4).

Creation paths of enemy elites:

1. **Sector defenders** - `AddSoldierInitListEnemyDefenceSoldiers` (`Soldier_Init_List.cc:623`):
   the elite placements filled there are converted by the running count (start: 0/0). Editor
   placements with a fixed inventory or a profile (detailed placements with
   `OBJECT_NO_OVERWRITE` items, Mike/Iggy) are not converted.
2. **Arriving groups / reinforcements** - `AddEnemiesToBattle` (`Queen_Command.cc:1060`): the
   running count starts with the elites and Elite Heavy already on `ENEMY_TEAM` in the sector
   (alive), so arrivals add only the difference (N8).
3. **Autoresolve** - `MakeEnemyTroops` (`Auto_Resolve.cc:1929`, called with `ar->ubElites`):
   the running count over the autoresolve elites (N10).
4. Test cheat ALT+Q (`Handle_UI.cc:1019`) stays a plain elite; a new cheat key for an Elite
   Heavy (proposal: ALT+H, if free) to test the kit without a big sector.

## 5. The new soldier class

- `SoldierClass` (`Soldier_Control.h:297`): append `SOLDIER_CLASS_ELITE_HEAVY` **after**
  `SOLDIER_CLASS_ELITE_HEAVY_MILITIA` (stored values must not move).
- `SOLDIER_CLASS_ENEMY(...)` (`:313`) is a range ADMIN..ARMY: add `|| == SOLDIER_CLASS_ELITE_HEAVY`.
- Every place that treats `SOLDIER_CLASS_ELITE` as "an elite of the strategic count" must also take
  the heavy one, so that his death, retreat or capture decrements `ubNumElites`:
  `Queen_Command.cc` (325, 397, 548, 649, 789, 884), `Enemy_Soldier_Save.cc` (237, 685),
  `Strategic_Status.cc:425` (kills by rank: counted as elite), `WorldDef.cc:2003`,
  `Auto_Resolve.cc:400` (rank), `OppList.cc:3145`, `AIUtils.cc:2089` (difficulty 4 as elite),
  `Civ_Quotes.cc:420`. A helper `IsEnemyElite(sc)` (elite or elite heavy) keeps this short.
- `Soldier_Create.cc`:
  - palette (`:750`): `greyVEST` + `BLACKPANTS` (H7);
  - body (`:1330`): always BIGMALE, also for the enemy team;
  - experience (`:1402`): base 8 like the heavy militia, strength 100, agility 94 (`:1490`),
    not cut by `ReduceHighExpLevels` (`:1498`);
  - traits (`:924`, `:953`): Heavy Weapons + Night Ops;
  - `GeneratePaletteForSoldier` switch (`:1700`): add the class.
- `GenerateRandomEquipment` (`Inventory_Choosing.cc:173`): the new class goes to
  `EquipEliteHeavyEnemy` (section 6), not to the random equipment.
- Labels: hover list `Interface.cc:1006` "Elite heavy", debug `OppList.cc:3163`
  "(Elite Heavy)", autoresolve portrait: the "Heavy" label of the militia cell
  (`DrawHeavyMilitiaLabel`, `Auto_Resolve.cc:1129`) on enemy cells too.
- Save format: the class value is stored with the soldiers -> new game needed (project rule,
  no compatibility code).

## 6. Kit and the Minimi rule (H3, H4, H5, N4, N6, N7)

`EquipEliteHeavyEnemy(pp, kitIndex, underground)` next to `EquipEliteHeavyMilitia`:

| Slot | LAW kit | Mortar kit |
|---|---|---|
| HELMETPOS | SPECTRA_HELMET | same |
| VESTPOS | SPECTRA_VEST + CERAMIC_PLATES | same |
| LEGPOS | SPECTRA_LEGGINGS | same |
| HEAD1POS / HEAD2POS | GASMASK / UVGOGGLES | same |
| HANDPOS | ROCKET_LAUNCHER (LAW) | MORTAR |
| big pockets | 2 x ROCKET_LAUNCHER, MINIMI (loaded) | 3 x MORTAR_SHELL (as many big pockets as the stack size needs), MINIMI (loaded) |
| small pockets | 2 Minimi magazines (its default magazine, today `CLIP556_30_AP`) | same |

- Fixed items ignore `minEnemyProgress` (it only steers the random choice).
- Drops: no `OBJECT_UNDROPPABLE` handling needed; with `drop_everything` the corpse leaves everything
  whose `minDropProgress` allows it (N3). Note: MINIMI has `minDropProgress` 100.
- Weight: ~kit + 3 LAWs + Minimi is far below the strength-100 limit (`MaxLoad`).

**AI (the only changes, N6):**

1. `HeavyMilitiaCanFireHeavyWeapon` (`DecideAction.cc:53`) becomes `EliteHeavyCanFireHeavyWeapon`:
   true for `SOLDIER_CLASS_ELITE_HEAVY_MILITIA` on the militia team **and** `SOLDIER_CLASS_ELITE_HEAVY`
   on the enemy team. Its two uses (item pickup `:2354`, gun search `:2767`) then cover the enemy (N7).
2. `fHeavyMilitia` in `Attacks.cc:417` (no saving LAWs/shells for groups of targets, the easy
   difficulty skip) is extended the same way (N7).
3. **Minimi rule (H5):** while `EliteHeavyCanFireHeavyWeapon` is true, the AI's gun search
   ignores every gun except the LAW: `FindAIUsableObjClass(…, IC_GUN)` in the shot decision
   (`DecideAction.cc:2794`) and the weapon choice in `Attacks.cc:1593-1602` get a heavy-aware variant
   (e.g. `FindAIUsableGunForSoldier`) that skips `MINIMI`/other guns for an Elite Heavy that still has
   a usable LAW or mortar shells. Once the heavy weapon is spent, the normal search finds the Minimi.
   No other exception (e.g. a target too close for the mortar) - N6.

Autoresolve: `IsHeavyMilitiaCell` (`Auto_Resolve.cc:1122`) -> `IsEliteHeavyCell` (both teams), so
`FireHeavyWeaponShot` (`:3448`) fires the enemy's LAWs/shells too; when they are spent the normal
gun shot uses the Minimi.

## 7. Tests

`Inventory_Choosing_unittest.cc` (or a new file):

- `heavies(E)` table of section 3 for all 3 difficulties, including 0-4 elites and the cap `<= E`;
- running count: defenders 12 elites Experienced -> 2 heavies; then 3 arriving elites (15) -> 1 more;
- kit order LAW, mortar, LAW, mortar; underground always LAW;
- `EquipEliteHeavyEnemy`: slots of section 6, 3 LAWs / mortar + 3 shells, Minimi loaded, 2 magazines;
- `IsEnemyElite` / `SOLDIER_CLASS_ENEMY` include the new class.

## 8. Game check (by the user)

1. New game, cheats on. Sector with 5-9 elites on Experienced: exactly 1 Elite Heavy (grey vest,
   black trousers, big male, hover label "Elite heavy"); with 10+: 2, the second one with a mortar.
2. Novice with 5-9 elites: none; Expert with 5-9: 2.
3. He fires all LAWs (or all shells) before he ever uses the Minimi; afterwards he uses the Minimi.
4. He does not pick up items while he can still fire the heavy weapon.
5. Underground sector: only LAWs.
6. Kill one: drops follow `minDropProgress`; the strategic elite count of the sector goes down by 1.
7. Reinforcements of elites into a sector raise the Elite Heavy count only by the difference.
8. Autoresolve of a sector with 10 elites: 2 Elite Heavy cells with the "Heavy" label, LAW/mortar
   explosions in the result.

## 9. Kit details (decided 2026-11-09)

| # | Question | Decision |
|---|---|---|
| O1 | Spectra plain or treated with Compound 18 (`SPECTRA_*_18`, as the heavy militia)? | plain `SPECTRA_*` |
| O2 | Condition of the fixed kit | 100% like the heavy militia (not the P-based condition of the random equipment) |

## 10. Stages

1. Class, count, conversion in the three creation paths, palette/body/stats/traits, labels,
   `game.json`, tests (sections 3-5, 7).
2. Kit (section 6) + AI changes (section 6, items 1-3) + autoresolve heavy shot.
3. Game check (section 8), commit on `Weapons` after the user's check, plan status.
