# Militia recruitment: upgrading militia already in the town

Status: plan, not implemented; decisions U1-U13 given by the user
(2026-10-24). Branch: `Militia`.
Code to change: `Strategic/Militia_Recruit_Panel.cc` (the panel),
`Strategic/Town_Militia.cc` / `.h` (the upgrade itself), the 9 translation
files (`assets/externalized/strings/translation-*.json`,
`pMilitiaRecruitStrings`) and `src/externalized/strings/Localization.h`
(the array's size).

## Goal

Militia that arrived in a town keep their rank; today only experience
promotes them. The Militia Recruitment Center panel
(`MILITIA_RECRUIT_PANEL_1366x768.png`) gets upgrade buttons:

- green -> regular (the green row),
- regular -> elite (the regular row),
- elite -> elite heavy (the elite row);
- the elite heavy row has none.

An upgrade costs the difference between the two ranks' prices: with a
regular at $200 and an elite at $500, one regular -> elite costs $300.

Each row also gets a "Current" label with the number of militia of that
rank the town has now, under it.

## Placeholders (user, to be placed by hand later)

- **Upgrade + and - buttons** (U4): the panel's own + and - graphics
  (`MILITIA_RECRUIT_BUTTONS.STI` 2/3 and 0/1, 39x33), on the middle
  ("normal male") portrait of the row. The portraits are at x 80..129 and y
  row+9..row+70 of the rows at y 139, 231, 323 (415 the elite heavy row,
  without buttons): centre (105, row+40). Two 39x33 buttons don't fit one
  49x61 portrait side by side, so for a start the + over the -: + at
  (86, row+8), - at (86, row+41) (they reach 2 px past the portrait's
  bottom). Constants `RECRUIT_UPGRADE_X`, `RECRUIT_UPGRADE_PLUS_DY`,
  `RECRUIT_UPGRADE_MINUS_DY`, as the - / + buttons'.
- **"Current" and its number:** next to "Your price" in the rank box
  (197,row+8 156x66), the label at row+46 right of it and the number under
  it; the box is narrow, so for the first version the rank box's text may
  overlap -- the user moves it. Constants `RECRUIT_CURRENT_X` /
  `RECRUIT_CURRENT_DY`.

## Existing code it builds on

- `StrategicPromoteMilitiaInSector(sector, rank, n)` (Town_Militia.cc,
  static): moves n militia of a rank to the next rank in a sector; works for
  elite -> elite heavy as well. To be made public or wrapped.
- `MilitiaRecruitUnitPrice(rank, sector)`: a rank's price for this town,
  the base price (game.json `militia_recruitment`) times the best
  recruiter's leadership percent.
- `ForEachRecruitSector(sector, f)`: the town's (or SAM site's) sectors of
  ours with no enemy.
- The panel's pick model: - / + change `g_counts[rank]`, right click 5 at a
  time, the Total cost, the arrival option, the Buy button
  (`BuyMilitia()`).
- A militia change in the sector loaded in tactical needs
  `gfStrategicMilitiaChangesMade = TRUE` (as `EnterMilitiaOrder()` does).

## Decisions (user, 2026-10-24)

U1. **Whose militia:** only the militia standing in the town's sectors of
    ours with no enemy (`ForEachRecruitSector`). No orders on their way or
    waiting (Deployment Center). Militia in a sector with enemies are not
    upgraded.

U2. **"Current"** counts only the militia of that rank standing in those
    sectors now (U1): no orders on their way or waiting, without the
    upgrades picked in the panel.

U3. **The buttons pick, Buy buys:** as the + button, clicks pick upgrades
    (a count per row, right click 5 at a time); their cost is added to the
    Total cost; the Buy button buys the new militia and the upgrades
    together.

U4. **A + and a - button per row** for the upgrades, on the middle
    ("normal male") portrait (Placeholders). The - takes a picked upgrade
    back; right click 5 at a time on both, as on the new militia's - / +.

U5. **The upgrade count and its cost:** a placeholder line under the
    upgrade buttons, e.g. "Upgrade: 3 = $900", placed by the user later.

U6. **At once on Buy** (for now): the upgrade is training in the town, no
    arrival time; the arrival option's extra cost applies only to new
    militia (upgrades alone cost no Express/Fast extra).

U7. **Price: the difference of the "Your price" values**, i.e. with the
    best recruiter's leadership percent (75-125 %), as new militia:
    `MilitiaRecruitUnitPrice(rank + 1) - MilitiaRecruitUnitPrice(rank)`.

U8. **No chains in one purchase:** each row upgrades only the militia of
    its rank standing now (U2); the next step is a next purchase. So a
    row's picks are capped by its "Current".

U9. **Which sector's militia:** the recruiting merc's sector first, then
    the town's other sectors in the town's order.

U10. **Limits:** the number of militia doesn't change, the room (free
     slots) is not involved; only the balance and "Current" (U8).

U11. **Loyalty:** upgrades don't raise the town's loyalty.

U12. **SAM sites:** upgrades work there too (one sector).

U13. **Texts** (9 languages): "Current", the upgrade buttons' help
     ("Upgrade a green militia to regular: $%d", ...), the - button's help,
     the U5 line. English wording confirmed first, translations in stage 2.

## Stages

1. **Upgrade logic and placeholders** (U1-U12): the + / - buttons,
   the picks, the cost in the Total, Buy applying them, "Current". English
   texts in all 9 files (translations in stage 2). Game test: a town of
   several sectors, a sector loaded in tactical, too little money, Buy with
   new militia and upgrades together.
2. **Translations**, the placeholders moved to their final places by the
   user, `docs/militia-arrival-time-plan.md` (the panel's description)
   updated.

Size: stage 1 about 150-200 lines in Militia_Recruit_Panel.cc and a public
upgrade function in Town_Militia.cc. It needs game testing, so better done
with the game at hand.
