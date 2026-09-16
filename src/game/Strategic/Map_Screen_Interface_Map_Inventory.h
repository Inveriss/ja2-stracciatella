#ifndef _MAP_INTERFACE_MAP_INVEN_H
#define _MAP_INTERFACE_MAP_INVEN_H

#include "Types.h"
#include "World_Items.h"

#include <vector>

// Compile-time array-sizing maximum for the sector-inventory pool's slot
// arrays -- 9 columns x 12 rows (the large strategic-screen tier's own,
// normal-mode page size, its own largest case). The actual per-page slot
// count in use is resolution- and "big images"-state-dependent (as low as
// 40, as high as 108) and resolved at runtime by GetMapInventoryPoolPageSize()
// (Map_Screen_Interface_Map_Inventory.cc) -- use that, not this macro,
// everywhere except array declarations.
#define MAP_INVENTORY_POOL_SLOT_COUNT 108

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

// Is the mouse cursor currently over the sector-inventory window's own
// on-screen area (main grid or stack-split popup -- both share the exact
// same rectangle, see g_sector_inv_box/g_stack_split_box in the .cc)?
// Used by Radar_Screen.cc's RenderBigRadarScreenIfVisible() -- the big
// minimap is only shown while this is true (in addition to
// fShowMapInventoryPool being TRUE and no item being on the cursor).
BOOLEAN IsCursorOverSectorInventoryWindow(void);

// Is the "stack split" popup (SECTOR_INVENTORY_STACK_*.sti) currently open?
// While it is, the big-minimap marker must come from
// GetHighlightedStackSplitSourceItem() below instead of iCurrentlyHighLightedItem
// -- the main grid's own slots are covered and no longer receive mouse
// events at all.
BOOLEAN IsStackSplitViewOpen(void);

// The WORLDITEM every slot in the "stack split" popup was split out of,
// while the cursor is hovering one of those slots -- else nullptr. See its
// own comment (Map_Screen_Interface_Map_Inventory.cc) for details.
WORLDITEM const* GetHighlightedStackSplitSourceItem(void);

// Sector-inventory "big images" toggle's persistence across new game/
// save/load -- see gfSectorInventoryBigImages's own comment
// (Map_Screen_Interface_Map_Inventory.cc) for the full story.
void InitSectorInventoryBigImagesForNewGame(void);
void SaveSectorInventoryBigImagesToSaveGameFile(void);
void LoadSectorInventoryBigImagesFromSaveGameFile(void);

// Sector-inventory category-filter mode toggle ("combine filters" checkbox)
// persistence across new game/save/load -- see
// gfSectorInventoryCombinableFilters's own comment
// (Map_Screen_Interface_Map_Inventory.cc) for the full story.
void InitSectorInventoryFilterModeForNewGame(void);
void SaveSectorInventoryFilterModeToSaveGameFile(void);
void LoadSectorInventoryFilterModeFromSaveGameFile(void);

#endif
