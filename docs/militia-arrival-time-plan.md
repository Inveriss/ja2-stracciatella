# Militia recruitment: arrival time — implementation plan

Status: plan only (2026-10-21), not implemented. Branch: `Militia`.
Panel: `Militia_Recruit_Panel.cc` (MILITIA_RECRUIT_PANEL_1366x768.png).

## Goal

Militia bought in the recruitment panel no longer appear at once. The player
picks one of three arrival options; the militia arrive after its time.

| Option   | Extra cost (per purchase) | Arrival after |
|----------|---------------------------|---------------|
| Express  | + $1000                   | 6 hours       |
| Fast     | + $500                    | 12 hours      |
| Standard | $0                        | 24 hours      |

## User decisions (binding)

1. The buy button (Done) buys; right after it a message box appears over the
   still open panel: "Militia will arrive in ### hours". OK returns to the
   panel and its functions (the panel stays open, as today).
2. The extra cost is per purchase (each buy click), whatever the number of
   militia.
3. Ordered militia that haven't arrived yet take their places in the town:
   the "+" buttons and the town-full check count them, so a town can never be
   over-ordered. There is always room for ordered militia.
4. At arrival:
   - a) If the player has moved militia into the ordered sector meanwhile,
     the arriving militia take free places in other sectors of the town.
   - b) Enemy in the target sector and the other town sectors full: a message
     asks whether they enter the sector (standard autoresolve starts) or wait
     for orders.
   - c) A battle going on in the sector (militia + enemy + mercs, enemy +
     mercs, or militia + enemy): a message asks whether they enter and help
     in the fight, or wait.
   - d) The whole town retaken by the enemy: a message asks whether they enter
     a sector and start autoresolve, or wait.
   - e) Only part of the order fits (e.g. a sector taken by the enemy): a
     message asks whether that part enters and the rest waits, or all wait.
   - f) Later stage: a separate panel to manage the waiting militia.
5. The loyalty bonus (every 10 bought in a town) counts at arrival, not at
   purchase.
6. Texts: English in all 9 translation files for now, translations later.

## Recommended stages

### Stage 1 — Arrival options in the panel (UI only)

- Three check boxes, graphic of the sector inventory's "Combine Item Filters"
  check box (`Map_Screen_Interface_Map_Inventory.cc`, `guiMapInvenButton[14]`),
  labels "Express (+$1000, 6 h)", "Fast (+$500, 12 h)", "Standard (24 h)".
- Exactly one is checked (radio behaviour); default Standard.
- Express / Fast are greyed out (disabled) when the current balance is lower
  than the militia's cost plus that option's extra cost; both when even Fast
  doesn't fit. A checked option that becomes unaffordable falls back to the
  most expensive affordable one.
- Total Cost shows the militia's cost plus the option's extra cost
  (recommended, so the balance check stays one number).
- Placeholder position: 20 px under the Total Cost box (box 618,286 127x66,
  bottom at y 352). Note: the buy button is at y 362–395, 10 px under that
  box, so y 372 overlaps it; recommended placeholder 20 px under the buy
  button (y 415) until the user places the final graphic.
- Buying still instant in this stage (only the extra cost is charged), so
  the stage can be tested alone.
- New strings (pMilitiaRecruitStrings): the three option labels, a section
  title "Arrival Time".

### Stage 2 — Pending orders (data, save, purchase)

- `struct PendingMilitiaOrder { SGPSector sector; INT8 town; UINT8
  counts[MAX_MILITIA_LEVELS]; UINT32 arrival_min; UINT8 option; UINT32 id; }`
  in a list (Town_Militia.cc), saved in the savegame: new SAVE_GAME_VERSION
  (no backward compatibility needed — new game; see memory no-save-compat).
- `MilitiaRecruitFreeSlots()` subtracts the town's pending militia
  (decision 3).
- `BuyMilitia()` charges the money at once (militia + extra cost, finance
  entry RECRUIT_TOWN_MILITIA), creates the order, adds no militia; the
  loyalty counter is NOT advanced here (decision 5).
- New strategic event `EVENT_MILITIA_ORDER_ARRIVAL` (Game_Event_Hook.h,
  appended before NUMBER_OF_EVENT_TYPES) with the order id as parameter,
  added with AddStrategicEvent at arrival_min.
- After a successful purchase: message box over the open panel
  "Militia will arrive in %d hours" (DoMapMessageBox, OK only; the callback
  returns to the panel — decision 1).

### Stage 3 — Arrival in peace

- On the event: place the order's militia as today's purchase did — the
  ordered sector first, then the other town sectors with room (decision 4a),
  then the loyalty bonus per 10 (decision 5); remove the order; screen
  message "Militia have arrived in <town>".
- If the town sectors can't take everybody without fighting, go to stage 4.

### Stage 4 — Arrival with enemies or a battle (decisions 4b–4e)

- Message box with two choices (YES/NO style, strings to add):
  - 4b enemy in the sector, others full: "Enter <sector> (battle by
    autoresolve)" / "Wait".
  - 4c a battle going on in the sector (tactical or about to be resolved):
    "Enter and help" / "Wait". Tactical with mercs: the militia join as
    reinforcements at the map edge (to investigate: the existing militia
    placement, `PrepareMilitiaForTactical()` in Militia_Control.cc, and how
    enemy reinforcements enter in Queen_Command.cc, AddEnemiesToBattle).
  - 4d the whole town retaken: "Enter <sector> (autoresolve)" / "Wait".
  - 4e part fits: "Let N enter, the rest waits" / "All wait".
- Enter → add the militia to the sector, then the standard pre-battle /
  autoresolve flow (as militia meeting an enemy group).
- Wait → the order stays pending, flagged waiting. To decide later: when to
  ask again (recommended: when the situation changes — sector liberated,
  battle over — and once a day otherwise), so the player isn't asked every
  hour.
- Only one such question at a time; others queue (the map screen message
  box is modal). Questions during tactical: wait for the map screen, or show
  in tactical — to decide.

### Stage 5 — Waiting militia panel (decision 4f, later)

- A list of pending and waiting orders: town, counts per rank, arrival time
  or "waiting", with actions (send in now, change target sector, cancel —
  refund rules to decide).

### Stage 6 — Translations and docs

- Strings translated in all 9 files; this document updated to what was built.

## Risks

- Save format change (stage 2): a new game needed.
- Events firing while in tactical, during battles or time compression: the
  arrival questions must not interrupt a running battle or a message box.
- Stage 4 tactical reinforcements: the most work and the most unknowns.

## Open questions for later

- Refund (militia cost and/or extra cost) when an order is cancelled or the
  town is lost for good.
- A limit on the number of pending orders.
- Where the questions of stage 4 appear when the player is in tactical.
