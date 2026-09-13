#ifndef _MAP_INTERFACE_MAP_INVEN_H
#define _MAP_INTERFACE_MAP_INVEN_H

#include "Types.h"
#include "World_Items.h"

#include <vector>

// Compile-time array-sizing maximum for the sector-inventory pool's slot
// arrays -- 9 columns x 10 rows (the large strategic-screen tier's own page
// size). The actual per-page slot count in use is resolution-dependent (81
// for the compact tier, height 720-767) and resolved at runtime by
// GetMapInventoryPoolPageSize() (Map_Screen_Interface_Map_Inventory.cc) --
// use that, not this macro, everywhere except array declarations.
#define MAP_INVENTORY_POOL_SLOT_COUNT 90

INT32 GetMapInventoryPoolPageSize(void);

// whether we are showing the inventory pool graphic
extern BOOLEAN fShowMapInventoryPool;

// remove inventory pool graphic
void RemoveInventoryPoolGraphic( void );

// blit the inventory graphic
void BlitInventoryPoolGraphic( void );

// which buttons in map invneotyr panel?
void HandleButtonStatesWhileMapInventoryActive( void );

// handle creation and destruction of map inventory pool buttons
void CreateDestroyMapInventoryPoolButtons( BOOLEAN fExitFromMapScreen );

// bail out of sector inventory mode if it is on
void CancelSectorInventoryDisplayIfOn( BOOLEAN fExitFromMapScreen );

// handle flash of inventory items
void HandleFlashForHighLightedItem( void );

// the list for the inventory
extern std::vector<WORLDITEM> pInventoryPoolList;

// autoplace down object
void AutoPlaceObjectInInventoryStash(OBJECTTYPE* pItemPtr);

// the current inventory item
extern INT32 iCurrentlyHighLightedItem;
extern BOOLEAN fFlashHighLightInventoryItemOnradarMap;
extern INT16 sObjectSourceGridNo;
extern INT32 iCurrentInventoryPoolPage;
extern BOOLEAN fMapInventoryItemCompatable[ ];

BOOLEAN IsMapScreenWorldItemVisibleInMapInventory(const WORLDITEM& wi);

#endif
