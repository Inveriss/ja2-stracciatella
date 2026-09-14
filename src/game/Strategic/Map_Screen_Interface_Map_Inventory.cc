#include "Auto_Resolve.h"
#include "Directories.h"
#include "Font.h"
#include "HImage.h"
#include "Handle_Items.h"
#include "Interface.h"
#include "Isometric_Utils.h"
#include "ItemModel.h"
#include "Map_Screen_Interface_Bottom.h"
#include "Map_Screen_Interface_Map_Inventory.h"
#include "MessageBoxScreen.h"
#include "Object_Cache.h"
#include "Timer_Control.h"
#include "UILayout.h"
#include "VObject.h"
#include "SysUtil.h"
#include "Map_Screen_Interface_Border.h"
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Map.h"
#include "Items.h"
#include "Interface_Items.h"
#include "Interface_Utils.h"
#include "Text.h"
#include "Font_Control.h"
#include "StrategicMap.h"
#include "World_Items.h"
#include "Tactical_Save.h"
#include "Soldier_Control.h"
#include "English.h"
#include "MapScreen.h"
#include "Radar_Screen.h"
#include "Interface_Panels.h"
#include "WordWrap.h"
#include "Button_System.h"
#include "ScreenIDs.h"
#include "VSurface.h"
#include "ShopKeeper_Interface.h"
#include "ArmsDealerInvInit.h"

#include "ContentManager.h"
#include "GameInstance.h"

#include <string_theory/format>
#include <string_theory/string>

#include <algorithm>
#include <climits>
#include <vector>

// status bar colors
#define DESC_STATUS_BAR FROMRGB( 201, 172,  133 )
#define DESC_STATUS_BAR_SHADOW FROMRGB( 140, 136,  119 )

// delay for flash of item
#define DELAY_FOR_HIGHLIGHT_ITEM_FLASH 200

// inventory slot font -- still used for the footer labels
// (DrawTextOnMapInventoryBackground()), not the item name below
#define MAP_IVEN_FONT						SMALLCOMPFONT

// dedicated font for the item name printed inside each sector-inventory
// slot (Data/Fonts/font_sector_inv.sti), kept separate from MAP_IVEN_FONT
// so it doesn't affect the unrelated footer labels that still use it
#define MAP_SECTOR_INV_ITEM_FONT			FONTSECTORINV

// Sector-inventory "big images" toggle (MapInventoryPoolBigImagesBtn()) --
// swaps every visible item icon, on BOTH this window and the stack split
// popup, from its usual MDITEMS graphic to its BIGITEMS one, and shrinks the
// grid from 9 columns to 5 (GetInventoryGridCols() below) and to 7 rows
// (GetInventoryGridRows() below) on both, regardless of the active
// resolution tier -- per user request. Reset to FALSE every time the panel
// (re)opens (CreateDestroyMapInventoryPoolButtons()), same convention as
// gubSectorInventoryActiveFilters.
static BOOLEAN gfSectorInventoryBigImages = FALSE;

// inventory pool slot positions and sizes. Column count (ROW X): 5 while
// gfSectorInventoryBigImages is on, else always 9. Row count (COL Y): 7
// while gfSectorInventoryBigImages is on (both resolution tiers alike, per
// user request); otherwise resolution-dependent as before -- 10 for the
// large strategic-screen tier (height 768+), 9 for the compact tier (height
// 720-767). Neither is known at static-initialization time, so both are
// resolved at runtime, on every call, same reasoning as
// GetMapInventoryPoolBackgroundFilename() below. Shared by the stack split
// grid too (GetStackSplitPageSize()), which uses the identical rule.
static INT32 GetInventoryGridCols(void)
{
	return gfSectorInventoryBigImages ? 5 : 9;
}
static INT32 GetInventoryGridRows(void)
{
	if (gfSectorInventoryBigImages) return 7;
	return g_ui.isCompactStrategicScreen() ? 9 : 10;
}
#define MAP_INV_SLOT_ROWS GetInventoryGridRows()

// The sector-inventory pool's actual per-page slot count, for the active
// resolution/big-images state (35 = 5x7 big images, 81 = 9x9 compact, 90 =
// 9x10 large -- see GetInventoryGridCols()/GetInventoryGridRows() above).
// Declared in Map_Screen_Interface_Map_Inventory.h and used by other files
// (MapScreen.cc, Interface_Items.cc, Radar_Screen.cc) wherever they used to
// reference MAP_INVENTORY_POOL_SLOT_COUNT directly. MAP_INVENTORY_POOL_SLOT_COUNT
// itself (Map_Screen_Interface_Map_Inventory.h) stays a plain compile-time
// constant -- it's only the array-sizing maximum (the large tier's own 90)
// now, not the per-page count actually in use.
INT32 GetMapInventoryPoolPageSize(void)
{
	return GetInventoryGridCols() * GetInventoryGridRows();
}

// Extra Y offset applied to the sector-inventory footer's Done buttons,
// arrow buttons, and text/value boxes (both windows) at the compact
// strategic-screen tier (height 720-767), per user request -- 0 at the
// large tier (height 768+), so the large-tier positions below are
// unaffected. Same runtime-resolved reasoning as GetInventoryGridRows()/
// GetMapInventoryPoolBackgroundFilename() above.
static INT32 CompactFooterYOffset(INT32 const offset)
{
	return g_ui.isCompactStrategicScreen() ? offset : 0;
}
// Done buttons (both windows), per user request.
#define COMPACT_DONE_BUTTON_Y_OFFSET CompactFooterYOffset(-52)
// Everything else in the footer (both windows): text labels, values, arrow
// buttons, per user request.
#define COMPACT_FOOTER_TEXT_Y_OFFSET CompactFooterYOffset(-48)


static const SGPBox g_sector_inv_box        = { 261,   0, 762, 768 };
static const SGPBox g_sector_inv_title_box  = { 266,   5, 370,  29 };
static const SGPBox g_sector_inv_slot_box   = { 274,  37,  78,  52 };
static const SGPBox g_sector_inv_region_box = {   27,   64,  67,  33 }; // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_item_box   = {   27,   64,  67,  33 }; // relative to g_sector_inv_slot_box
// x is intentionally UINT16(-1) (== 65535, wrapping) to shift the bar 1px
// left of the item box -- SGPBox's fields are unsigned so a plain -1
// literal here would silently narrow (MSVC C4838). The explicit cast keeps
// the exact same value (and thus the exact same on-screen position, since
// it's added to dx and truncated back down to INT16 in
// DrawItemUIBarEx()'s sXPos parameter, which cancels the wraparound out to
// dx - 1) while making the intent clear and silencing the warning.
static const SGPBox g_sector_inv_bar_box    = { (UINT16)21,   66,   2,  31 }; // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_name_box   = {   22,  100,  75,   10 }; // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_loc_box    = { 450, 628,  39,  10 };
static const SGPBox g_sector_inv_count_box  = { 570, 628,  39,  10 };
static const SGPBox g_sector_inv_page_box   = { 657, 628,  50,  10 };

// "Big images" toggle's own slot geometry (5x7 grid instead of 9 cols x
// 9-or-10 rows) -- PLACEHOLDER, not yet the final layout: same top-left
// origin as g_sector_inv_slot_box above, pitch recomputed for 5 columns x 7
// rows over roughly the same overall grid footprint, and the sub-boxes
// below scaled up from g_sector_inv_region_box/_item_box/_bar_box/_name_box
// by the same factor. Needs visual tuning once SECTOR_INVENTORY_1024_BIG.sti/
// SECTOR_INVENTORY_1280_BIG.sti actually exist.
static const SGPBox g_sector_inv_slot_box_big   = { 274,  37, 133,  69 };
static const SGPBox g_sector_inv_region_box_big = {  31,  67, 120,  47 }; // relative to g_sector_inv_slot_box_big
static const SGPBox g_sector_inv_item_box_big   = {  31,  67, 120,  47 }; // relative to g_sector_inv_slot_box_big
static const SGPBox g_sector_inv_bar_box_big    = {  21,  70,   4,  44 }; // relative to g_sector_inv_slot_box_big
static const SGPBox g_sector_inv_name_box_big   = {  22, 118, 135,  14 }; // relative to g_sector_inv_slot_box_big

// Small helpers picking the right slot-geometry set for the current
// gfSectorInventoryBigImages state -- keeps the branch in one place instead
// of scattering it across every render/slot-creation call site.
static SGPBox const& GetSectorInvSlotBox(void)   { return gfSectorInventoryBigImages ? g_sector_inv_slot_box_big   : g_sector_inv_slot_box; }
static SGPBox const& GetSectorInvRegionBox(void) { return gfSectorInventoryBigImages ? g_sector_inv_region_box_big : g_sector_inv_region_box; }
static SGPBox const& GetSectorInvItemBox(void)   { return gfSectorInventoryBigImages ? g_sector_inv_item_box_big   : g_sector_inv_item_box; }
static SGPBox const& GetSectorInvBarBox(void)    { return gfSectorInventoryBigImages ? g_sector_inv_bar_box_big    : g_sector_inv_bar_box; }
static SGPBox const& GetSectorInvNameBox(void)   { return gfSectorInventoryBigImages ? g_sector_inv_name_box_big   : g_sector_inv_name_box; }


// the current highlighted item
INT32 iCurrentlyHighLightedItem = -1;
BOOLEAN fFlashHighLightInventoryItemOnradarMap = FALSE;

// whether we are showing the inventory pool graphic
BOOLEAN fShowMapInventoryPool = FALSE;

// Not a plain cache_key_t constant: which file this is depends on the
// active resolution (see UILayout::isCompactStrategicScreen()), which isn't
// known yet at static-initialization time, so the choice has to be resolved
// at runtime, on every call. Suffix convention: _1280 for the compact
// strategic-screen tier (height 720-767), _1024 for the large tier (height
// 768+) -- see GetMapBorderGraphicsFilename() in Map_Screen_Interface_Border.cc
// for the same pattern.
static cache_key_t GetMapInventoryPoolBackgroundFilename(void)
{
	if (gfSectorInventoryBigImages)
	{
		return g_ui.isCompactStrategicScreen()
			? INTERFACEDIR "/SECTOR_INVENTORY_1280_BIG.sti"
			: INTERFACEDIR "/SECTOR_INVENTORY_1024_BIG.sti";
	}
	return g_ui.isCompactStrategicScreen()
		? INTERFACEDIR "/Sector_Inventory_1280.sti"
		: INTERFACEDIR "/Sector_Inventory_1024.sti";
}

// "Group Items" button -- see GroupSectorInventoryItems()/CreateMapInventoryGroupButton().
// Like DONE_BUTTON_Inventory.STI/map_screen_bottom_arrows.sti above,
// QuickCreateButtonImg() manages this image's lifetime itself; no
// cache_key_t needed here.
#define GROUP_BUTTON_READY   0
#define GROUP_BUTTON_PRESSED 1
// Placeholder position, per user request -- not yet the final layout.
#define GROUP_BUTTON_X 288
#define GROUP_BUTTON_Y 32

// Category-filter buttons -- "Wszystkie przedmioty" (clears every active
// filter) plus one toggle per category, all 7 persistent-state toggles per
// user request. Same sector_inventory_bookmarks.sti sheet as GROUP_BUTTON
// above, occupying the next sequential sub-image pairs per user instruction
// -- placeholder order/positions (this file's own precedent for
// GROUP_BUTTON_X/Y), not yet the final asset layout. Each button is
// 55x50px, chained 3px apart starting right after "Grupuj przedmioty", per
// user request.
#define ALL_ITEMS_BUTTON_OFF     2
#define ALL_ITEMS_BUTTON_ON      3
#define FILTER_WEAPONS_OFF       4
#define FILTER_WEAPONS_ON        5
#define FILTER_ATTACHMENTS_OFF   6
#define FILTER_ATTACHMENTS_ON    7
#define FILTER_AMMO_OFF          8
#define FILTER_AMMO_ON           9
#define FILTER_ARMOUR_OFF        10
#define FILTER_ARMOUR_ON         11
#define FILTER_EXPLOSIVES_OFF    12
#define FILTER_EXPLOSIVES_ON     13
#define FILTER_OTHER_OFF         14
#define FILTER_OTHER_ON          15

#define FILTER_BUTTON_WIDTH 55
#define FILTER_BUTTON_GAP    3
#define FILTER_BUTTON_STEP  (FILTER_BUTTON_WIDTH + FILTER_BUTTON_GAP)

#define ALL_ITEMS_BUTTON_X    (GROUP_BUTTON_X + FILTER_BUTTON_STEP + 1)
#define FILTER_WEAPONS_X      (GROUP_BUTTON_X + 2 * FILTER_BUTTON_STEP)
#define FILTER_ATTACHMENTS_X  (GROUP_BUTTON_X + 3 * FILTER_BUTTON_STEP - 1)
#define FILTER_AMMO_X         (GROUP_BUTTON_X + 4 * FILTER_BUTTON_STEP - 2)
#define FILTER_ARMOUR_X       (GROUP_BUTTON_X + 5 * FILTER_BUTTON_STEP - 1)
#define FILTER_EXPLOSIVES_X   (GROUP_BUTTON_X + 6 * FILTER_BUTTON_STEP - 1)
#define FILTER_OTHER_X        (GROUP_BUTTON_X + 7 * FILTER_BUTTON_STEP - 3)
#define FILTER_BUTTONS_Y      GROUP_BUTTON_Y

// "Big images" toggle -- per user request: 3px after "Pokaż różne/pozostałe"
// (FILTER_OTHER_X), same sector_inventory_bookmarks.sti sheet, next
// sequential sub-image pair (20/21). Persistent-state toggle, same as the 7
// category filters above (QuickCreateFilterToggleButton()) -- see
// MapInventoryPoolBigImagesBtn().
#define BIG_IMAGES_BUTTON_OFF 20
#define BIG_IMAGES_BUTTON_ON  21
#define BIG_IMAGES_BUTTON_X   (FILTER_OTHER_X + FILTER_BUTTON_STEP)

// Two more action buttons (momentary, like GROUP_BUTTON -- not toggles),
// per user request: transfer items between the selected soldier's own
// inventory (MapInv.sti/ItemInfoC.sti) and the sector-inventory stash.
// Same sheet, next sequential sub-image pair each, continuing the
// placeholder chain above -- not yet the final layout.
#define MOVE_TO_SECTOR_READY   16
#define MOVE_TO_SECTOR_PRESSED 17
#define MOVE_TO_MERC_READY     18
#define MOVE_TO_MERC_PRESSED   19
#define MOVE_TO_SECTOR_X (GROUP_BUTTON_X + 8 * FILTER_BUTTON_STEP + 189)
#define MOVE_TO_MERC_X   (GROUP_BUTTON_X + 9 * FILTER_BUTTON_STEP + 73)

// Bitmask of active category filters. "Wszystkie przedmioty" is a plain
// peer bit like the other 6, not a special reset button -- per user
// request, every one of the 7 toggles independently, with no bit
// special-cased to force itself back on or to clear the others. An item
// shows if ANY active bit matches it (a union, not an intersection); when
// SECTOR_INV_FILTER_ALL is one of the active bits, every item matches
// regardless of its own category (see GetSectorInventoryFilterCategory()'s
// caller, SplitPoolListByFilter()). With every bit off (0), nothing
// matches at all -- an intentional, reachable "show nothing" state, per
// user request -- rather than 0 being a sentinel for "show everything".
#define SECTOR_INV_FILTER_WEAPONS     0x01
#define SECTOR_INV_FILTER_ATTACHMENTS 0x02
#define SECTOR_INV_FILTER_AMMO        0x04
#define SECTOR_INV_FILTER_ARMOUR      0x08
#define SECTOR_INV_FILTER_EXPLOSIVES  0x10
#define SECTOR_INV_FILTER_OTHER       0x20
#define SECTOR_INV_FILTER_ALL         0x40
static UINT8 gubSectorInventoryActiveFilters = SECTOR_INV_FILTER_ALL;

// inventory pool list
std::vector<WORLDITEM> pInventoryPoolList;

// current page of inventory
INT32 iCurrentInventoryPoolPage = 0;
static INT32 iLastInventoryPoolPage = 0;

// Size of the visible (matching, already padded to a whole number of
// pages) prefix of pInventoryPoolList -- kept in sync by
// RebuildFilteredInventoryPoolList()/BuildStashForSelectedSector(), which
// are what decide where the visible/hidden-tail boundary sits while a
// category filter is active. Equals pInventoryPoolList.size() whenever no
// filter is active (no hidden tail exists). Every place that (re)computes
// iLastInventoryPoolPage must derive it from this, never from
// pInventoryPoolList.size() directly -- doing the latter previously let
// CheckAndUnDateSlotAllocation() (called every frame) silently re-widen
// pagination to cover the hidden tail again right after a filter had
// capped it, exactly undoing the filter on the very next page turn.
static size_t gVisibleInventorySlotCount = 0;

