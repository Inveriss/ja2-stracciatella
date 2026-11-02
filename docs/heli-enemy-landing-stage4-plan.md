# Helicopter: landing in enemy sectors, stage 4: plan

Status: **plan only, nothing implemented** (2026-10-31, branch `Vehicle_Inventories`).

Stages 1-3 are already in place:
- **Stage 1:** the "Allow landing in enemy sectors" checkbox and the "Choose Landing Zone" box.
- **Stage 2:** the Map Edges rope drop.
- **Stage 3:** Anywhere, with the landing circle.

This stage adds five things to that landing:
- **I.** Skyrider's quote.
- **II.** Immediate departure.
- **III.** The landing surcharge.
- **IV.** No helicopter graphic in the sector.
- **V.** The damage risk.

Everything below applies only to an **enemy landing**. An enemy landing means all of these hold at once:
- the checkbox is ticked;
- the destination has enemies;
- the helicopter is not returning to base;
- there are passengers aboard.

This is the `enemy_landing` condition in `HandleHeliEnteringSector()`, `src/game/Strategic/Map_Screen_Helicopter.cc`. Normal flights do not change.

---

## I. Skyrider's quote when the order is confirmed

### What the code does now

`HandleNewDestConfirmation()` (`src/game/Strategic/MapScreen.cc`, about line 8966) runs when a helicopter route is confirmed:
- If the destination has no enemies, or the player knows nothing about them, Skyrider says one of three lines:
  - `CONFIRM_DESTINATION` (27): "That's a big roger, Cap.";
  - `THINGS_ARE_GOING_WELL` (47);
  - `THINGS_ARE_GOING_BADLY` (46).
- If the player knows there are enemies there, he says `BELIEVED_ENEMY_SECTOR` (30): "Just keep in mind, Cap, I don't put my eggbeater down in hostile territory."

**The code has no quote that plays when a route is plotted into a red, SAM-controlled zone.** Red airspace only raises the cost: `COST_AIRSPACE_UNSAFE`, $1,000 per sector.

The "high risk, higher fee" line is Skyrider's **quote 21**, `SECOND_HALF_OF_MENTION_DRASSEN_SAM_SITE`. I checked it in `Npcdata.slf/097.EDT`:

> "Makes the risk higher, Cap. High risk, big fee. I know she's got a SAM site to the east of Drassen. Takin' it out would lower your costs and… well… help keep my pretty-boy butt intact."

It plays only once, in his monologue about the Drassen SAM site: quote 20 followed by quote 21 (`HandleSkyRiderMonologueAboutDrassenSAMSite`). That monologue usually comes right after the first flight orders, which is most likely why it sounds as if it belongs to the red zone.

### Plan

1. Add a helper to `Map_Screen_Helicopter.cc/.h` that answers: would Skyrider land among the enemies there? It returns true when:
   - the checkbox `HELI_INV_CHECK_ENEMY_LANDING` is ticked;
   - the helicopter has passengers (`GetNumberInVehicle(GetHelicopter()) > 0`);
   - the destination has enemies the player knows about (`NumEnemiesInSector(sector) > 0`, and `WhatPlayerKnowsAboutEnemiesInSector` is not `KNOWS_NOTHING`).

   Suggested declaration: `bool WouldHelicopterLandAmongEnemies(SGPSector const& dest);`.
2. In `HandleNewDestConfirmation()`, the branch that now says `BELIEVED_ENEMY_SECTOR` becomes:
   - if the helper returns true, he says `SECOND_HALF_OF_MENTION_DRASSEN_SAM_SITE` (21);
   - otherwise he still says 30.

   Quote 30 ("I don't put my eggbeater down…") would contradict the landing, so it must not play here.
3. Use the existing `SkyRiderTalk()`. No new strings or sound files are needed, because quote 21 already has its speech file.

### Decision for the user

**D-I:** Quote 21 mentions the Drassen SAM site ("…to the east of Drassen…"), which sounds odd once that site has been captured, or when flying elsewhere.
- **(A, recommended)** Play quote 21 as it is.
- **(B)** Play quote 20 followed by 21.
- **(C)** Have it play only while the Drassen SAM site is still enemy-held, and use quote 30 otherwise.

