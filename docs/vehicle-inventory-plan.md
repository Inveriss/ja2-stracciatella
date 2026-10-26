# Plan: vehicle inventory (transporting items)

Status: **plan only**, not implemented (2026-10-26, user on Remote Control).
Branch `Vehicles`. Background: [vehicles-current-implementation.md](vehicles-current-implementation.md).

## 1. Goal

1. Vehicles can be used to transport items.
2. A vehicle's inventory opens like a merc's; items are moved into its slots
   the same way as for a merc.
3. Vehicles use the same inventory configuration and panel as mercs.
4. Vehicles have a higher limit of items per small / big slot.

## 2. Findings (code analysis)

- A vehicle already has a full inventory: its `SOLDIERTYPE` has the same
  `inv[]` slots as a merc and is saved with the team — **no save format
  change**.
- The map screen inventory is closed for vehicles in one place:
  `MapCharacterHasAccessibleInventory()` (`MapScreen.cc`) rejects
  `IsMechanical(s)`, which is true for vehicles **and** the robot.
- Everything else is generic for any character: opening the panel, moving
  items between characters (`MapscreenCanPassItemToChar` — same sector, not
  between sectors, no enemies), taking items from the sector inventory,
  stacking.
- A vehicle destroyed in tactical drops its items on the ground like a dead
  merc (`Rotting_Corpses.cc`) — cargo is not lost.
