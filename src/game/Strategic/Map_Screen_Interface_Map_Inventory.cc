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

// inventory pool slot positions and sizes -- ROW Y = 11 per user request;
// column count (ROW X = 9) follows from MAP_INVENTORY_POOL_SLOT_COUNT / this
// (Map_Screen_Interface_Map_Inventory.h)
#define MAP_INV_SLOT_ROWS 11


static const SGPBox g_sector_inv_box        = { 261,   0, 762, 648 };
static const SGPBox g_sector_inv_title_box  = { 266,   5, 370,  29 };
static const SGPBox g_sector_inv_slot_box   = { 274,  37,  83,  52 };
static const SGPBox g_sector_inv_region_box = {   11,   28,  72,  33 }; // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_item_box   = {   11,   28,  72,  33 }; // relative to g_sector_inv_slot_box
// x is intentionally UINT16(-1) (== 65535, wrapping) to shift the bar 1px
// left of the item box -- SGPBox's fields are unsigned so a plain -1
// literal here would silently narrow (MSVC C4838). The explicit cast keeps
// the exact same value (and thus the exact same on-screen position, since
// it's added to dx and truncated back down to INT16 in
// DrawItemUIBarEx()'s sXPos parameter, which cancels the wraparound out to
// dx - 1) while making the intent clear and silencing the warning.
static const SGPBox g_sector_inv_bar_box    = { (UINT16)5,   30,   2,  31 }; // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_name_box   = {   6,  65,  75,   10 }; // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_loc_box    = { 709, 630,  39,  10 };
static const SGPBox g_sector_inv_count_box  = { 800, 630,  39,  10 };
static const SGPBox g_sector_inv_page_box   = { 868, 630,  50,  10 };


// the current highlighted item
INT32 iCurrentlyHighLightedItem = -1;
BOOLEAN fFlashHighLightInventoryItemOnradarMap = FALSE;

// whether we are showing the inventory pool graphic
BOOLEAN fShowMapInventoryPool = FALSE;

// the v-object index value for the background
static cache_key_t const guiMapInventoryPoolBackground{ INTERFACEDIR "/sector_inventory.sti" };

// "Group Items" button -- see GroupSectorInventoryItems()/CreateMapInventoryGroupButton().
// Like done_button.sti/map_screen_bottom_arrows.sti above, QuickCreateButtonImg()
// manages this image's lifetime itself; no cache_key_t needed here.
#define GROUP_BUTTON_READY   0
#define GROUP_BUTTON_PRESSED 1
// Placeholder position, per user request -- not yet the final layout.
#define GROUP_BUTTON_X 278
#define GROUP_BUTTON_Y 16

// inventory pool list
std::vector<WORLDITEM> pInventoryPoolList;

// current page of inventory
INT32 iCurrentInventoryPoolPage = 0;
static INT32 iLastInventoryPoolPage = 0;

INT16 sObjectSourceGridNo = 0;

// the inventory slots
static MOUSE_REGION MapInventoryPoolSlots[MAP_INVENTORY_POOL_SLOT_COUNT];
static MOUSE_REGION MapInventoryPoolMask;
BOOLEAN fMapInventoryItemCompatable[ MAP_INVENTORY_POOL_SLOT_COUNT ];
static BOOLEAN      fChangedInventorySlots = FALSE;

// the unseen items list...have to save this
static std::vector<WORLDITEM> pUnSeenItems;

UINT32 guiFlashHighlightedItemBaseTime = 0;
UINT32 guiCompatibleItemBaseTime = 0;

// [0] = next page, [1] = previous page, [2] = done, [3] = group items
static GUIButtonRef guiMapInvenButton[4];

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

static cache_key_t const guiStackSplitBackground{ INTERFACEDIR "/sector_inventory_2.sti" };

