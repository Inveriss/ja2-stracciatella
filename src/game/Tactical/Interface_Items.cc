#include "Assignments.h"
#include "Directories.h"
#include "Font.h"
#include "Handle_Items.h"
#include "Isometric_Utils.h"
#include "Local.h"
#include "Item_Types.h"
#include "LoadSaveData.h"
#include "LoadSaveObjectType.h"
#include "HImage.h"
#include "Map_Screen_Interface_Bottom.h"
#include "Object_Cache.h"
#include "Soldier_Macros.h"
#include "TileDef.h"
#include "Timer_Control.h"
#include "VObject.h"
#include "SysUtil.h"
#include "Overhead.h"
#include "MouseSystem.h"
#include "Button_System.h"
#include "Interface.h"
#include "VSurface.h"
#include "Input.h"
#include "Handle_UI.h"
#include "RenderWorld.h"
#include "Cursors.h"
#include "Font_Control.h"
#include "Render_Dirty.h"
#include "Interface_Panels.h"
#include "Animation_Control.h"
#include "Animation_Data.h"
#include "Shading.h"
#include "Soldier_Control.h"
#include "PathAI.h"
#include "Weapons.h"
#include "Faces.h"
#include "MapScreen.h"
#include "Message.h"
#include "Text.h"
#include "Cursor_Control.h"
#include "Interface_Cursors.h"
#include "Interface_Utils.h"
#include "Interface_Items.h"
#include "WordWrap.h"
#include "Interface_Control.h"
#include "VObject_Blitters.h"
#include "World_Items.h"
#include "Points.h"
#include "Physics.h"
#include "Finances.h"
#include "UI_Cursors.h"
#include "ShopKeeper_Interface.h"
#include "Dialogue_Control.h"
#include "English.h"
#include "Keys.h"
#include "Game_Clock.h"
#include "Squads.h"
#include "LaptopSave.h"
#include "MessageBoxScreen.h"
#include "GameSettings.h"
#include "Map_Screen_Interface_Map_Inventory.h"
#include "Quests.h"
#include "Map_Screen_Interface.h"
#include "OppList.h"
#include "LOS.h"
#include "JAScreens.h"
#include "ScreenIDs.h"
#include "Video.h"
#include "Debug.h"
#include "Items.h"
#include "UILayout.h"

#include "CalibreModel.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "MagazineModel.h"
#include "Soldier.h"
#include "WeaponModels.h"
#include "policy/GamePolicy.h"
#include "Logger.h"
#include "MercProfile.h"

#include <string_theory/format>
#include <string_theory/string>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <vector>

#define ITEMDESC_FONT					BLOCKFONT2
#define ITEMDESC_FONTSHADOW2				32

#define ITEMDESC_FONTAPFORE				218
#define ITEMDESC_FONTHPFORE				24
#define ITEMDESC_FONTBSFORE				125
#define ITEMDESC_FONTHEFORE				75
#define ITEMDESC_FONTHEAPFORE				76

#define ITEMDESC_AMMO_FORE				209

#define ITEMDESC_FONTHIGHLIGHT				FONT_MCOLOR_WHITE

#define STATUS_BAR_SHADOW				FROMRGB( 140, 136,  119 )
#define STATUS_BAR					FROMRGB( 201, 172,  133 )
#define DESC_STATUS_BAR_SHADOW				STATUS_BAR_SHADOW
#define DESC_STATUS_BAR				STATUS_BAR

#define INV_BAR_DX					7
#define INV_BAR_DY					30

// Status bar next to each inventory slot on the strategic map (mapinv.sti)
// -- independent of the tactical INV_BAR_DX/INV_BAR_DY/ITEM_BAR_HEIGHT
// above, which the item stack popup and keyring popup also share. Used only
// by INVRenderINVPanelItem() while in MAP_SCREEN.
// MAP_INV_BAR_DX: bar X = slot X - MAP_INV_BAR_DX (positive = further left).
// MAP_INV_BAR_DY: Y of the bar's BOTTOM edge = slot Y + MAP_INV_BAR_DY (the
//                 bar grows upwards from there).
// MAP_ITEM_BAR_HEIGHT: bar height at 100% status.
// MAP_ITEM_BAR_WIDTH: bar width; the last column is drawn in the shadow
//                 colour, the rest in the main colour (2 = original look).
#define MAP_INV_BAR_DX_NORMAL				7
#define MAP_INV_BAR_DY_NORMAL				30
#define MAP_ITEM_BAR_HEIGHT_NORMAL			31
#define MAP_ITEM_BAR_WIDTH_NORMAL			2

// Same four for the map's "Show Large Icons" mode (mapinv_big_1280_720/768.sti,
// IsMapInvBigImages() in MapScreen.cc) -- independent of the normal ones
// above; start equal to them.
#define MAP_INV_BAR_DX_BIG				7
#define MAP_INV_BAR_DY_BIG				49
#define MAP_ITEM_BAR_HEIGHT_BIG				50
#define MAP_ITEM_BAR_WIDTH_BIG				2

#define MAP_INV_BAR_DX      (IsMapInvBigImages() ? MAP_INV_BAR_DX_BIG      : MAP_INV_BAR_DX_NORMAL)
#define MAP_INV_BAR_DY      (IsMapInvBigImages() ? MAP_INV_BAR_DY_BIG      : MAP_INV_BAR_DY_NORMAL)
#define MAP_ITEM_BAR_HEIGHT (IsMapInvBigImages() ? MAP_ITEM_BAR_HEIGHT_BIG : MAP_ITEM_BAR_HEIGHT_NORMAL)
#define MAP_ITEM_BAR_WIDTH  (IsMapInvBigImages() ? MAP_ITEM_BAR_WIDTH_BIG  : MAP_ITEM_BAR_WIDTH_NORMAL)

// Cover (map_inv_2nd_gun_cover.sti) drawn over the second hand slot on the
// strategic map while the first hand holds a two-handed item -- independent
// of the tactical secondary_gun_hidden.sti position/size. X/Y are relative
// to MAP_SCREEN_X/MAP_SCREEN_Y (the left column's own origin, like
// m_invSlotPositionMap[] in UILayout.cc), not to the legacy 640x480
// STD_SCREEN_X/Y window. WIDTH/HEIGHT is the area refreshed after the blit
// and should match the graphic's size. The defaults mirror the tactical
// cover's placement relative to its slot (status bar + slot, 75x35) for the
// SECONDHANDPOS slot at MAP_SCREEN_X + 88, MAP_SCREEN_Y + 225.
#define MAP_INV_2ND_GUN_COVER_X				79
#define MAP_INV_2ND_GUN_COVER_Y				223
#define MAP_INV_2ND_GUN_COVER_WIDTH			75
#define MAP_INV_2ND_GUN_COVER_HEIGHT			35

// Same cover in the map's "Show Large Icons" mode (mapinv_big_1280_720/768.sti,
// IsMapInvBigImages() in MapScreen.cc) -- its own graphic,
// secondary_gun_hidden_big.sti, and its own position/size, independent of
// the MAP_INV_2ND_GUN_COVER_* above and of the tactical cover. X/Y relative
// to MAP_SCREEN_X/MAP_SCREEN_Y; WIDTH/HEIGHT is the area refreshed after
// the blit and should match the graphic. Defaults: 2 px left of the
// large-mode status bar (MAP_INV_BAR_DX_BIG) and 2 px above the SECONDHANDPOS
// slot of m_invSlotPositionMapBig[] (UILayout.cc), like the normal cover;
// size of the current graphic. Falls back to the normal cover if the file
// is missing.
#define MAP_INV_2ND_GUN_COVER_BIG_X			96
#define MAP_INV_2ND_GUN_COVER_BIG_Y			303
#define MAP_INV_2ND_GUN_COVER_BIG_WIDTH			130
#define MAP_INV_2ND_GUN_COVER_BIG_HEIGHT		52

// Adjustable offset (from the slot's own top-left corner) and size delta
// (relative to the slot's own width/height) for the "item doesn't fit
// here" hatch drawn over inventory slots in INVRenderINVPanelItem() --
// see DrawHatchOnInventory() below. All zero preserves the original
// behaviour (hatch pixel-for-pixel matches the slot rect). Positive
// OFFSET_X/Y moves it right/down, negative left/up; positive WIDTH_DELTA/
// HEIGHT_DELTA grows it past the slot's own edges, negative shrinks it.
// Scoped to this one call site only -- see ATTACHMENT_HATCH_* below for the
// separate attachment-slot hatch in the item description box.
#define HATCH_OFFSET_X					-1
#define HATCH_OFFSET_Y					0
#define HATCH_WIDTH_DELTA				0
#define HATCH_HEIGHT_DELTA				0

// Same idea as HATCH_OFFSET_X/Y/HATCH_WIDTH_DELTA/HATCH_HEIGHT_DELTA above,
// but for the attachment-slot hatch drawn in the item description box
// (Infobox.sti) when an item can't be attached/merged/launched onto the
// examined item -- see InternalInitItemDescriptionBox() below. Independent
// of the inventory-slot constants above; tune separately.
#define ATTACHMENT_HATCH_OFFSET_X			7
#define ATTACHMENT_HATCH_OFFSET_Y			1
#define ATTACHMENT_HATCH_WIDTH_DELTA			-8
#define ATTACHMENT_HATCH_HEIGHT_DELTA			-1

#define RENDER_ITEM_NOSTATUS				20
#define RENDER_ITEM_ATTACHMENT1			200

#define ITEM_STATS_WIDTH				26
#define ITEM_STATS_HEIGHT				8
#define MAX_STACK_POPUP_WIDTH				6

#define ITEMDESC_START_X				(INTERFACE_START_X + 214)
#define ITEMDESC_START_Y				(1 + ITEMDESC_PANEL_START_Y)


// #define ITEMDESC_HEIGHT				133
// #define ITEMDESC_WIDTH				320

// NEW POSITION
// Size of the tactical item description box's mouse region -- one
// independent copy per box background (Infobox.sti/Infobox_money.sti/
// Infobox_items.sti), selected in InternalInitItemDescriptionBox()/
// RenderItemDescriptionBox() via fIsWeapon/fIsMoney. Also used to size the
// RestoreExternBackgroundRect() call in RenderItemDescriptionBox() that
// restores the background under the whole popup -- that call Asserts the
// rect stays on-screen, so each _HEIGHT here MUST stay a few px under its
// matching ITEMDESC_PANEL_HEIGHT (UILayout.h), since the box is anchored
// ITEMDESC_PANEL_HEIGHT[_MONEY/_ITEMS] px up from the bottom of the screen.
// Derived from the panel height (with the same 3px margin ITEMDESC_HEIGHT/
// ITEMDESC_PANEL_HEIGHT already used) instead of a separate hardcoded
// number, so retuning the panel height can't silently push the box off the
// bottom of the screen again.
#define ITEMDESC_HEIGHT				    (ITEMDESC_PANEL_HEIGHT - 3)        // Infobox.sti (weapon)
#define ITEMDESC_WIDTH					542
#define ITEMDESC_HEIGHT_MONEY			    (ITEMDESC_PANEL_HEIGHT_MONEY - 3)  // Infobox_money.sti
#define ITEMDESC_WIDTH_MONEY			542
#define ITEMDESC_HEIGHT_ITEMS			    (ITEMDESC_PANEL_HEIGHT_ITEMS - 3)  // Infobox_items.sti
#define ITEMDESC_WIDTH_ITEMS			542


#define MAP_ITEMDESC_HEIGHT				268
#define MAP_ITEMDESC_WIDTH				272
#define ITEMDESC_ITEM_X				(8 + gsInvDescX)
#define ITEMDESC_ITEM_Y				(11 + gsInvDescY)

#define CAMO_REGION_HEIGHT				75
#define CAMO_REGION_WIDTH				75

/*
#define BULLET_SING_X				(222 + gsInvDescX)
#define BULLET_SING_Y				(49 + gsInvDescY)
#define BULLET_BURST_X				(263 + gsInvDescX)
#define BULLET_BURST_Y				(49 + gsInvDescY)
*/



// NEW POSITION
#define BULLET_SING_X					(435 + gsInvDescX)
#define BULLET_SING_Y					(42 + gsInvDescY)  
#define BULLET_BURST_X					(458 + gsInvDescX) 
#define BULLET_BURST_Y					(88 + gsInvDescY) 
#define BULLET_WIDTH					3



// Purely decorative row of 3 bullet icons — cosmetic only, not tied to any
// weapon stat (unlike the single/burst bullet icons above).
#define BULLET_DECOR_X					(431 + gsInvDescX)  // NEW POSITION
#define BULLET_DECOR_Y					(71 + gsInvDescY) // NEW POSITION

#define MAP_BULLET_SING_X				(77 + gsInvDescX)
#define MAP_BULLET_SING_Y				(135 + gsInvDescY)
#define MAP_BULLET_BURST_X				(117 + gsInvDescX)
#define MAP_BULLET_BURST_Y				(135 + gsInvDescY)

/*
static const SGPBox g_itemdesc_desc_box            = { 11,  80, 301,  0 };
static const SGPBox g_itemdesc_pros_cons_box       = { 11, 110, 301, 10 };
static const SGPBox g_itemdesc_item_status_box     = {  6,  60,   2, 51 };
*/

// NEW POSITION
// Item description text box. One independent copy per tactical item
// description box background (Infobox.sti/Infobox_money.sti/
// Infobox_items.sti) -- see fIsWeapon/fIsMoney in RenderItemDescriptionBox().
// All three currently equal (preserving today's shared position); tune
// independently as needed.
static const SGPBox g_itemdesc_desc_box            = { 14,  226, 375, 0 }; // Infobox.sti (weapon) / description
static const SGPBox g_itemdesc_desc_box_money      = { 19,  122, 334, 0 }; // Infobox_money.sti / description
static const SGPBox g_itemdesc_desc_box_items      = { 19,  122, 232, 0 }; // Infobox_items.sti / description
static const SGPBox g_itemdesc_pros_cons_box       = { 14, 270, 375, 10 };
// x position status bar in infobox.sti ### y position ### width of status
// bar ### length of status bar. One independent copy per tactical item
// description box background (Infobox.sti/Infobox_money.sti/
// Infobox_items.sti) -- see fIsWeapon/fIsMoney in RenderItemDescriptionBox().
// All three currently equal (preserving today's on-screen position); tune
// independently as needed.
static const SGPBox g_itemdesc_item_status_box     = { 156, 115,   2, 69 }; // Infobox.sti (weapon) / status bar
static const SGPBox g_itemdesc_item_status_box_money = { 14, 82,   2, 69 }; // Infobox_money.sti / status bar
static const SGPBox g_itemdesc_item_status_box_items = { 14, 82,   2, 69 }; // Infobox_items.sti / status bar

static const SGPBox g_map_itemdesc_desc_box        = { 23, 170, 220,  0 };
static const SGPBox g_map_itemdesc_pros_cons_box   = { 23, 230, 220, 10 };
static const SGPBox g_map_itemdesc_item_status_box = { 18,  54,   2, 42 };

// Map screen (ItemInfoC.sti), 1366x768 base with the normal ("small icons")
// merc inventory only: own copies of every position on the box for
// non-weapon items (_items), for money (_money) and for weapons (_weapons),
// picked by GetMapItemDescLayout()/PickMapItemDesc() below. The other
// resolutions and "Show Large Icons" keep the shared map ones. This goes for
// every _items/_money/_weapons map table in this file. The layouts have
// their own backgrounds, iteminfoc_items_1366x768.png,
// iteminfoc_money_1366x768.png and iteminfoc_weapons_1366x768.png, the same
// size and arrangement as the tactical Infobox_items.sti/Infobox_money.sti/
// Infobox.sti, so the values start out as the tactical ones -- tune
// independently as needed.
static const SGPBox g_map_itemdesc_desc_box_items        = { 19, 122, 232,  0 }; // iteminfoc_items / description
static const SGPBox g_map_itemdesc_desc_box_money        = { 19, 122, 334,  0 }; // iteminfoc_money / description
static const SGPBox g_map_itemdesc_item_status_box_items = { 14,  82,   2, 69 }; // iteminfoc_items / status bar
static const SGPBox g_map_itemdesc_item_status_box_money = { 14,  82,   2, 69 }; // iteminfoc_money / status bar
// iteminfoc_weapons_1366x768.png: the size and arrangement of the tactical
// Infobox.sti, so every _weapons map table starts out as the tactical one.
static const SGPBox g_map_itemdesc_desc_box_weapons        = { 14,  226, 375, 0 };  // iteminfoc_weapons / description
static const SGPBox g_map_itemdesc_pros_cons_box_weapons   = { 14, 270, 375, 10 };  // iteminfoc_weapons / pros and cons
static const SGPBox g_map_itemdesc_item_status_box_weapons = { 156, 115,   2, 69 }; // iteminfoc_weapons / status bar
// Bullet icons of iteminfoc_weapons: single shot, burst, the decorative row
// -- the map's counterparts of BULLET_SING_X/Y, BULLET_BURST_X/Y and
// BULLET_DECOR_X/Y, relative to the box.
static const SGPPoint g_map_weapons_bullet_sing  = { 435, 42 };
static const SGPPoint g_map_weapons_bullet_burst = { 458, 88 };
static const SGPPoint g_map_weapons_bullet_decor = { 431, 71 };
// "Hide empty attachment slots" checkbox on iteminfoc_weapons itself.
static const SGPPoint g_map_weapons_hide_empty_slots_checkbox = { 8, 178 };

// iteminfoc_weapons_big_1366x768.png (604x635), the weapon box of "Show
// Large Icons". Every _weapons_big map table in this file is a placeholder
// -- a copy of its _weapons counterpart, to be placed by hand on the new
// graphic.
// The description and pros / cons share the long box along the bottom of
// iteminfoc_weapons_big (682x38 inside, from 23,584), all in FONT12ARIAL
// (11 px high): the description on one line, 5 px from the box's top and
// left; pros and cons on one line 5 px from its bottom, pros 5 px from its
// left, cons from x 350. All three end in "..." where they don't fit. The
// weapon's name line has a box of its own, see gMapDescNameBox_Weapons_Big.
#define MAP_BIG_WEAPON_DESC_LINES    1 // lines of the description; the last ends in "..." if it goes on
#define MAP_BIG_WEAPON_DESC_LINE_GAP 1 // between the description's lines, if more than one
// The colour of the pros / cons lists. On the other boxes they are drawn in
// ITEMDESC_FONT's shadow colour, ITEMDESC_FONTSHADOW2 -- (201, 172, 133) in
// BLOCKFONT2's palette, black in FONT12ARIAL's -- so here that colour's
// nearest one in FONT12ARIAL's palette.
#define MAP_BIG_WEAPON_PROS_CONS_COLOUR 127 // (210, 169, 127)
static const SGPBox g_map_itemdesc_desc_box_weapons_big        = {  28, 589, 672,  0 }; // iteminfoc_weapons_big / description
static const SGPBox g_map_itemdesc_pros_cons_box_weapons_big   = {  28, 607, 312,  0 }; // iteminfoc_weapons_big / pros
static const SGPBox g_map_itemdesc_cons_box_weapons_big        = { 350, 607, 350,  0 }; // iteminfoc_weapons_big / cons, same line
static const SGPBox g_map_itemdesc_item_status_box_items_big   = {  14, 138,   2, 125 }; // iteminfoc_items_big / status bar, 2x125, y is its bottom
static const SGPBox g_map_itemdesc_desc_box_items_big          = {  19, 246, 276,   0 }; // iteminfoc_items_big / description (286x85 inside, from 14,241)
static const SGPBox g_map_itemdesc_item_status_box_weapons_big = { 312, 192,   2, 125 }; // iteminfoc_weapons_big / status bar, 2x125, y is its bottom
// The bullet icon of "AP:" in the Single AP field. The box has no burst or
// decorative bullets: Short / Long / Full have their own rounds.
static const SGPPoint g_map_weapons_big_bullet_sing  = { 386, 426 };

#define DOTDOTDOT "..."
#define COMMA_AND_SPACE ", "

#define ITEM_PROS_AND_CONS( usItem )			( ( GCM->getItem(usItem)->isGun()) )

/*
#define ITEMDESC_AMMO_TEXT_X				4
#define ITEMDESC_AMMO_TEXT_Y				2
#define ITEMDESC_AMMO_TEXT_WIDTH			40
*/

// NEW POSITION
#define ITEMDESC_AMMO_TEXT_X				4
#define ITEMDESC_AMMO_TEXT_Y				2
#define ITEMDESC_AMMO_TEXT_WIDTH			40


#define ITEM_BAR_HEIGHT				31

#define ITEM_FONT					TINYFONT1

#define EXCEPTIONAL_DAMAGE				30
constexpr grams EXCEPTIONAL_WEIGHT = 2000;
#define EXCEPTIONAL_RANGE				300
#define EXCEPTIONAL_MAGAZINE				30
#define EXCEPTIONAL_AP_COST				7
#define EXCEPTIONAL_BURST_SIZE				5
#define EXCEPTIONAL_RELIABILITY			2
#define EXCEPTIONAL_REPAIR_EASE			2

#define BAD_DAMAGE					23
#define BAD_WEIGHT					45
#define BAD_RANGE					150
#define BAD_MAGAZINE					10
#define BAD_AP_COST					11
#define BAD_RELIABILITY				-2
#define BAD_REPAIR_EASE				-2

// (on the 1366x768 SM panel in its top strip, 1 px left)
#define KEYRING_X      (INTERFACE_START_X + (g_ui.isBigSMPanel() ? 313 : 314))
#define KEYRING_Y      (INV_INTERFACE_START_Y + (g_ui.isBigSMPanel() ? 2 : 160))
// Strategic map keyring icon -- laid out next to the map inventory's Done
// button, see MAP_INV_KEYRING_X/Y in MapScreen.h.
#define MAP_KEYRING_X (MAP_INV_KEYRING_X)
#define MAP_KEYRING_Y (MAP_INV_KEYRING_Y)
#define KEYRING_WIDTH   32
#define KEYRING_HEIGHT  32
#define TACTICAL_INVENTORY_KEYRING_GRAPHIC_OFFSET_X 215
//enum used for the money buttons
enum
{
	M_1000,
	M_100,
	M_10,
	M_DONE,
};
// Number of buttons in the money withdrawal box (3 quick amounts + Done).
// Deliberately independent from MAX_ATTACHMENTS - the two happened to match
// by coincidence and must not be tied together.
#define NUM_MONEY_BUTTONS (M_DONE + 1)

BOOLEAN gfAddingMoneyToMercFromPlayersAccount;

MOUSE_REGION gInvDesc;

OBJECTTYPE *gpItemPointer;
OBJECTTYPE gItemPointer;
BOOLEAN gfItemPointerDifferentThanDefault = FALSE;
SOLDIERTYPE *gpItemPointerSoldier;
INT8 gbItemPointerSrcSlot;
static UINT16 gusItemPointer = 255;
static UINT32 guiNewlyPlacedItemTimer = 0;
static BOOLEAN gfBadThrowItemCTGH;
BOOLEAN gfDontChargeAPsToPickup = FALSE;
static BOOLEAN gbItemPointerLocateGood = FALSE;

// Which set of map-screen (ItemInfoC.sti) positions the open description box
// uses. The _items/_money/_weapons sets only exist for the 1366x768 base
// (_weapons with the normal, "small icons" merc inventory only), _weapons_big
// for the same base with "Show Large Icons"; other resolutions stay on the
// shared set. Items and money of "Show Large Icons" are on their own boxes
// too (gfMapItemDescArialBox), with the _items_big tables.
enum MapItemDescLayout
{
	MAP_ITEMDESC_LAYOUT_SHARED,
	MAP_ITEMDESC_LAYOUT_ITEMS,
	MAP_ITEMDESC_LAYOUT_MONEY,
	MAP_ITEMDESC_LAYOUT_WEAPONS,
	MAP_ITEMDESC_LAYOUT_WEAPONS_BIG // weapons, "Show Large Icons"
};

// Set once per box in InternalInitItemDescriptionBox(), so the mouse regions
// and buttons created there and RenderItemDescriptionBox() always agree,
// even if "Show Large Icons" is toggled while the box is open.
static MapItemDescLayout gMapItemDescLayout = MAP_ITEMDESC_LAYOUT_SHARED;

// Backgrounds of the _items/_money/_weapons layouts, and the money layout's own copy
// of info_bil.sti. Without its background a layout is not used at all -- its
// positions only fit that graphic.
#define MAP_ITEMDESC_BOX_ITEMS_FILE INTERFACEDIR "/iteminfoc_items_1366x768.png"
#define MAP_ITEMDESC_BOX_MONEY_FILE INTERFACEDIR "/iteminfoc_money_1366x768.png"
#define MAP_ITEMDESC_BOX_WEAPONS_FILE INTERFACEDIR "/iteminfoc_weapons_1366x768.png"
#define MAP_ITEMDESC_BOX_WEAPONS_BIG_FILE INTERFACEDIR "/iteminfoc_weapons_big_1366x768.png"
// "Show Large Icons" versions of the items / money boxes (604x340, both the
// same arrangement -- the _items_big tables, PickMapItemDesc()), their texts
// in FONT12ARIAL -- see gfMapItemDescArialBox.
#define MAP_ITEMDESC_BOX_ITEMS_BIG_FILE INTERFACEDIR "/iteminfoc_items_big_1366x768.png"
#define MAP_ITEMDESC_BOX_MONEY_BIG_FILE INTERFACEDIR "/iteminfoc_money_big_1366x768.png"

// The open box is the items or money box of "Show Large Icons"
// (MAP_ITEMDESC_BOX_ITEMS_BIG_FILE / MAP_ITEMDESC_BOX_MONEY_BIG_FILE): its
// texts are in FONT12ARIAL. Set with gMapItemDescLayout.
static BOOLEAN gfMapItemDescArialBox = FALSE;

// The open box is the 1366x768 SM panel's weapon box -- see TacBigWeaponShift().
#define TAC_ITEMDESC_BOX_WEAPONS_BIG_FILE INTERFACEDIR "/infobox_weapons_big_1366x768.png"
static bool gfTacBigWeaponBox = false;
// The open box is the 1366x768 SM panel's money box,
// infobox_money_big_1366x768.png: the map's "Show Large Icons" money box
// (iteminfoc_money_big) as it is -- its layout and tables unchanged.
#define TAC_ITEMDESC_BOX_MONEY_BIG_FILE INTERFACEDIR "/infobox_money_big_1366x768.png"
static bool gfTacBigMoneyBox = false;
// The open box is the 1366x768 SM panel's items box (not weapons, not money),
// infobox_items_big_1366x768.png: likewise the map's "Show Large Icons" items
// box (iteminfoc_items_big) as it is.
#define TAC_ITEMDESC_BOX_ITEMS_BIG_FILE INTERFACEDIR "/infobox_items_big_1366x768.png"
static bool gfTacBigItemsBox = false;
// the money and items boxes' x, at the screen's bottom
#define TAC_ITEMDESC_BOX_MONEY_BIG_X 236
// any of them (the map's box layouts; their places and what they hide:
// InternalInitItemDescriptionBox())
static bool IsTacBigItemDescBox() { return gfTacBigWeaponBox || gfTacBigMoneyBox || gfTacBigItemsBox; }
bool InTacBigWeaponBox() { return gfTacBigWeaponBox && InItemDescriptionBox(); }
#define MONEY_GRAPHICS_FILE         INTERFACEDIR "/info_bil.sti"
#define MAP_MONEY_GRAPHICS_FILE     INTERFACEDIR "/info_bil_1366x768.sti"

namespace {
// ITEM DESCRIPTION BOX STUFF
cache_key_t const guiItemDescBox{ INTERFACEDIR "/infobox.sti" };
// Map-screen item description box: iteminfoc.sti, or on the wide strategic
// screen the wider iteminfoc_wide.sti reaching across the free space up to
// MBS (falls back to iteminfoc.sti until delivered) -- see
// GetWideStrategicAsset() in MapScreen.cc. Only the background changes; every
// slot/button/text inside keeps its current position.
#define MAP_ITEMDESC_BOX_FILE      INTERFACEDIR "/iteminfoc.sti"
#define MAP_ITEMDESC_BOX_WIDE_FILE INTERFACEDIR "/iteminfoc_wide.sti"
cache_key_t GetMapItemDescBoxFilename()
{
	// 1366x768, normal merc inventory: non-weapon items, money and weapons
	// have their own boxes -- see gMapItemDescLayout
	switch (gMapItemDescLayout)
	{
		case MAP_ITEMDESC_LAYOUT_ITEMS: return gfMapItemDescArialBox ? (gfTacBigItemsBox ? TAC_ITEMDESC_BOX_ITEMS_BIG_FILE : MAP_ITEMDESC_BOX_ITEMS_BIG_FILE) : MAP_ITEMDESC_BOX_ITEMS_FILE;
		case MAP_ITEMDESC_LAYOUT_MONEY: return gfMapItemDescArialBox ? (gfTacBigMoneyBox ? TAC_ITEMDESC_BOX_MONEY_BIG_FILE : MAP_ITEMDESC_BOX_MONEY_BIG_FILE) : MAP_ITEMDESC_BOX_MONEY_FILE;
		case MAP_ITEMDESC_LAYOUT_WEAPONS: return MAP_ITEMDESC_BOX_WEAPONS_FILE;
		case MAP_ITEMDESC_LAYOUT_WEAPONS_BIG: return gfTacBigWeaponBox ? TAC_ITEMDESC_BOX_WEAPONS_BIG_FILE : MAP_ITEMDESC_BOX_WEAPONS_BIG_FILE;
		default: break;
	}
	return GetWideStrategicAsset(MAP_ITEMDESC_BOX_WIDE_FILE, MAP_ITEMDESC_BOX_FILE);
}

// Width actually covered by the map-screen description box: the legacy
// MAP_ITEMDESC_WIDTH, or the real width of iteminfoc_wide.sti when that one
// is in use, so the box's background restore and its click-blocking mouse
// region cover the whole (wider) graphic. The _items/_money/_weapons boxes
// are exactly their graphic's size.
INT16 GetMapItemDescWidth()
{
	cache_key_t const box = GetMapItemDescBoxFilename();
	if (gMapItemDescLayout != MAP_ITEMDESC_LAYOUT_SHARED) return GetVObject(box)->SubregionProperties(0).usWidth;
	if (std::strcmp(box, MAP_ITEMDESC_BOX_WIDE_FILE) != 0) return MAP_ITEMDESC_WIDTH;
	return std::max<INT16>(MAP_ITEMDESC_WIDTH, GetVObject(box)->SubregionProperties(0).usWidth);
}

INT16 GetMapItemDescHeight()
{
	if (gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_SHARED) return MAP_ITEMDESC_HEIGHT;
	return GetVObject(GetMapItemDescBoxFilename())->SubregionProperties(0).usHeight;
}
// Same-sized alternate background for the tactical item description box when
// displaying money - doesn't need the room reserved for weapon stats/attachments.
cache_key_t const guiMoneyItemDescBox{ INTERFACEDIR "/Infobox_money.sti" };
// Tactical item description box background for every non-weapon,
// non-money item class (armour, ammo, medkits, keys, misc, ...) -- see
// g_generic_item_attachment_info below for its (4-slot) attachment layout.
cache_key_t const guiGenericItemDescBox{ INTERFACEDIR "/Infobox_items.sti" };

cache_key_t const guiBullet{ INTERFACEDIR "/bullet.sti" };
cache_key_t const guiGoldKeyVO{ INTERFACEDIR "/gold_key_button.sti" };
// Same cache entry as guiSMBookmarksVO in Interface_Panels.cc (cache_key_t
// is just the path string -- GetVObject()/BltVideoObject() share one
// loaded copy across translation units).
cache_key_t const guiSMBookmarksVO{ INTERFACEDIR "/inventory_bottom_panel_bookmarks.sti" };
// Attachment slot frame, drawn per-slot in RenderItemDescriptionBox() on the
// tactical screen -- Infobox.sti/Infobox_money.sti no longer have it baked
// into their own art, so fHideEmptyAttachmentSlots can hide it per-slot.
cache_key_t const guiAttachmentSlotFrameVO{ INTERFACEDIR "/attachment_slot_frame.sti" };
}
static SGPVObject *guiItemGraphic;
static UINT8 guiItemGraphicIndex;
BOOLEAN gfInItemDescBox = FALSE;
BOOLEAN fHideEmptyAttachmentSlots = FALSE;
// The same option for the map screen's weapon box (iteminfoc_weapons), with
// its checkbox on the box itself -- independent of the tactical one, off by
// default.
static BOOLEAN      gfMapHideEmptyAttachmentSlots = FALSE;
static GUIButtonRef giMapHideEmptySlotsCheckbox;
static UINT32 guiCurrentItemDescriptionScreen=0;

static MapItemDescLayout GetMapItemDescLayout(OBJECTTYPE const& o)
{
	if (!g_ui.isExtraWideStrategicScreen()) return MAP_ITEMDESC_LAYOUT_SHARED;

	if (IsMapInvBigImages() && GCM->getItem(o.usItem)->isWeapon())
	{
		// "Show Large Icons": weapons have a big box of their own. Items
		// and money have none yet and use their normal ones below.
		cache_key_t const big = MAP_ITEMDESC_BOX_WEAPONS_BIG_FILE;
		return FirstUsableInterfaceAsset({ big, MAP_ITEMDESC_BOX_FILE }) == big ? MAP_ITEMDESC_LAYOUT_WEAPONS_BIG : MAP_ITEMDESC_LAYOUT_SHARED;
	}

	MapItemDescLayout layout = MAP_ITEMDESC_LAYOUT_ITEMS;
	cache_key_t       box    = MAP_ITEMDESC_BOX_ITEMS_FILE;
	if (o.usItem == MONEY)
	{
		layout = MAP_ITEMDESC_LAYOUT_MONEY;
		box    = MAP_ITEMDESC_BOX_MONEY_FILE;
	}
	else if (GCM->getItem(o.usItem)->isWeapon())
	{
		layout = MAP_ITEMDESC_LAYOUT_WEAPONS;
		box    = MAP_ITEMDESC_BOX_WEAPONS_FILE;
	}
	if (FirstUsableInterfaceAsset({ box, MAP_ITEMDESC_BOX_FILE }) != box) return MAP_ITEMDESC_LAYOUT_SHARED;
	return layout;
}

// The open box is the map screen's weapon box for "Show Large Icons"
// (iteminfoc_weapons_big) -- or its tactical twin, see gfTacBigWeaponBox.
static bool IsMapBigWeaponBox()
{
	return gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_WEAPONS_BIG;
}


// The tactical weapon box of the 1366x768 SM panel (UILayout::isBigSMPanel()):
// infobox_weapons_big_1366x768.png (1188x492), the map's big weapon box
// (iteminfoc_weapons_big, 773x635) with its blocks moved -- so it is drawn
// with that box's layout (MAP_ITEMDESC_LAYOUT_WEAPONS_BIG and its tables),
// every position moved with the block it is in (TacBigWeaponShift()).
// Set per box in InternalInitItemDescriptionBox() (gfTacBigWeaponBox).

// Where a point of the map's big weapon box went on the tactical one
// (infobox_weapons_big_1366x768.png, 1366x511): its top part (picture,
// ammo, status) 107 px right and 29 lower, the Base / Prone / Per Aim box 24
// right and 29 lower, the left stats frame (x < 330, y 318-547)
// to the bottom right, the right one to the top right, the name line (y
// 547-577, x >= 330) to the bottom left; the description lines (y >= 578) and
// the attachment slots have places of their own (TB(), g_tac_big_weapon_slots).
struct BoxShift { INT16 dx; INT16 dy; };
static BoxShift TacBigWeaponShift(INT32 const x, INT32 const y)
{
	if (!gfTacBigWeaponBox)   return BoxShift{ 0, 0 };
	if (y < 80 && x < 140)    return BoxShift{ 24, 29 };   // the Base / Prone / Per Aim box
	if (y < 318)              return BoxShift{ 107, 29 };
	if (y >= 578)             return BoxShift{ 33, -149 };
	if (y >= 547 && x >= 330) return BoxShift{ -71, -174 };
	if (x < 330)              return BoxShift{ 938, -57 };
	return BoxShift{ 560, -298 };
}

// The tactical big box's attachment slots, numbered as the map's (same item
// and bar boxes, g_map_attachment_info_weapons_big): the channel left of each
// field and its top.
static SGPPoint const g_tac_big_weapon_slots[MAX_ATTACHMENTS] =
{
	{ 335,  39 }, // 1
	{ 418,  39 }, // 2
	{ 578,  39 }, // 3
	{ 784,  39 }, // 4
	{  53, 133 }, // 5
	{ 194, 133 }, // 6
	{ 335, 133 }, // 7
	{ 711,  96 }, // 8
	{ 711, 171 }, // 9
	{ 784, 171 }, // 10
	{ 335, 228 }, // 11
	{ 418, 228 }, // 12
	{ 501, 228 }, // 13
	{ 628, 228 }, // 14
	{ 711, 228 }, // 15
	{ 335, 285 }, // 16
	{ 418, 285 }, // 17
	{ 628, 285 }, // 18
	{ 711, 285 }, // 19
	{ 784, 285 }, // 20
};

// an attachment slot's place on the open box
static SGPPoint AttachmentSlotPos(SGPPoint const* const slots, INT32 const i)
{
	return gfTacBigWeaponBox ? g_tac_big_weapon_slots[i] : slots[i];
}

static SGPBox TB(SGPBox b)
{
	// the tactical box's description field is wider (793x38 inside, from
	// 56,445): the description line, and pros and cons side by side
	if (gfTacBigWeaponBox && b.y >= 578)
	{
		if (b.x == 350) return SGPBox{ 436, 463, 403, b.h }; // cons
		if (b.w == 312) return SGPBox{  66, 463, 363, b.h }; // pros
		return SGPBox{ 66, 440, 773, b.h };                  // description
	}
	// the name line: 5 px more room on both sides (name left, calibre and
	// class right aligned)
	if (gfTacBigWeaponBox && b.x == 347 && b.y == 557) return SGPBox{ 281, 383, 343, b.h };
	BoxShift const d = TacBigWeaponShift(b.x, b.y);
	b.x += d.dx;
	b.y += d.dy;
	return b;
}

static SGPPoint TB(SGPPoint p)
{
	BoxShift const d = TacBigWeaponShift(p.iX, p.iY);
	p.iX += d.dx;
	p.iY += d.dy;
	return p;
}

// The open box is one of the map screen's own weapon boxes (iteminfoc_weapons,
// iteminfoc_weapons_big): they show everything the tactical Infobox.sti
// does, the tactical way.
static bool IsMapWeaponBox()
{
	return gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_WEAPONS || IsMapBigWeaponBox();
}

// info_bil.sti (money buttons and their frame) of the open description box.
static cache_key_t GetMoneyGraphicsFilename()
{
	if (gMapItemDescLayout != MAP_ITEMDESC_LAYOUT_MONEY) return MONEY_GRAPHICS_FILE;
	return FirstUsableInterfaceAsset({ MAP_MONEY_GRAPHICS_FILE, MONEY_GRAPHICS_FILE });
}

// The map-screen position matching gMapItemDescLayout.
template<typename T> static T const& PickMapItemDesc(T const& shared, T const& items, T const& money, T const& weapons, T const& weapons_big, T const& items_big, T const& money_big)
{
	switch (gMapItemDescLayout)
	{
		case MAP_ITEMDESC_LAYOUT_ITEMS: return gfMapItemDescArialBox ? items_big : items;
		case MAP_ITEMDESC_LAYOUT_MONEY: return gfMapItemDescArialBox ? money_big : money;
		case MAP_ITEMDESC_LAYOUT_WEAPONS: return weapons;
		case MAP_ITEMDESC_LAYOUT_WEAPONS_BIG: return weapons_big;
		default:                        return shared;
	}
}

// iteminfoc_money_big_1366x768.png has the arrangement of
// iteminfoc_items_big: its picture, status bar, name (the total amount) and
// description are where the items box has them.
template<typename T> static T const& PickMapItemDesc(T const& shared, T const& items, T const& money, T const& weapons, T const& weapons_big, T const& items_big)
{
	return PickMapItemDesc(shared, items, money, weapons, weapons_big, items_big, items_big);
}
OBJECTTYPE *gpItemDescObject = NULL;
static BOOLEAN gfItemDescObjectIsAttachment = FALSE;
static ST::string gzItemName;
static ST::string gzItemDesc;
static ST::string gzItemPros;
static ST::string gzItemCons;
static INT16 gsInvDescX;
static INT16 gsInvDescY;
static UINT8 gubItemDescStatusIndex;
// The item the box was laid out for (ReloadItemDesc()). On the map screen
// things go on around the open box -- transfers, the sector inventory -- and
// may take the item away or put another in its place: the box then closes,
// see RenderItemDescriptionBox().
static UINT16 gusItemDescShownItem = NOTHING;
// DeleteItemDescriptionBox() is only closing the map screen's box so that it
// can open again, laid out anew, on the same item -- see
// CloseMapItemDescBoxForRelayout().
static BOOLEAN gfItemDescBoxRelayout = FALSE;
static BUTTON_PICS *giItemDescAmmoButtonImages;
static GUIButtonRef giItemDescAmmoButton;
static SOLDIERTYPE *gpItemDescSoldier;
static BOOLEAN fItemDescDelete = FALSE;

// Animation surface currently loaded for the merc-preview picture on
// Infobox.sti (weapon only) -- see gMercPreviewFrames/GetMercPreviewFrame()
// below. INVALID_ANIMATION_SURFACE when nothing is loaded (money/generic
// items, map screen, or a non-merc body type never load one).
static UINT16 gusMercPreviewAnimSurface = INVALID_ANIMATION_SURFACE;
// the preview took its own usage of the surface; when the merc already held one
// (often: the preview is his current animation) it must not be given back, or
// the surface is unloaded under his feet and he freezes
static bool   gfMercPreviewSurfaceLocked = false;

// Extra frames, after DeleteItemDescriptionBox() closes the weapon box, to
// keep re-flipping the box's last on-screen rectangle to the screen -- see
// TickItemDescCloseCleanup() below.
static UINT32 gsItemDescCloseCleanupFrames = 0;
static INT16  gsItemDescCloseCleanupX;
static INT16  gsItemDescCloseCleanupY;
static INT16  gsItemDescCloseCleanupW;
static INT16  gsItemDescCloseCleanupH;

// The map screen's counterpart: frames, after the box closes there, to keep
// redrawing the left column (RenderItemDescriptionBox() runs every frame
// there), so that buttons of the box drawn once more after the redraw that
// DeleteItemDescriptionBox() asks for do not stay on the screen.
static UINT32 gsMapItemDescCloseCleanupFrames = 0;

MOUSE_REGION gItemDescAttachmentRegions[MAX_ATTACHMENTS];
static MOUSE_REGION gProsAndConsRegions[2];

static GUIButtonRef guiMoneyButtonBtn[NUM_MONEY_BUTTONS];
static BUTTON_PICS *guiMoneyButtonImage;
static BUTTON_PICS *guiMoneyDoneButtonImage;

static UINT16 gusOriginalAttachItem[MAX_ATTACHMENTS];
static UINT8 gbOriginalAttachStatus[MAX_ATTACHMENTS];
static SOLDIERTYPE *gpAttachSoldier;
// Slot the player is attaching to, stashed across the "permanent attachment?"
// confirmation dialog (PermanantAttachmentMessageBoxCallBack runs later, with
// no way to pass it a parameter directly).
static INT8 gbPendingAttachPos = NO_SLOT;

#define gMoneyButtonLoc				(g_ui.m_moneyButtonLoc)
#define gMapMoneyButtonLoc				(g_ui.m_MoneyButtonLocMap)

//static const MoneyLoc gMoneyButtonOffsets[] = { { 0, 0 }, { 34, 0 }, { 0, 32 }, { 34, 32 }, { 8, 22 } };

/*
static const MoneyLoc gMoneyButtonOffsets[] = { { 215, 49 }, // 1000
												{ 177, 49 }, // 100
												{ 177, 74 }, // 10
												{ 215, 74 }, // Done
												{ 187, 100 } }; // Separate
*/

// NEW POSITION
static const MoneyLoc gMoneyButtonOffsets[] = { { 35, 38 }, // 1000
												{ 73, 16 }, // 100
												{ 35, 16 }, // 10
												{ 73, 38 }, // Done
												{ 45, 67 } }; // Separate

// iteminfoc_money on the map screen, 1366x768 base, normal merc inventory
// -- see g_map_itemdesc_desc_box_money above. Unlike the shared ones these
// are relative to the box itself (gsInvDescX/Y), see GetMoneyButtonPos().
static const MoneyLoc gMapMoneyButtonOffsets_Money[] = { { 164, 48 }, // 1000
												{ 202, 26 }, // 100
												{ 164, 26 }, // 10
												{ 202, 48 }, // Done
												{ 174, 77 } }; // Separate

// iteminfoc_money_big ("Show Large Icons", 604x340): $10, $100, $1000 and
// Done centred in the four boxes under the picture (62x51 inside, from
// 19/92/166/239,146), no info_bil frame; the 'Separate' text centred in the
// right small dark box (MAP_MONEY_BIG_SEPARATE_BOX), its entry here unused.
static const MoneyLoc gMapMoneyButtonOffsets_MoneyBig[] = { { 180, 161 }, // 1000
												{ 107, 161 }, // 100
												{  33, 161 }, // 10
												{ 254, 161 }, // Done
												{ 467, 151 } }; // Separate
static const SGPBox MAP_MONEY_BIG_SEPARATE_BOX = { 462, 146, 128, 83 }; // iteminfoc_money_big / right small dark box

// Screen position of money button `i` (M_1000..M_DONE, 4 = the 'Separate'
// text) of the open description box.
static SGPPoint GetMoneyButtonPos(INT32 const i)
{
	SGPPoint pos;
	if (gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_MONEY)
	{
		MoneyLoc const* const loc = gfMapItemDescArialBox ? gMapMoneyButtonOffsets_MoneyBig : gMapMoneyButtonOffsets_Money;
		pos.set(gsInvDescX + loc[i].x, gsInvDescY + loc[i].y);
	}
	else
	{
		MoneyLoc const& loc = guiCurrentItemDescriptionScreen == MAP_SCREEN ? gMapMoneyButtonLoc : gMoneyButtonLoc;
		pos.set(loc.x + gMoneyButtonOffsets[i].x, loc.y + gMoneyButtonOffsets[i].y);
	}
	return pos;
}

// number of keys on keyring, temp for now
#define NUMBER_KEYS_ON_KEYRING				28
#define KEY_RING_ROW_WIDTH				7
#define MAP_KEY_RING_ROW_WIDTH				4

// ITEM STACK POPUP STUFF
static BOOLEAN gfInItemStackPopup = FALSE;
static SGPVObject* guiItemPopupBoxes;
static OBJECTTYPE* gpItemPopupObject;
static INT16 gsItemPopupX;
static INT16 gsItemPopupY;
// Was a literal 8 -- MAX_OBJECTS_PER_SLOT itself, spelled out by hand
// instead of using the macro. Harmless while every item's own ubPerPocket
// (game data) stayed <= 8, but InitItemStackPopup() below builds one region
// per unit up to ItemSlotLimit()'s result with no clamp of its own, so this
// must track MAX_OBJECTS_PER_SLOT (now raised past 8) or a
// higher-capacity pocket item overflows this array.
static MOUSE_REGION gItemPopupRegions[MAX_OBJECTS_PER_SLOT];
static MOUSE_REGION gKeyRingRegions[NUMBER_KEYS_ON_KEYRING];
BOOLEAN gfInKeyRingPopup = FALSE;
static UINT8 gubNumItemPopups = 0;
static MOUSE_REGION gItemPopupRegion;
static INT16 gsItemPopupInvX;
static INT16 gsItemPopupInvY;
static INT16 gsItemPopupInvWidth;
static INT16 gsItemPopupInvHeight;

static INT16 gsKeyRingPopupInvX;
static INT16 gsKeyRingPopupInvY;
static INT16 gsKeyRingPopupInvWidth;
static INT16 gsKeyRingPopupInvHeight;


SOLDIERTYPE *gpItemPopupSoldier;

// inventory description done button for mapscreen
GUIButtonRef giMapInvDescButton;

struct INV_DESC_STATS
{
	INT16 sX;
	INT16 sY;
	INT16 sValDx;
};

static INV_DESC_STATS TB(INV_DESC_STATS t)
{
	BoxShift const d = TacBigWeaponShift(t.sX, t.sY);
	t.sX += d.dx;
	t.sY += d.dy;
	return t;
}


static const SGPBox gMapDescNameBox = {  7, 65, 247, 8 };
// 1366x768 base, normal merc inventory -- see g_map_itemdesc_desc_box_items above.
static const SGPBox gMapDescNameBox_Items = { 19, 99, 334, 8 }; // iteminfoc_items / item name, money amount of gold and silver
static const SGPBox gMapDescNameBox_Money = { 19, 99, 334, 8 }; // iteminfoc_money / item name, total amount
static const SGPBox gMapDescNameBox_Weapons = { 14, 203, 375, 8 }; // iteminfoc_weapons / item name, calibre and weapon class
// iteminfoc_items_big_1366x768.png (604x340), the items box of "Show Large
// Icons": every _items_big map table in this file. Fields measured on the
// graphic; texts 5 px from the walls of their field.
static const SGPBox gMapDescNameBox_Items_Big = { 19, 213, 276, 11 }; // iteminfoc_items_big / item name (286x21 inside, from 14,208), money amount of gold and silver
static const SGPBox gMapDescNameBox_Weapons_Big = { 347, 557, 353, 13 }; // iteminfoc_weapons_big / item name, calibre and weapon class (FONT14ARIAL), in the short box under BALLISTICS (363x21 inside, from 342,552)

// Done button (itemdescdonebutton.sti) of the map screen's box. Only on
// ItemInfoC.sti: the _items/_money/_weapons boxes have none, like the tactical ones
// (a click on the box closes it, money has its own Done).
static const SGPPoint g_map_itemdesc_done_btn = { 204, 107 };

// static const SGPBox gDescNameBox    = { 11, 110, 301, 10 };
// Item name (and, in the branches that reuse the same box, weapon class/
// ammo type text and the money amount). One independent copy per tactical
// item description box background -- see fIsWeapon/fIsMoney in
// RenderItemDescriptionBox(). All three currently equal (preserving today's
// shared position); tune independently as needed.
static const SGPBox gDescNameBox       = { 14, 203, 375, 8 };  // Infobox.sti (weapon) / item name, ammo name, item class name
static const SGPBox gDescNameBox_Money = { 19, 99, 334, 8 };  // Infobox_money.sti / item name, ammo name, item class name
static const SGPBox gDescNameBox_Items = { 19, 99, 334, 8 };  // Infobox_items.sti / item name, ammo name, item class name

static const SGPBox g_desc_item_box_map = { 23, 10, 124, 48 };
static const SGPBox g_desc_item_box_map_items = { 21, 13, 133, 69 }; // iteminfoc_items / main picture
static const SGPBox g_desc_item_box_map_money = { 21, 13, 133, 69 }; // iteminfoc_money / main picture
static const SGPBox g_desc_item_box_map_weapons = { 163,  46, 133, 69 }; // iteminfoc_weapons / main picture, ammo button
static const SGPBox g_desc_item_box_map_items_big = { 20, 14, 280, 125 }; // iteminfoc_items_big / main picture
static const SGPBox g_desc_item_box_map_weapons_big = { 318,  68, 280, 125 }; // iteminfoc_weapons_big / main picture (280x125, for the big weapon pictures to come), ammo button

// static const SGPBox g_desc_item_box     = { 23, 230, 220, 10 };
// Main item picture: size and position within the box background. One
// independent copy per tactical item description box background --
// see fIsWeapon/fIsMoney in RenderItemDescriptionBox(). All three
// currently equal (preserving today's on-screen position); tune
// independently as needed.
static const SGPBox g_desc_item_box     = { 163,  46, 133, 69 }; // Infobox.sti (weapon) / main picture
static const SGPBox g_desc_item_box_money = { 21,  13, 133, 69 }; // Infobox_money.sti / main picture
static const SGPBox g_desc_item_box_items = { 21,  13, 133, 69 }; // Infobox_items.sti / main picture

// Merc preview picture on Infobox.sti (weapon only): a single static frame
// of the selected merc's OWN body animation, picked by body type + category
// of weapon currently held in HANDPOS (knife / short (one-handed gun) /
// long (two-handed gun)) -- see GunLaserScopeBonus-style precedent in
// DetermineSoldierAnimationSurface() (Animation_Control.cc) for the same
// gun-class/isTwoHanded() branching this reuses. Position is a placeholder
// -- tune independently once visible in-game.
//
// Deactivated (kept in source, per request) -- gates loading in
// InternalInitItemDescriptionBox() and drawing in
// RenderItemDescriptionBox() below. Flip to true to re-enable.
static const bool ENABLE_MERC_PREVIEW_PICTURE = false;

#define MERC_PREVIEW_X    (13 + gsInvDescX)
#define MERC_PREVIEW_Y    (139 + gsInvDescY)

enum MercPreviewWeaponCategory
{
	MERC_PREVIEW_KNIFE = 0,
	MERC_PREVIEW_SHORT_GUN,
	MERC_PREVIEW_LONG_GUN,
	NUM_MERC_PREVIEW_CATEGORIES
};

struct MercPreviewFrame
{
	UINT16 usAnimSurface;
	UINT16 usImageIndex;
};

// [ubBodyType][MercPreviewWeaponCategory] -- ubBodyType only ever indexes
// REGMALE/BIGMALE/STOCKYMALE/REGFEMALE here (IS_MERC_BODY_TYPE() below
// guards every other value, e.g. creatures/robots). STOCKYMALE has no
// animation files of its own, same as everywhere else in
// gAnimSurfaceDatabase -- it reuses REGMALE's, per user confirmation.
static const MercPreviewFrame gMercPreviewFrames[TOTALBODYTYPES][NUM_MERC_PREVIEW_CATEGORIES] =
{
	/* REGMALE    */ { { RGMBREATHKNIFE,     35 }, { RGMPISTOLBREATH, 8 }, { RGM_LOOK,  42 } },
	/* BIGMALE    */ { { BGMBREATHKNIFE,     35 }, { BGMPISTOLBREATH, 8 }, { BGMTHREATENSTAND, 42 } },
	/* STOCKYMALE */ { { RGMBREATHKNIFE,     35 }, { RGMPISTOLBREATH, 8 }, { RGM_LOOK,  42 } }, // fallback = REGMALE
	/* REGFEMALE  */ { { RGFBREATHKNIFE,     23 }, { RGFHANDGUN_1H,  38 }, { RGFSTANDAIM, 38 } },
};

// Which of the 3 preview categories the merc's currently held item
// (HANDPOS -- NOT the item whose description is being shown, which may be
// a different item in the same merc's inventory) falls into. Mirrors the
// gun-class / isTwoHanded() branching DetermineSoldierAnimationSurface()
// (Animation_Control.cc) already uses to pick a weapon-appropriate
// animation surface for real gameplay animations.
static MercPreviewWeaponCategory GetMercPreviewWeaponCategory(SOLDIERTYPE const& s)
{
	UINT16      const  usHeldItem = s.inv[HANDPOS].usItem;
	ItemModel const* const item       = GCM->getItem(usHeldItem);
	bool        const  isGun       = item->getItemClass() == IC_GUN || item->getItemClass() == IC_LAUNCHER;

	if (!isGun || usHeldItem == ROCKET_LAUNCHER) return MERC_PREVIEW_KNIFE;
	return item->isTwoHanded() ? MERC_PREVIEW_LONG_GUN : MERC_PREVIEW_SHORT_GUN;
}

// Resolves the merc-preview frame for the given soldier, or returns false
// (out left untouched) for non-merc body types (creatures/robots), which
// gMercPreviewFrames has no data for.
static bool GetMercPreviewFrame(SOLDIERTYPE const& s, MercPreviewFrame* const out)
{
	if (!IS_MERC_BODY_TYPE(&s)) return false;
	*out = gMercPreviewFrames[s.ubBodyType][GetMercPreviewWeaponCategory(s)];
	return true;
}

static const INV_DESC_STATS gWeaponStats[] =

/*{
	{ 202, 25, 83 },
	{ 202, 15, 83 },
	{ 265, 40, 20 },
	{ 202, 40, 32 },
	{ 202, 50, 32 },
	{ 265, 50, 20 },
	{ 234, 50,  0 },
	{ 290, 50,  0 }
};*/


// NEW POSITION
{
	{ 410, 220, 87 },  // Weight
	{ 410, 204, 87 },  // Status
	//{ 410, 204, 87 },  // Weight
	//{ 410, 248, 87 },  // Status
	{ 488, 14, 14 },   // Range
	{ 410, 14, 31 },   // Damage
	{ 410, 43, 31 },   // AP single
	{ 426, 72, 15 },   // AP burst
	{ 446, 43,  0 },   // = single
	{ 446, 72,  0 },   // = burst

	// Previously-hidden stats, shown in the extra room made by the enlarged
	// item description box. Only used when !in_map (gMapWeaponStats has no
	// matching entries for these indices).
	{ 470,  43, 39 },  // [8]  Ready time ("First Aim:")
	{ 410, 264, 87 },  // [9]  Reliability
	{ 410, 106, 39 },  // [10] Burst penalty
	{ 410, 280, 87 },  // [11] Repair ease
	{ 410, 164, 87 },  // [12] Attack volume
	{ 410, 236, 87 },  // [13] Deadliness
	//{ 410, 220, 87 },  // [13] Deadliness
	{ 410, 176, 87 },  // [14] Hit volume
	{ 410, 135, 87 },  // [15] Reload AP cost

	// "=" signs for the two other AP-cost stats (Ready time, Reload), same
	// convention as the existing single/burst-AP "=" signs above (ids[6]/[7]).
	{ 521,  43,  0 },  // [16] = (Ready time): no longer drawn
	{ 521,  43,  0 },  // [17] = (Reload AP cost): no longer drawn
	//{ 488, 135,  0 },  // [17] = (Reload AP cost): no longer drawn

	// Purely decorative labels — cosmetic only, no associated value, not
	// tied to any real stat. Kept translatable via gWeaponStatsDesc[15]/[16].
	{ 410,  72,  0 },  // [18] "AP:" (decorative)
	{ 410,  89,  0 },  // [19] "Rounds" (decorative)

	// Combined LASERSCOPE aim bonus + merc's live CROUCH-stance bonus +
	// merc's live roof/elevation bonus (see GunLaserScopeBonus() +
	// GunCrouchStanceBonus() + GunRoofBonus()). Label and value always
	// shown (0 when none apply).
	{ 14, 14, 48 },  // [20] "Base:" — LASERSCOPE aim bonus + CROUCH stance bonus + roof bonus

	// Combined LASERSCOPE + BIPOD + PRONE stance + roof/elevation aim
	// bonus (see GunLaserScopeBonus() + GunBipodDisplayBonus() +
	// GunProneStanceBonus() + GunRoofBonus()). Label and value always shown.
	{ 14, 26, 48 }, // [21] "Prone:" — Base: + BIPOD display bonus + PRONE stance bonus + roof bonus

	// Standalone SNIPERSCOPE display bonus (see GunSniperScopeDisplayBonus()).
	// Label and value always shown; never summed with Base:/Prone:.
	{ 14, 38, 48 }  // [22] "Per Aim:"
};

// Weight/Status (and, for keys, the sector-found/date-found box) label+value
// positions for Infobox_items.sti (every non-weapon, non-money item class --
// the "else" branch in RenderItemDescriptionBox() that used to reuse
// gWeaponStats[0]/[1]/[3] directly). Independent of gWeaponStats
// (Infobox.sti) and of the map screen's gMapWeaponStats/gMapGenericItemStats
// -- tune independently as needed.
static const INV_DESC_STATS gGenericItemStats[] =
{
	{ 273, 117, 87 },  // [0] Weight
	{ 273, 100, 87 },  // [1] Status / ammo amount
	{ 0,   0,   0  },  // [2] unused in this branch -- kept only so [3] lines up
	{ 273,  19, 31 },  // [3] Key description box (sector found / date found)
};


// displayed AFTER the mass/weight/"Kg" line
static const INV_DESC_STATS gMoneyStats[] =
{
	/*
	{ 202, 14, 78 },
	{ 212, 25, 78 },
	{ 202, 40, 78 },
	{ 212, 51, 78 }
	*/
	
	// NEW POSITION
	{ 251, 19, 78  }, // current
	{ 251, 31, 79 },  // balance
	{ 251, 59, 78  }, // amount to
	{ 251, 71, 79 }   // withdraw
};

// displayed AFTER the mass/weight/"Kg" line
static const INV_DESC_STATS gMapMoneyStats[] =
{
	{ 51,  97, 45 },
	{ 61, 107, 75 },
	{ 51, 125, 45 },
	{ 61, 135, 70 }
};

// iteminfoc_money on the map screen, 1366x768 base, normal merc inventory
// -- see g_map_itemdesc_desc_box_money above. The values are printed the
// tactical way (right-aligned in ITEM_STATS_WIDTH), not the shared map way.
static const INV_DESC_STATS gMapMoneyStats_Money[] =
{
	{ 251, 19, 78 }, // current / amount
	{ 251, 31, 79 }, // balance / remaining
	{ 251, 59, 78 }, // amount to / amount
	{ 251, 71, 79 }  // withdraw / to split
};

// iteminfoc_money_big ("Show Large Icons"): the two two-line stats in the
// left small dark box (128x83 inside, from 312,146), 5 px from its walls, 15
// px between a stat's lines; the values right-aligned in
// MAP_MONEY_BIG_VALUE_WIDTH on the second line.
#define MAP_MONEY_BIG_VALUE_WIDTH 118
static const INV_DESC_STATS gMapMoneyStats_MoneyBig[] =
{
	{ 317, 151, 0 }, // current / amount
	{ 317, 166, 0 }, // balance / remaining
	{ 317, 199, 0 }, // amount to / amount
	{ 317, 214, 0 }  // withdraw / to split
};

// iteminfoc_items on the map screen, 1366x768 base, normal merc inventory:
// Weight/Status (and the key box) of every non-weapon, non-money item --
// the map's counterpart of gGenericItemStats, same indices.
// iteminfoc_items_big: Status / amount and Weight in the left small dark box
// (128x83 inside, from 312,146), a key's two lines in the right one (128x83
// inside, from 462,146); 5 px from the walls, 10 px between the lines. Same
// indices as gMapGenericItemStats.
#define BIG_ITEMS_KEY_LINE_PITCH 21 // the key's two lines: FONT12ARIAL's 11 px + 10
#define BIG_ITEMS_KEY_WIDTH      118 // the key's values, right-aligned in the box
static const INV_DESC_STATS gMapGenericItemStats_Big[] =
{
	{ 317, 172, 91 },  // [0] Weight
	{ 317, 151, 91 },  // [1] Status / ammo amount
	{   0,   0,  0 },  // [2] unused in this branch -- kept only so [3] lines up
	{ 467, 151, 31 },  // [3] Key description box (sector found / date found)
};

static const INV_DESC_STATS gMapGenericItemStats[] =
{
	{ 273, 117, 87 },  // [0] Weight
	{ 273, 100, 87 },  // [1] Status / ammo amount
	{ 0,   0,   0  },  // [2] unused in this branch -- kept only so [3] lines up
	{ 273,  19, 31 },  // [3] Key description box (sector found / date found)
};


// iteminfoc_weapons on the map screen, 1366x768 base, normal merc inventory
// -- the map's counterpart of gWeaponStats, same indices, all of them shown.
static const INV_DESC_STATS gMapWeaponBoxStats[] =
{
	{ 410, 220, 87 },  // Weight
	{ 410, 204, 87 },  // Status
	{ 488, 14, 14 },   // Range
	{ 410, 14, 31 },   // Damage
	{ 410, 43, 31 },   // AP single
	{ 426, 72, 15 },   // AP burst
	{ 446, 43,  0 },   // = single
	{ 446, 72,  0 },   // = burst

	// The stats the shared map box (gMapWeaponStats) does not show.
	{ 470,  43, 39 },  // [8]  Ready time ("First Aim:")
	{ 410, 264, 87 },  // [9]  Reliability
	{ 410, 106, 39 },  // [10] Burst penalty
	{ 410, 280, 87 },  // [11] Repair ease
	{ 410, 164, 87 },  // [12] Attack volume
	{ 410, 236, 87 },  // [13] Deadliness
	{ 410, 176, 87 },  // [14] Hit volume
	{ 410, 135, 87 },  // [15] Reload AP cost

	// "=" signs for the two other AP-cost stats (Ready time, Reload), same
	// convention as the existing single/burst-AP "=" signs above (ids[6]/[7]).
	{ 521,  43,  0 },  // [16] = (Ready time): no longer drawn
	{ 521,  43,  0 },  // [17] = (Reload AP cost): no longer drawn

	// Purely decorative labels — cosmetic only, no associated value, not
	// tied to any real stat. Kept translatable via gWeaponStatsDesc[15]/[16].
	{ 410,  72,  0 },  // [18] "AP:" (decorative)
	{ 410,  89,  0 },  // [19] "Rounds" (decorative)

	// Combined LASERSCOPE aim bonus + merc's live CROUCH-stance bonus +
	// merc's live roof/elevation bonus (see GunLaserScopeBonus() +
	// GunCrouchStanceBonus() + GunRoofBonus()). Label and value always
	// shown (0 when none apply).
	{ 14, 14, 48 },  // [20] "Base:" — LASERSCOPE aim bonus + CROUCH stance bonus + roof bonus

	// Combined LASERSCOPE + BIPOD + PRONE stance + roof/elevation aim
	// bonus (see GunLaserScopeBonus() + GunBipodDisplayBonus() +
	// GunProneStanceBonus() + GunRoofBonus()). Label and value always shown.
	{ 14, 26, 48 }, // [21] "Prone:" — Base: + BIPOD display bonus + PRONE stance bonus + roof bonus

	// Standalone SNIPERSCOPE display bonus (see GunSniperScopeDisplayBonus()).
	// Label and value always shown; never summed with Base:/Prone:.
	{ 14, 38, 48 }  // [22] "Per Aim:"
};

// iteminfoc_weapons_big -- the map's counterpart of gWeaponStats, same
// indices, laid out in the fields of the graphic's sections (FONT12ARIAL).
// The right edge of a value is sX + sValDx + ITEM_STATS_WIDTH.
static const INV_DESC_STATS gMapBigWeaponBoxStats[] =
{
	{  44, 425, 82 },  // [0]  Weight        -- Physical
	{  44, 405, 82 },  // [1]  Status        -- Physical
	{ 449, 405, 44 },  // [2]  Range         -- Fire Power
	{ 363, 405, 27 },  // [3]  Damage        -- Fire Power
	{ 363, 425, 27 },  // [4]  AP single     -- Single AP (no title)
	{   0,   0,  0 },  // [5]  AP burst: not on this box, see Short / Long / Full
	{ 393, 425,  0 },  // [6]  = single      -- Single AP (no title)
	{   0,   0,  0 },  // [7]  = burst: not on this box
	{ 449, 425, 44 },  // [8]  Ready time ("First Aim:") -- Single AP (no title)
	{ 188, 475, 81 },  // [9]  Reliability   -- Handling
	{ 555, 490, 78 },  // [10] Burst penalty -- Burst AP, on the line of Shot Volume
	{ 188, 455, 81 },  // [11] Repair ease   -- Handling
	{ 363, 490, 130 }, // [12] Attack volume -- Loudness
	{  44, 490, 82 },  // [13] Deadliness    -- Valuation
	{ 363, 510, 130 }, // [14] Hit volume    -- Loudness
	{ 188, 405, 81 },  // [15] Reload AP cost -- Handling
	{ 474, 463,  0 },  // [16] = (Ready time): no longer drawn
	{ 474, 463,  0 },  // [17] = (Reload AP cost): no longer drawn
	{   0,   0,  0 },  // [18] "AP:" (decorative): not on this box
	{   0,   0,  0 },  // [19] "Rounds" (decorative): not on this box
	{  37,  18, 64 },  // [20] "Base:"
	{  37,  38, 64 },  // [21] "Prone:"
	{  37,  58, 64 }   // [22] "Per Aim:"
};

// Texts only iteminfoc_weapons_big shows -- indexes gzMapBigWeaponBoxStrings.
enum
{
	BIGW_TXT_BALLISTICS = 0, // headers, FONT14HUMANIST, the colour of the stat values
	BIGW_TXT_MAINTENANCE,
	BIGW_TXT_FIRE_POWER,     // section titles, FONT14ARIAL, the colour of the stat labels
	BIGW_TXT_SINGLE_AP,
	BIGW_TXT_BURST_AP,
	BIGW_TXT_LOUDNESS,
	BIGW_TXT_PHYSICAL,
	BIGW_TXT_HANDLING,
	BIGW_TXT_VALUATION,
	NUM_BIGW_TXT_TITLES,

	BIGW_TXT_SHORT = NUM_BIGW_TXT_TITLES, // labels with a value, FONT12ARIAL
	BIGW_TXT_LONG,
	BIGW_TXT_FULL,
	BIGW_TXT_BRST_PEN_PRONE,
	BIGW_TXT_UNJAM,
	BIGW_TXT_VALUE,
	NUM_BIGW_TXT_VALUES_END,

	BIGW_TXT_ROUNDS = NUM_BIGW_TXT_VALUES_END, // "Rounds:", next to Short / Long / Full
	BIGW_TXT_UP_TO                             // "Up to %d", the rounds of Full
};

// The bars of the graphic the headers and section titles are centred in.
static const SGPBox gMapBigWeaponBoxTitles[NUM_BIGW_TXT_TITLES] =
{
	/* BIGW_TXT_BALLISTICS  */ { 353, 339, 386, 22 },
	/* BIGW_TXT_MAINTENANCE */ {  34, 338, 271, 24 },
	/* BIGW_TXT_FIRE_POWER  */ { 353, 377, 176, 23 },
	/* BIGW_TXT_SINGLE_AP   */ { 353, 439, 176, 23 }, // not drawn
	/* BIGW_TXT_BURST_AP    */ { 545, 377, 194, 23 },
	/* BIGW_TXT_LOUDNESS    */ { 353, 462, 176, 23 },
	/* BIGW_TXT_PHYSICAL    */ {  34, 377, 128, 23 },
	/* BIGW_TXT_HANDLING    */ { 178, 377, 127, 23 },
	/* BIGW_TXT_VALUATION   */ {  34, 462, 128, 23 },
};

// Label position (sX, sY) and the distance to the right-aligned value
// (sValDx), like gMapBigWeaponBoxStats. Indexed from BIGW_TXT_SHORT.
static const INV_DESC_STATS gMapBigWeaponBoxTexts[NUM_BIGW_TXT_VALUES_END - BIGW_TXT_SHORT] =
{
	/* BIGW_TXT_SHORT          */ { 555, 405, 21 }, // Burst AP
	/* BIGW_TXT_LONG           */ { 555, 426, 21 }, // Burst AP
	/* BIGW_TXT_FULL           */ { 555, 447, 21 }, // Burst AP
	/* BIGW_TXT_BRST_PEN_PRONE */ { 555, 510, 78 }, // Burst AP, on the line of Hit Volume
	/* BIGW_TXT_UNJAM          */ { 188, 425, 81 }, // Handling
	/* BIGW_TXT_VALUE          */ {  44, 510, 82 }, // Valuation
};

// "Rounds:" on the lines of Short / Long / Full: label position and the
// distance to the bullet icons (Short, Long) or the "Up to" text (Full).
static const INV_DESC_STATS gMapBigWeaponBoxRounds[3] =
{
	/* Short */ { 637, 405, 46 },
	/* Long  */ { 637, 426, 46 },
	/* Full  */ { 637, 447, 46 },
};


static const INV_DESC_STATS gMapWeaponStats[] =
{
	{  72 - 20,      20 + 80 + 8, 86 },
	{  72 - 20,      20 + 80 - 2, 86 },
	{  72 - 20 + 65, 40 + 80 + 4, 21 },
	{  72 - 20,      40 + 80 + 4, 30 },
	{  72 - 20,      53 + 80 + 2, 30 },
	{  72 - 20 + 65, 53 + 80 + 2, 25 },
	{  86,           53 + 80 + 2,  0 },
	{ 145,           53 + 80 + 2,  0 }
};


struct AttachmentGfxInfo
{
	SGPBox   item_box; // Bounding box of the item relative to a slot
	SGPBox   bar_box;  // Bounding box of the status bar relative to a slot
	SGPPoint slot[MAX_ATTACHMENTS];
};

// Placeholder 4-column x 5-row grid (20 slots), top-left slot at (5,5), same
// column/row spacing as the old 2-column layout (34px / 26px). This is only a
// starting point for laying out the new INFOBOX.STI/ITEMINFOC.STI graphics -
// every coordinate below is expected to be repositioned by hand to match the
// final artwork.
static const AttachmentGfxInfo g_attachment_info =
{
	
//	{ 7, 0, 28, 25 },  // x position of picture attachment vs status bar ### y position of picture attachment vs status bar ### lenght and hight of attachment box and shadow when you can not attach something
//	{ 2, 2,  2, 22 }, // x position of status bar ### y position of status bar ### width and hight of status bar
	
// NEW POSITION
	{ 8, 1, 36, 31 },
    { 1, 1,  2, 31 },
	
	{ // x position of status bar and picture attachment ### y positon of status bar and picture attachment
		{   155,   8 }, {  204,   8 }, {  252,   8 }, { 350,   8 }, // First row
		{   8,  65 }, {  57,  65 }, {  106,  65 }, { 301,  46 },	{   301,  84 }, {  350,  65 }, // Second row
		{  105,  122 }, { 154,  122 },	{   203,  122 }, {  252,  122 }, {  349,  122 }, // Third row
		{ 57,  160 }, {   106, 160 }, { 154, 160 }, {  252, 160 }, {  301, 160 },  // Fourth row
	}
};

static const AttachmentGfxInfo g_map_attachment_info =
{
	{ 8, 0, 36, 25 },
	{ 1, 1,  2, 31 },
	{
		{   5,   5 }, {  39,   5 }, {  73,   5 }, { 107,   5 },
		{   5,  31 }, {  39,  31 }, {  73,  31 }, { 107,  31 },
		{   5,  57 }, {  39,  57 }, {  73,  57 }, { 107,  57 },
		{   5,  83 }, {  39,  83 }, {  73,  83 }, { 107,  83 },
		{   5, 109 }, {  39, 109 }, {  73, 109 }, { 107, 109 },
	}
};

// iteminfoc_items on the map screen, 1366x768 base, normal merc inventory:
// attachment slots of every non-weapon item -- see
// g_map_itemdesc_desc_box_items above. The same 2x2 grid as
// g_generic_item_attachment_info below (GENERIC_ITEM_ATTACHMENT_SLOTS).
// Money has no attachments.
static const AttachmentGfxInfo g_map_attachment_info_items =
{
	{ 8, 1, 36, 31 },   // item_box: x, y, w, h
	{ 1, 1, 2, 31 },   // bar_box
	{
		{ 164, 13 }, { 164, 51 },
		{ 213, 13 }, { 213, 51 },
		// remaining 16 unused
	}
};

// Placeholder 2x2 grid, 4 attachment slots for every non-weapon, non-money
// item class on the tactical screen (Infobox_items.sti) -- independent of
// g_attachment_info (weapons). Reposition by hand to match the final
// artwork, same as the other layouts above.
static const AttachmentGfxInfo g_generic_item_attachment_info =
{
	{ 8, 1, 36, 31 },   // item_box: x, y, w, h
	{ 1, 1, 2, 31 },   // bar_box
	{
		{ 164, 13 }, { 164, 51 },
		{ 213, 13 }, { 213, 51 },
		// remaining 16 unused -- the loop is capped to 4 for this class
	}
};
// iteminfoc_weapons on the map screen, 1366x768 base, normal merc inventory:
// the 20 attachment slots of a weapon -- the map's counterpart of
// g_attachment_info. Their frames are drawn per slot, like on Infobox.sti.
static const AttachmentGfxInfo g_map_attachment_info_weapons =
{
	{ 8, 1, 36, 31 },
	{ 1, 1,  2, 31 },
	{
		{ 155,   8 }, { 204,   8 }, { 252,   8 }, { 350,   8 }, // First row
		{   8,  65 }, {  57,  65 }, { 106,  65 }, { 301,  46 }, { 301,  84 }, { 350,  65 }, // Second row
		{ 105, 122 }, { 154, 122 }, { 203, 122 }, { 252, 122 }, { 349, 122 }, // Third row
		{  57, 160 }, { 106, 160 }, { 154, 160 }, { 252, 160 }, { 301, 160 }, // Fourth row
	}
};

// iteminfoc_weapons_big: the 20 attachment slots, numbered as in the
// graphic's layout sheet. A slot's origin is the left line of its status
// bar channel, 6 px left of its field; every slot is 70x50 for now (some
// fields of the graphic are wider or narrower), its status bar 2x50. The
// graphic has the slot frames in it, so none are drawn per slot and there
// is no "hide empty" option.
static const AttachmentGfxInfo g_map_attachment_info_weapons_big =
{
	{ 7, 1, 70, 50 },   // item_box: inside the field's 1 px border
	{ 1, 1,  2, 50 },   // bar_box: inside the channel
	{
		{ 228,  10 }, // 1
		{ 311,  10 }, // 2
		{ 471,  10 }, // 3
		{ 677,  10 }, // 4
		{  29, 104 }, // 5
		{ 170, 104 }, // 6
		{ 228, 199 }, // 7
		{ 604,  67 }, // 8
		{ 604, 142 }, // 9
		{ 677, 142 }, // 10
		{ 311, 199 }, // 11
		{ 394, 199 }, // 12
		{ 521, 199 }, // 13
		{ 604, 199 }, // 14
		{ 677, 256 }, // 15
		{ 145, 256 }, // 16
		{ 228, 256 }, // 17
		{ 311, 256 }, // 18
		{ 521, 256 }, // 19
		{ 604, 256 }, // 20
	}
};

// iteminfoc_items_big: three 70x50 attachment slots, with their 2x50 status
// bars in the channel left of each -- the same slot geometry as
// g_map_attachment_info_weapons_big.
#define BIG_ITEMS_ATTACHMENT_SLOTS 3
static const AttachmentGfxInfo g_map_attachment_info_items_big =
{
	{ 7, 1, 70, 50 },
	{ 1, 1,  2, 50 },
	{
		{  13, 145 }, { 118, 145 }, { 223, 145 },
		// remaining 17 unused
	}
};

// Slots of g_generic_item_attachment_info actually laid out: the only ones
// drawn and the only ones given a mouse region.
#define GENERIC_ITEM_ATTACHMENT_SLOTS 4


static BOOLEAN gfItemDescHelpTextOffset = FALSE;


// A STRUCT USED INTERNALLY FOR INV SLOT REGIONS
struct INV_REGIONS
{
	INT16 w;
	INT16 h;
};

// ARRAY FOR INV PANEL INTERFACE ITEM POSITIONS (sX,sY get set via InitInvSlotInterface() )
static INV_REGIONS const gSMInvData[] =
{
#define M(w, h) { w, h }
	M(HEAD_INV_SLOT_WIDTH, HEAD_INV_SLOT_HEIGHT), // HELMETPOS
	M(VEST_INV_SLOT_WIDTH, VEST_INV_SLOT_HEIGHT), // VESTPOS
	M(LEGS_INV_SLOT_WIDTH, LEGS_INV_SLOT_HEIGHT), // LEGPOS,
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // HEAD1POS
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // HEAD2POS
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // HEAD3POS
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // HEAD4POS
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // HANDPOS,
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // SECONDHANDPOS
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK1
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK2
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK3
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK4
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK5
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK6
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK7
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK8
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK9
	M(BIG_INV_SLOT_WIDTH,  BIG_INV_SLOT_HEIGHT ), // BIGPOCK10
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK1
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK2
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK3
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK4
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK5
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK6
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK7
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK8
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK9
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK10
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK11
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK12
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK13
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK14
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK15
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK16
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK17
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK18
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  ), // SMALLPOCK19
	M(SM_INV_SLOT_WIDTH,   SM_INV_SLOT_HEIGHT  )  // SMALLPOCK20
#undef M
};

// Slot sizes on the strategic map in "Show Large Icons" mode (see
// m_invSlotPositionMapBig[] in UILayout.cc and IsMapInvBigImages(),
// MapScreen.cc) -- independent of gSMInvData[] above, which the tactical
// panel and the map's normal mode share. Sized to mapinv_big_1280_720/768.sti's
// slots, one width/height pair per slot kind.
#define MAP_BIG_HEAD_SLOT_WIDTH			62	// HEAD1-4POS (face items)
#define MAP_BIG_HEAD_SLOT_HEIGHT		50
#define MAP_BIG_ARMOUR_SLOT_WIDTH		62	// HELMETPOS, VESTPOS, LEGPOS
#define MAP_BIG_ARMOUR_SLOT_HEIGHT		50
#define MAP_BIG_HAND_SLOT_WIDTH			122	// HANDPOS, SECONDHANDPOS
#define MAP_BIG_HAND_SLOT_HEIGHT		50
#define MAP_BIG_BIGPOCK_SLOT_WIDTH		122	// BIGPOCK1-10POS
#define MAP_BIG_BIGPOCK_SLOT_HEIGHT		50
#define MAP_BIG_SMALLPOCK_SLOT_WIDTH		72	// SMALLPOCK1-20POS
#define MAP_BIG_SMALLPOCK_SLOT_HEIGHT		50

static INV_REGIONS const gSMInvDataMapBig[] =
{
#define M(w, h) { w, h }
#define HEAD      M(MAP_BIG_HEAD_SLOT_WIDTH,      MAP_BIG_HEAD_SLOT_HEIGHT     )
#define ARMOUR    M(MAP_BIG_ARMOUR_SLOT_WIDTH,    MAP_BIG_ARMOUR_SLOT_HEIGHT   )
#define HAND      M(MAP_BIG_HAND_SLOT_WIDTH,      MAP_BIG_HAND_SLOT_HEIGHT     )
#define BIGPOCK   M(MAP_BIG_BIGPOCK_SLOT_WIDTH,   MAP_BIG_BIGPOCK_SLOT_HEIGHT  )
#define SMALLPOCK M(MAP_BIG_SMALLPOCK_SLOT_WIDTH, MAP_BIG_SMALLPOCK_SLOT_HEIGHT)
	ARMOUR,    // HELMETPOS
	ARMOUR,    // VESTPOS
	ARMOUR,    // LEGPOS
	HEAD,      // HEAD1POS
	HEAD,      // HEAD2POS
	HEAD,      // HEAD3POS
	HEAD,      // HEAD4POS
	HAND,      // HANDPOS
	HAND,      // SECONDHANDPOS
	BIGPOCK,   // BIGPOCK1
	BIGPOCK,   // BIGPOCK2
	BIGPOCK,   // BIGPOCK3
	BIGPOCK,   // BIGPOCK4
	BIGPOCK,   // BIGPOCK5
	BIGPOCK,   // BIGPOCK6
	BIGPOCK,   // BIGPOCK7
	BIGPOCK,   // BIGPOCK8
	BIGPOCK,   // BIGPOCK9
	BIGPOCK,   // BIGPOCK10
	SMALLPOCK, // SMALLPOCK1
	SMALLPOCK, // SMALLPOCK2
	SMALLPOCK, // SMALLPOCK3
	SMALLPOCK, // SMALLPOCK4
	SMALLPOCK, // SMALLPOCK5
	SMALLPOCK, // SMALLPOCK6
	SMALLPOCK, // SMALLPOCK7
	SMALLPOCK, // SMALLPOCK8
	SMALLPOCK, // SMALLPOCK9
	SMALLPOCK, // SMALLPOCK10
	SMALLPOCK, // SMALLPOCK11
	SMALLPOCK, // SMALLPOCK12
	SMALLPOCK, // SMALLPOCK13
	SMALLPOCK, // SMALLPOCK14
	SMALLPOCK, // SMALLPOCK15
	SMALLPOCK, // SMALLPOCK16
	SMALLPOCK, // SMALLPOCK17
	SMALLPOCK, // SMALLPOCK18
	SMALLPOCK, // SMALLPOCK19
	SMALLPOCK  // SMALLPOCK20
#undef SMALLPOCK
#undef BIGPOCK
#undef HAND
#undef ARMOUR
#undef HEAD
#undef M
};
static_assert(std::size(gSMInvDataMapBig) == NUM_INV_SLOTS, "one gSMInvDataMapBig entry per inventory slot");

// Slot sizes on the 1366x768 interface's tactical SM panel
// (inventory_bottom_panel_big_1366x768.png, UILayout::isBigSMPanel(); places in
// UILayout.cc's m_invSlotPositionTac): its slot boxes, the big pictures in them.
#define TAC_BIG_HEAD_SLOT_WIDTH			64	// HEAD1-4POS, HELMETPOS, VESTPOS, LEGPOS
#define TAC_BIG_SLOT_HEIGHT			52
#define TAC_BIG_HAND_SLOT_WIDTH			124	// HANDPOS, SECONDHANDPOS, BIGPOCK3-10POS
#define TAC_BIG_WIDE_SLOT_WIDTH			157	// BIGPOCK1-2POS
#define TAC_BIG_SMALLPOCK_SLOT_WIDTH		74	// SMALLPOCK1-20POS
// the item's status bar: in the 2 px black line 5 px left of the slot
#define TAC_BIG_BAR_DX				5
#define TAC_BIG_BAR_DY				50
#define TAC_BIG_BAR_HEIGHT			50
#define TAC_BIG_BAR_WIDTH			2

static INV_REGIONS const gSMInvDataTacBig[] =
{
#define M(w, h) { w, h }
#define HEAD      M(TAC_BIG_HEAD_SLOT_WIDTH,      TAC_BIG_SLOT_HEIGHT)
#define HAND      M(TAC_BIG_HAND_SLOT_WIDTH,      TAC_BIG_SLOT_HEIGHT)
#define WIDE      M(TAC_BIG_WIDE_SLOT_WIDTH,      TAC_BIG_SLOT_HEIGHT)
#define SMALLPOCK M(TAC_BIG_SMALLPOCK_SLOT_WIDTH, TAC_BIG_SLOT_HEIGHT)
	HEAD, HEAD, HEAD,                     // HELMETPOS, VESTPOS, LEGPOS
	HEAD, HEAD, HEAD, HEAD,               // HEAD1-4POS
	HAND, HAND,                           // HANDPOS, SECONDHANDPOS
	WIDE, WIDE,                           // BIGPOCK1-2
	HAND, HAND, HAND, HAND,               // BIGPOCK3-6
	HAND, HAND, HAND, HAND,               // BIGPOCK7-10
	SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK,
	SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK,
	SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK,
	SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK, SMALLPOCK  // SMALLPOCK1-20
#undef SMALLPOCK
#undef WIDE
#undef HAND
#undef HEAD
#undef M
};
static_assert(std::size(gSMInvDataTacBig) == NUM_INV_SLOTS, "one gSMInvDataTacBig entry per inventory slot");

// Ammo left (bottom-left of the item picture) and stack count (bottom-right)
// in the "Show Large Icons" panel's slots (mapinv_big_1280_720/768.sti --
// one set for both): own font, fonts/font_mapinv_big_count.sti, and own
// positions, independent of every other inventory (ITEM_FONT; ammo at
// x+1 / 11 px above the bottom, count 4 px from the right / 10 px above the
// bottom -- the values these start from). Colours are indices into the new
// font's OWN palette: the ammo-type ones match tinyfont1.sti's except HP,
// whose index 24 is black there -- 203 (0,0,255) is the nearest blue.
#define MAP_BIG_COUNT_FONT              FONTMAPINVBIGCOUNT
#define MAP_BIG_AMMO_TEXT_X             1
#define MAP_BIG_AMMO_TEXT_FROM_BOTTOM   12
#define MAP_BIG_STACK_TEXT_FROM_RIGHT   3
#define MAP_BIG_STACK_TEXT_FROM_BOTTOM  12
#define MAP_BIG_COUNT_COL_AP            218
#define MAP_BIG_COUNT_COL_HP            203
#define MAP_BIG_COUNT_COL_BUCKSHOT      125
#define MAP_BIG_COUNT_COL_HE            75
#define MAP_BIG_COUNT_COL_HEAP          76
#define MAP_BIG_COUNT_COL_AMMO          FONT_WHITE
#define MAP_BIG_COUNT_COL_STACK         FONT_WHITE
#define MAP_BIG_COUNT_COL_JAMMED        FONT_MCOLOR_RED


struct REMOVE_MONEY
{
	UINT32 uiTotalAmount;
	UINT32 uiMoneyRemaining;
	UINT32 uiMoneyRemoving;
};
static REMOVE_MONEY gRemoveMoney;

static MOUSE_REGION gSMInvRegion[NUM_INV_SLOTS];
static MOUSE_REGION gKeyRingPanel;
static MOUSE_REGION gSMInvCamoRegion;
static INT8 gbCompatibleAmmo[NUM_INV_SLOTS];
INT8 gbInvalidPlacementSlot[ NUM_INV_SLOTS ];
static UINT16 us16BPPItemCyclePlacedItemColors[20];
static SGPVObject* guiBodyInvVO[4][2];

INT8 gbCompatibleApplyItem = FALSE;


static SGPVObject *guiMapInvSecondHandBlockout;
static SGPVObject *guiSecItemHiddenVO;
// Loaded on first use (only drawn in the map's "Show Large Icons" mode).
static cache_key_t const guiMapInvSecondHandBlockoutBig{ INTERFACEDIR "/secondary_gun_hidden_big.sti" };
static SGPVObject *guiSmallInventoryGraphicMissingSmallPocket;
static SGPVObject *guiSmallInventoryGraphicMissingBigPocket;
static std::map<ST::string, SGPVObject*> allInventoryGraphics;
// Sector-inventory "big images" toggle's own pre-cached BIGITEMS lookup --
// see GetSectorInventoryBigGraphicForItem()'s own comment (Interface_Items.h)
// for why this is separate from GetBigInventoryGraphicForItem()'s existing,
// deliberately-uncached, one-at-a-time path.
static std::map<ST::string, SGPVObject*> allSectorInventoryBigGraphics;
const ST::string guiBigInventoryGraphicMissingPath = "sti/interface/inventory/inventory-graphic-not-found-big.sti";

// The font pros / cons are measured in by AttemptToAddSubstring():
// ITEMDESC_FONT when NULL, FONT12ARIAL on the map's big weapon box.
static SGPFont gProsConsFont = NULL;

static BOOLEAN AttemptToAddSubstring(ST::string& zDest, const ST::string& zTemp, UINT32* puiStringLength, UINT32 uiPixLimit)
{
	UINT32 uiRequiredStringLength, uiTempStringLength;
	SGPFont const font = gProsConsFont ? gProsConsFont : ITEMDESC_FONT;

	uiTempStringLength = StringPixLength( zTemp, font );
	uiRequiredStringLength = *puiStringLength + uiTempStringLength;
	if (!zDest.empty())
	{
		uiRequiredStringLength += StringPixLength( COMMA_AND_SPACE, font );
	}
	if (uiRequiredStringLength < uiPixLimit)
	{
		if (!zDest.empty())
		{
			zDest += COMMA_AND_SPACE;
		}
		zDest += zTemp;
		*puiStringLength = uiRequiredStringLength;
		return( TRUE );
	}
	else
	{
		zDest += DOTDOTDOT;
		return( FALSE );
	}
}


static void GenerateProsString(ST::string& zItemPros, const OBJECTTYPE& o, UINT32 uiPixLimit)
{
	UINT32 uiStringLength = 0;
	ST::string zTemp;
	UINT16 usItem = o.usItem;

	zItemPros.clear();

	if (GCM->getItem(usItem)->getWeight() * 100 <= EXCEPTIONAL_WEIGHT)
	{
		zTemp = g_langRes->Message[STR_LIGHT];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GCM->getItem(usItem)->getPerPocket() >= 1) // fits in a small pocket
	{
		zTemp = g_langRes->Message[STR_SMALL];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GunRange(o) >= EXCEPTIONAL_RANGE)
	{
		zTemp = g_langRes->Message[STR_LONG_RANGE];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GCM->getWeapon(usItem)->ubImpact >= EXCEPTIONAL_DAMAGE)
	{
		zTemp = g_langRes->Message[STR_HIGH_DAMAGE];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (BaseAPsToShootOrStab(DEFAULT_APS, DEFAULT_AIMSKILL, *gpItemDescObject) <= EXCEPTIONAL_AP_COST)
	{
		zTemp = g_langRes->Message[STR_QUICK_FIRING];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GCM->getWeapon(usItem)->ubShotsPerShortBurst >= EXCEPTIONAL_BURST_SIZE || usItem == G11)
	{
		zTemp = g_langRes->Message[STR_FAST_BURST];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GCM->getWeapon(usItem)->ubMagSize > EXCEPTIONAL_MAGAZINE)
	{
		zTemp = g_langRes->Message[STR_LARGE_AMMO_CAPACITY];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if ( GCM->getItem(usItem)->getReliability() >= EXCEPTIONAL_RELIABILITY )
	{
		zTemp = g_langRes->Message[STR_RELIABLE];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if ( GCM->getItem(usItem)->getRepairEase() >= EXCEPTIONAL_REPAIR_EASE )
	{
		zTemp = g_langRes->Message[STR_EASY_TO_REPAIR];
		if ( ! AttemptToAddSubstring( zItemPros, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if ( zItemPros[0] == 0 )
	{
		// empty string, so display "None"
		if ( ! AttemptToAddSubstring( zItemPros, g_langRes->Message[ STR_NONE ], &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}
}


static void GenerateConsString(ST::string& zItemCons, const OBJECTTYPE& o, UINT32 uiPixLimit)
{
	UINT32 uiStringLength = 0;
	ST::string zTemp;
	UINT8 ubWeight;
	UINT16 usItem = o.usItem;

	zItemCons.clear();

	// calculate the weight of the item plus ammunition but not including any attachments
	ubWeight = GCM->getItem(usItem)->getWeight();
	if (GCM->getItem(usItem)->getItemClass() == IC_GUN)
	{
		ubWeight += GCM->getItem(o.usGunAmmoItem)->getWeight();
	}

	if (ubWeight >= BAD_WEIGHT)
	{
		zTemp = g_langRes->Message[STR_HEAVY];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GunRange(o) <= BAD_RANGE)
	{
		zTemp = g_langRes->Message[STR_SHORT_RANGE];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GCM->getWeapon(usItem)->ubImpact <= BAD_DAMAGE)
	{
		zTemp = g_langRes->Message[STR_LOW_DAMAGE];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (BaseAPsToShootOrStab(DEFAULT_APS, DEFAULT_AIMSKILL, *gpItemDescObject) >= BAD_AP_COST)
	{
		zTemp = g_langRes->Message[STR_SLOW_FIRING];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (!GCM->getWeapon(usItem)->canBurst())
	{
		zTemp = g_langRes->Message[STR_NO_BURST];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if (GCM->getWeapon(usItem)->ubMagSize < BAD_MAGAZINE)
	{
		zTemp = g_langRes->Message[STR_SMALL_AMMO_CAPACITY];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if ( GCM->getItem(usItem)->getReliability() <= BAD_RELIABILITY )
	{
		zTemp = g_langRes->Message[STR_UNRELIABLE];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}

	if ( GCM->getItem(usItem)->getRepairEase() <= BAD_REPAIR_EASE )
	{
		zTemp = g_langRes->Message[STR_HARD_TO_REPAIR];
		if ( ! AttemptToAddSubstring( zItemCons, zTemp, &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}


	if ( zItemCons[0] == 0 )
	{
		// empty string, so display "None"
		if ( ! AttemptToAddSubstring( zItemCons, g_langRes->Message[ STR_NONE ], &uiStringLength, uiPixLimit ) )
		{
			return;
		}
	}
}


void InitInvSlotInterface(INV_REGION_DESC const* const pRegionDesc,
	INV_REGION_DESC const* const pCamoRegion,
	MOUSE_CALLBACK INVMoveCallback, MOUSE_CALLBACK INVClickCallback,
	MOUSE_CALLBACK INVMoveCamoCallback, MOUSE_CALLBACK INVClickCamoCallback,
	bool const fMapBigImages, bool const fTacticalBigImages)
{
	INV_REGIONS const* const sizes =
		fMapBigImages      ? gSMInvDataMapBig :
		fTacticalBigImages ? gSMInvDataTacBig :
		gSMInvData;

	// Load all four body type images
	guiBodyInvVO[0][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male.sti");
	guiBodyInvVO[0][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male_h.sti");
	guiBodyInvVO[1][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_large_male.sti");
	guiBodyInvVO[1][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_large_male_h.sti");
	guiBodyInvVO[2][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male.sti");
	guiBodyInvVO[2][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male.sti");
	guiBodyInvVO[3][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_female.sti");
	guiBodyInvVO[3][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_female_h.sti");

	// Add camo region
	UINT16 const x = pCamoRegion->uX;
	UINT16 const y = pCamoRegion->uY;
	MSYS_DefineRegion(&gSMInvCamoRegion, x, y, x + CAMO_REGION_WIDTH,
		y + CAMO_REGION_HEIGHT,	MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
		std::move(INVMoveCamoCallback), std::move(INVClickCamoCallback));

	// Add regions for inventory slots
	for (INT32 i = 0; i != NUM_INV_SLOTS; ++i)
	{
		// Set inventory pocket coordinates from the table passed in
		INT16       const  x = pRegionDesc[i].uX;
		INT16       const  y = pRegionDesc[i].uY;
		INV_REGIONS const& r = sizes[i];
		MOUSE_REGION&      m = gSMInvRegion[i];
		MSYS_DefineRegion(&m, x, y, x + r.w, y + r.h,
			MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
			INVMoveCallback, INVClickCallback);
		MSYS_SetRegionUserData(&m, 0, i);
	}

	std::fill(std::begin(gbCompatibleAmmo), std::end(gbCompatibleAmmo), 0);
}


// Resets fSMKeyringIconPressed (Interface_Panels.h) if the mouse is dragged
// off the region while still held down -- POINTER_UP never reaches
// KeyRingItemPanelButtonCallback in that case (see
// MSYS_UpdateMouseRegion()'s g_clicked_region gating). Used by both the
// tactical and the map screen's keyring region.
static void KeyRingMoveCallback(MOUSE_REGION*, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		fSMKeyringIconPressed = FALSE;
		// The map's keyring icon is drawn with the rest of the inventory panel.
		if (guiCurrentScreen == MAP_SCREEN) fTeamPanelDirty = TRUE;
	}
}


void InitKeyRingInterface(MOUSE_CALLBACK KeyRingClickCallback)
{
	MSYS_DefineRegion(&gKeyRingPanel, KEYRING_X, KEYRING_Y,
		KEYRING_X + KEYRING_WIDTH, KEYRING_Y + KEYRING_HEIGHT,
		MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, KeyRingMoveCallback,
		std::move(KeyRingClickCallback));
	gKeyRingPanel.SetFastHelpText(TacticalStr[KEYRING_HELP_TEXT]);
}


void InitMapKeyRingInterface( MOUSE_CALLBACK KeyRingClickCallback )
{
	MSYS_DefineRegion(&gKeyRingPanel, MAP_KEYRING_X, MAP_KEYRING_Y,
		MAP_KEYRING_X + KEYRING_WIDTH, MAP_KEYRING_Y + KEYRING_HEIGHT,
		MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, KeyRingMoveCallback,
		std::move(KeyRingClickCallback));
	gKeyRingPanel.SetFastHelpText(TacticalStr[KEYRING_HELP_TEXT]);
}


static void EnableKeyRing(BOOLEAN fEnable)
{
	if ( fEnable )
	{
		gKeyRingPanel.Enable();
	}
	else
	{
		gKeyRingPanel.Disable();
	}
}


void ShutdownKeyRingInterface( void )
{
	MSYS_RemoveRegion( &gKeyRingPanel );
}

void DisableInvRegions( BOOLEAN fDisable )
{
	INT32 cnt;

	for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
	{
		if ( fDisable )
		{
			gSMInvRegion[cnt].Disable();
		}
		else
		{
			gSMInvRegion[cnt].Enable();
		}
	}

	if ( fDisable )
	{
		gSMInvCamoRegion.Disable();
		gSM_SELMERCMoneyRegion.Disable();
		EnableKeyRing( FALSE );
	}
	else
	{
		gSMInvCamoRegion.Enable();
		gSM_SELMERCMoneyRegion.Enable();
		EnableKeyRing( TRUE );
	}

}


void ShutdownInvSlotInterface()
{
	// Remove all body type panels
	for (SGPVObject* (*i)[2] = guiBodyInvVO; i != endof(guiBodyInvVO); ++i)
	{
		FOR_EACH(SGPVObject*, k, *i) DeleteVideoObject(*k);
	}

	RemoveVObject(guiGoldKeyVO);

	FOR_EACH(MOUSE_REGION, i, gSMInvRegion)
	{
		MSYS_RemoveRegion(&*i);
	}

	MSYS_RemoveRegion(&gSMInvCamoRegion);
}


void RenderInvBodyPanel(const SOLDIERTYPE* pSoldier, INT16 sX, INT16 sY)
{
	// Only the merc bodies have a figure (guiBodyInvVO) -- not vehicles, whose
	// cargo inventory opens here too
	if (!IS_MERC_BODY_TYPE(pSoldier)) return;

	// Blit body inv, based on body type
	INT8 bSubImageIndex = gbCompatibleApplyItem;

	BltVideoObject(guiSAVEBUFFER, guiBodyInvVO[pSoldier->ubBodyType][bSubImageIndex], 0, sX, sY);
}


static void INVRenderINVPanelItem(SOLDIERTYPE const& s, INT16 const pocket, DirtyLevel const dirty_level)
{
	guiCurrentItemDescriptionScreen = guiCurrentScreen;
	bool       const  in_map = guiCurrentScreen == MAP_SCREEN;
	OBJECTTYPE const& o      = s.inv[pocket];
	MOUSE_REGION&     r      = gSMInvRegion[pocket];

	bool   hatch_out = false;
	UINT16 outline   = SGP_TRANSPARENT;
	if (dirty_level == DIRTYLEVEL2)
	{
		ST::string buf = GetHelpTextForItem(o);
		r.SetFastHelpText(buf);

		// If it's the second hand and this hand cannot contain anything, remove the
		// second hand position graphic
		if (pocket == SECONDHANDPOS && GCM->getItem(s.inv[HANDPOS].usItem)->isTwoHanded())
		{
			// Checked once -- secondary_gun_hidden_big.sti is optional.
			static bool const big_cover_exists = GCM->doesGameResExists(INTERFACEDIR "/secondary_gun_hidden_big.sti");
			if (in_map && IsMapInvBigImages() && big_cover_exists)
			{
				INT32 const x = MAP_SCREEN_X + MAP_INV_2ND_GUN_COVER_BIG_X;
				INT32 const y = MAP_SCREEN_Y + MAP_INV_SHIFT_Y + MAP_INV_2ND_GUN_COVER_BIG_Y;
				BltVideoObject(guiSAVEBUFFER, guiMapInvSecondHandBlockoutBig, 0, x, y);
				RestoreExternBackgroundRect(x, y, MAP_INV_2ND_GUN_COVER_BIG_WIDTH, MAP_INV_2ND_GUN_COVER_BIG_HEIGHT);
			}
			else if (in_map)
			{
				INT32 const x = MAP_SCREEN_X + MAP_INV_2ND_GUN_COVER_X;
				INT32 const y = MAP_SCREEN_Y + MAP_INV_SHIFT_Y + MAP_INV_2ND_GUN_COVER_Y;
				BltVideoObject(guiSAVEBUFFER, guiMapInvSecondHandBlockout, 0, x, y);
				RestoreExternBackgroundRect(x, y, MAP_INV_2ND_GUN_COVER_WIDTH, MAP_INV_2ND_GUN_COVER_HEIGHT);
			}
			else
			{
				if (g_ui.isBigSMPanel())
				{
					// the 1366x768 panel: the second hand's slot shaded
					MOUSE_REGION const& h = gSMInvRegion[SECONDHANDPOS];
					guiSAVEBUFFER->ShadowRect(h.X(), h.Y(), h.X() + h.W(), h.Y() + h.H());
					RestoreExternBackgroundRect(h.X(), h.Y(), h.W(), h.H());
				}
				else
				{
					INT32 const x = INTERFACE_START_X + 294;
					INT32 const y = INV_INTERFACE_START_Y + 116;
					BltVideoObject(guiSAVEBUFFER, guiSecItemHiddenVO, 0, x, y);
					RestoreExternBackgroundRect(x, y, 75, 35);
				}
			}
		}

		// Check for compatibility with magazines
		if (gbCompatibleAmmo[pocket]) outline = Get16BPPColor(FROMRGB(255, 255, 255));
	}

	INT16 const x = r.X();
	INT16 const y = r.Y();

	// Now render as normal
	DirtyLevel const render_dirty_level =
		s.bNewItemCount[pocket] <= 0 ||
		gsCurInterfacePanel != SM_PANEL ||
		fInterfacePanelDirty == DIRTYLEVEL2 ? dirty_level :
		DIRTYLEVEL0; // We have a new item and we are in the right panel
	// "Show Large Icons" on the map draws the same BIGITEMS graphics as the
	// sector inventory's big mode (IsMapInvBigImages(), MapScreen.cc).
	// The 1366x768 tactical SM panel too (UILayout::isBigSMPanel()).
	BOOLEAN const big = in_map ? IsMapInvBigImages() : g_ui.isBigSMPanel();
	InvItemTextLayout big_text{};
	if (big) big_text = GetMapInvBigItemTextLayout();
	INVRenderItem(guiSAVEBUFFER, &s, o, x, y, r.W(), r.H(), render_dirty_level, 0, outline, big, big ? &big_text : NULL);

	if (gbInvalidPlacementSlot[pocket])
	{
		if (pocket != SECONDHANDPOS && !gfSMDisableForItems)
		{
			// We are in inv panel and our guy is not = cursor guy
			hatch_out = true;
		}
	}
	else
	{
		if (guiCurrentScreen == SHOPKEEPER_SCREEN &&
			ShouldSoldierDisplayHatchOnItem(s.ubProfile, pocket))
		{
			hatch_out = true;
		}
	}

	if (hatch_out)
	{
		SGPVSurface* const dst = in_map ? guiSAVEBUFFER : FRAME_BUFFER;
		DrawHatchOnInventory(dst,
			x + HATCH_OFFSET_X, y + HATCH_OFFSET_Y,
			r.W() + HATCH_WIDTH_DELTA, r.H() + HATCH_HEIGHT_DELTA);
	}

	if (o.usItem != NOTHING)
	{
		// Add item status bar
		if (in_map)
		{
			DrawItemUIBarEx(o, 0, x - MAP_INV_BAR_DX, y + MAP_INV_BAR_DY, MAP_ITEM_BAR_HEIGHT, Get16BPPColor(STATUS_BAR),
					Get16BPPColor(STATUS_BAR_SHADOW), guiSAVEBUFFER, MAP_ITEM_BAR_WIDTH);
		}
		else if (g_ui.isBigSMPanel())
		{
			DrawItemUIBarEx(o, 0, x - TAC_BIG_BAR_DX, y + TAC_BIG_BAR_DY, TAC_BIG_BAR_HEIGHT, Get16BPPColor(STATUS_BAR),
					Get16BPPColor(STATUS_BAR_SHADOW), guiSAVEBUFFER, TAC_BIG_BAR_WIDTH);
		}
		else
		{
			DrawItemUIBarEx(o, 0, x - INV_BAR_DX, y + INV_BAR_DY, ITEM_BAR_HEIGHT, Get16BPPColor(STATUS_BAR),
					Get16BPPColor(STATUS_BAR_SHADOW), guiSAVEBUFFER);
		}
	}
}


void HandleRenderInvSlots(SOLDIERTYPE const& s, DirtyLevel const dirty_level)
{
	if (InItemDescriptionBox() || InItemStackPopup() || InKeyRingPopup() || InStatsPopup() || InSkillsPopup()) return;

	for (INT32 i = 0; i != NUM_INV_SLOTS; ++i)
	{
		INVRenderINVPanelItem(s, i, dirty_level);
	}

	// Keyring icons are not drawn here on either screen: the tactical one by
	// RenderSMKeyringIcon() below (called unconditionally from
	// RenderSMPanel(), like RenderSMMoneyAndTrashIcons()), the map one by
	// RenderMapInvBookmarkIcons() in MapScreen.cc together with the map's
	// money and trash-can icons -- this function's early-return above skips
	// exactly while the keyring's own popup is open.
}


// Tactical panel's persistent "Key Ring Panel" bookmark icon
// (inventory_bottom_panel_bookmarks.sti, sub-images 17/18, 0-based 16/17 --
// see guiSMBookmarksVO/SM_KEYRING_ICON_READY in Interface_Panels.cc, source
// of the shared cache entry). Always shown regardless of whether the
// keyring actually holds a key (unlike the map screen's gold_key_button.sti
// highlight, HandleRenderInvSlots() above, left untouched). Called
// unconditionally from RenderSMPanel() -- like RenderSMMoneyAndTrashIcons()
// -- rather than from HandleRenderInvSlots(), so it doesn't disappear while
// InKeyRingPopup()/InItemStackPopup() is true. fSMKeyringIconPressed is set
// from KeyRingItemPanelButtonCallback (Interface_Panels.cc).
void RenderSMKeyringIcon()
{
	BltVideoObject(guiSAVEBUFFER, guiSMBookmarksVO, fSMKeyringIconPressed ? 17 : 16, KEYRING_X, KEYRING_Y);
	RestoreExternBackgroundRect(KEYRING_X, KEYRING_Y, KEYRING_WIDTH, KEYRING_HEIGHT);
}


static bool CompatibleAmmoForGun(const OBJECTTYPE* pTryObject, const OBJECTTYPE* pTestObject)
{
	if ( ( GCM->getItem(pTryObject->usItem)->isAmmo() ) )
	{
		return GCM->getWeapon( pTestObject->usItem )->matches(GCM->getItem(pTryObject->usItem)->asAmmo()->calibre);
	}
	return false;
}


static bool CompatibleGunForAmmo(const OBJECTTYPE* pTryObject, const OBJECTTYPE* pTestObject)
{
	if ( ( GCM->getItem(pTryObject->usItem)->isGun()) )
	{
		return GCM->getWeapon( pTryObject->usItem )->matches(GCM->getItem(pTestObject->usItem)->asAmmo()->calibre);
	}
	return false;
}


static BOOLEAN CompatibleItemForApplyingOnMerc(const OBJECTTYPE* const test)
{
	// ATE: If in mapscreen, return false always....
	if (fInMapMode) return FALSE;

	switch (test->usItem)
	{
		// ATE: Would be nice to have flag here to check for these types....
		case CAMOUFLAGEKIT:
		case ADRENALINE_BOOSTER:
		case REGEN_BOOSTER:
		case SYRINGE_3:
		case SYRINGE_4:
		case SYRINGE_5:
		case ALCOHOL:
		case WINE:
		case BEER:
		case CANTEEN:
		case JAR_ELIXIR:
			return TRUE;

		default: return FALSE;
	}
}


static BOOLEAN SoldierContainsAnyCompatibleStuff(const SOLDIERTYPE* const s, const OBJECTTYPE* const test)
{
	const UINT16 item_class = GCM->getItem(test->usItem)->getItemClass();
	if (item_class & IC_GUN)
	{
		CFOR_EACH_SOLDIER_INV_SLOT(i, *s)
		{
			if (CompatibleAmmoForGun(i, test)) return TRUE;
		}
	}
	else if (item_class & IC_AMMO)
	{
		CFOR_EACH_SOLDIER_INV_SLOT(i, *s)
		{
			if (CompatibleGunForAmmo(i, test)) return TRUE;
		}
	}

	// ATE: Put attachment checking here.....

	return FALSE;
}


void HandleAnyMercInSquadHasCompatibleStuff(const OBJECTTYPE* const o)
{
	const INT32 squad = CurrentSquad();
	if (squad == NUMBER_OF_SQUADS) return;

	FOR_EACH_IN_SQUAD(i, squad)
	{
		SOLDIERTYPE const* const s = *i;
		FACETYPE*          const f = s->face;
		Assert(f || s->uiStatusFlags & SOLDIER_VEHICLE);
		if (f == NULL) continue;

		if (o == NULL)
		{
			f->fCompatibleItems = FALSE;
		}
		else if (SoldierContainsAnyCompatibleStuff(s, o))
		{
			f->fCompatibleItems = TRUE;
		}
	}
}


BOOLEAN HandleCompatibleAmmoUIForMapScreen(const SOLDIERTYPE* pSoldier, INT32 bInvPos, BOOLEAN fOn, BOOLEAN fFromMerc)
{
	BOOLEAN fFound = FALSE;
	INT32 cnt;

	const OBJECTTYPE* pTestObject;
	if (!fFromMerc)
	{
		pTestObject = &( pInventoryPoolList[ bInvPos ].o );
	}
	else
	{
		if ( bInvPos == NO_SLOT )
		{
			pTestObject = NULL;
		}
		else
		{
			pTestObject = &(pSoldier->inv[ bInvPos ]);
		}
	}

	// ATE: If pTest object is NULL, test only for existence of syringes, etc...
	if ( pTestObject == NULL )
	{
		for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
		{
			if (CompatibleItemForApplyingOnMerc(&pSoldier->inv[cnt]))
			{
				if ( fOn != gbCompatibleAmmo[ cnt ] )
				{
					fFound = TRUE;
				}

				// IT's an OK calibere ammo, do something!
				// Render Item with specific color
				gbCompatibleAmmo[ cnt ] = fOn;

			}
		}


		if ( gpItemPointer != NULL )
		{
			if ( CompatibleItemForApplyingOnMerc( gpItemPointer ) )
			{
				// OK, Light up portrait as well.....
				if ( fOn )
				{
					gbCompatibleApplyItem = TRUE;
				}
				else
				{
					gbCompatibleApplyItem = FALSE;
				}

				fFound = TRUE;
			}
		}

		if ( fFound )
		{
			fInterfacePanelDirty = DIRTYLEVEL2;
		}

		return( fFound );
	}

	if ((!(GCM->getItem(pTestObject->usItem)->getFlags() & ITEM_HIDDEN_ADDON)))
	{
		// First test attachments, which almost any type of item can have....
		for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
		{
			OBJECTTYPE const& o = pSoldier->inv[cnt];

			if (GCM->getItem(o.usItem)->getFlags() & ITEM_HIDDEN_ADDON)
			{
				// don't consider for UI purposes
				continue;
			}

			UINT16 const a = o.usItem;
			UINT16 const b = pTestObject->usItem;
			if (ValidAttachment(a, b) ||
				ValidAttachment(b, a) ||
				ValidLaunchable(b, a) ||
				ValidLaunchable(a, b))
			{
				if ( fOn != gbCompatibleAmmo[ cnt ] )
				{
					fFound = TRUE;
				}

				// IT's an OK calibere ammo, do something!
				// Render Item with specific color
				gbCompatibleAmmo[ cnt ] = fOn;
			}
		}
	}


	if ( ( GCM->getItem(pTestObject->usItem)->isGun()) )
	{
		for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
		{
			if (CompatibleAmmoForGun(&pSoldier->inv[cnt], pTestObject))
			{
				if ( fOn != gbCompatibleAmmo[ cnt ] )
				{
					fFound = TRUE;
				}

				// IT's an OK calibere ammo, do something!
				// Render Item with specific color
				gbCompatibleAmmo[ cnt ] = fOn;
			}
		}
	}
	else if( ( GCM->getItem(pTestObject->usItem)->isAmmo() ) )
	{
		for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
		{
			if (CompatibleGunForAmmo(&pSoldier->inv[cnt], pTestObject))
			{
				if ( fOn != gbCompatibleAmmo[ cnt ] )
				{
					fFound = TRUE;
				}

				// IT's an OK calibere ammo, do something!
				// Render Item with specific color
				gbCompatibleAmmo[ cnt ] = fOn;

			}
		}
	}


	return( fFound );
}

BOOLEAN HandleCompatibleAmmoUIForMapInventory( SOLDIERTYPE *pSoldier, INT32 bInvPos, INT32 iStartSlotNumber, BOOLEAN fOn, BOOLEAN fFromMerc   )
{
	// CJC: ATE, needs fixing here!

	BOOLEAN fFound = FALSE;
	INT32 cnt;
	OBJECTTYPE *pObject, *pTestObject ;

	if (!fFromMerc)
	{
		pTestObject = &( pInventoryPoolList[ iStartSlotNumber + bInvPos ].o);
	}
	else
	{
		if ( bInvPos == NO_SLOT )
		{
			pTestObject = NULL;
		}
		else
		{
			pTestObject = &(pSoldier->inv[ bInvPos ]);
		}
	}

	// First test attachments, which almost any type of item can have....
	for ( cnt = 0; cnt < GetMapInventoryPoolPageSize(); cnt++ )
	{
		pObject = &( pInventoryPoolList[ iStartSlotNumber + cnt ].o );

		if ( GCM->getItem(pObject->usItem)->getFlags() & ITEM_HIDDEN_ADDON )
		{
			// don't consider for UI purposes
			continue;
		}

		if ( ValidAttachment( pObject->usItem, pTestObject->usItem ) ||
			ValidAttachment( pTestObject->usItem, pObject->usItem ) ||
			ValidLaunchable( pTestObject->usItem, pObject->usItem ) ||
			ValidLaunchable( pObject->usItem, pTestObject->usItem ) )
		{
			if ( fOn != fMapInventoryItemCompatable[ cnt ] )
			{
				fFound = TRUE;
			}

			// IT's an OK calibere ammo, do something!
			// Render Item with specific color
			fMapInventoryItemCompatable[ cnt ] = fOn;
		}
	}


	if( ( GCM->getItem(pTestObject->usItem)->isGun()) )
	{
		for ( cnt = 0; cnt < GetMapInventoryPoolPageSize(); cnt++ )
		{
			pObject = &( pInventoryPoolList[ iStartSlotNumber + cnt ].o );

			if ( CompatibleAmmoForGun( pObject, pTestObject ) )
			{
				if ( fOn != fMapInventoryItemCompatable[ cnt ] )
				{
					fFound = TRUE;
				}

				// IT's an OK calibere ammo, do something!
				// Render Item with specific color
				fMapInventoryItemCompatable[ cnt ] = fOn;
			}
		}
	}
	else if( ( GCM->getItem(pTestObject->usItem)->isAmmo() ) )
	{
		for ( cnt = 0; cnt < GetMapInventoryPoolPageSize(); cnt++ )
		{
			pObject = &( pInventoryPoolList[ iStartSlotNumber + cnt ].o );

			if ( CompatibleGunForAmmo( pObject, pTestObject ) )
			{
				if ( fOn != fMapInventoryItemCompatable[ cnt ] )
				{
					fFound = TRUE;
				}

				// IT's an OK calibere ammo, do something!
				// Render Item with specific color
				fMapInventoryItemCompatable[ cnt ] = fOn;

			}
		}
	}


	return( fFound );
}


BOOLEAN InternalHandleCompatibleAmmoUI(const SOLDIERTYPE* pSoldier, const OBJECTTYPE* pTestObject, BOOLEAN fOn)
{
	BOOLEAN fFound = FALSE;
	INT32 cnt;
	//BOOLEAN fFoundAttachment = FALSE;

	// ATE: If pTest object is NULL, test only for existence of syringes, etc...
	if ( pTestObject == NULL )
	{
		for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
		{
			if ( CompatibleItemForApplyingOnMerc(&pSoldier->inv[cnt]))
			{
				if ( fOn != gbCompatibleAmmo[ cnt ] )
				{
					fFound = TRUE;
				}

				// IT's an OK calibere ammo, do something!
				// Render Item with specific color
				gbCompatibleAmmo[ cnt ] = fOn;

			}
		}


		if ( gpItemPointer != NULL )
		{
			if ( CompatibleItemForApplyingOnMerc( gpItemPointer ) )
			{
				// OK, Light up portrait as well.....
				if ( fOn )
				{
					gbCompatibleApplyItem = TRUE;
				}
				else
				{
					gbCompatibleApplyItem = FALSE;
				}

				fFound = TRUE;
			}
		}

		if ( fFound )
		{
			fInterfacePanelDirty = DIRTYLEVEL2;
		}

		return( fFound );
	}

	// First test attachments, which almost any type of item can have....
	for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
	{
		OBJECTTYPE const& o = pSoldier->inv[cnt];

		if (GCM->getItem(o.usItem)->getFlags() & ITEM_HIDDEN_ADDON)
		{
			// don't consider for UI purposes
			continue;
		}

		UINT16 const a = o.usItem;
		UINT16 const b = pTestObject->usItem;
		if (ValidAttachment(a, b) ||
			ValidAttachment(b, a) ||
			ValidLaunchable(b, a) ||
			ValidLaunchable(a, b) )
		{
			//fFoundAttachment = TRUE;

			if ( fOn != gbCompatibleAmmo[ cnt ] )
			{
				fFound = TRUE;
			}

			// IT's an OK calibere ammo, do something!
			// Render Item with specific color
			gbCompatibleAmmo[ cnt ] = fOn;
		}
	}

	//if ( !fFoundAttachment )
	//{
		if( ( GCM->getItem(pTestObject->usItem)->isGun()) )
		{
			for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
			{
				if (CompatibleAmmoForGun(&pSoldier->inv[cnt], pTestObject))
				{
					if ( fOn != gbCompatibleAmmo[ cnt ] )
					{
						fFound = TRUE;
					}

					// IT's an OK calibere ammo, do something!
					// Render Item with specific color
					gbCompatibleAmmo[ cnt ] = fOn;
				}
			}
		}

		else if( ( GCM->getItem(pTestObject->usItem)->isAmmo() ) )
		{
			for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
			{
				if (CompatibleGunForAmmo(&pSoldier->inv[cnt], pTestObject))
				{
					if ( fOn != gbCompatibleAmmo[ cnt ] )
					{
						fFound = TRUE;
					}

					// IT's an OK calibere ammo, do something!
					// Render Item with specific color
					gbCompatibleAmmo[ cnt ] = fOn;

				}
			}
		}
		else if ( CompatibleItemForApplyingOnMerc( pTestObject ) )
		{
			//If we are currently NOT in the Shopkeeper interface
			if (guiCurrentScreen != SHOPKEEPER_SCREEN)
			{
				fFound = TRUE;
				gbCompatibleApplyItem = fOn;
			}
		}
	//}


	if ( !fFound )
	{
		for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
		{
			if ( gbCompatibleAmmo[ cnt ] )
			{
				fFound = TRUE;
				gbCompatibleAmmo[ cnt ] = FALSE;
			}

			if ( gbCompatibleApplyItem )
			{
				fFound = TRUE;
				gbCompatibleApplyItem = FALSE;
			}
		}
	}

	if ( fFound )
	{
		fInterfacePanelDirty = DIRTYLEVEL2;
	}

	return( fFound );

}


void ResetCompatibleItemArray()
{
	FOR_EACH(INT8, i, gbCompatibleAmmo) *i = FALSE;
}


BOOLEAN HandleCompatibleAmmoUI(const SOLDIERTYPE* pSoldier, INT8 bInvPos, BOOLEAN fOn)
{
	INT32 cnt;

	//if we are in the shopkeeper interface
	const OBJECTTYPE* pTestObject;
	if (guiCurrentScreen == SHOPKEEPER_SCREEN)
	{
		// if the inventory position is -1, this is a flag from the Shopkeeper interface screen
		//indicating that we are to use a different object to do the search
		if( bInvPos == -1 )
		{
			if( fOn )
			{
				if( gpHighLightedItemObject )
				{
					pTestObject = gpHighLightedItemObject;
					//gubSkiDirtyLevel = SKI_DIRTY_LEVEL2;
				}
				else
					return( FALSE );
			}
			else
			{
				gpHighLightedItemObject = NULL;

				for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
				{
					if ( gbCompatibleAmmo[ cnt ] )
					{
						gbCompatibleAmmo[ cnt ] = FALSE;
					}
				}

				gubSkiDirtyLevel = SKI_DIRTY_LEVEL1;
				return( TRUE );
			}
		}
		else
		{
			if( fOn )
			{
				pTestObject = &(pSoldier->inv[ bInvPos ]);
				gpHighLightedItemObject = pTestObject;
			}
			else
			{
				pTestObject = &(pSoldier->inv[ bInvPos ]);
				gpHighLightedItemObject = NULL;
				gubSkiDirtyLevel = SKI_DIRTY_LEVEL1;
			}
		}
	}
	else
	{
		//if( fOn )

		if ( bInvPos == NO_SLOT )
		{
			pTestObject = NULL;
		}
		else
		{
			pTestObject = &(pSoldier->inv[ bInvPos ]);
		}

	}

	return( InternalHandleCompatibleAmmoUI( pSoldier, pTestObject, fOn ) );

}


void HandleNewlyAddedItems(SOLDIERTYPE& s, DirtyLevel* const dirty_level)
{
	// If item description up, stop
	if (gfInItemDescBox) return;

	for (UINT32 i = 0; i != NUM_INV_SLOTS; ++i)
	{
		INT8& new_item_count = s.bNewItemCount[i];
		if (new_item_count == -2)
		{ // Stop
			*dirty_level   = DIRTYLEVEL2;
			new_item_count = 0;
		}

		if (new_item_count <= 0) continue;
		OBJECTTYPE const& o        = s.inv[i];
		if (o.usItem == NOTHING) continue;
		MOUSE_REGION const& r      = gSMInvRegion[i];
		UINT16       const  colour = us16BPPItemCyclePlacedItemColors[s.bNewItemCycleCount[i]];
		INVRenderItem(guiSAVEBUFFER, &s, o, r.X(), r.Y(), r.W(), r.H(), DIRTYLEVEL2, 0, colour);
	}
}


void CheckForAnyNewlyAddedItems(SOLDIERTYPE *pSoldier )
{
	UINT32 cnt;

	// OK, l0ok for any new...
	for ( cnt = 0; cnt < NUM_INV_SLOTS; cnt++ )
	{
		if ( pSoldier->bNewItemCount[ cnt ] == -1 )
		{
			pSoldier->bNewItemCount[ cnt ]	= NEW_ITEM_CYCLES - 1;
		}
	}
}


void DegradeNewlyAddedItems( )
{
	// If time done
	const UINT32 uiTime = GetJA2Clock();
	if (uiTime - guiNewlyPlacedItemTimer <= 100) return;

	guiNewlyPlacedItemTimer = uiTime;

	for (UINT32 cnt2 = 0; cnt2 < NUM_TEAM_SLOTS; ++cnt2)
	{
		SOLDIERTYPE* const s = GetPlayerFromInterfaceTeamSlot(cnt2);
		if (s == NULL) continue;

		for (UINT32 cnt = 0; cnt < NUM_INV_SLOTS; ++cnt)
		{
			if (s->bNewItemCount[cnt] == 0) continue;

			// Decrement all the time!
			s->bNewItemCycleCount[cnt]--;
			if (s->bNewItemCycleCount[cnt] != 0) continue;

			// OK, cycle down....
			s->bNewItemCount[cnt]--;
			if (s->bNewItemCount[cnt] == 0)
			{
				// Stop...
				s->bNewItemCount[cnt] = -2;
			}
			else
			{
				// Reset!
				s->bNewItemCycleCount[cnt] = NEW_ITEM_CYCLE_COUNT;
			}
		}
	}
}

UINT8 GetAttachmentHintColor(const OBJECTTYPE* o) {
	return FindAttachmentByClass(o, IC_LAUNCHER) == NO_SLOT ? FONT_GREEN : FONT_YELLOW;
}


InvItemTextLayout GetMapInvBigItemTextLayout()
{
	// Falls back to ITEM_FONT if the dedicated font lacks a glyph the
	// counts or the (localized) "JAMMED" strings need.
	bool const has_glyphs = FontHasGlyphsFor(MAP_BIG_COUNT_FONT,
		ST::string("0123456789") + TacticalStr[JAMMED_ITEM_STR] + TacticalStr[SHORT_JAMMED_GUN]);
	return InvItemTextLayout{
		has_glyphs ? MAP_BIG_COUNT_FONT : ITEM_FONT,
		MAP_BIG_AMMO_TEXT_X, MAP_BIG_AMMO_TEXT_FROM_BOTTOM,
		MAP_BIG_STACK_TEXT_FROM_RIGHT, MAP_BIG_STACK_TEXT_FROM_BOTTOM,
		MAP_BIG_COUNT_COL_AP, MAP_BIG_COUNT_COL_HP, MAP_BIG_COUNT_COL_BUCKSHOT,
		MAP_BIG_COUNT_COL_HE, MAP_BIG_COUNT_COL_HEAP, MAP_BIG_COUNT_COL_AMMO,
		MAP_BIG_COUNT_COL_STACK, MAP_BIG_COUNT_COL_JAMMED };
}


void INVRenderItem(SGPVSurface* const buffer, SOLDIERTYPE const* const s, OBJECTTYPE const& o, INT16 const sX, INT16 const sY, INT16 const sWidth, INT16 const sHeight, DirtyLevel const dirty_level, UINT8 const ubStatusIndex, INT16 const outline_colour, BOOLEAN const fUseSectorInventoryBigGraphic, InvItemTextLayout const* const layout)
{
	if (o.usItem    == NOTHING)     return;
	if (dirty_level == DIRTYLEVEL0) return;

	const ItemModel * item =
		ubStatusIndex < RENDER_ITEM_ATTACHMENT1 ? GCM->getItem(o.usItem) :
		GCM->getItem(o.usAttachItem[ubStatusIndex - RENDER_ITEM_ATTACHMENT1]);

	if (dirty_level == DIRTYLEVEL2)
	{
		// Center the object in the slot
		auto graphic = fUseSectorInventoryBigGraphic
			? GetSectorInventoryBigGraphicForItem(item)
			: GetSmallInventoryGraphicForItem(item);
		auto item_vo = graphic.first;
		auto gfx_idx = graphic.second;
		ETRLEObject const& e       = item_vo->SubregionProperties(gfx_idx);
		INT16       const  cx      = sX + (sWidth  - e.usWidth)  / 2 - e.sOffsetX;
		INT16       const  cy      = sY + (sHeight - e.usHeight) / 2 - e.sOffsetY;

		if (gamepolicy(f_draw_item_shadow))
		{
			BltVideoObjectOutlineShadow(buffer, item_vo, gfx_idx, cx - 2, cy + 2);
		}
		BltVideoObjectOutline(buffer, item_vo, gfx_idx, cx,     cy, outline_colour);

		if (buffer == FRAME_BUFFER)
		{
			InvalidateRegion(sX, sY, sX + sWidth, sY + sHeight);
		}
		else
		{
			RestoreExternBackgroundRect(sX, sY, sWidth, sHeight);
		}
	}

	if (ubStatusIndex < RENDER_ITEM_ATTACHMENT1)
	{
		// Ammo/stack count text layout: the caller's (layout -- e.g. the map's
		// "Show Large Icons" panel), else the usual ITEM_FONT one.
		SGPFont const text_font        = layout ? layout->font            : ITEM_FONT;
		INT16   const ammo_x           = layout ? layout->ammoX           : 1;
		INT16   const ammo_from_bottom = layout ? layout->ammoFromBottom  : 11;
		INT16   const cnt_from_right   = layout ? layout->countFromRight  : 4;
		INT16   const cnt_from_bottom  = layout ? layout->countFromBottom : 10;
		// restore height: at least the original 15, more for a taller font
		INT16   const text_h           = std::max<INT16>(15, GetFontHeight(text_font) + 2);

		SetFont(text_font);
		SetFontBackground(FONT_MCOLOR_BLACK);

		if (item->getItemClass() == IC_GUN && o.usItem != ROCKET_LAUNCHER)
		{
			// Display free rounds remianing
			UINT8 colour;
			switch (o.ubGunAmmoType)
			{
				case AMMO_AP:
				case AMMO_SUPER_AP: colour = layout ? layout->colAP          : ITEMDESC_FONTAPFORE;   break;
				case AMMO_HP:       colour = layout ? layout->colHP          : ITEMDESC_FONTHPFORE;   break;
				case AMMO_BUCKSHOT: colour = layout ? layout->colBuckshot    : ITEMDESC_FONTBSFORE;   break;
				case AMMO_HE:       colour = layout ? layout->colHE          : ITEMDESC_FONTHEFORE;   break;
				case AMMO_HEAT:     colour = layout ? layout->colHEAP        : ITEMDESC_FONTHEAPFORE; break;
				default:            colour = layout ? layout->colAmmoDefault : FONT_MCOLOR_DKGRAY;    break;
			}
			SetFontForeground(colour);

			ST::string const ammo = ST::format("{}", o.ubGunShotsLeft);
			const INT16 sNewX = sX + ammo_x;
			const INT16 sNewY = sY + sHeight - ammo_from_bottom;
			if (buffer == guiSAVEBUFFER)
			{
				RestoreExternBackgroundRect(sNewX, sNewY, std::max<INT16>(20, StringPixLength(ammo, text_font) + 2), text_h);
			}
			GPrintInvalidate(sNewX, sNewY, ammo);

			// Display 'JAMMED' if we are jammed
			if (o.bGunAmmoStatus < 0)
			{
				SetFontForeground(layout ? layout->colJammed : FONT_MCOLOR_RED);

				ST::string jammed =
					sWidth >= BIG_INV_SLOT_WIDTH - 10 ?
						TacticalStr[JAMMED_ITEM_STR] :
						TacticalStr[SHORT_JAMMED_GUN];

				INT16 cx;
				INT16 cy;
				FindFontCenterCoordinates(sX, sY, sWidth, sHeight, jammed, text_font, &cx, &cy);
				GPrintInvalidate(cx, cy, jammed);
			}
		}

		// Independent of the IC_GUN branch above (not "else if") -- a gun
		// can now have ubNumberOfObjects > 1 too (see CanGunsStack(),
		// Items.cc), and the two occupy different corners of the icon
		// (ammo/jam bottom-left, count bottom-right), so there's nothing
		// to lose by letting both render for the same slot.
		if (ubStatusIndex != RENDER_ITEM_NOSTATUS && o.ubNumberOfObjects > 1)
		{
			// Display # of items
			SetFontForeground(layout ? layout->colCount : FONT_GRAY4);

			ST::string pStr = ST::format("{}", o.ubNumberOfObjects);

			const UINT16 uiStringLength = StringPixLength(pStr, text_font);
			const INT16  sNewX          = sX + sWidth - uiStringLength - cnt_from_right;
			const INT16  sNewY          = sY + sHeight - cnt_from_bottom;

			if (buffer == guiSAVEBUFFER)
			{
				// Width used to be a flat 15px, wide enough for the single
				// digit a count could be while MAX_OBJECTS_PER_SLOT was 8 --
				// now that a count can run to 3 digits, tie it to the text's
				// own measured width (already computed above for sNewX)
				// instead of leaving a ghost digit behind on the next
				// redraw.
				//
				// Defensively clamped to stay on-screen -- RestoreExternBackgroundRect()
				// asserts otherwise. sNewX/width are both derived from sX/sWidth
				// (the slot's own on-screen box, always valid) plus this text's
				// measured width, so this should already be in bounds, but the
				// old flat 15px never exercised this edge (a single digit was
				// never wide enough to matter) so clamp rather than assume.
				INT16 const clampedX  = std::max<INT16>(sNewX, 0);
				INT16 const rectWidth = std::max<INT16>(0, std::min<INT16>(uiStringLength + cnt_from_right, SCREEN_WIDTH - clampedX));
				if (rectWidth > 0)
				{
					RestoreExternBackgroundRect(clampedX, sNewY, rectWidth, text_h);
				}
			}
			GPrintInvalidate(sNewX, sNewY, pStr);
		}

		// the attachment / weapon mode markers below keep ITEM_FONT
		SetFont(ITEM_FONT);

		if (ItemHasAttachments(o))
		{
			SetFontForeground(GetAttachmentHintColor(&o));

			ST::string attach_marker = "*";
			UINT16         const uiStringLength = StringPixLength(attach_marker, ITEM_FONT);
			INT16          const sNewX          = sX + sWidth - uiStringLength - 4;
			INT16          const sNewY          = sY;

			if (buffer == guiSAVEBUFFER)
			{
				RestoreExternBackgroundRect(sNewX, sNewY, 15, 15);
			}
			GPrintInvalidate(sNewX, sNewY, attach_marker);
		}

		if (s && &o == &s->inv[HANDPOS] && GCM->getItem(o.usItem)->getItemClass() == IC_GUN && s->bWeaponMode != WM_NORMAL)
		{
			SetFontForeground(FONT_DKRED);

			ST::string mode_marker = IsBurstMode(s->bWeaponMode) ? "*" : "+";
			UINT16         const uiStringLength = StringPixLength(mode_marker, ITEM_FONT);
			INT16          const sNewX          = sX + sWidth - uiStringLength - 4;
			INT16          const sNewY          = sY + 13; // rather arbitrary

			if (buffer == guiSAVEBUFFER)
			{
				RestoreExternBackgroundRect(sNewX, sNewY, 15, 15);
			}
			GPrintInvalidate(sNewX, sNewY, mode_marker);
		}
	}
}


BOOLEAN InItemDescriptionBox( )
{
	return( gfInItemDescBox );
}


INT16 MapBigItemDescBoxRight(void)
{
	if (!gfInItemDescBox || !IsMapBigWeaponBox() || gfTacBigWeaponBox) return 0;
	return gsInvDescX + GetMapItemDescWidth();
}

void CycleItemDescriptionItem( )
{
	// Delete old box...
	DeleteItemDescriptionBox( );

	// Make new item....
	const auto oldItemIndex = gpItemDescSoldier->inv[HANDPOS].usItem;
	auto items = GCM->getItems();
	auto it = std::find_if(items.begin(), items.end(), [oldItemIndex](const ItemModel* item) -> bool {
		return item->getItemIndex() == oldItemIndex;
	});
	if (it == items.end()) {
		SLOGE("Failed to find current item {} for cycling", oldItemIndex);
		return;
	}

	if (_KeyDown(SDLK_END))
	{
		// cycle backwards
		it = it == items.begin() ? items.end() - 1 : it - 1;
	}
	else
	{
		// cycle forwards
		it = it++;
		if (it == items.end()) {
			it = items.begin();
		}
	}

	const auto newItemIndex = (*it)->getItemIndex();

	CreateItem(newItemIndex, 100, &gpItemDescSoldier->inv[HANDPOS]);

	InternalInitItemDescriptionBox( &( gpItemDescSoldier->inv[ HANDPOS ] ), INTERFACE_START_X + 214, (INT16)(ITEMDESC_PANEL_START_Y + 1 ), gubItemDescStatusIndex, gpItemDescSoldier );
}


void InitItemDescriptionBox(SOLDIERTYPE* pSoldier, UINT8 ubPosition, INT16 sX, INT16 sY, UINT8 ubStatusIndex)
{
	InternalInitItemDescriptionBox(&pSoldier->inv[ubPosition], sX, sY, ubStatusIndex, pSoldier);
}


void InitKeyItemDescriptionBox(SOLDIERTYPE* const pSoldier, const UINT8 ubPosition, const INT16 sX, const INT16 sY)
{
	OBJECTTYPE *pObject;

	AllocateObject( &pObject );
	CreateKeyObject( pObject, pSoldier->pKeyRing[ ubPosition ].ubNumber ,pSoldier->pKeyRing[ ubPosition ].ubKeyID );

	InternalInitItemDescriptionBox(pObject, sX, sY, 0, pSoldier);
}


static void SetAttachmentTooltips(void)
{
	for (UINT i = 0; i < MAX_ATTACHMENTS; ++i)
	{
		const UINT16 attachment = gpItemDescObject->usAttachItem[i];
		ST::string tip = (attachment != NOTHING ? GCM->getItem(attachment)->getName() : g_langRes->Message[STR_ATTACHMENTS]);
		gItemDescAttachmentRegions[i].SetFastHelpText(tip);
	}
}


static void BtnMoneyButtonCallbackPrimary(GUI_BUTTON* btn, UINT32 reason);
static void BtnMoneyButtonCallbackSecondary(GUI_BUTTON* btn, UINT32 reason);
static void BtnMoneyButtonCallbackOther(GUI_BUTTON* btn, UINT32 reason);
static void ItemDescAmmoCallback(GUI_BUTTON* btn, UINT32 reason);
static void RefreshItemDescAmmoIcon(void);
static BOOLEAN TryReloadItemDescGunFromCursor(void);
static void DoAttachment(INT8 bAttachPos);
static void ItemDescAttachmentsCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemDescAttachmentsCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemDescCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemDescCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemDescDoneButtonCallbackPrimary(GUI_BUTTON* btn, UINT32 reason);
static void ItemDescDoneButtonCallbackSecondary(GUI_BUTTON* btn, UINT32 reason);
static void ReloadItemDesc(void);


static void ToggleMapHideEmptyAttachmentSlotsCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		// the checkbox has already taken its new state
		gfMapHideEmptyAttachmentSlots = btn->Clicked();
	}
}


void InternalInitItemDescriptionBox(OBJECTTYPE* const o, const INT16 sX, const INT16 sY, const UINT8 ubStatusIndex, SOLDIERTYPE* const s)
{
	// Set the current screen
	guiCurrentItemDescriptionScreen = guiCurrentScreen;
	const BOOLEAN real_map = (guiCurrentItemDescriptionScreen == MAP_SCREEN);
	// the 1366x768 SM panel's weapon box: the map's big one, rearranged
	gfTacBigWeaponBox = !real_map && g_ui.isBigSMPanel() && GCM->getItem(o->usItem)->isWeapon() &&
		GCM->doesGameResExists(TAC_ITEMDESC_BOX_WEAPONS_BIG_FILE);
	// and its money box: the map's big one as it is
	gfTacBigMoneyBox = !real_map && g_ui.isBigSMPanel() && o->usItem == MONEY &&
		GCM->doesGameResExists(TAC_ITEMDESC_BOX_MONEY_BIG_FILE);
	// and its items box (the rest): the map's big one as it is, too
	gfTacBigItemsBox = !real_map && g_ui.isBigSMPanel() && o->usItem != MONEY &&
		!GCM->getItem(o->usItem)->isWeapon() && GCM->doesGameResExists(TAC_ITEMDESC_BOX_ITEMS_BIG_FILE);
	// from here on "in_map": the map's box layouts (the tactical big ones too)
	const BOOLEAN in_map = real_map || IsTacBigItemDescBox();
	gMapItemDescLayout =
		real_map          ? GetMapItemDescLayout(*o)        :
		gfTacBigWeaponBox ? MAP_ITEMDESC_LAYOUT_WEAPONS_BIG :
		gfTacBigMoneyBox  ? MAP_ITEMDESC_LAYOUT_MONEY       :
		gfTacBigItemsBox  ? MAP_ITEMDESC_LAYOUT_ITEMS       :
		MAP_ITEMDESC_LAYOUT_SHARED;
	// "Show Large Icons": items and money on their own boxes, where those exist
	gfMapItemDescArialBox = FALSE;
	if (IsMapInvBigImages() && (gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS || gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_MONEY))
	{
		cache_key_t const big = gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS ? MAP_ITEMDESC_BOX_ITEMS_BIG_FILE : MAP_ITEMDESC_BOX_MONEY_BIG_FILE;
		cache_key_t const box = gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS ? MAP_ITEMDESC_BOX_ITEMS_FILE     : MAP_ITEMDESC_BOX_MONEY_FILE;
		gfMapItemDescArialBox = FirstUsableInterfaceAsset({ big, box }) == big;
	}
	if (gfTacBigMoneyBox || gfTacBigItemsBox) gfMapItemDescArialBox = TRUE;

	gsInvDescX = sX;
	gsInvDescY = sY;

	gpItemDescObject       = o;
	gubItemDescStatusIndex = ubStatusIndex;
	gpItemDescSoldier      = s;
	fItemDescDelete        = FALSE;
	MOUSE_CALLBACK itemDescCallback = MouseCallbackPrimarySecondary(ItemDescCallbackPrimary, ItemDescCallbackSecondary);

	// Build a mouse region here that is over any others.....
	if (in_map)
	{
		if (gfTacBigWeaponBox)
		{
			// from the screen's left edge, at its bottom, over the SM panel
			gsInvDescX = 0;
			gsInvDescY = SCREEN_HEIGHT - GetMapItemDescHeight();
			// the whole SM panel's buttons and texts: the box covers it
			if (gsCurInterfacePanel == SM_PANEL) HideSMPanelUnderItemDesc();
			// The box reaches higher than the panel: the world's viewport ends
			// at its top meanwhile, so scrolling the map (its shift-blit stops
			// there) leaves the box whole. Back in DeleteItemDescriptionBox().
			gsVIEWPORT_WINDOW_END_Y = gsInvDescY;
			SetRenderFlags(RENDER_FLAG_FULL);
		}
		else if (gfTacBigMoneyBox || gfTacBigItemsBox)
		{
			// at the screen's bottom, from x 236: the SM panel's portrait and
			// buttons stay free; the viewport stays at the panel (for now)
			gsInvDescX = TAC_ITEMDESC_BOX_MONEY_BIG_X;
			gsInvDescY = SCREEN_HEIGHT - GetMapItemDescHeight();
			// bookmark buttons draw over the box otherwise -- see the
			// tactical branch below
			if (gsCurInterfacePanel == SM_PANEL) HideSMBookmarkButtons();
		}
		MSYS_DefineRegion(&gInvDesc, gsInvDescX, gsInvDescY, gsInvDescX + GetMapItemDescWidth(), gsInvDescY + GetMapItemDescHeight(), MSYS_PRIORITY_HIGHEST - 2, CURSOR_NORMAL, MSYS_NO_CALLBACK, itemDescCallback);

		// "Hide empty attachment slots", on the weapon box itself
		if (gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_WEAPONS)
		{
			SGPPoint const& xy = g_map_weapons_hide_empty_slots_checkbox;
			giMapHideEmptySlotsCheckbox = CreateCheckBoxButton(
				gsInvDescX + xy.iX, gsInvDescY + xy.iY,
				INTERFACEDIR "/Sector_Inventory_PopupCheck.sti", MSYS_PRIORITY_HIGHEST,
				ToggleMapHideEmptyAttachmentSlotsCallback); // sub-images 0-3, like the tactical one
			if (gfMapHideEmptyAttachmentSlots) giMapHideEmptySlotsCheckbox->uiFlags |= BUTTON_CLICKED_ON;
			giMapHideEmptySlotsCheckbox->SetFastHelpText("Hide empty attachment slots");
		}

		// No Done button on the _items/_money/_weapons boxes -- see g_map_itemdesc_done_btn
		if (gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_SHARED)
		{
			SGPPoint const& done = g_map_itemdesc_done_btn;
			giMapInvDescButton = QuickCreateButtonImg(INTERFACEDIR "/itemdescdonebutton.sti", 0, 1, gsInvDescX + done.iX, gsInvDescY + done.iY, MSYS_PRIORITY_HIGHEST, ButtonCallbackPrimarySecondary(ItemDescDoneButtonCallbackPrimary, ItemDescDoneButtonCallbackSecondary));
		}

		if (real_map) fShowDescriptionFlag = TRUE;
	}
	else
	{
		// Which of the three tactical box backgrounds (Infobox.sti/
		// Infobox_money.sti/Infobox_items.sti) is about to be shown for this
		// item -- see the matching fIsMoney/fIsWeapon logic in
		// RenderItemDescriptionBox(). Each has its own independent vertical
		// anchor and size (UILayout.h/above), so gsInvDescY is recomputed
		// here rather than trusting the caller's sY -- every current caller
		// just passes the same shared macro anyway (ITEMDESC_START_Y/
		// SM_ITEMDESC_START_Y), calculated before the item's class is known.
		bool const fIsMoney  = o->usItem == MONEY;
		bool const fIsWeapon = GCM->getItem(o->usItem)->isWeapon();

		INT16 const width =
			fIsMoney  ? ITEMDESC_WIDTH_MONEY :
			fIsWeapon ? ITEMDESC_WIDTH :
			            ITEMDESC_WIDTH_ITEMS;
		INT16 const height =
			fIsMoney  ? ITEMDESC_HEIGHT_MONEY :
			fIsWeapon ? ITEMDESC_HEIGHT :
			            ITEMDESC_HEIGHT_ITEMS;
		gsInvDescY =
			fIsMoney  ? (INT16)(1 + ITEMDESC_PANEL_START_Y_MONEY) :
			fIsWeapon ? (INT16)(1 + ITEMDESC_PANEL_START_Y) :
			            (INT16)(1 + ITEMDESC_PANEL_START_Y_ITEMS);

		// CURSOR_NORMAL (not MSYS_NO_CURSOR): this single region spans both
		// gSMPanelRegion's and gViewportRegion's territory (ITEMDESC_PANEL_HEIGHT
		// > INV_INTERFACE_HEIGHT -- see UILayout.h), which have different
		// fallback cursors (CURSOR_NORMAL vs VIDEO_NO_CURSOR). Since
		// MSYS_UpdateMouseRegion() only re-resolves MSYS_NO_CURSOR's fallback on
		// region entry (not every frame), which one "wins" depended on where the
		// mouse first entered the box, and entering from above the panel's own
		// top edge left no cursor at all until the box was exited. Giving the
		// region its own real cursor sidesteps the fallback entirely. The
		// MAP_SCREEN branch above already does this.
		MSYS_DefineRegion(&gInvDesc, gsInvDescX, gsInvDescY, gsInvDescX + width, gsInvDescY + height, MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL, MSYS_NO_CALLBACK, itemDescCallback);

		if (gsCurInterfacePanel == SM_PANEL)
		{
			// GUI_BUTTONs render through their own pass (RenderButtons()),
			// independent of guiSAVEBUFFER/InItemDescriptionBox() -- any
			// bookmark button whose rectangle overlaps Infobox.sti/
			// Infobox_money.sti would otherwise draw on top of it. Shown
			// again in DeleteItemDescriptionBox().
			HideSMBookmarkButtons();
		}

		// Merc preview picture, Infobox.sti (weapon) only -- see
		// gMercPreviewFrames/GetMercPreviewFrame() above and the matching
		// draw call in RenderItemDescriptionBox(). Loaded here (once, up
		// front) rather than every render pass, same as every other
		// cache_key_t/VObject used by this box; unloaded in
		// DeleteItemDescriptionBox().
		MercPreviewFrame previewFrame;
		if (ENABLE_MERC_PREVIEW_PICTURE && fIsWeapon && s && GetMercPreviewFrame(*s, &previewFrame))
		{
			gfMercPreviewSurfaceLocked = !AnimationSurfaceLockedBySoldier(SOLDIER2ID(s), previewFrame.usAnimSurface);
			LoadAnimationSurface(SOLDIER2ID(s), previewFrame.usAnimSurface, s->usAnimState);
			gusMercPreviewAnimSurface = previewFrame.usAnimSurface;
		}
	}

	if (GCM->getItem(o->usItem)->isGun()&& o->usItem != ROCKET_LAUNCHER)
	{
		ST::string pStr = ST::format("{}/{}", o->ubGunShotsLeft, GCM->getWeapon(o->usItem)->ubMagSize);

		INT32 img;
		switch (o->ubGunAmmoType)
		{
			case AMMO_AP:
			case AMMO_SUPER_AP: img = 5; break;
			case AMMO_HP:       img = 9; break;
			default:            img = 1; break;
		}
		// Ammo-type icons (indices 1-12) live in a separate .sti from the
		// weapon-description graphic (infobox.sti index 0), but keep the same
		// index numbering — infobox_bullets.sti also reserves index 0 unused,
		// so img/img+2/img+3 still point at the same logical pictures.
		BUTTON_PICS* const ammo_img = LoadButtonImage(INTERFACEDIR "/infobox_bullets.sti", img + 3, img, -1, img + 2, -1);
		giItemDescAmmoButtonImages = ammo_img;

		const INT16         h  = GetDimensionsOfButtonPic(ammo_img)->h;
		SGPBox const xy_box = TB(in_map ? PickMapItemDesc(g_desc_item_box_map, g_desc_item_box_map_items, g_desc_item_box_map_money, g_desc_item_box_map_weapons, g_desc_item_box_map_weapons_big, g_desc_item_box_map_items_big) : g_desc_item_box);
		const SGPBox* const xy = &xy_box;
		const INT16         x  = gsInvDescX + xy->x;
		const INT16         y  = gsInvDescY + xy->y + xy->h - h; // align with bottom
		const INT16         text_col   = ITEMDESC_AMMO_FORE;
		const INT16         shadow_col = FONT_MCOLOR_BLACK;
		GUIButtonRef  const ammo_btn   = CreateIconAndTextButton(ammo_img, pStr, TINYFONT1, text_col, shadow_col, text_col, shadow_col, x, y, MSYS_PRIORITY_HIGHEST, ItemDescAmmoCallback);
		giItemDescAmmoButton = ammo_btn;

		// Disable the eject button, if we are being init from the shop keeper
		// screen and this is a dealer item we are getting info from
		if (guiCurrentScreen == SHOPKEEPER_SCREEN && pShopKeeperItemDescObject)
		{
			ammo_btn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_HATCHED);
			DisableButton(ammo_btn);
		}
		else
		{
			ammo_btn->SetFastHelpText(g_langRes->Message[STR_EJECT_AMMO]);
		}

		INT16 usX;
		INT16 usY;
		FindFontCenterCoordinates(ITEMDESC_AMMO_TEXT_X, ITEMDESC_AMMO_TEXT_Y, ITEMDESC_AMMO_TEXT_WIDTH, GetFontHeight(TINYFONT1), pStr, TINYFONT1, &usX, &usY);
		ammo_btn->SpecifyTextOffsets(usX, usY, TRUE);
	}

	if (ITEM_PROS_AND_CONS(o->usItem))
	{
		// the map's big weapon box: FONT12ARIAL, pros and cons side by side
		bool          const big              = IsMapBigWeaponBox();
		SGPFont       const font             = big ? FONT12ARIAL : ITEMDESC_FONT;
		INT16         const pros_cons_indent = std::max(StringPixLength(gzProsLabel, font), StringPixLength(gzConsLabel, font)) + 10;
		SGPBox        const pros_box         = TB(big ? g_map_itemdesc_pros_cons_box_weapons_big : IsMapWeaponBox() ? g_map_itemdesc_pros_cons_box_weapons : in_map ? g_map_itemdesc_pros_cons_box : g_itemdesc_pros_cons_box);
		SGPBox        const cons_box         = TB(g_map_itemdesc_cons_box_weapons_big);
		const SGPBox* const box              = &pros_box;
		UINT16        const h                = GetFontHeight(font);
		for (INT32 i = 0; i < 2; ++i)
		{
			// Add region for pros/cons help text
			const SGPBox* const line = (big && i == 1 ? &cons_box : box);
			UINT16 const x = line->x + pros_cons_indent + gsInvDescX;
			UINT16 const y = line->y + (big ? 0 : i * box->h) + gsInvDescY;
			UINT16 const w = line->w - pros_cons_indent;
			MOUSE_REGION* const r = &gProsAndConsRegions[i];
			MSYS_DefineRegion(r, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, itemDescCallback);

			ST::string label;
			// use temp variable to prevent an initial comma from being displayed
			ST::string FullItemTemp;
			if (i == 0)
			{
				label = gzProsLabel;
				GenerateProsString(FullItemTemp, *o, 1000);
			}
			else
			{
				label = gzConsLabel;
				GenerateConsString(FullItemTemp, *o, 1000);
			}
			ST::string text = ST::format("{} {}", label, FullItemTemp);
			r->SetFastHelpText(text);
		}
	}

	if (o->usItem != MONEY)
	{
		// Same table and slot count as RenderItemDescriptionBox() draws, so
		// the regions sit on the slots: on the tactical screen non-weapon
		// items have their own 4-slot layout (Infobox_items.sti).
		// The map's iteminfoc_items box has the same 4 slots.
		bool const fGenericItem = !in_map && !GCM->getItem(o->usItem)->isWeapon();
		const AttachmentGfxInfo* const agi =
			in_map       ? &PickMapItemDesc(g_map_attachment_info, g_map_attachment_info_items, g_map_attachment_info, g_map_attachment_info_weapons, g_map_attachment_info_weapons_big, g_map_attachment_info_items_big, g_map_attachment_info) :
			fGenericItem ? &g_generic_item_attachment_info :
			               &g_attachment_info;
		INT32 const numAttachmentSlots =
			gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS && gfMapItemDescArialBox ? BIG_ITEMS_ATTACHMENT_SLOTS :
			fGenericItem || gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS ? GENERIC_ITEM_ATTACHMENT_SLOTS : MAX_ATTACHMENTS;
		for (INT32 i = 0; i < numAttachmentSlots; ++i)
		{
			// Build a mouse region here that is over any others.....
			SGPPoint const      slot = AttachmentSlotPos(agi->slot, i);
			const UINT16        x = agi->item_box.x + slot.iX + gsInvDescX;
			const UINT16        y = agi->item_box.y + slot.iY + gsInvDescY;
			const UINT16        w = agi->item_box.w;
			const UINT16        h = agi->item_box.h;
			MOUSE_REGION* const r = &gItemDescAttachmentRegions[i];
			MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MouseCallbackPrimarySecondary(ItemDescAttachmentsCallbackPrimary, ItemDescAttachmentsCallbackSecondary));
			MSYS_SetRegionUserData(r, 0, i);
		}
		SetAttachmentTooltips();
	}
	else
	{
		GUI_CALLBACK btnMoneyButtonCallback = ButtonCallbackPrimarySecondary(BtnMoneyButtonCallbackPrimary, BtnMoneyButtonCallbackSecondary, BtnMoneyButtonCallbackOther);

		gRemoveMoney = REMOVE_MONEY{};
		gRemoveMoney.uiTotalAmount    = o->uiMoneyAmount;
		gRemoveMoney.uiMoneyRemaining = o->uiMoneyAmount;
		gRemoveMoney.uiMoneyRemoving  = 0;

		// Create buttons for the money
		guiMoneyButtonImage = LoadButtonImage(GetMoneyGraphicsFilename(), 1, 2);
		INT32 i;
		for (i = 0; i < NUM_MONEY_BUTTONS - 1; i++)
		{
			guiMoneyButtonBtn[i] = CreateIconAndTextButton(
				guiMoneyButtonImage, gzMoneyAmounts[i], BLOCKFONT2,
				5, DEFAULT_SHADOW,
				5, DEFAULT_SHADOW,
				GetMoneyButtonPos(i).iX, GetMoneyButtonPos(i).iY, MSYS_PRIORITY_HIGHEST,
				btnMoneyButtonCallback
			);
			guiMoneyButtonBtn[i]->SetUserData(i);
		}
		if (gRemoveMoney.uiTotalAmount < 1000) DisableButton(guiMoneyButtonBtn[M_1000]);
		if (gRemoveMoney.uiTotalAmount <  100) DisableButton(guiMoneyButtonBtn[M_100]);
		if (gRemoveMoney.uiTotalAmount <   10) DisableButton(guiMoneyButtonBtn[M_10]);

		// Create the Done button
		guiMoneyDoneButtonImage = UseLoadedButtonImage(guiMoneyButtonImage, 3, 4);
		guiMoneyButtonBtn[i] = CreateIconAndTextButton(
			guiMoneyDoneButtonImage, gzMoneyAmounts[i], BLOCKFONT2,
			5, DEFAULT_SHADOW,
			5, DEFAULT_SHADOW,
			GetMoneyButtonPos(i).iX, GetMoneyButtonPos(i).iY, MSYS_PRIORITY_HIGHEST,
			btnMoneyButtonCallback
		);
		guiMoneyButtonBtn[i]->SetUserData(i);
	}

	fInterfacePanelDirty = DIRTYLEVEL2;
	gfInItemDescBox      = TRUE;

	ReloadItemDesc();

	gpAttachSoldier = (gpItemPointer ? gpItemPointerSoldier : s);
	// Store attachments that item originally had
	for (INT32 i = 0; i < MAX_ATTACHMENTS; ++i)
	{
		gusOriginalAttachItem[i]  = o->usAttachItem[i];
		gbOriginalAttachStatus[i] = o->bAttachStatus[i];
	}

	if (gpItemPointer != NULL && !gfItemDescHelpTextOffset && !CheckFact(FACT_ATTACHED_ITEM_BEFORE, 0))
	{
		ST::string text;
		if (!(GCM->getItem(o->usItem)->getFlags() & ITEM_HIDDEN_ADDON) && (
			ValidAttachment(gpItemPointer->usItem, o->usItem) ||
			ValidLaunchable(gpItemPointer->usItem, o->usItem) ||
			ValidMerge(gpItemPointer->usItem, o->usItem)))
		{
			text = g_langRes->Message[STR_ATTACHMENT_HELP];
		}
		else
		{
			text = g_langRes->Message[STR_ATTACHMENT_INVALID_HELP];
		}
		SetUpFastHelpRegion(69 + gsInvDescX, 12 + gsInvDescY, 170, text);

		StartShowingInterfaceFastHelpText();

		SetFactTrue(FACT_ATTACHED_ITEM_BEFORE);
		gfItemDescHelpTextOffset = TRUE;
	}
}


static void ReloadItemDesc(void)
{
	auto itemId = gpItemDescObject->usItem;
	gusItemDescShownItem = itemId;
	auto item = GCM->getItem(itemId);
	auto graphic = GetBigInventoryGraphicForItem(item);

	guiItemGraphic = graphic.first;
	guiItemGraphicIndex = graphic.second;

	//
	// Load name, desc
	//

	//if the player is extracting money from the players account, use a different item name and description
	if (itemId == MONEY && gfAddingMoneyToMercFromPlayersAccount)
	{
		itemId = MONEY_FOR_PLAYERS_ACCOUNT;
	}
	item = GCM->getItem(itemId);
	gzItemName = item->getName();
	gzItemDesc = item->getDescription();
}


static void ItemDescAmmoCallback(GUI_BUTTON*  const btn, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		if (gpItemPointer)
		{
			// Dropping a compatible ammo/magazine item onto the Eject Ammo
			// icon reloads the gun -- see TryReloadItemDescGunFromCursor()
			// below (also used by ItemDescCallbackPrimary() for the same
			// drop gesture onto the box's main picture) for how this
			// mirrors PlaceObject()'s own IC_GUN + IC_AMMO + calibre-match
			// dispatch to ReloadGun() (Items.cc) -- the mechanism actually
			// used when ammo is dropped onto a gun slot in
			// Inventory_bottom_panel.sti. ValidMerge()/AttachObject()
			// (used below by DoAttachment() for scopes/silencers/bipods,
			// and tried here previously) never apply to gun+ammo --
			// ValidMerge() only matches two identical ammo stacks merging
			// together, and a gun is never IC_AMMO.
			TryReloadItemDescGunFromCursor();
			return;
		}
		if (!EmptyWeaponMagazine(gpItemDescObject, &gItemPointer)) return;

		SetItemPointer(&gItemPointer, gpItemDescSoldier);
		fInterfacePanelDirty = DIRTYLEVEL2;

		btn->SpecifyText("0");
		RefreshItemDescAmmoIcon();

		if (guiCurrentItemDescriptionScreen == MAP_SCREEN)
		{
			SetMapCursorItem();
			fTeamPanelDirty = TRUE;
		}
		else
		{
			// if in SKI, load item into SKI's item pointer
			if (guiCurrentScreen == SHOPKEEPER_SCREEN)
			{
				// pick up bullets from weapon into cursor (don't try to sell)
				BeginSkiItemPointer(PLAYERS_INVENTORY, -1, FALSE);
			}

			// Deliberately does NOT auto-close the box (used to:
			// fItemDescDelete = TRUE, picked up on the next frame by
			// HandleItemDescriptionBox()) -- same as removing an attachment
			// (ItemDescAttachmentsCallbackPrimary() below never closes the
			// box either). That deferred close went through
			// DeleteItemDescriptionBox() one frame later than a direct
			// click-outside-the-box close (ItemDescCallbackPrimary/Secondary,
			// which call it synchronously from the mouse-click handler,
			// before RenderWorld.cc's RENDER_FLAG_FULL check that same
			// frame) -- landing one frame too late for that same-frame
			// check, which left Infobox.sti's extra height (above
			// INV_INTERFACE_HEIGHT -- see ITEMDESC_PANEL_HEIGHT,
			// UILayout.h) visibly stuck on screen instead of being cleanly
			// replaced by the freshly re-rendered tactical world. Ejecting
			// ammo now just leaves the box open, same as ejecting an
			// attachment -- the user closes it manually the same way either
			// way, through the already-reliable direct-call path.
		}
	}
}


// Refreshes the Eject Ammo icon's ammo-type color from gpItemDescObject's
// current ubGunAmmoType -- same img switch as InternalInitItemDescriptionBox()
// uses to pick it at box-open time, which is the only other place it's
// computed. Unlike the button's text (kept current via SpecifyText()
// wherever the shots-left count changes), the icon image is otherwise never
// touched again after creation, so callers that change gpItemDescObject's
// ammo (ejecting, reloading) must call this afterward or the icon keeps
// showing the pre-change ammo color until the box is closed and reopened.
static void RefreshItemDescAmmoIcon(void)
{
	if (!giItemDescAmmoButton) return;

	INT32 img;
	switch (gpItemDescObject->ubGunAmmoType)
	{
		case AMMO_AP:
		case AMMO_SUPER_AP: img = 5; break;
		case AMMO_HP:       img = 9; break;
		default:            img = 1; break;
	}

	BUTTON_PICS* const new_img = LoadButtonImage(INTERFACEDIR "/infobox_bullets.sti", img + 3, img, -1, img + 2, -1);
	UnloadButtonImage(giItemDescAmmoButtonImages);
	giItemDescAmmoButtonImages = new_img;
	giItemDescAmmoButton->image = new_img;
}


// Attempts to reload gpItemDescObject (the gun shown in the box) with
// whatever's held on the cursor (gpItemPointer), for the two places in
// this box a magazine/ammo item can be dropped to do so: the Eject Ammo
// icon (ItemDescAmmoCallback()) and the box's own main weapon picture
// (ItemDescCallbackPrimary()). Same IC_GUN + IC_AMMO + calibre-match
// precondition PlaceObject() uses before calling ReloadGun() (Items.cc)
// when ammo is dropped onto a gun slot in Inventory_bottom_panel.sti.
// Returns TRUE if a reload happened (cursor, button text and icon are
// already refreshed); FALSE if there was nothing to do -- no item held,
// gpItemDescObject isn't a gun, the held item isn't compatible ammo, or
// ReloadGun() itself refused (e.g. not enough APs in combat).
static BOOLEAN TryReloadItemDescGunFromCursor(void)
{
	if (!gpItemPointer) return FALSE;
	if (!GCM->getItem(gpItemDescObject->usItem)->isGun()) return FALSE;

	ItemModel const* const heldItem = GCM->getItem(gpItemPointer->usItem);
	if (heldItem->getItemClass() != IC_AMMO) return FALSE;
	if (!GCM->getWeapon(gpItemDescObject->usItem)->matches(heldItem->asAmmo()->calibre)) return FALSE;

	if (!ReloadGun(gpItemDescSoldier, gpItemDescObject, gpItemPointer)) return FALSE;

	// ReloadGun() mutates *gpItemPointer in place: it may empty it (ammo
	// fully consumed), leave a partial stack behind, or -- when swapping
	// two full magazines -- replace it with the ejected old magazine.
	// Same cursor bookkeeping UIHandleItemPlacement() does after its own
	// PlaceObject() call (Interface_Panels.cc).
	if (gpItemPointer->ubNumberOfObjects == 0)
	{
		EndItemPointer();
	}

	fInterfacePanelDirty = DIRTYLEVEL2;

	// Refresh the "shots left/mag size" text and the ammo-color icon on
	// the Eject Ammo button -- ReloadGun() doesn't touch either, so
	// without this they'd keep showing the pre-reload count/color until
	// the box is closed and reopened.
	if (giItemDescAmmoButton)
	{
		giItemDescAmmoButton->SpecifyText(ST::format("{}/{}", gpItemDescObject->ubGunShotsLeft, GCM->getWeapon(gpItemDescObject->usItem)->ubMagSize));
	}
	RefreshItemDescAmmoIcon();

	return TRUE;
}


static void DoAttachment(INT8 const bAttachPos)
{
	if (AttachObject(gpItemDescSoldier, gpItemDescObject, gpItemPointer, gubItemDescStatusIndex, bAttachPos))
	{
		if (gpItemPointer->usItem == NOTHING)
		{
			// attachment attached, merge item consumed, etc

			if (fInMapMode)
			{
				MAPEndItemPointer( );
			}
			else
			{
				// End Item pickup
				gpItemPointer = NULL;
				EnableSMPanelButtons( TRUE , TRUE );

				gSMPanelRegion.ChangeCursor(CURSOR_NORMAL);
				SetCurrentCursorFromDatabase( CURSOR_NORMAL );

				//if we are currently in the shopkeeper interface
				if (guiCurrentScreen == SHOPKEEPER_SCREEN)
				{
					//Clear out the moving cursor
					gMoveingItem = INVENTORY_IN_SLOT{};

					//change the curosr back to the normal one
					SetSkiCursor( CURSOR_NORMAL );
				}
			}
		}

		if ( gpItemDescObject->usItem == NOTHING )
		{
			// close desc panel panel
			DeleteItemDescriptionBox();
		}
		else
		{
			SetAttachmentTooltips();
		}
		//Dirty interface
		fInterfacePanelDirty = DIRTYLEVEL2;

		ReloadItemDesc( );
	}

	// re-evaluate repairs
	gfReEvaluateEveryonesNothingToDo = TRUE;
}


static void PermanantAttachmentMessageBoxCallBack(MessageBoxReturnValue const ubExitValue)
{
	if ( ubExitValue == MSG_BOX_RETURN_YES )
	{
		DoAttachment(gbPendingAttachPos);
	}
	// else do nothing
}


static void ItemDescAttachmentsCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if ( gfItemDescObjectIsAttachment )
	{
		// screen out completely
		return;
	}

	UINT32 uiItemPos = MSYS_GetRegionUserData( pRegion, 0 );

	// if the item being described belongs to a shopkeeper, ignore attempts to pick it up / replace it
	if (guiCurrentScreen == SHOPKEEPER_SCREEN && pShopKeeperItemDescObject)
	{
		return;
	}

	// Try to place attachment if something is in our hand
	// require as many APs as to reload
	if ( gpItemPointer != NULL )
	{
		// nb pointer could be NULL because of inventory manipulation in mapscreen from sector inv
		if ( !gpItemPointerSoldier || EnoughPoints( gpItemPointerSoldier, AP_RELOAD_GUN, 0, TRUE ) )
		{
			if ( (GCM->getItem(gpItemPointer->usItem)->getFlags() & ITEM_INSEPARABLE) && ValidAttachment( gpItemPointer->usItem, gpItemDescObject->usItem ) )
			{
				gbPendingAttachPos = (INT8)uiItemPos;
				DoScreenIndependantMessageBox(g_langRes->Message[STR_PERMANENT_ATTACHMENT], MSG_BOX_FLAG_YESNO, PermanantAttachmentMessageBoxCallBack);
				return;
			}

			DoAttachment((INT8)uiItemPos);
		}
	}
	else
	{
		// ATE: Make sure we have enough AP's to drop it if we pick it up!
		if ( EnoughPoints( gpItemDescSoldier, ( AP_RELOAD_GUN + AP_PICKUP_ITEM ), 0, TRUE ) )
		{
			// Get attachment if there is one
			// The follwing function will handle if no attachment is here
			if ( RemoveAttachment( gpItemDescObject, (UINT8)uiItemPos, &gItemPointer ) )
			{
				SetItemPointer(&gItemPointer, gpItemDescSoldier);

				//if( guiCurrentScreen == MAP_SCREEN )
				if( guiCurrentItemDescriptionScreen == MAP_SCREEN )
				{
					SetMapCursorItem();
					fTeamPanelDirty=TRUE;
				}

				//if we are currently in the shopkeeper interface
				else if (guiCurrentScreen == SHOPKEEPER_SCREEN)
				{
					// pick up attachment from item into cursor (don't try to sell)
					BeginSkiItemPointer( PLAYERS_INVENTORY, -1, FALSE );
				}

				//Dirty interface
				fInterfacePanelDirty = DIRTYLEVEL2;

				// re-evaluate repairs
				gfReEvaluateEveryonesNothingToDo = TRUE;

				UpdateItemHatches();
				SetAttachmentTooltips();
			}
		}
	}
}

static void ItemDescAttachmentsCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if ( gfItemDescObjectIsAttachment )
	{
		// screen out completely
		return;
	}

	UINT32 uiItemPos = MSYS_GetRegionUserData( pRegion, 0 );

	static OBJECTTYPE Object2;

	if ( gpItemDescObject->usAttachItem[ uiItemPos ] != NOTHING )
	{
		BOOLEAN fShopkeeperItem = FALSE;

		// remember if this is a shopkeeper's item we're viewing ( pShopKeeperItemDescObject will get nuked on deletion )
		if (guiCurrentScreen == SHOPKEEPER_SCREEN && pShopKeeperItemDescObject)
		{
			fShopkeeperItem = TRUE;
		}

		DeleteItemDescriptionBox( );

		CreateItem(gpItemDescObject->usAttachItem[uiItemPos], gpItemDescObject->bAttachStatus[uiItemPos], &Object2);

		gfItemDescObjectIsAttachment = TRUE;
		InternalInitItemDescriptionBox(&Object2, gsInvDescX, gsInvDescY, 0, gpItemDescSoldier);

		if (fShopkeeperItem)
		{
			pShopKeeperItemDescObject = &Object2;
			StartSKIDescriptionBox();
		}
	}
}


static ST::string GetObjectImprint(OBJECTTYPE const& o)
{
	return !HasObjectImprint(o) ? ST::string() :
		o.ubImprintID == NO_PROFILE + 1 ? pwMiscSectorStrings[3] :
		GetProfile(o.ubImprintID).zNickname;
}


// Colours of a stat's label and value in ITEMDESC_FONT (BLOCKFONT2), and the
// same two colours in the Arial / Humanist fonts of the map's big weapon box
// (iteminfoc_weapons_big): those fonts have another palette, where 5 and 6
// are black.
#define ITEMDESC_LABEL_COLOUR			6
#define ITEMDESC_VALUE_COLOUR			5
#define MAP_BIG_WEAPON_LABEL_COLOUR		126 // (178, 168, 118), for BLOCKFONT2's (173, 148, 115)
#define MAP_BIG_WEAPON_VALUE_COLOUR		FONT_BEIGE // (201, 197, 143), for BLOCKFONT2's (214, 201, 156)

static void HighlightIf(const BOOLEAN cond, UINT8 const value_colour = ITEMDESC_VALUE_COLOUR)
{
	SetFontForeground(cond ? ITEMDESC_FONTHIGHLIGHT : value_colour);
}


// Called every frame from RenderTopmostTacticalInterface() (Interface_Control.cc),
// regardless of whether the box is currently open -- see the comment on
// gsItemDescCloseCleanupFrames/DeleteItemDescriptionBox() above for why this
// exists (bridging the 1-frame gap between RENDER_FLAG_FULL being set and it
// actually being consumed by RenderWorld.cc's own full-redraw check).
void TickItemDescCloseCleanup(void)
{
	if (gsItemDescCloseCleanupFrames == 0) return;

	InvalidateRegionEx(gsItemDescCloseCleanupX, gsItemDescCloseCleanupY,
		gsItemDescCloseCleanupX + gsItemDescCloseCleanupW, gsItemDescCloseCleanupY + gsItemDescCloseCleanupH);

	--gsItemDescCloseCleanupFrames;
}


// What a weapon is worth, its ammunition and attachments included: each part's
// price by its condition, the way dealers value an object
// (ItemConditionModifier(), ShopKeeper_Interface.cc), without their markup.
static FLOAT ItemValueConditionModifier(UINT16 const usItem, INT8 bStatus)
{
	ItemModel const* const item = GCM->getItem(usItem);
	if (item->getItemClass() == IC_AMMO)
	{
		// bullets left / magazine capacity
		UINT16 const capacity = item->asAmmo()->capacity;
		return bStatus / (FLOAT)(capacity ? capacity : 1);
	}
	if (bStatus < 0) bStatus = -bStatus; // jammed
	// a repairable item at 0% is still worth half its price
	return item->getFlags() & ITEM_REPAIRABLE ? 0.5f + bStatus / 200.0f : bStatus / 100.0f;
}


static UINT32 CalcWeaponValue(OBJECTTYPE const& o)
{
	ItemModel const* const item  = GCM->getItem(o.usItem);
	FLOAT                  value = item->getPrice() * ItemValueConditionModifier(o.usItem, o.bGunStatus);

	if (item->getItemClass() == IC_GUN && o.usGunAmmoItem != NONE)
	{
		// magazines by the bullets left, other payloads (grenades, shells) by status
		bool const magazine = GCM->getItem(o.usGunAmmoItem)->getItemClass() == IC_AMMO;
		value += GCM->getItem(o.usGunAmmoItem)->getPrice() *
			ItemValueConditionModifier(o.usGunAmmoItem, magazine ? o.ubGunShotsLeft : o.bGunAmmoStatus);
	}

	for (INT32 i = 0; i != MAX_ATTACHMENTS; ++i)
	{
		if (o.usAttachItem[i] == NONE) continue;
		value += GCM->getItem(o.usAttachItem[i])->getPrice() * ItemValueConditionModifier(o.usAttachItem[i], o.bAttachStatus[i]);
	}
	return (UINT32)value;
}


// The texts only the map screen's big weapon box (iteminfoc_weapons_big)
// shows -- see gMapBigWeaponBoxTitles/gMapBigWeaponBoxTexts. Drawn to the frame buffer, after the
// box itself has been copied there.
static void RenderMapBigWeaponBoxTexts(OBJECTTYPE const& obj, INT16 const dx, INT16 const dy)
{
	ItemModel      const* const item = GCM->getItem(obj.usItem);
	WeaponModel    const* const w    = GCM->getWeapon(obj.usItem);

	auto const title = [&](INT32 const i, SGPFont const font)
	{
		SGPBox      const  box = TB(gMapBigWeaponBoxTitles[i]);
		ST::string  const& str = gzMapBigWeaponBoxStrings[i];
		INT16 x;
		INT16 y;
		FindFontCenterCoordinates(dx + box.x, dy + box.y, box.w, box.h, str, font, &x, &y);
		MPrint(x, y, str);
	};
	auto const label_and_value = [&](INT32 const i, ST::string const& value)
	{
		INV_DESC_STATS const t = TB(gMapBigWeaponBoxTexts[i - BIGW_TXT_SHORT]);
		SetFontForeground(MAP_BIG_WEAPON_LABEL_COLOUR);
		MPrint(dx + t.sX, dy + t.sY, gzMapBigWeaponBoxStrings[i]);
		SetFontForeground(MAP_BIG_WEAPON_VALUE_COLOUR);
		INT16 x;
		INT16 y;
		FindFontRightCoordinates(dx + t.sX + t.sValDx, dy + t.sY, ITEM_STATS_WIDTH, GetFontHeight(FONT12ARIAL), value, FONT12ARIAL, &x, &y);
		MPrint(x, y, value);
	};

	SetFontAttributes(FONT14HUMANIST, MAP_BIG_WEAPON_VALUE_COLOUR);
	title(BIGW_TXT_BALLISTICS,  FONT14HUMANIST);
	title(BIGW_TXT_MAINTENANCE, FONT14HUMANIST);

	SetFontAttributes(FONT14ARIAL, MAP_BIG_WEAPON_LABEL_COLOUR);
	// "Burst AP" only for a weapon with a burst mode, like its lines below
	bool const has_burst = w->ubShotsPerShortBurst > 0 || w->ubShotsPerLongBurst > 0 || w->ubShotsPerFullBurst > 0;
	for (INT32 i = BIGW_TXT_FIRE_POWER; i <= BIGW_TXT_VALUATION; ++i)
	{
		if (i == BIGW_TXT_BURST_AP && !has_burst) continue;
		// its lines sit in the Fire Power field now, without a title
		if (i == BIGW_TXT_SINGLE_AP) continue;
		title(i, FONT14ARIAL);
	}

	SetFontAttributes(FONT12ARIAL, MAP_BIG_WEAPON_LABEL_COLOUR);

	UINT8 const single_aps = BaseAPsToShootOrStab(DEFAULT_APS, DEFAULT_AIMSKILL, obj);

	// Short / Long / Full: the AP cost of the mode (the weapon's own
	// ubAPsPer...Burst through the usual formula, see CalcAPsToBurst()) and
	// its rounds. A mode the weapon doesn't have is left out.
	static WeaponModes const modes[] = { WM_BURST_SHORT, WM_BURST_LONG, WM_BURST_FULL };
	UINT8 const mode_shots[] = { w->ubShotsPerShortBurst, w->ubShotsPerLongBurst, w->ubShotsPerFullBurst };
	bool        any_burst    = false;
	for (INT32 m = 0; m != 3; ++m)
	{
		if (mode_shots[m] == 0) continue;
		any_burst = true;

		label_and_value(BIGW_TXT_SHORT + m, ST::format("{}", single_aps + CalcAPsToBurst(DEFAULT_APS, DEFAULT_AIMSKILL, obj, modes[m])));

		INV_DESC_STATS const r = TB(gMapBigWeaponBoxRounds[m]);
		SetFontForeground(MAP_BIG_WEAPON_LABEL_COLOUR);
		MPrint(dx + r.sX, dy + r.sY, gzMapBigWeaponBoxStrings[BIGW_TXT_ROUNDS]);
		SetFontForeground(MAP_BIG_WEAPON_VALUE_COLOUR);
		if (modes[m] == WM_BURST_FULL)
		{
			// the factory magazine, a larger one doesn't lengthen the burst
			MPrint(dx + r.sX + r.sValDx, dy + r.sY, st_format_printf(gzMapBigWeaponBoxStrings[BIGW_TXT_UP_TO], (INT32)w->ubShotsPerFullBurst));
		}
		else
		{
			INT32 x = dx + r.sX + r.sValDx;
			for (INT32 i = GunShotsPerBurst(obj, modes[m]); i != 0; --i)
			{
				BltVideoObject(FRAME_BUFFER, guiBullet, 0, x, dy + r.sY);
				x += BULLET_WIDTH + 1;
			}
		}
	}

	if (any_burst)
	{
		label_and_value(BIGW_TXT_BRST_PEN_PRONE, ST::format("{}%", GunProneBurstPenalty(obj)));
	}

	if (item->isGun())
	{
		// clearing a jam costs a single shot for now
		label_and_value(BIGW_TXT_UNJAM, ST::format("{}", single_aps));
	}

	label_and_value(BIGW_TXT_VALUE, SPrintMoney(CalcWeaponValue(obj)));
}


// `text` word-wrapped onto at most `lines` lines, `pitch` px apart, in
// white with the usual shadow. If it goes on past them, the last one ends in
// "...".
static void PrintLinesWithEllipsis(INT16 const x, INT16 const y, INT16 const w, INT16 const pitch, INT32 const lines, SGPFont const font, ST::string const& text)
{
	std::vector<ST::string> const words = text.split(' ');
	INT16 const dots = StringPixLength(DOTDOTDOT, font);

	// DEFAULT_SHADOW: shadow colour 0 (FONT_MCOLOR_BLACK) means no shadow
	SetFontAttributes(font, FONT_FCOLOR_WHITE, DEFAULT_SHADOW);

	size_t next = 0;
	for (INT32 line = 0; line != lines && next < words.size(); ++line)
	{
		ST::string out;
		while (next < words.size())
		{
			ST::string const longer = out.empty() ? words[next] : out + " " + words[next];
			if (StringPixLength(longer, font) > w) break;
			out = longer;
			++next;
		}
		// a single word wider than the line: it goes on its own
		if (out.empty()) out = words[next++];

		if (line == lines - 1 && next < words.size())
		{
			// more than fits: drop words until "..." fits after them
			while (StringPixLength(out, font) + dots > w)
			{
				ST_ssize_t const space = out.find_last(' ');
				if (space <= 0) break;
				out = out.left(space);
			}
			out += DOTDOTDOT;
		}
		MPrint(x, y + line * pitch, out);
	}
}


void RenderItemDescriptionBox(void)
{
	if (!gfInItemDescBox)
	{
		// Map screen, after the box closed: redraw the left column a few
		// more frames -- see gsMapItemDescCloseCleanupFrames
		if (gsMapItemDescCloseCleanupFrames != 0 && guiCurrentScreen == MAP_SCREEN)
		{
			--gsMapItemDescCloseCleanupFrames;
			fTeamPanelDirty          = TRUE;
			fCharacterInfoPanelDirty = TRUE;
		}
		return;
	}

	// Map screen: the item is gone, or another one is in its place -- close
	// the box, which was laid out for the item it opened on.
	if (guiCurrentItemDescriptionScreen == MAP_SCREEN &&
			gpItemDescObject != NULL && gpItemDescObject->usItem != gusItemDescShownItem)
	{
		DeleteItemDescriptionBox();
		return;
	}

	// This function now runs every frame (RenderTopmostTacticalInterface(),
	// Interface_Control.cc), not just on DIRTYLEVEL2 refreshes -- but the
	// ammo-eject button (giItemDescAmmoButton) is a GUI_BUTTON, rendered by
	// its own independent pass (RenderButtons(), called after this in the
	// same function) that only actually redraws a button when it's marked
	// dirty. Without this, this function's own background restore below
	// erases the button's pixels every frame (guiSAVEBUFFER, which it
	// restores from, doesn't contain the button's graphic), and
	// RenderButtons() wouldn't necessarily repaint it back the same frame --
	// making it flicker away to nothing.
	if (giItemDescAmmoButton) MarkAButtonDirty(giItemDescAmmoButton);
	if (giMapHideEmptySlotsCheckbox) MarkAButtonDirty(giMapHideEmptySlotsCheckbox);

	ST::string pStr;
	INT16   usX;
	INT16   usY;

	OBJECTTYPE const& obj    = *gpItemDescObject;
	// the map's box layouts, the 1366x768 tactical big weapon box too
	BOOLEAN    const  in_map = guiCurrentItemDescriptionScreen == MAP_SCREEN || IsTacBigItemDescBox();
	INT16      const  dx     = gsInvDescX;
	INT16      const  dy     = gsInvDescY;

	// Money keeps its own, already-established path (guiMoneyItemDescBox,
	// weapon-layout attachment math below left inert/unused as before);
	// everything that isn't a weapon (guns/blades/thrown/launchers --
	// isWeapon()) and isn't money now gets Infobox_items.sti with its own
	// 4-slot layout. Scoped to the tactical screen -- map screen
	// (iteminfoc.sti) is unaffected.
	bool const fIsMoney  = obj.usItem == MONEY;
	bool const fIsWeapon = GCM->getItem(obj.usItem)->isWeapon();

	// gDescNameBox variant for this render pass -- reused by every "item
	// name" style text below (name, weapon class/ammo line, money amount).
	SGPBox const descNameBox = TB(
		in_map    ? PickMapItemDesc(gMapDescNameBox, gMapDescNameBox_Items, gMapDescNameBox_Money, gMapDescNameBox_Weapons, gMapDescNameBox_Weapons_Big, gMapDescNameBox_Items_Big) :
		fIsMoney  ? gDescNameBox_Money :
		fIsWeapon ? gDescNameBox :
		            gDescNameBox_Items);

	auto * const box_gfx =
		in_map    ? GetMapItemDescBoxFilename() :
		fIsMoney  ? guiMoneyItemDescBox :
		fIsWeapon ? guiItemDescBox :
		            guiGenericItemDescBox;
	BltVideoObject(guiSAVEBUFFER, box_gfx, 0, dx, dy);

	// Display the money 'separating' border (iteminfoc_money_big has its
	// boxes in its art)
	if (obj.usItem == MONEY && !(gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_MONEY && gfMapItemDescArialBox))
	{
		// Render the money Boxes
		SGPPoint const xy = GetMoneyButtonPos(M_1000);
		INT32    const x  = xy.iX - 1;
		INT32    const y  = xy.iY;
		BltVideoObject(guiSAVEBUFFER, GetMoneyGraphicsFilename(), 0, x, y);
	}

	{
		// Display item
		// center in slot, remove offsets
		ETRLEObject const& e  = guiItemGraphic->SubregionProperties(guiItemGraphicIndex);
		SGPBox      const  xy = TB(
			in_map    ? PickMapItemDesc(g_desc_item_box_map, g_desc_item_box_map_items, g_desc_item_box_map_money, g_desc_item_box_map_weapons, g_desc_item_box_map_weapons_big, g_desc_item_box_map_items_big) :
			fIsMoney  ? g_desc_item_box_money :
			fIsWeapon ? g_desc_item_box :
			            g_desc_item_box_items);
		INT32       const  x  = dx + xy.x + (xy.w - e.usWidth)  / 2 - e.sOffsetX;
		INT32       const  y  = dy + xy.y + (xy.h - e.usHeight) / 2 - e.sOffsetY;
		if (gamepolicy(f_draw_item_shadow))
		{
			BltVideoObjectOutlineShadow(guiSAVEBUFFER, guiItemGraphic, guiItemGraphicIndex, x - 2, y + 2);
		}
		BltVideoObject(guiSAVEBUFFER, guiItemGraphic, guiItemGraphicIndex, x, y);
	}

	// Merc preview picture, Infobox.sti (weapon) only -- a single static
	// frame of the selected merc's own body animation (see
	// gMercPreviewFrames/GetMercPreviewFrame() above), coloured with the
	// merc's own clothing/hair/skin palette via pShades[] -- the same
	// mechanism RenderWorld.cc uses to draw mercs on the tactical map (see
	// pShadeTable = s.pShades[ubShadeLevel] there). Drawn at native (x1)
	// size, directly onto guiSAVEBUFFER, same as every other element in this
	// function.
	if (ENABLE_MERC_PREVIEW_PICTURE && !in_map && fIsWeapon && gusMercPreviewAnimSurface != INVALID_ANIMATION_SURFACE)
	{
		HVOBJECT const hMercVObject = gAnimSurfaceDatabase[gusMercPreviewAnimSurface].hVideoObject;
		if (hMercVObject && gpItemDescSoldier)
		{
			MercPreviewFrame previewFrame;
			if (GetMercPreviewFrame(*gpItemDescSoldier, &previewFrame))
			{
				SGPVSurface::Lock l(guiSAVEBUFFER);
				if (hMercVObject->IsRGBA())
				{
					Blt32BPPDataTo16BPPBufferShadeZ(l.Buffer<UINT16>(), l.Pitch(), NULL, 0, hMercVObject,
						MERC_PREVIEW_X, MERC_PREVIEW_Y, previewFrame.usImageIndex, NULL,
						gpItemDescSoldier->rgbaShades[DEFAULT_SHADE_LEVEL], &gpItemDescSoldier->rgbaRecolour, false, false, false);
				}
				else
				{
					Blt8BPPDataTo16BPPBufferTransShadow(l.Buffer<UINT16>(), l.Pitch(), hMercVObject,
						MERC_PREVIEW_X, MERC_PREVIEW_Y, previewFrame.usImageIndex,
						gpItemDescSoldier->pShades[DEFAULT_SHADE_LEVEL]);
				}
			}
		}
	}

	{ // Display status
		// (y is the bar's bottom: shifted by its top's block)
		SGPBox box =
			in_map    ? PickMapItemDesc(g_map_itemdesc_item_status_box, g_map_itemdesc_item_status_box_items, g_map_itemdesc_item_status_box_money, g_map_itemdesc_item_status_box_weapons, g_map_itemdesc_item_status_box_weapons_big, g_map_itemdesc_item_status_box_items_big) :
			fIsMoney  ? g_itemdesc_item_status_box_money :
			fIsWeapon ? g_itemdesc_item_status_box :
			            g_itemdesc_item_status_box_items;
		BoxShift const shift = TacBigWeaponShift(box.x, box.y - box.h);
		box.x += shift.dx;
		box.y += shift.dy;
		INT16  const  x   = box.x + dx;
		INT16  const  y   = box.y + dy;
		INT16  const  h   = box.h;
		DrawItemUIBarEx(obj, gubItemDescStatusIndex, x, y, h, Get16BPPColor(DESC_STATUS_BAR), Get16BPPColor(DESC_STATUS_BAR_SHADOW), guiSAVEBUFFER);
	}

	bool hatch_out_attachments = gfItemDescObjectIsAttachment; // if examining attachment, always hatch out attachment slots
	if (OBJECTTYPE const* const ptr_obj = gpItemPointer)
	{
		if (GCM->getItem(ptr_obj->usItem)->getFlags() & ITEM_HIDDEN_ADDON || (
			!ValidItemAttachment(&obj, ptr_obj->usItem, FALSE) &&
			!ValidMerge(ptr_obj->usItem, obj.usItem) &&
			!ValidLaunchable(ptr_obj->usItem, obj.usItem)))
		{
			hatch_out_attachments = TRUE;
		}
	}

	{
		// Display attachments
		AttachmentGfxInfo const& agi =
			in_map                 ? PickMapItemDesc(g_map_attachment_info, g_map_attachment_info_items, g_map_attachment_info, g_map_attachment_info_weapons, g_map_attachment_info_weapons_big, g_map_attachment_info_items_big, g_map_attachment_info) :
			fIsWeapon || fIsMoney  ? g_attachment_info :
			                         g_generic_item_attachment_info;
		// Non-weapon, non-money items only have 4 slots laid out in
		// g_generic_item_attachment_info -- the rest of its slot[] array is
		// unused placeholder data.
		// The map's iteminfoc_items box (g_map_attachment_info_items) has
		// the same 4.
		INT32 const numAttachmentSlots =
			gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS && gfMapItemDescArialBox ? BIG_ITEMS_ATTACHMENT_SLOTS :
			(!in_map && !fIsWeapon && !fIsMoney) || gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS ? GENERIC_ITEM_ATTACHMENT_SLOTS : MAX_ATTACHMENTS;
		for (INT32 i = 0; i < numAttachmentSlots; ++i)
		{
			SGPPoint const slot = AttachmentSlotPos(agi.slot, i);
			INT16 const x = dx + slot.iX;
			INT16 const y = dy + slot.iY;
			bool  const fSlotOccupied = obj.usAttachItem[i] != NOTHING;

			// Attachment slot frame: baked into iteminfoc.sti on the map
			// screen (unchanged, left alone below), but a separate per-slot
			// graphic on the tactical Infobox.sti. Excluded for both
			// Infobox_money.sti and Infobox_items.sti (fIsWeapon == false) --
			// those two graphics stay exactly as they were before this
			// feature, with no per-slot frame drawn over them at all; only
			// Infobox.sti (weapons) shows it.
			// The map's iteminfoc_weapons box has them too, with its own
			// "hide empty" option (gfMapHideEmptyAttachmentSlots).
			bool const fHideEmpty = in_map ? gfMapHideEmptyAttachmentSlots : fHideEmptyAttachmentSlots;
			// Not on iteminfoc_weapons_big, which has the frames in its art.
			if ((!in_map || gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_WEAPONS) && fIsWeapon && (!fHideEmpty || fSlotOccupied))
			{
				BltVideoObject(guiSAVEBUFFER, guiAttachmentSlotFrameVO, 0, x, y);
			}

			if (fSlotOccupied)
			{
				INT16 const item_x = agi.item_box.x + x;
				INT16 const item_y = agi.item_box.y + y;
				INT16 const item_w = agi.item_box.w;
				INT16 const item_h = agi.item_box.h;
				// the map's big weapon box: the large icons, as in the sector
				// inventory's "Show Large Icons"
				INVRenderItem(guiSAVEBUFFER, NULL, obj, item_x, item_y, item_w, item_h, DIRTYLEVEL2, RENDER_ITEM_ATTACHMENT1 + i, SGP_TRANSPARENT,
					IsMapBigWeaponBox() || (gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS && gfMapItemDescArialBox));

				INT16 const bar_x = agi.bar_box.x + x;
				INT16 const bar_h = agi.bar_box.h;
				INT16 const bar_y = agi.bar_box.y + y + bar_h - 1;
				DrawItemUIBarEx(obj, DRAW_ITEM_STATUS_ATTACHMENT1 + i, bar_x, bar_y, bar_h, Get16BPPColor(STATUS_BAR), Get16BPPColor(STATUS_BAR_SHADOW), guiSAVEBUFFER);
			}

			if (hatch_out_attachments)
			{
				UINT16 const hatch_w = agi.item_box.x + agi.item_box.w;
				UINT16 const hatch_h = agi.item_box.y + agi.item_box.h;
				DrawHatchOnInventory(guiSAVEBUFFER,
					x + ATTACHMENT_HATCH_OFFSET_X, y + ATTACHMENT_HATCH_OFFSET_Y,
					hatch_w + ATTACHMENT_HATCH_WIDTH_DELTA, hatch_h + ATTACHMENT_HATCH_HEIGHT_DELTA);
			}
		}
	}

	const ItemModel * item = GCM->getItem(obj.usItem);

	if (item->isGun())
	{
		// display bullets for ROF
		{
			INT32 const x = IsMapWeaponBox() ? dx + TB(IsMapBigWeaponBox() ? g_map_weapons_big_bullet_sing : g_map_weapons_bullet_sing).iX : in_map ? MAP_BULLET_SING_X : BULLET_SING_X;
			INT32 const y = IsMapWeaponBox() ? dy + TB(IsMapBigWeaponBox() ? g_map_weapons_big_bullet_sing : g_map_weapons_bullet_sing).iY : in_map ? MAP_BULLET_SING_Y : BULLET_SING_Y;
			BltVideoObject(guiSAVEBUFFER, guiBullet, 0, x, y);
		}

		const WeaponModel * w = GCM->getWeapon(obj.usItem);
		// not on the map's big weapon box: its Short / Long / Full lines have
		// their own rounds (RenderMapBigWeaponBoxTexts())
		if (w->ubShotsPerShortBurst > 0 && !IsMapBigWeaponBox())
		{
			INT32       x = IsMapWeaponBox() ? dx + g_map_weapons_bullet_burst.iX : in_map ? MAP_BULLET_BURST_X : BULLET_BURST_X;
			INT32 const y = IsMapWeaponBox() ? dy + g_map_weapons_bullet_burst.iY : in_map ? MAP_BULLET_BURST_Y : BULLET_BURST_Y;
			// the item description shows the short burst, see docs/burst-modes.md
			for (INT32 i = GunShotsPerBurst(obj, WM_BURST_SHORT); i != 0; --i)
			{
				BltVideoObject(guiSAVEBUFFER, guiBullet, 0, x, y);
				x += BULLET_WIDTH + 1;
			}
		}

		if ((!in_map || gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_WEAPONS) && w->ubShotsPerShortBurst > 0)
		{
			// Purely decorative: always exactly 3 bullet icons, regardless of
			// the weapon's actual burst size — cosmetic only. Shown under the
			// same condition as the real burst bullets/stats above: only for
			// weapons that have a burst mode at all.
			INT32 x = IsMapWeaponBox() ? dx + g_map_weapons_bullet_decor.iX : BULLET_DECOR_X;
			INT32 const y = IsMapWeaponBox() ? dy + g_map_weapons_bullet_decor.iY : BULLET_DECOR_Y;
			for (INT32 i = 0; i < 3; ++i)
			{
				BltVideoObject(guiSAVEBUFFER, guiBullet, 0, x, y);
				x += BULLET_WIDTH + 1;
			}
		}
	}

	{
		INT16 const w =
			in_map    ? GetMapItemDescWidth() :
			fIsMoney  ? ITEMDESC_WIDTH_MONEY :
			fIsWeapon ? ITEMDESC_WIDTH :
			            ITEMDESC_WIDTH_ITEMS;
		INT16 const h =
			in_map    ? GetMapItemDescHeight() :
			fIsMoney  ? ITEMDESC_HEIGHT_MONEY :
			fIsWeapon ? ITEMDESC_HEIGHT :
			            ITEMDESC_HEIGHT_ITEMS;
		RestoreExternBackgroundRect(dx, dy, w, h);
	}

	// Render font desc
	SetFontAttributes(ITEMDESC_FONT, FONT_FCOLOR_WHITE);

	{
		// Render name -- on the map's big weapon box in FONT14ARIAL, in the
		// colour of its MAINTENANCE / BALLISTICS headers
		SGPBox const& xy = descNameBox;
		if (IsMapBigWeaponBox())
		{
			SetFont(FONT14ARIAL);
			SetFontForeground(MAP_BIG_WEAPON_VALUE_COLOUR);
		}
		else if (gfMapItemDescArialBox)
		{
			SetFont(FONT12ARIAL);
		}
		MPrint(dx + xy.x, dy + xy.y, gzItemName);
		SetFont(ITEMDESC_FONT);
		SetFontForeground(FONT_FCOLOR_WHITE);
	}

	// Same shadow colour as the weapon name above (DEFAULT_SHADOW, set by the
	// SetFontAttributes() call further up) - covers the description text
	// below and, since nothing resets it in between, the weapon class + ammo
	// type line too.
	SetFontShadow(DEFAULT_SHADOW);

	{
		// Weapon description text: same font colour as the weapon name above.
		SGPBox const box = TB(
			in_map    ? PickMapItemDesc(g_map_itemdesc_desc_box, g_map_itemdesc_desc_box_items, g_map_itemdesc_desc_box_money, g_map_itemdesc_desc_box_weapons, g_map_itemdesc_desc_box_weapons_big, g_map_itemdesc_desc_box_items_big) :
			fIsMoney  ? g_itemdesc_desc_box_money :
			fIsWeapon ? g_itemdesc_desc_box :
			            g_itemdesc_desc_box_items);
		if (IsMapBigWeaponBox())
		{
			// FONT12ARIAL on the map's big weapon box, one line
			PrintLinesWithEllipsis(dx + box.x, dy + box.y, box.w, GetFontHeight(FONT12ARIAL) + MAP_BIG_WEAPON_DESC_LINE_GAP, MAP_BIG_WEAPON_DESC_LINES, FONT12ARIAL, gzItemDesc);
		}
		else
		{
			// justified on the map's items boxes, normal and "Show Large Icons",
			// and on the money box of "Show Large Icons" (laid out as the latter)
			UINT32 const justify =
				gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS ||
				(gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_MONEY && gfMapItemDescArialBox) ? FULL_JUSTIFIED : LEFT_JUSTIFIED;
			DisplayWrappedString(dx + box.x, dy + box.y, box.w, 2, gfMapItemDescArialBox ? FONT12ARIAL : ITEMDESC_FONT, FONT_FCOLOR_WHITE, gzItemDesc, FONT_MCOLOR_BLACK, justify);
		}
	}

	if (ITEM_PROS_AND_CONS(obj.usItem))
	{
		{
			const WeaponModel * w = GCM->getWeapon(obj.usItem);
			if (w->calibre->index != CalibreModel::NOAMMO)
			{
				ST::string name = *w->calibre->getName();
				pStr += ST::format("{} ", name);
			}
			pStr += ST::format("{}", WeaponType[w->ubWeaponType]);
			ST::string imprint = GetObjectImprint(obj);
			if (!imprint.empty())
			{
				// Add name noting imprint
				pStr += ST::format(" ({})", imprint);
			}

			// Weapon class + ammo type: same font colour as the weapon name
			// above (explicit, rather than relying on whatever foreground
			// DisplayWrappedString() happened to leave set globally).
			SetFontForeground(IsMapBigWeaponBox() ? MAP_BIG_WEAPON_VALUE_COLOUR : FONT_FCOLOR_WHITE);

			// FONT14ARIAL on the map's big weapon box, like the name
			SGPFont const name_font = IsMapBigWeaponBox() ? FONT14ARIAL : ITEMDESC_FONT;
			SetFont(name_font);
			SGPBox const& xy = descNameBox;
			FindFontRightCoordinates(dx + xy.x, dy + xy.y, xy.w, xy.h, pStr, name_font, &usX, &usY);
			MPrint(usX, usY, pStr);
			SetFont(ITEMDESC_FONT);
		}

		{
			if (IsMapBigWeaponBox())
			{
				// the map's big weapon box: one line, pros, then cons from
				// x 350, both in FONT12ARIAL and ending in "..." if too long
				SGPBox const pros_box = TB(g_map_itemdesc_pros_cons_box_weapons_big);
				SGPBox const cons_box = TB(g_map_itemdesc_cons_box_weapons_big);
				SGPFont const font     = FONT12ARIAL;
				INT16   const indent   = std::max(StringPixLength(gzProsLabel, font), StringPixLength(gzConsLabel, font)) + 10;
				INT16   const dots     = StringPixLength(DOTDOTDOT, font);

				SetFont(font);
				SetFontForeground(FONT_MCOLOR_DKWHITE2);
				SetFontShadow(DEFAULT_SHADOW);
				MPrint(dx + pros_box.x, dy + pros_box.y, gzProsLabel);
				MPrint(dx + cons_box.x, dy + cons_box.y, gzConsLabel);

				SetFontForeground(MAP_BIG_WEAPON_PROS_CONS_COLOUR);
				SetFontShadow(DEFAULT_SHADOW);

				gProsConsFont = font;
				GenerateProsString(gzItemPros, obj, pros_box.w - indent - dots);
				GenerateConsString(gzItemCons, obj, cons_box.w - indent - dots);
				gProsConsFont = NULL;
				MPrint(dx + pros_box.x + indent, dy + pros_box.y, gzItemPros);
				MPrint(dx + cons_box.x + indent, dy + cons_box.y, gzItemCons);
				SetFont(ITEMDESC_FONT);
			}
			else
			{
			SGPBox const& box = IsMapWeaponBox() ? g_map_itemdesc_pros_cons_box_weapons : in_map ? g_map_itemdesc_pros_cons_box : g_itemdesc_pros_cons_box;
			INT32         x   = box.x + dx;
			INT32  const  y   = box.y + dy;
			INT32         w   = box.w;
			INT32  const  h   = box.h;

			SetFontForeground(FONT_MCOLOR_DKWHITE2);
			SetFontShadow(DEFAULT_SHADOW);
			MPrint(x, y,     gzProsLabel);
			MPrint(x, y + h, gzConsLabel);

			SetFontForeground(FONT_BLACK);
			SetFontShadow(ITEMDESC_FONTSHADOW2);

			INT16 const pros_cons_indent = std::max(StringPixLength(gzProsLabel, ITEMDESC_FONT), StringPixLength(gzConsLabel, ITEMDESC_FONT)) + 10;
			x += pros_cons_indent;
			w -= pros_cons_indent + StringPixLength(DOTDOTDOT, ITEMDESC_FONT);

			GenerateProsString(gzItemPros, obj, w);
			MPrint(x, y, gzItemPros);

			GenerateConsString(gzItemCons, obj, w);
			MPrint(x, y + h, gzItemCons);
			}
		}
	}

	// Calculate total weight of item and attachments
	grams const objectWeight = Weight(obj);
	double const convertedWeight = objectWeight /
		(gGameSettings.fOptions[TOPTION_USE_METRIC_SYSTEM]
		? 1000.0      // Weight in kilograms
		: 453.59237); // Weight in pounds

	SetFontShadow(DEFAULT_SHADOW);

	// Render, stat  name
	// Items and money on the "Show Large Icons" boxes (gfMapItemDescArialBox):
	// FONT12ARIAL, in the nearest colours of its palette, where 5 and 6 are
	// black.
	SGPFont const misc_font         = gfMapItemDescArialBox ? FONT12ARIAL : BLOCKFONT2;
	INT16   const misc_h            = gfMapItemDescArialBox ? GetFontHeight(FONT12ARIAL) : ITEM_STATS_HEIGHT;
	UINT8   const misc_label_colour = gfMapItemDescArialBox ? MAP_BIG_WEAPON_LABEL_COLOUR : 6;
	UINT8   const misc_value_colour = gfMapItemDescArialBox ? MAP_BIG_WEAPON_VALUE_COLOUR : 5;

	if (item->isWeapon())
	{
		// The map's big weapon box (iteminfoc_weapons_big) has the stats in
		// FONT12ARIAL, and Short / Long / Full lines of its own in place of
		// the burst AP cost (see RenderMapBigWeaponBoxTexts()).
		bool    const big        = IsMapBigWeaponBox();
		SGPFont const stats_font = big ? FONT12ARIAL : BLOCKFONT2;
		INT16   const stats_h    = big ? GetFontHeight(FONT12ARIAL) : ITEM_STATS_HEIGHT;
		UINT8   const label_colour = big ? MAP_BIG_WEAPON_LABEL_COLOUR : ITEMDESC_LABEL_COLOUR;
		UINT8   const value_colour = big ? MAP_BIG_WEAPON_VALUE_COLOUR : ITEMDESC_VALUE_COLOUR;
		if (big) SetFont(stats_font);

		SetFontForeground(label_colour);

		INV_DESC_STATS const* ids =
			IsMapBigWeaponBox() ? gMapBigWeaponBoxStats :
			IsMapWeaponBox() ? gMapWeaponBoxStats :
			in_map           ? gMapWeaponStats :
			                   gWeaponStats;
		// the tactical big box: the same, each moved with its block
		std::vector<INV_DESC_STATS> tac_ids;
		if (gfTacBigWeaponBox)
		{
			for (INV_DESC_STATS const& t : gMapBigWeaponBoxStats) tac_ids.push_back(TB(t));
			ids = tac_ids.data();
		}

		//LABELS
		MPrint(dx + ids[0].sX, dy + ids[0].sY, st_format_printf(gWeaponStatsDesc[0], GetWeightUnitString())); // mass
		if (item->getItemClass() & (IC_GUN | IC_LAUNCHER))
		{
			MPrint(dx + ids[2].sX, dy + ids[2].sY, gWeaponStatsDesc[3]); // range
		}
		if (!(item->isLauncher()) && obj.usItem != ROCKET_LAUNCHER)
		{
			MPrint(dx + ids[3].sX, dy + ids[3].sY, gWeaponStatsDesc[4]); // damage
		}
		MPrint(dx + ids[4].sX, dy + ids[4].sY, gWeaponStatsDesc[5]); // APs
		if (item->isGun())
		{
			MPrint(dx + ids[6].sX, dy + ids[6].sY, gWeaponStatsDesc[6]); // = (sic)
		}
		MPrint(dx + ids[1].sX, dy + ids[1].sY, gWeaponStatsDesc[1]); // status

		const WeaponModel * w = GCM->getWeapon(obj.usItem);
		if (w->ubShotsPerShortBurst > 0 && !big)
		{
			MPrint(dx + ids[7].sX, dy + ids[7].sY, gWeaponStatsDesc[6]); // = (sic)
		}

		//Status
		SetFontForeground(value_colour);
		pStr = ST::format("{2d}%", obj.bGunStatus);
		FindFontRightCoordinates(dx + ids[1].sX + ids[1].sValDx, dy + ids[1].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
		MPrint(usX, usY, pStr);

		//Weight
		HighlightIf(objectWeight <= EXCEPTIONAL_WEIGHT, value_colour);
		pStr = ST::format("{1.1f}", convertedWeight);
		FindFontRightCoordinates(dx + ids[0].sX + ids[0].sValDx, dy + ids[0].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
		MPrint(usX, usY, pStr);

		if (item->getItemClass() & (IC_GUN | IC_LAUNCHER))
		{
			// Range
			UINT16 const range = GunRange(obj);
			HighlightIf(range >= EXCEPTIONAL_RANGE, value_colour);
			pStr = ST::format("{2d}", range / 10);
			FindFontRightCoordinates(dx + ids[2].sX + ids[2].sValDx, dy + ids[2].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}

		if (!(item->isLauncher()) && obj.usItem != ROCKET_LAUNCHER)
		{
			// Damage
			HighlightIf(w->ubImpact >= EXCEPTIONAL_DAMAGE, value_colour);
			pStr = ST::format("{2d}", w->ubImpact);
			FindFontRightCoordinates(dx + ids[3].sX + ids[3].sValDx, dy + ids[3].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}

		UINT8 const ubAttackAPs = BaseAPsToShootOrStab(DEFAULT_APS, DEFAULT_AIMSKILL, obj);

		//APs
		HighlightIf(ubAttackAPs <= EXCEPTIONAL_AP_COST, value_colour);
		pStr = ST::format("{2d}", ubAttackAPs);
		FindFontRightCoordinates(dx + ids[4].sX + ids[4].sValDx, dy + ids[4].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
		MPrint(usX, usY, pStr);

		if (w->ubShotsPerShortBurst > 0 && !big)
		{
			HighlightIf(GunShotsPerBurst(obj, WM_BURST_SHORT) >= EXCEPTIONAL_BURST_SIZE || obj.usItem == G11, value_colour);
			pStr = ST::format("{2d}", ubAttackAPs + CalcAPsToBurst(DEFAULT_APS, DEFAULT_AIMSKILL, obj, WM_BURST_SHORT));
			FindFontRightCoordinates(dx + ids[5].sX + ids[5].sValDx, dy + ids[5].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}

		if (!in_map || IsMapWeaponBox())
		{
			// Previously-hidden stats (ubReadyTime, ubBurstPenalty, ubAttackVolume,
			// ubHitVolume, ubDeadliness, bReliability, bRepairEase) plus the actual
			// AP cost to reload, shown in the extra room made by the enlarged box.
			bool const showReload = (item->getItemClass() & (IC_GUN | IC_LAUNCHER)) != 0;

			SetFontForeground(label_colour);
			MPrint(dx + ids[8].sX,  dy + ids[8].sY,  gWeaponStatsDesc[7]);  // Ready time
			MPrint(dx + ids[9].sX,  dy + ids[9].sY,  gWeaponStatsDesc[8]);  // Reliability
			if (w->ubShotsPerShortBurst > 0)
			{
				MPrint(dx + ids[10].sX, dy + ids[10].sY, gWeaponStatsDesc[9]); // Burst penalty
			}
			MPrint(dx + ids[11].sX, dy + ids[11].sY, gWeaponStatsDesc[10]); // Repair ease
			MPrint(dx + ids[12].sX, dy + ids[12].sY, gWeaponStatsDesc[11]); // Attack volume
			MPrint(dx + ids[13].sX, dy + ids[13].sY, gWeaponStatsDesc[12]); // Deadliness
			MPrint(dx + ids[14].sX, dy + ids[14].sY, gWeaponStatsDesc[13]); // Hit volume
			if (showReload)
			{
				MPrint(dx + ids[15].sX, dy + ids[15].sY, gWeaponStatsDesc[14]); // Reload AP cost
			}

			// Purely decorative labels — cosmetic only, no associated value,
			// not tied to any real stat (unlike every label above). Shown
			// under the same condition as the real burst stats: only for
			// weapons that have a burst mode at all.
			if (w->ubShotsPerShortBurst > 0 && !big)
			{
				MPrint(dx + ids[18].sX, dy + ids[18].sY, gWeaponStatsDesc[15]); // "AP:" (decorative)
				MPrint(dx + ids[19].sX, dy + ids[19].sY, gWeaponStatsDesc[16]); // "Rounds" (decorative)
			}

			// "Base:" label always shown; the value next to it (below) only
			// appears once the weapon actually has a LASERSCOPE (attached or
			// built into a rocket rifle).
			MPrint(dx + ids[20].sX, dy + ids[20].sY, gWeaponStatsDesc[17]); // "Base:"

			// "Prone:" label AND value are always shown - it's the combined
			// LASERSCOPE + BIPOD bonus (0 when neither is attached).
			MPrint(dx + ids[21].sX, dy + ids[21].sY, gWeaponStatsDesc[18]); // "Prone:"

			// "Per Aim:" label AND value are always shown - standalone
			// SNIPERSCOPE display bonus, never summed with Base:/Prone:.
			MPrint(dx + ids[22].sX, dy + ids[22].sY, gWeaponStatsDesc[19]); // "Per Aim:"

			SetFontForeground(value_colour);

			// Ready time: APs needed to ready/unready this weapon
			pStr = ST::format("{2d}", w->ubReadyTime);
			FindFontRightCoordinates(dx + ids[8].sX + ids[8].sValDx, dy + ids[8].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);

			// Reliability: comparative rating, higher = less prone to jamming
			pStr = ST::format("{2d}", item->getReliability());
			FindFontRightCoordinates(dx + ids[9].sX + ids[9].sValDx, dy + ids[9].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);

			// Burst penalty: % accuracy penalty per shot after the first, in a burst
			if (w->ubShotsPerShortBurst > 0)
			{
				pStr = ST::format("{2d}%", w->ubBurstPenalty);
				FindFontRightCoordinates(dx + ids[10].sX + ids[10].sValDx, dy + ids[10].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
				MPrint(usX, usY, pStr);
			}

			// Repair ease: comparative rating, higher = cheaper/easier to repair
			pStr = ST::format("{2d}", item->getRepairEase());
			FindFontRightCoordinates(dx + ids[11].sX + ids[11].sValDx, dy + ids[11].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);

			// Attack volume: noise made when firing this weapon (reflects a
			// silencer attachment, unlike the raw ubAttackVolume field)
			pStr = ST::format("{3d}", GunAttackVolume(obj));
			FindFontRightCoordinates(dx + ids[12].sX + ids[12].sValDx, dy + ids[12].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);

			// Deadliness: comparative lethality rating used by the AI
			pStr = ST::format("{3d}", w->ubDeadliness);
			FindFontRightCoordinates(dx + ids[13].sX + ids[13].sValDx, dy + ids[13].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);

			// Hit volume: noise made when a shot from this weapon hits something
			pStr = ST::format("{2d}", w->ubHitVolume);
			FindFontRightCoordinates(dx + ids[14].sX + ids[14].sValDx, dy + ids[14].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
			MPrint(usX, usY, pStr);

			// Reload cost, in APs: base cost, doubled when the currently loaded
			// ammo's capacity doesn't match the weapon's magazine size — mirrors
			// GetAPsToReloadGunWithAmmo() in Points.cc.
			if (showReload)
			{
				INT8 reloadAP = AP_RELOAD_GUN;
				if (item->getItemClass() != IC_LAUNCHER && obj.usGunAmmoItem != NOTHING)
				{
					const ItemModel* const ammoItem = GCM->getItem(obj.usGunAmmoItem);
					if (ammoItem->isAmmo() && !w->isSameMagCapacity(ammoItem->asAmmo()))
					{
						reloadAP = AP_RELOAD_GUN + AP_RELOAD_GUN;
					}
				}
				pStr = ST::format("{2d}", reloadAP);
				FindFontRightCoordinates(dx + ids[15].sX + ids[15].sValDx, dy + ids[15].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
				MPrint(usX, usY, pStr);
			}

			// Combined LASERSCOPE aim bonus (see GunLaserScopeBonus()) + the
			// merc's own live crouch-stance bonus (see
			// GunCrouchStanceBonus()) + the merc's own live roof/elevation
			// bonus (see GunRoofBonus()). Always shown (0 when none apply),
			// always signed: "+" when the net effect helps, "-" when a
			// badly damaged laser scope is hurting aim more than the other
			// bonuses offset.
			{
				INT8 const baseBonus = GunLaserScopeBonus(obj) + GunCrouchStanceBonus(gpItemDescSoldier) + GunRoofBonus(gpItemDescSoldier);
				pStr = ST::format("{}{}%", baseBonus >= 0 ? "+" : "", baseBonus);
				FindFontRightCoordinates(dx + ids[20].sX + ids[20].sValDx, dy + ids[20].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
				MPrint(usX, usY, pStr);
			}

			// Combined "prone" aim bonus: LASERSCOPE bonus (always active)
			// plus BIPOD's simplified display bonus, plus the merc's own
			// simplified prone-stance bonus, plus the merc's own live
			// roof/elevation bonus (see GunBipodDisplayBonus()/
			// GunProneStanceBonus()/GunRoofBonus()). Always shown, even
			// when nothing applies (then 0).
			{
				INT8 const proneBonus = GunLaserScopeBonus(obj) + GunBipodDisplayBonus(obj) + GunProneStanceBonus(gpItemDescSoldier) + GunRoofBonus(gpItemDescSoldier);
				pStr = ST::format("{}{}%", proneBonus >= 0 ? "+" : "", proneBonus);
				FindFontRightCoordinates(dx + ids[21].sX + ids[21].sValDx, dy + ids[21].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
				MPrint(usX, usY, pStr);
			}

			// Standalone SNIPERSCOPE display bonus (see
			// GunSniperScopeDisplayBonus()) - always shown, on its own, never
			// added into Base:/Prone:.
			{
				INT8 const sniperBonus = GunSniperScopeDisplayBonus(obj);
				pStr = ST::format("{}{}%", sniperBonus >= 0 ? "+" : "", sniperBonus);
				FindFontRightCoordinates(dx + ids[22].sX + ids[22].sValDx, dy + ids[22].sY, ITEM_STATS_WIDTH, stats_h, pStr, stats_font, &usX, &usY);
				MPrint(usX, usY, pStr);
			}
		}

		if (IsMapBigWeaponBox()) RenderMapBigWeaponBoxTexts(obj, dx, dy);
	}
	else if (obj.usItem == MONEY)
	{
		SetFont(misc_font);
		SetFontForeground(FONT_WHITE);

		// The map's own way of printing the money box (ItemInfoC.sti); its
		// iteminfoc_money box is printed the tactical way, like its positions.
		bool const map_style = in_map && gMapItemDescLayout != MAP_ITEMDESC_LAYOUT_MONEY;

		{
			// Display the total amount of money
			pStr = SPrintMoney(in_map && gfAddingMoneyToMercFromPlayersAccount ? LaptopSaveInfo.iCurrentBalance : gRemoveMoney.uiTotalAmount);
			SGPBox const& xy = descNameBox;
			FindFontRightCoordinates(dx + xy.x, dy + xy.y, xy.w, xy.h, pStr, misc_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}

		{
			// Display the 'Separate' text
			SetFontForeground(map_style ? 5 : misc_label_colour);
			SGPPoint const xy = GetMoneyButtonPos(4);
			ST::string label = gfAddingMoneyToMercFromPlayersAccount ? gzMoneyAmounts[5] : gzMoneyAmounts[4];
			if (gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_MONEY && gfMapItemDescArialBox)
			{
				SGPBox const& box = MAP_MONEY_BIG_SEPARATE_BOX;
				FindFontCenterCoordinates(dx + box.x, dy + box.y, box.w, box.h, label, misc_font, &usX, &usY);
				MPrint(usX, usY, label);
			}
			else
			{
				MPrint(xy.iX, xy.iY, label);
			}
		}

		SetFontForeground(misc_label_colour);

		INV_DESC_STATS const* const xy =
			!in_map                                         ? gMoneyStats :
			gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_MONEY ? (gfMapItemDescArialBox ? gMapMoneyStats_MoneyBig : gMapMoneyStats_Money) :
			                                                  gMapMoneyStats;

		if (gfAddingMoneyToMercFromPlayersAccount)
		{
			MPrint(dx + xy[0].sX, dy + xy[0].sY, gMoneyStatsDesc[MONEY_DESC_PLAYERS]);           // current ...
			MPrint(dx + xy[1].sX, dy + xy[1].sY, gMoneyStatsDesc[MONEY_DESC_BALANCE]);           // ... balance
			MPrint(dx + xy[2].sX, dy + xy[2].sY, gMoneyStatsDesc[MONEY_DESC_AMOUNT_2_WITHDRAW]); // amount to ...
			MPrint(dx + xy[3].sX, dy + xy[3].sY, gMoneyStatsDesc[MONEY_DESC_TO_WITHDRAW]);       // ... widthdraw
		}
		else
		{
			MPrint(dx + xy[0].sX, dy + xy[0].sY, gMoneyStatsDesc[MONEY_DESC_AMOUNT]);         // amount ...
			MPrint(dx + xy[1].sX, dy + xy[1].sY, gMoneyStatsDesc[MONEY_DESC_REMAINING]);      // ... remaining
			MPrint(dx + xy[2].sX, dy + xy[2].sY, gMoneyStatsDesc[MONEY_DESC_AMOUNT_2_SPLIT]); // amount ...
			MPrint(dx + xy[3].sX, dy + xy[3].sY, gMoneyStatsDesc[MONEY_DESC_TO_SPLIT]);       // ... to split
		}

		SetFontForeground(misc_value_colour);

		// Get length of string
		UINT16 const uiRightLength = 35;
		// the values' width, the tactical way
		INT16  const value_w = gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_MONEY && gfMapItemDescArialBox ? MAP_MONEY_BIG_VALUE_WIDTH : ITEM_STATS_WIDTH - 3;

		//Display the total amount of money remaining
		pStr = SPrintMoney(gRemoveMoney.uiMoneyRemaining);
		if (map_style)
		{
			UINT16 const uiStringLength = StringPixLength(pStr, ITEMDESC_FONT);
			INT16  const sStrX          = dx + xy[1].sX + xy[1].sValDx + (uiRightLength - uiStringLength);
			MPrint(sStrX, dy + xy[1].sY, pStr);
		}
		else
		{
			FindFontRightCoordinates(dx + xy[1].sX + xy[1].sValDx, dy + xy[1].sY, value_w, misc_h, pStr, misc_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}

		//Display the total amount of money removing
		pStr = SPrintMoney(gRemoveMoney.uiMoneyRemoving);
		if (map_style)
		{
			UINT16 const uiStringLength = StringPixLength(pStr, ITEMDESC_FONT);
			INT16  const sStrX          = dx + xy[3].sX + xy[3].sValDx + (uiRightLength - uiStringLength);
			MPrint(sStrX, dy + xy[3].sY, pStr);
		}
		else
		{
			FindFontRightCoordinates(dx + xy[3].sX + xy[3].sValDx, dy + xy[3].sY, value_w, misc_h, pStr, misc_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}
	}
	else if (item->getItemClass() == IC_MONEY)
	{
		SetFontForeground(FONT_FCOLOR_WHITE);
		pStr = SPrintMoney(obj.uiMoneyAmount);
		SGPBox const& xy = descNameBox;
		FindFontRightCoordinates(dx + xy.x, dy + xy.y, xy.w, xy.h, pStr, misc_font, &usX, &usY);
		MPrint(usX, usY, pStr);
	}
	else
	{
		//Labels
		SetFont(misc_font);
		SetFontForeground(misc_label_colour);

		// Infobox_items.sti gets its own Weight/Status positions
		// (gGenericItemStats) instead of reusing the weapon box's
		// (gWeaponStats). Map screen (iteminfoc.sti): gMapGenericItemStats
		// on the 1366x768 base with the normal merc inventory, else still
		// gMapWeaponStats, shared with every other item class there.
		INV_DESC_STATS const* const ids =
			!in_map                                         ? gGenericItemStats :
			gMapItemDescLayout == MAP_ITEMDESC_LAYOUT_ITEMS ? (gfMapItemDescArialBox ? gMapGenericItemStats_Big : gMapGenericItemStats) :
			                                                  gMapWeaponStats;

		// amount for ammunition, status otherwise
		ST::string label = GCM->getItem(gpItemDescObject->usItem)->isAmmo() ? gWeaponStatsDesc[2] : gWeaponStatsDesc[1];
		MPrint(dx + ids[1].sX, dy + ids[1].sY, label);

		//Weight
		MPrint(dx + ids[0].sX, dy + ids[0].sY, st_format_printf(gWeaponStatsDesc[0], GetWeightUnitString()));

		// Values
		SetFontForeground(misc_value_colour);

		if (item->isAmmo())
		{
			// Ammo - print amount
			pStr = ST::format("{}/{}", obj.ubShotsLeft[gubItemDescStatusIndex], item->asAmmo()->capacity);
			FindFontRightCoordinates(dx + ids[1].sX + ids[1].sValDx, dy + ids[1].sY, ITEM_STATS_WIDTH, misc_h, pStr, misc_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}
		else
		{
			// Status
			pStr = ST::format("{2d}%", obj.bStatus[gubItemDescStatusIndex]);
			FindFontRightCoordinates(dx + ids[1].sX + ids[1].sValDx, dy + ids[1].sY, ITEM_STATS_WIDTH, misc_h, pStr, misc_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}

		//Weight
		pStr = ST::format("{1.1f}", convertedWeight);
		FindFontRightCoordinates(dx + ids[0].sX + ids[0].sValDx, dy + ids[0].sY, ITEM_STATS_WIDTH, misc_h, pStr, misc_font, &usX, &usY);
		MPrint(usX, usY, pStr);

		if (InKeyRingPopup() || item->isKey())
		{
			SetFontForeground(misc_label_colour);

			INT32 const x  = dx + ids[3].sX;
			INT32 const y0 = dy + ids[3].sY;
			// the "Show Large Icons" box: its own line pitch and width
			INT32 const y1 = y0 + (gfMapItemDescArialBox ? BIG_ITEMS_KEY_LINE_PITCH : GetFontHeight(BLOCKFONT) + 2);
			INT16 const key_w = gfMapItemDescArialBox ? BIG_ITEMS_KEY_WIDTH : 113;

			// build description for keys .. the sector found
			MPrint(x, y0, sKeyDescriptionStrings[0]);
			MPrint(x, y1, sKeyDescriptionStrings[1]);

			KEY const& key = KeyTable[obj.ubKeyID];

			SetFontForeground(misc_value_colour);
			ST::string sTempString = SGPSector(key.usSectorFound).AsShortString();
			FindFontRightCoordinates(x, y0, key_w, misc_h, sTempString, misc_font, &usX, &usY);
			MPrint(usX, usY, sTempString);

			pStr = ST::format("{}", key.usDateFound);
			FindFontRightCoordinates(x, y1, key_w, misc_h, pStr, misc_font, &usX, &usY);
			MPrint(usX, usY, pStr);
		}
	}
}


void HandleItemDescriptionBox(DirtyLevel* const dirty_level)
{
	if ( fItemDescDelete )
	{
		DeleteItemDescriptionBox( );
		fItemDescDelete = FALSE;
		*dirty_level = DIRTYLEVEL2;
	}

}


void DeleteItemDescriptionBox( )
{
	INT32 cnt, cnt2;
	BOOLEAN	fFound, fAllFound;

	if (!gfInItemDescBox) return;

	//DEF:

	//Used in the shopkeeper interface
	if (guiCurrentScreen == SHOPKEEPER_SCREEN)
	{
		DeleteShopKeeperItemDescBox();
	}

	// check for any AP costs
	if (gTacticalStatus.uiFlags & INCOMBAT)
	{
		if (gpAttachSoldier)
		{
			// check for change in attachments, starting with removed attachments
			fAllFound = TRUE;
			for ( cnt = 0; cnt < MAX_ATTACHMENTS; cnt++ )
			{
				if ( gusOriginalAttachItem[ cnt ] != NOTHING )
				{
					fFound = FALSE;
					for ( cnt2 = 0; cnt2 < MAX_ATTACHMENTS; cnt2++ )
					{
						if ( (gusOriginalAttachItem[ cnt ] == gpItemDescObject->usAttachItem[ cnt2 ]) && (gpItemDescObject->bAttachStatus[ cnt2 ] == gbOriginalAttachStatus[ cnt ]) )
						{
							fFound = TRUE;
						}
					}
					if (!fFound)
					{
						// charge APs
						fAllFound = FALSE;
						break;
					}
				}
			}

			if (fAllFound)
			{
				// nothing was removed; search for attachment added
				for ( cnt = 0; cnt < MAX_ATTACHMENTS; cnt++ )
				{
					if ( gpItemDescObject->usAttachItem[ cnt ] != NOTHING )
					{
						fFound = FALSE;
						for ( cnt2 = 0; cnt2 < MAX_ATTACHMENTS; cnt2++ )
						{
							if ( (gpItemDescObject->usAttachItem[ cnt ] == gusOriginalAttachItem[ cnt2 ]) && (gbOriginalAttachStatus[ cnt2 ] == gpItemDescObject->bAttachStatus[ cnt ]) )
							{
								fFound = TRUE;
							}
						}
						if (!fFound)
						{
							// charge APs
							fAllFound = FALSE;
							break;
						}
					}
				}
			}

			if (!fAllFound)
			{
				DeductPoints( gpAttachSoldier, AP_RELOAD_GUN, 0 );
			}
		}
	}

	RemoveVObject(guiItemDescBox);
	RemoveVObject(GetMapItemDescBoxFilename());
	RemoveVObject(guiMoneyItemDescBox);
	RemoveVObject(guiBullet);
	DeleteVideoObject(guiItemGraphic);

	// Merc preview picture (see InternalInitItemDescriptionBox()) -- only
	// ever loaded for the weapon box, so only ever needs unloading here.
	if (gusMercPreviewAnimSurface != INVALID_ANIMATION_SURFACE && gpItemDescSoldier)
	{
		if (gfMercPreviewSurfaceLocked)
		{
			UnLoadAnimationSurface(SOLDIER2ID(gpItemDescSoldier), gusMercPreviewAnimSurface);
		}
		gfMercPreviewSurfaceLocked = false;
		gusMercPreviewAnimSurface  = INVALID_ANIMATION_SURFACE;
	}

	gfInItemDescBox = FALSE;

	// the viewport the 1366x768 weapon box shortened: back to its panel's top
	if (gfTacBigWeaponBox)
	{
		gsVIEWPORT_WINDOW_END_Y = gsCurInterfacePanel == SM_PANEL ? INV_INTERFACE_START_Y : TEAM_PANEL_Y;
		SetRenderFlags(RENDER_FLAG_FULL);
	}

	if (guiCurrentItemDescriptionScreen != MAP_SCREEN && gsCurInterfacePanel == SM_PANEL)
	{
		// Infobox.sti can be taller than the SM panel's own protected footer
		// (ITEMDESC_PANEL_HEIGHT vs INV_INTERFACE_HEIGHT -- see UILayout.h),
		// so part of it is drawn above the area DIRTYLEVEL2 normally
		// repaints on close. Force a full world redraw (bounded by the
		// fixed gsVIEWPORT_END_Y, which already reaches higher than
		// ITEMDESC_PANEL_START_Y) so no remnants of the box are left behind.
		// Deliberately NOT touching gsVIEWPORT_WINDOW_END_Y here (unlike a
		// prior version of this fix) -- that's also used by the smooth-
		// scroll shift-blit, and raising it made the whole protected strip
		// visibly pan with the map while the box was open.
		SetRenderFlags(RENDER_FLAG_FULL);

		// RENDER_FLAG_FULL is consumed by RenderWorld.cc's own full-redraw
		// check, which runs BEFORE RenderTacticalInterface()/RenderSMPanel()
		// (and so before this DeleteItemDescriptionBox() call, which happens
		// from inside HandleItemDescriptionBox()) in the very same frame --
		// so setting the flag here doesn't actually force a full terrain repaint
		// until the FOLLOWING frame. A single InvalidateRegionEx() call made
		// right here (the frame the flag is merely SET) would flip a rect
		// that can still hold stale box pixels, one frame too early; nothing
		// would then re-flip it once the terrain genuinely gets fixed.
		// TickItemDescCloseCleanup() (called every frame from
		// RenderTopmostTacticalInterface(), Interface_Control.cc) instead
		// keeps re-flipping this box's last on-screen rectangle for a few
		// frames, so whichever frame the terrain actually gets repainted on,
		// that repaint also reaches the screen.
		{
			bool const fIsMoney  = gpItemDescObject->usItem == MONEY;
			bool const fIsWeapon = GCM->getItem(gpItemDescObject->usItem)->isWeapon();
			gsItemDescCloseCleanupW =
				IsTacBigItemDescBox() ? GetMapItemDescWidth() :
				fIsMoney  ? ITEMDESC_WIDTH_MONEY :
				fIsWeapon ? ITEMDESC_WIDTH :
				            ITEMDESC_WIDTH_ITEMS;
			gsItemDescCloseCleanupH =
				IsTacBigItemDescBox() ? GetMapItemDescHeight() :
				fIsMoney  ? ITEMDESC_HEIGHT_MONEY :
				fIsWeapon ? ITEMDESC_HEIGHT :
				            ITEMDESC_HEIGHT_ITEMS;
			gsItemDescCloseCleanupX     = gsInvDescX;
			gsItemDescCloseCleanupY     = gsInvDescY;
			gsItemDescCloseCleanupFrames = 6;
		}

		// Undo the HideSMBookmarkButtons() call from
		// InternalInitItemDescriptionBox().
		if (IsSMPanelUnderItemDesc()) ShowSMPanelUnderItemDesc();
		else ShowSMBookmarkButtons();
	}

	if( guiCurrentItemDescriptionScreen == MAP_SCREEN )
	{
		if (giMapInvDescButton) RemoveButton( giMapInvDescButton );
		if (giMapHideEmptySlotsCheckbox) RemoveButton(giMapHideEmptySlotsCheckbox);

		// The sector-inventory panel's own transfer buttons
		// (MapInventoryPoolMoveToSectorBtn()/-ToMercBtn(), Map_Screen_
		// Interface_Map_Inventory.cc) enable/disable based partly on
		// InItemDescriptionBox(), re-evaluated inside
		// HandleButtonStatesWhileMapInventoryActive() -- which only runs
		// from BlitInventoryPoolGraphic(), itself gated behind
		// fMapPanelDirty (RenderMapRegionBackground(), MapScreen.cc).
		// Without this, closing ItemInfoC.sti while the sector-inventory
		// panel is open (but nothing else happens to mark it dirty first)
		// left those buttons showing their last, now-stale enabled state.
		fMapPanelDirty = TRUE;
	}

	// Remove region
	MSYS_RemoveRegion( &gInvDesc);


	// By what InternalInitItemDescriptionBox() created, not by what the
	// item is now: the object may have changed while the box was open (the
	// money withdrawn from the account sits in gItemPointer), and money
	// buttons left behind stay on the screen.
	for ( cnt = 0; cnt < MAX_ATTACHMENTS; cnt++ )
	{
		// Non-weapon items only get regions for their
		// GENERIC_ITEM_ATTACHMENT_SLOTS slots, money for none.
		if (!(gItemDescAttachmentRegions[cnt].uiFlags & MSYS_REGION_EXISTS)) continue;
		MSYS_RemoveRegion( &gItemDescAttachmentRegions[cnt]);
	}
	if (guiMoneyButtonBtn[M_DONE])
	{
		UnloadButtonImage( guiMoneyButtonImage );
		UnloadButtonImage( guiMoneyDoneButtonImage );
		for ( cnt = 0; cnt < NUM_MONEY_BUTTONS; cnt++ )
		{
			RemoveButton( guiMoneyButtonBtn[cnt] );
		}
	}

	// Like the regions and buttons above, by what was created: the box also
	// closes because its item is gone or has become another one.
	for (MOUSE_REGION& r : gProsAndConsRegions)
	{
		if (r.uiFlags & MSYS_REGION_EXISTS) MSYS_RemoveRegion(&r);
	}

	if (giItemDescAmmoButton)
	{
		// Remove button
		UnloadButtonImage( giItemDescAmmoButtonImages );
		RemoveButton( giItemDescAmmoButton );
	}
	if( guiCurrentItemDescriptionScreen == MAP_SCREEN )
	{
		fCharacterInfoPanelDirty=TRUE;
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fMapScreenBottomDirty = TRUE;
		gsMapItemDescCloseCleanupFrames = 3;
	}

	if (InKeyRingPopup())
	{
		DeleteKeyObject(gpItemDescObject);
		gpItemDescObject = NULL;
		fShowDescriptionFlag = FALSE;
		fInterfacePanelDirty = DIRTYLEVEL2;
		return;
	}

	fShowDescriptionFlag = FALSE;
	fInterfacePanelDirty = DIRTYLEVEL2;

	if( gpItemDescObject->usItem == MONEY )
	{
		//if there is no money remaining
		if( gRemoveMoney.uiMoneyRemaining == 0 && !gfAddingMoneyToMercFromPlayersAccount )
		{
			//get rid of the money in the slot
			*gpItemDescObject = OBJECTTYPE{};
			gpItemDescObject = NULL;
		}
	}

	if( gfAddingMoneyToMercFromPlayersAccount )
		gfAddingMoneyToMercFromPlayersAccount = FALSE;

	gfItemDescObjectIsAttachment = FALSE;

	// an item of the sector inventory goes back into its list -- unless the
	// box only closes to open again on it (CloseMapItemDescBoxForRelayout())
	if (!gfItemDescBoxRelayout) ReturnDescribedItemToSectorInventory();
}


// The map screen's box closed by CloseMapItemDescBoxForRelayout(), to be
// opened again by ReopenMapItemDescBoxAfterRelayout().
static OBJECTTYPE*  gpRelayoutItemDescObject     = NULL;
static UINT8        gubRelayoutItemDescStatusIndex;
static SOLDIERTYPE* gpRelayoutItemDescSoldier;
static BOOLEAN      gfRelayoutItemDescIsAttachment;


BOOLEAN CloseMapItemDescBoxForRelayout(void)
{
	if (!gfInItemDescBox || guiCurrentItemDescriptionScreen != MAP_SCREEN) return FALSE;
	// not in the middle of taking money, nor on a key of the key ring
	// (a temporary object the box deletes when it closes)
	if (gpItemDescObject->usItem == MONEY || InKeyRingPopup()) return FALSE;

	gpRelayoutItemDescObject       = gpItemDescObject;
	gubRelayoutItemDescStatusIndex = gubItemDescStatusIndex;
	gpRelayoutItemDescSoldier      = gpItemDescSoldier;
	gfRelayoutItemDescIsAttachment = gfItemDescObjectIsAttachment;

	gfItemDescBoxRelayout = TRUE;
	DeleteItemDescriptionBox();
	gfItemDescBoxRelayout = FALSE;
	return TRUE;
}


void ReopenMapItemDescBoxAfterRelayout(void)
{
	if (gpRelayoutItemDescObject == NULL) return;

	OBJECTTYPE* const o = gpRelayoutItemDescObject;
	gpRelayoutItemDescObject = NULL;

	gfItemDescObjectIsAttachment = gfRelayoutItemDescIsAttachment;
	InternalInitItemDescriptionBox(o, gsInvDescX, gsInvDescY, gubRelayoutItemDescStatusIndex, gpRelayoutItemDescSoldier);
}


void InternalBeginItemPointer( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObject, INT8 bHandPos )
{
	//BOOLEAN fOk;

	// If not null return
	if ( gpItemPointer != NULL )
	{
		return;
	}

	// Copy into cursor...
	gItemPointer = *pObject;

	// Dirty interface
	fInterfacePanelDirty = DIRTYLEVEL2;
	SetItemPointer(&gItemPointer, pSoldier);
	gbItemPointerSrcSlot = bHandPos;
	gbItemPointerLocateGood = TRUE;

	CheckForDisabledForGiveItem( );

	EnableSMPanelButtons( FALSE, TRUE );

	gfItemPointerDifferentThanDefault = FALSE;

	// re-evaluate repairs
	gfReEvaluateEveryonesNothingToDo = TRUE;
}

void BeginItemPointer( SOLDIERTYPE *pSoldier, UINT8 ubHandPos )
{
	BOOLEAN fOk;
	OBJECTTYPE pObject;

	pObject = OBJECTTYPE{};

	if (_KeyDown( SHIFT ))
	{
		// Remove all from soldier's slot
		fOk = RemoveObjectFromSlot( pSoldier, ubHandPos, &pObject );
	}
	else
	{
		GetObjFrom( &(pSoldier->inv[ubHandPos]), 0, &pObject );
		fOk = (pObject.ubNumberOfObjects == 1);
	}
	if (fOk)
	{
		InternalBeginItemPointer( pSoldier, &pObject, ubHandPos );
	}
}


void BeginKeyRingItemPointer( SOLDIERTYPE *pSoldier, UINT8 ubKeyRingPosition )
{
	// If not null return
	if ( gpItemPointer != NULL )
	{
		return;
	}

	// With shift down, remove all keys from this slot
	// With shift up, remove only one key
	BOOLEAN const fOk = RemoveKeysFromSlot( pSoldier, ubKeyRingPosition,
		_KeyDown( SHIFT ) ? pSoldier->pKeyRing[ ubKeyRingPosition ].ubNumber : 1,
		&gItemPointer );

	if (fOk)
	{
		// Dirty interface
		fInterfacePanelDirty = DIRTYLEVEL2;
		SetItemPointer(&gItemPointer, pSoldier);
		gbItemPointerSrcSlot = ubKeyRingPosition;

		if (fInMapMode) SetMapCursorItem();
	}

	gfItemPointerDifferentThanDefault = FALSE;
}

void EndItemPointer( )
{
	if ( gpItemPointer != NULL )
	{
		gpItemPointer = NULL;
		gbItemPointerSrcSlot = NO_SLOT;
		gSMPanelRegion.ChangeCursor(CURSOR_NORMAL);
		MSYS_SetCurrentCursor( CURSOR_NORMAL );

		if (guiCurrentScreen == SHOPKEEPER_SCREEN)
		{
			gMoveingItem = INVENTORY_IN_SLOT{};
			SetSkiCursor( CURSOR_NORMAL );
		}
		else
		{
			EnableSMPanelButtons( TRUE , TRUE );
		}

		gbItemPointerLocateGood = FALSE;

		// re-evaluate repairs
		gfReEvaluateEveryonesNothingToDo = TRUE;
	}
}

void DrawItemFreeCursor( )
{
	SetMouseCursorFromCurrentItem();
	gSMPanelRegion.ChangeCursor(EXTERN_CURSOR);
}


static BOOLEAN SoldierCanSeeCatchComing(const SOLDIERTYPE* pSoldier, INT16 sSrcGridNo)
{
	return( TRUE );
	/*
	INT32 cnt;
	INT8  bDirection, bTargetDirection;

	bTargetDirection = (INT8)GetDirectionToGridNoFromGridNo( pSoldier->sGridNo, sSrcGridNo );

	// Look 3 directions Clockwise from what we are facing....
	bDirection = pSoldier->bDirection;

	for ( cnt = 0; cnt < 3; cnt++ )
	{
		if ( bDirection == bTargetDirection )
		{
			return( TRUE );
		}

		bDirection = OneCDirection(bDirection);
	}

	// Look 3 directions CounterClockwise from what we are facing....
	bDirection = pSoldier->bDirection;

	for ( cnt = 0; cnt < 3; cnt++ )
	{
		if ( bDirection == bTargetDirection )
		{
			return( TRUE );
		}

		bDirection = OneCCDirection(bDirection);
	}

	// If here, nothing good can happen!
	return( FALSE );*/

}

void DrawItemTileCursor( )
{
	INT16 sAPCost;
	BOOLEAN fRecalc;
	INT16 sFinalGridNo;
	UINT32 uiCursorId = CURSOR_ITEM_GOOD_THROW;
	BOOLEAN fGiveItem = FALSE;
	INT16 sActionGridNo;
	static UINT32 uiOldCursorId = 0;
	static UINT16 usOldMousePos = 0;
	INT16 sEndZ = 0;
	INT16 sDist;
	INT8 bLevel;

	GridNo usMapPos = guiCurrentCursorGridNo;
	if (usMapPos != NOWHERE)
	{
		// Force mouse position to guy...
		if (gUIFullTarget != NULL) usMapPos = gUIFullTarget->sGridNo;

		gusCurMousePos = usMapPos;

		if( gusCurMousePos != usOldMousePos )
		{
			gfItemPointerDifferentThanDefault = FALSE;
		}

		// Save old one..
		usOldMousePos = gusCurMousePos;

		// Default to turning adjacent area gridno off....
		gfUIHandleShowMoveGrid = FALSE;

		// If we are over a talkable guy, set flag
		if (GetValidTalkableNPCFromMouse(TRUE, FALSE, TRUE) != NULL)
		{
			fGiveItem = TRUE;
		}


		// OK, if different than default, change....
		if ( gfItemPointerDifferentThanDefault )
		{
			if (fGiveItem)
			{
				// We are targeting a talkable NPC and are using the alternative cursor to
				// throw instead.
				fGiveItem = FALSE;
			}
			else
			{
				// We are using the alternative cursor and the target is not a talkable NPC
				// Only use the give item cursor if the target is a merc.
				fGiveItem = GetValidTalkableNPCFromMouse(TRUE, TRUE, FALSE) != NULL;
			}
		}

		// Get recalc and cursor flags
		MouseMoveState uiCursorFlags;
		fRecalc = GetMouseRecalcAndShowAPFlags( &uiCursorFlags, NULL );

		// OK, if we begin to move, reset the cursor...
		if (uiCursorFlags != MOUSE_STATIONARY)
		{
			EndPhysicsTrajectoryUI( );
		}

		// Get Pyth spaces away.....
		sDist = PythSpacesAway( gpItemPointerSoldier->sGridNo, gusCurMousePos );

		// If we are here and we are not selected, select!
		// ATE Design discussion propably needed here...
		SelectSoldier(gpItemPointerSoldier, SELSOLDIER_NONE);

		// ATE: if good for locate, locate to selected soldier....
		if ( gbItemPointerLocateGood )
		{
			gbItemPointerLocateGood = FALSE;
			LocateSoldier(GetSelectedMan(), FALSE);
		}

		if ( !fGiveItem )
		{
			if ( UIHandleOnMerc( FALSE ) && usMapPos != gpItemPointerSoldier->sGridNo )
			{
				// We are on a guy.. check if they can catch or not....
				const SOLDIERTYPE* const tgt = gUIFullTarget;
				if (tgt != NULL)
				{
					// Are they on our team?
					// ATE: Can't be an EPC
					if (tgt->bTeam == OUR_TEAM && !AM_AN_EPC(tgt) && !(tgt->uiStatusFlags & SOLDIER_VEHICLE))
					{
						if ( sDist <= PASSING_ITEM_DISTANCE_OKLIFE )
						{
							// OK, on a valid pass
							gfUIMouseOnValidCatcher = 4;
							gUIValidCatcher         = tgt;
						}
						else
						{
							// Can they see the throw?
							if (SoldierCanSeeCatchComing(tgt, gpItemPointerSoldier->sGridNo))
							{
								// OK, set global that this buddy can see catch...
								gfUIMouseOnValidCatcher = TRUE;
								gUIValidCatcher         = tgt;
							}
						}
					}
				}
			}

			// We're going to toss it!
			if ( gTacticalStatus.uiFlags & INCOMBAT )
			{
				gfUIDisplayActionPoints = TRUE;
				gUIDisplayActionPointsOffX = 15;
				gUIDisplayActionPointsOffY = 15;
			}

			// If we are tossing...
			if (  (sDist <= 1 && gfUIMouseOnValidCatcher == 0) || gfUIMouseOnValidCatcher == 4 )
			{
				gsCurrentActionPoints = AP_PICKUP_ITEM;
			}
			else
			{
				gsCurrentActionPoints = AP_TOSS_ITEM;
			}

		}
		else
		{
			const SOLDIERTYPE* const tgt = gUIFullTarget;
			if (tgt != NULL)
			{
				UIHandleOnMerc( FALSE );

				// OK, set global that this buddy can see catch...
				gfUIMouseOnValidCatcher = 2;
				gUIValidCatcher = tgt;

				// If this is a robot, change to say 'reload'
				if (tgt->uiStatusFlags & SOLDIER_ROBOT)
				{
					gfUIMouseOnValidCatcher = 3;
				}

				if (uiCursorFlags == MOUSE_STATIONARY)
				{
					// Find adjacent gridno...
					sActionGridNo = FindAdjacentGridEx(gpItemPointerSoldier, gusCurMousePos, NULL, NULL, FALSE, FALSE);
					if ( sActionGridNo == -1 )
					{
						sActionGridNo = gusCurMousePos;
					}

					// Display location...
					gsUIHandleShowMoveGridLocation = sActionGridNo;
					gfUIHandleShowMoveGrid = TRUE;


					// Get AP cost
					if (tgt->uiStatusFlags & SOLDIER_ROBOT)
					{
						sAPCost = GetAPsToReloadRobot(gpItemPointerSoldier, tgt);
					}
					else
					{
						sAPCost = GetAPsToGiveItem( gpItemPointerSoldier, sActionGridNo );
					}

					gsCurrentActionPoints = sAPCost;
				}

				// Set value
				if ( gTacticalStatus.uiFlags & INCOMBAT )
				{
					gfUIDisplayActionPoints = TRUE;
					gUIDisplayActionPointsOffX = 15;
					gUIDisplayActionPointsOffY = 15;
				}
			}
		}


		if ( fGiveItem )
		{
			uiCursorId = CURSOR_ITEM_GIVE;
		}
		else
		{
			// How afar away are we?
			if ( sDist <= 1 && gfUIMouseOnValidCatcher == 0 )
			{
				// OK, we want to drop.....

				// Write the word 'drop' on cursor...
				SetIntTileLocationText(pMessageStrings[MSG_DROP]);
			}
			else
			{
				if ( usMapPos == gpItemPointerSoldier->sGridNo )
				{
					EndPhysicsTrajectoryUI( );
				}
				else if ( gfUIMouseOnValidCatcher == 4 )
				{
					// ATE: Don't do if we are passing....
				}
				else
				// ( sDist > PASSING_ITEM_DISTANCE_OKLIFE )
				{
					// Write the word 'drop' on cursor...
					if ( gfUIMouseOnValidCatcher == 0 )
					{
						SetIntTileLocationText(pMessageStrings[MSG_THROW]);
					}

					gfUIHandlePhysicsTrajectory = TRUE;

					if ( fRecalc && usMapPos != gpItemPointerSoldier->sGridNo )
					{
						if ( gfUIMouseOnValidCatcher )
						{
							switch (gAnimControl[gUIValidCatcher->usAnimState].ubHeight)
							{
								case ANIM_STAND:

									sEndZ = 150;
									break;

								case ANIM_CROUCH:

									sEndZ = 80;
									break;

								case ANIM_PRONE:

									sEndZ = 10;
									break;
							}

							if (gUIValidCatcher->bLevel > 0) sEndZ = 0;
						}

						// Calculate chance to throw here.....
						if ( !CalculateLaunchItemChanceToGetThrough( gpItemPointerSoldier, gpItemPointer, usMapPos, (INT8)gsInterfaceLevel, (INT16)( ( gsInterfaceLevel * 256 ) + sEndZ ), &sFinalGridNo, FALSE, &bLevel, TRUE ) )
						{
							gfBadThrowItemCTGH = TRUE;
						}
						else
						{
							gfBadThrowItemCTGH = FALSE;
						}

						BeginPhysicsTrajectoryUI( sFinalGridNo, bLevel, gfBadThrowItemCTGH );
					}
				}

				if ( gfBadThrowItemCTGH )
				{
					uiCursorId = CURSOR_ITEM_BAD_THROW;
				}
			}
		}

		//Erase any cursor in viewport
		//gViewportRegion.ChangeCursor(VIDEO_NO_CURSOR);

		// Get tile graphic fro item
		UINT16 const usIndex = GetTileGraphicForItem(GCM->getItem(gpItemPointer->usItem));

		// ONly load if different....
		if ( usIndex != gusItemPointer || uiOldCursorId != uiCursorId )
		{
			// OK, Tile database gives me subregion and video object to use...
			const TILE_ELEMENT* const te = &gTileDatabase[usIndex];
			SetExternVOData(uiCursorId, te->hTileSurface, te->usRegionIndex);
			gusItemPointer = usIndex;
			uiOldCursorId = uiCursorId;
		}

		gViewportRegion.ChangeCursor(uiCursorId);
	}
}


static bool IsValidAmmoToReloadRobot(SOLDIERTYPE const& s, OBJECTTYPE const& ammo)
{
	OBJECTTYPE const& weapon = s.inv[HANDPOS];
	if (!CompatibleAmmoForGun(&ammo, &weapon))
	{
		ST::string name = *GCM->getWeapon(weapon.usItem)->calibre->getName();
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, st_format_printf(TacticalStr[ROBOT_NEEDS_GIVEN_CALIBER_STR], name));
		return false;
	}
	return true;
}


BOOLEAN HandleItemPointerClick( UINT16 usMapPos )
{
	// Determine what to do
	UINT8 ubDirection;
	UINT16 usItem;
	INT16 sAPCost;
	UINT8 ubThrowActionCode=0;
	INT16 sEndZ = 0;
	OBJECTTYPE TempObject;
	INT16 sGridNo;
	INT16 sDist;
	INT16 sDistVisible;


	if ( SelectedGuyInBusyAnimation( ) )
	{
		return( FALSE );
	}

	if (g_ui_message_overlay != NULL)
	{
		EndUIMessage( );
		return( FALSE );
	}

	// Don't allow if our soldier is a # of things...
	if ( AM_AN_EPC( gpItemPointerSoldier ) || gpItemPointerSoldier->bLife < OKLIFE || gpItemPointerSoldier->bOverTerrainType == DEEP_WATER )
	{
		return( FALSE );
	}

	// This implies we have no path....
	if ( gsCurrentActionPoints == 0 )
	{
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[ NO_PATH ] );
		return( FALSE );
	}

	if (gUIFullTarget != NULL)
	{
		// Force mouse position to guy...
		usMapPos = gUIFullTarget->sGridNo;

		if (gAnimControl[gUIFullTarget->usAnimState].uiFlags & ANIM_MOVING)
		{
			return( FALSE );
		}

	}

	// Check if we have APs....
	if ( !EnoughPoints( gpItemPointerSoldier, gsCurrentActionPoints, 0, TRUE ) )
	{
		if ( gfDontChargeAPsToPickup && gsCurrentActionPoints == AP_PICKUP_ITEM )
		{

		}
		else
		{
			return( FALSE );
		}
	}

	// SEE IF WE ARE OVER A TALKABLE GUY!
	SOLDIERTYPE* const tgt = gUIFullTarget;
	BOOLEAN fGiveItem = tgt != NULL && IsValidTalkableNPC(tgt, TRUE, FALSE, TRUE);

	// OK, if different than default, change....
	if ( gfItemPointerDifferentThanDefault )
	{
		if (fGiveItem)
		{
			// We are targeting a talkable NPC and are using the alternative cursor to
			// throw instead.
			fGiveItem = FALSE;
		}
		else
		{
			// We are using the alternative cursor and the target is not a talkable NPC
			// Only give an item if the target is a merc.
			fGiveItem = tgt != NULL && IsValidTalkableNPC(tgt, TRUE, TRUE, TRUE);
		}
	}


	// Get Pyth spaces away.....
	sDist = PythSpacesAway( gpItemPointerSoldier->sGridNo, gusCurMousePos );


	if ( fGiveItem )
	{
		usItem = gpItemPointer->usItem;

		// If the target is a robot,
		if (tgt->uiStatusFlags & SOLDIER_ROBOT)
		{
			// Charge APs to reload robot!
			sAPCost = GetAPsToReloadRobot(gpItemPointerSoldier, tgt);
		}
		else
		{
			// Calculate action point costs!
			sAPCost = GetAPsToGiveItem( gpItemPointerSoldier, usMapPos );
		}

		// Place it back in our hands!

		TempObject = *gpItemPointer;

		if ( gbItemPointerSrcSlot != NO_SLOT )
		{
			PlaceObject( gpItemPointerSoldier, gbItemPointerSrcSlot, gpItemPointer );
			fInterfacePanelDirty = DIRTYLEVEL2;
		}
		/*
		//if the user just clicked on an arms dealer
		if (IsMercADealer(tgt->ubProfile))
		{
			if ( EnoughPoints( gpItemPointerSoldier, sAPCost, 0, TRUE ) )
			{
				//Enter the shopkeeper interface
				EnterShopKeeperInterfaceScreen(tgt->ubProfile);

				EndItemPointer( );
			}

			return( TRUE );
		}*/

		if ( EnoughPoints( gpItemPointerSoldier, sAPCost, 0, TRUE ) )
		{
			// If we are a robot, check if this is proper item to reload!
			if (tgt->uiStatusFlags & SOLDIER_ROBOT)
			{
				// Check if we can reload robot....
				if (IsValidAmmoToReloadRobot(*tgt, TempObject))
				{
					INT16 sActionGridNo;
					UINT8 ubDirection;
					INT16 sAdjustedGridNo;

					// Walk up to him and reload!
					// See if we can get there to stab
					sActionGridNo = FindAdjacentGridEx(gpItemPointerSoldier, tgt->sGridNo, &ubDirection, &sAdjustedGridNo, TRUE, FALSE);

					if ( sActionGridNo != -1 && gbItemPointerSrcSlot != NO_SLOT )
					{
							// Make a temp object for ammo...
							gpItemPointerSoldier->pTempObject  = new OBJECTTYPE{};
							*gpItemPointerSoldier->pTempObject = TempObject;

							// Remove from soldier's inv...
							RemoveObjs( &( gpItemPointerSoldier->inv[ gbItemPointerSrcSlot ] ), 1 );

							gpItemPointerSoldier->sPendingActionData2  = sAdjustedGridNo;
							gpItemPointerSoldier->uiPendingActionData1 = gbItemPointerSrcSlot;
							gpItemPointerSoldier->bPendingActionData3  = ubDirection;
							gpItemPointerSoldier->ubPendingActionAnimCount = 0;

							// CHECK IF WE ARE AT THIS GRIDNO NOW
							if ( gpItemPointerSoldier->sGridNo != sActionGridNo )
							{
								Soldier{gpItemPointerSoldier}.setPendingAction(MERC_RELOADROBOT);

								// WALK UP TO DEST FIRST
								EVENT_InternalGetNewSoldierPath( gpItemPointerSoldier, sActionGridNo, gpItemPointerSoldier->usUIMovementMode, FALSE, FALSE );
							}
							else
							{
								EVENT_SoldierBeginReloadRobot( gpItemPointerSoldier, sAdjustedGridNo, ubDirection, gbItemPointerSrcSlot );
							}

							// OK, set UI
							SetUIBusy(gpItemPointerSoldier);
					}

				}

				gfDontChargeAPsToPickup = FALSE;
				EndItemPointer( );
			}
			else
			{
				//if (gbItemPointerSrcSlot != NO_SLOT )
				{
					// Give guy this item.....
					SoldierGiveItem(gpItemPointerSoldier, tgt, &TempObject, gbItemPointerSrcSlot);

					gfDontChargeAPsToPickup = FALSE;
					EndItemPointer( );

					// If we are giving it to somebody not on our team....
					if (tgt->ubProfile != NO_PROFILE && !MercProfile(tgt->ubProfile).isPlayerMerc() && !RPC_RECRUITED(tgt))
					{
						SetEngagedInConvFromPCAction( gpItemPointerSoldier );
					}
				}
			}
		}

		return( TRUE );
	}

	// CHECK IF WE ARE NOT ON THE SAME GRIDNO
	if (sDist <= 1 &&
			(gUIFullTarget == NULL || gUIFullTarget == gpItemPointerSoldier))
	{
		// Check some things here....
		// 1 ) are we at the exact gridno that we stand on?
		if ( usMapPos == gpItemPointerSoldier->sGridNo )
		{
			// Drop
			if ( !gfDontChargeAPsToPickup )
			{
				// Deduct points
				DeductPoints( gpItemPointerSoldier, AP_PICKUP_ITEM, 0 );
			}

			SoldierDropItem( gpItemPointerSoldier, gpItemPointer );
		}
		else
		{
			// Try to drop in an adjacent area....
			// 1 ) is this not a good OK destination
			// this will sound strange, but this is OK......
			if ( !NewOKDestination( gpItemPointerSoldier, usMapPos, FALSE, gpItemPointerSoldier->bLevel ) || FindBestPath( gpItemPointerSoldier, usMapPos, gpItemPointerSoldier->bLevel, WALKING, NO_COPYROUTE, 0 ) == 1 )
			{
				// Drop
				if ( !gfDontChargeAPsToPickup )
				{
					// Deduct points
					DeductPoints( gpItemPointerSoldier, AP_PICKUP_ITEM, 0 );
				}

				// Play animation....
				// Don't show animation of dropping item, if we are not standing



				switch ( gAnimControl[ gpItemPointerSoldier->usAnimState ].ubHeight )
				{
					case ANIM_STAND:
						gpItemPointerSoldier->pTempObject = new OBJECTTYPE{};
						*gpItemPointerSoldier->pTempObject = *gpItemPointer;
						gpItemPointerSoldier->sPendingActionData2 = usMapPos;

						// Turn towards.....gridno
						EVENT_SetSoldierDesiredDirectionForward(gpItemPointerSoldier, (INT8)GetDirectionFromGridNo(usMapPos, gpItemPointerSoldier));

						EVENT_InitNewSoldierAnim( gpItemPointerSoldier, DROP_ADJACENT_OBJECT, 0 , FALSE );
						break;

					case ANIM_CROUCH:
					case ANIM_PRONE:
						AddItemToPool(usMapPos, gpItemPointer, VISIBLE, gpItemPointerSoldier->bLevel, 0 , -1);
						NotifySoldiersToLookforItems( );
						break;
				}
			}
			else
			{
				// Drop in place...
				if ( !gfDontChargeAPsToPickup )
				{
					// Deduct points
					DeductPoints( gpItemPointerSoldier, AP_PICKUP_ITEM, 0 );
				}

				SoldierDropItem( gpItemPointerSoldier, gpItemPointer );
			}
		}
	}
	else
	{
		sGridNo = usMapPos;

		SOLDIERTYPE* const pSoldier = gUIFullTarget;
		if (sDist <= PASSING_ITEM_DISTANCE_OKLIFE &&
			pSoldier != NULL &&
			pSoldier->bTeam == OUR_TEAM &&
			!AM_AN_EPC(pSoldier) &&
			!(pSoldier->uiStatusFlags & SOLDIER_VEHICLE))
		{
			// OK, do the transfer...
			{
				{
					if ( !EnoughPoints( pSoldier, 3, 0, TRUE ) ||
						!EnoughPoints( gpItemPointerSoldier, 3, 0, TRUE ) )
					{
						return( FALSE );
					}

					sDistVisible = DistanceVisible( pSoldier, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, gpItemPointerSoldier->sGridNo, gpItemPointerSoldier->bLevel );

					// Check LOS....
					if ( !SoldierTo3DLocationLineOfSightTest( pSoldier, gpItemPointerSoldier->sGridNo,  gpItemPointerSoldier->bLevel, 3, (UINT8) sDistVisible, TRUE ) )
					{
						return( FALSE );
					}

					// Charge AP values...
					DeductPoints( pSoldier, 3, 0 );
					DeductPoints( gpItemPointerSoldier, 3, 0 );

					usItem = gpItemPointer->usItem;

					// try to auto place object....
					if ( AutoPlaceObject( pSoldier, gpItemPointer, TRUE ) )
					{
						ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, st_format_printf(pMessageStrings[ MSG_ITEM_PASSED_TO_MERC ], GCM->getItem(usItem)->getShortName(), pSoldier->name) );

						// Check if it's the same now!
						if ( gpItemPointer->ubNumberOfObjects == 0 )
						{
							EndItemPointer( );
						}

						// OK, make guys turn towards each other and do animation...
						{
							UINT8 ubFacingDirection;

							// Get direction to face.....
							ubFacingDirection = (UINT8)GetDirectionFromGridNo( gpItemPointerSoldier->sGridNo, pSoldier );

							// Stop merc first....
							EVENT_StopMerc(pSoldier);

							// If we are standing only...
							if ( gAnimControl[ pSoldier->usAnimState ].ubEndHeight == ANIM_STAND && !MercInWater( pSoldier ) )
							{
								// Turn to face, then do animation....
								EVENT_SetSoldierDesiredDirection( pSoldier, ubFacingDirection );
								pSoldier->fTurningUntilDone = TRUE;
								pSoldier->usPendingAnimation = PASS_OBJECT;
							}

							if ( gAnimControl[ gpItemPointerSoldier->usAnimState ].ubEndHeight == ANIM_STAND && !MercInWater( gpItemPointerSoldier ) )
							{
								EVENT_SetSoldierDesiredDirection(gpItemPointerSoldier, OppositeDirection(ubFacingDirection));
								gpItemPointerSoldier->fTurningUntilDone = TRUE;
								gpItemPointerSoldier->usPendingAnimation = PASS_OBJECT;
							}
						}

						return( TRUE );
					}
					else
					{
						ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, st_format_printf(pMessageStrings[ MSG_NO_ROOM_TO_PASS_ITEM ], GCM->getItem(usItem)->getShortName(), pSoldier->name) );
						return( FALSE );
					}
				}
			}
		}
		else
		{
			// CHECK FOR VALID CTGH
			if ( gfBadThrowItemCTGH )
			{
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[ CANNOT_THROW_TO_DEST_STR ] );
				return( FALSE );
			}

			// Deduct points
			//DeductPoints( gpItemPointerSoldier, AP_TOSS_ITEM, 0 );
			gpItemPointerSoldier->fDontChargeTurningAPs = TRUE;
			// Will be dome later....

			ubThrowActionCode = NO_THROW_ACTION;

			// OK, CHECK FOR VALID THROW/CATCH
			// IF OVER OUR GUY...
			SOLDIERTYPE* target = NULL;
			if (pSoldier != NULL)
			{
				if (pSoldier->bTeam == OUR_TEAM && pSoldier->bLife >= OKLIFE && !AM_AN_EPC(pSoldier) &&
					!(pSoldier->uiStatusFlags & SOLDIER_VEHICLE))
				{
					// OK, on our team,

					// How's our direction?
					if ( SoldierCanSeeCatchComing( pSoldier, gpItemPointerSoldier->sGridNo ) )
					{
						// Setup as being the catch target
						ubThrowActionCode = THROW_TARGET_MERC_CATCH;
						target            = pSoldier;

						sGridNo = pSoldier->sGridNo;

						switch( gAnimControl[ pSoldier->usAnimState ].ubHeight )
						{
							case ANIM_STAND:
								sEndZ = 150;
								break;

							case ANIM_CROUCH:
								sEndZ = 80;
								break;

							case ANIM_PRONE:
								sEndZ = 10;
								break;
						}

						if ( pSoldier->bLevel > 0 )
						{
							sEndZ = 0;
						}

						// Get direction
						ubDirection = (UINT8)GetDirectionFromGridNo( gpItemPointerSoldier->sGridNo, pSoldier );

						// ATE: Goto stationary...
						SoldierGotoStationaryStance( pSoldier );

						// Set direction to turn...
						EVENT_SetSoldierDesiredDirection( pSoldier, ubDirection );
					}
				}
			}

			// CHANGE DIRECTION AT LEAST
			ubDirection = (UINT8)GetDirectionFromGridNo( sGridNo, gpItemPointerSoldier );
			EVENT_SetSoldierDesiredDirection( gpItemPointerSoldier, ubDirection );
			gpItemPointerSoldier->fTurningUntilDone = TRUE;

			// Increment attacker count...
			gTacticalStatus.ubAttackBusyCount++;
			SLOGD("INcremtning ABC: Throw item to {}", gTacticalStatus.ubAttackBusyCount);

			// Given our gridno, throw grenate!
			CalculateLaunchItemParamsForThrow(gpItemPointerSoldier, sGridNo, gsInterfaceLevel, gsInterfaceLevel * 256 + sEndZ, gpItemPointer, 0, ubThrowActionCode, target);

			// OK, goto throw animation
			HandleSoldierThrowItem( gpItemPointerSoldier, usMapPos );
		}
	}

	gfDontChargeAPsToPickup = FALSE;
	EndItemPointer( );


	return( TRUE );
}


BOOLEAN InItemStackPopup( )
{
	return( gfInItemStackPopup );
}


BOOLEAN InKeyRingPopup( )
{
	return( gfInKeyRingPopup );
}


static void ItemPopupFullRegionCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemPopupFullRegionCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemPopupRegionCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemPopupRegionCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason);


// The 1366x768 SM panel's key ring and item stack popups:
// keyring_tactical_big_1366x768.png (538x247) at x 236 on the screen's bottom,
// 7x4 slots of 64x52 (like the panel's own head slots, the same big pictures
// and status bars), 73 px apart across, 55 down. The stack popup uses it while
// the stack fits in its 28 slots; the slots past the stack's limit shaded.
#define TAC_BIG_KEYRING_FILE      INTERFACEDIR "/keyring_tactical_big_1366x768.png"
#define TAC_BIG_KEYRING_X         236
#define TAC_BIG_KEYRING_SLOT_X    21
#define TAC_BIG_KEYRING_SLOT_Y    15
#define TAC_BIG_KEYRING_PITCH_X   73
#define TAC_BIG_KEYRING_PITCH_Y   55
#define TAC_BIG_KEYRING_COLS      7
#define TAC_BIG_KEYRING_SLOTS     28
#define TAC_BIG_KEYRING_SLOT_W    64
#define TAC_BIG_KEYRING_SLOT_H    52
static bool gfTacBigKeyRingPopup = false;
static bool gfTacBigStackPopup   = false;

static bool UseTacBigKeyRingPanel()
{
	return guiCurrentScreen != MAP_SCREEN && guiCurrentScreen != SHOPKEEPER_SCREEN &&
		g_ui.isBigSMPanel() && GCM->doesGameResExists(TAC_BIG_KEYRING_FILE);
}

static INT16 TacBigKeyRingPanelY()
{
	return SCREEN_HEIGHT - GetVObject(TAC_BIG_KEYRING_FILE)->SubregionProperties(0).usHeight;
}

// slot i's picture box
static void TacBigKeyRingSlot(UINT const i, INT16& x, INT16& y)
{
	x = TAC_BIG_KEYRING_X     + TAC_BIG_KEYRING_SLOT_X + i % TAC_BIG_KEYRING_COLS * TAC_BIG_KEYRING_PITCH_X;
	y = TacBigKeyRingPanelY() + TAC_BIG_KEYRING_SLOT_Y + i / TAC_BIG_KEYRING_COLS * TAC_BIG_KEYRING_PITCH_Y;
}

static void DefineTacBigKeyRingSlotRegion(MOUSE_REGION& r, UINT const i, MOUSE_CALLBACK const& click)
{
	INT16 x;
	INT16 y;
	TacBigKeyRingSlot(i, x, y);
	MSYS_DefineRegion(&r, x, y, x + TAC_BIG_KEYRING_SLOT_W, y + TAC_BIG_KEYRING_SLOT_H,
		MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, click);
	MSYS_SetRegionUserData(&r, 0, i);
}

static void RenderTacBigKeyRingPanel()
{
	BltVideoObject(FRAME_BUFFER, GetVObject(TAC_BIG_KEYRING_FILE), 0, TAC_BIG_KEYRING_X, TacBigKeyRingPanelY());
}

// in slot i the object's big picture and the status bar of its object status_index
static void RenderTacBigKeyRingSlotItem(UINT const i, OBJECTTYPE const& o, UINT8 const status_index, UINT8 const render_flags)
{
	INT16 x;
	INT16 y;
	TacBigKeyRingSlot(i, x, y);
	InvItemTextLayout const text = GetMapInvBigItemTextLayout();
	INVRenderItem(FRAME_BUFFER, NULL, o, x, y, TAC_BIG_KEYRING_SLOT_W, TAC_BIG_KEYRING_SLOT_H, DIRTYLEVEL2, render_flags, SGP_TRANSPARENT, TRUE, &text);
	DrawItemUIBarEx(o, status_index, x - TAC_BIG_BAR_DX, y + TAC_BIG_BAR_DY, TAC_BIG_BAR_HEIGHT,
		Get16BPPColor(STATUS_BAR), Get16BPPColor(STATUS_BAR_SHADOW), FRAME_BUFFER, TAC_BIG_BAR_WIDTH);
}


// Shared body of InitItemStackPopup()/InitSectorInventoryStackPopup() --
// factored out the same way InitItemDescriptionBox() was split into
// InternalInitItemDescriptionBox(), so a caller whose item isn't a
// SOLDIERTYPE::inv[] slot (the sector-inventory stash, a plain OBJECTTYPE*)
// can still use it. sourceRegion replaces gSMInvRegion[ubPosition] (only
// used to center the popup near where it was opened from) and gfxFilename
// lets the sector-inventory caller use its own dedicated art instead of
// extra_inventory.sti.
static void InternalInitItemStackPopup(OBJECTTYPE* const pObject, SOLDIERTYPE* const pSoldier, MOUSE_REGION const& sourceRegion, const char* const gfxFilename, UINT8 const ubLimit, INT16 const sInvX, INT16 const sInvY, INT16 const sInvWidth, INT16 const sInvHeight)
{
	SGPRect aRect;
	UINT8 ubCols;
	UINT8 ubRows;
	INT32 cnt;

	// Set some globals
	gsItemPopupInvX = sInvX;
	gsItemPopupInvY = sInvY;
	gsItemPopupInvWidth = sInvWidth;
	gsItemPopupInvHeight = sInvHeight;


	gpItemPopupSoldier = pSoldier;


	// Determine # of items
	gpItemPopupObject = pObject;

	// Return if #objects not >1
	if (ubLimit < 1)
	{
		gfTacBigStackPopup = false;
		return;
	}

	SortItemStackByStatus(pObject);

	// Keep this in sync with whichever screen is actually asking for the
	// popup -- ItemPopupRegionCallbackSecondary() below relies on it to
	// pick MAPInternalInitItemDescriptionBox() vs InternalInitItemDescriptionBox()
	// for the individual item it's opened on.
	guiCurrentItemDescriptionScreen = guiCurrentScreen;

	if( ubLimit > MAX_STACK_POPUP_WIDTH )
	{
		ubCols = MAX_STACK_POPUP_WIDTH;
		ubRows = ubLimit / MAX_STACK_POPUP_WIDTH;
	} else {
		ubCols = ubLimit;
		ubRows = 0;
	}

	// Load graphics
	guiItemPopupBoxes = AddVideoObjectFromFile(gfxFilename);

	// Get size
	ETRLEObject const& pTrav        = guiItemPopupBoxes->SubregionProperties(0);
	UINT16      const  usPopupWidth = pTrav.usWidth;
	UINT16      const  usPopupHeight = pTrav.usHeight;

	// Get Width, Height
	INT16 gsItemPopupWidth = ubCols * usPopupWidth;
	INT16 gsItemPopupHeight = ubRows * usPopupHeight;
	gubNumItemPopups = ubLimit;

	// Calculate X,Y, first center
	MOUSE_REGION const& r = sourceRegion;
	INT16 sCenX = r.X() - (gsItemPopupWidth / 2 + r.W() / 2);
	INT16 sCenY	= r.Y()- (gsItemPopupHeight / 2 + r.H() / 2);

	// Limit it to window for item desc
	if ( sCenX < gsItemPopupInvX )
	{
		sCenX = gsItemPopupInvX;
	}
	if ( ( sCenX + gsItemPopupWidth ) > ( gsItemPopupInvX + gsItemPopupInvWidth ) )
	{
		sCenX = gsItemPopupInvX + gsItemPopupInvWidth - gsItemPopupWidth;
	}
	if ( sCenY < gsItemPopupInvY )
	{
		sCenY = gsItemPopupInvY;
	}
	if ( sCenY + gsItemPopupHeight > ( gsItemPopupInvY + gsItemPopupInvHeight ) )
	{
		sCenY = gsItemPopupInvY + gsItemPopupInvHeight - gsItemPopupHeight;
	}

	// Cap it at 0....
	if ( sCenX < 0 )
	{
		sCenX = 0;
	}
	if ( sCenY < 0 )
	{
		sCenY = 0;
	}

	// Set
	gsItemPopupX	= sCenX;
	gsItemPopupY	= sCenY;

	for ( cnt = 0; cnt < gubNumItemPopups; cnt++ )
	{
		UINT32 row = cnt / MAX_STACK_POPUP_WIDTH;
		UINT32 col = cnt % MAX_STACK_POPUP_WIDTH;

		// Build a mouse region here that is over any others.....
		MOUSE_CALLBACK itemPopupRegionCallback = MouseCallbackPrimarySecondary(ItemPopupRegionCallbackPrimary, ItemPopupRegionCallbackSecondary, MSYS_NO_CALLBACK, true);
		if (gfTacBigStackPopup)
		{
			DefineTacBigKeyRingSlotRegion(gItemPopupRegions[cnt], cnt, itemPopupRegionCallback);
		}
		else
		{
			MSYS_DefineRegion(&gItemPopupRegions[cnt], sCenX + col * usPopupWidth, sCenY + row * usPopupHeight, sCenX + (col + 1) * usPopupWidth, sCenY + (row+1) * usPopupHeight, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, itemPopupRegionCallback);
			MSYS_SetRegionUserData( &gItemPopupRegions[cnt], 0, cnt );
		}

		//OK, for each item, set dirty text if applicable!
		gItemPopupRegions[cnt].SetFastHelpText(GCM->getItem(pObject->usItem)->getName());
	}


	// Build a mouse region here that is over any others.....
	MSYS_DefineRegion(&gItemPopupRegion, gsItemPopupInvX, gsItemPopupInvY, gsItemPopupInvX + gsItemPopupInvWidth, gsItemPopupInvY + gsItemPopupInvHeight, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MouseCallbackPrimarySecondary(ItemPopupFullRegionCallbackPrimary, ItemPopupFullRegionCallbackSecondary));


	//Disable all faces
	SetAllAutoFacesInactive( );


	fInterfacePanelDirty = DIRTYLEVEL2;

	gfInItemStackPopup = TRUE;

	if( guiCurrentItemDescriptionScreen != MAP_SCREEN )
	{
		EnableSMPanelButtons( FALSE, FALSE );
	}

	//Reserict mouse cursor to panel
	aRect.iTop = sInvY;
	aRect.iLeft = sInvX;
	aRect.iBottom = sInvY + sInvHeight;
	aRect.iRight = sInvX + sInvWidth;

	RestrictMouseCursor( &aRect );
}


void InitItemStackPopup(SOLDIERTYPE* const pSoldier, UINT8 const ubPosition, INT16 const sInvX, INT16 const sInvY, INT16 const sInvWidth, INT16 const sInvHeight)
{
	OBJECTTYPE* const pObject = &(pSoldier->inv[ ubPosition ] );
	// ApplyBigPerPocketOverride() applies the item's own ubBigPerPocket (if
	// set) for a real BIGPOCK1-10POS slot, so this popup's grid matches
	// whatever PlaceObject() actually allowed to accumulate there -- see
	// its own comment for why this is a real inventory slot, not the
	// sector-inventory stash's BIGPOCK1POS dummy (which never sees it).
	//
	// Clamped to MAX_OBJECTS_PER_SLOT -- ItemSlotLimit() (an item's own
	// ubPerPocket from game data, possibly halved for a small pocket) isn't
	// itself bounded by it, but gItemPopupRegions[] below is, and a single
	// OBJECTTYPE can never actually hold more than MAX_OBJECTS_PER_SLOT
	// units regardless of what ubPerPocket claims.
	UINT8       const ubLimit = std::min<UINT8>(ApplyBigPerPocketOverride(pObject->usItem, (INT8)ubPosition, ItemSlotLimit( pObject->usItem, ubPosition )), MAX_OBJECTS_PER_SLOT);

	// the 1366x768 SM panel: in the key ring's panel, if the stack fits
	gfTacBigStackPopup = UseTacBigKeyRingPanel() && ubLimit <= TAC_BIG_KEYRING_SLOTS;
	InternalInitItemStackPopup(pObject, pSoldier, gSMInvRegion[ubPosition], INTERFACEDIR "/extra_inventory.sti", ubLimit, sInvX, sInvY, sInvWidth, sInvHeight);
}


void RenderItemStackPopup( BOOLEAN fFullRender )
{
	if ( gfInItemStackPopup )
	{

		//Disable all faces
		SetAllAutoFacesInactive( );

		// Shadow Area
		if ( fFullRender )
		{
			// Left bound hardcoded to 0 (not gsItemPopupInvX), matching
			// RenderKeyRingPopup()'s own ShadowRect -- with gsItemPopupInvWidth
			// now SCREEN_WIDTH (InitItemStackPopup() caller, Interface_Panels.cc)
			// this extends the shading to both the left and right edges of the
			// panel, same as the keyring popup's.
			FRAME_BUFFER->ShadowRect(0, gsItemPopupInvY, gsItemPopupInvX + gsItemPopupInvWidth, gsItemPopupInvY + gsItemPopupInvHeight);
		}

	}
	if (gfTacBigStackPopup)
	{
		RenderTacBigKeyRingPanel();
		for (UINT cnt = 0; cnt != TAC_BIG_KEYRING_SLOTS; ++cnt)
		{
			if (cnt >= gubNumItemPopups)
			{
				// past the stack's limit
				INT16 x;
				INT16 y;
				TacBigKeyRingSlot(cnt, x, y);
				FRAME_BUFFER->ShadowRect(x, y, x + TAC_BIG_KEYRING_SLOT_W, y + TAC_BIG_KEYRING_SLOT_H);
				continue;
			}
			if (cnt < gpItemPopupObject->ubNumberOfObjects)
			{
				RenderTacBigKeyRingSlotItem(cnt, *gpItemPopupObject, cnt, RENDER_ITEM_NOSTATUS);
			}
		}
		InvalidateRegion( gsItemPopupInvX, gsItemPopupInvY, gsItemPopupInvX + gsItemPopupInvWidth, gsItemPopupInvY + gsItemPopupInvHeight );
		return;
	}

	// TAKE A LOOK AT THE VIDEO OBJECT SIZE ( ONE OF TWO SIZES ) AND CENTER!
	ETRLEObject const& pTrav  = guiItemPopupBoxes->SubregionProperties(0);
	UINT32 const usWidth = pTrav.usWidth;
	UINT32 const usHeight = pTrav.usHeight;

	for (UINT32 cnt = 0; cnt < gubNumItemPopups; cnt++)
	{
		UINT32 row = cnt / MAX_STACK_POPUP_WIDTH;
		UINT32 col = cnt % MAX_STACK_POPUP_WIDTH;

		BltVideoObject(FRAME_BUFFER, guiItemPopupBoxes, 0, gsItemPopupX + col * usWidth, gsItemPopupY + row * usHeight);

		if ( cnt < gpItemPopupObject->ubNumberOfObjects )
		{
			INT16 sX = gsItemPopupX + col * usWidth + 13;
			INT16 sY = gsItemPopupY + row * usHeight + 0;

			INVRenderItem(FRAME_BUFFER, NULL, *gpItemPopupObject, sX, sY, 36, 31, DIRTYLEVEL2, RENDER_ITEM_NOSTATUS, SGP_TRANSPARENT);

			// Do status bar here...
			INT16 sNewX = gsItemPopupX + col * usWidth + 6;
			INT16 sNewY = gsItemPopupY + row * usHeight + INV_BAR_DY + 1;
			DrawItemUIBarEx(*gpItemPopupObject, cnt, sNewX, sNewY, ITEM_BAR_HEIGHT, Get16BPPColor(STATUS_BAR), Get16BPPColor(STATUS_BAR_SHADOW), FRAME_BUFFER);
		}
	}

	//RestoreExternBackgroundRect( gsItemPopupInvX, gsItemPopupInvY, gsItemPopupInvWidth, gsItemPopupInvHeight );
	InvalidateRegion( gsItemPopupInvX, gsItemPopupInvY, gsItemPopupInvX + gsItemPopupInvWidth, gsItemPopupInvY + gsItemPopupInvHeight );

}


void DeleteItemStackPopup(void)
{
	INT32 cnt;

	DeleteVideoObject(guiItemPopupBoxes);

	MSYS_RemoveRegion( &gItemPopupRegion);


	gfInItemStackPopup = FALSE;
	gfTacBigStackPopup = false;

	for ( cnt = 0; cnt < gubNumItemPopups; cnt++ )
	{
		MSYS_RemoveRegion( &gItemPopupRegions[cnt]);
	}


	fInterfacePanelDirty = DIRTYLEVEL2;

	if( guiCurrentItemDescriptionScreen != MAP_SCREEN )
	{
		EnableSMPanelButtons( TRUE, FALSE );
	}

	FreeMouseCursor( );

}


void InitKeyRingPopup(SOLDIERTYPE* const pSoldier, INT16 const sInvX, INT16 const sInvY, INT16 const sInvWidth, INT16 const sInvHeight)
{
	SGPRect aRect;
	INT16 sKeyRingItemWidth = 0;
	INT16 sOffSetY = 0, sOffSetX = 0;

	if( guiCurrentScreen == MAP_SCREEN )
	{
		// sInvX is MAP_KEYRING_POPUP_X (MapScreen.h)
		gsKeyRingPopupInvX = sInvX;
		sKeyRingItemWidth = MAP_KEY_RING_ROW_WIDTH;
		sOffSetX = MAP_KEYRING_POPUP_BOX_OFFSET_X;
		sOffSetY = MAP_KEYRING_POPUP_BOX_OFFSET_Y;
	}
	else
	{
		// Set some globals
		gsKeyRingPopupInvX = sInvX + TACTICAL_INVENTORY_KEYRING_GRAPHIC_OFFSET_X;
		sKeyRingItemWidth = KEY_RING_ROW_WIDTH;
		sOffSetY = 8;
	}

	gsKeyRingPopupInvY = sInvY;
	gsKeyRingPopupInvWidth = sInvWidth;
	gsKeyRingPopupInvHeight = sInvHeight;


	gpItemPopupSoldier = pSoldier;

	// Load graphics
	guiItemPopupBoxes = AddVideoObjectFromFile(INTERFACEDIR "/extra_inventory.sti");

	// Get size
	ETRLEObject const& pTrav         = guiItemPopupBoxes->SubregionProperties(0);
	UINT16      const  usPopupWidth  = pTrav.usWidth;
	UINT16      const  usPopupHeight = pTrav.usHeight;

	// the 1366x768 SM panel: its own key ring panel
	gfTacBigKeyRingPopup = UseTacBigKeyRingPanel();
	for (INT32 cnt = 0; cnt < NUMBER_KEYS_ON_KEYRING; cnt++)
	{
		if (gfTacBigKeyRingPopup)
		{
			DefineTacBigKeyRingSlotRegion(gKeyRingRegions[cnt], cnt, KeyRingSlotInvClickCallback);
			continue;
		}
		// Build a mouse region here that is over any others.....
		MSYS_DefineRegion(&gKeyRingRegions[cnt],
			gsKeyRingPopupInvX + (cnt % sKeyRingItemWidth      * usPopupWidth)  + sOffSetX, // top left
			sInvY              + (cnt / sKeyRingItemWidth      * usPopupHeight) + sOffSetY, // top right
			gsKeyRingPopupInvX + (cnt % sKeyRingItemWidth + 1) * usPopupWidth   + sOffSetX, // bottom left
			sInvY              + (cnt / sKeyRingItemWidth + 1) * usPopupHeight  + sOffSetY, // bottom right
			MSYS_PRIORITY_HIGHEST,
			MSYS_NO_CURSOR, MSYS_NO_CALLBACK, KeyRingSlotInvClickCallback
		);
		MSYS_SetRegionUserData( &gKeyRingRegions[cnt], 0, cnt );
	}


	// Build a mouse region here that is over any others.....
	MSYS_DefineRegion(&gItemPopupRegion, sInvX, sInvY, sInvX + sInvWidth, sInvY + sInvHeight, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MouseCallbackPrimarySecondary(ItemPopupFullRegionCallbackPrimary, ItemPopupFullRegionCallbackSecondary));


	//Disable all faces
	SetAllAutoFacesInactive( );


	fInterfacePanelDirty = DIRTYLEVEL2;

	if( guiCurrentItemDescriptionScreen != MAP_SCREEN )
	{
		EnableSMPanelButtons( FALSE , FALSE );

		// GUI_BUTTONs render after this popup's own background every frame
		// (RenderTopmostTacticalInterface(), Interface_Control.cc), and
		// EnableSMPanelButtons(FALSE, ...) above only disables them -- it
		// doesn't stop it being drawn. Without hiding it too, the "hide
		// empty attachment slots" checkbox would paint on top of this
		// popup whenever fInterfacePanelDirty == DIRTYLEVEL2 forces a
		// button refresh. Unlike Infobox_stats.sti/Infobox_skills.sti (see
		// HideSMBookmarkButtonsUnderInfoPopups()), this popup never covers
		// any of the bookmark row buttons themselves, just the checkbox.
		HideSMHideEmptySlotsCheckbox();
	}

	gfInKeyRingPopup = TRUE;

	//Reserict mouse cursor to panel
	aRect.iTop = sInvY;
	aRect.iLeft = sInvX;
	aRect.iBottom = sInvY + sInvHeight;
	aRect.iRight = sInvX + sInvWidth;

	RestrictMouseCursor( &aRect );
}


void RenderKeyRingPopup(const BOOLEAN fFullRender)
{
	const INT16 dx = gsKeyRingPopupInvX;
	const INT16 dy = gsKeyRingPopupInvY;

	if (gfInKeyRingPopup)
	{
		SetAllAutoFacesInactive();

		if (fFullRender)
		{
			FRAME_BUFFER->ShadowRect(0, dy, dx + gsKeyRingPopupInvWidth, dy + gsKeyRingPopupInvHeight);
		}
	}

	OBJECTTYPE o;
	o = OBJECTTYPE{};
	o.bStatus[0] = 100;

	if (gfTacBigKeyRingPopup) RenderTacBigKeyRingPanel();

	ETRLEObject const& pTrav = guiItemPopupBoxes->SubregionProperties(0);
	UINT32      const  box_w = pTrav.usWidth;
	UINT32      const  box_h = pTrav.usHeight;

	INT16 offset_x;
	INT16 offset_y;
	INT16 key_ring_cols;
	if (guiCurrentScreen == MAP_SCREEN)
	{
		offset_x      = MAP_KEYRING_POPUP_BOX_OFFSET_X;
		offset_y      = MAP_KEYRING_POPUP_BOX_OFFSET_Y;
		key_ring_cols = MAP_KEY_RING_ROW_WIDTH;
	}
	else
	{
		offset_x      = 0;
		offset_y      = 8;
		key_ring_cols = KEY_RING_ROW_WIDTH;
	}

	const KEY_ON_RING* const key_ring = gpItemPopupSoldier->pKeyRing;
	for (UINT32 i = 0; i < NUMBER_KEYS_ON_KEYRING; ++i)
	{
		const UINT x = dx + offset_x + i % key_ring_cols * box_w;
		const UINT y = dy + offset_y + i / key_ring_cols * box_h;

		if (!gfTacBigKeyRingPopup) BltVideoObject(FRAME_BUFFER, guiItemPopupBoxes, 0, x, y);

		const KEY_ON_RING& key = key_ring[i];
		if (!key.isValid()) continue;

		o.ubNumberOfObjects = key.ubNumber;

		auto keyId = LockTable[key.ubKeyID].usKeyItem;
		auto item = GCM->getKeyItemForKeyId(keyId);
		if (item == NULL) {
			throw std::runtime_error(ST::format("Could not find key item for key id `{}` when rendering key popup", keyId).to_std_string());
		}
		o.usItem            = item->getItemIndex();

		if (gfTacBigKeyRingPopup)
		{
			RenderTacBigKeyRingSlotItem(i, o, 0, 0);
			continue;
		}
		DrawItemUIBarEx(o, 0, x + 6, y + 31, ITEM_BAR_HEIGHT, Get16BPPColor(STATUS_BAR), Get16BPPColor(STATUS_BAR_SHADOW), FRAME_BUFFER);
		INVRenderItem(FRAME_BUFFER, NULL, o, x + 11, y -2, box_w - 8, box_h - 2, DIRTYLEVEL2, 0, SGP_TRANSPARENT);
	}

	InvalidateRegion(dx, dy, dx + gsKeyRingPopupInvWidth, dy + gsKeyRingPopupInvHeight);
}


void DeleteKeyRingPopup(void)
{
	if (!gfInKeyRingPopup) return;

	DeleteVideoObject(guiItemPopupBoxes);

	MSYS_RemoveRegion(&gItemPopupRegion);

	gfInKeyRingPopup     = FALSE;
	gfTacBigKeyRingPopup = false;

	for (INT32 i = 0; i < NUMBER_KEYS_ON_KEYRING; i++)
	{
		MSYS_RemoveRegion(&gKeyRingRegions[i]);
	}

	fInterfacePanelDirty = DIRTYLEVEL2;

	if (guiCurrentItemDescriptionScreen != MAP_SCREEN)
	{
		EnableSMPanelButtons(TRUE, FALSE);

		// Undo the HideSMHideEmptySlotsCheckbox() call from
		// InitKeyRingPopup() above.
		ShowSMHideEmptySlotsCheckbox();
	}

	FreeMouseCursor();
}

std::pair<const SGPVObject*, UINT8> GetFallbackSmallInventoryGraphicForItem(const ItemModel *item) {
	if (item->getPerPocket() != 0) {
		return std::make_pair(guiSmallInventoryGraphicMissingSmallPocket, 0);
	}
	return std::make_pair(guiSmallInventoryGraphicMissingBigPocket, 0);
}

std::pair<const SGPVObject*, UINT8> GetSmallInventoryGraphicForItem(const ItemModel *item)
{
	auto path = item->getInventoryGraphicSmall().getPath().to_lower();
	auto subImageIndex = item->getInventoryGraphicSmall().getSubImageIndex();
	auto i = allInventoryGraphics.find(path);
	if (i == allInventoryGraphics.end()) {
		SLOGE("Could not find small inventory graphic for item `{}`", item->getInternalName());
		return GetFallbackSmallInventoryGraphicForItem(item);
	}
	if (subImageIndex >= i->second->SubregionCount()) {
		SLOGE("subImageIndex out of range for small inventory graphic `{}` for item `{}`: subregion count is `{}`, subImageIndex is `{}`",
			path,
			item->getInternalName(),
			i->second->SubregionCount(),
			subImageIndex
		);
		return GetFallbackSmallInventoryGraphicForItem(item);
	}
	return std::make_pair(i->second, subImageIndex);
}

UINT16 GetTileGraphicForItem(const ItemModel * item)
{
	return GetTileIndexFromTypeSubIndex(item->getTileGraphic().tileType, item->getTileGraphic().subIndex);
}

std::pair<SGPVObject*, UINT8> GetFallbackBigInventoryGraphic() {
	return std::make_pair(AddVideoObjectFromFile(guiBigInventoryGraphicMissingPath), 0);
}

std::pair<SGPVObject*, UINT8> GetBigInventoryGraphicForItem(const ItemModel * item)
{
	auto path = item->getInventoryGraphicBig().getPath();
	auto subImageIndex = item->getInventoryGraphicBig().getSubImageIndex();

	SGPVObject* vObject = NULL;
	try {
		vObject = AddVideoObjectFromFile(path);
	} catch (const std::runtime_error &ex) {
		SLOGE("Error loading big inventory graphic for item `{}`", item->getInternalName());
	}
	if (vObject == NULL) {
		return GetFallbackBigInventoryGraphic();
	}
	if (subImageIndex >= vObject->SubregionCount()) {
		SLOGE("subImageIndex out of range for big inventory graphic `{}` for item `{}`: subregion count is `{}`, subImageIndex is `{}`",
			path,
			item->getInternalName(),
			vObject->SubregionCount(),
			subImageIndex
		);
		return GetFallbackBigInventoryGraphic();
	}
	return std::make_pair(vObject, subImageIndex);
}

// Sector-inventory "big images" toggle's own lookup -- mirrors
// GetSmallInventoryGraphicForItem() above exactly (pre-cached in
// allSectorInventoryBigGraphics by LoadInterfaceItemsGraphics(), freed by
// DeleteInterfaceItemsGraphics()), just reading the item's BIG graphic
// instead of its small one. See this function's own comment in
// Interface_Items.h for why it doesn't just reuse GetBigInventoryGraphicForItem()
// above.
std::pair<const SGPVObject*, UINT8> GetSectorInventoryBigGraphicForItem(const ItemModel *item)
{
	auto path = item->getInventoryGraphicBig().getPath().to_lower();
	auto subImageIndex = item->getInventoryGraphicBig().getSubImageIndex();
	auto i = allSectorInventoryBigGraphics.find(path);
	if (i == allSectorInventoryBigGraphics.end()) {
		SLOGE("Could not find sector-inventory big graphic for item `{}`", item->getInternalName());
		return GetFallbackBigInventoryGraphic();
	}
	if (subImageIndex >= i->second->SubregionCount()) {
		SLOGE("subImageIndex out of range for sector-inventory big graphic `{}` for item `{}`: subregion count is `{}`, subImageIndex is `{}`",
			path,
			item->getInternalName(),
			i->second->SubregionCount(),
			subImageIndex
		);
		return GetFallbackBigInventoryGraphic();
	}
	return std::make_pair(i->second, subImageIndex);
}


static void ItemDescCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	// Dropping compatible ammo/magazine directly onto the box's main
	// weapon picture reloads the gun, same as dropping it on the Eject
	// Ammo icon (TryReloadItemDescGunFromCursor()) -- scoped to the
	// picture's own rectangle (g_desc_item_box/g_desc_item_box_map, the
	// same ones RenderItemDescriptionBox() draws the picture into) so the
	// rest of the box's background keeps its plain click-to-close
	// behaviour below.
	if (gpItemPointer)
	{
		BOOLEAN const in_map = (guiCurrentItemDescriptionScreen == MAP_SCREEN) || IsTacBigItemDescBox();
		SGPBox const xy = TB(in_map ? PickMapItemDesc(g_desc_item_box_map, g_desc_item_box_map_items, g_desc_item_box_map_money, g_desc_item_box_map_weapons, g_desc_item_box_map_weapons_big, g_desc_item_box_map_items_big) : g_desc_item_box);
		BOOLEAN const fOverMainPicture =
			gusMouseXPos >= gsInvDescX + xy.x && gusMouseXPos < gsInvDescX + xy.x + xy.w &&
			gusMouseYPos >= gsInvDescY + xy.y && gusMouseYPos < gsInvDescY + xy.y + xy.h;

		if (fOverMainPicture && TryReloadItemDescGunFromCursor())
		{
			return;
		}
	}

	//Only exit the screen if we are NOT in the money interface.  Only the DONE button should exit the money interface.
	if( gpItemDescObject->usItem != MONEY )
	{
		DeleteItemDescriptionBox( );
	}
}

static void ItemDescCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	//Only exit the screen if we are NOT in the money interface.  Only the DONE button should exit the money interface.
	//if( gpItemDescObject->usItem != MONEY )
	{
		DeleteItemDescriptionBox( );
	}
}


static void RemoveMoney(void);


static void ItemDescDoneButtonCallbackPrimary(GUI_BUTTON *btn, UINT32 reason)
{
	if (gpItemDescObject->usItem == MONEY) RemoveMoney();
	DeleteItemDescriptionBox();
}

static void ItemDescDoneButtonCallbackSecondary(GUI_BUTTON *btn, UINT32 reason)
{
	DeleteItemDescriptionBox();
}


static void ItemPopupRegionCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	UINT32 uiItemPos = MSYS_GetRegionUserData( pRegion, 0 );

	//If one in our hand, place it
	if ( gpItemPointer != NULL )
	{
		if ( !PlaceObjectAtObjectIndex( gpItemPointer, gpItemPopupObject, (UINT8)uiItemPos ) )
		{
			if (fInMapMode)
			{
				MAPEndItemPointer( );
			}
			else
			{
				gpItemPointer = NULL;
				gSMPanelRegion.ChangeCursor(CURSOR_NORMAL);
				SetCurrentCursorFromDatabase( CURSOR_NORMAL );

				if (guiCurrentScreen == SHOPKEEPER_SCREEN)
				{
					gMoveingItem = INVENTORY_IN_SLOT{};
					SetSkiCursor( CURSOR_NORMAL );
				}
			}

			// re-evaluate repairs
			gfReEvaluateEveryonesNothingToDo = TRUE;
		}

		//Dirty interface
		//fInterfacePanelDirty = DIRTYLEVEL2;
		//RenderItemStackPopup( FALSE );
	}
	else
	{
		if ( uiItemPos < gpItemPopupObject->ubNumberOfObjects )
		{
			// Here, grab an item and put in cursor to swap
			//RemoveObjFrom( OBJECTTYPE * pObj, UINT8 ubRemoveIndex )
			GetObjFrom( gpItemPopupObject, (UINT8)uiItemPos, &gItemPointer );

			if (fInMapMode)
			{
				// pick it up
				InternalMAPBeginItemPointer( gpItemPopupSoldier );
			}
			else
			{
				SetItemPointer(&gItemPointer, gpItemPopupSoldier);
			}

			//if we are in the shop keeper interface
			if (guiCurrentScreen == SHOPKEEPER_SCREEN)
			{
				// pick up stacked item into cursor and try to sell it ( unless CTRL is held down )
				BeginSkiItemPointer(PLAYERS_INVENTORY, -1, !_KeyDown(CTRL));

				// if we've just removed the last one there
				if ( gpItemPopupObject->ubNumberOfObjects == 0 )
				{
					// we must immediately get out of item stack popup, because the item has been deleted
					// (memset to 0), and errors like a right bringing up an item description for item 0
					// could happen then.  ARM.
					DeleteItemStackPopup( );
				}
			}

			// re-evaluate repairs
			gfReEvaluateEveryonesNothingToDo = TRUE;

			//Dirty interface
			//RenderItemStackPopup( FALSE );
			//fInterfacePanelDirty = DIRTYLEVEL2;
		}
	}

	UpdateItemHatches();
}

static void ItemPopupRegionCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	UINT32 uiItemPos = MSYS_GetRegionUserData( pRegion, 0 );

	DeleteItemStackPopup( );

	if ( !InItemDescriptionBox( ) )
	{
		// RESTORE BACKGROUND
		RestoreExternBackgroundRect( gsItemPopupInvX, gsItemPopupInvY, gsItemPopupInvWidth, gsItemPopupInvHeight );
		if ( guiCurrentItemDescriptionScreen == MAP_SCREEN )
		{
			MAPInternalInitItemDescriptionBox( gpItemPopupObject, (UINT8)uiItemPos, gpItemPopupSoldier );
		}
		else
		{
			InternalInitItemDescriptionBox( gpItemPopupObject, (INT16) ITEMDESC_START_X, (INT16) ITEMDESC_START_Y, (UINT8)uiItemPos, gpItemPopupSoldier );
		}
	}
}


static void ItemPopupFullRegionCallbackPrimary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if ( InItemStackPopup( ) )
	{
		// Unconditional close, same as the keyring branch below (and the
		// stack popup's own right-click, ItemPopupFullRegionCallbackSecondary)
		// -- previously only closed via EndItemStackPopupWithItemInHand() if
		// an item was already on the cursor, so left-click on the background
		// silently did nothing otherwise, unlike the keyring's.
		DeleteItemStackPopup( );
		fTeamPanelDirty = TRUE;
	}
	else if( InKeyRingPopup() )
	{
		// end pop up with key in hand
		DeleteKeyRingPopup( );
		fTeamPanelDirty = TRUE;

	}
}

static void ItemPopupFullRegionCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if ( InItemStackPopup( ) )
	{
		DeleteItemStackPopup( );
		fTeamPanelDirty = TRUE;
	}
	else
	{
		DeleteKeyRingPopup( );
		fTeamPanelDirty = TRUE;
	}
}

#define NUM_PICKUP_SLOTS				6

struct ITEM_PICKUP_MENU_STRUCT
{
	ITEM_POOL *pItemPool;
	INT16 sX;
	INT16 sY;
	INT16 sWidth;
	INT16 sHeight;
	INT8 bScrollPage;
	INT32 ubScrollAnchor;
	INT32 ubTotalItems;
	INT32 bCurSelect;
	UINT8 bNumSlotsPerPage;
	SGPVObject* uiPanelVo;
	BUTTON_PICS* iUpButtonImages;
	BUTTON_PICS* iDownButtonImages;
	BUTTON_PICS* iAllButtonImages;
	BUTTON_PICS* iCancelButtonImages;
	BUTTON_PICS* iOKButtonImages;
	GUIButtonRef iUpButton;
	GUIButtonRef iDownButton;
	GUIButtonRef iAllButton;
	GUIButtonRef iOKButton;
	GUIButtonRef iCancelButton;
	BOOLEAN fDirtyLevel;
	BOOLEAN fHandled;
	INT16 sGridNo;
	INT8 bZLevel;
	INT16 sButtomPanelStartY;
	SOLDIERTYPE *pSoldier;
	INT32 items[NUM_PICKUP_SLOTS];
	MOUSE_REGION Regions[ NUM_PICKUP_SLOTS ];
	MOUSE_REGION BackRegions;
	MOUSE_REGION BackRegion;
	BOOLEAN *pfSelectedArray;
	OBJECTTYPE CompAmmoObject;
	BOOLEAN fAllSelected;
};

#define ITEMPICK_UP_X					55
#define ITEMPICK_UP_Y					5
#define ITEMPICK_DOWN_X				111
#define ITEMPICK_DOWN_Y				5
#define ITEMPICK_ALL_X					79
#define ITEMPICK_ALL_Y					6
#define ITEMPICK_OK_X					16
#define ITEMPICK_OK_Y					6
#define ITEMPICK_CANCEL_X				141
#define ITEMPICK_CANCEL_Y				6

#define ITEMPICK_START_X_OFFSET			10

#define ITEMPICK_GRAPHIC_X				10
#define ITEMPICK_GRAPHIC_Y				12
#define ITEMPICK_GRAPHIC_YSPACE			26

#define ITEMPICK_TEXT_X				56
#define ITEMPICK_TEXT_Y				22
#define ITEMPICK_TEXT_YSPACE				26
#define ITEMPICK_TEXT_WIDTH				109


static ITEM_PICKUP_MENU_STRUCT gItemPickupMenu;
BOOLEAN gfInItemPickupMenu = FALSE;


// STUFF FOR POPUP ITEM INFO BOX
void SetItemPickupMenuDirty( BOOLEAN fDirtyLevel )
{
	gItemPickupMenu.fDirtyLevel = fDirtyLevel;
}


static void CalculateItemPickupMenuDimensions(void);
static void ItemPickMenuMouseClickCallback(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemPickMenuMouseMoveCallback(MOUSE_REGION* pRegion, UINT32 iReason);
static void ItemPickupAll(GUI_BUTTON* btn, UINT32 reason);
static void ItemPickupCancel(GUI_BUTTON* btn, UINT32 reason);
static void ItemPickupOK(GUI_BUTTON* btn, UINT32 reason);
static void ItemPickupScrollDown(GUI_BUTTON* btn, UINT32 reason);
static void ItemPickupScrollUp(GUI_BUTTON* btn, UINT32 reason);
static void SetupPickupPage(INT8 bPage);


void InitializeItemPickupMenu(SOLDIERTYPE* const pSoldier, INT16 const sGridNo, ITEM_POOL* const pItemPool, INT8 const bZLevel)
{
	EraseInterfaceMenus(TRUE);
	LocateSoldier(pSoldier, FALSE);

	ITEM_PICKUP_MENU_STRUCT& menu = gItemPickupMenu;
	menu = ITEM_PICKUP_MENU_STRUCT{};
	menu.pItemPool = pItemPool;

	InterruptTime();
	PauseGame();
	LockPauseState(LOCK_PAUSE_ITEM_PICKUP);
	PauseTime(TRUE);

	// Alrighty, cancel lock UI if we havn't done so already
	UnSetUIBusy(pSoldier);

	// Change to INV panel if not there already...
	SetNewPanel(pSoldier);

	//Determine total #
	INT32 cnt = 0;
	for (ITEM_POOL* i = pItemPool; i; i = i->pNext)
	{
		if (!ItemPoolOKForDisplay(i, bZLevel)) continue;
		++cnt;
	}
	menu.ubTotalItems = (UINT8)cnt;

	// Determine # of slots per page
	menu.bNumSlotsPerPage = menu.ubTotalItems < NUM_PICKUP_SLOTS ?
		menu.ubTotalItems : NUM_PICKUP_SLOTS;

	menu.uiPanelVo = AddVideoObjectFromFile(INTERFACEDIR "/itembox.sti");

	menu.pfSelectedArray = new BOOLEAN[menu.ubTotalItems]{};

	CalculateItemPickupMenuDimensions();

	// First get mouse xy screen location
	INT16 sX = gusMouseXPos;
	INT16 sY = gusMouseYPos;

	// CHECK FOR LEFT/RIGHT
	if (sX + menu.sWidth > SCREEN_WIDTH)
	{
		sX = SCREEN_WIDTH - menu.sWidth - ITEMPICK_START_X_OFFSET;
	}
	else
	{
		sX = sX + ITEMPICK_START_X_OFFSET;
	}

	// Now check for top
	// Center in the y
	INT16 const sCenterYVal = menu.sHeight / 2;

	sY -= sCenterYVal;
	if (sY < gsVIEWPORT_WINDOW_START_Y)
	{
		sY = gsVIEWPORT_WINDOW_START_Y;
	}

	// Check for bottom
	if (sY + menu.sHeight > gsVIEWPORT_WINDOW_END_Y)
	{
		sY = gsVIEWPORT_WINDOW_END_Y - menu.sHeight;
	}

	menu.sX           = sX;
	menu.sY           = sY;
	menu.bCurSelect   = 0;
	menu.pSoldier     = pSoldier;
	menu.fHandled     = FALSE;
	menu.sGridNo      = sGridNo;
	menu.bZLevel      = bZLevel;
	menu.fAllSelected = FALSE;

	//Load images for buttons
	BUTTON_PICS* const pics  = LoadButtonImage(INTERFACEDIR "/itembox.sti", 5, 10);
	menu.iUpButtonImages     = pics;
	menu.iDownButtonImages   = UseLoadedButtonImage(pics, 7, 12);
	menu.iAllButtonImages    = UseLoadedButtonImage(pics, 6, 11);
	menu.iCancelButtonImages = UseLoadedButtonImage(pics, 8, 13);
	menu.iOKButtonImages     = UseLoadedButtonImage(pics, 4,  9);

	// Build a mouse region here that is over any others.....
	MSYS_DefineRegion(&menu.BackRegion, 532, 367, SCREEN_WIDTH, SCREEN_HEIGHT, MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

	// Build a mouse region here that is over any others.....
	MSYS_DefineRegion(&menu.BackRegions, sX, sY, menu.sX + menu.sWidth, sY + menu.sHeight, MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

	INT16 const by = sY + menu.sButtomPanelStartY;

	// Create buttons
	if (menu.bNumSlotsPerPage == NUM_PICKUP_SLOTS && menu.ubTotalItems > NUM_PICKUP_SLOTS)
	{
		menu.iUpButton = QuickCreateButton(menu.iUpButtonImages, sX + ITEMPICK_UP_X, by + ITEMPICK_UP_Y, MSYS_PRIORITY_HIGHEST, ItemPickupScrollUp);
		menu.iUpButton->SetFastHelpText(ItemPickupHelpPopup[1]);

		menu.iDownButton = QuickCreateButton(menu.iDownButtonImages, sX + ITEMPICK_DOWN_X, by + ITEMPICK_DOWN_Y, MSYS_PRIORITY_HIGHEST, ItemPickupScrollDown);
		menu.iDownButton->SetFastHelpText(ItemPickupHelpPopup[3]);
	}

	menu.iOKButton = QuickCreateButton(menu.iOKButtonImages, sX + ITEMPICK_OK_X, by + ITEMPICK_OK_Y, MSYS_PRIORITY_HIGHEST, ItemPickupOK);
	menu.iOKButton->SetFastHelpText(ItemPickupHelpPopup[0]);

	menu.iAllButton = QuickCreateButton(menu.iAllButtonImages, sX + ITEMPICK_ALL_X, by + ITEMPICK_ALL_Y, MSYS_PRIORITY_HIGHEST, ItemPickupAll);
	menu.iAllButton->SetFastHelpText(ItemPickupHelpPopup[2]);

	menu.iCancelButton = QuickCreateButton(menu.iCancelButtonImages, sX + ITEMPICK_CANCEL_X, by + ITEMPICK_CANCEL_Y, MSYS_PRIORITY_HIGHEST, ItemPickupCancel);
	menu.iCancelButton->SetFastHelpText(ItemPickupHelpPopup[4]);

	DisableButton(menu.iOKButton);

	// Create regions
	INT16 const sCenX = sX;
	INT16       sCenY = sY + ITEMPICK_GRAPHIC_Y;
	for (INT32 i = 0; i < menu.bNumSlotsPerPage; ++i)
	{
		MOUSE_REGION* const r = &menu.Regions[i];
		MSYS_DefineRegion(r, sCenX, sCenY + 1, sCenX + menu.sWidth, sCenY + ITEMPICK_GRAPHIC_YSPACE, MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL, ItemPickMenuMouseMoveCallback, ItemPickMenuMouseClickCallback);
		MSYS_SetRegionUserData(r, 0, i);

		sCenY += ITEMPICK_GRAPHIC_YSPACE;
	}

	SetupPickupPage(0);

	gfInItemPickupMenu = TRUE;
	gfIgnoreScrolling  = TRUE;

	HandleAnyMercInSquadHasCompatibleStuff(NULL);
	gSelectSMPanelToMerc = pSoldier;
	ReEvaluateDisabledINVPanelButtons();
	DisableTacticalTeamPanelButtons(TRUE);
}


static void SetupPickupPage(INT8 bPage)
{
	INT32 cnt, iStart, iEnd;
	ITEM_POOL *pTempItemPool;
	INT16 sValue;

	// Reset page slots
	FOR_EACH(INT32, i, gItemPickupMenu.items)
	{
		*i = -1;
	}

	// Get lower bound
	iStart = bPage * NUM_PICKUP_SLOTS;
	if ( iStart > gItemPickupMenu.ubTotalItems )
	{
		return;
	}


	iEnd   = iStart + NUM_PICKUP_SLOTS;
	if ( iEnd >= gItemPickupMenu.ubTotalItems )
	{
		iEnd = gItemPickupMenu.ubTotalItems;
	}

	// Setup slots!
	// These slots contain an inventory pool pointer for each slot...
	pTempItemPool = gItemPickupMenu.pItemPool;

	// ATE: Patch fix here for crash :(
	// Clear help text!
	for ( cnt = 0; cnt < NUM_PICKUP_SLOTS; cnt++ )
	{
		gItemPickupMenu.Regions[cnt].SetFastHelpText({});
	}

	for ( cnt = 0; cnt < iEnd; )
	{
		// Move to the closest one that can be displayed....
		while( !ItemPoolOKForDisplay( pTempItemPool, gItemPickupMenu.bZLevel ) )
		{
			pTempItemPool = pTempItemPool->pNext;
		}

		if ( cnt >= iStart )
		{
			INT32 const item = pTempItemPool->iItemIndex;
			gItemPickupMenu.items[cnt - iStart] = item;

			OBJECTTYPE const& o = GetWorldItem(item).o;

			sValue = o.bStatus[0];

			// Adjust for ammo, other thingys..
			ST::string pStr;
			if (GCM->getItem(o.usItem)->isAmmo() || GCM->getItem(o.usItem)->isKey())
			{
				pStr.clear();
			}
			else
			{
				pStr = ST::format("{}%", sValue);
			}

			gItemPickupMenu.Regions[cnt - iStart].SetFastHelpText(pStr);
		}

		cnt++;

		pTempItemPool = pTempItemPool->pNext;
	}

	gItemPickupMenu.bScrollPage = bPage;
	gItemPickupMenu.ubScrollAnchor = (UINT8)iStart;

	if ( gItemPickupMenu.bNumSlotsPerPage == NUM_PICKUP_SLOTS && gItemPickupMenu.ubTotalItems > NUM_PICKUP_SLOTS )
	{
		// Setup enabled/disabled buttons
		EnableButton(gItemPickupMenu.iUpButton, bPage > 0);
		// Setup enabled/disabled buttons
		EnableButton(gItemPickupMenu.iDownButton, iEnd < gItemPickupMenu.ubTotalItems);
	}
	SetItemPickupMenuDirty( DIRTYLEVEL2 );

}


static void CalculateItemPickupMenuDimensions(void)
{
	// Build background
	INT16 sY = 0;

	for (INT32 cnt = 0; cnt < gItemPickupMenu.bNumSlotsPerPage; cnt++)
	{
		// Add height of object
		UINT16 usSubRegion = (cnt == 0 ? 0 : 1);
		ETRLEObject const& ETRLEProps = gItemPickupMenu.uiPanelVo->SubregionProperties(usSubRegion);
		sY += ETRLEProps.usHeight;
	}
	gItemPickupMenu.sButtomPanelStartY = sY;

	// Do end
	ETRLEObject const& ETRLEProps = gItemPickupMenu.uiPanelVo->SubregionProperties(2);
	sY += ETRLEProps.usHeight;

	// Set height, width
	gItemPickupMenu.sHeight = sY;
	gItemPickupMenu.sWidth  = ETRLEProps.usWidth;
}


void RenderItemPickupMenu()
{
	ST::string pStr;

	if (!gfInItemPickupMenu) return;

	ITEM_PICKUP_MENU_STRUCT& menu = gItemPickupMenu;
	if (menu.fDirtyLevel != DIRTYLEVEL2) return;

	MarkButtonsDirty();

	// Build background
	INT16 sX = menu.sX;
	INT16 sY = menu.sY;

	for (INT32 cnt = 0; cnt < menu.bNumSlotsPerPage; ++cnt)
	{
		UINT16 const usSubRegion = (cnt == 0 ? 0 : 1);

		BltVideoObject(FRAME_BUFFER, menu.uiPanelVo, usSubRegion, sX, sY);

		// Add height of object
		ETRLEObject const& ETRLEProps = menu.uiPanelVo->SubregionProperties(usSubRegion);
		sY += ETRLEProps.usHeight;
	}

	// Do end
	UINT16 const gfx =
		menu.bNumSlotsPerPage == NUM_PICKUP_SLOTS &&
		menu.ubTotalItems     >  NUM_PICKUP_SLOTS ?
			2 : 3;
	BltVideoObject(FRAME_BUFFER, menu.uiPanelVo, gfx, sX, sY);

	// Render items....
	sX = menu.sX + ITEMPICK_GRAPHIC_X;
	sY = menu.sY + ITEMPICK_GRAPHIC_Y;

	SetFont(ITEMDESC_FONT);
	SetFontBackground(FONT_MCOLOR_BLACK);
	SetFontShadow(ITEMDESC_FONTSHADOW2);

	{
		SGPVSurface::Lock l(FRAME_BUFFER);
		UINT16* const pDestBuf         = l.Buffer<UINT16>();
		UINT32  const uiDestPitchBYTES = l.Pitch();

		UINT16 const outline_col = Get16BPPColor(FROMRGB(255, 255, 0));
		for (INT32 cnt = 0; cnt < menu.bNumSlotsPerPage; ++cnt)
		{
			INT32 const world_item = menu.items[cnt];
			if (world_item == -1) continue;

			// Get item to render
			OBJECTTYPE const& o    = GetWorldItem(world_item).o;
			const ItemModel * item = GCM->getItem(o.usItem);

			UINT16              const usItemTileIndex = GetTileGraphicForItem(item);
			TILE_ELEMENT const* const te              = &gTileDatabase[usItemTileIndex];

			// ATE: Adjust to basic shade.....
			te->hTileSurface->CurrentShade(4);

			UINT16 const outline = menu.pfSelectedArray[cnt + menu.ubScrollAnchor] ? outline_col : SGP_TRANSPARENT;
			Blt8BPPDataTo16BPPBufferOutline(pDestBuf, uiDestPitchBYTES, te->hTileSurface, sX, sY, te->usRegionIndex, outline);

			if (o.ubNumberOfObjects > 1)
			{
				SetFontAttributes(ITEM_FONT, FONT_GRAY4);

				pStr = ST::format("{}", o.ubNumberOfObjects);

				INT16 sFontX;
				INT16 sFontY;
				FindFontRightCoordinates(sX - 4, sY + 14, 42, 1, pStr, ITEM_FONT, &sFontX, &sFontY);
				MPrintBuffer(pDestBuf, uiDestPitchBYTES, sFontX, sFontY, pStr);
				SetFont(ITEMDESC_FONT);
			}

			if (ItemHasAttachments(o))
			{
				// Render attachment symbols
				SetFontForeground(GetAttachmentHintColor(&o));
				SetFontShadow(DEFAULT_SHADOW);
				ST::string AttachMarker = "*";
				UINT16         const uiStringLength = StringPixLength(AttachMarker, ITEM_FONT);
				INT16          const sNewX          = sX + 43 - uiStringLength - 4;
				INT16          const sNewY          = sY + 2;
				MPrintBuffer(pDestBuf, uiDestPitchBYTES, sNewX, sNewY, AttachMarker);
			}

			if (menu.bCurSelect == cnt + menu.ubScrollAnchor)
			{
				SetFontForeground(FONT_WHITE);
				SetFontShadow(DEFAULT_SHADOW);
			}
			else
			{
				SetFontForeground(FONT_BLACK);
				SetFontShadow(ITEMDESC_FONTSHADOW2);
			}

			// Render name
			if (item->getItemClass() == IC_MONEY)
			{
				ST::string pStr2 = SPrintMoney(o.uiMoneyAmount);
				pStr = ST::format("{} ({})", GCM->getItem(o.usItem)->getName(), pStr2);
			}
			else
			{
				pStr = GCM->getItem(o.usItem)->getShortName();
			}
			INT16 sFontX;
			INT16 sFontY;
			INT16 const x = ITEMPICK_TEXT_X + menu.sX;
			INT16 const y = ITEMPICK_TEXT_Y + menu.sY + ITEMPICK_TEXT_YSPACE * cnt;
			FindFontCenterCoordinates(x, y, ITEMPICK_TEXT_WIDTH, 1, pStr, ITEMDESC_FONT, &sFontX, &sFontY);
			MPrintBuffer(pDestBuf, uiDestPitchBYTES, sFontX, sFontY, pStr);

			sY += ITEMPICK_GRAPHIC_YSPACE;
		}
	}

	SetFontShadow(DEFAULT_SHADOW);
	InvalidateRegion(menu.sX, menu.sY, menu.sX + menu.sWidth, menu.sY + menu.sHeight);
	menu.fDirtyLevel = 0;
}


void RemoveItemPickupMenu( )
{
	INT32 cnt;

	if ( gfInItemPickupMenu )
	{
		gfSMDisableForItems = FALSE;

		HandleAnyMercInSquadHasCompatibleStuff(NULL);

		UnLockPauseState();
		UnPauseGame();
		// UnPause timers as well....
		PauseTime( FALSE );

		// Unfreese guy!
		gItemPickupMenu.pSoldier->fPauseAllAnimation = FALSE;

		DeleteVideoObject(gItemPickupMenu.uiPanelVo);

		// Remove buttons
		if ( gItemPickupMenu.bNumSlotsPerPage == NUM_PICKUP_SLOTS && gItemPickupMenu.ubTotalItems > NUM_PICKUP_SLOTS )
		{
			RemoveButton( gItemPickupMenu.iUpButton );
			RemoveButton( gItemPickupMenu.iDownButton );
		}
		RemoveButton( gItemPickupMenu.iAllButton );
		RemoveButton( gItemPickupMenu.iOKButton );
		RemoveButton( gItemPickupMenu.iCancelButton );

		// Remove button images
		UnloadButtonImage( gItemPickupMenu.iUpButtonImages );
		UnloadButtonImage( gItemPickupMenu.iDownButtonImages );
		UnloadButtonImage( gItemPickupMenu.iAllButtonImages );
		UnloadButtonImage( gItemPickupMenu.iCancelButtonImages );
		UnloadButtonImage( gItemPickupMenu.iOKButtonImages );

		MSYS_RemoveRegion( &(gItemPickupMenu.BackRegions ) );
		MSYS_RemoveRegion( &(gItemPickupMenu.BackRegion ) );

		// Remove regions
		for ( cnt = 0; cnt < gItemPickupMenu.bNumSlotsPerPage; cnt++ )
		{
			MSYS_RemoveRegion( &(gItemPickupMenu.Regions[cnt]));
		}

		// Free selection list...
		delete[] gItemPickupMenu.pfSelectedArray;
		gItemPickupMenu.pfSelectedArray = NULL;


		// Set cursor back to normal mode...
		guiPendingOverrideEvent = A_CHANGE_TO_MOVE;

		// Rerender world
		SetRenderFlags( RENDER_FLAG_FULL );

		gfInItemPickupMenu = FALSE;

		//gfSMDisableForItems = FALSE;
		EnableSMPanelButtons( TRUE , TRUE );
		gfSMDisableForItems = FALSE;

		fInterfacePanelDirty = DIRTYLEVEL2;

		// Turn off Ignore scrolling
		gfIgnoreScrolling = FALSE;
		DisableTacticalTeamPanelButtons( FALSE );
		gSelectSMPanelToMerc = gpSMCurrentMerc;
	}
}


static void ItemPickupScrollUp(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		SetupPickupPage( (UINT8)( gItemPickupMenu.bScrollPage - 1 ) );
	}
}


static void ItemPickupScrollDown(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		SetupPickupPage( (UINT8)( gItemPickupMenu.bScrollPage + 1 ) );
	}
}


static void ItemPickupAll(GUI_BUTTON* btn, UINT32 reason)
{
	INT32 cnt;

	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		gItemPickupMenu.fAllSelected = !gItemPickupMenu.fAllSelected;


		// OK, pickup item....
		//gItemPickupMenu.fHandled = TRUE;
		// Tell our soldier to pickup this item!
		//SoldierGetItemFromWorld( gItemPickupMenu.pSoldier, ITEM_PICKUP_ACTION_ALL, gItemPickupMenu.sGridNo, gItemPickupMenu.bZLevel, NULL );
		for ( cnt = 0; cnt < gItemPickupMenu.ubTotalItems; cnt++ )
		{
			gItemPickupMenu.pfSelectedArray[ cnt ] = gItemPickupMenu.fAllSelected;
		}

		EnableButton(gItemPickupMenu.iOKButton, gItemPickupMenu.fAllSelected);
	}
}


static void ItemPickupOK(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		// OK, pickup item....
		gItemPickupMenu.fHandled = TRUE;

		// Tell our soldier to pickup this item!
		SoldierGetItemFromWorld( gItemPickupMenu.pSoldier, ITEM_PICKUP_SELECTION, gItemPickupMenu.sGridNo, gItemPickupMenu.bZLevel, gItemPickupMenu.pfSelectedArray );
	}
}


static void ItemPickupCancel(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		// OK, pickup item....
		gItemPickupMenu.fHandled = TRUE;
	}
}


static void ItemPickMenuMouseMoveCallback(MOUSE_REGION* const pRegion, UINT32 const iReason)
{
	static BOOLEAN bChecked = FALSE;

	if (iReason & MSYS_CALLBACK_REASON_MOVE)
	{
		UINT32 const uiItemPos = MSYS_GetRegionUserData(pRegion, 0);
		INT32  const bPos      = uiItemPos + gItemPickupMenu.ubScrollAnchor;
		if (bPos >= gItemPickupMenu.ubTotalItems) return;

		gItemPickupMenu.bCurSelect = bPos;

		if (bChecked) return;

		// Show compatible ammo
		INT32      const  item = gItemPickupMenu.items[uiItemPos];
		OBJECTTYPE const& o    = GetWorldItem(item).o;

		gItemPickupMenu.CompAmmoObject = o;

		HandleAnyMercInSquadHasCompatibleStuff(0); // Turn off first
		InternalHandleCompatibleAmmoUI(gpSMCurrentMerc, &gItemPickupMenu.CompAmmoObject, TRUE);
		HandleAnyMercInSquadHasCompatibleStuff(&o);

		SetItemPickupMenuDirty(DIRTYLEVEL2);

		bChecked = TRUE;
	}
	else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		gItemPickupMenu.bCurSelect = 255;

		InternalHandleCompatibleAmmoUI(gpSMCurrentMerc, &gItemPickupMenu.CompAmmoObject, FALSE);
		HandleAnyMercInSquadHasCompatibleStuff(NULL);

		SetItemPickupMenuDirty(DIRTYLEVEL2);

		bChecked = FALSE;
	}
}


static void ItemPickMenuMouseClickCallback(MOUSE_REGION* const pRegion, UINT32 const iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		INT32 const item_pos = MSYS_GetRegionUserData(pRegion, 0) + gItemPickupMenu.ubScrollAnchor;
		if (item_pos >= gItemPickupMenu.ubTotalItems) return;

		BOOLEAN& selected = gItemPickupMenu.pfSelectedArray[item_pos];
		selected = !selected;

		// Loop through all and set /unset OK
		bool enable = false;
		for (UINT8 i = 0; i < gItemPickupMenu.ubTotalItems; ++i)
		{
			if (!gItemPickupMenu.pfSelectedArray[i]) continue;
			enable = true;
			break;
		}
		EnableButton(gItemPickupMenu.iOKButton, enable);
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		INT8 const page = gItemPickupMenu.bScrollPage;
		if (page > 0) SetupPickupPage(page - 1);
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		INT8 const page = gItemPickupMenu.bScrollPage;
		if ((page + 1) * NUM_PICKUP_SLOTS < gItemPickupMenu.ubTotalItems)
		{
			SetupPickupPage(page + 1);
		}
	}
}


BOOLEAN HandleItemPickupMenu( )
{

	if ( !gfInItemPickupMenu )
	{
		return( FALSE );
	}

	if ( gItemPickupMenu.fHandled )
	{
		RemoveItemPickupMenu( );
	}

	return( gItemPickupMenu.fHandled );
}


static void BtnMoneyButtonCallbackPrimary(GUI_BUTTON* const btn, UINT32 const reason)
{
	UINT32      amount   = 0;
	UINT8 const ubButton = btn->GetUserData();
	switch (ubButton)
	{
		case M_1000: amount = 1000; break;
		case M_100:  amount =  100; break;
		case M_10:   amount =   10; break;

		case M_DONE:
			RemoveMoney();
			DeleteItemDescriptionBox();
			break;
	}

	if (amount != 0 && gRemoveMoney.uiMoneyRemaining >= amount)
	{
		if (gfAddingMoneyToMercFromPlayersAccount && gRemoveMoney.uiMoneyRemoving + amount > MAX_MONEY_PER_SLOT)
		{
			if (guiCurrentScreen == MAP_SCREEN)
			{
				DoMapMessageBox(MSG_BOX_BASIC_STYLE, gzMoneyWithdrawMessageText[MONEY_TEXT_WITHDRAW_MORE_THEN_MAXIMUM], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
				return;
			}
			ScreenID const exit_screen = guiCurrentScreen == SHOPKEEPER_SCREEN ?
				SHOPKEEPER_SCREEN : GAME_SCREEN;
			DoMessageBox(MSG_BOX_BASIC_STYLE, gzMoneyWithdrawMessageText[MONEY_TEXT_WITHDRAW_MORE_THEN_MAXIMUM], exit_screen, MSG_BOX_FLAG_OK, NULL, NULL);
			return;
		}

		gRemoveMoney.uiMoneyRemaining -= amount;
		gRemoveMoney.uiMoneyRemoving  += amount;

		RenderItemDescriptionBox( );
		for (INT8 i = 0; i < NUM_MONEY_BUTTONS; ++i)
		{
			MarkAButtonDirty(guiMoneyButtonBtn[i]);
		}
	}
}

static void BtnMoneyButtonCallbackSecondary(GUI_BUTTON* const btn, UINT32 const reason)
{
	btn->uiFlags &= ~BUTTON_CLICKED_ON;

	UINT32      amount   = 0;
	UINT8 const ubButton = btn->GetUserData();
	switch (ubButton)
	{
		case M_1000: amount = 1000; break;
		case M_100:  amount =  100; break;
		case M_10:   amount =   10; break;
	}

	if (amount != 0 && gRemoveMoney.uiMoneyRemoving >= amount)
	{
		gRemoveMoney.uiMoneyRemaining += amount;
		gRemoveMoney.uiMoneyRemoving  -= amount;
	}

	RenderItemDescriptionBox();
	for (INT8 i = 0; i < NUM_MONEY_BUTTONS; ++i)
	{
		MarkAButtonDirty(guiMoneyButtonBtn[i]);
	}
}

static void BtnMoneyButtonCallbackOther(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
}


static void RemoveMoney(void)
{
	if( gRemoveMoney.uiMoneyRemoving != 0 )
	{
		//if we are in the shop keeper interface
		if (guiCurrentScreen == SHOPKEEPER_SCREEN)
		{
			INVENTORY_IN_SLOT InvSlot;

			InvSlot = INVENTORY_IN_SLOT{};

			InvSlot.fActive = TRUE;
			InvSlot.sItemIndex = MONEY;
			InvSlot.bSlotIdInOtherLocation = -1;

			//Remove the money from the money in the pocket
			gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemaining;

				//Create an item to get the money that is being removed
			CreateMoney(gRemoveMoney.uiMoneyRemoving, &InvSlot.ItemObject);

			InvSlot.ubIdOfMercWhoOwnsTheItem = gpItemDescSoldier->ubProfile;

			//if we are removing money from the players account
			if( gfAddingMoneyToMercFromPlayersAccount )
			{
				gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemoving;

				//take the money from the player
				AddTransactionToPlayersBook ( TRANSFER_FUNDS_TO_MERC, gpSMCurrentMerc->ubProfile, GetWorldTotalMin() , -(INT32)( gpItemDescObject->uiMoneyAmount ) );
			}

			gMoveingItem = InvSlot;

			gItemPointer = InvSlot.ItemObject;
			SetItemPointer(&gItemPointer, gpSMCurrentMerc);

			// Set mouse
			SetSkiCursor( EXTERN_CURSOR );

			//Restrict the cursor to the proper area
			RestrictSkiMouseCursor();
		}
		else
		{
			CreateMoney( gRemoveMoney.uiMoneyRemoving, &gItemPointer );
			SetItemPointer(&gItemPointer, gpItemDescSoldier);

			//Remove the money from the money in the pocket
			//if we are removing money from the players account
			if( gfAddingMoneyToMercFromPlayersAccount )
			{
				gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemoving;

				//take the money from the player -- booked to the merc the box was
				//opened for (gpSMCurrentMerc on the tactical panel, the selected
				//character on the map screen, where gpSMCurrentMerc isn't his)
				AddTransactionToPlayersBook ( TRANSFER_FUNDS_TO_MERC, gpItemDescSoldier->ubProfile, GetWorldTotalMin() , -(INT32)( gpItemDescObject->uiMoneyAmount ) );
			}
			else
				gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemaining;



			if( guiCurrentItemDescriptionScreen == MAP_SCREEN )
			{
				SetMapCursorItem();
				fTeamPanelDirty=TRUE;
			}

		}
	}

	//if( gfAddingMoneyToMercFromPlayersAccount )
	//	gfAddingMoneyToMercFromPlayersAccount = FALSE;
}


ST::string GetHelpTextForItem(const OBJECTTYPE& obj)
{
	ST::string dst;
	UINT16 const usItem = obj.usItem;
	if (usItem == MONEY)
	{
		dst = SPrintMoney(obj.uiMoneyAmount);
	}
	else if (GCM->getItem(usItem)->getItemClass() == IC_MONEY)
	{
		// alternate money like silver or gold
		ST::string pStr2 = SPrintMoney(obj.uiMoneyAmount);
		dst = ST::format("{} ({})", GCM->getItem(usItem)->getName(), pStr2);
	}
	else if (usItem == NOTHING)
	{
		dst.clear();
	}
	else
	{
		dst = ST::format("{}", GCM->getItem(usItem)->getName());
		if (!gGameOptions.fGunNut && GCM->getItem(usItem)->getItemClass() == IC_GUN)
		{
			const CalibreModel * calibre = GCM->getWeapon(usItem)->calibre;
			if (calibre->showInHelpText)
			{
				ST::string name = *calibre->getName();
				dst += ST::format(" ({})", name);
			}
		}

		ST::string imprint = GetObjectImprint(obj);
		if (!imprint.empty())
		{
			dst += ST::format(" [{}]", imprint);
		}

		// Add attachment string....
		ST::string first_prefix = " (";
		ST::string prefix       = first_prefix;
		FOR_EACH(UINT16 const, i, obj.usAttachItem)
		{
			UINT16 const attachment = *i;
			if (attachment == NOTHING) continue;

			dst += ST::format("{}{}", prefix, GCM->getItem(attachment)->getName());
			prefix = ",\n";
		}
		if (prefix != first_prefix)
		{
			dst += ST::format("{}", pMessageStrings[MSG_END_ATTACHMENT_LIST]);
		}
	}
	return dst;
}


void CancelItemPointer( )
{
	// ATE: If we have an item pointer end it!
	if ( gpItemPointer != NULL )
	{
		if ( gbItemPointerSrcSlot != NO_SLOT )
		{
			// Place it back in our hands!
			PlaceObject( gpItemPointerSoldier, gbItemPointerSrcSlot, gpItemPointer );

			// ATE: This could potnetially swap!
			// Make sure # of items is 0, if not, auto place somewhere else...
			if ( gpItemPointer->ubNumberOfObjects > 0 )
			{
				if ( !AutoPlaceObject( gpItemPointerSoldier, gpItemPointer, FALSE ) )
				{
					// Alright, place of the friggen ground!
					AddItemToPool(gpItemPointerSoldier->sGridNo, gpItemPointer, VISIBLE, gpItemPointerSoldier->bLevel, 0 , -1);
					NotifySoldiersToLookforItems( );
				}
			}
		}
		else
		{
			// We drop it here.....
			AddItemToPool(gpItemPointerSoldier->sGridNo, gpItemPointer, VISIBLE, gpItemPointerSoldier->bLevel, 0 , -1);
			NotifySoldiersToLookforItems( );
		}
		EndItemPointer( );
	}
}


void LoadItemCursorFromSavedGame(HWFILE const f)
{
	// Sized for ExtractObject() (OBJECTTYPE at the current MAX_OBJECTS_PER_SLOT/
	// MAX_ATTACHMENTS) plus SoldierID + slot + active flag + 5 bytes of padding.
	// 92 at OBJECTTYPE == 84; grown by the same +88 ExtractObject() itself grew
	// by when MAX_OBJECTS_PER_SLOT went from 8 to 100.
	BYTE data[180];
	f->read(data, sizeof(data));

	BOOLEAN      active;
	SOLDIERTYPE* soldier;
	DataReader d{data};
	ExtractObject(d, &gItemPointer);
	EXTR_SOLDIER(d, soldier)
	EXTR_U8(     d, gbItemPointerSrcSlot)
	EXTR_BOOL(   d, active)
	EXTR_SKIP(   d, 5)
	Assert(d.getConsumed() == lengthof(data));

	if (active)
	{
		SetItemPointer(&gItemPointer, soldier);
		ReEvaluateDisabledINVPanelButtons();
	}
	else
	{
		gpItemPointer        = 0;
		gpItemPointerSoldier = 0;
	}
}


void SaveItemCursorToSavedGame(HWFILE const f)
{
	// Sized for InjectObject() (OBJECTTYPE at the current MAX_OBJECTS_PER_SLOT/
	// MAX_ATTACHMENTS) plus SoldierID + slot + active flag + 5 bytes of padding.
	// 92 at OBJECTTYPE == 84; grown by the same +88 InjectObject() itself grew
	// by when MAX_OBJECTS_PER_SLOT went from 8 to 100.
	BYTE  data[180];
	DataWriter d{data};
	InjectObject(d, &gItemPointer);
	INJ_SOLDIER(d, gpItemPointerSoldier)
	INJ_U8(     d, gbItemPointerSrcSlot)
	INJ_BOOL(   d, gpItemPointer != 0)
	INJ_SKIP(   d, 5)
	Assert(d.getConsumed() == lengthof(data));

	f->write(data, sizeof(data));
}


void UpdateItemHatches(void)
{
	SOLDIERTYPE *pSoldier = NULL;

	if (fInMapMode)
	{
		if (fShowInventoryFlag) pSoldier = GetSelectedInfoChar();
	}
	else
	{
		pSoldier = gpSMCurrentMerc;
	}

	if ( pSoldier != NULL )
	{
		ReevaluateItemHatches(pSoldier, FALSE);
	}
}


void SetMouseCursorFromItem(UINT16 const item_idx)
{
	const ItemModel * item = GCM->getItem(item_idx);
	auto graphic = GetSmallInventoryGraphicForItem(item);
	auto vo = graphic.first;
	SetExternMouseCursor(*vo, graphic.second);
	SetCurrentCursorFromDatabase(EXTERN_CURSOR);
}


void SetMouseCursorFromCurrentItem()
{
	SetMouseCursorFromItem(gpItemPointer->usItem);
}


void SetItemPointer(OBJECTTYPE* const o, SOLDIERTYPE* const s)
{
	gpItemPointer        = o;
	gpItemPointerSoldier = s;
}


void LoadInterfaceItemsGraphics()
{
	guiMapInvSecondHandBlockout = AddVideoObjectFromFile(INTERFACEDIR "/map_inv_2nd_gun_cover.sti");
	guiSecItemHiddenVO          = AddVideoObjectFromFile(INTERFACEDIR "/secondary_gun_hidden.sti");
	guiSmallInventoryGraphicMissingSmallPocket = AddVideoObjectFromFile("sti/interface/inventory/inventory-graphic-not-found-small-sp.sti");
	guiSmallInventoryGraphicMissingBigPocket = AddVideoObjectFromFile("sti/interface/inventory/inventory-graphic-not-found-small-bp.sti");

	for (auto const& item : GCM->getAllSmallInventoryGraphicPaths()) {
		auto path = item.to_lower();
		if (allInventoryGraphics.find(path) == allInventoryGraphics.end()) {
			try {
				auto vObject = AddVideoObjectFromFile(item);
				allInventoryGraphics.insert_or_assign(path, vObject);
			} catch (const std::runtime_error &ex) {
				SLOGE("Error loading small inventory graphic `{}`: {}", item, ex.what());
			}
		}
	}

	// Sector-inventory "big images" toggle's own pre-cached set -- see
	// GetSectorInventoryBigGraphicForItem()'s own comment for why this is a
	// separate cache from allInventoryGraphics above.
	for (auto const& item : GCM->getAllBigInventoryGraphicPaths()) {
		auto path = item.to_lower();
		if (allSectorInventoryBigGraphics.find(path) == allSectorInventoryBigGraphics.end()) {
			try {
				auto vObject = AddVideoObjectFromFile(item);
				allSectorInventoryBigGraphics.insert_or_assign(path, vObject);
			} catch (const std::runtime_error &ex) {
				SLOGE("Error loading sector-inventory big graphic `{}`: {}", item, ex.what());
			}
		}
	}

	// Build a sawtooth black-white-black colour gradient
	size_t const length = lengthof(us16BPPItemCyclePlacedItemColors);
	for (INT32 i = 0; i != length / 2; ++i)
	{
		UINT32 const l = 25 * (i + 1);
		UINT16 const c = Get16BPPColor(FROMRGB(l, l, l));
		us16BPPItemCyclePlacedItemColors[i]              = c;
		us16BPPItemCyclePlacedItemColors[length - i - 1] = c;
	}
}


void DeleteInterfaceItemsGraphics()
{
	DeleteVideoObject(guiMapInvSecondHandBlockout);
	DeleteVideoObject(guiSecItemHiddenVO);
	RemoveVObject(guiMapInvSecondHandBlockoutBig);
	DeleteVideoObject(guiSmallInventoryGraphicMissingSmallPocket);
	DeleteVideoObject(guiSmallInventoryGraphicMissingBigPocket);
	for (auto const& v : allInventoryGraphics) {
		DeleteVideoObject(v.second);
	}
	allInventoryGraphics.clear();
	for (auto const& v : allSectorInventoryBigGraphics) {
		DeleteVideoObject(v.second);
	}
	allSectorInventoryBigGraphics.clear();
}
