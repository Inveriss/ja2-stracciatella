# Plan: Skyrider always next to his helicopter (Drassen, Estoni, every sector)

Status: **implemented** (2026-10-28, stages 1–2 verified in the game; see
section 6). Written as a plan over Remote Control. Branch `Vehicles`.
Decisions S1–S8 taken.

## 1. Goal

Once Skyrider has been found, escorted to his helicopter and the helicopter
works for the player, his character always stands next to the helicopter
whenever it is landed at one of its two refuel sites (Drassen airport B13,
Estoni I6), so the player can talk to him there — placed by the code, not by
the sector's map file (no per-map placement needed in `maps.slf`).

## 2. Findings (code analysis)

- The landed helicopter at a refuel site is map scenery
  (`AddHelicopterToMaps()`, `Map_Screen_Helicopter.cc`); the sites are
  `g_refuel_site[]`: Drassen B13 gridno 9001, Estoni I6 gridno 13067.
- `HandleHelicopterOnGround()` (called on sector load, landing and take-off):
  - helicopter landed in the loaded sector **and** `iHelicopterVehicleId != -1`
    → Skyrider's profile sector is set to the loaded sector;
  - otherwise → his profile sector is cleared and his character, if in the
    sector and not on our team, is removed (`TacticalRemoveSoldier()`).
- The character itself is only put in by the usual NPC mechanisms:
  - `AddPlacementToWorld()` (`Soldier_Init_List.cc`) — only if the map has a
    placement with his profile, his profile sector is the loaded one, he is
    alive, not recruited / EPC, and `fUseProfileInsertionInfo` is off;
  - `AddProfilesUsingProfileInsertionData()` — only with
    `fUseProfileInsertionInfo` on (an earlier stored gridno).
- So today he only appears when the airport's map has his placement (expected
  in Drassen B13, unknown in Estoni I6) or with stored insertion data.
- The helicopter's soldier of stage 4e (off the world at the site's gridno)
  doesn't touch any of this.

## 3. Design

### 3.1 Condition (all of them)

- Skyrider found, escorted to the helicopter and the helicopter set up for the
  player: `fSkyRiderSetUp` and `iHelicopterVehicleId != -1` (the condition the
  game already uses in `HandleHelicopterOnGround()`).
- Skyrider alive (`bLife > 0`, `bMercStatus != MERC_IS_DEAD`), not recruited,
  not an active EPC.
- The helicopter not destroyed, landed (`!fHelicopterIsAirBorne`) at the refuel
  site of the loaded sector (`IsHelicopterOnGroundAtRefuelingSite()`), ground
  level (`gWorldSector.z == 0`).

### 3.2 Putting him in

- Checked in `UpdateHelicopterInTactical()` (every tactical frame, stage 4e),
  so it covers entering the sector, the helicopter landing while the sector is
  loaded, and loading a saved game — always **after** the map's own
  placements and `AddProfilesUsingProfileInsertionData()` (sector load,
  `StrategicMap.cc`), so there is never a second Skyrider.
- If the conditions hold and his character is not in the sector
  (`FindSoldierByProfileID(SKYRIDER)` missing or `!bInSector`):
  - create him on the civilian team from his profile, as
    `AddProfilesUsingProfileInsertionData()` does (`TacticalCreateSoldier()`,
    `CIV_TEAM`, profile `SKYRIDER`, the loaded sector), unless he exists;
  - his place: a free gridno next to the helicopter — searched from the
    site's gridno outwards (`FindGridNoFromSweetSpot()`), outside the area
    `AddHelicopterToMaps()` bumps mercs out of (±5 tiles around the site's
    gridno), so he never stands inside the scenery helicopter;
  - insertion `INSERTION_CODE_GRIDNO` with that gridno, `UpdateMercInSector()`;
  - orders `STATIONARY` (he stays by the helicopter), facing the helicopter.
