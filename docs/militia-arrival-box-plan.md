# Militia arrival: placing them in the militia box

Status: plan, not implemented; decisions D1-D8 given by the user
(2026-10-24). Branch: `Militia`.
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

## Decisions (user, 2026-10-24)

D1. **When the box opens: also for SAM sites.** For any order that doesn't
    fit whole: some room left (today's 4e), or no room but an enemy-held
    sector to attack (today's 4b/4d). A SAM site is always one sector; the
    same holds for a town whose sectors are all held by the enemy when the
    order arrives. The text under the box then depends on the case:

    - Town, some sectors ours (the Goal's case): the text in the Goal.
    - Town with all its sectors held by the enemy:

      > The militia has arrived in (town), but there is room for only
      > (COUNT) of the (COUNT) militia.
      >
      > Choose which militia:
      > - Send to recapture the occupied sector
      > - Send to Deployment Center

    - SAM site ours (the order doesn't fit whole in its one sector):

      > The militia has arrived in (SAM site).
      >
      > Choose which militia:
      > - Send to available sector
      > - Send to Deployment Center

    - SAM site taken back by the enemy meanwhile:

      > The militia has arrived in (SAM site), but the sector is occupied by
      > enemy forces.
      >
      > Choose which militia:
      > - Send to recapture the occupied sector
      > - Send to Deployment Center

    Joining a battle in tactical (4c) is D5.2.

D1b. **Also when the whole order fits** (user, 2026-10-24): today an order
    that fits whole enters the sectors by itself and a message box lists
    the militia that arrived ("10 green militia have arrived in Drassen.",
    commit 99bac6a04). Instead the box opens with the order on the cursor,
    to be placed by hand, as in every other case. So the box opens for
    every arriving order (towns now, SAM sites with their frames). The
    lines per rank stay in the message log. Text under the box in this case
    (proposed, as the SAM site's own text in D1):

    > The militia has arrived in (town).
    >
    > Choose which militia:
    > - Send to available sectors
    > - Send to Deployment Center

    It is the same arrival mode (the order owns the cursor, Done, D3, D8),
    so it is part of stage 1, not a change of its own: the room only decides
    the text.

D1a. **Liberated first, without exception** (2026-10-24, commits 3cc60b4ec
    and the next): militia alone never attack a sector the mercs never
    liberated (`fSurfaceWasEverPlayerControlled`). In the box they may be put
    there, but Done refuses with "Your mercenaries must liberate %s first.
    After that, you can recapture sectors using militia alone." and the box
    stays open; an arriving order is not asked to attack it (it waits, the
    same line in the message log). The recapture choice in D1's texts is
    therefore only for sectors that were ours once.

D2. **Existing militia: yes**, the box's full normal functions (the down
    arrows also take militia out of the sectors).

D3. **Closing the box other than with Done** (ESC, the map border's militia
    button, the team list, leaving the map screen): what is on the cursor
    goes to the Deployment Center.

D4. **Done** stays disabled while anything is on the cursor.

D5. **The order arriving on the tactical screen**, in sub-stages:

    - D5.1: tactical screen, no battle: the box appears at once.
    - D5.2: tactical screen during a battle, every kind: the real-time
      watched militia battle, our mercs against the enemy, our mercs and
      militia against the enemy: the box appears at once.

    The user asks whether the box can open directly on the tactical screen,
    without going to the strategic map. See "The box on the tactical screen"
    below.

D6. **AUTO in this mode** fills the sectors of ours up to 20 each; what
    doesn't fit stays on the cursor. (Today's AUTO divides everything evenly
    and can put more than 20 in a sector, then clears the cursor -- only
    harmless today because the cursor never holds more than the room. It
    gets the cap anyway.)

D7. **Several orders arriving together**: one box at a time, in the order
    they arrived. An order for the same town arriving while its box is open
    joins the cursor.

D8. **Send to Deployment Center does not close the box.** It moves the
    militia on the cursor to the Deployment Center; the box stays open (the
    player may go on placing militia from the sectors, then press Done).

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
- Send to Deployment Center (D8): a new waiting order is split off with the
  cursor's counts (it shows in the deployment panel at once), the box's
  order is left with 0 and the cursor is cleared; the box stays open. What
  the player takes out of the sectors afterwards belongs to the box's order
  again (and may be sent the same way).
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
On the tactical screen: D5.1/D5.2 (stages 4 and 5); until then the
question stays queued until the map screen. Opening sets `fShowMilitia` (the map's militia view) and
`sSelectedMilitiaTown`, as clicking a town in that view does.

### SAM sites (D1)

The box draws the town's 3x3 map from `militiamaps.sti` (a frame per town).
N4 belongs to a town; D2, D15 and I8 are SAM sites outside towns and have no
such frame. Decided: the user adds frames for them to `militiamaps.sti`
later (presumably its sector and the 8 around, only the middle one usable;
the frame order and the SAM sector's place in it to be agreed with the
graphic). Until then the SAM site box is not opened (the order's message
boxes stay for SAM sites outside towns).

**Opening the box on a SAM site by a click** (user, 2026-10-24): today the
box opens only for a town (`MapScreen.cc`, the militia view's map click:
`GetTownIdForSector() != BLANK_SECTOR`), as in the original game, so D2,
D15 and I8 can't open it (N4 lies in a town and opens that town's box). The block
goes with the SAM frames: the box is keyed by the town id throughout
(`sSelectedMilitiaTown`, `sBaseSectorList`, `FOR_EACH_SECTOR_IN_TOWN`, the
town checks in picking up and dropping, AUTO, the title). A SAM site gets
its own key (e.g. a value above the town ids mapped to its sector), its
3x3 base sector, its frame and its name; the sector filters accept its one
sector. Done when the user's frames exist (with stage 1 or right before).

### The box on the tactical screen (D5)

The question: can the box open on the tactical screen itself? It can, two
ways. **Decided: B** (2026-10-24).

A. **Directly on the tactical screen.** The box is map screen code: its
   position on the strategic map, the map screen's redraw (`fMapPanelDirty`,
   its save buffer), its mouse regions and screen mask, the town maps. On
   the tactical screen it would be a second copy drawn over the world every
   frame (like the tactical big boxes), with the game paused under it
   (real-time battles too) and the tactical input blocked. Possible, but
   about as large again as stage 1, with two copies to keep in step.

B. **A jump to the strategic map and back (chosen).** The arrival
   switches to the map screen on its own (as the M key does; whether every
   kind of battle allows it is checked in stage 5 -- tactical stops while
   the map screen is up), the box opens
   there; Done returns to the tactical screen (Send to Deployment Center keeps
   the box open, D8). It reuses the whole box; for the player it looks like the box
   appearing at once.

D5.2 in either way: militia placed in the sector of a battle going on in
tactical are reinforcements and enter from a map edge (stage 4.3's
`AddMilitiaReinforcementsToBattle()`), as many as the battle takes; the
rest stays on the cursor. The real-time watched battle (watch mode) is to
be tested on its own: the switch to the map screen must not end the watch
mode or let the battle run on.

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
- `MILITIA_STR_ARRIVAL_BOX_HEADER_SAM`: "The militia has arrived in %s."
- `MILITIA_STR_ARRIVAL_BOX_HEADER_OCCUPIED`: "The militia has arrived in %s,
  but the sector is occupied by enemy forces."
- `MILITIA_STR_ARRIVAL_BOX_CHOOSE`: "Choose which militia:"
- `MILITIA_STR_ARRIVAL_BOX_SECTORS`: "- Send to available sectors"
- `MILITIA_STR_ARRIVAL_BOX_SECTOR`: "- Send to available sector" (SAM site)
- `MILITIA_STR_ARRIVAL_BOX_RECAPTURE`: "- Send to recapture the occupied
  sector"
- `MILITIA_STR_ARRIVAL_BOX_DEPLOYMENT`: "- Send to Deployment Center"
- `MILITIA_STR_ARRIVAL_BOX_BUTTON`: "Deployment Center" (button label, if
  the button graphic has no text)
- `MILITIA_STR_ARRIVAL_IN_TACTICAL`: the D5 log line.

Which header and lines: D1. The recapture line only when there is an
enemy-held sector the militia can attack, the sector line(s) only when there
is a sector of ours. Translations as in stage 6, checked for width with
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

4. **D5.1**: arrival on the tactical screen without a battle (way B).
5. **D5.2**: arrival during a battle, each kind tested: watched real-time
   battle, mercs vs enemy, mercs and militia vs enemy.

Stage 1 is the biggest (about 250-350 lines, mostly in
Map_Screen_Interface_Map.cc). It needs game testing and is better done with
the game at hand, not from the mobile app.