// Placeholder positions, per user request -- not yet the final layout.
// g_stack_split_box is the whole window (background + slots); the slot box
// is the pitch between slots, and the rest are relative to each individual
// slot -- same layering as g_sector_inv_slot_box/_region_box/_item_box/etc.
// above.
static const SGPBox g_stack_split_box        = { 261, 0, 762, 468 };
static const SGPBox g_stack_split_slot_box   = {  10,  30,  83,  52 };
static const SGPBox g_stack_split_region_box = {  11,  34,  72,  33 }; // relative to g_stack_split_slot_box
static const SGPBox g_stack_split_item_box   = {  11,  34,  72,  33 }; // relative to g_stack_split_slot_box
static const SGPBox g_stack_split_bar_box    = { (UINT16)8, 37, 2, 31 }; // relative to g_stack_split_slot_box
static const SGPBox g_stack_split_name_box   = {   9,  72,  75,  10 }; // relative to g_stack_split_slot_box

// Placeholder position, per user request -- not yet the final layout.
#define STACK_SPLIT_DONE_X 950
#define STACK_SPLIT_DONE_Y 446

// Slots laid out in a small grid, wide enough for a whole stack (a stack
// can never hold more than MAX_OBJECTS_PER_SLOT items to begin with).
#define STACK_SPLIT_COLS 4

// The physically-split-out items, one per slot -- empty (gStackSplitItems
// cleared) when the view is closed.
static std::vector<OBJECTTYPE> gStackSplitItems;
// Index into pInventoryPoolList (absolute -- already includes the page
// offset) of the stack currently split open here, or -1 when closed.
static INT32 gStackSplitSourceIndex = -1;
static MOUSE_REGION gStackSplitSlots[MAX_OBJECTS_PER_SLOT];
// Background region: purely a click-blocker so a stray click inside the
// window's background doesn't fall through to the main sector-inventory
// grid underneath it -- does NOT close the view. Per user request, this
// window closes only via its own Done button (gStackSplitDoneButton)
// below, not via left/right click.
static MOUSE_REGION gStackSplitBackgroundRegion;
static GUIButtonRef gStackSplitDoneButton;


// remove background panel graphics for inventory
void RemoveInventoryPoolGraphic( void )
{
	RemoveVObject(guiMapInventoryPoolBackground);
}


static void CheckAndUnDateSlotAllocation(void);
static void DisplayCurrentSector(void);
static void DisplayPagesForMapInventoryPool(void);
static void DrawNumberOfInventoryPoolItems();
static void DrawTextOnMapInventoryBackground(void);
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
	BltVideoObject(guiSAVEBUFFER, guiMapInventoryPoolBackground, 0, MAP_SCREEN_X + box->x, MAP_SCREEN_Y + box->y);

	// resize list
	CheckAndUnDateSlotAllocation( );


	// now the items
	RenderItemsForCurrentPageOfInventoryPool( );

	// now update help text
	UpdateHelpTextForInvnentoryStashSlots( );

	// show which page and last page
	DisplayPagesForMapInventoryPool( );

	// draw number of items in current inventory
	DrawNumberOfInventoryPoolItems();

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
	for( iCounter = 0; iCounter < MAP_INVENTORY_POOL_SLOT_COUNT ; iCounter++ )
	{
		RenderItemInPoolSlot( iCounter, ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT ) );
	}
}