- Talking to him then works as with any NPC (his profile's dialogue).

### 3.3 Taking him out

Unchanged: `HandleHelicopterOnGround()` already removes him when the
helicopter takes off or isn't landed there; leaving the sector saves / drops
him with the other civilians as now.

### 3.4 Every sector: Skyrider next to the hovering helicopter (added 2026-10-28)

In every other sector the helicopter only hovers (strategic code), but stage
4e shows it standing at the mercs' drop spot (its own body, heli.sti). From
the player's view Skyrider has landed there and waits in front of it, so his
character stands next to it there too.

- **Strategic code unchanged:** the helicopter stays "hovering"
  (`fHoveringHelicopter`, `fHelicopterIsAirBorne`); its hover time runs as now —
  after `TIME_DELAY_FOR_HOVER_WAIT` (10 minutes) he says he has been waiting a
  while (`HandleHeliHoverLong()`), after `TIME_DELAY_FOR_HOVER_WAIT_TOO_LONG`
  (20 minutes) he flies back to base (`HandleHeliHoverTooLong()`, those aboard
  are put out first when the sector is safe). For the player: Skyrider waits
  in front of the helicopter for those 20 minutes, then flies off.
- **Condition:** as 3.1 (Skyrider escorted, the helicopter set up, he alive and
  not on our team), but instead of "landed at the refuel site": the
  helicopter's soldier is in tactical with its own body — mode
  `HELI_TACTICAL_BODY` of `UpdateHelicopterInTactical()` (stage 4e).
- **Putting him in:** as 3.2 — created from his profile on the civilian team,
  at the nearest free spot next to the helicopter's body (searched from its
  gridno outwards, outside its JSD footprint), on the side of the map's
  centre, `STATIONARY`, facing the helicopter. His profile sector is set to
  the loaded sector while he stands there (as `HandleHelicopterOnGround()`
  does at the refuel sites) and cleared when he goes.
- **Taking him out:** together with the helicopter's body — when
  `UpdateHelicopterInTactical()` takes it out (it flies on, back to base, goes
  down, the sector changes): his character is removed
  (`TacticalRemoveSoldier()`, unless he is on our team) and his profile sector
  cleared, as `HandleHelicopterOnGround()` does at take-off.
- **Talking:** as with any NPC, with his current dialogue.

### 3.5 Later, outside this plan

- The animation of the landed helicopter with the engine running and the
  rotor blades turning (an animation stage of the project).
- New dialogue options for Skyrider (an NPC stage of the project).
- Enemies attacking our vehicles, the pilot and those aboard; defending them
  (a combat stage of the project, see S7).
- The original strategic "hovering" handling is not changed now.

## 4. Decisions (taken 2026-10-28, all as proposed)

