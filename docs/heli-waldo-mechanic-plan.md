# Helicopter: Waldo as the helicopter's mechanic: analysis and plan

Status: **plan only, nothing implemented** (2026-10-31, branch `Vehicle_Inventories`). Decisions D1–D7 settled (section 4).

This plan covers the request to make Waldo the helicopter's own mechanic. The helicopter panels already list his terms:

```
Waldo:            [check box]
  - auto-repairs:   in Drassen
  - repair rate:    3 HP/hour
  - repair cost:    $200/hour
```

These are his individual abilities. His stats (Mechanical, level, and so on) are **not** used.

---

## 1. Analysis: what exists today

### 1.1 Waldo in the code

| What | Where | State |
| --- | --- | --- |
| Profile `WALDO` | `Soldier_Profile.h:153`, `mercs-profile-info.json` | An NPC at the Drassen airport (B13). |
| `FACT_WALDO_ALIVE` (295) | `Quests.cc:955` | Computed from `gMercProfiles[WALDO].bMercStatus != MERC_IS_DEAD`. |
| Facts 126 "Helicopter is in working condition" and 129 "Waldo agreed to fix helicopter" | Names only, in `QuestText.cc` | **No code at all.** `Facts.h` has no constants for them, and `Quests.cc` never evaluates them. Waldo's original helicopter repair ran from the NPC script (`.npc`/`.EDT`) and did nothing to the helicopter's condition here. |
| `RefreshNPCScriptRecord(WALDO, …)` | `NPC.cc:2471` | An old save conversion of his dialogue records only. |

Conclusion: **nothing in the code repairs the helicopter through Waldo today.** The mechanic has to be written from scratch. It does not touch his dialogue or the NPC script.

### 1.2 The helicopter's condition and how it is repaired today

- The condition is `bLife` of the helicopter soldier, 0–100, with `bLifeMax = 100` (`Map_Screen_Helicopter.cc:854`).
- If there is no helicopter soldier (no free team slot), the condition is counted from `gubHelicopterHitsTaken`: 100 − 25 per hit.
- It drops only because of a SAM hit, 25 points (`HandleSAMSiteAttackOfHelicopterInSector`). The planned enemy landing in stage 4 would add 10 points.
- Flight requires a condition of at least 50 (`HELI_FLIGHT_MIN_CONDITION`, `IsHelicopterTooDamagedToFly()`). Below 50 the helicopter is grounded.
- **Today, only mercs on the Repair assignment can repair it.**
  - Each hour, `UpdateAssignments()` → `RepairVehicle()` (`Vehicles.cc:772`) adds repair points ÷ `repair_points_divisor` to `bLife`.
  - The merc must be in the same sector, and the helicopter must have landed (`IsVehicleInRepairReach`, `Assignments.cc:386`).
  - The panel shows this as "Mercenaries: mechanics only".
- After a major hit, the helicopter flies to the nearest refuel site, Drassen or Estoni (`MakeHeliReturnToBase` → `LandHelicopter`). It then stands on the ground there: `fHelicopterIsAirBorne == FALSE` and `IsHelicopterOnGroundAtRefuelingSite()`.

### 1.3 The checkbox next to "Waldo:"

- It is `HELI_INV_CHECK_WALDO` in `Interface_Panels.cc`. It is unchecked in a new game and saved in `g_gameStates` under "HeliInventory::waldo". Code reads it through `IsHeliInventoryChecked()`.
- **Today it only shows or hides Waldo's three lines** (`DrawHeliInventoryTexts`, lines 1061–1065). It has no effect on gameplay.

### 1.4 Hooks this needs

- **Hourly update:** `HandleHourlyUpdate()` (`Hourly_Update.cc:35`), which already calls `PayOffSkyriderDebtIfAny()`. Waldo's hourly tick fits next to it.
- **Payments:** `AddTransactionToPlayersBook(PAYMENT_TO_NPC, <profile>, GetWorldTotalMin(), -amount)`. The finances screen shows it as a payment to that NPC (`Finances.cc:808`), so with `WALDO` it reads as a payment to Waldo.
- **Drassen refuel site:** `g_refuel_site[DRASSEN_REFUELING_SITE]`, sector B13 (13,2). It is available while `fRefuelingSiteAvailable[DRASSEN_REFUELING_SITE]` is set, meaning the player holds the airport.

---