INT16 sObjectSourceGridNo = 0;

// the inventory slots
static MOUSE_REGION MapInventoryPoolSlots[MAP_INVENTORY_POOL_SLOT_COUNT];
// How many of the above were actually MSYS_DefineRegion()'d by the last
// CreateMapInventoryPoolSlots() call -- see its own and
// DestroyMapInventoryPoolSlots()'s comments.
static UINT gMapInventoryPoolSlotsCreatedCount = 0;
static MOUSE_REGION MapInventoryPoolMask;
BOOLEAN fMapInventoryItemCompatable[ MAP_INVENTORY_POOL_SLOT_COUNT ];
static BOOLEAN      fChangedInventorySlots = FALSE;

// the unseen items list...have to save this
static std::vector<WORLDITEM> pUnSeenItems;

UINT32 guiFlashHighlightedItemBaseTime = 0;
UINT32 guiCompatibleItemBaseTime = 0;

// [0] = next page, [1] = previous page, [2] = done, [3] = group items,
// [4] = all items (clears filters), [5] = weapons, [6] = attachments,
// [7] = ammo, [8] = armour, [9] = explosives, [10] = other,
// [11] = move to sector, [12] = move to merc, [13] = big images toggle
static GUIButtonRef guiMapInvenButton[14];

static BOOLEAN gfCheckForCursorOverMapSectorInventoryItem = FALSE;


// ---------------------------------------------------------------------
// "Stack split view" (Wariant B) -- right-clicking a stack (ubNumberOfObjects
// > 1) opens this small independent window instead of a second
// sector_inventory.sti (the old InitSectorInventoryStackPopup(), now
// removed). It shows each item of the stack in its own slot, using its own
// background art (newgoldpiece3.sti) and reuses ItemInfoC.sti
// (MAPInternalInitItemDescriptionBox()) for the per-item description, same
// as the main sector-inventory grid does for a single item.
//
// The stack is PHYSICALLY split into up to MAX_OBJECTS_PER_SLOT separate
// 1-count OBJECTTYPEs (gStackSplitItems) for as long as this view is open --
// the source slot in pInventoryPoolList sits empty in the meantime -- and
// CloseStackSplitView() re-merges them back into a single stack, so the
// split is never permanent. Per user request: "przedmioty ze stosu mają
// zostać fizycznie rozdzielone na osobne OBJECTTYPE w nowej, małej liście
// (wymaga logiki ponownego scalenia przy zamknięciu, żeby nie rozbić stosu
// na trwałe)".
// ---------------------------------------------------------------------

// Not a plain cache_key_t constant: same resolution-dependent runtime choice
// as GetMapInventoryPoolBackgroundFilename() above -- _1280 for the compact
// strategic-screen tier (height 720-767), _1024 for the large tier (height
// 768+).
static cache_key_t GetStackSplitBackgroundFilename(void)
{
	if (gfSectorInventoryBigImages)
	{
		return g_ui.isCompactStrategicScreen()
			? INTERFACEDIR "/SECTOR_INVENTORY_Second_1280_BIG.sti"
			: INTERFACEDIR "/SECTOR_INVENTORY_SECOND_1024_BIG.sti";
	}
	return g_ui.isCompactStrategicScreen()
		? INTERFACEDIR "/Sector_Inventory_Second_1280.sti"
		: INTERFACEDIR "/Sector_Inventory_Second_1024.sti";
}

// Placeholder positions, per user request -- not yet the final layout.
// g_stack_split_box is the whole window (background + slots); the slot box
// is the pitch between slots, and the rest are relative to each individual
// slot -- same layering as g_sector_inv_slot_box/_region_box/_item_box/etc.
// above.
static const SGPBox g_stack_split_box        = { 261, 0, 762, 768 };
static const SGPBox g_stack_split_slot_box   = {  10,  30,  78,  52 };
static const SGPBox g_stack_split_region_box = {  27,  71,  67,  33 }; // relative to g_stack_split_slot_box
static const SGPBox g_stack_split_item_box   = {  27,  71,  67,  33 }; // relative to g_stack_split_slot_box
static const SGPBox g_stack_split_bar_box    = { (UINT16)24, 72, 2, 31 }; // relative to g_stack_split_slot_box
static const SGPBox g_stack_split_name_box   = {   25,  107,  70,  10 }; // relative to g_stack_split_slot_box

// "Big images" toggle's own slot geometry -- PLACEHOLDER, not yet the final
// layout, same reasoning/scaling as g_sector_inv_slot_box_big above (5x7
// grid, same pitch as the main grid's own big-mode slot box since both
// share the identical column/row rule).
static const SGPBox g_stack_split_slot_box_big   = {  10,  30, 140,  74 };
static const SGPBox g_stack_split_region_box_big = {  48, 101, 120,  47 }; // relative to g_stack_split_slot_box_big
static const SGPBox g_stack_split_item_box_big   = {  48, 101, 120,  47 }; // relative to g_stack_split_slot_box_big
static const SGPBox g_stack_split_bar_box_big    = {  43, 102,   4,  44 }; // relative to g_stack_split_slot_box_big
static const SGPBox g_stack_split_name_box_big   = {  45, 152, 126,  14 }; // relative to g_stack_split_slot_box_big

// Same idea as GetSectorInvSlotBox()/etc. above, for the stack split grid.
static SGPBox const& GetStackSplitSlotBox(void)   { return gfSectorInventoryBigImages ? g_stack_split_slot_box_big   : g_stack_split_slot_box; }
static SGPBox const& GetStackSplitRegionBox(void) { return gfSectorInventoryBigImages ? g_stack_split_region_box_big : g_stack_split_region_box; }
static SGPBox const& GetStackSplitItemBox(void)   { return gfSectorInventoryBigImages ? g_stack_split_item_box_big   : g_stack_split_item_box; }
static SGPBox const& GetStackSplitBarBox(void)    { return gfSectorInventoryBigImages ? g_stack_split_bar_box_big    : g_stack_split_bar_box; }
static SGPBox const& GetStackSplitNameBox(void)   { return gfSectorInventoryBigImages ? g_stack_split_name_box_big   : g_stack_split_name_box; }

// Placeholder position, per user request -- not yet the final layout.
#define STACK_SPLIT_DONE_X 762
#define STACK_SPLIT_DONE_Y (632 + COMPACT_DONE_BUTTON_Y_OFFSET)

// Slots laid out in a small grid, wide enough for a whole stack (a stack
// can never hold more than MAX_OBJECTS_PER_SLOT items to begin with).
// ROW X = 9 per user request, matching the main sector-inventory grid's
// own column count (MAP_INV_SLOT_ROWS' column count above).
#define STACK_SPLIT_COLS 9
// ROW Y = 10, matching the main grid's own row count (MAP_INV_SLOT_ROWS) --
// a page therefore holds 90, same as the main grid's own page size
// (GetMapInventoryPoolPageSize()). A full MAX_OBJECTS_PER_SLOT (100) stack
// no longer overflows the window (rows 11/12 past the visible area, per
// user report) -- it spans 2 independent pages instead, per user request.
// STACK_SPLIT_ROWS/STACK_SPLIT_PAGE_SIZE are the compile-time maximum (10
// rows / 90), used only to size gStackSplitSlots[] below -- the actual
// per-page slot count in use is resolution-dependent (81 for the compact
// tier, same rule as the main grid's own GetInventoryGridRows() above), see
// GetStackSplitPageSize().
#define STACK_SPLIT_ROWS 10
#define STACK_SPLIT_PAGE_SIZE (STACK_SPLIT_COLS * STACK_SPLIT_ROWS)

static INT32 GetStackSplitPageSize(void)
{
	return GetInventoryGridCols() * GetInventoryGridRows();
}

// Independent pagination controls, per user request -- own page state and
// own next/prev arrows, entirely separate from the main grid's own
// iCurrentInventoryPoolPage/iLastInventoryPoolPage. Placeholder positions
// (reusing the main grid's own map_screen_bottom_arrows.sti sub-images and
// a page-count box the same shape as g_sector_inv_page_box), not yet the
// final layout.
#define STACK_SPLIT_PREV_X 638
#define STACK_SPLIT_NEXT_X 711
#define STACK_SPLIT_ARROWS_Y (626 + COMPACT_FOOTER_TEXT_Y_OFFSET)
static const SGPBox g_stack_split_page_box = { 657, 628, 50, 10 };

// "Total Items" label + value, independent of the main grid's own
// (pMapInventoryStrings[1]/g_sector_inv_count_box) -- reuses the same
// already-localized string and the same relative X/Y the main grid uses
// for it (DrawTextOnMapInventoryBackground()), since this window shares
// the same overall box width (762) and doesn't otherwise use that space.
// Placeholder positions, per user request -- not yet the final layout.
#define STACK_SPLIT_TOTAL_TEXT_X 506
#define STACK_SPLIT_TOTAL_TEXT_Y (634 + COMPACT_FOOTER_TEXT_Y_OFFSET)
static const SGPBox g_stack_split_count_box = { 572, 628, 39, 10 };

// The physically-split-out items, one per slot -- empty (gStackSplitItems
// cleared) when the view is closed.
static std::vector<OBJECTTYPE> gStackSplitItems;
// Index into pInventoryPoolList (absolute -- already includes the page
// offset) of the stack currently split open here, or -1 when closed.
static INT32 gStackSplitSourceIndex = -1;
// This window's own, independent page state -- reset to 0 every time it
// opens (OpenStackSplitView()). gLastStackSplitPage is recomputed there
// too, from gStackSplitItems.size() (fixed for as long as the view stays
// open -- items become NOTHING as they're picked up, but the vector itself
// is never resized until CloseStackSplitView()).
static INT32 gCurrentStackSplitPage = 0;
static INT32 gLastStackSplitPage    = 0;
static MOUSE_REGION gStackSplitSlots[STACK_SPLIT_PAGE_SIZE];
// Background region: purely a click-blocker so a stray click inside the
// window's background doesn't fall through to the main sector-inventory
// grid underneath it -- does NOT close the view. Per user request, this
// window closes only via its own Done button (gStackSplitDoneButton)
// below, not via left/right click.
static MOUSE_REGION gStackSplitBackgroundRegion;
static GUIButtonRef gStackSplitDoneButton;
static GUIButtonRef gStackSplitPrevBtn;
static GUIButtonRef gStackSplitNextBtn;


// remove background panel graphics for inventory
void RemoveInventoryPoolGraphic( void )
{
	RemoveVObject(GetMapInventoryPoolBackgroundFilename());
}


static void CheckAndUnDateSlotAllocation(void);
static void DisplayCurrentSector(void);
static void DisplayPagesForMapInventoryPool(void);
static void DrawNumberOfInventoryPoolItems();
static void DrawTextOnMapInventoryBackground(void);
static size_t GetTotalNumberOfItemsInStackSplit(void);
static void RenderItemsForCurrentPageOfInventoryPool(void);
static void RenderStackSplitItems(void);
static void UpdateHelpTextForInvnentoryStashSlots(void);

namespace {
// Print text horizontally and vertically centered inside box
// x and y are added to the box's x and y.
void MPrintCenteredInBox(int x, int y, ST::string const& text, SGPBox const& box)
{
	MPrint(x + box.x, y + box.y, text, HCenterVCenterAlign(box.w, box.h));
}
}

// blit the background panel for the inventory
void BlitInventoryPoolGraphic( void )
{
	const SGPBox* const box = &g_sector_inv_box;
	BltVideoObject(guiSAVEBUFFER, GetMapInventoryPoolBackgroundFilename(), 0, MAP_SCREEN_X + box->x, MAP_SCREEN_Y + box->y);

	// resize list
	CheckAndUnDateSlotAllocation( );

	// now update help text
	UpdateHelpTextForInvnentoryStashSlots( );

	// Main grid's own item icons and page-number/Total Items VALUES -- all
	// three sit underneath the stack split popup's own footprint (unlike
	// the "Location"/"Total Items" text labels below, which sit further
	// left, outside it), so they must not draw while the popup is open, or
	// they bleed through/over its own independent equivalents
	// (RenderStackSplitItems() below). Same reasoning as the main grid's
	// next/prev arrow buttons being Hidden while the popup is open
	// (HandleButtonStatesWhileMapInventoryActive()).
	if (gStackSplitSourceIndex == -1)
	{
		// now the items
		RenderItemsForCurrentPageOfInventoryPool( );

		// show which page and last page
		DisplayPagesForMapInventoryPool( );

		// draw number of items in current inventory
		DrawNumberOfInventoryPoolItems();
	}

	// display current sector inventory pool is at
	DisplayCurrentSector( );

	DrawTextOnMapInventoryBackground( );

	// stack split view (Wariant B) -- renders on top of everything else
	RenderStackSplitItems( );

	// re render buttons
	MarkButtonsDirty( );

	// which buttons will be active and which ones not
	HandleButtonStatesWhileMapInventoryActive( );
}


static BOOLEAN RenderItemInPoolSlot(INT32 iCurrentSlot, INT32 iFirstSlotOnPage);


static void RenderItemsForCurrentPageOfInventoryPool(void)
{
	INT32 iCounter = 0;

	// go through list of items on this page and place graphics to screen
	for( iCounter = 0; iCounter < GetMapInventoryPoolPageSize() ; iCounter++ )
	{
		RenderItemInPoolSlot( iCounter, ( iCurrentInventoryPoolPage * GetMapInventoryPoolPageSize() ) );
	}
}


static BOOLEAN RenderItemInPoolSlot(INT32 iCurrentSlot, INT32 iFirstSlotOnPage)
{
	// render item in this slot of the list
	const WORLDITEM& item = pInventoryPoolList[iCurrentSlot + iFirstSlotOnPage];

	// check if anything there
	if (item.o.ubNumberOfObjects == 0) return FALSE;

	const SGPBox* const slot_box = &GetSectorInvSlotBox();
	const INT32 dx = MAP_SCREEN_X + slot_box->x + slot_box->w * (iCurrentSlot / MAP_INV_SLOT_ROWS);
	const INT32 dy = MAP_SCREEN_Y + slot_box->y + slot_box->h * (iCurrentSlot % MAP_INV_SLOT_ROWS);

	SetFontDestBuffer(guiSAVEBUFFER);
	const SGPBox* const item_box = &GetSectorInvItemBox();
	const UINT16        outline  = fMapInventoryItemCompatable[iCurrentSlot] ? Get16BPPColor(FROMRGB(255, 255, 255)) : SGP_TRANSPARENT;
	INVRenderItem(guiSAVEBUFFER, NULL, item.o, dx + item_box->x, dy + item_box->y, item_box->w, item_box->h, DIRTYLEVEL2, 0, outline, gfSectorInventoryBigImages);

	// draw bar for condition
	const UINT16 col0 = Get16BPPColor(DESC_STATUS_BAR);
	const UINT16 col1 = Get16BPPColor(DESC_STATUS_BAR_SHADOW);
	const SGPBox* const bar_box = &GetSectorInvBarBox();
	DrawItemUIBarEx(item.o, 0, dx + bar_box->x, dy + bar_box->y + bar_box->h - 1, bar_box->h, col0, col1, guiSAVEBUFFER);

	// if the item is not reachable, or if the selected merc is not in the current sector
	const SOLDIERTYPE* const s = GetSelectedInfoChar();
	if (!(item.usFlags & WORLD_ITEM_REACHABLE) ||
			s           == NULL     ||
			s->sSector.x != sSelMap.x ||
			s->sSector.y != sSelMap.y ||
			s->sSector.z != iCurrentMapSectorZ)
	{
		//Shade the item
		DrawHatchOnInventory(guiSAVEBUFFER, dx + item_box->x, dy + item_box->y, item_box->w, item_box->h);
	}

	// the name
	const SGPBox* const name_box = &GetSectorInvNameBox();
	auto sString = ReduceStringLength(GCM->getItem(item.o.usItem)->getShortName(), name_box->w, MAP_SECTOR_INV_ITEM_FONT);

	// Same color+shadow as the merc stat values (0-100 Agility/Dexterity/
	// Strength/etc.) on the single-merc panel (Inventory_bottom_panel.sti),
	// per user request -- see STATS_TEXT_FONT_COLOR (5) + the inherited
	// DEFAULT_SHADOW in PrintStat() (Interface_Panels.cc).
	SetFontAttributes(MAP_SECTOR_INV_ITEM_FONT, 5, DEFAULT_SHADOW);
	// -1 X per user request, applied here rather than baked into
	// g_sector_inv_name_box.x (like the (UINT16)-1 trick above) --
	// MPrintCenteredInBox()'s x ends up passed as a plain 32-bit int all
	// the way down to the glyph blitter (no INT16 truncation to cancel the
	// unsigned wraparound out, unlike DrawItemUIBarEx()'s sXPos), so a
	// negative UINT16 box field would push the text off past
	// FontDestRegion's clip and make it disappear instead of shifting it.
	// dx itself is a plain signed int here, so dx - 1 just works.
	MPrintCenteredInBox(dx - 1, dy, sString, *name_box);
	SetFontDestBuffer(FRAME_BUFFER);

	return TRUE;
}


