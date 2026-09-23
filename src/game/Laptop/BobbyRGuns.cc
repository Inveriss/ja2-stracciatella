#include "Directories.h"
#include "Font.h"
#include "HImage.h"
#include "Laptop.h"
#include "BobbyRGuns.h"
#include "BobbyR.h"
#include "MessageBoxScreen.h"
#include "VObject.h"
#include "WordWrap.h"
#include "Cursors.h"
#include "Interface_Items.h"
#include "Text.h"
#include "Store_Inventory.h"
#include "LaptopSave.h"
#include "Finances.h"
#include "Overhead.h"
#include "Button_System.h"
#include "Video.h"
#include "VSurface.h"
#include "ScreenIDs.h"
#include "Font_Control.h"

#include "CalibreModel.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "ArmourModel.h"
#include "Weapons.h"
#include "MagazineModel.h"
#include "WeaponModels.h"

#include "ContentManager.h"
#include "GameInstance.h"
#include "policy/GamePolicy.h"

#include <string_theory/format>
#include <string_theory/string>

#include <algorithm>
#include <iterator>

#define BOBBYR_GRID_PIC_WIDTH		118
#define BOBBYR_GRID_PIC_HEIGHT		69

// Restock-notification checkbox (BOBBY_NOTIFY.STI), top-right corner of an out-of-stock
// item's own picture -- placeholder size, tune once visible in-game.
#define BOBBYR_NOTIFY_WIDTH		20

#define BOBBYR_GRID_PIC_X		BOBBYR_GRIDLOC_X + 3
#define BOBBYR_GRID_PIC_Y		BOBBYR_GRIDLOC_Y + 3

#define BOBBYR_GRID_OFFSET		72

#define BOBBYR_ORDER_TITLE_FONT		FONT14ARIAL
#define BOBBYR_ORDER_TEXT_FONT		FONT10ARIAL
#define BOBBYR_ORDER_TEXT_COLOR		75

#define BOBBYR_STATIC_TEXT_COLOR	75
#define   BOBBYR_ITEM_DESC_TEXT_FONT	FONT10ARIAL
#define   BOBBYR_ITEM_DESC_TEXT_COLOR	FONT_MCOLOR_WHITE
#define   BOBBYR_ITEM_NAME_TEXT_FONT	FONT10ARIAL
#define   BOBBYR_ITEM_NAME_TEXT_COLOR	FONT_MCOLOR_WHITE

#define NUM_BOBBYRPAGE_MENU		6
#define NUM_CATALOGUE_BUTTONS		5
#define MAX_FILTER_BUTTONS		8
// the second row of the buttons is this far under the first one
#define BOBBYR_CATALOGUE_ROW_GAP	13
// the order form button of the guns page is moved this far to the right
#define BOBBYR_GUNS_ORDER_FORM_SHIFT	50
#define BOBBYR_NUM_WEAPONS_ON_PAGE	4

#define BOBBYR_BRTITLE_X		LAPTOP_SCREEN_UL_X + 4
#define BOBBYR_BRTITLE_Y		LAPTOP_SCREEN_WEB_UL_Y + 3
#define BOBBYR_BRTITLE_WIDTH		46
#define BOBBYR_BRTITLE_HEIGHT		42

#define BOBBYR_TO_ORDER_TITLE_X		(STD_SCREEN_X + 195)
#define BOBBYR_TO_ORDER_TITLE_Y		(STD_SCREEN_Y + 42 + LAPTOP_SCREEN_WEB_DELTA_Y - 9)

#define BOBBYR_TO_ORDER_TEXT_X		BOBBYR_TO_ORDER_TITLE_X + 75
#define BOBBYR_TO_ORDER_TEXT_Y		(STD_SCREEN_Y + 33 + LAPTOP_SCREEN_WEB_DELTA_Y + 2)
#define BOBBYR_TO_ORDER_TEXT_WIDTH	330

#define BOBBYR_PREVIOUS_BUTTON_X	LAPTOP_SCREEN_UL_X + 5	//BOBBYR_HOME_BUTTON_X + BOBBYR_CATALOGUE_BUTTON_WIDTH + 5
#define BOBBYR_PREVIOUS_BUTTON_Y	LAPTOP_SCREEN_WEB_UL_Y + 340	//BOBBYR_HOME_BUTTON_Y

#define BOBBYR_NEXT_BUTTON_X		LAPTOP_SCREEN_UL_X + 412	//BOBBYR_ORDER_FORM_X + BOBBYR_ORDER_FORM_WIDTH + 5
#define BOBBYR_NEXT_BUTTON_Y		BOBBYR_PREVIOUS_BUTTON_Y	//BOBBYR_PREVIOUS_BUTTON_Y

// The five buttons of the classes of the guns, 7 pixels between them, in the middle between Previous and Next
#define BOBBYR_CATALOGUE_BUTTON_START_X	(LAPTOP_SCREEN_UL_X + 100)
#define BOBBYR_CATALOGUE_BUTTON_GAP	(BOBBYR_CATALOGUE_BUTTON_WIDTH + 7)
#define BOBBYR_CATALOGUE_BUTTON_Y	LAPTOP_SCREEN_WEB_UL_Y + 340
#define BOBBYR_CATALOGUE_BUTTON_WIDTH	56//75

#define   BOBBYR_HOME_BUTTON_X		(STD_SCREEN_X + 120)
#define   BOBBYR_HOME_BUTTON_Y		(STD_SCREEN_Y + 400 + LAPTOP_SCREEN_WEB_DELTA_Y)

// Catalogue shortcuts row (GUNS/ATTACH/AMMO/ARMOR/EXPL./MISC.), at the very top of
// the page, above the BR logo/"To Order" header -- placeholder position, tune once
// visible in-game.
#define BOBBYR_CATALOG_SHORTCUT_START_X	(BOBBYR_BRTITLE_X + 76)
#define BOBBYR_CATALOG_SHORTCUT_Y		(LAPTOP_SCREEN_WEB_UL_Y + 1 + 20)
#define BOBBYR_CATALOG_SHORTCUT_GAP		66

#define BOBBYR_CATALOGUE_BUTTON_TEXT_Y	BOBBYR_CATALOGUE_BUTTON_Y + 5

#define BOBBYR_ITEM_DESC_START_X	BOBBYR_GRIDLOC_X + 172 + 5
#define BOBBYR_ITEM_DESC_START_Y	BOBBYR_GRIDLOC_Y + 6
#define BOBBYR_ITEM_DESC_START_WIDTH	214 - 10 + 20

#define BOBBYR_ITEM_NAME_X		BOBBYR_GRIDLOC_X + 6
#define BOBBYR_ITEM_NAME_Y_OFFSET	54

#define BOBBYR_ORDER_NUM_WIDTH		15
#define BOBBYR_ORDER_NUM_X		BOBBYR_GRIDLOC_X + 120 - BOBBYR_ORDER_NUM_WIDTH	//BOBBYR_ITEM_STOCK_TEXT_X
#define BOBBYR_ORDER_NUM_Y_OFFSET	1

#define BOBBYR_ITEM_WEIGHT_TEXT_X	BOBBYR_GRIDLOC_X + 409 + 3
#define BOBBYR_ITEM_WEIGHT_TEXT_Y	3

#define BOBBYR_ITEM_WEIGHT_NUM_X	BOBBYR_GRIDLOC_X + 429 - 2
#define BOBBYR_ITEM_WEIGHT_NUM_Y	3
#define BOBBYR_ITEM_WEIGHT_NUM_WIDTH	60

#define BOBBYR_ITEM_SPEC_GAP		2

#define BOBBYR_ITEM_COST_TEXT_X		BOBBYR_GRIDLOC_X + 125
#define BOBBYR_ITEM_COST_TEXT_Y		BOBBYR_GRIDLOC_Y + 6
#define BOBBYR_ITEM_COST_TEXT_WIDTH	42

#define BOBBYR_ITEM_COST_NUM_X		BOBBYR_ITEM_COST_TEXT_X
#define BOBBYR_ITEM_COST_NUM_Y		BOBBYR_ITEM_COST_TEXT_Y + 10

#define BOBBYR_ITEM_STOCK_TEXT_X	BOBBYR_ITEM_COST_TEXT_X

#define BOBBYR_ITEM_QTY_TEXT_X		BOBBYR_GRIDLOC_X + 5//BOBBYR_ITEM_COST_TEXT_X
#define BOBBYR_ITEM_QTY_TEXT_Y		BOBBYR_ITEM_COST_TEXT_Y + 28
#define BOBBYR_ITEM_QTY_WIDTH		95

#define BOBBYR_ITEM_QTY_NUM_X		BOBBYR_GRIDLOC_X + 105//BOBBYR_ITEM_COST_TEXT_X + 1
#define BOBBYR_ITEM_QTY_NUM_Y		BOBBYR_ITEM_QTY_TEXT_Y//BOBBYR_ITEM_COST_TEXT_Y + 40

#define BOBBYR_ITEMS_BOUGHT_X		BOBBYR_GRIDLOC_X + 105 - BOBBYR_ORDER_NUM_WIDTH//BOBBYR_ITEM_QTY_NUM_X

#define BOBBY_RAY_NOT_PURCHASED		255
#define BOBBY_RAY_MAX_AMOUNT_OF_ITEMS_TO_PURCHASE	200

#define BOBBYR_ORDER_FORM_X		LAPTOP_SCREEN_UL_X + 200//204
#define BOBBYR_ORDER_FORM_Y		LAPTOP_SCREEN_WEB_UL_Y + 367
#define BOBBYR_ORDER_FORM_WIDTH		95

#define BOBBYR_ORDER_SUBTOTAL_X		STD_SCREEN_X + 490
#define BOBBYR_ORDER_SUBTOTAL_Y		BOBBYR_ORDER_FORM_Y+2//BOBBYR_HOME_BUTTON_Y

#define BOBBYR_PERCENT_FUNTCIONAL_X	BOBBYR_ORDER_SUBTOTAL_X
#define BOBBYR_PERCENT_FUNTCIONAL_Y	BOBBYR_ORDER_SUBTOTAL_Y + 15


BobbyRayPurchaseStruct BobbyRayPurchases[ MAX_PURCHASE_AMOUNT ];


extern	BOOLEAN fExitingLaptopFlag;

static SGPVObject* guiGunBackground;
static SGPVObject* guiGunsGrid;
static SGPVObject* guiBrTitle;

UINT16 gusCurWeaponIndex;
static UINT8 gubCurPage;
// The buttons of the classes of the guns at the bottom of the guns page: none pressed shows all the guns,
// pressing one shows its class only (the button is released with another click)
// The attachments page has the same kind of buttons (Front, Top, Rear, Down); the last two are disabled.
struct BobbyRFilterBar
{
	int                  count;    // the buttons
	int                  enabled;  // the first ones that can be pressed
	char const* const*   names;
	UINT32 const*        masks;
	UINT8*               filter;   // 0: none pressed (all), 1-count: the button
	UINT32               allMask;
	int                  bottom;   // the first buttons that are in the second row (under the first ones of the first row)
	int                  xOffset = 0; // shifts the whole row/columns left (negative) or right
};

