# Gun stacking plan

Status: **plan only, nothing implemented.** Written 2026-11-07 from an analysis of branch
`Item_Limits` at 53bc70897. Line numbers must be re-checked before implementation.

Related: [item-limits-plan.md](item-limits-plan.md). This is one more item limit, so it
belongs to the branch `Item_Limits`.

## Goal

Guns can be stacked in one slot of the **sector inventory** and of **vehicle cargo** like
other items, also when their **conditions differ**, at least when they are unloaded and
have no attachments. A merc's own inventory keeps one gun per slot.

## Current state (analysis)

### 1. The data: no gun has room for a second unit

`ItemSlotLimit()` (Items.cc ~297) gives the stack size of a slot from the item's
`ubPerPocket` (halved for small pockets; hands are always 1). In `weapons.json`
38 guns have `ubPerPocket` 0 (big items) and 9 pistols have 1, so every slot holds one
gun. The sector inventory uses the same value (`ItemSlotLimit(item, BIGPOCK1POS)`).

`ubPerPocket` also means something else for guns: 0 = a big item. Code relies on it:
AI (`AIUtils.cc` ~2031), the enemies' equipment (`Inventory_Choosing.cc` ~1623), the
second hand (`Items.cc` ~2135), IMP (`IMP_Confirm.cc` ~421), the editor. Raising it to
stack guns would change those meanings too.

### 2. The code: a gun has no per-unit state

`OBJECTTYPE` (Item_Types.h) keeps a stack's per-unit condition in the union member
`bStatus[MAX_OBJECTS_PER_SLOT]`. A gun uses the start of the same union for its own
state:

| union byte | gun field |
|---|---|
| 0 | `bGunStatus` (condition) |
| 1 | `ubGunAmmoType` |
| 2 | `ubGunShotsLeft` |
| 3 | (padding) |
| 4-5 | `usGunAmmoItem` |
| 6 | `bGunAmmoStatus` (jam) |

`usAttachItem[]` / `bAttachStatus[]` are one set for the whole object. So a gun stack
can't store a condition per unit: writing unit 2's condition to `bStatus[1]` would
overwrite the ammo type, unit 3's would overwrite the shots left, and so on.

Commit 015391349 (2026-09-12, "Allow safe gun stacking via a clean-identical-stack
invariant") already handles this with a rule: two guns may share one object only when
they are **identical** in everything, condition included (`CanGunsStack()`,
Items.cc ~670). For guns `StackObjs()`, `RemoveObjFrom()` and `GetObjFrom()` only change
the count (Items.cc ~1071-1193). That is why two guns of different condition never
stack, even unloaded and bare.

### 3. Other places that assume one gun per slot

- Weight: `CalculateCarriedWeight()` (Items.cc ~1038) and `Inventory_Choosing.cc` ~538
  multiply by the count only when `ubPerPocket > 1`; a gun stack would weigh as one gun.
- 45 places read a unit's status as `bStatus[i]` (Items.cc 29, Vehicles.cc 3,
  Assignments.cc 3 (repair), ShopKeeper_Interface.cc 2, Map_Screen_Interface_Map_Inventory.cc 2,
  World_Items.cc, Interface_Utils.cc, Interface_Items.cc, Drugs_And_Alcohol.cc,
  Arms_Dealer_Init.cc, LoadSaveObjectType.cc 1 each); for a gun stack those would read
  ammo bytes.
- `ReloadGun()` already refuses a gun stack (Items.cc ~1394); hands always hold one item.

## Design

### Per-unit condition after the gun's own fields

The union's gun branch gets the free bytes after `bGunAmmoStatus`:

```cpp
struct
{
	INT8   bGunStatus;          // condition of unit 0
	UINT8  ubGunAmmoType;
	UINT8  ubGunShotsLeft;
	UINT16 usGunAmmoItem;
	INT8   bGunAmmoStatus;
	INT8   bGunUnitStatus[MAX_OBJECTS_PER_SLOT - 7]; // condition of units 1, 2, ...
};
```

Unit 0's condition stays `bGunStatus`; unit k's is `bGunUnitStatus[k - 1]`. The union
stays 100 bytes, so `sizeof(OBJECTTYPE)` and the static_assert don't change. Two
helpers give every item's unit status: `UnitStatus(o, i)` / `SetUnitStatus(o, i, v)` —
`bStatus[i]` for other items, the mapping above for guns. A gun stack holds at most 94
units (the free bytes); the gun's stack size is capped there.

### Stacking rule

Guns stack when they are equal in everything **except the condition**: the same gun,
ammo type, shots left, magazine item, jam state and attachments. So:

- unloaded guns without attachments stack whatever their condition (the main case);
- identically loaded guns stack too (every unit holds the same ammo, as today);
- a gun with an attachment stacks only with the same attachment of the same condition.

Taking a unit out gives a normal single gun: unit 0's ammo/attachment state, the unit's
own condition.