static BOOLEAN RenderItemInPoolSlot(INT32 iCurrentSlot, INT32 iFirstSlotOnPage)
{
	// render item in this slot of the list
	const WORLDITEM& item = pInventoryPoolList[iCurrentSlot + iFirstSlotOnPage];

	// check if anything there
	if (item.o.ubNumberOfObjects == 0) return FALSE;

	const SGPBox* const slot_box = &g_sector_inv_slot_box;
	const INT32 dx = MAP_SCREEN_X + slot_box->x + slot_box->w * (iCurrentSlot / MAP_INV_SLOT_ROWS);
	const INT32 dy = MAP_SCREEN_Y + slot_box->y + slot_box->h * (iCurrentSlot % MAP_INV_SLOT_ROWS);

	SetFontDestBuffer(guiSAVEBUFFER);
	const SGPBox* const item_box = &g_sector_inv_item_box;
	const UINT16        outline  = fMapInventoryItemCompatable[iCurrentSlot] ? Get16BPPColor(FROMRGB(255, 255, 255)) : SGP_TRANSPARENT;
	INVRenderItem(guiSAVEBUFFER, NULL, item.o, dx + item_box->x, dy + item_box->y, item_box->w, item_box->h, DIRTYLEVEL2, 0, outline);

	// draw bar for condition
	const UINT16 col0 = Get16BPPColor(DESC_STATUS_BAR);
	const UINT16 col1 = Get16BPPColor(DESC_STATUS_BAR_SHADOW);
	const SGPBox* const bar_box = &g_sector_inv_bar_box;
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
	const SGPBox* const name_box = &g_sector_inv_name_box;
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
	INT32 iFirstSlotOnPage = ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT );


	// run through list of items in slots and update help text for mouse regions
	for( iCounter = 0; iCounter < MAP_INVENTORY_POOL_SLOT_COUNT; iCounter++ )
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
static void CreateStackSplitSlots(void);
static void CreateStackSplitDoneButton(void);
static void DestroyInventoryPoolDoneButton(void);
static void DestroyMapInventoryButtons(void);
static void DestroyMapInventoryPoolSlots();
static void DestroyMapInventoryGroupButton(void);
static void DestroyStackSplitSlots(void);
static void DestroyStackSplitDoneButton(void);
static void DestroyStash(void);
static void GroupSectorInventoryItems(void);
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

		// also create the inventory slot
		CreateMapInventoryPoolSlots( );

		// create buttons
		CreateMapInventoryButtons( );

		// build stash
		BuildStashForSelectedSector(sector);

		CreateMapInventoryPoolDoneButton( );

		CreateMapInventoryGroupButton( );

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

		// now save results
		SaveSeenAndUnseenItems( );

		DestroyStash( );



		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;

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

	const SGPBox* const slot_box = &g_sector_inv_slot_box;
	const SGPBox* const reg_box  = &g_sector_inv_region_box;
	for (UINT i = 0; i < MAP_INVENTORY_POOL_SLOT_COUNT; ++i)
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
	FOR_EACH(MOUSE_REGION, i, MapInventoryPoolSlots) MSYS_RemoveRegion(&*i);
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
	WORLDITEM& slot = pInventoryPoolList[iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT + slot_idx];

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
	INT32      const abs_idx  = iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT + slot_idx;
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

	size_t const count = gStackSplitItems.size();
	for (size_t i = 0; i < count; ++i)
	{
		UINT16        const col = static_cast<UINT16>(i % STACK_SPLIT_COLS);
		UINT16        const row = static_cast<UINT16>(i / STACK_SPLIT_COLS);
		UINT16        const dx  = bx + g_stack_split_slot_box.x + col * g_stack_split_slot_box.w;
		UINT16        const dy  = by + g_stack_split_slot_box.y + row * g_stack_split_slot_box.h;
		UINT16        const x   = dx + g_stack_split_region_box.x;
		UINT16        const y   = dy + g_stack_split_region_box.y;
		MOUSE_REGION* const r   = &gStackSplitSlots[i];
		MSYS_DefineRegion(r, x, y, x + g_stack_split_region_box.w - 1, y + g_stack_split_region_box.h - 1,
			MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR, MSYS_NO_CALLBACK,
			MouseCallbackPrimarySecondary(StackSplitSlotPrimary, StackSplitSlotSecondary, MSYS_NO_CALLBACK));
		MSYS_SetRegionUserData(r, 0, static_cast<UINT32>(i));
	}
}


static void DestroyStackSplitSlots(void)
{
	size_t const count = gStackSplitItems.size();
	for (size_t i = 0; i < count; ++i) MSYS_RemoveRegion(&gStackSplitSlots[i]);
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
	gStackSplitDoneButton = QuickCreateButtonImg(INTERFACEDIR "/done_button.sti", 0, 1,
		MAP_SCREEN_X + STACK_SPLIT_DONE_X, MAP_SCREEN_Y + STACK_SPLIT_DONE_Y, MSYS_PRIORITY_HIGHEST, StackSplitDoneBtn);
}


static void DestroyStackSplitDoneButton(void)
{
	RemoveButton(gStackSplitDoneButton);
}