static UINT8 gubGunFilter = 0;
static char const* const gGunFilterNames[] = { "Knives", "Pistol", "SMG", "Assault", "Sniper", "Shotgun", "Heavy" };
static UINT32 const gGunFilterMasks[] =
{
	BOBBYR_GUNS_KNIVES_ITEMS, BOBBYR_GUNS_PISTOL_ITEMS, BOBBYR_GUNS_SMG_ITEMS, BOBBYR_GUNS_ASSAULT_ITEMS,
	BOBBYR_GUNS_SNIPER_ITEMS, BOBBYR_GUNS_SHOTGUN_ITEMS, BOBBYR_GUNS_HEAVY_ITEMS
};
static BobbyRFilterBar const gGunFilterBar = { 7, 7, gGunFilterNames, gGunFilterMasks, &gubGunFilter, BOBBYR_ALL_GUN_ITEMS, 2 };

// Default 1 ("ALL"), same reasoning as gubMiscFilter above.
static UINT8 gubAttachmentFilter = 1;
static char const* const gAttachmentFilterNames[] = { "ALL", "Front", "Top", "Rear", "Down" };
static UINT32 const gAttachmentFilterMasks[] =
{
	BOBBYR_ATTACHMENT_ITEMS, BOBBYR_ATTACH_FRONT_ITEMS, BOBBYR_ATTACH_TOP_ITEMS, BOBBYR_ATTACH_REAR_ITEMS, BOBBYR_ATTACH_DOWN_ITEMS
};
static BobbyRFilterBar const gAttachmentFilterBar = { 5, 5, gAttachmentFilterNames, gAttachmentFilterMasks, &gubAttachmentFilter, BOBBYR_ATTACHMENT_ITEMS, 0, -2 };

// Default 1 ("ALL"), same reasoning as gubMiscFilter above.
static UINT8 gubAmmoFilter = 1;
// "ALL" first -- with bottom=1 below, lands it 13px under "Up to 15" (index 1, first row),
// same row-wrapping mechanism as gExplosivesFilterBar.
static char const* const gAmmoFilterNames[] = { "ALL", "Up to 15", "Up to 30", "Up to 50", "Up to 100", "Up to 250" };
static UINT32 const gAmmoFilterMasks[] =
{
	IC_AMMO, BOBBYR_AMMO_UP_TO_15_ITEMS, BOBBYR_AMMO_UP_TO_30_ITEMS, BOBBYR_AMMO_UP_TO_50_ITEMS,
	BOBBYR_AMMO_UP_TO_100_ITEMS, BOBBYR_AMMO_UP_TO_250_ITEMS
};
static BobbyRFilterBar const gAmmoFilterBar = { 6, 6, gAmmoFilterNames, gAmmoFilterMasks, &gubAmmoFilter, IC_AMMO, 1, -2 };

// Default 1 ("ALL"), same reasoning as gubMiscFilter above.
static UINT8 gubArmourFilter = 1;
static char const* const gArmourFilterNames[] = { "ALL", "Head", "Vest", "Legs", "HeadGear" };
static UINT32 const gArmourFilterMasks[] =
{
	BOBBYR_ARMOUR_ITEMS, BOBBYR_ARMOUR_HEAD_ITEMS, BOBBYR_ARMOUR_VEST_ITEMS, BOBBYR_ARMOUR_LEGS_ITEMS, BOBBYR_ARMOUR_HEADGEAR_ITEMS
};
static BobbyRFilterBar const gArmourFilterBar = { 5, 5, gArmourFilterNames, gArmourFilterMasks, &gubArmourFilter, BOBBYR_ARMOUR_ITEMS, 0, -2 };

// Default 1 ("ALL"), same reasoning as gubMiscFilter above.
static UINT8 gubExplosivesFilter = 1;
// "ALL" first -- with bottom=1 below, array index 0 is the one placed in the second row (see
// the positioning loop in InitBobbyMenuBar()), landing it 13px under "Flares" (index 1, first
// row) since both end up sharing the same x0 anchor for this exact count/bottom combination.
static char const* const gExplosivesFilterNames[] = { "ALL", "Flares", "Gas", "Grenades", "40mm", "Heavy" };
static UINT32 const gExplosivesFilterMasks[] =
{
	BOBBYR_EXPLOSIVES_ALL_ITEMS, BOBBYR_EXPL_FLARES_ITEMS, BOBBYR_EXPL_GAS_ITEMS, BOBBYR_EXPL_GRENADES_ITEMS,
	BOBBYR_EXPL_40MM_ITEMS, BOBBYR_EXPL_HEAVY_ITEMS
};
static BobbyRFilterBar const gExplosivesFilterBar = { 6, 6, gExplosivesFilterNames, gExplosivesFilterMasks, &gubExplosivesFilter, BOBBYR_EXPLOSIVES_ALL_ITEMS, 1, -2 };

// Default 1 ("ALL"), not the usual 0: unlike every other filter bar (where 0 means "no
// button pressed, show allMask" implicitly), Misc has ALL as a real, explicitly selectable
// button -- pressed by default on first entry, same as any other filter choice.
static UINT8 gubMiscFilter = 1;
static char const* const gMiscFilterNames[] = { "ALL", "Medkits", "Tools", "Containers", "Others" };
static UINT32 const gMiscFilterMasks[] =
{
	BOBBYR_MISC_ITEMS, BOBBYR_MISC_MEDKITS_ITEMS, BOBBYR_MISC_TOOLS_ITEMS, BOBBYR_MISC_CONTAINERS_ITEMS, BOBBYR_MISC_OTHERS_ITEMS
};
static BobbyRFilterBar const gMiscFilterBar = { 5, 5, gMiscFilterNames, gMiscFilterMasks, &gubMiscFilter, BOBBYR_MISC_ITEMS, 0, -2 };

// The buttons at the bottom of the current page, if it has them
static BobbyRFilterBar const* gpFilterBar = nullptr;

static BobbyRFilterBar const* FilterBarOfPage(LaptopMode const mode)
{
	switch (mode)
	{
		case LAPTOP_MODE_BOBBY_R_GUNS:        return &gGunFilterBar;
		case LAPTOP_MODE_BOBBY_R_ATTACHMENTS: return &gAttachmentFilterBar;
		case LAPTOP_MODE_BOBBY_R_AMMO:        return &gAmmoFilterBar;
		case LAPTOP_MODE_BOBBY_R_ARMOR:        return &gArmourFilterBar;
		case LAPTOP_MODE_BOBBY_R_EXPLOSIVES:   return &gExplosivesFilterBar;
		case LAPTOP_MODE_BOBBY_R_MISC:         return &gMiscFilterBar;
		default:                              return nullptr;
	}
}

static UINT32 FilterBarMask(BobbyRFilterBar const& bar)
{
	return *bar.filter == 0 ? bar.allMask : bar.masks[*bar.filter - 1];
}

static UINT32 GunsPageMask()
{
	return FilterBarMask(gGunFilterBar);
}

UINT32 BobbyRAttachmentsPageMask()
{
	return FilterBarMask(gAttachmentFilterBar);
}

UINT32 BobbyRAmmoPageMask()
{
	return FilterBarMask(gAmmoFilterBar);
}

UINT32 BobbyRArmourPageMask()
{
	return FilterBarMask(gArmourFilterBar);
}

UINT32 BobbyRExplosivesPageMask()
{
	return FilterBarMask(gExplosivesFilterBar);
}

UINT32 BobbyRMiscPageMask()
{
	return FilterBarMask(gMiscFilterBar);
}

static UINT16 gusLastItemIndex  = 0;
static UINT16 gusFirstItemIndex = 0;
static UINT8  gubNumItemsOnScreen;
static UINT8  gubNumPages;

static BOOLEAN gfBigImageMouseRegionCreated;
static UINT16  gusItemNumberForItemsOnScreen[ BOBBYR_NUM_WEAPONS_ON_PAGE ];


static BOOLEAN gfOnUsedPage;

static UINT16 gusOldItemNumOnTopOfPage = 65535;

//The buttons of the classes of the guns at the bottom of the guns page
//The menu bar at the bottom that changes to different pages
static BUTTON_PICS* guiBobbyRPageMenuImage;
static GUIButtonRef guiBobbyRPageMenu[MAX_FILTER_BUTTONS];
static void BtnBobbyRPageMenuCallback(GUI_BUTTON* btn, UINT32 reason);
// The button of the current class of the guns is pressed
static void SyncGunFilterButtons()
{
	if (!gpFilterBar) return;
	for (int i = 0; i < gpFilterBar->count; ++i)
	{
		if (!guiBobbyRPageMenu[i]) continue;
		if (*gpFilterBar->filter == i + 1) guiBobbyRPageMenu[i]->uiFlags |= BUTTON_CLICKED_ON;
		else                        guiBobbyRPageMenu[i]->uiFlags &= ~BUTTON_CLICKED_ON;
	}
}

//The next and previous buttons
static BUTTON_PICS* guiBobbyRPreviousPageImage;
static GUIButtonRef guiBobbyRPreviousPage;

static BUTTON_PICS* guiBobbyRNextPageImage;
static GUIButtonRef guiBobbyRNextPage;


static MOUSE_REGION g_scroll_region;

// Big Image Mouse region
static MOUSE_REGION gSelectedBigImageRegion[BOBBYR_NUM_WEAPONS_ON_PAGE];

// The order form button
static void BtnBobbyROrderFormCallback(GUI_BUTTON* btn, UINT32 reason);
static BUTTON_PICS* guiBobbyROrderFormImage;
static GUIButtonRef guiBobbyROrderForm;

// The Home button
static void BtnBobbyRHomeButtonCallback(GUI_BUTTON* btn, UINT32 reason);
static BUTTON_PICS* guiBobbyRHomeImage;
static GUIButtonRef guiBobbyRHome;

// The 6 catalogue shortcuts (GUNS/ATTACH/AMMO/ARMOR/EXPL./MISC.) at the top of every
// Bobby Ray's catalogue page -- jump straight to another catalogue without going back
// through Home, same one-line navigation as SelectTitleImageLinkRegionCallBack() below.
enum
{
	BOBBYR_CATALOG_SHORTCUT_GUNS = 0,
	BOBBYR_CATALOG_SHORTCUT_ATTACH,
	BOBBYR_CATALOG_SHORTCUT_AMMO,
	BOBBYR_CATALOG_SHORTCUT_ARMOR,
	BOBBYR_CATALOG_SHORTCUT_EXPL,
	BOBBYR_CATALOG_SHORTCUT_MISC,
	NUM_BOBBYR_CATALOG_SHORTCUTS,
};
static BUTTON_PICS* guiBobbyRCatalogShortcutsImage;
static GUIButtonRef guiBobbyRCatalogShortcuts[NUM_BOBBYR_CATALOG_SHORTCUTS];
static void BtnBobbyRCatalogShortcutCallback(GUI_BUTTON* btn, UINT32 reason);

// Restock-notification checkbox, one per out-of-stock row currently on screen (up to
// BOBBYR_NUM_WEAPONS_ON_PAGE), New page only (the Used page/filter is deactivated -- see
// BOBBYR_USED_ITEMS's callers). The image is shared/loaded once per page entry
// (InitBobbyMenuBar(), like guiBobbyRCatalogShortcutsImage above); the buttons themselves
// are recreated whenever the set of items on screen changes (DisplayItemInfo() below),
// same lifecycle as gSelectedBigImageRegion/CreateMouseRegionForBigImage().
static BUTTON_PICS* guiBobbyRNotifyImage;
static GUIButtonRef guiBobbyRNotifyButtons[BOBBYR_NUM_WEAPONS_ON_PAGE];
static void BtnBobbyRNotifyCallback(GUI_BUTTON* btn, UINT32 reason);
static void DeleteBobbyRNotifyButtons(void);