static void UpdateHelpTextForInvnentoryStashSlots(void)
{
	ST::string pStr;
	INT32 iCounter = 0;
	INT32 iFirstSlotOnPage = ( iCurrentInventoryPoolPage * GetMapInventoryPoolPageSize() );


	// run through list of items in slots and update help text for mouse regions
	for( iCounter = 0; iCounter < GetMapInventoryPoolPageSize(); iCounter++ )
	{
		ST::string help;
		OBJECTTYPE const& o    = pInventoryPoolList[iCounter + iFirstSlotOnPage].o;
		if  (o.ubNumberOfObjects > 0)
		{
			pStr = GetHelpTextForItem(o);
			help = pStr;
		}
		MapInventoryPoolSlots[iCounter].SetFastHelpText(help);
	}
}


static void BuildStashForSelectedSector(const SGPSector& sector);
static void CreateMapInventoryButtons(void);
static void CreateMapInventoryPoolDoneButton(void);
static void CreateMapInventoryPoolSlots(void);
static void CreateMapInventoryGroupButton(void);
static void CreateMapInventoryFilterButtons(void);
static void CreateMapInventoryBigImagesButton(void);
static void CreateMapInventoryTransferButtons(void);
static void CreateStackSplitSlots(void);
static void CreateStackSplitDoneButton(void);
static void CreateStackSplitPageButtons(void);
static void DestroyInventoryPoolDoneButton(void);
static void DestroyMapInventoryButtons(void);
static void DestroyMapInventoryPoolSlots();
static void DestroyMapInventoryGroupButton(void);
static void DestroyMapInventoryFilterButtons(void);
static void DestroyMapInventoryBigImagesButton(void);
static void DestroyMapInventoryTransferButtons(void);
static void DestroyStackSplitSlots(void);
static void DestroyStackSplitDoneButton(void);
static void DestroyStackSplitPageButtons(void);
static void DestroyStash(void);
static void GroupSectorInventoryItems(void);
static void ApplySectorInventoryFilter(void);
static void HandleMapSectorInventory(void);
static void OpenStackSplitView(INT32 sourceIndex);
static void CloseStackSplitView(void);
static void StackSplitSlotPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void StackSplitSlotSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
static void SaveSeenAndUnseenItems(void);


// create and remove buttons for inventory
void CreateDestroyMapInventoryPoolButtons( BOOLEAN fExitFromMapScreen )
{
	static BOOLEAN fCreated = FALSE;

/* player can leave items underground, no?
	if( iCurrentMapSectorZ )
	{
		fShowMapInventoryPool = FALSE;
	}
*/
	auto const& sector{ sSelMap };
	if (fShowMapInventoryPool && !fCreated)
	{
		if (gWorldSector == sector)
		{
			// handle all reachable before save
			HandleAllReachAbleItemsInTheSector(gWorldSector);
		}

		// destroy buttons for map border
		DeleteMapBorderButtons( );

		fCreated = TRUE;

		// Reset the "big images" toggle every time the panel opens, before
		// CreateMapInventoryPoolSlots() below (which reads it to size/lay
		// out the grid) -- same convention as gubSectorInventoryActiveFilters
		// just below.
		gfSectorInventoryBigImages = FALSE;

		// also create the inventory slot
		CreateMapInventoryPoolSlots( );

		// create buttons
		CreateMapInventoryButtons( );

		// Reset category filters to "Wszystkie przedmioty" every time the
		// panel opens -- a fresh BuildStashForSelectedSector() below always
		// builds an unfiltered pInventoryPoolList, and CreateMapInventoryFilterButtons()
		// below starts guiMapInvenButton[4] ("Wszystkie przedmioty") ON to
		// match, every other filter button OFF.
		gubSectorInventoryActiveFilters = SECTOR_INV_FILTER_ALL;

		// build stash
		BuildStashForSelectedSector(sector);

		CreateMapInventoryPoolDoneButton( );

		CreateMapInventoryGroupButton( );
		CreateMapInventoryFilterButtons( );
		CreateMapInventoryBigImagesButton( );
		CreateMapInventoryTransferButtons( );

		fMapPanelDirty = TRUE;
		fMapScreenBottomDirty = TRUE;
	}
	else if (!fShowMapInventoryPool && fCreated)
	{
		// An item-description box left open when the whole Sector Inventory
		// panel closes would end up pointing into pInventoryPoolList after
		// DestroyStash() clears it below -- gpItemDescObject would dangle,
		// and RenderItemDescriptionBox() reads it every frame regardless of
		// fShowMapInventoryPool. Close it first.
		if (InItemDescriptionBox()) DeleteItemDescriptionBox();

		// Same risk, more severe, for the stack split view: it holds the
		// stack's items OUTSIDE pInventoryPoolList entirely
		// (gStackSplitItems) while open, so closing without merging back
		// first would permanently lose them the moment DestroyStash() below
		// clears the list they belong back into.
		if (gStackSplitSourceIndex != -1) CloseStackSplitView();

		// check fi we are in fact leaving mapscreen
		if (!fExitFromMapScreen)
		{
			// recreate mapborder buttons
			CreateButtonsForMapBorder( );
		}
		fCreated = FALSE;

		// destroy the map inventory slots
		DestroyMapInventoryPoolSlots( );

		// destroy map inventory buttons
		DestroyMapInventoryButtons( );

		DestroyInventoryPoolDoneButton( );

		DestroyMapInventoryGroupButton( );
		DestroyMapInventoryFilterButtons( );
		DestroyMapInventoryBigImagesButton( );
		DestroyMapInventoryTransferButtons( );

		// now save results
		SaveSeenAndUnseenItems( );

		DestroyStash( );



		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
		// RenderMapScreenInterfaceBottom() skips itself entirely while this
		// panel is showing (Map_Screen_Interface_Bottom.cc), so its own
		// background never got re-blitted over whatever this (now taller)
		// panel drew in that same screen area -- force it to on the very
		// next frame after closing.
		fMapScreenBottomDirty = TRUE;

		//DEF: added to remove the 'item blip' from staying on the radar map
		iCurrentlyHighLightedItem = -1;

		// re render radar map
		RenderRadarScreen( );
	}

	// do our handling here
	HandleMapSectorInventory( );

}


void CancelSectorInventoryDisplayIfOn( BOOLEAN fExitFromMapScreen )
{
	if ( fShowMapInventoryPool )
	{
		// get rid of sector inventory mode & buttons
		fShowMapInventoryPool = FALSE;
		CreateDestroyMapInventoryPoolButtons( fExitFromMapScreen );
	}
}


static size_t GetTotalNumberOfItems(void);
static void ReBuildWorldItemStashForLoadedSector(const std::vector<WORLDITEM>& pSeenItemsList, const std::vector<WORLDITEM>& pUnSeenItemsList);


static void SaveSeenAndUnseenItems(void)
{
	// if there are seen items, build a temp world items list of them and save them
	std::vector<WORLDITEM> pSeenItemsList;
	for (WORLDITEM& pi : pInventoryPoolList)
	{
		if (pi.o.ubNumberOfObjects == 0) continue;

		WORLDITEM si = pi;
		if (si.sGridNo == 0)
		{
			// Use gridno of predecessor, if there is one
			if (pSeenItemsList.size() != 0)
			{
				// borrow from predecessor
				si.sGridNo = pSeenItemsList.back().sGridNo;
			}
			else
			{
				// get entry grid location
			}
		}
		si.fExists = TRUE;
		si.bVisible = TRUE;
		pSeenItemsList.push_back(si);
	}

	// if this is the loaded sector handle here
	auto const& sector{ sSelMap };
	if (gWorldSector == sector)
	{
		ReBuildWorldItemStashForLoadedSector(pSeenItemsList, pUnSeenItems);
	}
	else
	{
		// now copy over unseen and seen
		SaveWorldItemsToTempItemFile(sector, pUnSeenItems);
		AddWorldItemsToUnLoadedSector(sector, pSeenItemsList);
	}
}


static void InventoryNextPage()
{
	if (iCurrentInventoryPoolPage < iLastInventoryPoolPage)
	{
		++iCurrentInventoryPoolPage;
		fMapPanelDirty = TRUE;
	}
}


static void InventoryPrevPage()
{
	if (iCurrentInventoryPoolPage > 0)
	{
		--iCurrentInventoryPoolPage;
		fMapPanelDirty = TRUE;
	}
}


// the screen mask bttn callaback...to disable the inventory and lock out the map itself
static void MapInvenPoolScreenMaskCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	// Right-click no longer closes the sector inventory, per user request --
	// only the Done button does that now.
}

static void MapInvenPoolScreenMaskCallbackScroll(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		InventoryPrevPage();
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		InventoryNextPage();
	}
}


static void MapInvenPoolSlotsPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void MapInvenPoolSlotsSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
static void MapInvenPoolSlotsScroll(MOUSE_REGION* pRegion, UINT32 iReason);
static void MapInvenPoolSlotsMove(MOUSE_REGION* pRegion, UINT32 iReason);


static void CreateMapInventoryPoolSlots(void)
{
	{
		const SGPBox* const inv_box = &g_sector_inv_box;
		UINT16        const x       = MAP_SCREEN_X + inv_box->x;
		UINT16        const y       = MAP_SCREEN_Y + inv_box->y;
		UINT16        const w       = inv_box->w;
		UINT16        const h       = inv_box->h;
		MSYS_DefineRegion(&MapInventoryPoolMask, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MouseCallbackPrimarySecondary(MSYS_NO_CALLBACK, MapInvenPoolScreenMaskCallbackSecondary, MapInvenPoolScreenMaskCallbackScroll));
	}

	const SGPBox* const slot_box = &GetSectorInvSlotBox();
	const SGPBox* const reg_box  = &GetSectorInvRegionBox();
	// Tracked so DestroyMapInventoryPoolSlots() below only removes exactly
	// the regions actually defined here -- GetMapInventoryPoolPageSize() can
	// now be as low as 35 ("big images" toggle), well under
	// MAP_INVENTORY_POOL_SLOT_COUNT's compile-time array size (90), so the
	// remainder of MapInventoryPoolSlots[] stays undefined and must not be
	// blindly iterated.
	gMapInventoryPoolSlotsCreatedCount = GetMapInventoryPoolPageSize();
	for (UINT i = 0; i < gMapInventoryPoolSlotsCreatedCount; ++i)
	{
		UINT16        const sx = i / MAP_INV_SLOT_ROWS;
		UINT16        const sy = i % MAP_INV_SLOT_ROWS;
		UINT16        const x  = reg_box->x + MAP_SCREEN_X + slot_box->x + sx * slot_box->w;
		UINT16        const y  = reg_box->y + MAP_SCREEN_Y + slot_box->y + sy * slot_box->h;
		UINT16        const w  = reg_box->w;
		UINT16        const h  = reg_box->h;
		MOUSE_REGION* const r  = &MapInventoryPoolSlots[i];
		MSYS_DefineRegion(r, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MapInvenPoolSlotsMove, MouseCallbackPrimarySecondary(MapInvenPoolSlotsPrimary, MapInvenPoolSlotsSecondary, MapInvenPoolSlotsScroll));
		MSYS_SetRegionUserData(r, 0, i);
	}
}


static void DestroyMapInventoryPoolSlots()
{
	for (UINT i = 0; i < gMapInventoryPoolSlotsCreatedCount; ++i) MSYS_RemoveRegion(&MapInventoryPoolSlots[i]);
	gMapInventoryPoolSlotsCreatedCount = 0;
	MSYS_RemoveRegion(&MapInventoryPoolMask);
}


static void MapInvenPoolSlotsMove(MOUSE_REGION* pRegion, UINT32 iReason)
{
	INT32 iCounter = 0;


	iCounter = MSYS_GetRegionUserData( pRegion, 0 );

	if( iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		iCurrentlyHighLightedItem = iCounter;
		fChangedInventorySlots = TRUE;
		gfCheckForCursorOverMapSectorInventoryItem = TRUE;
	}
	else if( iReason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		iCurrentlyHighLightedItem = -1;
		fChangedInventorySlots = TRUE;
		gfCheckForCursorOverMapSectorInventoryItem = FALSE;

		// re render radar map
		RenderRadarScreen( );
	}
}


static void BeginInventoryPoolPtr(OBJECTTYPE* pInventorySlot);
static BOOLEAN CanPlayerUseSectorInventory(void);
static BOOLEAN PlaceObjectInInventoryStash(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr);