---

## II. Immediate departure after the rope drop

### What the code does now

Arriving at the destination calls `StartHoverTime()`. After that:
- after 10 minutes he says `HOVERING_A_WHILE` (32): "Gotta make it snappy, Cap. Fuel's a-burnin'.";
- after 20 minutes `HandleHeliHoverTooLong()` makes him say `RETURN_TO_BASE` (33) and runs `MakeHeliReturnToBase()`.

`MakeHeliReturnToBase()` already picks the base: `NearestRefuelPoint(true)` → `FindClosestRefuelSite()`. That is the nearest **available** refuel site by strategic path length, so Drassen, or Estoni when `fRefuelingSiteAvailable[ESTONI_REFUELING_SITE]` is set and it is closer. This is exactly the requested rule, so nothing in the choice needs to change.

### Plan

1. **Hook: the end of the rope animation.**
   - `Merc_Entering.cc` ends a run in three places: after `PutHeliDropMercInSector`, and in the two fade-out branches. Each of them calls `HandleFirstHeliDropOfGame()`.
   - Next to that call, add a call to a new function, `HelicopterRunFinished()`, in `Map_Screen_Helicopter.cc`.
   - Alternatively, `Merc_Entering` could keep a callback pointer. The direct call is simpler; `Merc_Entering.cc` already includes the helicopter headers.
2. **Mark the run as an enemy-landing drop.** Add `static bool gfHeliEnemyDropRunning` in `Map_Screen_Helicopter.cc`:
   - set it when `HandleHeliMapEdgesDrop()` or `LandHelicopterPassengersAt()` starts the run;
   - clear it in `HelicopterRunFinished()`, and also in `ResetHelicopterInTactical()` and when leaving tactical, as stage 3's selection mode does.

   A normal heli run, the one at the start of the game, does not touch it.
3. **`HelicopterRunFinished()`:** if the flag is set, it does the following, in this order:
   1. Rolls the damage (V) and plays the damage quote if needed.
   2. Clears the enemy-landing state (`giHeliEnemyLandingSector = -1`) and saves it with the game, as before.
   3. Deletes the pending hover events: `DeleteAllStrategicEventsOfType(EVENT_HELICOPTER_HOVER_TOO_LONG)` and `…_WAY_TOO_LONG`. Both are existing functions in `Game_Event_Hook.h`.
      - This matters because `HandleHeliHoverLong()` does not compare times.
      - Without it, an event left from this hover could make him say "Gotta make it snappy" at once during a later hover elsewhere.
   4. Calls `MakeHeliReturnToBase()`.
      - It sets `fHoveringHelicopter = FALSE` and `fHeliReturnStraightToBase = TRUE`, and builds the route.
      - It does **not** play quote 33 ("…before I run out of fuel"). That quote belongs to `HandleHeliHoverTooLong()`, which is skipped here.
      - It stops time compression.
4. **Unchanged:**
   - If the player stays on the map screen and never enters tactical, nobody goes down the rope, and the normal 10/20 minute rule still applies. He leaves only after a drop has happened.
   - The way home is a normal return to base: SAM fire on the route still applies, and there is no charge (`fHeliReturnStraightToBase`).
   - Landing at the base works as now: `LandHelicopter()`, and Waldo repairs in Drassen.

### Decision for the user

**D-II:** None. The departure is silent unless there is damage, in which case the damage quote from V plays.

---

## III. The landing surcharge

### What the code does now

The tactical and strategic panels already show the prices:
- "Land at map edges: +$1,000";
- "Land anywhere: +$2,500".

Nothing charges them yet.

`HandleHeliEnteringSector()` currently allows an enemy landing **only when the player can pay the whole trip**: `LaptopSaveInfo.iCurrentBalance >= iTotalAccumulatedCostByPlayer`. Without the money he refuses. The reason is that `PaySkyriderBill()`, when it lacks the money, puts everyone out on foot (`MoveAllInHelicopterToFootMovementGroup`) and sends the helicopter home. Among enemies, that would leave the squad standing in the middle of them.

