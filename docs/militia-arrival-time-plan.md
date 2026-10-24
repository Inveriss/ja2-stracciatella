# Militia recruitment: arrival time

Status: implemented, stages 1-6 done and verified in the game (2026-10-21 to
2026-10-22). Branch: `Militia`.
Code: `Strategic/Town_Militia.cc` (orders, arrival, questions),
`Strategic/Militia_Recruit_Panel.cc` (recruitment panel),
`Strategic/Militia_Deployment_Panel.cc` (deployment panel),
`Strategic/Assignments.cc` (the Militia menu),
`Strategic/PreBattle_Interface.cc` (militia attack battles),
`Tactical/Militia_Control.cc` (reinforcements in tactical).

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
   - f) A separate panel to manage the waiting militia.
5. The loyalty bonus (every 10 bought in a town) counts at arrival, not at
   purchase.
6. Texts: English first, translated into all 9 languages at the end
   (stage 6).
7. No refunds for now. Militia that cannot enter go to the waiting list.
8. The waiting militia are shown in a separate Deployment Panel
   (MILITIA_DEPLOYMENT_PANEL_1366x768.png, 761x647): title "MILITIA
   DEPLOYMENT", two headers "INCOMING" and "WAITING", one row per rank (green,
   regular, elite, elite heavy) with the rank's three portraits, an Incoming
   box (count, town, ETA per order), a Waiting box (count, town and a Done
   button per order) and a TOTAL box (Incoming / Waiting).
9. The Done button of a waiting order tries again to enter the target town;
   graphic SECTOR_INVENTORY_DONE_BUTTONS.STI, images 2 and 3.
10. No limit on the number of orders; every order is listed in the
    Deployment Panel.
11. The questions of stage 4 are shown on the tactical screen too.
12. The Deployment Panel shows values only, no labels, except in the TOTAL
    box.
13. Later stage: scrolling or arrows in the Deployment Panel.
14. Stage 4 in three parts, tested one by one: 4.1 the question box (map
    screen and tactical), its queue, Wait and the partial entry (4e); 4.2
    entering a sector with the enemy or a retaken town: the pre-battle panel
    with autoresolve / watch (4b, 4d); 4.3 a battle going on: joining it, in
    tactical with mercs as reinforcements from the map edge (4c).
15. 4c reinforcements come in from the edge facing a neighbouring sector of
    the same town; without one, from the edge farthest from the enemies.
16. Waiting orders are not asked about again by themselves: the Deployment
    Panel's Done button tries again (decision 9).
17. Both panels open wherever militia can be bought (a town or SAM site
    allowed to have militia, with the loyalty to train them), also in a town
    full of militia; there the recruitment panel's + buttons stay greyed out.
    The loyalty requirement stays.

## What was built

### Stage 1 -- Arrival options (commit 6955477fc)

- The recruitment panel's right column: Current Balance (y 147), Arrival
  Time (y 239, new box), Total Cost (y 331); the buy button 10 px under it
  (y 407). Boxes 127x66 at x 618.
- Arrival Time: a header and three check boxes (the sector inventory's check
  box sheet `Sector_Inventory_PopupCheck.sti`, its 15x15 images 0-3, so that
  three lines fit), each with the option's name and hours; the extra cost is
  in the check box's fast help. Exactly one checked, Standard when the panel
  opens; an option the balance can't pay (with the picked militia) is greyed
  out, and a checked one falls back to the most expensive affordable one.
- Total Cost, the + buttons and the purchase include the extra cost
  (`MilitiaArrivalHours()`, `MilitiaArrivalExtraCost()`).

### Stage 2 and 3 -- Orders, save, peaceful arrival (commit df9f38451)