static void RenderStackSplitItems(void)
{
	if (gStackSplitSourceIndex == -1) return;

	UINT16 const bx = MAP_SCREEN_X + g_stack_split_box.x;
	UINT16 const by = MAP_SCREEN_Y + g_stack_split_box.y;

	BltVideoObject(guiSAVEBUFFER, guiStackSplitBackground, 0, bx, by);

	SetFontDestBuffer(guiSAVEBUFFER);
	for (size_t i = 0; i < gStackSplitItems.size(); ++i)
	{
		OBJECTTYPE const& o = gStackSplitItems[i];
		if (o.usItem == NOTHING) continue;

		INT32 const col = static_cast<INT32>(i % STACK_SPLIT_COLS);
		INT32 const row = static_cast<INT32>(i / STACK_SPLIT_COLS);
		INT32 const dx  = bx + g_stack_split_slot_box.x + col * g_stack_split_slot_box.w;
		INT32 const dy  = by + g_stack_split_slot_box.y + row * g_stack_split_slot_box.h;

		const SGPBox* const item_box = &g_stack_split_item_box;
		INVRenderItem(guiSAVEBUFFER, NULL, o, dx + item_box->x, dy + item_box->y, item_box->w, item_box->h, DIRTYLEVEL2, 0, SGP_TRANSPARENT);

		const UINT16        col0    = Get16BPPColor(DESC_STATUS_BAR);
		const UINT16        col1    = Get16BPPColor(DESC_STATUS_BAR_SHADOW);
		const SGPBox* const bar_box = &g_stack_split_bar_box;
		DrawItemUIBarEx(o, 0, dx + bar_box->x, dy + bar_box->y + bar_box->h - 1, bar_box->h, col0, col1, guiSAVEBUFFER);

		const SGPBox* const name_box = &g_stack_split_name_box;
		auto sString = ReduceStringLength(GCM->getItem(o.usItem)->getShortName(), name_box->w, MAP_SECTOR_INV_ITEM_FONT);
		SetFontAttributes(MAP_SECTOR_INV_ITEM_FONT, 5, DEFAULT_SHADOW);
		MPrintCenteredInBox(dx - 1, dy, sString, *name_box);
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

	CreateStackSplitSlots();
	CreateStackSplitDoneButton();

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
	guiMapInvenButton[0] = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", 10, 1, -1, 3, -1, MAP_SCREEN_X + 922, MAP_SCREEN_Y + 629, MSYS_PRIORITY_HIGHEST, MapInventoryPoolNextBtn);
	guiMapInvenButton[1] = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti",  9, 0, -1, 2, -1, MAP_SCREEN_X + 850, MAP_SCREEN_Y + 629, MSYS_PRIORITY_HIGHEST, MapInventoryPoolPrevBtn);

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
	size_t empty_slots = MAP_INVENTORY_POOL_SLOT_COUNT - visible_slots % MAP_INVENTORY_POOL_SLOT_COUNT;
	pInventoryPoolList.resize(visible_slots + empty_slots, WORLDITEM{});
	iLastInventoryPoolPage  = static_cast<INT32>((pInventoryPoolList.size() - 1) / MAP_INVENTORY_POOL_SLOT_COUNT);

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
		pInventoryPoolList.insert(pInventoryPoolList.end(), MAP_INVENTORY_POOL_SLOT_COUNT, WORLDITEM{});
		it = pInventoryPoolList.begin() + old_size;
		iLastInventoryPoolPage = static_cast<INT32>((pInventoryPoolList.size() - 1) / MAP_INVENTORY_POOL_SLOT_COUNT);
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
	SetFontAttributes(COMPFONT, 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y,
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
	SetFontAttributes(COMPFONT, 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y,
		ST::string::from_uint(GetTotalNumberOfItemsInSectorStash()),
		g_sector_inv_count_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


static void CreateMapInventoryPoolDoneButton(void)
{
	// create done button
	guiMapInvenButton[2] = QuickCreateButtonImg(INTERFACEDIR "/done_button.sti", 0, 1, MAP_SCREEN_X + 950, MAP_SCREEN_Y + 626, MSYS_PRIORITY_HIGHEST, MapInventoryPoolDoneBtn);
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
}


static void DestroyMapInventoryGroupButton(void)
{
	// destroy "group items" button
	RemoveButton( guiMapInvenButton[ 3 ] );
}


// Groups every item of the same kind in the sector-inventory stash into as
// few slots as possible, up to that item's own per-pocket capacity --
// mirroring PlaceObjectInInventoryStash()'s own limit
// (GCM->getItem(usItem)->getPerPocket(), capped defensively at
// MAX_OBJECTS_PER_SLOT), NOT a flat 8 for everything: non-stackable items
// (guns, armour, unique items -- getPerPocket() < 2) naturally never get
// merged, since their single existing unit already fills that capacity, so
// no separate check for them is needed below.
//
// Also, for every gun already in the stash: ejects its loaded ammo
// (EmptyWeaponMagazine()) and strips its attachments (RemoveAttachment(),
// which itself refuses to remove ITEM_INSEPARABLE ones -- respected, not
// bypassed), so those get grouped together with any other loose
// ammo/attachments of the same kind in the pass that follows. Per user
// request.
static void GroupSectorInventoryItems(void)
{
	// Step 1: eject ammo and strip attachments from every gun already in
	// the stash. Collected into a separate list and appended only once
	// this loop is done, rather than push_back()-ing into
	// pInventoryPoolList directly -- a reallocation mid-loop would
	// invalidate the WORLDITEM& reference this loop is still using.
	size_t const original_count = pInventoryPoolList.size();
	std::vector<WORLDITEM> extracted;

	for (size_t i = 0; i < original_count; ++i)
	{
		WORLDITEM& slot = pInventoryPoolList[i];
		// Occupancy is decided by ubNumberOfObjects alone here -- same as
		// RenderItemInPoolSlot()/GetTotalNumberOfItems() -- NOT fExists.
		// Items dropped into the stash by hand from a merc's own inventory
		// (PlaceObjectInInventoryStash()) only ever touch the OBJECTTYPE
		// half of the slot, never WORLDITEM::fExists, so a real,
		// non-empty item can legitimately have fExists == FALSE here.
		// Requiring fExists too silently dropped exactly those items
		// during the step 3 compaction below.
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

	pInventoryPoolList.insert(pInventoryPoolList.end(), extracted.begin(), extracted.end());

	// Step 2: merge/compact every occupied slot, grouped by item type.
	for (size_t i = 0; i < pInventoryPoolList.size(); ++i)
	{
		WORLDITEM& dest_wi = pInventoryPoolList[i];
		if (dest_wi.o.ubNumberOfObjects == 0) continue;

		UINT16 const usItem = dest_wi.o.usItem;

		if (usItem == MONEY)
		{
			// Money doesn't use bStatus[]/ubNumberOfObjects the way every
			// other stackable item does (it's a single uiMoneyAmount), so
			// it can't go through CleanUpStack()/StackObjs() below --
			// combine it the same way PlaceObjectInInventoryStash() already
			// does for a single manual drop.
			for (size_t j = i + 1; j < pInventoryPoolList.size(); ++j)
			{
				WORLDITEM& src_wi = pInventoryPoolList[j];
				if (src_wi.o.usItem != MONEY || src_wi.o.ubNumberOfObjects == 0) continue;

				dest_wi.o.bMoneyStatus = 100;
				dest_wi.o.uiMoneyAmount += src_wi.o.uiMoneyAmount;
				DeleteObj(&src_wi.o);
			}
			continue;
		}

		UINT8 const slot_limit = std::min<UINT8>(GCM->getItem(usItem)->getPerPocket(), MAX_OBJECTS_PER_SLOT);

		for (size_t j = i + 1; j < pInventoryPoolList.size(); ++j)
		{
			WORLDITEM& src_wi = pInventoryPoolList[j];
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

	// Step 3: drop now-empty slots, then re-pad to a whole number of pages
	// -- same convention as BuildStashForSelectedSector().
	std::vector<WORLDITEM> compacted;
	compacted.reserve(pInventoryPoolList.size());
	for (WORLDITEM const& wi : pInventoryPoolList)
	{
		// See the occupancy comment at the top of this function -- fExists
		// is not reliable here, ubNumberOfObjects is.
		if (wi.o.ubNumberOfObjects > 0) compacted.push_back(wi);
	}

	size_t const visible_slots = compacted.size();
	size_t const empty_slots   = MAP_INVENTORY_POOL_SLOT_COUNT - visible_slots % MAP_INVENTORY_POOL_SLOT_COUNT;
	compacted.resize(visible_slots + empty_slots, WORLDITEM{});

	pInventoryPoolList        = std::move(compacted);
	iLastInventoryPoolPage    = static_cast<INT32>((pInventoryPoolList.size() - 1) / MAP_INVENTORY_POOL_SLOT_COUNT);
	iCurrentInventoryPoolPage = 0;

	CheckGridNoOfItemsInMapScreenMapInventory();
	SortSectorInventory(pInventoryPoolList.data(), visible_slots);

	fMapPanelDirty = TRUE;
}


static void DisplayCurrentSector(void)
{
	// grab current sector being displayed
	SetFontAttributes(COMPFONT, 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(MAP_SCREEN_X, MAP_SCREEN_Y,
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
		pInventoryPoolList.insert(pInventoryPoolList.end(), MAP_INVENTORY_POOL_SLOT_COUNT, WORLDITEM{});
	}

	iLastInventoryPoolPage = ( ( static_cast<INT32>(pInventoryPoolList.size()) - 1 ) / MAP_INVENTORY_POOL_SLOT_COUNT );
}


static void DrawTextOnSectorInventory(void);


static void DrawTextOnMapInventoryBackground(void)
{
	UINT16 usStringHeight;

	SetFontDestBuffer(guiSAVEBUFFER);

	int xPos = MAP_SCREEN_X + 651;
	int yPos = MAP_SCREEN_Y + 635;

	//Calculate the height of the string, as it needs to be vertically centered.
	usStringHeight = DisplayWrappedString(xPos, yPos, 53, 1, MAP_IVEN_FONT, FONT_BEIGE, pMapInventoryStrings[0], FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
	DisplayWrappedString(xPos, yPos - (usStringHeight / 2), 53, 1, MAP_IVEN_FONT, FONT_BEIGE, pMapInventoryStrings[0], FONT_BLACK, RIGHT_JUSTIFIED);

	xPos = MAP_SCREEN_X + 732;

	//Calculate the height of the string, as it needs to be vertically centered.
	usStringHeight = DisplayWrappedString(xPos, yPos, 65, 1, MAP_IVEN_FONT, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
	DisplayWrappedString( xPos, yPos - (usStringHeight / 2), 65, 1, MAP_IVEN_FONT, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED);

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

	// first page, can't go back any
	EnableButton(guiMapInvenButton[1], !fStackSplitOpen && iCurrentInventoryPoolPage != 0);
	// last page, go no further
	EnableButton(guiMapInvenButton[0], !fStackSplitOpen && iCurrentInventoryPoolPage != iLastInventoryPoolPage);
	// item picked up ..disable button
	EnableButton(guiMapInvenButton[2], !fMapInventoryItem);
	// "Group Items" -- disabled while the stack split view is open
	EnableButton(guiMapInvenButton[3], !fStackSplitOpen);

	// Stack split view's own Done button -- disabled while holding an item
	// on the cursor (picked up from here via StackSplitSlotPrimary(), or
	// from anywhere else on the map screen), same convention as the main
	// panel's own Done button above.
	if (fStackSplitOpen) EnableButton(gStackSplitDoneButton, !fMapInventoryItem);
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
				if( HandleCompatibleAmmoUIForMapScreen( pSoldier, iCurrentSlot + ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT ), TRUE, FALSE ) )
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
			if( HandleCompatibleAmmoUIForMapInventory( pSoldier, iCurrentSlot, ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT ) , TRUE, FALSE ) )
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