The default debt rules:
- A missing amount goes into `gMercProfiles[SKYRIDER].iBalance` as a negative value.
- Skyrider then says `OWED_MONEY_TO_SKYRIDER` (11): "You're grounded, Cap, 'til I see my money."
- The message "Skyrider is still owed $%d" (`pSkyriderText[1]`) appears.
- While `iBalance < 0`, `IsHelicopterPilotAvailable()` returns false, so the helicopter is grounded.
- `PayOffSkyriderDebtIfAny()` (`Hourly_Update.cc`) pays the debt off every hour from the player's balance.

### Plan

1. Add two constants to `Map_Screen_Helicopter.h`, next to `COST_AIRSPACE_*`:
   ```cpp
   #define COST_HELI_LANDING_MAP_EDGES 1000
   #define COST_HELI_LANDING_ANYWHERE  2500
   ```
   The panel texts keep their current strings. They could later be formatted from these constants, but that is not part of this stage.
2. Add a new `static void PayHeliLandingSurcharge(INT32 cost)` in `Map_Screen_Helicopter.cc`. It follows `PaySkyriderBill()`, **without putting anyone out or sending him home**:
   - **The player can pay:** `AddTransactionToPlayersBook(PAYMENT_TO_NPC, SKYRIDER, …, -cost)` and `ScreenMsg(pSkyriderText[0], cost)`, "Skyrider was paid $%d".
   - **The player cannot pay:**
     - pay whatever there is, if the balance is above 0 (transaction plus `pSkyriderText[0]`);
     - add the rest to the debt: `gMercProfiles[SKYRIDER].iBalance -= rest`;
     - play quote `OWED_MONEY_TO_SKYRIDER` and show `pSkyriderText[1]`.
     - Use `-=` on the balance rather than `=`, so that an existing debt is not wiped.
   - The landing goes ahead either way.
3. In `HeliLandingZoneCallback()`, the moment the player picks Map Edges or Anywhere, charge the matching surcharge before setting `gHeliLandingZone`.
   - The box has only those two buttons, so the charge always happens exactly once.
4. **What a debt changes:**
   - After the drop he flies home (II). `MakeHeliReturnToBase()` does not check `CanHelicopterFly()`, so a debt does not stop him returning.
   - At the base, `LandHelicopter()` finds `!CanHelicopterFly()` and puts out anyone aboard. Nobody is aboard by then.
   - The helicopter stays grounded until the debt is paid, which is the default rule.

### Decision for the user

**D-III:** What happens when the **trip itself** (the sectors flown) cannot be paid for.
- **(A, recommended)** Keep the current rule: without the money for the trip, he refuses to land among enemies and says quote 29, as now. The debt rule applies only to the surcharge, as written in the request.
- **(B)** For an enemy landing, the unpaid part of the trip also goes into the debt, without putting anyone out. This needs a variant of `PaySkyriderBill()`, and the balance check in `enemy_landing` would be removed.

---

## IV. Helicopter graphic in the sector: check and fix

### Check (current code): the helicopter **does** appear

`WantedHelicopterInTactical()` returns `HELI_TACTICAL_BODY` whenever the helicopter hovers over the loaded sector (`fHelicopterIsAirBorne && fHoveringHelicopter`), and that includes a sector with enemies. As a result:
- `PutHelicopterInTactical()` places the helicopter body at the map's drop spot: the centre entry point, or else another entry point.
- `UpdateSkyriderInTactical()` places **Skyrider next to it**. This happens even with enemies in the sector, by design: points S3 and S7 of the Skyrider plan.

What the player sees in each mode:
- **Map Edges:** while the Omerta helicopter flies in at the map edge, a second, hovering helicopter and Skyrider stand in the middle of the map. Enemies can see Skyrider.
- **Anywhere:** both are hidden only during the landing selection, because the renderer skips every character there. They show up during the rope animation and stay after it.
- **After the departure in II:** `WantedHelicopterInTactical()` becomes NONE (`fHoveringHelicopter = FALSE`). The body and Skyrider then go (`TakeSkyriderAwayFromHelicopter`, `TakeHelicopterOffTactical`), but by then they have already been on screen.