## Decisions

All decisions were taken by the user on 2026-11-07 (G1, G2, G4 as recommended).

| ID | Decision |
|----|----------|
| G1 | Per-unit condition in `bGunUnitStatus[]` after the gun fields, `UnitStatus()` / `SetUnitStatus()` for every item |
| G2 | Stacking rule: equal in everything except the condition (see above) |
| G3 | A gun's stack size: its own `"stackSize"` in weapons.json, else the game.json default `gun_stack_size` (5); both ways together. `ubPerPocket` keeps its "big item" meaning |
| G4 | Weight of a gun stack = one gun's weight (with its ammo and attachments) x the count |
| G5 | At most 94 guns per stack (the union's free bytes) |
| G6 | `SAVE_GAME_VERSION` bump: the bytes after the gun fields get a meaning (project rule: new game) |
| G7 | **Merc inventory unchanged:** one gun per slot, big and small pockets, and the hands |
| G8 | **Sector inventory:** a slot holds up to the gun's stack size (G3) |
| G9 | **Vehicle cargo:** its own limits for guns per big and per small cargo slot, per vehicle in vehicles.json (`gunStackBigPocket`, `gunStackSmallPocket`; default: big = the gun's stack size, small = 1). A slot holds the smaller of the vehicle's limit and the gun's stack size; a small slot only takes guns that fit a small pocket at all (as today). The existing `slotLimitMultiplier` keeps leaving guns out |

### Slot limits of a gun

| Where | Big slot | Small slot |
|---|---|---|
| Merc (pockets, hands) | 1 | 1 (if it fits a small pocket) |
| Sector inventory | stack size (G3) | - |
| Vehicle cargo | min(`gunStackBigPocket`, stack size) | min(`gunStackSmallPocket`, stack size), if it fits a small pocket |

## Implementation stages

### Stage 1 - data model and stack operations

- Item_Types.h: `bGunUnitStatus[]`, `UnitStatus()` / `SetUnitStatus()`.
- Items.cc: `CanGunsStack()` without the condition check; `StackObjs()`,
  `RemoveObjFrom()`, `GetObjFrom()`, `RemoveObjs()` keep and move the per-unit condition
  of guns (unit 0 stays in `bGunStatus`).
- Unit tests: stacking guns of different conditions and splitting them back keeps every
  condition and leaves the ammo bytes untouched; loaded or attached guns that differ
  don't stack.

### Stage 2 - stack sizes and weight

- weapons.json field `stackSize` (schema), game.json `gun_stack_size`; vehicles.json
  `gunStackBigPocket` / `gunStackSmallPocket` (schema, VehicleModel).
- A gun's slot limit by place (G7-G9, table above), capped at 94: the merc's own
  inventory stays at `ItemSlotLimit()` (1); the sector inventory and
  `SoldierSlotLimit()` for a vehicle use the new values.
- Weight (G4) in `CalculateCarriedWeight()` and the enemies' equipment.
- Sector inventory and vehicle cargo merges (`Map_Screen_Interface_Map_Inventory.cc`
  ~2108, ~2662, ~3871) with the new rule.

### Stage 3 - every per-unit status read

- The 45 `bStatus[i]` places use `UnitStatus()`: repair (Assignments.cc), water damage,
  the stack popup, dealers (ShopKeeper splits a stack into units, each with its own
  condition and price), arms dealer intake, vehicle cargo, world items, status bars.

### Stage 4 - interface

- The stack count badge and per-unit condition bars for gun stacks; the description
  box of a stack describes unit 0; the ammo count only for a loaded stack.

### Stage 5 - test data and game verification

- `stackSize` for a few guns in the test mod.
- Checks: a merc still holds one gun per slot; unload two Glocks of different condition
  and put them on one sector slot and on one vehicle cargo slot (big; small for
  pistols, up to the vehicle's limits); split them back (each keeps its condition); loaded and attached guns that
  differ stay apart; weight grows with the count; sell a stack to a dealer (one price per
  condition); repair a stack; save and load; enemies' dropped guns, Bobby Ray deliveries
  (a stack of new guns), militia/AI taking a gun from a stacked pocket.

## Consequences and limits

- With the merc inventory unchanged (G7), most of the per-unit status reads of stage 3
  only matter in the sector inventory, vehicle cargo, shops and repair.

- Only the condition is per unit. Guns with different ammo or attachments never share a
  slot; a loaded stack means every gun holds the same ammo (as today).
- A merc's own slots always hold one gun, so mercs, militia and the AI never carry a
  gun stack; stacks only exist in the sector inventory, vehicle cargo and shops.
- Code that reads `ubGunShotsLeft` etc. of a stack sees unit 0, which is right because
  every unit has the same ammo state.
- Existing saves can't be loaded (G6).

## Open questions

- None; G1-G9 are decided. The default vehicle values of G9 (big = stack size,
  small = 1) can be changed per vehicle in vehicles.json.
