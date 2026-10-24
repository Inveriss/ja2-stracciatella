# Militia recruitment: upgrading militia already in the town

Status: plan, not implemented (2026-10-24). Branch: `Militia`.
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

- **Upgrade button:** the panel's own + button graphic
  (`MILITIA_RECRUIT_BUTTONS.STI` 2 = normal, 3 = pressed, 39x33), centred
  on the third ("big male") portrait of its row. The portraits are at x
  137..186 and y row+9..row+70 of the rows at y 139, 231, 323 (415 the
  heavy row): centre (161, row+40), so the button at (142, row+24).
  Constants `RECRUIT_UPGRADE_X` / `RECRUIT_UPGRADE_DY`, as the - / +
  buttons'.
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

## Decisions to make (recommendations)

U1. **Whose militia.** The town's sectors of ours with no enemy, as the
    recruitment room (`ForEachRecruitSector`). Militia in a sector with
    enemies (a battle) are not upgraded. Orders on their way or waiting
    (Deployment Center) are not counted and not upgraded. Recommended: so.

U2. **"Current" counts what.** The militia of that rank standing in those
    sectors now (U1), without orders and without the upgrades picked in the
    panel. Recommended: so; optionally a second number "after upgrade".

U3. **How the button works.** Two ways:
    a) each click upgrades one militia at once and pays at once;
    b) as the + button: clicks pick upgrades (a count per row, right click
       5 at a time), their cost is added to the Total cost, the Buy button
       buys the new militia and the upgrades together.
    Recommended: b), the panel already works so. It needs a "-" or a way to
    take an upgrade back (U4).

U4. **Taking a picked upgrade back.** a) a second placeholder button (the -
    graphic) next to it; b) right click on the upgrade button takes one
    back (right click no longer 5 at a time). Recommended: a), symmetric
    with - / +.

U5. **Where the upgrade count and its cost are shown.** The row's Count /
    Cost box is for new militia. Recommended: a placeholder line under the
    upgrade button ("Upgrade: 3 = $900"), placed by the user later.

U6. **When it takes effect.** a) at once on Buy (training in the town);
    b) after the arrival option's hours, as an order. Recommended: a), and
    the arrival option's extra cost applies only to new militia (an upgrade
    alone costs no Express/Fast extra).

U7. **Price.** a) the difference of the base prices (game.json: 100 / 200
    / 500 / 1000 -> 100, 300, 500); b) the difference of the "Your price"
    values, i.e. with the leadership percent (75-125 %) as new militia.
    Recommended: b), so that a better recruiter makes upgrades cheaper
    too, as he makes new militia cheaper.

U8. **Chains in one purchase.** Can a green be picked green -> regular and
    the same militia regular -> elite in one purchase? Recommended: no, each
    row upgrades only the militia of its rank standing now (U2); the next
    step is a next purchase.

U9. **Which sector's militia.** In a town of several sectors: the
    recruiting merc's sector first, then the other sectors in the town's
    order. Recommended: so (the rank of a sector matters little).

U10. **Limits.** Upgrades don't change the number of militia, so the room
     (free slots) is not involved; only the balance. Up to "Current" per
     row (U8).

U11. **Loyalty.** New militia raise the town's loyalty
     (`AddMilitiaRecruitLoyalty`). Upgrades: recommended none.

U12. **SAM sites.** The panel opens for SAM sites too; upgrades work there
     the same (one sector). Recommended: yes.

U13. **Texts** (9 languages): "Current", the upgrade buttons' help ("Upgrade
     a green militia to regular: $%d", ...), the U5 line, the take-back
     button's help. Recommended wording to be confirmed in English first.

## Stages

1. **Upgrade logic and placeholders** (U1-U12 as decided): the buttons,
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