So they must **not appear at all** in an enemy landing.

### Plan

1. **New mode.** Add `HELI_TACTICAL_ROPE` to the `HeliTacticalMode` enum. It means: the helicopter hovers over the enemy-landing sector, the only helicopter on screen is the rope-drop animation, and its soldier has no body on the map.
2. **When it is used.** In `WantedHelicopterInTactical()`, before the line that returns `HELI_TACTICAL_BODY`: if `giHeliEnemyLandingSector == gWorldSector.AsByte()`, return `HELI_TACTICAL_ROPE`.
3. **Placing the soldier.** In `PutHelicopterInTactical()`, the `ROPE` mode works like `SCENERY`:
   - it sets the soldier's position (`sGridNo` = `HelicopterDropSpot()`, its X/Y, `bInSector = TRUE`);
   - it does not call `AddSoldierToSector()`, so there is no body, no structure and no sight lines.

   The passengers in `UpdateHelicopterPassengersInTactical()` keep using that position, as they do now.
4. **No Skyrider.** In `UpdateSkyriderInTactical()`, `ROPE` is treated like `NONE` for putting him in. Simplest: return early when `g_heli_tactical == HELI_TACTICAL_ROPE`.
5. **Triggers.** `HandleHeliMapEdgesDrop()` and `HandleHeliAnywhereSelection()` now check `g_heli_tactical != HELI_TACTICAL_BODY`. Change both checks to `HELI_TACTICAL_ROPE`.
6. **Passenger view.** Check `UpdateHelicopterPassengersView()` and the team list with the new mode, since both already handle `SCENERY`. In ROPE mode, the passengers shown in the helicopter panel are the expected behaviour, up to the drop.
7. **After the drop**, II clears `giHeliEnemyLandingSector`, and `fHoveringHelicopter` becomes false, so the mode becomes `NONE`. Nothing shows up for a moment either.
8. **Stage 3 render skip.** The skip in `RenderWorld.cc` (hide characters during the Anywhere selection) stays. It still hides the enemies and civilians.

### Decision for the user

**D-IV:** None.

---

## V. Landing damage and Skyrider's damage quote

### What the code does now

A SAM hit costs `HELI_SAM_HIT_DAMAGE` (25) from the helicopter soldier's `bLife`, which is the helicopter's condition. If there is no helicopter soldier, the condition is counted from `gubHelicopterHitsTaken`. The quote then depends on the condition left:

| Condition left | Quote | What happens |
| --- | --- | --- |
| 50 or more | `HELI_TOOK_MINOR_DAMAGE` (52): "Looks like the bird's been wounded, Cap. Think I'll be all right, but I'll need to have her checked out when I get back to the base." | Nothing else. |
| 25 to 49 | `HELI_TOOK_MAJOR_DAMAGE` (53): "I got some serious trouble here, Cap. Been hit good. Have to hightail it back to the base." | He returns to base. |
| Below 25 | `HELI_GOING_DOWN` (54) | Crash. |

The panel already shows "Risk of heli damage: 25%".

### Plan

1. Add two constants in `Map_Screen_Helicopter.cc`, next to `HELI_SAM_HIT_DAMAGE`:
   ```cpp
   #define HELI_ENEMY_LANDING_DAMAGE_CHANCE 25 // percent
   #define HELI_ENEMY_LANDING_DAMAGE        10 // condition points
   ```
2. **When:** in `HelicopterRunFinished()` (II), before `MakeHeliReturnToBase()`. The landing has just happened, and he says the quote as he leaves, as requested.
3. **The roll:** `if (PreRandom(100) < HELI_ENEMY_LANDING_DAMAGE_CHANCE)`. If it hits, take 10 points off the helicopter soldier's `bLife`, and mark the team and character panels dirty, the same way as the SAM code.
   - With no helicopter soldier, add the damage to the fallback count. Today `gubHelicopterHitsTaken` counts whole 25-point hits, so 10 points does not fit it. Option: add a separate `gubHelicopterLandingDamage` value, saved in g_gameStates, to that formula. Check whether that case still happens; it is the "no free team slot" case. If it does not, skip the damage there.