static void MapInvenPoolSlotsPrimary(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	// check if item in cursor, if so, then swap, and no item in curor, pick up, if item in cursor but not box, put in box
	INT32      const slot_idx = MSYS_GetRegionUserData(pRegion, 0);
	WORLDITEM& slot = pInventoryPoolList[iCurrentInventoryPoolPage * GetMapInventoryPoolPageSize() + slot_idx];

	// Return if empty
	if (gpItemPointer == NULL && slot.o.usItem == NOTHING) return;

	// is this item reachable
	if (slot.o.usItem != NOTHING && !(slot.usFlags & WORLD_ITEM_REACHABLE))
	{
		// not reachable
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, gzLateLocalizedString[STR_LATE_38], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// Valid character?
	const SOLDIERTYPE* const s = GetSelectedInfoChar();
	if (s == NULL)
	{
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, pMapInventoryErrorString[0], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// Check if selected merc is in this sector, if not, warn them and leave
	if (s->sSector.x != sSelMap.x           ||
			s->sSector.y != sSelMap.y           ||
			s->sSector.z != iCurrentMapSectorZ ||
			s->fBetweenSectors)
	{
		ST::string msg = (gpItemPointer == NULL ? pMapInventoryErrorString[1] : pMapInventoryErrorString[4]);
		ST::string buf = st_format_printf(msg, s->name);
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, buf, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// If in battle inform player they will have to do this in tactical
	if (!CanPlayerUseSectorInventory())
	{
		ST::string msg = (gpItemPointer == NULL ? pMapInventoryErrorString[2] : pMapInventoryErrorString[3]);
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, msg, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// If we do not have an item in hand, start moving it
	if (gpItemPointer == NULL)
	{
		sObjectSourceGridNo = slot.sGridNo;
		BeginInventoryPoolPtr(&slot.o);
	}
	else
	{
		const INT32 iOldNumberOfObjects = slot.o.ubNumberOfObjects;

		// Else, try to place here
		if (PlaceObjectInInventoryStash(&slot.o, gpItemPointer))
		{
			// nothing here before, then place here
			if (iOldNumberOfObjects == 0)
			{
				slot.sGridNo                  = sObjectSourceGridNo;
				slot.ubLevel                  = s->bLevel;
				slot.usFlags                  = 0;
				slot.bRenderZHeightAboveLevel = 0;

				if (sObjectSourceGridNo == NOWHERE)
				{
					slot.usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;
				}
			}

			slot.usFlags |= WORLD_ITEM_REACHABLE;

			// Check if it's the same now!
			if (gpItemPointer->ubNumberOfObjects == 0)
			{
				MAPEndItemPointer();
			}
			else
			{
				SetMapCursorItem();
			}
		}
	}

	// dirty region, force update
	fMapPanelDirty = TRUE;
}

static void MapInvenPoolSlotsSecondary(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	// Right-click no longer closes the sector inventory, per user request --
	// only the Done button does that now. Instead, right-clicking an item
	// opens its description box (iteminfoc.sti), same as right-clicking an
	// item in the merc's own map-screen inventory panel -- see
	// MAPInternalInitItemDescriptionBox() (MapScreen.cc). No-op while
	// holding an item on the cursor, same as the old close-on-right-click
	// behavior was.
	if (gpItemPointer != NULL) return;

	// If the stack split view is already open (for this or a different
	// stack), close (and merge back) it first rather than refusing the
	// click -- same "switch directly" behavior as the item-description box
	// below.
	if (gStackSplitSourceIndex != -1) CloseStackSplitView();

	// If a box is already open, close it first rather than refusing the
	// click -- per user request, scoped to Sector Inventory only (every
	// other item-description call site in the game still uses the
	// refuse-if-already-open guard, e.g. ItemPopupRegionCallbackSecondary()
	// in Interface_Items.cc). DeleteItemDescriptionBox() is a clean,
	// self-contained teardown (removes gInvDesc/giMapInvDescButton/
	// attachment regions etc., no side effects beyond that -- the money
	// "cash out" step lives in the Done button's own callback, not here),
	// so it's safe to call directly before opening the next box. Without
	// this, MSYS_DefineRegion()/QuickCreateButtonImg() would redefine the
	// still-active region/button from the previous box and crash.
	if (InItemDescriptionBox()) DeleteItemDescriptionBox();

	INT32      const slot_idx = MSYS_GetRegionUserData(pRegion, 0);
	INT32      const abs_idx  = iCurrentInventoryPoolPage * GetMapInventoryPoolPageSize() + slot_idx;
	WORLDITEM& slot = pInventoryPoolList[abs_idx];

	if (slot.o.usItem == NOTHING) return;

	if (slot.o.ubNumberOfObjects > 1)
	{
		// Stack of >1 -- show each individual item in its own slot first
		// (Wariant B stack split view, newgoldpiece3.sti), per user
		// request, instead of going straight to the whole stack's
		// description box. Right-clicking one of those items then opens
		// its own description box (StackSplitSlotSecondary() below).
		OpenStackSplitView(abs_idx);
		return;
	}

	MAPInternalInitItemDescriptionBox(&slot.o, 0, GetSelectedInfoChar());
}


// ---------------------------------------------------------------------
// Stack split view (Wariant B) -- see the big comment block with the other
// g_stack_split_*/gStackSplit* declarations near the top of this file.
// ---------------------------------------------------------------------

static void CreateStackSplitSlots(void)
{
	UINT16 const bx = MAP_SCREEN_X + g_stack_split_box.x;
	UINT16 const by = MAP_SCREEN_Y + g_stack_split_box.y;

	// Click-blocker only -- per user request, this window no longer closes
	// on a left/right click of its own background, only via
	// gStackSplitDoneButton below.
	MSYS_DefineRegion(&gStackSplitBackgroundRegion, bx, by, bx + g_stack_split_box.w - 1, by + g_stack_split_box.h - 1,
		MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

	// Only the CURRENT PAGE's items get a region -- gStackSplitSlots[] is
	// reused across pages (same screen positions each time), with the
	// absolute gStackSplitItems index (first + i) stored directly as each
	// region's user data, so StackSplitSlotPrimary()/Secondary() need no
	// changes at all to stay page-aware.
	INT32 const first   = gCurrentStackSplitPage * GetStackSplitPageSize();
	INT32 const visible = std::max<INT32>(0, std::min<INT32>(GetStackSplitPageSize(), static_cast<INT32>(gStackSplitItems.size()) - first));
	SGPBox const& slot_box = GetStackSplitSlotBox();
	SGPBox const& reg_box  = GetStackSplitRegionBox();
	for (INT32 i = 0; i < visible; ++i)
	{
		UINT16        const col = static_cast<UINT16>(i % GetInventoryGridCols());
		UINT16        const row = static_cast<UINT16>(i / GetInventoryGridCols());
		UINT16        const dx  = bx + slot_box.x + col * slot_box.w;
		UINT16        const dy  = by + slot_box.y + row * slot_box.h;
		UINT16        const x   = dx + reg_box.x;
		UINT16        const y   = dy + reg_box.y;
		MOUSE_REGION* const r   = &gStackSplitSlots[i];
		MSYS_DefineRegion(r, x, y, x + reg_box.w - 1, y + reg_box.h - 1,
			MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR, MSYS_NO_CALLBACK,
			MouseCallbackPrimarySecondary(StackSplitSlotPrimary, StackSplitSlotSecondary, MSYS_NO_CALLBACK));
		MSYS_SetRegionUserData(r, 0, static_cast<UINT32>(first + i));
	}
}


static void DestroyStackSplitSlots(void)
{
	// Mirrors CreateStackSplitSlots()'s own page-size computation --
	// gStackSplitItems.size() and gCurrentStackSplitPage are both
	// unchanged between the matching Create call and this one.
	INT32 const first   = gCurrentStackSplitPage * GetStackSplitPageSize();
	INT32 const visible = std::max<INT32>(0, std::min<INT32>(GetStackSplitPageSize(), static_cast<INT32>(gStackSplitItems.size()) - first));
	for (INT32 i = 0; i < visible; ++i) MSYS_RemoveRegion(&gStackSplitSlots[i]);
	MSYS_RemoveRegion(&gStackSplitBackgroundRegion);
}


static void StackSplitDoneBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		CloseStackSplitView();
	}
}


static void CreateStackSplitDoneButton(void)
{
	// The only way to close this window, per user request -- background
	// and item-slot clicks no longer do it (see gStackSplitBackgroundRegion
	// above and StackSplitSlotSecondary() below).
	//
	// Same DONE_BUTTON_Inventory.STI sheet as the main grid's own Done
	// button (CreateMapInventoryPoolDoneButton()), next sequential
	// sub-image pair (2/3, vs. the main grid's own 0/1), per user request.
	gStackSplitDoneButton = QuickCreateButtonImg(INTERFACEDIR "/DONE_BUTTON_Inventory.STI", 2, 3,
		MAP_SCREEN_X + STACK_SPLIT_DONE_X, MAP_SCREEN_Y + STACK_SPLIT_DONE_Y, MSYS_PRIORITY_HIGHEST, StackSplitDoneBtn);
	gStackSplitDoneButton->SetFastHelpText("Done (Stack Inventory)");
}


static void DestroyStackSplitDoneButton(void)
{
	RemoveButton(gStackSplitDoneButton);
}


// This window's own, independent page-turn logic -- mirrors
// InventoryNextPage()/InventoryPrevPage() (the main grid's own), but reruns
// CreateStackSplitSlots()/DestroyStackSplitSlots() around the page change
// since gStackSplitSlots[] holds real mouse regions bound to absolute
// gStackSplitItems indices (see CreateStackSplitSlots()'s own comment),
// not just a rendering offset like the main grid's iCurrentInventoryPoolPage.
static void InventoryStackSplitNextPage(void)
{
	if (gCurrentStackSplitPage < gLastStackSplitPage)
	{
		DestroyStackSplitSlots();
		++gCurrentStackSplitPage;
		CreateStackSplitSlots();
		fMapPanelDirty = TRUE;
	}
}


static void InventoryStackSplitPrevPage(void)
{
	if (gCurrentStackSplitPage > 0)
	{
		DestroyStackSplitSlots();
		--gCurrentStackSplitPage;
		CreateStackSplitSlots();
		fMapPanelDirty = TRUE;
	}
}


static void StackSplitNextBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) InventoryStackSplitNextPage();
}


static void StackSplitPrevBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) InventoryStackSplitPrevPage();
}


static void CreateStackSplitPageButtons(void)
{
	// Placeholder positions, per user request -- not yet the final layout.
	// Same map_screen_bottom_arrows.sti sub-images as the main grid's own
	// next/prev (CreateMapInventoryButtons()) -- a generic page-arrow
	// graphic, reused here for this window's own, independent pagination.
	gStackSplitNextBtn = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", 10, 1, -1, 3, -1, MAP_SCREEN_X + STACK_SPLIT_NEXT_X, MAP_SCREEN_Y + STACK_SPLIT_ARROWS_Y, MSYS_PRIORITY_HIGHEST, StackSplitNextBtn);
	gStackSplitPrevBtn = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti",  9, 0, -1, 2, -1, MAP_SCREEN_X + STACK_SPLIT_PREV_X, MAP_SCREEN_Y + STACK_SPLIT_ARROWS_Y, MSYS_PRIORITY_HIGHEST, StackSplitPrevBtn);
}


static void DestroyStackSplitPageButtons(void)
{
	RemoveButton(gStackSplitNextBtn);
	RemoveButton(gStackSplitPrevBtn);
}


static void RenderStackSplitItems(void)
{
	if (gStackSplitSourceIndex == -1) return;

	UINT16 const bx = MAP_SCREEN_X + g_stack_split_box.x;
	UINT16 const by = MAP_SCREEN_Y + g_stack_split_box.y;

	BltVideoObject(guiSAVEBUFFER, GetStackSplitBackgroundFilename(), 0, bx, by);

	SetFontDestBuffer(guiSAVEBUFFER);

	// Only the current page -- see CreateStackSplitSlots()'s own comment on
	// why gStackSplitSlots[]/absolute indices work the same way.
	INT32 const first = gCurrentStackSplitPage * GetStackSplitPageSize();
	INT32 const last  = std::min<INT32>(first + GetStackSplitPageSize(), static_cast<INT32>(gStackSplitItems.size()));
	for (INT32 abs_idx = first; abs_idx < last; ++abs_idx)
	{
		OBJECTTYPE const& o = gStackSplitItems[abs_idx];
		if (o.usItem == NOTHING) continue;

		INT32 const i   = abs_idx - first;
		INT32 const col = i % GetInventoryGridCols();
		INT32 const row = i / GetInventoryGridCols();
		SGPBox const& slot_box = GetStackSplitSlotBox();
		INT32 const dx  = bx + slot_box.x + col * slot_box.w;
		INT32 const dy  = by + slot_box.y + row * slot_box.h;

		const SGPBox* const item_box = &GetStackSplitItemBox();
		INVRenderItem(guiSAVEBUFFER, NULL, o, dx + item_box->x, dy + item_box->y, item_box->w, item_box->h, DIRTYLEVEL2, 0, SGP_TRANSPARENT, gfSectorInventoryBigImages);

		const UINT16        col0    = Get16BPPColor(DESC_STATUS_BAR);
		const UINT16        col1    = Get16BPPColor(DESC_STATUS_BAR_SHADOW);
		const SGPBox* const bar_box = &GetStackSplitBarBox();
		DrawItemUIBarEx(o, 0, dx + bar_box->x, dy + bar_box->y + bar_box->h - 1, bar_box->h, col0, col1, guiSAVEBUFFER);

		const SGPBox* const name_box = &GetStackSplitNameBox();
		auto sString = ReduceStringLength(GCM->getItem(o.usItem)->getShortName(), name_box->w, MAP_SECTOR_INV_ITEM_FONT);
		SetFontAttributes(MAP_SECTOR_INV_ITEM_FONT, 5, DEFAULT_SHADOW);
		MPrintCenteredInBox(dx - 1, dy, sString, *name_box);
	}

	// This window's own, independent page indicator -- per user request.
	SetFontAttributes(FONT_VALUE_INVENTORY, 183);
	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y + COMPACT_FOOTER_TEXT_Y_OFFSET,
		ST::format("{} / {}", gCurrentStackSplitPage + 1, gLastStackSplitPage + 1),
		g_stack_split_page_box);

	// This window's own, independent "Total Items" label + value -- per
	// user request. Reuses the main grid's own already-localized label
	// (pMapInventoryStrings[1]) and its exact DisplayWrappedString() call
	// shape (DrawTextOnMapInventoryBackground()), just at this window's own
	// X/Y; the value mirrors DrawNumberOfInventoryPoolItems() but counts
	// gStackSplitItems instead of pInventoryPoolList
	// (GetTotalNumberOfItemsInStackSplit()).
	{
		int const textX = MAP_SCREEN_X + STACK_SPLIT_TOTAL_TEXT_X;
		int const textY = MAP_SCREEN_Y + STACK_SPLIT_TOTAL_TEXT_Y;
		UINT16 const textH = DisplayWrappedString(textX, textY, 65, 1, FONT_TEXT_INVENTORY, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
		DisplayWrappedString(textX, textY - (textH / 2), 65, 1, FONT_TEXT_INVENTORY, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED);

		SetFontAttributes(FONT_VALUE_INVENTORY, 183);
		MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y + COMPACT_FOOTER_TEXT_Y_OFFSET,
			ST::string::from_uint(GetTotalNumberOfItemsInStackSplit()),
			g_stack_split_count_box);
	}

	SetFontDestBuffer(FRAME_BUFFER);
}


static void OpenStackSplitView(INT32 const sourceIndex)
{
	WORLDITEM& src = pInventoryPoolList[sourceIndex];

	// Physically pull every unit of the stack out into its own 1-count
	// OBJECTTYPE -- same primitive ItemPopupRegionCallbackPrimary()
	// (Interface_Items.cc) already uses to split a single item off a stack
	// onto the cursor. Repeating it at index 0 drains the whole stack, down
	// to usItem == NOTHING/ubNumberOfObjects == 0.
	gStackSplitItems.clear();
	gStackSplitItems.reserve(src.o.ubNumberOfObjects);
	while (src.o.ubNumberOfObjects > 0)
	{
		OBJECTTYPE single{};
		GetObjFrom(&src.o, 0, &single);
		gStackSplitItems.push_back(single);
	}

	gStackSplitSourceIndex = sourceIndex;

	// This window's own, independent pagination -- always starts at page 1,
	// per user request.
	gCurrentStackSplitPage = 0;
	gLastStackSplitPage    = static_cast<INT32>(gStackSplitItems.empty() ? 0 : (gStackSplitItems.size() - 1) / GetStackSplitPageSize());

	CreateStackSplitSlots();
	CreateStackSplitDoneButton();
	CreateStackSplitPageButtons();

	fMapPanelDirty = TRUE;
}


static void CloseStackSplitView(void)
{
	if (gStackSplitSourceIndex == -1) return;

	// A dangling gpItemDescObject risk identical to the one guarded against
	// for the whole Sector Inventory panel in
	// CreateDestroyMapInventoryPoolButtons() -- ItemInfoC.sti may still be
	// open on one of gStackSplitItems (StackSplitSlotSecondary() leaves it
	// open on top of this window, same as the main grid does). Close it
	// before the merge below invalidates that pointer.
	if (InItemDescriptionBox()) DeleteItemDescriptionBox();

	// Re-merge every physically split-out item still here back into one
	// stack -- same pairwise CleanUpStack()/StackObjs() consolidation
	// GroupSectorInventoryItems() uses for the whole stash, just applied to
	// this one stack's own items. "Still here" because StackSplitSlotPrimary()
	// may have picked one or more up onto the cursor (typically dropped into
	// a merc's own inventory) since the window opened, leaving those slots
	// NOTHING -- the first slot that's still occupied becomes the merge
	// base instead of always assuming index 0.
	OBJECTTYPE merged{};
	BOOLEAN    fHaveBase = FALSE;
	for (size_t i = 0; i < gStackSplitItems.size(); ++i)
	{
		OBJECTTYPE& src = gStackSplitItems[i];
		if (src.usItem == NOTHING) continue;

		if (!fHaveBase)
		{
			merged    = src;
			fHaveBase = TRUE;
			continue;
		}

		if (src.usItem != merged.usItem)
		{
			// A different item type ended up here -- StackSplitSlotPrimary()
			// now allows placing/swapping any item from the cursor into a
			// slot, same as the main grid's own left-click, so this slot no
			// longer necessarily matches the rest of the original stack.
			// Can't merge it into `merged`; hand it back to the stash
			// directly instead of losing it.
			AutoPlaceObjectInInventoryStash(&src);
			continue;
		}

		// Merge partial charges first (ammo/kits/canteens/alcohol/etc.).
		CleanUpStack(&merged, &src);

		// CanGunsStack(): a gun pulled out of this window (e.g. reloaded or
		// given an attachment while split out, both now possible for a
		// single split-out unit) may no longer be physically identical to
		// the rest of the stack -- if so, don't merge it back in, hand it
		// to the stash separately below instead.
		if (src.ubNumberOfObjects > 0 && CanGunsStack(merged, src))
		{
			UINT8 const slot_limit = std::min<UINT8>(ItemSlotLimit(merged.usItem, BIGPOCK1POS), MAX_OBJECTS_PER_SLOT);
			if (merged.ubNumberOfObjects < slot_limit)
			{
				UINT8 const room    = slot_limit - merged.ubNumberOfObjects;
				UINT8 const to_move = std::min<UINT8>(src.ubNumberOfObjects, room);
				StackObjs(&src, &merged, to_move);
			}
		}

		// Should never trigger -- nothing in this window can grow a stack
		// past its own original size -- but if it somehow did, don't drop
		// the remainder on the floor.
		if (src.ubNumberOfObjects > 0) AutoPlaceObjectInInventoryStash(&src);
	}

	// Write back into the source slot -- unless every item was picked up
	// out of this window already (fHaveBase == FALSE), in which case
	// there's nothing left to put back; the whole stack was manually
	// handed out one by one.
	if (fHaveBase)
	{
		// PlaceObjectInInventoryStash() covers both the expected case (slot
		// still empty, exactly as OpenStackSplitView() left it) and the edge
		// case of something else having been placed there in the meantime
		// (merges if compatible, otherwise swaps it into `merged`).
		WORLDITEM& dest = pInventoryPoolList[gStackSplitSourceIndex];
		PlaceObjectInInventoryStash(&dest.o, &merged);
		if (merged.usItem != NOTHING && merged.ubNumberOfObjects > 0)
		{
			// Leftover from a swap above (a different item was sitting in
			// this slot) -- never drop it, place it in the first free slot
			// instead.
			AutoPlaceObjectInInventoryStash(&merged);
		}
	}

	DestroyStackSplitSlots();
	DestroyStackSplitDoneButton();
	DestroyStackSplitPageButtons();
	gStackSplitItems.clear();
	gStackSplitSourceIndex = -1;

	fMapPanelDirty = TRUE;
}