- `BuyMilitia(sector, counts, arrival_option)` charges the militia and the
  extra cost (one finance entry RECRUIT_TOWN_MILITIA) and creates a
  `MilitiaOrder` { id, sector, counts per rank, arrival_min, option, waiting }
  instead of placing militia; the strategic event
  `EVENT_MILITIA_ORDER_ARRIVAL` (79, parameter: the order's id) fires at
  arrival_min. The panel shows "Militia will arrive in %d hours."
- `MilitiaRecruitFreeSlots()` subtracts the militia ordered for the same town
  or SAM site, on their way or waiting (decision 3).
- At arrival, when everybody fits: the ordered sector first, then the town's
  other sectors with room (decision 4a), the loyalty bonus (decision 5),
  "Militia have arrived in <town>."
- The orders are saved in the game states (key "Militia::orders", a list of
  numbers, 10 per order); no SAVE_GAME_VERSION change was needed. A new game
  starts without orders. An order saved while its question was pending waits
  after loading.

### Stage 4.1 -- Questions, partial entry (commit efcf59c1d)

- Questions are queued and asked one at a time with a YES/NO message box
  (`DoScreenIndependantMessageBox()`, map screen and tactical) when nothing
  else is going on: no other message box, pre-battle panel, watched battle
  or turn-based combat (`HandleMilitiaOrderQuestions()`, every map screen
  and tactical frame). The room is checked again just before asking.
- 4e: "Only %d of the %d militia ordered for %s can enter. Let them enter?"
  YES: those who fit enter, the best ranks first; the rest waits. NO: the
  whole order waits.
- Nothing fits and nothing to ask: the order waits ("... can't enter and are
  waiting for orders.").
- Later replaced for towns and SAM sites by the militia box's arrival mode
  (docs/militia-arrival-box-plan.md): every arriving order opens the box
  with its militia to be placed by hand; only joining a battle in tactical
  (4c) keeps its question.

### Stage 4.2 -- Attacking an enemy-held sector (commit bd4535077)

- 4b / 4d: the order's sector held by the enemy (no battle there already, the
  sector not full of militia): "The enemy holds %s, where the militia
  ordered for it should go (%s). Attack?"
- YES: as many as the sector holds (20) enter it, the best ranks first; the
  rest waits. `StartMilitiaAttackBattle()` opens the persistent pre-battle
  panel without a battle group (a flag in PreBattle_Interface.cc gives it the
  sector and ENEMY_ENCOUNTER_CODE instead of a creature attack); the
  autoresolve starts by itself, with Watch the Battle available. A victory
  makes the sector the player's.

### Stage 4.3 -- Joining a battle (commit 840379e44)

- 4c: a battle going on in the order's sector, loaded in tactical (mercs, or
  a watched militia battle): "A battle is going on in %s (%s). Should the
  militia ordered for it join the fight?" -- asked on the tactical screen
  only, in real time or in the player's turn, between attacks. Battles on
  the pre-battle panel or in autoresolve aren't asked about: the order waits
  until they are over and is checked again.
- YES (`AddMilitiaReinforcementsToBattle()`): as many as the battle takes
  (the sector's 20 and the militia team's free slots), the best ranks first,
  from the edge facing a neighbouring sector of the same town, else the edge
  farthest from the enemies (decision 15), on its edgepoints as enemy
  reinforcements; alert, seeking the enemy; counted in the sector at once.

### Stage 5 -- Deployment panel (commits 5c7584689, 154967dda)

- Opened by Assign > Train > Militia > Militia Deployment Center (the Militia
  menu: Militia Recruitment Center / Militia Deployment Center, ef055a13f);
  at the map screen's right edge as the recruitment panel, one of the two at
  a time; from tactical it opens on the map screen.
- Per rank: Incoming (count of the rank, town or SAM sector, hours to arrival
  rounded up; the soonest first), Waiting (count, town, Done button) and
  TOTAL (Incoming / Waiting of the rank). Up to 3 orders a box for now.
- Done (`RetryMilitiaOrder()`) tries again with the whole order, as at its
  arrival, with the same questions.
- Decision 17: the panels open in a town full of militia too.

### Stage 6 -- Translations and this document

- All militia feature strings (pMilitiaRecruitStrings, pMilitiaWatchStrings,
  pMilitiaHeavyStrings, the finance entry pTransactionText[28]) translated
  into Chinese, Dutch, French, German, Italian, Polish and Russian (Russian
  Gold the same as Russian), with each language's existing militia and menu
  terms (e.g. the Italian translation's "esercito"). The panels' texts were
  checked for width with tools/interface_preview (the Latin-script
  languages).

## Open for later

- Scrolling or arrows in the Deployment Panel (more than 3 orders a box).
- Refunds, cancelling an order or changing its target sector.