4. **The quote**, by the condition left:
   - 50 or more: quote 52, "…checked out when I get back to the base". This fits a departure exactly.
   - Below 50: quote 53, "…have to hightail it back to the base". He is flying back anyway.
5. **Order:** the quote first, then `MakeHeliReturnToBase()`. Since the quote 33 path is skipped (II), the damage quote is the only line he says on departure. With no damage, he leaves silently.

### Decision for the user

**D-V: settled, no decision needed.** Checked in the code (2026-10-31): the landing damage cannot cause a crash.
- **Take-off:** a flight needs `CanHelicopterFly()`, which requires a condition of at least 50 (`HELI_FLIGHT_MIN_CONDITION`).
- **On the way:** the only thing that lowers the condition in flight is a SAM hit (`HandleSAMSiteAttackOfHelicopterInSector`). A hit that leaves 25 to 49 calls `MakeHeliReturnToBase()`, which sets `fHeliReturnStraightToBase`.
  - This also applies to a hit in the destination sector itself, because the SAM check runs before the landing decision.
  - `enemy_landing` requires `!fHeliReturnStraightToBase`, so he never lands among enemies with less than 50.
- **Result:** the lowest condition at an enemy landing is 50. After the 10-point damage it is 40, which is quote 53 and never below 25.
- **Hardening during implementation:** add an explicit `!IsHelicopterTooDamagedToFly()` to `enemy_landing`, so that the rule does not depend on the SAM code alone.
- **Tactical gunfire:** in the ROPE mode from IV the helicopter has no body on the map, so enemies cannot shoot it in tactical either.

---

## Files touched (expected)

| File | Change |
| --- | --- |
| `src/game/Strategic/Map_Screen_Helicopter.cc` | <ul><li>`WouldHelicopterLandAmongEnemies`;</li><li>`PayHeliLandingSurcharge` plus the `HeliLandingZoneCallback` charge;</li><li>`gfHeliEnemyDropRunning`;</li><li>`HelicopterRunFinished` (damage, quote, events, return);</li><li>`HELI_TACTICAL_ROPE` (Wanted / Put / Skyrider / triggers);</li><li>constants.</li></ul> |
| `src/game/Strategic/Map_Screen_Helicopter.h` | <ul><li>`COST_HELI_LANDING_*`;</li><li>`WouldHelicopterLandAmongEnemies`;</li><li>`HelicopterRunFinished`.</li></ul> |
| `src/game/Strategic/MapScreen.cc` | `HandleNewDestConfirmation`: quote 21 instead of 30 for an enemy landing. |
| `src/game/Tactical/Merc_Entering.cc` | Call `HelicopterRunFinished()` at the three ends of a run. |

There are **no new strings** and **no new sound files**: quotes 11, 21, 52 and 53 and the `pSkyriderText` strings all exist already. The save format does not change, except possibly for the fallback damage value in V.3, which would be a new g_gameStates key. Under the project rule, old saves are not supported anyway.

## Suggested order and tests

1. **IV:** the ROPE mode. Test Map Edges and Anywhere: no second helicopter and no Skyrider, and the passengers still go down the rope.
2. **II:** departure after the run. On the strategic map, he heads to Drassen, or to Estoni if it is available and closer. Check that no "Gotta make it snappy" plays later.
3. **III:** the surcharge.
   - With enough money: $1,000 or $2,500 appears in the finances.
   - Without: a debt, quote 11, and the landing still happens. The helicopter stays grounded until the hourly payoff clears the debt.
4. **V:** the damage. Temporarily set the chance to 100: −10 condition and quote 52 or 53 at departure.
5. **I:** the quote. Plot a route into a known enemy sector, with the checkbox ticked and passengers aboard → quote 21. Without passengers, or with the checkbox unticked → quote 30.