static void StackSplitSlotPrimary(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	INT32 const idx = MSYS_GetRegionUserData(pRegion, 0);
	if (idx < 0 || static_cast<size_t>(idx) >= gStackSplitItems.size()) return;

	OBJECTTYPE& slot = gStackSplitItems[idx];

	// Nothing to pick up and nothing in hand to place here -- no-op.
	if (gpItemPointer == NULL && slot.usItem == NOTHING) return;

	// Same soldier/sector/battle gate as the main grid's own left-click
	// (MapInvenPoolSlotsPrimary()) -- an item already sitting in the stash
	// could still fail these mid-view (selected merc moved out of the
	// sector, or a battle started, while this window was open).
	const SOLDIERTYPE* const s = GetSelectedInfoChar();
	if (s == NULL)
	{
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, pMapInventoryErrorString[0], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}
	if (s->sSector.x != sSelMap.x || s->sSector.y != sSelMap.y || s->sSector.z != iCurrentMapSectorZ || s->fBetweenSectors)
	{
		ST::string const msg = (gpItemPointer == NULL ? pMapInventoryErrorString[1] : pMapInventoryErrorString[4]);
		ST::string const buf = st_format_printf(msg, s->name);
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, buf, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}
	if (!CanPlayerUseSectorInventory())
	{
		ST::string const msg = (gpItemPointer == NULL ? pMapInventoryErrorString[2] : pMapInventoryErrorString[3]);
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, msg, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	if (gpItemPointer == NULL)
	{
		// Pick up onto the cursor -- typically to drop into a merc's own
		// map-screen inventory panel (MAPINV.STI), whose click handlers
		// already accept whatever's on gpItemPointer generically, so no
		// changes are needed there.
		//
		// If the item's own description box is open
		// (StackSplitSlotSecondary() leaves it open on top of this
		// window), close it -- it would otherwise dangle once the slot
		// below is cleared.
		if (InItemDescriptionBox()) DeleteItemDescriptionBox();

		gItemPointer = slot;
		slot         = OBJECTTYPE{};

		SetItemPointer(&gItemPointer, 0);
		SetMapCursorItem();
	}
	else
	{
		// Place (or merge/swap) whatever's on the cursor back into this
		// slot -- same generic OBJECTTYPE-level primitive the main grid's
		// own left-click already uses for its own slots
		// (MapInvenPoolSlotsPrimary()). CloseStackSplitView() handles a
		// slot that ends up with a different item type than the rest (a
		// swap) safely -- it never tries to merge it, just hands it back
		// to the stash directly.
		if (PlaceObjectInInventoryStash(&slot, gpItemPointer))
		{
			if (gpItemPointer->ubNumberOfObjects == 0)
			{
				MAPEndItemPointer();
			}
			else
			{
				SetMapCursorItem();
			}
		}
	}

	fMapPanelDirty = TRUE;
}


static void StackSplitSlotSecondary(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	if (gpItemPointer != NULL) return;

	INT32 const idx = MSYS_GetRegionUserData(pRegion, 0);
	if (idx < 0 || static_cast<size_t>(idx) >= gStackSplitItems.size()) return;

	OBJECTTYPE* const item = &gStackSplitItems[idx];
	if (item->usItem == NOTHING) return;

	// Same "switch directly" behavior as the main grid's own right-click
	// handler above -- close any already-open description box before
	// opening this one, but leave the stack split window itself open,
	// mirroring how the main grid leaves the whole Sector Inventory panel
	// open underneath its own description box.
	if (InItemDescriptionBox()) DeleteItemDescriptionBox();

	MAPInternalInitItemDescriptionBox(item, 0, GetSelectedInfoChar());
}


static void MapInvenPoolSlotsScroll(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		InventoryPrevPage();
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		InventoryNextPage();
	}
}


static void MapInventoryPoolPrevBtn(GUI_BUTTON* btn, UINT32 reason);
static void MapInventoryPoolNextBtn(GUI_BUTTON* btn, UINT32 reason);


static void CreateMapInventoryButtons(void)
{
	guiMapInvenButton[0] = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", 10, 1, -1, 3, -1, MAP_SCREEN_X + 711, MAP_SCREEN_Y + 626 + COMPACT_FOOTER_TEXT_Y_OFFSET, MSYS_PRIORITY_HIGHEST, MapInventoryPoolNextBtn);
	guiMapInvenButton[1] = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti",  9, 0, -1, 2, -1, MAP_SCREEN_X + 638, MAP_SCREEN_Y + 626 + COMPACT_FOOTER_TEXT_Y_OFFSET, MSYS_PRIORITY_HIGHEST, MapInventoryPoolPrevBtn);

	//reset the current inventory page to be the first page
	iCurrentInventoryPoolPage = 0;
}


static void DestroyMapInventoryButtons(void)
{
	RemoveButton( guiMapInvenButton[ 0 ] );
	RemoveButton( guiMapInvenButton[ 1 ] );
}


static void CheckGridNoOfItemsInMapScreenMapInventory(void);
static void SortSectorInventory(WORLDITEM* pInventory, size_t sizeOfArray);


static void BuildStashForSelectedSector(const SGPSector& sector)
{
	std::vector<WORLDITEM> temp;
	std::vector<WORLDITEM>* items = nullptr;
	if (sector == gWorldSector)
	{
		items = &gWorldItems;
	}
	else
	{
		temp = LoadWorldItemsFromTempItemFile(sector);
		items = &temp;
	}

	pInventoryPoolList.clear();
	pUnSeenItems.clear();

	for (const WORLDITEM& wi : *items)
	{
		if (!wi.fExists) continue;
		if (IsMapScreenWorldItemVisibleInMapInventory(wi))
		{
			pInventoryPoolList.push_back(wi);
		}
		else
		{
			pUnSeenItems.push_back(wi);
		}
	}

	size_t visible_slots = pInventoryPoolList.size();
	size_t empty_slots = GetMapInventoryPoolPageSize() - visible_slots % GetMapInventoryPoolPageSize();
	pInventoryPoolList.resize(visible_slots + empty_slots, WORLDITEM{});
	// No filter is active yet at this point (reset right before this call
	// -- CreateDestroyMapInventoryPoolButtons()), so the whole (now padded)
	// list is the visible prefix.
	gVisibleInventorySlotCount = pInventoryPoolList.size();
	iLastInventoryPoolPage  = static_cast<INT32>((gVisibleInventorySlotCount - 1) / GetMapInventoryPoolPageSize());

	CheckGridNoOfItemsInMapScreenMapInventory();
	SortSectorInventory(pInventoryPoolList.data(), visible_slots);
}


static void ReBuildWorldItemStashForLoadedSector(const std::vector<WORLDITEM>& pSeenItemsList, const std::vector<WORLDITEM>& pUnSeenItemsList)
{
	TrashWorldItems();

	std::vector<WORLDITEM> pTotalList;
	pTotalList.insert(pTotalList.end(), pSeenItemsList.begin(), pSeenItemsList.end());
	pTotalList.insert(pTotalList.end(), pUnSeenItemsList.begin(), pUnSeenItemsList.end());

	size_t remainder = pTotalList.size() % 10;
	if (remainder)
	{
		pTotalList.insert(pTotalList.end(), 10 - remainder, WORLDITEM{});
	}

	RefreshItemPools(pTotalList);

	//Count the total number of visible items
	UINT32 uiTotalNumberOfVisibleItems = 0;
	for (const WORLDITEM& si : pSeenItemsList)
	{
		uiTotalNumberOfVisibleItems += si.o.ubNumberOfObjects;
	}

	//reset the visible item count in the sector info struct
	SetNumberOfVisibleWorldItemsInSectorStructureForSector(gWorldSector, uiTotalNumberOfVisibleItems);
}


static void DestroyStash(void)
{
	// clear out stash
	pInventoryPoolList.clear();
	pUnSeenItems.clear();
}


static BOOLEAN GetObjFromInventoryStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr);
static BOOLEAN RemoveObjectFromStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr);


static void BeginInventoryPoolPtr(OBJECTTYPE* pInventorySlot)
{
	BOOLEAN fOk = FALSE;

	// If not null return
	if ( gpItemPointer != NULL )
	{
		return;
	}

	// if shift key get all

	if (_KeyDown( SHIFT ))
	{
		// Remove all from soldier's slot
		fOk = RemoveObjectFromStashSlot( pInventorySlot, &gItemPointer );
	}
	else
	{
		GetObjFromInventoryStashSlot( pInventorySlot, &gItemPointer );
		fOk = (gItemPointer.ubNumberOfObjects == 1);
	}

	if (fOk)
	{
		// Dirty interface
		fMapPanelDirty = TRUE;
		SetItemPointer(&gItemPointer, 0);
		SetMapCursorItem();

		if (fShowInventoryFlag)
		{
			SOLDIERTYPE* const s = GetSelectedInfoChar();
			if (s != NULL)
			{
				ReevaluateItemHatches(s, FALSE);
				fTeamPanelDirty = TRUE;
			}
		}
	}
}


// get this item out of the stash slot
static BOOLEAN GetObjFromInventoryStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr)
{
	// item ptr
	if (!pItemPtr )
	{
		return( FALSE );
	}

	// Delegate to the shared primitive (Items.cc) instead of duplicating
	// its logic -- this used to copy only usItem + bStatus[0] itself for
	// a pick-up from a stack of more than one, silently dropping a gun's
	// ammo/attachment state (everything but bGunStatus/condition). The
	// picked-up gun then looked completely unloaded regardless of what it
	// actually had loaded, so CanGunsStack() (Items.cc) correctly saw it
	// as different from what was left behind and refused to merge it back
	// onto its own stack -- forcing it into a different, empty slot
	// instead. GetObjFrom() already does a full-struct copy for guns.
	GetObjFrom( pInventorySlot, 0, pItemPtr );

	return ( TRUE );
}


static BOOLEAN RemoveObjectFromStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr)
{
	if (pInventorySlot -> ubNumberOfObjects == 0)
	{
		return( FALSE );
	}
	else
	{
		*pItemPtr = *pInventorySlot;
		DeleteObj( pInventorySlot );
		return( TRUE );
	}
}


static BOOLEAN PlaceObjectInInventoryStash(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr)
{
	UINT8 ubNumberToDrop, ubSlotLimit, ubLoop;

	// if there is something there, swap it, if they are of the same type and stackable then add to the count

	// Clamped to MAX_OBJECTS_PER_SLOT -- an item's own ubPerPocket (game
	// data) isn't itself bounded by it, but bStatus[]/ubShotsLeft[] below
	// physically are. Missing this clamp let the "stacking" branch below
	// push pInventorySlot->ubNumberOfObjects past MAX_OBJECTS_PER_SLOT for
	// any item whose ubPerPocket exceeds it, and StackObjs() would then
	// write bStatus[] past its own bounds, corrupting usAttachItem[]/
	// bAttachStatus[] right after it in OBJECTTYPE -- surfacing later as a
	// "invalid vector subscript" crash the next time something (e.g.
	// GetHelpTextForItem()) reads that corrupted attachment data and looks
	// it up as an item ID.
	ubSlotLimit = std::min<UINT8>(GCM->getItem(pItemPtr -> usItem)->getPerPocket(), MAX_OBJECTS_PER_SLOT);

	if (pInventorySlot->ubNumberOfObjects == 0)
	{
		// placement in an empty slot
		ubNumberToDrop = pItemPtr->ubNumberOfObjects;

		if (ubNumberToDrop > ubSlotLimit && ubSlotLimit != 0)
		{
			// drop as many as possible into pocket
			ubNumberToDrop = ubSlotLimit;
		}

		// could be wrong type of object for slot... need to check...
		// but assuming it isn't
		*pInventorySlot = *pItemPtr;

		// Guns skip this: bStatus[0..4] alias bGunStatus/ubGunAmmoType/
		// ubGunShotsLeft/usGunAmmoItem/bGunAmmoStatus, a single value
		// shared by the whole stack (see CanGunsStack(), Items.cc) --
		// there's nothing per-unit to zero here.
		if (ubNumberToDrop != pItemPtr->ubNumberOfObjects && !GCM->getItem(pItemPtr->usItem)->isGun())
		{
			// in the InSlot copy, zero out all the objects we didn't drop
			for (ubLoop = ubNumberToDrop; ubLoop < pItemPtr->ubNumberOfObjects; ubLoop++)
			{
				pInventorySlot->bStatus[ubLoop] = 0;
			}
		}
		pInventorySlot->ubNumberOfObjects = ubNumberToDrop;

		// remove a like number of objects from pObj
		RemoveObjs( pItemPtr, ubNumberToDrop );
	}
	else
	{
		// replacement/reloading/merging/stacking

		// placement in an empty slot
		ubNumberToDrop = pItemPtr->ubNumberOfObjects;

		if (pItemPtr->usItem == pInventorySlot->usItem)
		{
			if (pItemPtr->usItem == MONEY)
			{
				// always allow money to be combined!
				// status of money is always 100
				pInventorySlot->bMoneyStatus = 100;
				pInventorySlot->uiMoneyAmount += pItemPtr->uiMoneyAmount;

				DeleteObj( pItemPtr );
			}
			else if (ubSlotLimit < 2 || !CanGunsStack(*pItemPtr, *pInventorySlot))
			{
				// swapping -- either genuinely non-stackable here, or (see
				// CanGunsStack(), Items.cc) two physically distinguishable
				// guns that can't share pInventorySlot's single shared gun
				// state (different ammo/attachments/condition).
				SwapObjs( pItemPtr, pInventorySlot );
			}
			else
			{
				// stacking
				if( ubNumberToDrop > ubSlotLimit - pInventorySlot -> ubNumberOfObjects )
				{
					ubNumberToDrop = ubSlotLimit - pInventorySlot -> ubNumberOfObjects;
				}

				StackObjs( pItemPtr, pInventorySlot, ubNumberToDrop );
			}
		}
		else
		{

				SwapObjs( pItemPtr, pInventorySlot );
		}
	}
	return( TRUE );
}