## 2. The mechanic (proposal)

### 2.1 When Waldo repairs

He repairs on **each full game hour** when **all** of these hold:

1. The **checkbox** next to "Waldo:" is checked (see decision D1).
2. The helicopter exists and is not destroyed (`iHelicopterVehicleId != -1`, `!fHelicopterDestroyed`).
3. It **stands on the ground at the Drassen airport**: `!fHelicopterIsAirBorne` and `IsHelicopterOnGroundAtRefuelingSite(g_refuel_site[DRASSEN_REFUELING_SITE])`.
4. It **needs repair**: `DoesVehicleNeedAnyRepairs(GetHelicopter())`.
5. **Waldo is alive**: `CheckFact(FACT_WALDO_ALIVE, 0)`.
6. The **Drassen airport is the player's**: `fRefuelingSiteAvailable[DRASSEN_REFUELING_SITE]` (see D2).
7. The player **can pay** for that hour (see D3).

### 2.2 One hour of work (as decided: D1, D4, D5)

Waldo works in **started hours**. Every hour he starts costs the full $200, and it is **paid when the hour starts**. That includes the last, shorter hour (D4). An hour he has started is always finished, even if the checkbox is unchecked during it (D1).

State: `static bool gfWaldoHourStarted`. It is saved in `g_gameStates` under "HeliWaldo::hourStarted" and is false in a new game.

At each full hour, `HandleWaldoHelicopterRepairHourly()` runs two steps in this order:

1. **Finishing the started hour.** If `gfWaldoHourStarted` is set:
   - Add `min(3, bLifeMax − bLife)` to the helicopter soldier's `bLife`. The checkbox is not checked here, per D1.
   - Mark `fTeamPanelDirty` and `fCharacterInfoPanelDirty`.
   - Clear `gfWaldoHourStarted`.
   - If the condition has now reached 100, show "Waldo has finished repairing the helicopter." (D5).
2. **Starting a new hour.** If all of the conditions in 2.1 hold, including the checkbox and the money:
   - Charge $200: `AddTransactionToPlayersBook(PAYMENT_TO_NPC, WALDO, …, -200)`.
   - Show "Waldo was paid $200 for repairs." This is the hourly message from D5.
   - Set `gfWaldoHourStarted`.

How the decisions play out:
- **Unchecking the checkbox in the middle of an hour (D1):** the hour he already started is finished and credited at the next full hour. No new hour starts after that, until the checkbox is checked again.
- **Checking it again:** the next hour starts at the next full hour.
- **The last hour (D4):** if only 1–2 points are missing, the hour still cost $200, and the points are capped at 100.
- **The helicopter takes off during a started hour:** the started hour is credited at once, at take-off. This is a small call in `TakeOffHelicopter()` that does step 1 early. This way a paid hour is never lost. Landing in Drassen again starts the next hour at the next full hour.
- **The mercs reach 100 first, during a started hour:** that hour stays paid and is not refunded. At its end, step 1 adds nothing and shows the "finished" message.
- **No money when an hour should start (D3 = A):** that hour does not start. "Waldo stopped repairing the helicopter: not enough money." is shown once, using a flag reset when an hour does start. Waldo starts again at the first full hour with $200 or more.
- **When the first hour starts:** at the first full hour after the helicopter lands in Drassen, or after the checkbox is checked. Up to 59 minutes of waiting is acceptable, because work is counted in full hours.

**Stats do not matter:** the 3 points and the $200 are constants, not calculated from Waldo's profile.

```cpp
// Map_Screen_Helicopter.cc
#define WALDO_HELI_REPAIR_PER_HOUR  3   // condition points
#define WALDO_HELI_REPAIR_COST      200 // $ per hour of work
```

### 2.3 Working alongside mercs

Mechanics on the Repair assignment in B13 keep repairing as they do today, independently of Waldo. Their points simply add up. The cap is always `bLifeMax`.

The order within the hour:
1. `UpdateAssignments()`, the mercs.
2. Waldo, who gets whatever is still missing.

So he is never charged for points the mercs have already done.

### 2.4 Flying

- Waldo works only while the helicopter **is on the ground**. Once it takes off, he stops, with no message, and picks up again when it lands in Drassen.
- From condition 50 up, the helicopter can fly again. `IsHelicopterTooDamagedToFly()` stops returning true, with no extra code.
- Whether the player waits for 100 or flies earlier is up to them.