// Link from the title
static MOUSE_REGION gSelectedTitleImageLinkRegion;


void GameInitBobbyRGuns()
{
	std::fill_n(BobbyRayPurchases, MAX_PURCHASE_AMOUNT, BobbyRayPurchaseStruct{});
	gubGunFilter = 0;
	gubAttachmentFilter = 0;
	gubAmmoFilter = 0;
	gubArmourFilter = 0;
	gubExplosivesFilter = 0;
	gubMiscFilter = 0;
}


void EnterBobbyRGuns()
{
	gfBigImageMouseRegionCreated = FALSE;

	// load the background graphic and add it
	guiGunBackground = AddVideoObjectFromFile(LAPTOPDIR "/gunbackground.sti");

	// load the gunsgrid graphic and add it
	guiGunsGrid = AddVideoObjectFromFile(LAPTOPDIR "/gunsgrid.sti");

	InitBobbyBrTitle();


	SetFirstLastPagesForNew( GunsPageMask() );
	//Draw menu bar
	InitBobbyMenuBar();

	// render once
	RenderBobbyRGuns( );

	//RenderBobbyRGuns();
}


void ExitBobbyRGuns()
{
	DeleteVideoObject(guiGunBackground);
	DeleteVideoObject(guiGunsGrid);
	DeleteBobbyBrTitle();
	DeleteBobbyMenuBar();

	DeleteMouseRegionForBigImage();

	giCurrentSubPage = gusCurWeaponIndex;
	guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_GUNS;
}


void RenderBobbyRGuns()
{
	WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES, BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT, guiGunBackground);

	//Display title at top of page
	DisplayBobbyRBrTitle();

	BltVideoObject(FRAME_BUFFER, guiGunsGrid, 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

	//DeleteMouseRegionForBigImage();
	DisplayItemInfo( GunsPageMask() );
	UpdateButtonText(guiCurrentLaptopMode);
	MarkButtonsDirty( );
	RenderWWWProgramTitleBar( );
	InvalidateScreen();
}


void DisplayBobbyRBrTitle()
{
	BltVideoObject(FRAME_BUFFER, guiBrTitle, 0, BOBBYR_BRTITLE_X, BOBBYR_BRTITLE_Y);

	// To Order Text
	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_TO_ORDER], BOBBYR_TO_ORDER_TITLE_X, BOBBYR_TO_ORDER_TITLE_Y, 0, BOBBYR_ORDER_TITLE_FONT, BOBBYR_ORDER_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	//First put a shadow behind the image
	FRAME_BUFFER->ShadowRect(BOBBYR_TO_ORDER_TEXT_X - 2, BOBBYR_TO_ORDER_TEXT_Y - 2, BOBBYR_TO_ORDER_TEXT_X + BOBBYR_TO_ORDER_TEXT_WIDTH, BOBBYR_TO_ORDER_TEXT_Y + 31);

	//To Order text
	DisplayWrappedString(BOBBYR_TO_ORDER_TEXT_X, BOBBYR_TO_ORDER_TEXT_Y, BOBBYR_TO_ORDER_TEXT_WIDTH, 2, BOBBYR_ORDER_TEXT_FONT, BOBBYR_ORDER_TEXT_COLOR, BobbyRText[BOBBYR_GUNS_CLICK_ON_ITEMS], FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
}


static void SelectTitleImageLinkRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);


void InitBobbyBrTitle()
{
	// load the br title graphic and add it
	guiBrTitle = AddVideoObjectFromFile(LAPTOPDIR "/br.sti");

	//initialize the link to the homepage by clicking on the title
	MSYS_DefineRegion(&gSelectedTitleImageLinkRegion, BOBBYR_BRTITLE_X, BOBBYR_BRTITLE_Y,
				(BOBBYR_BRTITLE_X + BOBBYR_BRTITLE_WIDTH),
				(UINT16)(BOBBYR_BRTITLE_Y + BOBBYR_BRTITLE_HEIGHT),
				MSYS_PRIORITY_HIGH,
				CURSOR_WWW, MSYS_NO_CALLBACK, SelectTitleImageLinkRegionCallBack);

	gusOldItemNumOnTopOfPage=65535;
}


void DeleteBobbyBrTitle()
{
	DeleteVideoObject(guiBrTitle);
	MSYS_RemoveRegion(&gSelectedTitleImageLinkRegion);
	DeleteMouseRegionForBigImage();
}


static void SelectTitleImageLinkRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_BOBBY_R;
	}
}


static void BtnBobbyRCatalogShortcutCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	static LaptopMode const modes[NUM_BOBBYR_CATALOG_SHORTCUTS] =
	{
		LAPTOP_MODE_BOBBY_R_GUNS, LAPTOP_MODE_BOBBY_R_ATTACHMENTS, LAPTOP_MODE_BOBBY_R_AMMO,
		LAPTOP_MODE_BOBBY_R_ARMOR, LAPTOP_MODE_BOBBY_R_EXPLOSIVES, LAPTOP_MODE_BOBBY_R_MISC,
	};
	guiCurrentLaptopMode = modes[btn->GetUserData()];
}


static void DeleteBobbyRNotifyButtons(void)
{
	FOR_EACH(GUIButtonRef, i, guiBobbyRNotifyButtons) { if (*i) RemoveButton(*i); *i = GUIButtonRef(); }
}


static void BtnBobbyRNotifyCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	UINT16 const slot = (UINT16)btn->GetUserData();
	STORE_INVENTORY& inv = LaptopSaveInfo.BobbyRayInventory[slot];
	// This guard is what actually locks the checkbox -- not DisableButton(): a disabled
	// GUI_BUTTON always draws its OffNormal (frame 0) picture regardless of
	// BUTTON_CLICKED_ON (see DrawQuickButton(), Button_System.cc), so disabling it after
	// checking would show the unchecked picture again instead of staying checked.
	if (inv.fNotifyOnRestock) return;

	inv.fNotifyOnRestock = TRUE;
	btn->uiFlags |= BUTTON_CLICKED_ON; // show the checked (frame 1) picture, staying that way
}


static GUIButtonRef MakeButton(BUTTON_PICS* img, const ST::string& text, INT16 x, INT16 y, GUI_CALLBACK click, INT16 priority = MSYS_PRIORITY_HIGH, SGPFont font = BOBBYR_GUNS_BUTTON_FONT)
{
	const INT16 shadow_col = BOBBYR_GUNS_SHADOW_COLOR;
	GUIButtonRef const btn = CreateIconAndTextButton(img, text, font, BOBBYR_GUNS_TEXT_COLOR_ON, shadow_col, BOBBYR_GUNS_TEXT_COLOR_OFF, shadow_col, x, y, priority, click);
	btn->SetCursor(CURSOR_LAPTOP_SCREEN);
	return btn;
}


static void BtnBobbyRNextPageCallback(GUI_BUTTON*, UINT32 reason);
static void BtnBobbyRPreviousPageCallback(GUI_BUTTON*, UINT32 reason);


void InitBobbyMenuBar()
{
	// Previous button
	guiBobbyRPreviousPageImage = LoadButtonImage(LAPTOPDIR "/previousbutton.sti", 0, 1);
	guiBobbyRPreviousPage      = MakeButton(guiBobbyRPreviousPageImage, BobbyRText[BOBBYR_GUNS_PREVIOUS_ITEMS], BOBBYR_PREVIOUS_BUTTON_X, BOBBYR_PREVIOUS_BUTTON_Y, BtnBobbyRPreviousPageCallback);
	guiBobbyRPreviousPage->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_SHADED);

	// Next button
	guiBobbyRNextPageImage = LoadButtonImage(LAPTOPDIR "/nextbutton.sti", 0, 1);
	guiBobbyRNextPage      = MakeButton(guiBobbyRNextPageImage, BobbyRText[BOBBYR_GUNS_MORE_ITEMS], BOBBYR_NEXT_BUTTON_X, BOBBYR_NEXT_BUTTON_Y, BtnBobbyRNextPageCallback);
	guiBobbyRNextPage->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_SHADED);

	// The buttons of the classes of the guns, only on the guns page
	gpFilterBar = FilterBarOfPage(guiCurrentLaptopMode);
	if (gpFilterBar)
	{
		BUTTON_PICS* const gfx = LoadButtonImage(LAPTOPDIR "/cataloguebutton1.sti", 0, 1);
		guiBobbyRPageMenuImage = gfx;

		// the buttons of the first row are centred in the room of the five buttons of the page, those of the
		// second row start at the first one, 13 pixels under it
		int const bottom = gpFilterBar->bottom;
		UINT16 const x0 = BOBBYR_CATALOGUE_BUTTON_START_X + gpFilterBar->xOffset;
		UINT16 const x1 = x0 + (NUM_CATALOGUE_BUTTONS - (gpFilterBar->count - bottom)) * BOBBYR_CATALOGUE_BUTTON_GAP / 2;
		UINT16 const y0 = BOBBYR_CATALOGUE_BUTTON_Y;
		// the first row is made first, the height of its buttons is that of the second row
		for (int k = 0; k < gpFilterBar->count; ++k)
		{
			int const i = (k + bottom) % gpFilterBar->count;
			bool const second = i < bottom;
			UINT16 const x = second ? x0 + i * BOBBYR_CATALOGUE_BUTTON_GAP : x1 + (i - bottom) * BOBBYR_CATALOGUE_BUTTON_GAP;
			UINT16 y = y0;
			if (second) y += guiBobbyRPageMenu[bottom]->H() + BOBBYR_CATALOGUE_ROW_GAP;
			GUIButtonRef const b = MakeButton(gfx, gpFilterBar->names[i], x, y, BtnBobbyRPageMenuCallback, MSYS_PRIORITY_HIGH, FONT10ARIALBOLD);
			b->SetUserData(i + 1);
			b->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_SHADED);
			if (i >= gpFilterBar->enabled) DisableButton(b);
			guiBobbyRPageMenu[i] = b;
		}
		SyncGunFilterButtons();
	}

	// Catalogue shortcuts row -- same 5 buttons' graphic as the class filter buttons,
	// always all 6 active regardless of which page is currently shown.
	{
		static char const* const names[NUM_BOBBYR_CATALOG_SHORTCUTS] = { "GUNS", "ATTACH.", "AMMO", "ARMOR", "EXPL.", "MISC." };
		BUTTON_PICS* const gfx = LoadButtonImage(LAPTOPDIR "/cataloguebutton1.sti", 0, 1);
		guiBobbyRCatalogShortcutsImage = gfx;
		UINT16 x = BOBBYR_CATALOG_SHORTCUT_START_X;
		for (int i = 0; i < NUM_BOBBYR_CATALOG_SHORTCUTS; ++i, x += BOBBYR_CATALOG_SHORTCUT_GAP)
		{
			GUIButtonRef const b = MakeButton(gfx, names[i], x, BOBBYR_CATALOG_SHORTCUT_Y, BtnBobbyRCatalogShortcutCallback, MSYS_PRIORITY_HIGH, FONT10ARIALBOLD);
			b->SetUserData(i);
			guiBobbyRCatalogShortcuts[i] = b;
		}
	}

	// Restock-notification checkbox image -- shared by up to BOBBYR_NUM_WEAPONS_ON_PAGE
	// buttons created/destroyed per page-turn inside DisplayItemInfo(), not here.
	guiBobbyRNotifyImage = LoadButtonImage(LAPTOPDIR "/BOBBY_NOTIFY.STI", 0, 1);

	// Order Form button
	guiBobbyROrderFormImage = LoadButtonImage(LAPTOPDIR "/orderformbutton.sti", 0, 1);
	guiBobbyROrderForm      = MakeButton(guiBobbyROrderFormImage, BobbyRText[BOBBYR_GUNS_ORDER_FORM], BOBBYR_ORDER_FORM_X + (gpFilterBar && gpFilterBar->bottom ? BOBBYR_GUNS_ORDER_FORM_SHIFT : 0), BOBBYR_ORDER_FORM_Y, BtnBobbyROrderFormCallback);

	// Home button
	guiBobbyRHomeImage = LoadButtonImage(LAPTOPDIR "/cataloguebutton.sti", 0, 1);
	guiBobbyRHome      = MakeButton(guiBobbyRHomeImage, BobbyRText[BOBBYR_GUNS_HOME], BOBBYR_HOME_BUTTON_X, BOBBYR_HOME_BUTTON_Y, BtnBobbyRHomeButtonCallback);
}