- The panel already hides weight / armour / camouflage for vehicles; weight
  does not matter (vehicles don't walk).
- Tactical SM panel is closed for non-human bodies (`Interface.cc`,
  `!IS_MERC_BODY_TYPE`).
- Slot limits in the data: almost all items have `ubPerPocket` 100 (sector
  stash limit) with the merc limits as overrides `ubBigPerPocket` 8 /
  `ubSmallPerPocket` 4; items with `ubPerPocket` 0 are one per slot and don't
  fit small pockets; weapons are 0–1 per slot except combat knife 2, rocks 2–3,
  throwing knives 4, brass knuckles 4.
- The per-slot limit is computed for a soldier's inventory in three places
  only: `PlaceObject` (`Items.cc`), automatic placement (`AutoPlaceObject`,
  `Items.cc`), the stack-split popup (`Interface_Items.cc`,
  `InitItemStackPopup`). Other uses only test "fits at all" (limit 0 or not).

## 3. Decisions (2026-10-26)

| # | Question | Decision |
|---|---|---|
| V1 | Multiplier of the merc limit for vehicles | **×4** for now |
| V2 | One global multiplier or per vehicle? | **per vehicle**, a field in `vehicles.json` |
| V3 | Money limits | **unchanged** for vehicles (100000 big / 50000 small pocket) |
| V4 | Excluded items | **all weapons (`IC_WEAPON`) and brass knuckles (`IC_PUNCH`)** keep the merc limit; no other exceptions for now |
| V5 | Where can the vehicle inventory be opened? | **variant B**: map screen in stages 1–2, **and tactical as a completely separate, independent stage 3**, started only after stages 1–2 are finished |

Open (proposals from the analysis, not decided yet):

| # | Question | Proposal |
|---|---|---|
| V6 | Which slots? | pockets only; hands and armour/head slots blocked (no effect on a vehicle, hand items could affect tactical) |
| V7 | Damaged vehicle (condition < `OKLIFE` 15) | inventory still accessible (today's rule would close it) |

## 4. Design

### 4.1 Data

`vehicles.json`, new optional field per vehicle:

```
"slotLimitMultiplier": 4
```

`VehicleModel` gets `uint8_t slot_limit_multiplier` (default 1 when the field
is missing = merc limits). Set 4 for the Hummer and the ice cream truck (and the
other ground vehicles); the helicopter has no inventory (no `SOLDIERTYPE`).

### 4.2 One limit function for a soldier's slot

New function in `Items.cc`:

```
UINT8 SoldierSlotLimit(SOLDIERTYPE const& s, UINT16 usItem, INT8 bSlot)
{
	UINT8 limit = min(ApplyBigPerPocketOverride(usItem, bSlot, ItemSlotLimit(usItem, bSlot)), MAX_OBJECTS_PER_SLOT);
	if (s is a vehicle && limit > 1 && !(item class & (IC_WEAPON | IC_PUNCH)))
		limit = min(limit * multiplier_of_this_vehicle, MAX_OBJECTS_PER_SLOT);
	return limit;
}
```

- `limit > 1`: items that are one per slot (or 0 = not in this pocket) stay
  as they are, so "doesn't fit a small pocket" is kept.
- Weapons and brass knuckles: merc limit (V4).
- Cap 100 (`MAX_OBJECTS_PER_SLOT`).
- `MoneySlotLimit` is not touched (V3).

Effect for a typical 8/4 item: merc 8 / 4, vehicle ×4 **32 / 16**.

### 4.3 Call sites switched to `SoldierSlotLimit`

1. `PlaceObject` (`Items.cc`, the `ubSlotLimit` computed at the top).
2. `AutoPlaceObject` stacking check (`Items.cc`, the `ItemSlotLimit(...)`
   compare for non-money items).
3. `InitItemStackPopup` (`Interface_Items.cc`, `ubLimit`).

Unchanged: `CanItemFitInPosition` (tests limit 0 only), sector stash code,
arms dealers, `Inventory_Choosing` (enemy/militia kits), map/tactical hand
checks (> 0 only).

### 4.4 Opening the inventory

- `MapCharacterHasAccessibleInventory`: allow vehicles (keep the robot
  blocked), per V5–V7.
- Slot restriction (V6): `CanItemFitInPosition` refuses hands and
  armour/head slots for a vehicle; the panel can grey them out.
- Check the asserts that call `MapCharacterHasAccessibleInventory`
  (`MapScreen.cc`) and the inventory-button enable logic.

### 4.5 Map screen transfers: battle and travel

- **During a battle** `MapscreenCanPassItemToChar` refuses every transfer
  between characters when the loaded sector has enemies
  (`gTacticalStatus.fEnemyInSector`) — silently: clicking another character
  while holding an item does nothing. Proposal: show a message instead (e.g.
  the existing "You can't use this panel during combat." or a new text).
  Note: the flag is about the sector loaded in tactical, so a battle there
  blocks transfers on the map everywhere (original behaviour).
- **While travelling** transfers are only allowed within the same squad or
  vehicle (`bAssignment` equal). The vehicle soldier has its squad number as
  `bAssignment`, its passengers have `VEHICLE`, so passenger ↔ vehicle on the
  road would be refused — allow it when the passenger's `iVehicleId` is that
  vehicle.

## 5. Stages

1. **Inventory access** (map screen): gate change + slot restriction. Test:
   open the Hummer's inventory, move items from a merc and from the sector
   inventory, drive to another sector, take them out; save/load.
2. **Vehicle limits**: `slotLimitMultiplier`, `SoldierSlotLimit`, the three
   call sites. Test: stack 32 magazines in a big pocket and 16 in a small one
   of the Hummer; a merc still 8/4; weapons and brass knuckles still at the
   merc limit; money limits unchanged; destroy the vehicle in tactical and
   check the items drop.

Size of stages 1–2: small–medium; no save format change.

### Stage 3 — vehicle inventory in tactical (separate, independent)

Started **only after stages 1–2 are finished and tested** (V5). Purpose: use
the vehicle's cargo during a battle. Needs its own detailed analysis and
decisions before implementation; known starting points:

- The SM (inventory) panel is closed for non-human bodies
  (`Interface.cc`, `!IS_MERC_BODY_TYPE(s)` → team panel); the vehicle has no
  body figure, stats or hand buttons that make sense on that panel.
- Giving items to another character in tactical excludes vehicles
  (`Interface_Items.cc`, the give-item target check).
- Questions to decide then: who can reach the cargo (a passenger, a merc
  standing next to the vehicle), AP cost per transfer, what the panel shows
  for a vehicle, behaviour in turn-based combat and when the vehicle is
  destroyed.

Size of stage 3: medium–big, to be estimated in its own analysis.