### 2.5 No helicopter soldier

If there is no helicopter soldier, the condition comes from `gubHelicopterHitsTaken`. As decided in D6, **Waldo does nothing then**: no new saved state, and the case is rare.

---

## 3. Code changes (expected)

| File | Change |
| --- | --- |
| `src/game/Strategic/Map_Screen_Helicopter.cc` | <ul><li>Constants.</li><li>`static bool CanWaldoRepairHelicopter()`, the conditions in 2.1.</li><li>`void HandleWaldoHelicopterRepairHourly()`, the hour in 2.2, plus the messages.</li></ul> |
| `src/game/Strategic/Map_Screen_Helicopter.h` | Declare `HandleWaldoHelicopterRepairHourly()`. |
| `src/game/Strategic/Hourly_Update.cc` | Call `HandleWaldoHelicopterRepairHourly()` after `UpdateAssignments()`. Placing it after `PayOffSkyriderDebtIfAny()` means Skyrider's debt gets paid from the balance first. |
| `src/game/Strategic/Map_Screen_Helicopter.cc` (`TakeOffHelicopter`) | Credit the started hour at take-off (2.2). |
| `src/externalized/strings/Localization.h/.cc`, `src/game/Utils/Text.h`, the 9 `translation-*.json` files | A new array `pWaldoHeliRepairStrings`, in English as placeholders:<ol start="0"><li>"Waldo has finished repairing the helicopter."</li><li>"Waldo was paid $%d for repairs." (shown every hour)</li><li>"Waldo stopped repairing the helicopter: not enough money."</li></ol>String 1 duplicates the finances entry and can be dropped if the message is not wanted. |
| Save format | One new flag, `gfWaldoHourStarted` (`g_gameStates` "HeliWaldo::hourStarted"), saved like the other helicopter states, and false in a new game (`Campaign_Init.cc`, `SaveLoadGame.cc`). Everything else is derived each hour: the condition comes from the soldier, and the checkbox is already saved. Old saves are not supported anyway, under the project rule. |

---

## 4. Decisions (settled by the user, 2026-10-31)

| | Decision |
| --- | --- |
| **D1** | The checkbox turns Waldo's service on and off, and his lines stay shown only while it is checked. **Unchecking it does not interrupt a started hour:** he finishes it, and then does not start another until the checkbox is checked again (2.2). |
| **D2** | Waldo repairs only while the Drassen airport (B13) is the player's. |
| **D3** | No money when an hour should start: he pauses, with one message, and starts again by himself once $200 is there. |
| **D4** | Always the full $200 for **every started hour**, paid when the hour starts (2.2). |
| **D5** | **Both** messages: "Waldo was paid $200 for repairs." at the start of every hour, and "Waldo has finished repairing the helicopter." at the end. |
| **D6** | No helicopter soldier: Waldo does nothing. No new saved state for that case. |
| **D7** | Only in Drassen, not in Estoni. |

One detail not covered by the decisions, my choice: if the helicopter takes off during a paid hour, the hour is credited at once at take-off, so a paid hour is never lost (2.2).

---

## 5. Suggested order and tests

1. **Constants and the conditions** in `CanWaldoRepairHelicopter()`.
2. **The hourly tick, the payment and the message at 100.**
3. **Strings:** the 9 translation files. Copy `assets/externalized/strings` to `E:\Release1\externalized` before testing.
4. **Game tests.** For the tests, temporarily set the SAM hit chance to 100:
   - After a hit, the helicopter returns to Drassen at condition 75. With the checkbox checked: +3 per hour and −$200 per hour in the finances. After 9 hours it reaches 100, and the message appears.
   - With the checkbox unchecked: no repair.
   - A mechanic on Repair in B13 at the same time: the points add up, and Waldo gets only what is left.
   - Not enough money: the pause and its message, and work picks up again once the money arrives.
   - Unchecking the checkbox during an hour: the started hour is finished and credited (+3), then nothing more happens. Checking it again starts work at the next full hour.
   - The last hour with 1 point missing: −$200, the condition reaches 100, and the "finished" message appears.
   - A save during a started hour, then a load: the hour is still finished.
   - Taking off mid-repair: Waldo stops, and starts again when the helicopter lands in Drassen.
   - Waldo dead: no repair.
   - Condition 49 → after 1 hour, 52: the helicopter can fly again.