void DeleteBobbyMenuBar()
{
	RemoveButton(guiBobbyRPreviousPage);
	UnloadButtonImage(guiBobbyRPreviousPageImage);

	RemoveButton(guiBobbyRNextPage);
	UnloadButtonImage(guiBobbyRNextPageImage);

	if (gpFilterBar)
	{
		FOR_EACH(GUIButtonRef, i, guiBobbyRPageMenu) { if (*i) RemoveButton(*i); *i = GUIButtonRef(); }
		UnloadButtonImage(guiBobbyRPageMenuImage);
		gpFilterBar = nullptr;
	}

	RemoveButton(guiBobbyROrderForm);
	UnloadButtonImage(guiBobbyROrderFormImage);

	RemoveButton(guiBobbyRHome);
	UnloadButtonImage(guiBobbyRHomeImage);

	FOR_EACH(GUIButtonRef, i, guiBobbyRCatalogShortcuts) { RemoveButton(*i); *i = GUIButtonRef(); }
	UnloadButtonImage(guiBobbyRCatalogShortcutsImage);

	DeleteBobbyRNotifyButtons();
	UnloadButtonImage(guiBobbyRNotifyImage);
}


static void BtnBobbyRPageMenuCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) { SyncGunFilterButtons(); return; }

	// pressing the pressed button releases it: all the guns again
	if (!gpFilterBar) return;
	UINT8 const filter = static_cast<UINT8>(btn->GetUserData());
	*gpFilterBar->filter = filter == *gpFilterBar->filter ? 0 : filter;
	SyncGunFilterButtons();

	SetFirstLastPagesForNew(FilterBarMask(*gpFilterBar));
	DeleteMouseRegionForBigImage();
	gusOldItemNumOnTopOfPage = 65535;
	fReDrawScreenFlag       = TRUE;
	fPausedReDrawScreenFlag = TRUE;
}


static void NextPage()
{
	if (gubCurPage == gubNumPages - 1) return;
	++gubCurPage;
	DeleteMouseRegionForBigImage();
	fReDrawScreenFlag       = TRUE;
	fPausedReDrawScreenFlag = TRUE;
}


static void BtnBobbyRNextPageCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		NextPage();
	}
}


static void PrevPage()
{
	if (gubCurPage == 0) return;
	--gubCurPage;
	DeleteMouseRegionForBigImage();
	fReDrawScreenFlag       = TRUE;
	fPausedReDrawScreenFlag = TRUE;
}


static void BtnBobbyRPreviousPageCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		PrevPage();
	}
}


static void CalcFirstIndexForPage(STORE_INVENTORY* pInv, UINT32 uiItemClass);
static UINT32 CalculateTotalPurchasePrice();
static UINT8 CheckIfItemIsPurchased(UINT16 usItemNumber);
static void CreateMouseRegionForBigImage(UINT16 usPosY, UINT8 ubCount, const ItemModel* const items[]);
static void DisableBobbyRButtons(void);
static void DisplayAmmoInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex);
static void DisplayArmourInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex);
static void DisplayBigItemImage(const ItemModel* item, UINT16 PosY);
static void DisplayGunInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex);
static void DisplayItemNameAndInfo(UINT16 usPosY, UINT16 usIndex, UINT16 usBobbyIndex, BOOLEAN fUsed);
static void DisplayMiscInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex);
static void DisplayNonGunWeaponInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex);


void DisplayItemInfo(UINT32 uiItemClass)
{
	UINT16  i;
	UINT8   ubCount=0;
	UINT16  PosY, usTextPosY;
	UINT16  usItemIndex;
	ST::string sDollarTemp;
	ST::string sTemp;

	PosY = BOBBYR_GRID_PIC_Y;
	usTextPosY = BOBBYR_ITEM_DESC_START_Y;

	//if there are no items then return
	if( gusFirstItemIndex == BOBBYR_NO_ITEMS )
	{
		if (fExitingLaptopFlag) return;
		if (gfShowBookmarks)	return;
		if (fLoadPendingFlag)	return;

		DisableBobbyRButtons();
		// No popup about the empty stock: the page stays empty, the class buttons still work
		return;
	}


	if( uiItemClass == BOBBYR_USED_ITEMS )
		CalcFirstIndexForPage(LaptopSaveInfo.BobbyRayUsedInventory, IC_ALL);
	else
		CalcFirstIndexForPage( LaptopSaveInfo.BobbyRayInventory, uiItemClass );

	DisableBobbyRButtons();

	// Captured once, before CreateMouseRegionForBigImage() below updates gusOldItemNumOnTopOfPage --
	// the notify checkboxes are recreated exactly when the item mouse regions are (the set of items
	// on screen changed), not on every redraw of an unchanged page.
	bool const fItemsOnScreenChanged = gusOldItemNumOnTopOfPage != gusCurWeaponIndex;

	if( fItemsOnScreenChanged )
	{
		DeleteMouseRegionForBigImage();
		DeleteBobbyRNotifyButtons();
	}

	const ItemModel* items[BOBBYR_NUM_WEAPONS_ON_PAGE];
	std::fill(std::begin(items), std::end(items), nullptr);
	for(i=gusCurWeaponIndex; ((i<=gusLastItemIndex) && (ubCount < 4)); i++)
	{
		BOOLEAN fOutOfStock;
		if( uiItemClass == BOBBYR_USED_ITEMS )
		{
			//If the item was never eligible at all, it doesn't belong on the page
			if( !LaptopSaveInfo.BobbyRayUsedInventory[ i ].fPreviouslyEligible )
				continue;

			fOutOfStock = LaptopSaveInfo.BobbyRayUsedInventory[ i ].ubQtyOnHand == 0;
			usItemIndex = LaptopSaveInfo.BobbyRayUsedInventory[ i ].usItemIndex;
			gfOnUsedPage = TRUE;
		}
		else
		{
			//If the item was never eligible at all, it doesn't belong on the page
			if( !LaptopSaveInfo.BobbyRayInventory[ i ].fPreviouslyEligible )
				continue;

			fOutOfStock = LaptopSaveInfo.BobbyRayInventory[ i ].ubQtyOnHand == 0;
			usItemIndex = LaptopSaveInfo.BobbyRayInventory[ i ].usItemIndex;
			gfOnUsedPage = FALSE;
		}

		// New page only: the player's own cart can claim the last units on hand without the
		// stock actually reaching zero -- shown greyed out same as a genuinely empty shelf
		// (fShowAsOutOfStock below), but NOT eligible for the restock-notification checkbox
		// (fOutOfStock, unchanged), since there's nothing left to restock from Bobby Ray's own
		// point of view.
		BOOLEAN fShowAsOutOfStock = fOutOfStock;
		if (!fOutOfStock && uiItemClass != BOBBYR_USED_ITEMS)
		{
			UINT8 const ubPurchaseNumber = CheckIfItemIsPurchased(i);
			if (ubPurchaseNumber != BOBBY_RAY_NOT_PURCHASED &&
				BobbyRayPurchases[ubPurchaseNumber].ubNumberPurchased >= LaptopSaveInfo.BobbyRayInventory[i].ubQtyOnHand)
			{
				fShowAsOutOfStock = TRUE;
			}
		}

		// skip items that aren't of the right item class
		const ItemModel * item = GCM->getItem(usItemIndex);
		if (!BobbyRItemMatchesClass(item, uiItemClass)) continue;

		items[ubCount] = item;

		// this row's top, before the switch below advances PosY/usTextPosY for it
		UINT16 const usRowPosY = PosY;
		UINT8  const ubCountBeforeRow = ubCount;

		switch (item->getItemClass())
		{
			case IC_GUN:
			case IC_LAUNCHER:
				gusItemNumberForItemsOnScreen[ ubCount ] = i;

				DisplayBigItemImage(item, PosY);

				//Display Items Name
				DisplayItemNameAndInfo(usTextPosY, usItemIndex, i, gfOnUsedPage);

				DisplayGunInfo(usItemIndex, usTextPosY, gfOnUsedPage, i);

				PosY += BOBBYR_GRID_OFFSET;
				usTextPosY += BOBBYR_GRID_OFFSET;
				ubCount++;
				break;

			case IC_AMMO:
				gusItemNumberForItemsOnScreen[ ubCount ] = i;

				DisplayBigItemImage(item, PosY);

				//Display Items Name
				DisplayItemNameAndInfo(usTextPosY, usItemIndex, i, gfOnUsedPage);

				DisplayAmmoInfo( usItemIndex, usTextPosY, gfOnUsedPage, i);

				PosY += BOBBYR_GRID_OFFSET;
				usTextPosY += BOBBYR_GRID_OFFSET;
				ubCount++;
				break;

			case IC_ARMOUR:
				gusItemNumberForItemsOnScreen[ ubCount ] = i;

				DisplayBigItemImage(item, PosY);

				//Display Items Name
				DisplayItemNameAndInfo(usTextPosY, usItemIndex, i, gfOnUsedPage);

				DisplayArmourInfo( usItemIndex, usTextPosY, gfOnUsedPage, i);

				PosY += BOBBYR_GRID_OFFSET;
				usTextPosY += BOBBYR_GRID_OFFSET;
				ubCount++;
				break;

			case IC_BLADE:
			case IC_THROWING_KNIFE:
			case IC_PUNCH:
				gusItemNumberForItemsOnScreen[ ubCount ] = i;

				DisplayBigItemImage(item, PosY);

				//Display Items Name
				DisplayItemNameAndInfo(usTextPosY, usItemIndex, i, gfOnUsedPage);

				DisplayNonGunWeaponInfo(usItemIndex, usTextPosY, gfOnUsedPage, i);

				PosY += BOBBYR_GRID_OFFSET;
				usTextPosY += BOBBYR_GRID_OFFSET;
				ubCount++;
				break;

			case IC_GRENADE:
			case IC_BOMB:
			case IC_MISC:
			case IC_MEDKIT:
			case IC_KIT:
			case IC_FACE:
				gusItemNumberForItemsOnScreen[ ubCount ] = i;

				DisplayBigItemImage(item, PosY);

				//Display Items Name
				DisplayItemNameAndInfo(usTextPosY, usItemIndex, i, gfOnUsedPage);

				DisplayMiscInfo( usItemIndex, usTextPosY, gfOnUsedPage, i);

				PosY += BOBBYR_GRID_OFFSET;
				usTextPosY += BOBBYR_GRID_OFFSET;
				ubCount++;
				break;
		}

		// Out of stock at this stage of the game (as opposed to never having been
		// eligible at all -- those never reach this loop, filtered out above): dim
		// the whole row instead of hiding the item entirely, per user request. Also covers
		// fShowAsOutOfStock's "claimed entirely by the player's own cart" case above.
		if (fShowAsOutOfStock && ubCount != ubCountBeforeRow)
		{
			FRAME_BUFFER->ShadowRect(BOBBYR_GRIDLOC_X, usRowPosY - 3, BOBBYR_GRIDLOC_X + 450 + 43, usRowPosY - 3 + BOBBYR_GRID_OFFSET + 1);

			// Same font/colour/shadow as the "On Assign" text on an unavailable AIM
			// merc's portrait (AimFiText[AIM_FI_DEAD + 1], AIMFacialIndex.cc), centred
			// on the item's own picture rather than the whole (wider) dimmed row.
			DrawTextToScreen(BobbyRText[BOBBYR_GUNS_OUT_OF_STOCK], BOBBYR_GRID_PIC_X,
				usRowPosY + (BOBBYR_GRID_PIC_HEIGHT - GetFontHeight(FONT10ARIAL)) / 2,
				BOBBYR_GRID_PIC_WIDTH, FONT10ARIAL, 145, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

			// "Email me when new stock arrives" checkbox, New page only (the Used page is
			// deactivated) -- top-right corner of the item's own picture; recreated only when
			// the items on screen actually changed (see fItemsOnScreenChanged above), not on
			// every redraw of an unchanged page. fOutOfStock (not fShowAsOutOfStock): only
			// for genuinely zero stock on hand -- an item merely claimed entirely by the
			// player's own cart has nothing for Bobby Ray's to actually restock.
			if (fItemsOnScreenChanged && fOutOfStock && uiItemClass != BOBBYR_USED_ITEMS)
			{
				STORE_INVENTORY& inv = LaptopSaveInfo.BobbyRayInventory[i];

				// QuickCreateButtonNoMove(), not MakeButton() (-> QuickCreateButton()): the
				// latter wires up DefaultMoveCallback, which makes the generic button-press
				// handler (QuickButtonCallbackMButn(), Button_System.cc) itself force
				// BUTTON_CLICKED_ON on and off across every press/release -- fine for a normal
				// push button's momentary "pressed" look, but it fights this checkbox's own
				// persistent checked state, which BtnBobbyRNotifyCallback() alone should own.
				// MSYS_PRIORITY_HIGHEST (not MSYS_PRIORITY_HIGH): this button sits inside the
				// item's own picture, which gSelectedBigImageRegion (CreateMouseRegionForBigImage()
				// below) covers too, at that lower priority -- without outranking it, clicks on
				// the checkbox's corner fall through to the big-image region's buy click instead.
				GUIButtonRef const b = QuickCreateButtonNoMove(guiBobbyRNotifyImage,
					BOBBYR_GRID_PIC_X + BOBBYR_GRID_PIC_WIDTH - BOBBYR_NOTIFY_WIDTH, usRowPosY,
					MSYS_PRIORITY_HIGHEST, BtnBobbyRNotifyCallback);
				// The item picture underneath shows CURSOR_WWW (FINGERCURSOR.STI) -- match that
				// instead of the default arrow cursor.
				b->SetCursor(CURSOR_WWW);
				b->SetUserData(i); // slot index into BobbyRayInventory
				b->SetFastHelpText("Email me when new stock arrives.");
				if (inv.fNotifyOnRestock) b->uiFlags |= BUTTON_CLICKED_ON;
				guiBobbyRNotifyButtons[ubCountBeforeRow] = b;
			}

			// Drawn ourselves, every frame, right after the row's own dimming/text above --
			// same reason as the map screen's Stats/Skills Done button (Interface_Panels.cc):
			// RenderButtons()'s own pass only redraws a button when something marks it dirty
			// again (e.g. a hover), so relying on it alone left a just-checked box's checked
			// picture erased by this same per-frame row redraw one frame later.
			if (guiBobbyRNotifyButtons[ubCountBeforeRow]) guiBobbyRNotifyButtons[ubCountBeforeRow]->Draw();
		}
	}

	if( gusOldItemNumOnTopOfPage != gusCurWeaponIndex )
	{
		CreateMouseRegionForBigImage(BOBBYR_GRID_PIC_Y, ubCount, items);
		gusOldItemNumOnTopOfPage = gusCurWeaponIndex;
	}

	//Display the subtotal at the bottom of the screen
	sDollarTemp = SPrintMoney(CalculateTotalPurchasePrice());
	sTemp = ST::format("{} {}", BobbyRText[BOBBYR_GUNS_SUB_TOTAL], sDollarTemp);
	DrawTextToScreen(sTemp, BOBBYR_ORDER_SUBTOTAL_X, BOBBYR_ORDER_SUBTOTAL_Y, 0, BOBBYR_ORDER_TITLE_FONT, BOBBYR_ORDER_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED | TEXT_SHADOWED);

	//Display the Used item disclaimer
	if( gfOnUsedPage )
	{
		DrawTextToScreen(BobbyRText[BOBBYR_GUNS_PERCENT_FUNCTIONAL], BOBBYR_PERCENT_FUNTCIONAL_X, BOBBYR_PERCENT_FUNTCIONAL_Y, 0, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ORDER_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED | TEXT_SHADOWED);
	}
}


static UINT16 DisplayCaliber(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight);
static UINT16 DisplayCostAndQty(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight, UINT16 usBobbyIndex, BOOLEAN fUsed);
static UINT16 DisplayDamage(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight);
static UINT16 DisplayMagazine(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight);
static UINT16 DisplayRange(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight);
static UINT16 DisplayRof(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight);


static void DisplayGunInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex)
{
	UINT16	usHeight;
	UINT16 usFontHeight;
	usFontHeight = GetFontHeight(BOBBYR_ITEM_DESC_TEXT_FONT);

	//Display Items Name
	//DisplayItemNameAndInfo(usTextPosY, usIndex, fUsed);

	usHeight = usTextPosY;
	//Display the weight, caliber, mag, rng, dam, rof text

	//Caliber
	usHeight = DisplayCaliber(usHeight, usIndex, usFontHeight);

	//Magazine
	usHeight = DisplayMagazine(usHeight, usIndex, usFontHeight);

	//Range
	usHeight = DisplayRange(usHeight, usIndex, usFontHeight);

	//Damage
	usHeight = DisplayDamage(usHeight, usIndex, usFontHeight);

	//ROF
	usHeight = DisplayRof(usHeight, usIndex, usFontHeight);

	//Display the Cost and the qty bought and on hand
	usHeight = DisplayCostAndQty(usTextPosY, usIndex, usFontHeight, usBobbyIndex, fUsed);
}


static void DisplayNonGunWeaponInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex)
{
	UINT16	usHeight;
	UINT16 usFontHeight;
	usFontHeight = GetFontHeight(BOBBYR_ITEM_DESC_TEXT_FONT);

	//Display Items Name
	//DisplayItemNameAndInfo(usTextPosY, usIndex, fUsed);

	usHeight = usTextPosY;
	//Display the weight, caliber, mag, rng, dam, rof text

	//Damage
	usHeight = DisplayDamage(usHeight, usIndex, usFontHeight);

	//Display the Cost and the qty bought and on hand
	usHeight = DisplayCostAndQty(usTextPosY, usIndex, usFontHeight, usBobbyIndex, fUsed);
}


static void DisplayAmmoInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex)
{
	UINT16	usHeight;
	UINT16 usFontHeight;
	usFontHeight = GetFontHeight(BOBBYR_ITEM_DESC_TEXT_FONT);

	//Display Items Name
	//DisplayItemNameAndInfo(usTextPosY, usIndex, fUsed);

	usHeight = usTextPosY;
	//Display the weight, caliber, mag, rng, dam, rof text

	//Caliber
	usHeight = DisplayCaliber(usHeight, usIndex, usFontHeight);

	//Magazine
	//usHeight = DisplayMagazine(usHeight, usIndex, usFontHeight);

	//Display the Cost and the qty bought and on hand
	usHeight = DisplayCostAndQty(usTextPosY, usIndex, usFontHeight, usBobbyIndex, fUsed);
}


static void DisplayBigItemImage(const ItemModel* item, const UINT16 PosY)
{
	INT16 PosX = BOBBYR_GRID_PIC_X;

	auto graphic = GetBigInventoryGraphicForItem(item);
	AutoSGPVObject uiImage(graphic.first);
	auto subImageIndex = graphic.second;

	//center picture in frame
	ETRLEObject const& pTrav   = uiImage->SubregionProperties(subImageIndex);
	UINT32      const  usWidth = pTrav.usWidth;
	INT16       const  sCenX   = PosX + std::abs(int(BOBBYR_GRID_PIC_WIDTH - usWidth)) / 2 - pTrav.sOffsetX;
	INT16       const  sCenY   = PosY + 8;

	if (gamepolicy(f_draw_item_shadow))
	{
		//blt the shadow of the item
		BltVideoObjectOutlineShadow(FRAME_BUFFER, uiImage.get(), subImageIndex, sCenX - 2, sCenY + 2);
	}

	BltVideoObject(FRAME_BUFFER, uiImage.get(), subImageIndex, sCenX, sCenY);
}


static void DisplayArmourInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex)
{
	UINT16 usFontHeight = GetFontHeight(BOBBYR_ITEM_DESC_TEXT_FONT);

	//Display the Cost and the qty bought and on hand
	DisplayCostAndQty(usTextPosY, usIndex, usFontHeight, usBobbyIndex, fUsed);
}


static void DisplayMiscInfo(UINT16 usIndex, UINT16 usTextPosY, BOOLEAN fUsed, UINT16 usBobbyIndex)
{
	UINT16 usFontHeight;
	usFontHeight = GetFontHeight(BOBBYR_ITEM_DESC_TEXT_FONT);

	//Display Items Name
	//DisplayItemNameAndInfo(usTextPosY, usIndex, fUsed);

	//Display the Cost and the qty bought and on hand
	DisplayCostAndQty(usTextPosY, usIndex, usFontHeight, usBobbyIndex, fUsed);
}


static UINT8 CheckIfItemIsPurchased(UINT16 usItemNumber);


