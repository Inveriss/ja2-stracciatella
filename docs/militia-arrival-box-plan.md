# Militia arrival: placing them in the militia box

Status: plan, not implemented (2026-10-24). Branch: `Militia`.
Code to change: `Strategic/Town_Militia.cc` (orders, arrival questions),
`Strategic/Map_Screen_Interface_Map.cc` (the militia box),
`Strategic/MapScreen.cc` (opening the box from the arrival),
the 9 language files (`Utils/*Text.cc`).
Builds on: [militia-arrival-time-plan.md](militia-arrival-time-plan.md)
(orders, stage 4.1 partial entry, stage 4.2 attack, stage 5 deployment
panel) and the big militia box with arrows (`militia.sti` 19, commits
0cdb73246, 045fb69ec, bdd15a4c6, cc80f6864).

## Goal

All the town's sectors are ours; militia are ordered for all of them and are
on their way. Meanwhile the enemy takes one sector. When the order arrives,
part of it has no room. Today a YES/NO message box asks "Only %d of the %d
militia ordered for %s can enter. Let them enter?" (stage 4.1, 4e).

Instead, the big militia box (`militia.sti` 19) opens for the order's town:

- The whole order is shown at once as squares on the cursor (the
  "unassigned" militia and their counters per rank), to be placed by hand.
- The player places them with the arrows:
  - into the town's sectors of ours, up to 20 per sector;
  - into the sector the enemy holds: they attack it on Done (autoresolve, as
    in commit cc80f6864);
  - the rest go to the Deployment Center (a new button): the order waits
    there, as today's "waiting for orders" (stage 5 panel).
- Under the box, under AUTO and DONE, a text box (graphic by the user):

  > The militia has arrived in (town), but there is room for only (COUNT) of
  > the (COUNT) militia.
  >
  > Choose which militia:
  > - Send to available sectors
  > - Send to recapture the occupied sector
  > - Send to Deployment Center

- The "Send to Deployment Center" button (graphic by the user) sits 1 px
  above AUTO and DONE; the panel is updated for it (new `militia.sti` frame
  or a taller frame 19, by the user).

## Terms