void AutoPlaceObjectInInventoryStash(OBJECTTYPE* pItemPtr)
{
	// Find an actual free slot -- growing the stash by a page if none
	// exists, same low-space handling CheckAndUnDateSlotAllocation() itself
	// uses. FIXME (acknowledged, now fixed): this used to index
	// pInventoryPoolList[pInventoryPoolList.size()], one past the end --
	// undefined behaviour that silently wrote into unrelated memory instead
	// of a real slot, so the item was effectively lost. That went
	// unnoticed while this function had no caller that could actually be
	// exercised in practice; CloseStackSplitView()'s "never drop it" safety
	// net (Map_Screen_Interface_Map_Inventory.cc) made it a real,
	// user-visible item-loss bug.
	auto it = std::find_if(pInventoryPoolList.begin(), pInventoryPoolList.end(),
		[](WORLDITEM const& wi) { return wi.o.ubNumberOfObjects == 0; });
	if (it == pInventoryPoolList.end())
	{
		size_t const old_size = pInventoryPoolList.size();
		pInventoryPoolList.insert(pInventoryPoolList.end(), GetMapInventoryPoolPageSize(), WORLDITEM{});
		it = pInventoryPoolList.begin() + old_size;
		// Growing at the absolute end only extends the visible span when
		// SECTOR_INV_FILTER_ALL guarantees no hidden tail exists (the whole
		// list IS the visible span) -- see gVisibleInventorySlotCount's own
		// comment. Otherwise this appends after the hidden tail instead, so
		// the visible boundary/page count don't move.
		if (gubSectorInventoryActiveFilters & SECTOR_INV_FILTER_ALL) gVisibleInventorySlotCount = pInventoryPoolList.size();
		iLastInventoryPoolPage = static_cast<INT32>((gVisibleInventorySlotCount - 1) / GetMapInventoryPoolPageSize());
	}

	WORLDITEM& slot = *it;

	// placement in an empty slot
	UINT8       ubNumberToDrop = pItemPtr->ubNumberOfObjects;
	// Clamped to MAX_OBJECTS_PER_SLOT for consistency with the other
	// ItemSlotLimit()/getPerPocket() call sites in this file -- this
	// particular slot always starts empty (found via find_if above), and
	// pItemPtr itself can never hold more than MAX_OBJECTS_PER_SLOT to
	// begin with, so it's not reachable here in practice, but keeping this
	// clamped avoids relying on that invariant holding forever.
	UINT8 const ubSlotLimit    = std::min<UINT8>(ItemSlotLimit( pItemPtr->usItem, BIGPOCK1POS ), MAX_OBJECTS_PER_SLOT);

	if (ubNumberToDrop > ubSlotLimit && ubSlotLimit != 0)
	{
		// drop as many as possible into pocket
		ubNumberToDrop = ubSlotLimit;
	}

	// could be wrong type of object for slot... need to check...
	// but assuming it isn't
	slot.o = *pItemPtr;

	// Guns skip this: bStatus[0..4] alias bGunStatus/ubGunAmmoType/
	// ubGunShotsLeft/usGunAmmoItem/bGunAmmoStatus, a single value shared by
	// the whole stack (see CanGunsStack(), Items.cc) -- there's nothing
	// per-unit to zero here.
	if (ubNumberToDrop != pItemPtr->ubNumberOfObjects && !GCM->getItem(pItemPtr->usItem)->isGun())
	{
		// in the InSlot copy, zero out all the objects we didn't drop
		for (UINT8 ubLoop = ubNumberToDrop; ubLoop < pItemPtr->ubNumberOfObjects; ubLoop++)
		{
			slot.o.bStatus[ubLoop] = 0;
		}
	}
	slot.o.ubNumberOfObjects = ubNumberToDrop;

	// Same WORLDITEM bookkeeping MapInvenPoolSlotsPrimary() does when
	// placing into a previously-empty slot -- without this the item would
	// render hatched (missing WORLD_ITEM_REACHABLE) or carry a stale
	// sGridNo/usFlags left over from whatever this slot held before.
	// There's no meaningful source position here (unlike a drag, which
	// tracks sObjectSourceGridNo), so this always takes the same NOWHERE
	// fallback that path uses.
	slot.sGridNo                  = NOWHERE;
	slot.ubLevel                  = 0;
	slot.usFlags                  = WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT | WORLD_ITEM_REACHABLE;
	slot.bRenderZHeightAboveLevel = 0;

	// remove a like number of objects from pObj
	RemoveObjs( pItemPtr, ubNumberToDrop );
}


static void MapInventoryPoolNextBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		InventoryNextPage();
	}
}


static void MapInventoryPoolPrevBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		InventoryPrevPage();
	}
}


static void MapInventoryPoolDoneBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		fShowMapInventoryPool = FALSE;
	}
}


static void DisplayPagesForMapInventoryPool(void)
{
	// get the current and last pages and display them
	SetFontAttributes(FONT_VALUE_INVENTORY, 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y + COMPACT_FOOTER_TEXT_Y_OFFSET,
		ST::format("{} / {}", iCurrentInventoryPoolPage + 1, iLastInventoryPoolPage + 1),
		g_sector_inv_page_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


static size_t GetTotalNumberOfItemsInSectorStash(void)
{
	size_t numObjects = 0;

	// run through list of items and find out how many are there
	for (WORLDITEM& wi : pInventoryPoolList)
	{
		if (wi.o.ubNumberOfObjects > 0)
		{
			numObjects += wi.o.ubNumberOfObjects;
		}
	}

	return numObjects;
}


// Same idea as GetTotalNumberOfItemsInSectorStash() above, but for this
// window's own gStackSplitItems -- each entry is a physically split-out,
// 1-count OBJECTTYPE (see OpenStackSplitView()), so this naturally goes
// down as items are picked up onto the cursor (StackSplitSlotPrimary())
// while the window stays open.
static size_t GetTotalNumberOfItemsInStackSplit(void)
{
	size_t numObjects = 0;

	for (OBJECTTYPE const& o : gStackSplitItems)
	{
		numObjects += o.ubNumberOfObjects;
	}

	return numObjects;
}


// get total number of items in sector
static size_t GetTotalNumberOfItems(void)
{
	size_t numSlots = 0;

	// run through list of items and find out how many are there
	for (WORLDITEM& wi : pInventoryPoolList)
	{
		if (wi.o.ubNumberOfObjects > 0)
		{
			numSlots++;
		}
	}

	return numSlots;
}


static void DrawNumberOfInventoryPoolItems()
{
	SetFontAttributes(FONT_VALUE_INVENTORY, 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y + COMPACT_FOOTER_TEXT_Y_OFFSET,
		ST::string::from_uint(GetTotalNumberOfItemsInSectorStash()),
		g_sector_inv_count_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


static void CreateMapInventoryPoolDoneButton(void)
{
	// create done button
	guiMapInvenButton[2] = QuickCreateButtonImg(INTERFACEDIR "/DONE_BUTTON_Inventory.STI", 0, 1, MAP_SCREEN_X + 808, MAP_SCREEN_Y + 621 + COMPACT_DONE_BUTTON_Y_OFFSET, MSYS_PRIORITY_HIGHEST, MapInventoryPoolDoneBtn);
	guiMapInvenButton[2]->SetFastHelpText("Done (Sector Inventory)");
}


static void DestroyInventoryPoolDoneButton(void)
{
	// destroy ddone button
	RemoveButton( guiMapInvenButton[ 2 ] );
}


static void MapInventoryPoolGroupBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		GroupSectorInventoryItems();
	}
}


static void CreateMapInventoryGroupButton(void)
{
	// create "group items" button -- placeholder position, per user request
	guiMapInvenButton[3] = QuickCreateButtonImg(INTERFACEDIR "/sector_inventory_bookmarks.sti", GROUP_BUTTON_READY, GROUP_BUTTON_PRESSED, MAP_SCREEN_X + GROUP_BUTTON_X, MAP_SCREEN_Y + GROUP_BUTTON_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolGroupBtn);
	guiMapInvenButton[3]->SetFastHelpText("Stack, consolidate and unload items.");
}


static void DestroyMapInventoryGroupButton(void)
{
	// destroy "group items" button
	RemoveButton( guiMapInvenButton[ 3 ] );
}


// Small helper mirroring QuickCreateButtonImg()'s own simple overload, but
// creating a persistent-state TOGGLE button (BUTTON_NEWTOGGLE, via
// QuickCreateButtonToggle()) instead of a momentary one -- needed for the
// category filters below, which stay visually "on" while active, unlike
// "Grupuj przedmioty"/"Wszystkie przedmioty" (one-shot actions, plain
// QuickCreateButtonImg()).
static GUIButtonRef QuickCreateFilterToggleButton(char const* const gfx, INT32 const off_normal, INT32 const on_normal, INT16 const x, INT16 const y, INT16 const priority, GUI_CALLBACK const click)
{
	BUTTON_PICS* const img = LoadButtonImage(gfx, off_normal, on_normal);
	GUIButtonRef const btn = QuickCreateButtonToggle(img, x, y, priority, click);
	btn->uiFlags |= BUTTON_SELFDELETE_IMAGE;
	return btn;
}


// Shared body of all 7 filter toggle buttons (including "Wszystkie
// przedmioty") -- flips `category` in gubSectorInventoryActiveFilters and
// re-applies the filter. Several can be active at once (a union, not an
// intersection); per user request, every one of them -- "Wszystkie
// przedmioty" included -- toggles independently with no special-casing, so
// turning every single one off (ALL included) is a valid, reachable state
// that shows nothing.
static void ToggleSectorInventoryFilter(UINT8 category)
{
	gubSectorInventoryActiveFilters ^= category;
	ApplySectorInventoryFilter();
}

static void MapInventoryPoolAllItemsBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleSectorInventoryFilter(SECTOR_INV_FILTER_ALL);
}

static void MapInventoryPoolFilterWeaponsBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleSectorInventoryFilter(SECTOR_INV_FILTER_WEAPONS);
}

static void MapInventoryPoolFilterAttachmentsBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleSectorInventoryFilter(SECTOR_INV_FILTER_ATTACHMENTS);
}

static void MapInventoryPoolFilterAmmoBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleSectorInventoryFilter(SECTOR_INV_FILTER_AMMO);
}

static void MapInventoryPoolFilterArmourBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleSectorInventoryFilter(SECTOR_INV_FILTER_ARMOUR);
}

static void MapInventoryPoolFilterExplosivesBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleSectorInventoryFilter(SECTOR_INV_FILTER_EXPLOSIVES);
}

static void MapInventoryPoolFilterOtherBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleSectorInventoryFilter(SECTOR_INV_FILTER_OTHER);
}


static void CreateMapInventoryFilterButtons(void)
{
	// Placeholder positions, per user request -- not yet the final layout.
	// gubSectorInventoryActiveFilters is reset to SECTOR_INV_FILTER_ALL
	// whenever the panel opens (CreateDestroyMapInventoryPoolButtons()), so
	// "Wszystkie przedmioty" starts ON to match and every other filter
	// button starts OFF. All 7 are otherwise identical, independent
	// toggles from here on (ToggleSectorInventoryFilter()) -- this is a
	// one-time initialization, not an ongoing sync.
	guiMapInvenButton[4]  = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", ALL_ITEMS_BUTTON_OFF, ALL_ITEMS_BUTTON_ON, MAP_SCREEN_X + ALL_ITEMS_BUTTON_X, MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolAllItemsBtn);
	guiMapInvenButton[4]->uiFlags |= BUTTON_CLICKED_ON;
	guiMapInvenButton[4]->SetFastHelpText("Show All");
	guiMapInvenButton[5]  = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", FILTER_WEAPONS_OFF,     FILTER_WEAPONS_ON,     MAP_SCREEN_X + FILTER_WEAPONS_X,     MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolFilterWeaponsBtn);
	guiMapInvenButton[5]->SetFastHelpText("Show Guns");
	guiMapInvenButton[6]  = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", FILTER_ATTACHMENTS_OFF, FILTER_ATTACHMENTS_ON, MAP_SCREEN_X + FILTER_ATTACHMENTS_X, MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolFilterAttachmentsBtn);
	guiMapInvenButton[6]->SetFastHelpText("Show Attachments");
	guiMapInvenButton[7]  = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", FILTER_AMMO_OFF,        FILTER_AMMO_ON,        MAP_SCREEN_X + FILTER_AMMO_X,        MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolFilterAmmoBtn);
	guiMapInvenButton[7]->SetFastHelpText("Show Ammo");
	guiMapInvenButton[8]  = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", FILTER_ARMOUR_OFF,      FILTER_ARMOUR_ON,      MAP_SCREEN_X + FILTER_ARMOUR_X,      MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolFilterArmourBtn);
	guiMapInvenButton[8]->SetFastHelpText("Show Armour");
	guiMapInvenButton[9]  = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", FILTER_EXPLOSIVES_OFF,  FILTER_EXPLOSIVES_ON,  MAP_SCREEN_X + FILTER_EXPLOSIVES_X,  MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolFilterExplosivesBtn);
	guiMapInvenButton[9]->SetFastHelpText("Show Explosives");
	guiMapInvenButton[10] = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", FILTER_OTHER_OFF,       FILTER_OTHER_ON,       MAP_SCREEN_X + FILTER_OTHER_X,       MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolFilterOtherBtn);
	guiMapInvenButton[10]->SetFastHelpText("Show Miscellaneous");
}


static void DestroyMapInventoryFilterButtons(void)
{
	for (UINT32 i = 4; i <= 10; ++i) RemoveButton( guiMapInvenButton[ i ] );
}


// "Big images" toggle -- per user request. Flips gfSectorInventoryBigImages,
// then rebuilds the main grid's own slot regions from scratch at the new
// grid size/geometry (DestroyMapInventoryPoolSlots()/CreateMapInventoryPoolSlots()
// both already read GetMapInventoryPoolPageSize()/GetSectorInvSlotBox()/etc.
// fresh every time, so they pick up the new state automatically). Resets to
// page 1 -- the same absolute page number would otherwise show a
// completely unrelated slice of pInventoryPoolList once the page size
// changes (35 vs 81/90), same reasoning as OpenStackSplitView()'s own
// "always starts at page 1".
static void MapInventoryPoolBigImagesBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	DestroyMapInventoryPoolSlots();

	gfSectorInventoryBigImages = !gfSectorInventoryBigImages;
	iCurrentInventoryPoolPage  = 0;

	CreateMapInventoryPoolSlots();
	fMapPanelDirty = TRUE;
}


static void CreateMapInventoryBigImagesButton(void)
{
	guiMapInvenButton[13] = QuickCreateFilterToggleButton(INTERFACEDIR "/sector_inventory_bookmarks.sti", BIG_IMAGES_BUTTON_OFF, BIG_IMAGES_BUTTON_ON, MAP_SCREEN_X + BIG_IMAGES_BUTTON_X, MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolBigImagesBtn);
	guiMapInvenButton[13]->SetFastHelpText("Show big item icons.");
}


static void DestroyMapInventoryBigImagesButton(void)
{
	RemoveButton( guiMapInvenButton[13] );
}


// ---------------------------------------------------------------------
// Bulk transfer buttons -- move items between the selected soldier's own
// inventory (Inventory_bottom_panel.sti/Mapinv.sti, and the gun currently
// shown in ItemInfoC.sti) and the sector-inventory stash. Per user
// request.
// ---------------------------------------------------------------------

// Ejects every attachment and all loaded ammo from `gun` -- wherever it
// actually lives (a soldier's inv[] slot, or a WORLDITEM's .o already
// sitting in the stash) -- straight into the sector-inventory stash.
// Same primitives GroupWorlditemRange()'s own step 1 uses on every gun in
// the stash, just applied here to one specific gun: whichever one is
// currently shown in ItemInfoC.sti (gpItemDescObject).
static void MoveGunContentsToSectorStash(OBJECTTYPE* const gun)
{
	OBJECTTYPE ammo{};
	if (EmptyWeaponMagazine(gun, &ammo))
	{
		AutoPlaceObjectInInventoryStash(&ammo);
	}

	for (INT8 pos = MAX_ATTACHMENTS - 1; pos >= 0; --pos)
	{
		OBJECTTYPE attachment{};
		if (RemoveAttachment(gun, pos, &attachment))
		{
			AutoPlaceObjectInInventoryStash(&attachment);
		}
	}
}


// Moves every item out of the soldier's own inventory -- every slot, worn
// gear included (HANDPOS/SECONDHANDPOS/VESTPOS/HELMETPOS/LEGPOS/HEAD1-4POS,
// not just the BIGPOCK/SMALLPOCK pockets) -- into the sector-inventory
// stash. Per user request, no exceptions. The while loop only matters when
// a single inv[] slot holds more than what one stash slot can take (the
// stash ignores ubBigPerPocket/ubSmallPerPocket -- see
// PlaceObjectInInventoryStash()'s own comment -- so in practice this is a
// rare, defensive case, not the common one).
static void MoveAllMercItemsToSectorStash(SOLDIERTYPE* const soldier)
{
	for (UINT8 i = 0; i < NUM_INV_SLOTS; ++i)
	{
		while (soldier->inv[i].ubNumberOfObjects > 0)
		{
			AutoPlaceObjectInInventoryStash(&soldier->inv[i]);
		}
	}
}


// Moves items from the sector-inventory stash into the soldier's own
// inventory, in on-screen display order on the CURRENTLY VISIBLE page
// only -- per user request, "the first items that safely fit". Stops
// trying a given stash slot as soon as AutoPlaceObject() can't place any
// more of it anywhere (soldier full, or the item doesn't fit at all), but
// keeps going through the rest of the page -- a later, smaller item might
// still fit even after an earlier, bulkier one didn't.
static void MoveSectorItemsToMerc(SOLDIERTYPE* const soldier)
{
	INT32 const first_slot = iCurrentInventoryPoolPage * GetMapInventoryPoolPageSize();
	for (INT32 i = 0; i < GetMapInventoryPoolPageSize(); ++i)
	{
		WORLDITEM& wi = pInventoryPoolList[first_slot + i];
		while (wi.o.ubNumberOfObjects > 0)
		{
			if (!AutoPlaceObject(soldier, &wi.o, FALSE)) break;
		}
	}
}