static UINT16 DisplayCostAndQty(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight, UINT16 usBobbyIndex, BOOLEAN fUsed)
{
	ST::string sTemp;
	//UINT8	ubPurchaseNumber;

	//
	//Display the cost and the qty
	//

	//Display the cost
	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_COST], BOBBYR_ITEM_COST_TEXT_X, usPosY, BOBBYR_ITEM_COST_TEXT_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	usPosY += usFontHeight + 2;

	DrawTextToScreen(SPrintMoney(CalcBobbyRayCost(usIndex, usBobbyIndex, fUsed)), BOBBYR_ITEM_COST_NUM_X, usPosY, BOBBYR_ITEM_COST_TEXT_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
	usPosY += usFontHeight + 2;


	//Display Weight Number
	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_WGHT], BOBBYR_ITEM_STOCK_TEXT_X, usPosY, BOBBYR_ITEM_COST_TEXT_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	usPosY += usFontHeight + 2;


	sTemp = ST::format("{3.2f} {}", GetWeightBasedOnMetricOption(GCM->getItem(usIndex)->getWeight()) / 10.0f, GetWeightUnitString());
	DrawTextToScreen(sTemp, BOBBYR_ITEM_STOCK_TEXT_X, usPosY, BOBBYR_ITEM_COST_TEXT_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
	usPosY += usFontHeight + 2;


	//Display the # In Stock
	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_IN_STOCK], BOBBYR_ITEM_STOCK_TEXT_X, usPosY, BOBBYR_ITEM_COST_TEXT_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	usPosY += usFontHeight + 2;

	if( fUsed )
		sTemp = ST::format("{_ 4d}", LaptopSaveInfo.BobbyRayUsedInventory[ usBobbyIndex ].ubQtyOnHand);
	else
		sTemp = ST::format("{_ 4d}", LaptopSaveInfo.BobbyRayInventory[ usBobbyIndex ].ubQtyOnHand);

	DrawTextToScreen(sTemp, BOBBYR_ITEM_STOCK_TEXT_X, usPosY, BOBBYR_ITEM_COST_TEXT_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
	usPosY += usFontHeight + 2;


	return(usPosY);
}


static UINT16 DisplayRof(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight)
{
	ST::string sTemp;

	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_ROF], BOBBYR_ITEM_WEIGHT_TEXT_X, usPosY, 0, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	sTemp = ST::format("{3d}/{}", GCM->getWeapon(usIndex)->getRateOfFire(), pMessageStrings[ MSG_MINUTE_ABBREVIATION ]);


	DrawTextToScreen(sTemp, BOBBYR_ITEM_WEIGHT_NUM_X, usPosY, BOBBYR_ITEM_WEIGHT_NUM_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
	usPosY += usFontHeight + 2;
	return(usPosY);
}


static UINT16 DisplayDamage(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight)
{
	ST::string sTemp;

	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_DAMAGE], BOBBYR_ITEM_WEIGHT_TEXT_X, usPosY, 0, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	sTemp = ST::format("{4d}", GCM->getWeapon( usIndex )->ubImpact);
	DrawTextToScreen(sTemp, BOBBYR_ITEM_WEIGHT_NUM_X, usPosY, BOBBYR_ITEM_WEIGHT_NUM_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
	usPosY += usFontHeight + 2;
	return(usPosY);
}


static UINT16 DisplayRange(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight)
{
	ST::string sTemp;

	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_RANGE], BOBBYR_ITEM_WEIGHT_TEXT_X, usPosY, 0, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	sTemp = ST::format("{3d} {}", GCM->getWeapon( usIndex )->usRange, pMessageStrings[ MSG_METER_ABBREVIATION ]);
	DrawTextToScreen(sTemp, BOBBYR_ITEM_WEIGHT_NUM_X, usPosY, BOBBYR_ITEM_WEIGHT_NUM_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
	usPosY += usFontHeight + 2;
	return(usPosY);
}


static UINT16 DisplayMagazine(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight)
{
	ST::string sTemp;

	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_MAGAZINE], BOBBYR_ITEM_WEIGHT_TEXT_X, usPosY, 0, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	sTemp = ST::format("{3d} {}", GCM->getWeapon(usIndex)->ubMagSize, pMessageStrings[ MSG_ROUNDS_ABBREVIATION ]);
	DrawTextToScreen(sTemp, BOBBYR_ITEM_WEIGHT_NUM_X, usPosY, BOBBYR_ITEM_WEIGHT_NUM_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
	usPosY += usFontHeight + 2;
	return(usPosY);
}