| # | Question | Decision |
|---|---|---|
| S1 | He is already in the sector (the map's placement or stored insertion data) | leave him where he is (no second one, no moving) |
| S2 | Distance from the helicopter | the nearest free spot just outside the ±5 tile bump area, on the side of the map's centre |
| S3 | A sector with enemies / combat | put him in all the same (the helicopter doesn't land in hostile sectors anyway) |
| S4 | Before Skyrider is escorted (helicopter at Drassen, `iHelicopterVehicleId == -1`) | nothing changes: he stays in his starting sectors (B15, E14, D12, C16) / with the escort, as now |

For 3.4 (every sector), taken 2026-10-28:

| # | Question | Decision |
|---|---|---|
| S5 | A sector with enemies | **A)** the default rule stays as it is: Skyrider doesn't come down to a sector with enemies (no change to that code now). **B)** if he is already waiting in a sector and enemies attack it during his wait, he keeps to the standard wait — the rest of the 20 minutes (`HandleHeliHoverTooLong()`, unchanged) — and then flies off; the attack doesn't shorten or cancel it |
| S6 | His place relative to the mercs getting out of it (stage E1 of the helicopter passengers plan) | his spot is taken first, the mercs getting out find free spots around it |

Follows from S5 B, taken 2026-10-28:

| # | Question | Decision |
|---|---|---|
| S7 | Where is Skyrider's character during that fight? | he stays in front of the helicopter, nothing special is done for the fight now: he is handled as the civilian NPC he is under the current rules; the helicopter takes no damage in tactical (H5 of stage 4e) |

| S8 | The helicopter destroyed in tactical while he stands by it (added 2026-10-28) | his character dies too, his body stays by the wreck |

Later, outside this plan: enemies attacking our vehicles and those using them
is a separate project stage — the player will have to defend the pilot and
the helicopter in such a fight, otherwise the pilot, the helicopter or both
may be killed.

## 5. Stages

1. **Implementation** (3.1, 3.2) in `Map_Screen_Helicopter.cc`, next to
   `UpdateHelicopterInTactical()`. Size: small.
2. **Tests (in the game):** Drassen B13 with the helicopter landed — Skyrider
   next to it, talk to him; Estoni I6 the same (no map placement needed);
   helicopter taking off with the sector loaded — he disappears; landing
   again — he comes back; save / load in the sector; before the escort
   quest — nothing changes.
3. **Every sector** (3.4, S5–S7): the helicopter hovering over another
   sector — Skyrider in front of its body; after 10 minutes his "waiting a
   while" line, after 20 minutes he and the helicopter go; enemies attacking
   during the wait — the wait runs on to its end (S5 B), Skyrider as S7;
   save / load while he stands there. Size: small.

## 6. Implementation (from 2026-10-28, on the PC)

- **C1 from the code:** the Drassen B13 map places Skyrider — after his talk
  by the helicopter the game makes him "always appear at his map editor
  placement spot in the airport sector" (`Interface_Dialogue.cc`, quote 15);
  nothing in the code points to a placement in Estoni I6.
- **Stage 1 (3.1, 3.2, S1–S4):** `UpdateSkyriderInTactical()`
  (`Map_Screen_Helicopter.cc`, from `UpdateHelicopterInTactical()`, every
  tactical frame): with the helicopter landed at the loaded sector's refuel
  site (its tactical mode "scenery"), set up for the player, Skyrider alive,
  not recruited / escorted, and his character not in the sector, he is
  created on the civilian team from his profile (as
  `AddProfilesUsingProfileInsertionData()`) and put in at the nearest free
  spot from a point 6 tiles from the site's gridno towards the map's centre
  (S2), `STATIONARY`, facing the helicopter. One already there stays (S1) —
  unless he stands more than 10 tiles from it: the game keeps the spot he had
  in the last sector in his profile's insertion data (`Tactical_Save.cc`,
  traversal) and puts him there, e.g. Drassen's spot somewhere on the Estoni
  map; then he is moved next to the helicopter, once per landing / new world.
  Taking off, `HandleHelicopterOnGround()` takes him out, as before.
- **Why not in Estoni (2026-10-28 test):** the helicopter only lands there
  when the Estoni refuel site is available (`UpdateRefuelSiteAvailability()`:
  `FACT_ESTONI_REFUELLING_POSSIBLE`, no enemy ground / air control);
  otherwise it hovers over the sector (its own body) — stage 2.
- **Stage 2 (3.4, S5–S8):** `UpdateSkyriderInTactical()` also with the
  helicopter hovering with its body: the nearest free spot by it
  (`FindGridNoFromSweetSpotWithStructDataFromSoldier()` around its soldier,
  radius 5, else 15), facing it, put in with it so the mercs getting out find
  spots around him (S6); his profile's sector is the loaded one while he
  stands there. When its body leaves (`UpdateHelicopterInTactical()`: flies
  on, back to base, goes down) `TakeSkyriderAwayFromHelicopter()` removes his
  character (alive, not on our team) and clears his profile's sector; on a
  new world (`ResetHelicopterInTactical()`) only the profile's sector. The
  strategic hover handling is unchanged (S5): he waits the usual 20 minutes
  also when enemies come (S5 B), an ordinary civilian then (S7).
- **S8 (decided 2026-10-28, as proposed):** the helicopter destroyed in
  tactical (stage 4f) — his character standing by it dies too
  (`HelicopterDestroyedInTactical()`), his body stays by the wreck.

## 7. Checks on the PC

- **C1:** does the Drassen B13 map already place Skyrider (decision S1 keeps
  that one)? Does Estoni I6?