// Shared validation for both transfer buttons -- same checks
// MapInvenPoolSlotsPrimary() already applies before touching the stash on
// behalf of the selected soldier (valid selection, soldier physically in
// this sector, not mid-battle), plus Mapinv.sti or ItemInfoC.sti being
// open, per user request. fShowInventoryFlag alone isn't enough:
// ItemInfoC.sti can also be opened directly from the sector-inventory
// stash itself (MAPInternalInitItemDescriptionBox(), this file), with
// Mapinv.sti never having been open at all -- InItemDescriptionBox() is
// the other half of "either one". fShowMapInventoryPool itself isn't
// re-checked here: the buttons live on that very panel, so it's already
// open by construction. HandleButtonStatesWhileMapInventoryActive() already
// disables both buttons under the same conditions; this is the defensive
// re-check right before actually moving anything.
static SOLDIERTYPE* GetSoldierForInventoryTransfer(void)
{
	if (!fShowInventoryFlag && !InItemDescriptionBox()) return NULL;

	SOLDIERTYPE* const s = GetSelectedInfoChar();
	if (s == NULL) return NULL;

	if (s->sSector.x != sSelMap.x || s->sSector.y != sSelMap.y ||
		s->sSector.z != iCurrentMapSectorZ || s->fBetweenSectors)
	{
		return NULL;
	}

	if (!CanPlayerUseSectorInventory()) return NULL;

	return s;
}


static void MapInventoryPoolMoveToSectorBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	SOLDIERTYPE* const s = GetSoldierForInventoryTransfer();
	if (s == NULL) return;

	// Combined button, per user request: if ItemInfoC.sti is currently
	// showing a gun, this empties THAT gun's ammo/attachments into the
	// stash; otherwise it empties the whole soldier's inventory instead.
	if (InItemDescriptionBox() && gpItemDescObject != NULL && GCM->getItem(gpItemDescObject->usItem)->isGun())
	{
		MoveGunContentsToSectorStash(gpItemDescObject);
	}
	else
	{
		MoveAllMercItemsToSectorStash(s);
	}

	fMapPanelDirty = TRUE;
}


static void MapInventoryPoolMoveToMercBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	SOLDIERTYPE* const s = GetSoldierForInventoryTransfer();
	if (s == NULL) return;

	MoveSectorItemsToMerc(s);
	fMapPanelDirty = TRUE;
}


static void CreateMapInventoryTransferButtons(void)
{
	// Placeholder positions, per user request -- not yet the final layout.
	guiMapInvenButton[11] = QuickCreateButtonImg(INTERFACEDIR "/sector_inventory_bookmarks.sti", MOVE_TO_SECTOR_READY, MOVE_TO_SECTOR_PRESSED, MAP_SCREEN_X + MOVE_TO_SECTOR_X, MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolMoveToSectorBtn);
	guiMapInvenButton[11]->SetFastHelpText("Move all items from mercenary to sector inventory.");
	guiMapInvenButton[12] = QuickCreateButtonImg(INTERFACEDIR "/sector_inventory_bookmarks.sti", MOVE_TO_MERC_READY,   MOVE_TO_MERC_PRESSED,   MAP_SCREEN_X + MOVE_TO_MERC_X,   MAP_SCREEN_Y + FILTER_BUTTONS_Y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolMoveToMercBtn);
	guiMapInvenButton[12]->SetFastHelpText("Move all possible items from sector to mercenary inventory.");
}


static void DestroyMapInventoryTransferButtons(void)
{
	RemoveButton( guiMapInvenButton[11] );
	RemoveButton( guiMapInvenButton[12] );
}


// Which category-filter button (if any) an item belongs to -- see
// gubSectorInventoryActiveFilters above. Order matters, since some items
// would otherwise match more than one bucket:
//  - IC_ARMOUR and IC_FACE are checked before the generic ITEM_ATTACHMENT
//    flag, so ceramic plates (themselves IC_ARMOUR -- see
//    ArmourModel::canBeAttached()) and head-slot items (night vision/UV/sun
//    goggles, gas mask, extended ear, robot remote control -- all IC_FACE,
//    worn in HEAD1-4POS the same way armour is worn in VEST/HELMET/LEGPOS)
//    land in "Tylko umundurowanie" rather than "Tylko dodatki do broni",
//    per user request.
//  - isWeapon()/isExplosive() are checked before ITEM_ATTACHMENT too, so an
//    under-barrel launcher (IC_LAUNCHER, itself attachable) or a 40mm
//    grenade (loadable into one, but IC_GRENADE) land in "Tylko bronie"/
//    "Tylko materiały wybuchowe" rather than "Tylko dodatki do broni".
//  - What's left with ITEM_ATTACHMENT is therefore narrowed down to
//    weapon-compatible attachments (scopes, silencers, bipods, ...), per
//    user request ("zawężyć wyłącznie do dodatków kompatybilnych z bronią").
//  - IC_AMMO only (not grenades) for "Tylko amunicja", per user request.
//  - Everything else (medkits, kits, keys, money, misc, ...) is
//    "Tylko pozostałe przedmioty".
static UINT8 GetSectorInventoryFilterCategory(UINT16 usItem)
{
	if (usItem == NOTHING) return 0;
	const ItemModel* const item = GCM->getItem(usItem);

	if (item->isArmour() || item->isFace())  return SECTOR_INV_FILTER_ARMOUR;
	if (item->isWeapon())                   return SECTOR_INV_FILTER_WEAPONS;
	if (item->isExplosive())                return SECTOR_INV_FILTER_EXPLOSIVES;
	if (item->getFlags() & ITEM_ATTACHMENT) return SECTOR_INV_FILTER_ATTACHMENTS;
	// GUN_BARREL_EXTENDER/SPRING_AND_BOLT_UPGRADE are crafted, permanently
	// installed gun upgrades (bInseparable/bNotBuyable in their own JSON)
	// that don't carry the generic ITEM_ATTACHMENT flag there, even though
	// the engine elsewhere (Items.cc's AttachmentInfoStruct table) already
	// treats them as real IC_GUN attachments. Per user request.
	if (usItem == GUN_BARREL_EXTENDER || usItem == SPRING_AND_BOLT_UPGRADE) return SECTOR_INV_FILTER_ATTACHMENTS;
	if (item->isAmmo())                     return SECTOR_INV_FILTER_AMMO;
	return SECTOR_INV_FILTER_OTHER;
}


// Splits every occupied slot in pInventoryPoolList into `matching` (shown
// while gubSectorInventoryActiveFilters is active -- or every occupied slot
// when it's 0, i.e. no filter) and `rest` (everything else, hidden but not
// deleted). Shared by ApplySectorInventoryFilter() and
// GroupSectorInventoryItems() so both agree on exactly what "visible" means.
static void SplitPoolListByFilter(std::vector<WORLDITEM>& matching, std::vector<WORLDITEM>& rest)
{
	for (WORLDITEM const& wi : pInventoryPoolList)
	{
		// See the occupancy comment inside GroupWorlditemRange() -- fExists
		// is not reliable here, ubNumberOfObjects is.
		if (wi.o.ubNumberOfObjects == 0) continue;

		// SECTOR_INV_FILTER_ALL short-circuits to "everything visible"
		// regardless of the item's own category (GetSectorInventoryFilterCategory()
		// never returns that bit itself). With every bit off, including
		// ALL, nothing matches -- see gubSectorInventoryActiveFilters'
		// own comment.
		bool const visible = (gubSectorInventoryActiveFilters & SECTOR_INV_FILTER_ALL) != 0 ||
			(GetSectorInventoryFilterCategory(wi.o.usItem) & gubSectorInventoryActiveFilters) != 0;
		(visible ? matching : rest).push_back(wi);
	}
}


// Rebuilds pInventoryPoolList from `matching` (kept visible: sorted, padded
// to a whole number of pages, and what pagination is based on) and `rest`
// (the hidden tail while a filter is active -- items kept, just placed
// after the last visible page so ordinary paging can never reach them;
// empty when no filter is active). This is the one place that defines what
// filtering means for the underlying list, shared by
// ApplySectorInventoryFilter() and GroupSectorInventoryItems() so a
// "Grupuj przedmioty" while filtered lands in the exact same shape a plain
// filter change would.
static void RebuildFilteredInventoryPoolList(std::vector<WORLDITEM>&& matching, std::vector<WORLDITEM>&& rest)
{
	// Grouping (if the caller ran it) can turn a previously-occupied slot
	// in `matching` into an empty one -- drop those now, same as step 3
	// used to. `rest` is never touched by grouping, so this is a no-op for
	// it, but doesn't hurt to keep both paths identical.
	std::vector<WORLDITEM> compacted_matching;
	compacted_matching.reserve(matching.size());
	for (WORLDITEM const& wi : matching) if (wi.o.ubNumberOfObjects > 0) compacted_matching.push_back(wi);

	size_t const visible_slots = compacted_matching.size();

	// Always pad the visible prefix to a whole page, even when empty -- same
	// "at least one page" convention BuildStashForSelectedSector() already
	// uses. `rest` (the hidden tail) has no such precedent: when there's
	// nothing hidden (no filter active), it must stay a true empty vector,
	// not gain a wasted blank page appended after the visible content every
	// single time the unfiltered case runs.
	size_t const matching_empty = GetMapInventoryPoolPageSize() - compacted_matching.size() % GetMapInventoryPoolPageSize();
	compacted_matching.resize(compacted_matching.size() + matching_empty, WORLDITEM{});
	if (!rest.empty())
	{
		size_t const rest_empty = GetMapInventoryPoolPageSize() - rest.size() % GetMapInventoryPoolPageSize();
		rest.resize(rest.size() + rest_empty, WORLDITEM{});
	}

	pInventoryPoolList = std::move(compacted_matching);
	pInventoryPoolList.insert(pInventoryPoolList.end(), rest.begin(), rest.end());

	// Pagination is capped to the visible prefix alone -- paging forward
	// can never reach the hidden tail, which is what makes this a real
	// filter and not just a reordering. gVisibleInventorySlotCount must be
	// kept in step: CheckAndUnDateSlotAllocation() (called every frame)
	// re-derives iLastInventoryPoolPage from it too, and re-deriving from
	// pInventoryPoolList.size() there instead re-widens pagination to cover
	// the hidden tail on the very next frame.
	gVisibleInventorySlotCount = visible_slots;
	iLastInventoryPoolPage = static_cast<INT32>(visible_slots == 0 ? 0 : (visible_slots - 1) / GetMapInventoryPoolPageSize());
	if (iCurrentInventoryPoolPage > iLastInventoryPoolPage) iCurrentInventoryPoolPage = iLastInventoryPoolPage;

	// CheckGridNoOfItemsInMapScreenMapInventory() assumes occupied slots
	// are continuous from the front (its own FIXME) -- true here only when
	// there's no hidden tail, since GetTotalNumberOfItems() (which it uses)
	// counts every occupied slot including the ones sitting in `rest`.
	// SECTOR_INV_FILTER_ALL guarantees rest is empty (see
	// SplitPoolListByFilter()); skipped otherwise, and reruns next time a
	// filter change (or BuildStashForSelectedSector()) calls this with
	// rest empty again.
	if (gubSectorInventoryActiveFilters & SECTOR_INV_FILTER_ALL) CheckGridNoOfItemsInMapScreenMapInventory();
	SortSectorInventory(pInventoryPoolList.data(), visible_slots);

	fMapPanelDirty = TRUE;
}


// The eject-ammo/strip-attachments + pairwise-merge core of "Grupuj
// przedmioty", extracted so it can run on either the whole stash (no filter
// active) or just the currently-visible filtered subset (per user request:
// grouping while filtered only affects the visible category) without
// duplicating this logic. Leaves now-empty slots in `items` for the caller
// to drop (RebuildFilteredInventoryPoolList() does this).
//
// Groups every item of the same kind into as few slots as possible, up to
// that item's own per-pocket capacity -- mirroring
// PlaceObjectInInventoryStash()'s own limit (GCM->getItem(usItem)->
// getPerPocket(), capped defensively at MAX_OBJECTS_PER_SLOT), NOT a flat 8
// for everything: non-stackable items (guns, armour, unique items --
// getPerPocket() < 2) naturally never get merged, since their single
// existing unit already fills that capacity, so no separate check for them
// is needed below.
//
// Also, for every gun already present: ejects its loaded ammo
// (EmptyWeaponMagazine()) and strips its attachments (RemoveAttachment(),
// which itself refuses to remove ITEM_INSEPARABLE ones -- respected, not
// bypassed), so those get grouped together with any other loose
// ammo/attachments of the same kind in the pass that follows. Per user
// request.
static void GroupWorlditemRange(std::vector<WORLDITEM>& items)
{
	// Step 1: eject ammo and strip attachments from every gun already
	// present. Collected into a separate list and appended only once this
	// loop is done, rather than push_back()-ing into `items` directly -- a
	// reallocation mid-loop would invalidate the WORLDITEM& reference this
	// loop is still using.
	size_t const original_count = items.size();
	std::vector<WORLDITEM> extracted;

	for (size_t i = 0; i < original_count; ++i)
	{
		WORLDITEM& slot = items[i];
		// Occupancy is decided by ubNumberOfObjects alone here -- same as
		// RenderItemInPoolSlot()/GetTotalNumberOfItems() -- NOT fExists.
		// Items dropped into the stash by hand from a merc's own inventory
		// (PlaceObjectInInventoryStash()) only ever touch the OBJECTTYPE
		// half of the slot, never WORLDITEM::fExists, so a real,
		// non-empty item can legitimately have fExists == FALSE here.
		// Requiring fExists too would silently drop exactly those items
		// once the caller compacts the result.
		if (slot.o.ubNumberOfObjects == 0) continue;

		const ItemModel* const item = GCM->getItem(slot.o.usItem);

		if (item->isGun())
		{
			OBJECTTYPE ammo{};
			if (EmptyWeaponMagazine(&slot.o, &ammo))
			{
				WORLDITEM new_item = slot;
				new_item.o = ammo;
				extracted.push_back(new_item);
			}
		}

		if (item->isWeapon())
		{
			for (INT8 pos = MAX_ATTACHMENTS - 1; pos >= 0; --pos)
			{
				OBJECTTYPE attachment{};
				if (RemoveAttachment(&slot.o, pos, &attachment))
				{
					WORLDITEM new_item = slot;
					new_item.o = attachment;
					extracted.push_back(new_item);
				}
			}
		}
	}

	items.insert(items.end(), extracted.begin(), extracted.end());

	// Step 2: merge/compact every occupied slot, grouped by item type.
	for (size_t i = 0; i < items.size(); ++i)
	{
		WORLDITEM& dest_wi = items[i];
		if (dest_wi.o.ubNumberOfObjects == 0) continue;

		UINT16 const usItem = dest_wi.o.usItem;

		if (usItem == MONEY)
		{
			// Money doesn't use bStatus[]/ubNumberOfObjects the way every
			// other stackable item does (it's a single uiMoneyAmount), so
			// it can't go through CleanUpStack()/StackObjs() below --
			// combine it the same way PlaceObjectInInventoryStash() already
			// does for a single manual drop.
			for (size_t j = i + 1; j < items.size(); ++j)
			{
				WORLDITEM& src_wi = items[j];
				if (src_wi.o.usItem != MONEY || src_wi.o.ubNumberOfObjects == 0) continue;

				dest_wi.o.bMoneyStatus = 100;
				dest_wi.o.uiMoneyAmount += src_wi.o.uiMoneyAmount;
				DeleteObj(&src_wi.o);
			}
			continue;
		}

		UINT8 const slot_limit = std::min<UINT8>(GCM->getItem(usItem)->getPerPocket(), MAX_OBJECTS_PER_SLOT);

		for (size_t j = i + 1; j < items.size(); ++j)
		{
			WORLDITEM& src_wi = items[j];
			if (src_wi.o.usItem != usItem || src_wi.o.ubNumberOfObjects == 0) continue;

			// Merge partial charges first (ammo/kits/canteens/alcohol/etc.
			// -- see Merge[] in Items.cc). Safe no-op for items it doesn't
			// recognize as combinable.
			CleanUpStack(&dest_wi.o, &src_wi.o);

			// Physically move any whole units still left in src into dest,
			// up to dest's own per-pocket capacity. For non-stackable items
			// (slot_limit <= 1) dest already holds exactly 1, so this never
			// triggers -- no separate guard needed.
			//
			// CanGunsStack() additionally requires two guns to be
			// physically indistinguishable (no ammo, no attachments,
			// identical condition) before they may share one OBJECTTYPE --
			// see its own comment (Items.cc). Guns that don't qualify are
			// simply left as separate slots (a no-op here, not an error).
			if (src_wi.o.ubNumberOfObjects > 0 && dest_wi.o.ubNumberOfObjects < slot_limit &&
				CanGunsStack(dest_wi.o, src_wi.o))
			{
				UINT8 const room    = slot_limit - dest_wi.o.ubNumberOfObjects;
				UINT8 const to_move = std::min<UINT8>(src_wi.o.ubNumberOfObjects, room);
				StackObjs(&src_wi.o, &dest_wi.o, to_move);
			}
		}
	}
}