static UINT16 DisplayCaliber(UINT16 usPosY, UINT16 usIndex, UINT16 usFontHeight)
{
	const ItemModel * item = GCM->getItem(usIndex);
	ST::string zTemp;
	DrawTextToScreen(BobbyRText[BOBBYR_GUNS_CALIBRE], BOBBYR_ITEM_WEIGHT_TEXT_X, usPosY, 0, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	// ammo or gun?
	const CalibreModel *calibre = item->getItemClass() == IC_AMMO ? item->asAmmo()->calibre : item->asWeapon()->calibre;
	zTemp = *GCM->getCalibreNameForBobbyRay(calibre->index);

	zTemp = ReduceStringLength(zTemp, BOBBYR_GRID_PIC_WIDTH, BOBBYR_ITEM_NAME_TEXT_FONT);
	DrawTextToScreen(zTemp, BOBBYR_ITEM_WEIGHT_NUM_X, usPosY, BOBBYR_ITEM_WEIGHT_NUM_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);

	usPosY += usFontHeight + 2;
	return(usPosY);
}


static void DisplayItemNameAndInfo(UINT16 usPosY, UINT16 usIndex, UINT16 usBobbyIndex, BOOLEAN fUsed)
{
	ST::string sTemp;
	UINT32 uiStartLoc;
	UINT8	ubPurchaseNumber;

	{
		//Display Items Name
		uiStartLoc = BOBBYR_ITEM_DESC_FILE_SIZE * usIndex;
		ST::string sText = GCM->loadEncryptedString(BOBBYRDESCFILE, uiStartLoc, BOBBYR_ITEM_DESC_NAME_SIZE);
		sText = ReduceStringLength(sText, BOBBYR_GRID_PIC_WIDTH - 6, BOBBYR_ITEM_NAME_TEXT_FONT);
		DrawTextToScreen(sText, BOBBYR_ITEM_NAME_X, usPosY + BOBBYR_ITEM_NAME_Y_OFFSET, 0, BOBBYR_ITEM_NAME_TEXT_FONT, BOBBYR_ITEM_NAME_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}

	//number bought
	//Display the # bought
	ubPurchaseNumber = CheckIfItemIsPurchased(usBobbyIndex);
	if( ubPurchaseNumber != BOBBY_RAY_NOT_PURCHASED)
	{
		DrawTextToScreen(BobbyRText[BOBBYR_GUNS_QTY_ON_ORDER], BOBBYR_ITEM_QTY_TEXT_X, usPosY, BOBBYR_ITEM_QTY_WIDTH, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);

		if( ubPurchaseNumber != BOBBY_RAY_NOT_PURCHASED)
		{
			sTemp = ST::format("{_ 4d}", BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased);
			DrawTextToScreen(sTemp, BOBBYR_ITEMS_BOUGHT_X, usPosY, 0, FONT14ARIAL, BOBBYR_ITEM_DESC_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		}
	}




	//if it's a used item, display how damaged the item is
	if( fUsed )
	{
		sTemp = ST::format("*{3d}%", LaptopSaveInfo.BobbyRayUsedInventory[usBobbyIndex].ubItemQuality);
		DrawTextToScreen(sTemp, BOBBYR_ITEM_NAME_X - 2, usPosY - BOBBYR_ORDER_NUM_Y_OFFSET, BOBBYR_ORDER_NUM_WIDTH, BOBBYR_ITEM_NAME_TEXT_FONT, BOBBYR_ITEM_NAME_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}

	{
		//Display Items description
		uiStartLoc += BOBBYR_ITEM_DESC_NAME_SIZE;
		ST::string sText = GCM->loadEncryptedString(BOBBYRDESCFILE, uiStartLoc, BOBBYR_ITEM_DESC_INFO_SIZE);
		DisplayWrappedString(BOBBYR_ITEM_DESC_START_X, usPosY, BOBBYR_ITEM_DESC_START_WIDTH, 2, BOBBYR_ITEM_DESC_TEXT_FONT, BOBBYR_ITEM_DESC_TEXT_COLOR, sText, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}
}


//Loops through Bobby Rays Inventory to find the first and last index
// An attachment of a weapon (scopes, silencers, bipods...). Armour (the ceramic plates), face items,
// weapons and explosives are not counted, even when they are attachable, like in the sector
// inventory (GetSectorInventoryFilterCategory()).
static bool IsBobbyRAttachment(const ItemModel* const item)
{
	if (item->isArmour() || item->isFace() || item->isWeapon() || item->isExplosive()) return false;
	if (item->getFlags() & ITEM_ATTACHMENT) return true;
	return item->getItemIndex() == GUN_BARREL_EXTENDER || item->getItemIndex() == SPRING_AND_BOLT_UPGRADE;
}


// Is the item a magazine of the given class of the ammo page (BOBBYR_AMMO_UP_TO_*_ITEMS)? The class is
// the number of rounds of the magazine.
static bool IsInAmmoClass(const ItemModel* const item, UINT32 const cls)
{
	if (item->getItemClass() != IC_AMMO || !item->asAmmo()) return false;
	unsigned const rounds = item->asAmmo()->capacity;
	switch (cls)
	{
		case BOBBYR_AMMO_UP_TO_15_ITEMS:  return rounds <= 15;
		case BOBBYR_AMMO_UP_TO_30_ITEMS:  return rounds >= 16 && rounds <= 30;
		case BOBBYR_AMMO_UP_TO_50_ITEMS:  return rounds >= 31 && rounds <= 50;
		case BOBBYR_AMMO_UP_TO_100_ITEMS: return rounds >= 51 && rounds <= 100;
		default:                          return rounds >= 101 && rounds <= 250;
	}
}


// Is the item on the miscellaneous page at all? The crowbar (a punch weapon) is a tool there, the
// other melee weapons are on the guns page, the face items on the armour page.
static bool IsBobbyRMisc(const ItemModel* const item)
{
	if (item->getItemIndex() == CROWBAR) return true;
	return (item->getItemClass() & IC_BOBBY_MISC) &&
		!(item->getItemClass() & (IC_EXPLOSV | IC_FACE | IC_BLADE | IC_THROWING_KNIFE | IC_PUNCH)) && !IsBobbyRAttachment(item);
}


// Is the item in the given class of the miscellaneous page (BOBBYR_MISC_*_ITEMS)?
static bool IsInMiscClass(const ItemModel* const item, UINT32 const cls)
{
	if (!IsBobbyRMisc(item)) return false;
	UINT16 const i = item->getItemIndex();
	bool const medkit = i == FIRSTAIDKIT || i == MEDICKIT;
	bool const tool = i == TOOLKIT || i == LOCKSMITHKIT || i == METALDETECTOR || i == CROWBAR;
	bool const container = i == CANTEEN;
	switch (cls)
	{
		case BOBBYR_MISC_MEDKITS_ITEMS:    return medkit;
		case BOBBYR_MISC_TOOLS_ITEMS:      return tool;
		case BOBBYR_MISC_CONTAINERS_ITEMS: return container;
		case BOBBYR_MISC_OTHERS_ITEMS:     return !medkit && !tool && !container;
		default:                           return true; // all
	}
}


// Is the item in the given class of the explosives page (BOBBYR_EXPL_*_ITEMS, or all of them)?
static bool IsInExplosivesClass(const ItemModel* const item, UINT32 const cls)
{
	if (!(item->getItemClass() & IC_EXPLOSV)) return false;
	switch (item->getItemIndex())
	{
		case TRIP_FLARE: case TRIP_KLAXON: case BREAK_LIGHT:
			return cls == BOBBYR_EXPLOSIVES_ALL_ITEMS || cls == BOBBYR_EXPL_FLARES_ITEMS;
		case TEARGAS_GRENADE: case MUSTARD_GRENADE: case SMOKE_GRENADE:
			return cls == BOBBYR_EXPLOSIVES_ALL_ITEMS || cls == BOBBYR_EXPL_GAS_ITEMS;
		case HAND_GRENADE: case MINI_GRENADE: case STUN_GRENADE:
			return cls == BOBBYR_EXPLOSIVES_ALL_ITEMS || cls == BOBBYR_EXPL_GRENADES_ITEMS;
		case GL_HE_GRENADE: case GL_TEARGAS_GRENADE: case GL_STUN_GRENADE: case GL_SMOKE_GRENADE:
			return cls == BOBBYR_EXPLOSIVES_ALL_ITEMS || cls == BOBBYR_EXPL_40MM_ITEMS;
		case MORTAR_SHELL: case TANK_SHELL: case SHAPED_CHARGE: case MINE:
			return cls == BOBBYR_EXPLOSIVES_ALL_ITEMS || cls == BOBBYR_EXPL_HEAVY_ITEMS;
		default:
			return false;
	}
}


// Is the item in the given class of the armour page (BOBBYR_ARMOUR_*_ITEMS)? The ceramic plates
// protect the torso, so they are with the vests.
static bool IsInArmourClass(const ItemModel* const item, UINT32 const cls)
{
	if (cls == BOBBYR_ARMOUR_HEADGEAR_ITEMS) return (item->getItemClass() & IC_FACE) != 0;
	if (item->getItemClass() != IC_ARMOUR || !item->asArmour()) return false;
	UINT8 const armourClass = item->asArmour()->getArmourClass();
	switch (cls)
	{
		case BOBBYR_ARMOUR_HEAD_ITEMS: return armourClass == ARMOURCLASS_HELMET;
		case BOBBYR_ARMOUR_VEST_ITEMS: return armourClass == ARMOURCLASS_VEST || armourClass == ARMOURCLASS_PLATE;
		default:                       return armourClass == ARMOURCLASS_LEGGINGS; // legs
	}
}


// Is the item in the given class of the attachments page (BOBBYR_ATTACH_*_ITEMS)?
static bool IsInAttachmentClass(const ItemModel* const item, UINT32 const cls)
{
	UINT16 const i = item->getItemIndex();
	switch (cls)
	{
		case BOBBYR_ATTACH_FRONT_ITEMS: return i == SILENCER || i == GUN_BARREL_EXTENDER || i == DUCKBILL;
		case BOBBYR_ATTACH_TOP_ITEMS:   return i == LASERSCOPE || i == SNIPERSCOPE;
		case BOBBYR_ATTACH_REAR_ITEMS:  return i == SPRING_AND_BOLT_UPGRADE;
		default:                        return i == BIPOD; // down
	}
}


// Is the item in the given class of the guns page (BOBBYR_GUNS_*_ITEMS)?
static bool IsInGunsClass(const ItemModel* const item, UINT32 const cls)
{
	UINT8 const type = item->asWeapon() ? item->asWeapon()->ubWeaponType : NOT_GUN;
	bool const isGun = item->getItemClass() == IC_GUN;
	switch (cls)
	{
		case BOBBYR_GUNS_PISTOL_ITEMS: return isGun && (type == GUN_PISTOL || type == GUN_M_PISTOL);
		case BOBBYR_GUNS_SMG_ITEMS:    return isGun && type == GUN_SMG;
		case BOBBYR_GUNS_KNIVES_ITEMS:
			return item->isBlade() || item->isThrowingKnife() || (item->isPunch() && item->getItemIndex() != CROWBAR);
		case BOBBYR_GUNS_ASSAULT_ITEMS: return isGun && type == GUN_AS_RIFLE;
		case BOBBYR_GUNS_SNIPER_ITEMS:  return isGun && (type == GUN_SN_RIFLE || (type == GUN_RIFLE && item->getItemIndex() != ROCKET_RIFLE));
		case BOBBYR_GUNS_SHOTGUN_ITEMS: return isGun && type == GUN_SHOTGUN;
		default: // heavy
			return item->isLauncher() || (isGun && type == GUN_LMG) || item->getItemIndex() == ROCKET_RIFLE;
	}
}


bool BobbyRItemMatchesClass(const ItemModel* const item, UINT32 const uiClassMask)
{
	if (uiClassMask == BOBBYR_ATTACHMENT_ITEMS) return IsBobbyRAttachment(item);
	if (uiClassMask >= BOBBYR_EXPL_HEAVY_ITEMS && uiClassMask <= BOBBYR_EXPLOSIVES_ALL_ITEMS) return IsInExplosivesClass(item, uiClassMask);
	if (uiClassMask >= BOBBYR_ARMOUR_HEADGEAR_ITEMS && uiClassMask <= BOBBYR_ARMOUR_HEAD_ITEMS) return IsInArmourClass(item, uiClassMask);
	if (uiClassMask >= BOBBYR_AMMO_UP_TO_250_ITEMS && uiClassMask <= BOBBYR_AMMO_UP_TO_15_ITEMS) return IsInAmmoClass(item, uiClassMask);
	if (uiClassMask >= BOBBYR_ATTACH_DOWN_ITEMS && uiClassMask <= BOBBYR_ATTACH_FRONT_ITEMS) return IsInAttachmentClass(item, uiClassMask);
	if ((uiClassMask >= BOBBYR_GUNS_HEAVY_ITEMS && uiClassMask <= BOBBYR_GUNS_PISTOL_ITEMS) ||
		uiClassMask == BOBBYR_GUNS_KNIVES_ITEMS || uiClassMask == BOBBYR_GUNS_SMG_ITEMS) return IsInGunsClass(item, uiClassMask);
	if (uiClassMask >= BOBBYR_MISC_OTHERS_ITEMS && uiClassMask <= BOBBYR_MISC_ITEMS) return IsInMiscClass(item, uiClassMask);
	return (item->getItemClass() & uiClassMask) != 0;
}


void SetFirstLastPagesForNew( UINT32 uiClassMask )
{
	UINT16 i;
	INT16	sFirst = -1;
	INT16	sLast = -1;
	UINT8	ubNumItems=0;

	gubCurPage = 0;

	//First loop through to get the first and last index indexs
	for(i=0; i<MAXITEMS; i++)
	{
		//If the item is available at this stage of the game (in stock or not -- an
		//out-of-stock item is still shown, greyed out, in DisplayItemInfo() below)
		if( LaptopSaveInfo.BobbyRayInventory[ i ].fPreviouslyEligible )
		{
			if( BobbyRItemMatchesClass(GCM->getItem(LaptopSaveInfo.BobbyRayInventory[ i ].usItemIndex), uiClassMask) )
			{
				ubNumItems++;

				if( sFirst == -1 )
					sFirst = i;
				sLast = i;
			}
		}
	}

	if( ubNumItems == 0 )
	{
		gusFirstItemIndex = BOBBYR_NO_ITEMS;
		gusLastItemIndex = BOBBYR_NO_ITEMS;
		gubNumPages = 0;
		return;
	}

	gusFirstItemIndex = (UINT16)sFirst;
	gusLastItemIndex = (UINT16)sLast;
	gubNumPages = (UINT8)( ubNumItems / (FLOAT)BOBBYR_NUM_WEAPONS_ON_PAGE );
	if( (ubNumItems % BOBBYR_NUM_WEAPONS_ON_PAGE ) != 0 )
		gubNumPages += 1;
}

//Loops through Bobby Rays Used Inventory to find the first and last index
void SetFirstLastPagesForUsed()
{
	UINT16 i;
	INT16	sFirst = -1;
	INT16	sLast = -1;
	UINT8	ubNumItems=0;

	gubCurPage = 0;

	//First loop through to get the first and last index indexs
	for(i=0; i<MAXITEMS; i++)
	{
		//If the item is available at this stage of the game -- see the matching
		//comment in SetFirstLastPagesForNew() above.
		if( LaptopSaveInfo.BobbyRayUsedInventory[ i ].fPreviouslyEligible )
		{
			ubNumItems++;

			if( sFirst == -1 )
				sFirst = i;
			sLast = i;
		}
	}
	if( sFirst == -1 )
	{
		gusFirstItemIndex = BOBBYR_NO_ITEMS;
		gusLastItemIndex = BOBBYR_NO_ITEMS;
		gubNumPages = 0;
		return;
	}

	gusFirstItemIndex = (UINT16)sFirst;
	gusLastItemIndex = (UINT16)sLast;
	gubNumPages = (UINT8)( ubNumItems / (FLOAT)BOBBYR_NUM_WEAPONS_ON_PAGE );
	if( (ubNumItems % BOBBYR_NUM_WEAPONS_ON_PAGE ) != 0 )
		gubNumPages += 1;
}


static void ScrollRegionCallback(MOUSE_REGION* const, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		PrevPage();
	}
	else if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		NextPage();
	}
}


static UINT8 CheckPlayersInventoryForGunMatchingGivenAmmoID(const ItemModel* ammo);
static void SelectBigImageRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);


static void CreateMouseRegionForBigImage(UINT16 y, const UINT8 n_regions, const ItemModel* const items[])
{
	if (gfBigImageMouseRegionCreated) return;

	{
		UINT16 const x = BOBBYR_GRIDLOC_X;
		UINT16 const y = BOBBYR_GRIDLOC_Y;
		UINT16 const w = 493;
		UINT16 const h = 290;
		MSYS_DefineRegion(&g_scroll_region, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, ScrollRegionCallback);
	}

	UINT16 const x = BOBBYR_GRID_PIC_X;
	UINT16 const w = BOBBYR_GRID_PIC_WIDTH;
	UINT16 const h = BOBBYR_GRID_PIC_HEIGHT;
	for (UINT8 i = 0; i != n_regions; y += BOBBYR_GRID_OFFSET, ++i)
	{
		// Mouse region for the Big Item Image
		MOUSE_REGION& r = gSelectedBigImageRegion[i];
		MSYS_DefineRegion(&r, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, SelectBigImageRegionCallBack);
		MSYS_SetRegionUserData(&r, 0, i);

		// Specify the help text only if the items is ammo
		ItemModel const* const item = items[i];
		if (item->getItemClass() != IC_AMMO) continue;
		// And only if the user has an item that can use the particular type of ammo
		UINT8 const n_guns = CheckPlayersInventoryForGunMatchingGivenAmmoID(item);
		if (n_guns == 0) continue;

		ST::string buf = st_format_printf(str_bobbyr_guns_num_guns_that_use_ammo, n_guns);
		r.SetFastHelpText(buf);
	}

	gubNumItemsOnScreen          = n_regions;
	gfBigImageMouseRegionCreated = TRUE;
}


void DeleteMouseRegionForBigImage()
{
	if (!gfBigImageMouseRegionCreated) return;

	MSYS_RemoveRegion(&g_scroll_region);

	for (UINT8 i = 0; i != gubNumItemsOnScreen; ++i)
	{
		MSYS_RemoveRegion(&gSelectedBigImageRegion[i]);
	}

	gfBigImageMouseRegionCreated = FALSE;
	gusOldItemNumOnTopOfPage     = 65535;
	gubNumItemsOnScreen          = 0;
}


static void PurchaseBobbyRayItem(UINT16 usItemNumber);
static void UnPurchaseBobbyRayItem(UINT16 usItemNumber);


static void SelectBigImageRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		UINT16 usItemNum = (UINT16)MSYS_GetRegionUserData( pRegion, 0 );

		PurchaseBobbyRayItem( gusItemNumberForItemsOnScreen[ usItemNum] );

		fReDrawScreenFlag = TRUE;
		fPausedReDrawScreenFlag = TRUE;
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		UINT16 usItemNum = (UINT16)MSYS_GetRegionUserData( pRegion, 0 );

		UnPurchaseBobbyRayItem( gusItemNumberForItemsOnScreen[ usItemNum] );
		fReDrawScreenFlag = TRUE;
		fPausedReDrawScreenFlag = TRUE;
	}
	if (iReason & MSYS_CALLBACK_REASON_TFINGER_UP)
	{
		UINT16 usItemNum = (UINT16)MSYS_GetRegionUserData( pRegion, 0 );

		// For touch: Upper half increases, lower half decreases
		auto relY = FLOAT(gusMouseYPos - pRegion->Y()) / FLOAT(pRegion->H());

		if (relY <= 0.5) {
			PurchaseBobbyRayItem( gusItemNumberForItemsOnScreen[ usItemNum] );
		} else {
			UnPurchaseBobbyRayItem( gusItemNumberForItemsOnScreen[ usItemNum] );
		}

		fReDrawScreenFlag = TRUE;
		fPausedReDrawScreenFlag = TRUE;
	}
	else if(iReason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT)
	{
		UINT16 usItemNum = (UINT16)MSYS_GetRegionUserData( pRegion, 0 );

		PurchaseBobbyRayItem( gusItemNumberForItemsOnScreen[ usItemNum] );
		fReDrawScreenFlag = TRUE;
		fPausedReDrawScreenFlag = TRUE;
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_REPEAT)
	{
		UINT16 usItemNum = (UINT16)MSYS_GetRegionUserData( pRegion, 0 );

		UnPurchaseBobbyRayItem( gusItemNumberForItemsOnScreen[ usItemNum] );
		fReDrawScreenFlag = TRUE;
		fPausedReDrawScreenFlag = TRUE;
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		PrevPage();
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		NextPage();
	}
}