**On the cursor** (the code's `sGreensOnCursor` ...): the unassigned militia --
the squares in the box under the rank buttons and down arrows, and the
counters per rank under them -- taken out of the sectors and not placed yet.
Not the militia standing in the sectors (the squares in the town's map).

## Decisions to confirm (recommendations)

D1. **When the box opens.** For a town order (not a SAM site) that doesn't
    fit whole: some room left (today's 4e), or no room but an enemy-held
    sector of the town to attack (today's 4b/4d). Recommended: both. SAM
    site orders and joining a battle in tactical (4c) keep their message
    boxes.

D2. **Existing militia.** In this mode the box works as always: the player
    may also take militia out of the sectors with the down arrows (to swap
    ranks). Recommended: yes.

D3. **Closing the box other than with Done** (ESC, the map border's militia
    button, the team list, leaving the map screen). Recommended: whatever is
    on the cursor waits at the Deployment Center (as the button), nothing is
    lost or spread automatically.

D4. **Done** stays disabled while anything is on the cursor (as today): the
    player places everything or sends the rest to the Deployment Center.

D5. **The order arriving on the tactical screen.** The box lives on the map
    screen. Recommended: a line in the message log ("The militia have arrived
    in Drassen. Place them in the strategic map's militia box.") and the box
    opens when the player enters the map screen (as the deployment panel
    opened from tactical, stage 5). During a battle it waits until the battle
    ends (as today's questions).

D6. **AUTO in this mode** fills the town's sectors of ours up to 20 each;
    what doesn't fit stays on the cursor. (Today's AUTO divides everything
    evenly and can put more than 20 in a sector, then clears the cursor --
    only harmless today because the cursor never holds more than the room.
    It needs the cap anyway.)

D7. **Several orders arriving together** for the same or other towns: one
    box at a time, in the order they arrived (today's question queue). An
    order for the same town arriving while its box is open joins the cursor
    (recommended) or waits for the next box.

## Design

### The order stays the owner of its militia

To keep saves and closing safe, the order is not removed when the box opens:

- Opening: the order's counts go onto the cursor (`sGreensOnCursor` ...),
  the box remembers the order id (`g_arrival_order`), the order is marked
  `in_box` (not saved as a field: a save/load turns it back into a waiting
  order).
- Every change of the cursor: the order's counts are set to the cursor's
  counts. The cursor then always holds exactly the order's militia (also
  those the player took out of a sector -- they become part of it; the
  militia total stays the same). A save at any moment writes the sectors and
  the order consistently; after loading, the order is a waiting one in the
  Deployment Center.
- Done (cursor empty): the order's total is 0 -> removed. Recruitment
  loyalty (`AddMilitiaRecruitLoyalty`) for the militia that entered: the
  order's total at opening less those that went to the Deployment Center.
  An enemy-held sector with militia: the attack starts (existing code).
- Send to Deployment Center: the order gets `waiting = true` (counts =
  cursor), the cursor is cleared; the box stays open with nothing on the
  cursor so the player can still move things and press Done.
- Forced close (D3): as the Deployment Center button, then the box closes.

`HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor()` (spreads the cursor over
the sectors and asserts it is empty after) must not run in this mode: the
cursor may hold more than the room.

### Opening

`HandleMilitiaOrderQuestions()` (Town_Militia.cc), for a town order with
room < total (D1): instead of `MILITIA_STR_ORDER_PARTIAL` / `_ATTACK` it
calls a new `OpenMilitiaArrivalBox(order_id)` (Map_Screen_Interface_Map.cc)
when the map screen is up, no other box/panel is open (recruitment,
deployment, militia box in normal mode, pre-battle) and no message box.
On the tactical screen it logs the message once and keeps the question
queued (D5). Opening sets `fShowMilitia` (the map's militia view) and
`sSelectedMilitiaTown`, as clicking a town in that view does.

### Drawing

- The text box: 4-5 lines in FONT14ARIAL, FONT_BEIGE (the box's texts),
  wrapped to the box width; position and size from the user's graphic. The
  first line with the town's name, the room and the order's total (room as
  at opening: the town's free places in sectors of ours).
- The "Send to Deployment Center" button: `militia.sti` frames for normal
  and pressed (as AUTO/DONE use 1/2) or a new STI, at 1 px above AUTO/DONE;
  shown only in this mode. Help text: "The militia on the cursor wait at the
  Deployment Center."
- Placement: the box's top is `MAP_SCREEN_Y + 125`; with the text box under
  it the box may reach past the map's bottom (the map view ends near y 605
  at 1366x768, today's box ends at 125 + 443 = 568). The box's y may have to
  move up for this mode -- decided when the graphic's height is known.

### Texts (9 language files, pMilitiaRecruitStrings)

- `MILITIA_STR_ARRIVAL_BOX_HEADER`: "The militia has arrived in %s, but
  there is room for only %d of the %d militia."
- `MILITIA_STR_ARRIVAL_BOX_CHOOSE`: "Choose which militia:"
- `MILITIA_STR_ARRIVAL_BOX_SECTORS`: "- Send to available sectors"
- `MILITIA_STR_ARRIVAL_BOX_RECAPTURE`: "- Send to recapture the occupied
  sector"
- `MILITIA_STR_ARRIVAL_BOX_DEPLOYMENT`: "- Send to Deployment Center"
- `MILITIA_STR_ARRIVAL_BOX_BUTTON`: "Deployment Center" (button label, if
  the button graphic has no text)
- `MILITIA_STR_ARRIVAL_IN_TACTICAL`: the D5 log line.

The recapture line only when the town has an enemy-held sector the militia
can attack. Translations as in stage 6, checked for width with
tools/interface_preview.

## Stages

1. **Logic without the new graphics.** Arrival mode of the box: opening from
   the question queue (map screen, D5 log line in tactical), the order on the
   cursor and kept in sync, Done, forced close to the Deployment Center, AUTO
   capped at 20 (D6), no `HandleShutDown...` in this mode. A temporary
   Deployment Center button with the AUTO/DONE graphic and the text drawn
   without its box, both inside the current panel. Game test: the Goal's
   scenario, a save and load with the box open, ESC.
2. **The user's graphics.** The text box and the button at their final
   places, the box's y if needed (Drawing).
3. **Texts and translations**, `docs/militia-arrival-time-plan.md` updated
   (4e replaced for towns).

Stage 1 is the biggest (about 250-350 lines, mostly in
Map_Screen_Interface_Map.cc). It needs game testing and is better done with
the game at hand, not from the mobile app.
