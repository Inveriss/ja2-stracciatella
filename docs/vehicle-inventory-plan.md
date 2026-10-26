# Plan: vehicle inventory (transporting items)

Status: **plan only**, not implemented (2026-10-26, user on Remote Control;
V9 for the helicopter added later the same day).
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
| V5 | Where can the vehicle inventory be opened? | **variant B**: map screen first, **and tactical as a completely separate, independent stage** (stage 4), started only after the map screen stages are finished |
| V6 | Which slots? | **pockets only**; hands and armour/head slots blocked |
| V7 | Damaged vehicle (condition < `OKLIFE` 15) | **inventory still accessible**, as recommended |
| V8 | Cargo when the vehicle is destroyed | **items inside are damaged by the explosion**, depending on its strength (section 4.5) |
| V8a | Damage strength | **100** = 2 × the vehicle explosion's blast damage (`GREAT_BIG_EXPLOSION` 50), the same factor the game uses for items lying at an explosion |
| V8b | Explosives in the cargo | **can go off in a chain reaction** (existing rule for mercs' items) |
| V8c | Broken items | **stay in the dropped pile as broken** (status below `USABLE`); in stacks, units at status 1 are removed (existing rule) |
| V9 | The helicopter (since it got its own soldier, plan docs/helicopter-team-list-repair-plan.md) | **it gets an openable inventory too**; when it crashes, **its cargo is lost completely** (section 4.6) |

Open (proposals for V9 details, not decided yet):

| # | Question | Proposal |
|---|---|---|
| V9a | When can items be moved to / from the helicopter? | only while it has **landed**, with anyone in its sector; and with **its own passengers** at any time, also in flight |
| V9b | Its `slotLimitMultiplier` | **4**, like the ground vehicles |

## 4. Design

### 4.1 Data

`vehicles.json`, new optional field per vehicle:

```
"slotLimitMultiplier": 4
```

`VehicleModel` gets `uint8_t slot_limit_multiplier` (default 1 when the field
is missing = merc limits). Set 4 for the Hummer and the ice cream truck (and the
other ground vehicles); the helicopter per V9b.

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

### 4.5 Cargo damage when the vehicle explodes (V8)

How it works today:

- A merc hit by an explosion gets item damage from
  `CheckEquipmentForDamage()` (`Items.cc`), called in `SoldierTakeDamage()`;
  per unit `DamageItem()` removes a random 0…(damage − 1) of status (halved
  for metal items, reduced by armour protection), ammo has a damage-% chance
  to be destroyed, jars smash, explosives can chain-react.
- **Vehicles skip this**: `SoldierTakeDamage()` returns early for a vehicle
  (`VehicleTakeDamage` path) before the item damage, and
  `CheckEquipmentForDamage()` itself returns at once for tanks.
- On destruction `HandleCriticalHitForVehicleInLocation()` ignites
  `GREAT_BIG_EXPLOSION` (blast damage 50, radius 5) at the vehicle; items
  are dropped on the ground later, when the corpse is made
  (`Rotting_Corpses.cc`). Items already lying on a tile hit by an explosion
  are damaged by `DamageItemOnGround()` with 2 × the wound amount.
- Enemy tanks are created with a tank cannon and an FN MINIMI at 80–100 %
  status (`Inventory_Choosing.cc`). The code has no special rule that damages
  a tank's items on its death; whether the wreck's items get extra damage from
  its own explosion depends on the order of the explosion and the drop —
  **not confirmed without a game test**.

Plan: when a player vehicle is destroyed, right after `fDestroyed` is set,
apply `CheckEquipmentForDamage(vehicle soldier, V8a damage)` (not for tanks)
to its inventory, then let the normal drop place what is left on the ground.

### 4.6 The helicopter's inventory (V9)

- Since stage 1 of the helicopter plan the helicopter has a soldier on the
  player's team (`IsHelicopterSoldier()`, strategic only), so the same gate
  change (4.4) opens its inventory; its sector follows the helicopter.
- Transfers (V9a): `MapscreenCanPassItemToChar` only checks the sector and
  between-sectors state, and the helicopter soldier's sector is the one it
  flies over. So for the helicopter add: items move only while it has landed
  (`!fHelicopterIsAirBorne`), except between the helicopter and its own
  passengers (`InHelicopter(s)`), who may do it in flight too. The sector
  inventory needs it landed as well.
- Crash (V9): `SkyriderDestroyed()` removes the helicopter soldier with
  `TacticalRemoveSoldier()` -- its inventory goes with it, nothing is
  dropped. That is the decided outcome, so nothing more is needed; just
  don't add a drop there. The cargo damage of 4.5 doesn't apply (no
  tactical explosion).
- Repair keeps working on the condition only; the inventory doesn't matter
  for flying (the helicopter has no weight model).

### 4.7 Map screen transfers: battle and travel

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

1. **Inventory access** (map screen): gate change + slot restriction, and the
   helicopter's transfer rule (V9a). Test: open the Hummer's inventory, move
   items from a merc and from the sector inventory, drive to another sector,
   take them out; the same with the helicopter (landed / in flight with its
   passengers); its crash loses the cargo; save/load.
2. **Vehicle limits**: `slotLimitMultiplier`, `SoldierSlotLimit`, the three
   call sites. Test: stack 32 magazines in a big pocket and 16 in a small one
   of the Hummer; a merc still 8/4; weapons and brass knuckles still at the
   merc limit; money limits unchanged.
3. *(before the tactical stage)* **Cargo damage** (V8): destroy a loaded
   vehicle in tactical; the items drop damaged, ammo partly destroyed,
   explosives may go off (V8b).

Size of stages 1–3: small–medium; no save format change.

### Stage 4 — vehicle inventory in tactical (separate, independent)

Started **only after stages 1–3 are finished and tested** (V5). Purpose: use
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

Size of stage 4: medium–big, to be estimated in its own analysis.