static UINT8 GetNextPurchaseNumber(void);

static void PurchaseBobbyRayItem(UINT16 usItemNumber)
{
	UINT8	ubPurchaseNumber;

	ubPurchaseNumber = CheckIfItemIsPurchased(usItemNumber);

	//if we are in the used page
	if( guiCurrentLaptopMode == LAPTOP_MODE_BOBBY_R_USED )
	{
		//if there is enough inventory in stock to cover the purchase -- unlike the
		//original "|| ubPurchaseNumber == BOBBY_RAY_NOT_PURCHASED" short-circuit,
		//an out-of-stock item's very first click (not yet in BobbyRayPurchases[])
		//is checked too, needing 1 unit rather than ubNumberPurchased + 1
		if( LaptopSaveInfo.BobbyRayUsedInventory[ usItemNumber ].ubQtyOnHand >=
			( UINT8 )( ubPurchaseNumber == BOBBY_RAY_NOT_PURCHASED ? 1 : BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased + 1 ) )
		{
			// If the item has not yet been purchased
			if( ubPurchaseNumber == BOBBY_RAY_NOT_PURCHASED )
			{
				ubPurchaseNumber = GetNextPurchaseNumber();

				if( ubPurchaseNumber != BOBBY_RAY_NOT_PURCHASED )
				{
					BobbyRayPurchases[ ubPurchaseNumber ].usItemIndex = LaptopSaveInfo.BobbyRayUsedInventory[ usItemNumber ].usItemIndex;
					BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased = 1;
					BobbyRayPurchases[ ubPurchaseNumber ].bItemQuality = LaptopSaveInfo.BobbyRayUsedInventory[ usItemNumber ].ubItemQuality;
					BobbyRayPurchases[ ubPurchaseNumber ].usBobbyItemIndex = usItemNumber;
					BobbyRayPurchases[ ubPurchaseNumber ].fUsed = TRUE;
				}
				else
				{
					//display error popup because the player is trying to purchase more thenn 10 items
					DoLapTopMessageBox( MSG_BOX_LAPTOP_DEFAULT, BobbyRText[ BOBBYR_MORE_THEN_10_PURCHASES ], LAPTOP_SCREEN, MSG_BOX_FLAG_OK, NULL);

				}
			}
			// Else If the item is already purchased increment purchase amount.  Only if ordering less then the max amount!
			else
			{
				if( BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased <= BOBBY_RAY_MAX_AMOUNT_OF_ITEMS_TO_PURCHASE)
					BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased++;
			}
		}
		else
		{
			DoLapTopMessageBox( MSG_BOX_LAPTOP_DEFAULT, BobbyRText[ BOBBYR_MORE_NO_MORE_IN_STOCK ], LAPTOP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		}
	}
	//else the player is on a any other page except the used page
	else
	{
		//if there is enough inventory in stock to cover the purchase -- see the
		//matching comment in the used-page branch above.
		if( LaptopSaveInfo.BobbyRayInventory[ usItemNumber ].ubQtyOnHand >=
			( UINT8 )( ubPurchaseNumber == BOBBY_RAY_NOT_PURCHASED ? 1 : BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased + 1 ) )
		{
			// If the item has not yet been purchased
			if( ubPurchaseNumber == BOBBY_RAY_NOT_PURCHASED )
			{
				ubPurchaseNumber = GetNextPurchaseNumber();

				if( ubPurchaseNumber != BOBBY_RAY_NOT_PURCHASED )
				{
					BobbyRayPurchases[ ubPurchaseNumber ].usItemIndex = LaptopSaveInfo.BobbyRayInventory[ usItemNumber ].usItemIndex;
					BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased = 1;
					BobbyRayPurchases[ ubPurchaseNumber ].bItemQuality = 100;
					BobbyRayPurchases[ ubPurchaseNumber ].usBobbyItemIndex = usItemNumber;
					BobbyRayPurchases[ ubPurchaseNumber ].fUsed = FALSE;
				}
				else
				{
					//display error popup because the player is trying to purchase more thenn 10 items
					DoLapTopMessageBox( MSG_BOX_LAPTOP_DEFAULT, BobbyRText[ BOBBYR_MORE_THEN_10_PURCHASES ], LAPTOP_SCREEN, MSG_BOX_FLAG_OK, NULL);
				}
			}
			// Else If the item is already purchased increment purchase amount.  Only if ordering less then the max amount!
			else
			{
				if( BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased <= BOBBY_RAY_MAX_AMOUNT_OF_ITEMS_TO_PURCHASE)
					BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased++;
			}
		}
		else
		{
			// No more "Sorry, we don't have any more..." popup here: an item fully claimed
			// by the player's own cart now shows greyed out with "Out of stock." instead
			// (fShowAsOutOfStock, DisplayItemInfo()), so a further click just does nothing.
		}
	}
}


// Checks to see if the clicked item is already bought or not.
static UINT8 CheckIfItemIsPurchased(UINT16 usItemNumber)
{
	UINT8	i;

	for(i=0; i<MAX_PURCHASE_AMOUNT; i++)
	{
		if( ( usItemNumber == BobbyRayPurchases[i].usBobbyItemIndex ) && ( BobbyRayPurchases[i].ubNumberPurchased != 0 ) && ( BobbyRayPurchases[i].fUsed == gfOnUsedPage ) )
			return(i);
	}
	return(BOBBY_RAY_NOT_PURCHASED);
}


static UINT8 GetNextPurchaseNumber(void)
{
	UINT8	i;

	for(i=0; i<MAX_PURCHASE_AMOUNT; i++)
	{
		if( ( BobbyRayPurchases[i].usBobbyItemIndex == 0) && ( BobbyRayPurchases[i].ubNumberPurchased == 0 ) )
			return(i);
	}
	return(BOBBY_RAY_NOT_PURCHASED);
}


static void UnPurchaseBobbyRayItem(UINT16 usItemNumber)
{
	UINT8	ubPurchaseNumber;

	ubPurchaseNumber = CheckIfItemIsPurchased(usItemNumber);

	if( ubPurchaseNumber != BOBBY_RAY_NOT_PURCHASED )
	{
		if( BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased > 1)
			BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased--;
		else
		{
			BobbyRayPurchases[ ubPurchaseNumber ].ubNumberPurchased = 0;
			BobbyRayPurchases[ ubPurchaseNumber ].usBobbyItemIndex = 0;
		}
	}
}


static void BtnBobbyROrderFormCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_BOBBY_R_MAILORDER;
	}
}


static void BtnBobbyRHomeButtonCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_BOBBY_R;
	}
}


void UpdateButtonText(UINT32	uiCurPage)
{
	// the page buttons are gone (the guns page has the buttons of its classes)
}


UINT16 CalcBobbyRayCost( UINT16 usIndex, UINT16 usBobbyIndex, BOOLEAN fUsed)
{
	DOUBLE value;
	if( fUsed )
		value = GCM->getItem(LaptopSaveInfo.BobbyRayUsedInventory[ usBobbyIndex ].usItemIndex)->getPrice() *
								( .5 + .5 * ( LaptopSaveInfo.BobbyRayUsedInventory[ usBobbyIndex ].ubItemQuality ) / 100 ) + .5;
	else
		value = GCM->getItem(LaptopSaveInfo.BobbyRayInventory[ usBobbyIndex ].usItemIndex)->getPrice();

	return( (UINT16) value);
}


static UINT32 CalculateTotalPurchasePrice()
{
	UINT32 total = 0;
	FOR_EACH(BobbyRayPurchaseStruct const, i, BobbyRayPurchases)
	{
		BobbyRayPurchaseStruct const& p = *i;
		if (p.ubNumberPurchased == 0) continue;
		total += CalcBobbyRayCost(p.usItemIndex, p.usBobbyItemIndex, p.fUsed) * p.ubNumberPurchased;
	}
	return total;
}


static void DisableBobbyRButtons(void)
{
	//if it is the last page, disable the next page button
	EnableButton(guiBobbyRNextPage, gubNumPages != 0 && gubCurPage < gubNumPages - 1);

	// if it is the first page, disable the prev page buitton
	EnableButton(guiBobbyRPreviousPage, gubCurPage != 0);
}


static void CalcFirstIndexForPage(STORE_INVENTORY* const pInv, UINT32 const item_class)
{
	// Reset the Current weapon Index
	gusCurWeaponIndex = 0;

	// Get to the first index on the page
	UINT16 inv_idx = 0;
	for (UINT16 i = gusFirstItemIndex; i <= gusLastItemIndex; ++i)
	{
		if (!BobbyRItemMatchesClass(GCM->getItem(pInv[i].usItemIndex), item_class)) continue;
		// If the item isn't available at this stage of the game at all -- see the
		// matching comment in SetFirstLastPagesForNew() above.
		if (!pInv[i].fPreviouslyEligible) continue;

		gusCurWeaponIndex = i;
		if (inv_idx++ == gubCurPage * 4) break;
	}
}


static UINT8 CheckPlayersInventoryForGunMatchingGivenAmmoID(ItemModel const* const ammo)
{
	UINT8 n_items = 0;
	const CalibreModel *calibre = ammo->asAmmo()->calibre;
	CFOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		// Loop through all the pockets on the merc
		CFOR_EACH_SOLDIER_INV_SLOT(i, *s)
		{
			OBJECTTYPE const& o = *i;
			// If there is a weapon here
			if (GCM->getItem(o.usItem)->getItemClass() != IC_GUN) continue;
			// If the weapon uses the same kind of ammo as the one passed in
			if (!GCM->getWeapon(o.usItem)->matches(calibre)) continue;

			++n_items;
		}
	}
	return n_items;
}