static void GroupSectorInventoryItems(void)
{
	// With no filter active, SplitPoolListByFilter() puts every occupied
	// slot into `matching` and leaves `rest` empty -- identical to
	// grouping the whole stash, as before. With a filter active, only the
	// visible category is grouped; `rest` (the hidden tail) is never
	// touched, per user request.
	std::vector<WORLDITEM> matching, rest;
	SplitPoolListByFilter(matching, rest);
	GroupWorlditemRange(matching);
	RebuildFilteredInventoryPoolList(std::move(matching), std::move(rest));
}


// "Wszystkie przedmioty"/category-filter buttons -- see
// gubSectorInventoryActiveFilters above. Re-partitions the existing list by
// the (now-changed) active filter set; nothing is grouped or otherwise
// mutated, items just become visible/hidden.
static void ApplySectorInventoryFilter(void)
{
	std::vector<WORLDITEM> matching, rest;
	SplitPoolListByFilter(matching, rest);
	iCurrentInventoryPoolPage = 0; // per user request: filter change resets to page 1
	RebuildFilteredInventoryPoolList(std::move(matching), std::move(rest));
}


static void DisplayCurrentSector(void)
{
	// grab current sector being displayed
	SetFontAttributes(FONT_VALUE_INVENTORY, 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y + COMPACT_FOOTER_TEXT_Y_OFFSET,
		ST::format("{}{}{}", pMapVertIndex[ sSelMap.y ],
			pMapHortIndex[ sSelMap.x ], pMapDepthIndex[ iCurrentMapSectorZ ]),
		g_sector_inv_loc_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


static void CheckAndUnDateSlotAllocation(void)
{
	// will check number of available slots, if less than half a page, allocate a new page
	size_t numTakenSlots = GetTotalNumberOfItems();

	if ((pInventoryPoolList.size() - numTakenSlots) < 2)
	{
		// not enough space
		// need to make more space
		pInventoryPoolList.insert(pInventoryPoolList.end(), GetMapInventoryPoolPageSize(), WORLDITEM{});
	}

	// Derived from gVisibleInventorySlotCount, NOT pInventoryPoolList.size()
	// -- this runs every frame (BlitInventoryPoolGraphic()), and the list's
	// full size includes the hidden tail while a category filter is
	// active. Re-deriving from the full size here silently re-widened
	// pagination to cover that hidden tail again right after a filter
	// change had capped it (gubSectorInventoryActiveFilters,
	// RebuildFilteredInventoryPoolList()) -- the very next page turn would
	// then reveal items from other categories.
	iLastInventoryPoolPage = ( ( static_cast<INT32>(gVisibleInventorySlotCount) - 1 ) / GetMapInventoryPoolPageSize() );
}


static void DrawTextOnSectorInventory(void);


static void DrawTextOnMapInventoryBackground(void)
{
	UINT16 usStringHeight;

	SetFontDestBuffer(guiSAVEBUFFER);

	int xPos = MAP_SCREEN_X + 392;
	int yPos = MAP_SCREEN_Y + 634 + COMPACT_FOOTER_TEXT_Y_OFFSET;

	//Calculate the height of the string, as it needs to be vertically centered.
	usStringHeight = DisplayWrappedString(xPos, yPos, 53, 1, FONT_TEXT_INVENTORY, FONT_BEIGE, pMapInventoryStrings[0], FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
	DisplayWrappedString(xPos, yPos - (usStringHeight / 2), 53, 1, FONT_TEXT_INVENTORY, FONT_BEIGE, pMapInventoryStrings[0], FONT_BLACK, RIGHT_JUSTIFIED);

	xPos = MAP_SCREEN_X + 506;

	//Calculate the height of the string, as it needs to be vertically centered.
	usStringHeight = DisplayWrappedString(xPos, yPos, 65, 1, FONT_TEXT_INVENTORY, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
	DisplayWrappedString( xPos, yPos - (usStringHeight / 2), 65, 1, FONT_TEXT_INVENTORY, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED);

	DrawTextOnSectorInventory( );

	SetFontDestBuffer(FRAME_BUFFER);
}


void HandleButtonStatesWhileMapInventoryActive( void )
{
	// are we even showing the amp inventory pool graphic?
	if (!fShowMapInventoryPool) return;

	// Stack split view (Wariant B) open -- changing page or re-grouping
	// while a stack is physically split out into gStackSplitItems would
	// strand it away from its (about to change) source slot;
	// CloseStackSplitView() must run first (see MapInvenPoolSlotsSecondary()/
	// CreateDestroyMapInventoryPoolButtons()).
	BOOLEAN const fStackSplitOpen = (gStackSplitSourceIndex != -1);

	// The main grid's own next/prev arrows -- HIDDEN entirely (not just
	// disabled) while the stack split view is open, per user report:
	// EnableButton(FALSE) alone only blocks clicks (BUTTON_ENABLED), it
	// doesn't stop GUI_BUTTON::Draw() (which checks the underlying mouse
	// region's own enabled state instead) -- so these stayed visibly drawn
	// on screen, at the exact same coordinates as this window's own,
	// independent arrows (STACK_SPLIT_PREV_X/NEXT_X, STACK_SPLIT_ARROWS_Y),
	// causing a visible animation/z-order conflict between the two
	// overlapping button pairs when clicked.
	if (fStackSplitOpen)
	{
		HideButton(guiMapInvenButton[0]);
		HideButton(guiMapInvenButton[1]);
	}
	else
	{
		ShowButton(guiMapInvenButton[0]);
		ShowButton(guiMapInvenButton[1]);
		// first page, can't go back any
		EnableButton(guiMapInvenButton[1], iCurrentInventoryPoolPage != 0);
		// last page, go no further
		EnableButton(guiMapInvenButton[0], iCurrentInventoryPoolPage != iLastInventoryPoolPage);
	}
	// item picked up ..disable button
	EnableButton(guiMapInvenButton[2], !fMapInventoryItem);
	// "Group Items" -- disabled while the stack split view is open
	EnableButton(guiMapInvenButton[3], !fStackSplitOpen);
	// "Wszystkie przedmioty" + the 6 category filters -- same rule: they
	// all mutate pInventoryPoolList, which the stack split view is
	// borrowing items out of (gStackSplitItems) while open (A5: the split
	// view itself is independent of filtering, but the main grid's own
	// controls still can't safely run underneath it).
	for (UINT32 i = 4; i <= 10; ++i) EnableButton(guiMapInvenButton[i], !fStackSplitOpen);

	// "Big images" toggle -- same rule as the filters above: it rebuilds
	// the main grid's own slot regions (MapInventoryPoolBigImagesBtn()),
	// which the stack split view's items are borrowed out of while open.
	EnableButton(guiMapInvenButton[13], !fStackSplitOpen);

	// The two transfer buttons -- disabled under the same fStackSplitOpen
	// rule as above, plus GetSoldierForInventoryTransfer()'s own checks
	// (Mapinv.sti open, a valid soldier selected, physically in this
	// sector, not mid-battle) -- see its own comment for why
	// fShowMapInventoryPool itself isn't re-checked here.
	BOOLEAN const fCanTransfer = !fStackSplitOpen && GetSoldierForInventoryTransfer() != NULL;
	EnableButton(guiMapInvenButton[11], fCanTransfer);
	EnableButton(guiMapInvenButton[12], fCanTransfer);

	// Stack split view's own Done button -- disabled while holding an item
	// on the cursor (picked up from here via StackSplitSlotPrimary(), or
	// from anywhere else on the map screen), same convention as the main
	// panel's own Done button above.
	if (fStackSplitOpen) EnableButton(gStackSplitDoneButton, !fMapInventoryItem);

	// Stack split view's own, independent page arrows -- same first/last
	// page rule as the main grid's own next/prev above.
	if (fStackSplitOpen)
	{
		EnableButton(gStackSplitPrevBtn, gCurrentStackSplitPage != 0);
		EnableButton(gStackSplitNextBtn, gCurrentStackSplitPage != gLastStackSplitPage);
	}
}


static void DrawTextOnSectorInventory(void)
{
	// Prints "Sector Inventory" in the English localization.

	SetFontDestBuffer(guiSAVEBUFFER);
	SetFontAttributes(FONT14ARIAL, FONT_WHITE);

	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y,
		zMarksMapScreenText[11], g_sector_inv_title_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


void HandleFlashForHighLightedItem( void )
{
	UINT32 uiCurrentTime = 0;
	INT32 iDifference = 0;


	// if there is an invalid item, reset
	if( iCurrentlyHighLightedItem == -1 )
	{
		fFlashHighLightInventoryItemOnradarMap = FALSE;
		guiFlashHighlightedItemBaseTime = 0;
	}

	// get the current time
	uiCurrentTime = GetJA2Clock();

	// if there basetime is uninit
	if( guiFlashHighlightedItemBaseTime == 0 )
	{
		guiFlashHighlightedItemBaseTime = uiCurrentTime;
	}


	iDifference = uiCurrentTime - guiFlashHighlightedItemBaseTime;

	if( iDifference > DELAY_FOR_HIGHLIGHT_ITEM_FLASH )
	{
		// reset timer
		guiFlashHighlightedItemBaseTime = uiCurrentTime;

		// flip flag
		fFlashHighLightInventoryItemOnradarMap = !fFlashHighLightInventoryItemOnradarMap;

		// re render radar map
		RenderRadarScreen( );

	}
}


static void ResetMapSectorInventoryPoolHighLights();


static void HandleMouseInCompatableItemForMapSectorInventory(INT32 iCurrentSlot)
{
	SOLDIERTYPE *pSoldier = NULL;
	static BOOLEAN fItemWasHighLighted = FALSE;

	if( iCurrentSlot == -1 )
	{
		guiCompatibleItemBaseTime = 0;
	}

	if (fChangedInventorySlots)
	{
		guiCompatibleItemBaseTime = 0;
		fChangedInventorySlots = FALSE;
	}

	// reset the base time to the current game clock
	if( guiCompatibleItemBaseTime == 0 )
	{
		guiCompatibleItemBaseTime = GetJA2Clock( );

		if (fItemWasHighLighted)
		{
			fTeamPanelDirty = TRUE;
			fMapPanelDirty = TRUE;
			fItemWasHighLighted = FALSE;
		}
	}

	ResetCompatibleItemArray( );
	ResetMapSectorInventoryPoolHighLights( );

	if( iCurrentSlot == -1 )
	{
		return;
	}

	// given this slot value, check if anything in the displayed sector inventory or on the mercs inventory is compatable
	if( fShowInventoryFlag )
	{
		// check if any compatable items in the soldier inventory matches with this item
		if( gfCheckForCursorOverMapSectorInventoryItem )
		{
			const SOLDIERTYPE* const pSoldier = GetSelectedInfoChar();
			if( pSoldier )
			{
				if( HandleCompatibleAmmoUIForMapScreen( pSoldier, iCurrentSlot + ( iCurrentInventoryPoolPage * GetMapInventoryPoolPageSize() ), TRUE, FALSE ) )
				{
					if( GetJA2Clock( ) - guiCompatibleItemBaseTime > 100 )
					{
						if (!fItemWasHighLighted)
						{
							fTeamPanelDirty = TRUE;
							fItemWasHighLighted = TRUE;
						}
					}
				}
			}
		}
		else
		{
			guiCompatibleItemBaseTime = 0;
		}
	}


	// now handle for the sector inventory
	if( fShowMapInventoryPool )
	{
		// check if any compatable items in the soldier inventory matches with this item
		if( gfCheckForCursorOverMapSectorInventoryItem )
		{
			if( HandleCompatibleAmmoUIForMapInventory( pSoldier, iCurrentSlot, ( iCurrentInventoryPoolPage * GetMapInventoryPoolPageSize() ) , TRUE, FALSE ) )
			{
				if( GetJA2Clock( ) - guiCompatibleItemBaseTime > 100 )
				{
					if (!fItemWasHighLighted)
					{
						fItemWasHighLighted = TRUE;
						fMapPanelDirty = TRUE;
					}
				}
			}
		}
		else
		{
			guiCompatibleItemBaseTime = 0;
		}
	}
}


static void ResetMapSectorInventoryPoolHighLights()
{ // Reset the highlight list for the map sector inventory.
	FOR_EACH(BOOLEAN, i, fMapInventoryItemCompatable) *i = FALSE;
}


static void HandleMapSectorInventory(void)
{
	// handle mouse in compatable item map sectors inventory
	HandleMouseInCompatableItemForMapSectorInventory( iCurrentlyHighLightedItem );
}


//CJC look here to add/remove checks for the sector inventory
BOOLEAN IsMapScreenWorldItemVisibleInMapInventory(const WORLDITEM& wi)
{
	if (wi.fExists             &&
			wi.bVisible == VISIBLE &&
			wi.o.usItem != SWITCH &&
			wi.o.usItem != ACTION_ITEM &&
			wi.o.bTrap <= 0 )
	{
		return( TRUE );
	}

	return( FALSE );
}


//Check to see if any of the items in the list have a gridno of NOWHERE and the entry point flag NOT set
static void CheckGridNoOfItemsInMapScreenMapInventory(void)
{
	size_t uiNumFlagsNotSet = 0;
	size_t numTakenSlots = GetTotalNumberOfItems();


	for (size_t iCnt = 0; iCnt < numTakenSlots; iCnt++)// FIXME this only works properly when the taken slots are continuous
	{
		if( pInventoryPoolList[ iCnt ].sGridNo == NOWHERE && !( pInventoryPoolList[ iCnt ].usFlags & WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT ) )
		{
			//set the flag
			pInventoryPoolList[ iCnt ].usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;

			//count the number
			uiNumFlagsNotSet++;
		}
	}


	//loop through all the UNSEEN items
	for (size_t iCnt = 0; iCnt < pUnSeenItems.size(); iCnt++)
	{
		if( pUnSeenItems[ iCnt ].sGridNo == NOWHERE && !( pUnSeenItems[ iCnt ].usFlags & WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT ) )
		{
			//set the flag
			pUnSeenItems[ iCnt ].usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;

			//count the number
			uiNumFlagsNotSet++;
		}
	}

	if( uiNumFlagsNotSet > 0 )
	{
		SLOGD("Item with invalid gridno doesnt have flag set: {}", uiNumFlagsNotSet);
	}
}


static INT32 MapScreenSectorInventoryCompare(const void* pNum1, const void* pNum2);


static void SortSectorInventory(WORLDITEM* pInventory, size_t sizeOfArray)
{
	qsort(pInventory, sizeOfArray, sizeof(WORLDITEM), MapScreenSectorInventoryCompare);
}


static INT32 MapScreenSectorInventoryCompare(const void* pNum1, const void* pNum2)
{
	WORLDITEM *pFirst = (WORLDITEM *)pNum1;
	WORLDITEM *pSecond = (WORLDITEM *)pNum2;
	UINT16	usItem1Index;
	UINT16	usItem2Index;
	UINT8		ubItem1Quality;
	UINT8		ubItem2Quality;

	usItem1Index = pFirst->o.usItem;
	usItem2Index = pSecond->o.usItem;

	ubItem1Quality = pFirst->o.bStatus[ 0 ];
	ubItem2Quality = pSecond->o.bStatus[ 0 ];

	return( CompareItemsForSorting( usItem1Index, usItem2Index, ubItem1Quality, ubItem2Quality ) );
}


static BOOLEAN CanPlayerUseSectorInventory(void)
{
	SGPSector sector;
	return
		!GetCurrentBattleSectorXYZAndReturnTRUEIfThereIsABattle(sector) ||
		sSelMap.x           != sector.x ||
		sSelMap.y           != sector.y ||
		iCurrentMapSectorZ != sector.z;
}
