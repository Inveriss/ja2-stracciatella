#include "Map_Screen_Interface_Map.h"

#include "Assignments.h"
#include "Button_System.h"
#include "Campaign_Types.h"
#include "ContentManager.h"
#include "Debug.h"
#include "Directories.h"
#include "Finances.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "GameInstance.h"
#include "HImage.h"
#include "Input.h"
#include "Interface.h"
#include "Line.h"
#include "Map_Information.h"
#include "Map_Screen_Helicopter.h"
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Border.h"
#include "MapScreen.h"
#include "Merc_Hiring.h"
#include "Message.h"
#include "Militia_Control.h"
#include "Militia_Recruit_Panel.h"
#include "Militia_Deployment_Panel.h"
#include "SAM_Sites.h"
#include "Militia_Control.h"
#include "GameScreen.h"
#include "JAScreens.h"
#include "Map_Screen_Interface_Bottom.h"
#include "WordWrap.h"
#include "Map_Screen_Interface_Map_Inventory.h"
#include "Map_Screen_Interface_Border.h"
#include "MineModel.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "Overhead.h"
#include "Overhead_Types.h"
#include "PreBattle_Interface.h"
#include "Queen_Command.h"
#include "Render_Dirty.h"
#include "SamSiteModel.h"
#include "SGPStrings.h"
#include "Soldier_Control.h"
#include "Squads.h"
#include "StrategicMap_Secrets.h"
#include "StrategicMapSecretModel.h"
#include "Strategic_Mines.h"
#include "Strategic_Movement.h"
#include "Strategic_Pathing.h"
#include "Strategic_Town_Loyalty.h"
#include "StrategicMap.h"
#include "SysUtil.h"
#include "Tactical_Save.h"
#include "Text.h"
#include "Timer_Control.h"
#include "Town_Militia.h"
#include "TownModel.h"
#include "UILayout.h"
#include "Vehicles.h"
#include "Video.h"
#include "VObject.h"
#include "VObject_Blitters.h"
#include "VSurface.h"
#include <stdexcept>
#include <string_theory/format>
#include <string_theory/string>
#include <utility>
#include <vector>
#include <algorithm>

// // Scroll region width
// #define SCROLL_REGION 4

// // The Map/Mouse Scroll defines
// #define EAST_DIR  0
// #define WEST_DIR  1
// #define NORTH_DIR 2
// #define SOUTH_DIR 3
// #define TOP_NORTH  2
// #define TOP_SOUTH  13
// #define RIGHT_WEST 250
// #define RIGHT_EAST 260
// #define LEFT_EAST  640
// #define LEFT_WEST  630
// #define BOTTOM_NORTH 320
// #define BOTTOM_SOUTH 330

// // Map Scroll Defines
// #define SCROLL_EAST  0
// #define SCROLL_WEST  1
// #define SCROLL_NORTH 2
// #define SCROLL_SOUTH 3
// #define SCROLL_DELAY 50
// #define HORT_SCROLL 14
// #define VERT_SCROLL 10

// the pop up for helicopter stuff -- two independent locations (normal and
// "upper", used when a low-on-the-map sector is selected so the popup
// doesn't run off the bottom of the screen), each with its own graphic
// (pos2_first.sti / pos2_second.sti, see guiMapBorderHeliSectorsFirst/
// Second below) and fully independent X/Y.
#define MAP_HELICOPTER_ETA_POPUP_X (MAP_SCREEN_RIGHT_BLOCK_X + 873)
#define MAP_HELICOPTER_ETA_POPUP_Y (MAP_SCREEN_Y + 185)
#define MAP_HELICOPTER_UPPER_ETA_POPUP_X (MAP_SCREEN_RIGHT_BLOCK_X + 873)
#define MAP_HELICOPTER_UPPER_ETA_POPUP_Y (MAP_SCREEN_Y + 359)
#define MAP_HELICOPTER_ETA_POPUP_WIDTH 129
#define MAP_HELICOPTER_ETA_POPUP_HEIGHT 103

// Text layout inside the pos2_first.sti/pos2_second.sti popup (Total
// Distance/Safe/Unsafe/Total Cost/ETA, and the passenger count below them)
// -- one shared (X, starting Y, and a value column width used to
// right-align each numeric value) applied to every line, independent of the
// popup's own X/Y/size above. Separate set for the "upper" location so each
// location can be tuned on its own.
#define MAP_HELICOPTER_ETA_TEXT_X       (MAP_HELICOPTER_ETA_POPUP_X + 14)
#define MAP_HELICOPTER_ETA_TEXT_Y       (MAP_HELICOPTER_ETA_POPUP_Y + 13)
#define MAP_HELICOPTER_ETA_VALUE_MARGIN MAP_HELICOPTER_ETA_POPUP_WIDTH -27

#define MAP_HELICOPTER_UPPER_ETA_TEXT_X       (MAP_HELICOPTER_UPPER_ETA_POPUP_X + 14)
#define MAP_HELICOPTER_UPPER_ETA_TEXT_Y       (MAP_HELICOPTER_UPPER_ETA_POPUP_Y + 13)
#define MAP_HELICOPTER_UPPER_ETA_VALUE_MARGIN MAP_HELICOPTER_ETA_POPUP_WIDTH -27

// X shifted +190 per user request.
#define MAP_LEVEL_STRING_X (MAP_SCREEN_RIGHT_BLOCK_X + 432 + 190)
// Bottom-anchored to the (now recalibrated) map viewport instead of a fixed
// MAP_SCREEN_Y offset, per user request. Was 3px above the old viewport's
// bottom edge (old MAP_VIEW_START_Y + MAP_VIEW_HEIGHT = 10+298 = 308;
// 308-305=3); preserved here as the same 3px margin above the new bottom.
#define MAP_LEVEL_STRING_Y (MAP_VIEW_START_Y + MAP_VIEW_HEIGHT - 3)

// font
// Dedicated, user-authored font for the strategic map screen (town/mine/SAM
// names, grid letters/numbers, town loyalty %, "Sublevel: N", helicopter
// ETA, militia counts) -- was BLOCKFONT2, now its own font/asset so it can
// be sized independently of blockfont2.sti's other (non-map) uses elsewhere
// in the game.
#define MAP_FONT FONTMAP

// index color
#define MAP_INDEX_COLOR 32*4-9

// max number of sectors viewable
#define MAX_VIEW_SECTORS      16


//Map Location index regions

// x start of hort index (numbers 1-16). Shifted +23 X / +7 Y per user
// request.
#define MAP_HORT_INDEX_X (MAP_SCREEN_RIGHT_BLOCK_X + 292 + 23)

// y position of hort index
#define MAP_HORT_INDEX_Y  (MAP_SCREEN_Y + 10 + 7)

// height of hort index
#define MAP_HORT_HEIGHT  GetFontHeight(MAP_FONT)

// vert index start x (letters A-P). Shifted +10 X / +15 Y per user request.
#define MAP_VERT_INDEX_X (MAP_SCREEN_RIGHT_BLOCK_X + 273 + +10)

// vert index start y
#define MAP_VERT_INDEX_Y  (MAP_SCREEN_Y + 31 + 15)

// vert width
#define MAP_VERT_WIDTH   GetFontHeight(MAP_FONT)

// "Boxes" Icons
#define SMALL_YELLOW_BOX      0
#define BIG_YELLOW_BOX        1
#define SMALL_DULL_YELLOW_BOX 2
#define BIG_DULL_YELLOW_BOX   3
#define SMALL_WHITE_BOX       4
#define BIG_WHITE_BOX         5
#define SMALL_RED_BOX         6
#define BIG_RED_BOX           7
#define SMALL_QUESTION_MARK   8
#define BIG_QUESTION_MARK     9


#define MERC_ICONS_PER_LINE 6
#define ROWS_PER_SECTOR     5

#define MAP_X_ICON_OFFSET   2
#define MAP_Y_ICON_OFFSET   1

// Arrow Offsets
#define UP_X                13
#define UP_Y                 7
#define DOWN_X               0
#define DOWN_Y              -2
#define RIGHT_X             -2
#define RIGHT_Y             11
#define LEFT_X               2
#define LEFT_Y               5


// The Path Lines
#define NORTH_LINE           1
#define SOUTH_LINE           0
#define WEST_LINE            3
#define EAST_LINE            2
#define N_TO_E_LINE          4
#define E_TO_S_LINE          5
#define W_TO_N_LINE          6
#define S_TO_W_LINE          7
#define W_TO_S_LINE          8
#define N_TO_W_LINE          9
#define S_TO_E_LINE         10
#define E_TO_N_LINE         11
#define W_TO_E_LINE         12
#define N_TO_S_LINE         13
#define E_TO_W_LINE         14
#define S_TO_N_LINE         15
#define W_TO_E_PART1_LINE   16
#define W_TO_E_PART2_LINE   17
#define E_TO_W_PART1_LINE   18
#define E_TO_W_PART2_LINE   19
#define N_TO_S_PART1_LINE   20
#define N_TO_S_PART2_LINE   21
#define S_TO_N_PART1_LINE   22
#define S_TO_N_PART2_LINE   23
#define GREEN_X_WEST        36
#define GREEN_X_EAST        37
#define GREEN_X_NORTH       38
#define GREEN_X_SOUTH       39
#define RED_X_WEST          40
#define RED_X_EAST          41
#define RED_X_NORTH         42
#define RED_X_SOUTH         43

// The arrows
#define Y_NORTH_ARROW       24
#define Y_SOUTH_ARROW       25
#define Y_EAST_ARROW        26
#define Y_WEST_ARROW        27
#define W_NORTH_ARROW       28
#define W_SOUTH_ARROW       29
#define W_EAST_ARROW        30
#define W_WEST_ARROW        31
#define NORTH_ARROW         32
#define SOUTH_ARROW         33
#define EAST_ARROW          34
#define WEST_ARROW          35

#define ARROW_DELAY         20
#define PAUSE_DELAY       1000


#define CHAR_FONT_COLOR 32*4-9

// Arrow Offsets
#define EAST_OFFSET_X  11
#define EAST_OFFSET_Y  0
#define NORTH_OFFSET_X 9
#define NORTH_OFFSET_Y -9
#define SOUTH_OFFSET_X -9
#define SOUTH_OFFSET_Y 9
#define WEST_OFFSET_X  -11
#define WEST_OFFSET_Y  0
#define WEST_TO_SOUTH_OFFSET_Y  0
#define EAST_TO_NORTH_OFFSET_Y  0
#define RED_WEST_OFF_X  -MAP_GRID_X
#define RED_EAST_OFF_X  MAP_GRID_X
#define RED_NORTH_OFF_Y -21
#define RED_SOUTH_OFF_Y 21

// the font use on the mvt icons for mapscreen
#define MAP_MVT_ICON_FONT SMALLCOMPFONT

// Merc-count text on merc_between_sector_icons.sti/merc_mvt_green_arrows.sti
// (ShowPeopleInMotion()) only -- kept separate from MAP_MVT_ICON_FONT so this
// doesn't also change the helicopter icon's passenger-count text
// (DisplayPositionOfHelicopter()), which still uses MAP_MVT_ICON_FONT.
#define MAP_MVT_TRANSIT_FONT FONTGRID


// map shading colors

enum{
	MAP_SHADE_BLACK =0,
	MAP_SHADE_LT_GREEN,
	MAP_SHADE_DK_GREEN,
	MAP_SHADE_LT_RED,
	MAP_SHADE_DK_RED,
};
// the big map .pcx
static SGPVSurface* guiBIGMAP;


// The militia box (militia.sti 0, the town's map from militiamaps.sti) in one
// of two layouts, chosen by the panel graphic's size -- see MilitiaBox()
#define MAP_MILITIA_MAP_X (MilitiaBox().map_x)
#define MAP_MILITIA_MAP_Y (MilitiaBox().map_y)
#define MAP_MILITIA_LOWER_ROW_Y (MilitiaBox().lower_y)
#define NUMBER_OF_MILITIA_ICONS_PER_LOWER_ROW (MilitiaBox().lower_per_row)
#define MILITIA_BOX_ROWS 3
#define MILITIA_BOX_BOX_HEIGHT (MilitiaBox().cell_h)
#define MILITIA_BOX_BOX_WIDTH (MilitiaBox().cell_w)
// the big box centred on the map's width (its panel 259 px without the
// shadow), then 40 px further right
#define MAP_MILITIA_BOX_POS_X (MilitiaBox().big ? MAP_VIEW_START_X + (MAP_VIEW_WIDTH - MilitiaBox().panel_w) / 2 + 40 : MAP_SCREEN_RIGHT_BLOCK_X + 400)
#define MAP_MILITIA_BOX_POS_Y (MAP_SCREEN_Y + MilitiaBox().box_y)

#define POPUP_MILITIA_ICONS_PER_ROW (MilitiaBox().sector_per_row)
#define MEDIUM_MILITIA_ICON_SPACING (MilitiaBox().icon_pitch)

#define MILITIA_BTN_OFFSET_X 26 // the small layout's rank buttons, in the selected sector
#define MILITIA_BTN_HEIGHT 11
#define MILITIA_BOX_WIDTH (MilitiaBox().title_w)
#define MILITIA_BOX_TEXT_OFFSET_Y 4
#define MILITIA_BOX_TEXT_TITLE_HEIGHT (MilitiaBox().title_h)

#define MAP_MILITIA_BOX_AUTO_BOX_X (MilitiaBox().auto_x)
#define MAP_MILITIA_BOX_AUTO_BOX_Y (MilitiaBox().bottom_y)
#define MAP_MILITIA_BOX_DONE_BOX_X (MilitiaBox().done_x)

#define HELI_ICON               0
#define HELI_SHADOW_ICON        1

#define HELI_ICON_WIDTH         20
#define HELI_ICON_HEIGHT        10
#define HELI_SHADOW_ICON_WIDTH  19
#define HELI_SHADOW_ICON_HEIGHT 11


// the militia box buttons: 0-3 the ranks of the selected sector (MilitiaLevel), then AUTO and DONE
#define MILITIA_AUTO_BUTTON MAX_MILITIA_LEVELS
#define MILITIA_DONE_BUTTON (MAX_MILITIA_LEVELS + 1)
static GUIButtonRef giMapMilitiaButton[MAX_MILITIA_LEVELS + 2];
// the big box's arrows, per rank: up puts one into the sector, down takes one out
static GUIButtonRef g_militia_up_arrow[MAX_MILITIA_LEVELS];
static GUIButtonRef g_militia_down_arrow[MAX_MILITIA_LEVELS];


static INT16 const gsMilitiaSectorButtonColors[] =
{
	FONT_LTGREEN,
	FONT_LTBLUE,
	16,
	FONT_YELLOW
};
// the big militia box's: elite (0, 0, 255), elite heavy (24, 20, 16) -- the
// font palette's nearest, (27, 22, 17)
static INT16 const gsBigMilitiaSectorButtonColors[] =
{
	FONT_LTGREEN,
	FONT_LTBLUE,
	FONT_BLUE,
	192
};

// track number of townspeople picked up
INT16 sGreensOnCursor = 0;
INT16 sRegularsOnCursor = 0;
INT16 sElitesOnCursor = 0;
INT16 sHeaviesOnCursor = 0;

// the current militia town id
INT16 sSelectedMilitiaTown = 0;

namespace {
// boxes for characters on the map
cache_key_t const guiCHARICONS{ INTERFACEDIR "/boxes.sti" };

// the merc arrival sector landing zone icon
cache_key_t const guiBULLSEYE{ INTERFACEDIR "/bullseye.sti" };

// Sublevel (mine) graphics. Not plain cache_key_t constants: which file each
// is depends on the active resolution (see
// UILayout::isCompactStrategicScreen()), which isn't known yet at
// static-initialization time, so the choice has to be resolved at runtime,
// on every call -- see GetCharListGraphicsFilename() in MapScreen.cc for the
// same pattern. Suffix convention: _1280 for the compact strategic-screen
// tier (height 720-767), _1024 for the large tier (height 768+).
//
// Naming note: sectorZ (iCurrentMapSectorZ) is 0 at the surface and 1/2/3 for
// the three mine sublevels, but this screen's own UI labels them "Map Level
// 1" (surface, no mine graphic) through "Map Level 4" -- one higher. The
// filenames follow that UI numbering (mine_2/3/4), not sectorZ, hence the
// mine_1.sti -> mine_2_*.sti-looking offset below.
cache_key_t GetMineLevelGraphicsFilename(int const sectorZ)
{
	bool const compact = g_ui.isCompactStrategicScreen();
	switch (sectorZ)
	{
		case 1: return compact ? INTERFACEDIR "/mine_2_1280.sti" : INTERFACEDIR "/mine_2_1024.sti";
		case 2: return compact ? INTERFACEDIR "/mine_3_1280.sti" : INTERFACEDIR "/mine_3_1024.sti";
		case 3: return compact ? INTERFACEDIR "/mine_4_1280.sti" : INTERFACEDIR "/mine_4_1024.sti";

		default: abort(); // HACK000E
	}
}

// militia graphics
cache_key_t const guiMilitia{ INTERFACEDIR "/militia.sti" };

// the big militia box's panel with the arrows (militia.sti 19), the arrows
#define MILITIA_ARROWS_PANEL 19
// the arrival mode's Send to Deployment Center button (militia.sti 20 normal,
// 21 pressed, 252x30), in the panel's slot over AUTO and DONE
#define MILITIA_DEPLOY_BUTTON_NORMAL  20
#define MILITIA_DEPLOY_BUTTON_PRESSED 21
#define MILITIA_ARROWS_FILE  INTERFACEDIR "/militia_arrows.sti"

// the ELITE_HEAVY militia square in militia.sti (no smaller variants); until the
// graphic has it, the elite square of the same place is shown instead
#define HEAVY_MILITIA_ICON 14
static UINT16 HeavyMilitiaIcon(UINT16 const elite_icon)
{
	return GetVObject(guiMilitia)->SubregionCount() > HEAVY_MILITIA_ICON ? HEAVY_MILITIA_ICON : elite_icon;
}

// The militia box's layout, positions inside its panel (militia.sti 0):
// - small (the original 137x190 panel, 127x109 maps): the rank buttons in the
//   selected sector, 3-4 px icons, FONT10ARIAL;
// - big (a panel 250 px wide or more, 268x370 with 254x218 maps): 84x72
//   sectors, the militia as 11x11 squares (militia.sti 15-18: green, regular,
//   elite, elite heavy), the rank buttons (32x22) in a row under the
//   unassigned militia, the sector outlines drawn, all texts in FONT14ARIAL
//   in the colour of the big weapon box's BALLISTICS / MAINTENANCE headers
//   (the rank buttons' counts in their ranks' colours), the box centred on
//   the map and 40 px right of it;
// - big with arrows (militia.sti 19, 268x416, used when the graphic has it):
//   as big, the rank buttons in a row between an up arrow (into the sector)
//   and a down arrow (out of the sector) each (militia_arrows.sti), the
//   unassigned militia under them, under those their numbers per rank on
//   the rank button's graphic (militia.sti 3); the panel (268x478) has a
//   slot over AUTO and DONE for the arrival mode's Send to Deployment Center
//   button and a text box at its bottom (268x560 in all, at y 44); the
//   "Unassigned" label right of the counters.

// the rank button's graphic in militia.sti
#define MILITIA_RANK_BUTTON_GRAPHIC 3
struct MilitiaBoxLayout
{
	bool   big;
	INT16  panel_w;                              // without its shadow
	INT16  map_x, map_y, cell_w, cell_h;         // the town's map and its sectors
	INT16  title_x, title_w, title_h;            // the title bar (from y MILITIA_BOX_TEXT_OFFSET_Y)
	INT16  unassigned_x, unassigned_y, unassigned_w, unassigned_h; // the "Unassigned" label
	INT16  lower_x, lower_y, lower_per_row;      // the unassigned militia
	INT16  sector_per_row, icon_pitch;           // the militia in a sector
	INT16  rank_btn_x, rank_btn_y, rank_btn_dx;  // big: the rank buttons
	INT16  auto_x, done_x, bottom_y;             // the AUTO and DONE buttons
	SGPFont font;
	UINT8  colour;                               // big: every text's; small: the original colours
	UINT16 panel;                                // the panel's militia.sti frame
	INT16  arrow_up_y, arrow_down_y, arrow_dx;   // the arrows, x from their rank button's (up y 0: none)
	INT16  counter_y;                            // the unassigned militia counters per rank, x as the rank buttons (0: none)
	INT16  deploy_x, deploy_y;                   // the arrival mode's Send to Deployment Center button (y 0: none)
	INT16  box_y;                                // the box's top from the map screen's
};

static MilitiaBoxLayout const& MilitiaBox()
{
	static MilitiaBoxLayout const small_box =
	{
		false, 137, 4, 20, 42, 36, 0, 133, 13, 0, 132, 133, 0, 5, 142, 25, 5, 5, 0, 0, 0, 4, 67, 167, FONT10ARIAL, FONT_WHITE, 0, 0, 0, 0, 0, 0, 0, 125
	};
	static MilitiaBoxLayout const big_box =
	{
		true, 259, 3, 30, 84, 72, 3, 254, 22, 151, 301, 104, 22, 4, 252, 23, 7, 11, 5, 300, 37, 4, 134, 328, FONT14ARIAL, FONT_BEIGE, 0, 0, 0, 0, 0, 0, 0, 125
	};
	static MilitiaBoxLayout const big_arrows_box =
	{
		true, 259, 3, 30, 84, 72, 3, 254, 22, 140, 375, 83, 22, 4, 325, 23, 7, 11, 4, 275, 33, 4, 134, 436, FONT14ARIAL, FONT_BEIGE, MILITIA_ARROWS_PANEL, 252, 298, 5, 374, 4, 401, 44
	};
	SGPVObject const* const vo = GetVObject(guiMilitia);
	if (vo->SubregionCount() > MILITIA_ARROWS_PANEL && vo->SubregionProperties(MILITIA_ARROWS_PANEL).usWidth >= 250) return big_arrows_box;
	return vo->SubregionProperties(0).usWidth >= 250 ? big_box : small_box;
}

// The big layout's square of a militia rank (militia.sti 15-18)
#define BIG_MILITIA_ICON_FIRST 15

cache_key_t const guiMilitiaMaps{ INTERFACEDIR "/militiamaps.sti" };
cache_key_t const guiMilitiaSectorHighLight{ INTERFACEDIR "/militiamapsectoroutline2.sti" };
cache_key_t const guiMilitiaSectorOutline{ INTERFACEDIR "/militiamapsectoroutline.sti" };

// heli pop up -- two independent graphics, one per popup location (normal
// vs "upper"), per user request.
cache_key_t const guiMapBorderHeliSectorsFirst{ INTERFACEDIR "/pos2_first.sti" };
cache_key_t const guiMapBorderHeliSectorsSecond{ INTERFACEDIR "/pos2_second.sti" };

// sam and mine icons
cache_key_t const guiSAMICON{ INTERFACEDIR "/sam.sti" };

// the mine icon
cache_key_t const guiMINEICON{ INTERFACEDIR "/mine.sti" };

// helicopter icon
cache_key_t const guiHelicopterIcon{ INTERFACEDIR "/helicop.sti" };

// Ground-vehicle destination marker -- its own dedicated graphic
// (vehiclecursor_destination.sti), frame 0, per user request. Shown at a
// player ground vehicle's confirmed destination sector, the same way
// HELI_SHADOW_ICON marks the helicopter's -- see ShowVehicleDestinations().
cache_key_t const guiVehicleCursorIcon{ CURSORSDIR "/vehiclecursor_destination.sti" };
#define VEHICLE_DESTINATION_ICON 0

// On-foot merc destination marker -- same idea as guiVehicleCursorIcon
// above, but for walking player groups, per user request. Shown at a
// walking group's confirmed destination sector -- see ShowMercDestinations().
cache_key_t const guiWalkingCursorIcon{ CURSORSDIR "/walkingcursor_destination.sti" };
#define MERC_DESTINATION_ICON 0

// the between sector icons
cache_key_t const guiCHARBETWEENSECTORICONS{ INTERFACEDIR "/merc_between_sector_icons.sti" };
cache_key_t const guiCHARBETWEENSECTORICONSCLOSE{ INTERFACEDIR "/merc_mvt_green_arrows.sti" };

// the map arrows graphics
cache_key_t const guiMAPCURSORS{ INTERFACEDIR "/mapcursr.sti" };
}

// highlighted sector
SGPSector gsHighlightSector(-1, -1);

// the palettes
static UINT16* pMapLTRedPalette;
static UINT16* pMapDKRedPalette;
static UINT16* pMapLTGreenPalette;
static UINT16* pMapDKGreenPalette;


// base sectors (sector value for the upper left corner) of towns. List start at zero, indexed by (townId - 1)
static std::vector<SGPSector> sBaseSectorList;

// The militia box's key (sSelectedMilitiaTown): a town id, or for a SAM site
// outside the towns MILITIA_BOX_SAM_KEY + its sector. A SAM site's box shows
// militiamaps.sti MILITIA_BOX_SAM_MAP (the site in the middle of a 3x3 map);
// only its own sector is used.
#define MILITIA_BOX_SAM_KEY 1000
#define MILITIA_BOX_SAM_MAP 12

static bool IsSamBoxKey(INT16 const key)
{
	return key >= MILITIA_BOX_SAM_KEY;
}

static UINT8 SamBoxSector(INT16 const key)
{
	return UINT8(key - MILITIA_BOX_SAM_KEY);
}

// The sector belongs to the box of this key
static bool MilitiaBoxHasSector(INT16 const key, INT16 const sector)
{
	return IsSamBoxKey(key) ? sector == SamBoxSector(key) : GetTownIdForSector(sector) == key;
}

// f(sector) for each sector of the box of this key
template<typename F> static void ForEachMilitiaBoxSector(INT16 const key, F f)
{
	if (IsSamBoxKey(key))
	{
		f(SamBoxSector(key));
		return;
	}
	FOR_EACH_SECTOR_IN_TOWN(i, key) f(UINT8(i->sector));
}

// position of town names on the map (list by townId, starting at 1)
// these are no longer PIXELS, but 10 * the X,Y position in SECTORS (fractions possible) to the X-CENTER of the town
static std::vector<SGPPoint> pTownPoints;


// map region
SGPRect MapScreenRect;

static SGPRect gOldClipRect;

// temp helicopter path
PathSt* pTempHelicopterPath = NULL;

// character temp path
PathSt* pTempCharacterPath = NULL;

// draw temp path?
BOOLEAN fDrawTempHeliPath = FALSE;

// destination plotting character
INT8 bSelectedDestChar = -1;

// assignment selection character
INT8 bSelectedAssignChar=-1;

// current contract char
INT8 bSelectedContractChar = -1;


// has the temp path for character or helicopter been already drawn?
BOOLEAN   fTempPathAlreadyDrawn = FALSE;

// the regions for the mapscreen militia box
static MOUSE_REGION gMapScreenMilitiaBoxRegions[9];
// the whole militia box under its sectors and buttons: clicks on its
// background don't reach the map below (only DONE closes the box)
static MOUSE_REGION gMapScreenMilitiaBoxBlanket;

// the sector that is highlighted on the militia map
static INT16 sSectorMilitiaMapSector        = -1;
static bool  g_militia_box_done             = false; // the box is being closed with Done

// The arrival mode (docs/militia-arrival-box-plan.md): the box opened for an
// arrived order, whose militia are the unassigned ones. The order is kept in
// step with them (so a save holds them as a waiting order); Send to
// Deployment Center splits them off as a waiting order; closing the box with
// Done finishes the order, closing it otherwise sends them to the Deployment
// Center too.
static UINT32       g_arrival_order    = 0; // 0: the box's normal mode
static UINT32       g_arrival_total    = 0; // the order's militia when the box opened (and joined later)
static UINT32       g_arrival_sent     = 0; // sent to the Deployment Center since
static UINT32       g_arrival_room     = 0; // the free places in the sectors of ours when it opened
static bool         g_arrival_ours     = false; // the box had a sector of ours with no enemy then

// The arrival mode's text box (the panel's bottom box, inside its frame): the
// lines in FONT12ARIAL, beige with a shadow, 12 px apart, the block centred
// in its height
#define MILITIA_TEXT_X      9
#define MILITIA_TEXT_Y      474
#define MILITIA_TEXT_W      240
#define MILITIA_TEXT_H      71
#define MILITIA_TEXT_FONT   FONT12ARIAL
#define MILITIA_TEXT_PITCH  12
static GUIButtonRef g_militia_deploy_button;
// D5.1: the map screen was entered for an arrival on the tactical screen
// (pending until the box opens); the box goes back there when it closes
static bool         g_arrival_jump_pending = false;
// D5.2: the militia put into the battle loaded in tactical, per rank; they
// join it when the box is closed with Done
static UINT8        g_reinforce[MAX_MILITIA_LEVELS] = {};
static bool         g_arrival_back_to_tactical = false;
static bool  fMilitiaMapButtonsCreated      = false;
static INT16 sSectorMilitiaMapSectorOutline = -1;


// have any nodes in the current path list been deleted?
BOOLEAN fDeletedNode = FALSE;

static UINT16 gusUndergroundNearBlack;

BOOLEAN gfMilitiaPopupCreated = FALSE;

UINT32 guiAnimateRouteBaseTime = 0;
UINT32 guiPotHeliPathBaseTime = 0;

// map secret icons
static std::map<ST::string, SGPVObject*> gSecretSiteIcons;

static const SGPSector northOffset(NORTH_OFFSET_X, NORTH_OFFSET_Y);
static const SGPSector eastOffset(EAST_OFFSET_X, EAST_OFFSET_Y);
static const SGPSector southOffset(SOUTH_OFFSET_X, SOUTH_OFFSET_Y);
static const SGPSector westOffset(WEST_OFFSET_X, WEST_OFFSET_Y);

void InitMapScreenInterfaceMap()
{
	sBaseSectorList.clear();
	pTownPoints.clear();
	pTownPoints.push_back(SGPPoint());

	for (auto const& pair : GCM->getTowns())
	{
		auto town = pair.second;
		sBaseSectorList.push_back(town->getBaseSector());
		pTownPoints.push_back(town->townPoint);
	}

	// Bottom edge was "MAP_VIEW_HEIGHT-10 + MAP_GRID_Y" -- the "-10" was
	// canceling out the old MAP_VIEW_HEIGHT's hidden +10px margin (298 =
	// 16*18 + 10), recovering the true 16-row grid height before adding one
	// more row. Now that MAP_VIEW_HEIGHT is exactly
	// MAX_VIEW_SECTORS*MAP_GRID_Y with no hidden margin, there's nothing left
	// to cancel out -- "-1" (matching iTop's own inclusive-pixel adjustment)
	// is the correct, self-scaling equivalent. This rect drives both
	// RestrictMouseCursor() and the clip used when drawing the grey town
	// border lines (ClipBlitsToMapViewRegionForRectangleAndABit) -- with the
	// stale "-10", both were cut short in the last (16th) row at the new,
	// bigger grid size.
	MapScreenRect.set((MAP_VIEW_START_X+MAP_GRID_X - 2), ( MAP_VIEW_START_Y+MAP_GRID_Y - 1),
				MAP_VIEW_START_X + MAP_VIEW_WIDTH - 1 + MAP_GRID_X , MAP_VIEW_START_Y+MAP_VIEW_HEIGHT-1+MAP_GRID_Y);
}

void DrawMapIndexBigMap(BOOLEAN fSelectedCursorIsYellow)
{
	// this procedure will draw the coord indexes on the zoomed out map
	SetFontDestBuffer(FRAME_BUFFER);
	SetFont(MAP_FONT);
	SetFontBackground(FONT_MCOLOR_BLACK);

	bool  const draw_cursors  = CanDrawSectorCursor();
	bool  const sel_candidate = bSelectedDestChar == -1 && !fPlotForHelicopter;
	UINT8 const sel_colour    = fSelectedCursorIsYellow ? FONT_YELLOW : FONT_WHITE;
	for (INT32 i = 1; i <= MAX_VIEW_SECTORS; ++i)
	{
		INT16 usX;
		INT16 usY;

		UINT8 const colour_x =
			!draw_cursors                  ? MAP_INDEX_COLOR :
			i == sSelMap.x && sel_candidate ? sel_colour      :
			i == gsHighlightSector.x       ? FONT_WHITE      :
			MAP_INDEX_COLOR;
		SetFontForeground(colour_x);
		FindFontCenterCoordinates(MAP_HORT_INDEX_X + (i - 1) * MAP_GRID_X, MAP_HORT_INDEX_Y, MAP_GRID_X, MAP_HORT_HEIGHT, pMapHortIndex[i], MAP_FONT, &usX, &usY);
		MPrint(usX, usY, pMapHortIndex[i]);

		UINT8 const colour_y =
			!draw_cursors                  ? MAP_INDEX_COLOR :
			i == sSelMap.y && sel_candidate ? sel_colour      :
			i == gsHighlightSector.y       ? FONT_WHITE      :
			MAP_INDEX_COLOR;
		SetFontForeground(colour_y);
		FindFontCenterCoordinates(MAP_VERT_INDEX_X, MAP_VERT_INDEX_Y + (i - 1) * MAP_GRID_Y, MAP_HORT_HEIGHT, MAP_GRID_Y, pMapVertIndex[i], MAP_FONT, &usX, &usY);
		MPrint(usX, usY, pMapVertIndex[i]);
	}

	InvalidateRegion(MAP_VERT_INDEX_X, MAP_VERT_INDEX_Y, MAP_VERT_INDEX_X + MAP_HORT_HEIGHT,               MAP_VERT_INDEX_Y + MAX_VIEW_SECTORS * MAP_GRID_Y);
	InvalidateRegion(MAP_HORT_INDEX_X, MAP_HORT_INDEX_Y, MAP_HORT_INDEX_X + MAX_VIEW_SECTORS * MAP_GRID_X, MAP_HORT_INDEX_Y + MAP_HORT_HEIGHT);
}

static void HandleShowingOfEnemyForcesInSector(const SGPSector& sSector, UINT8 ubIconPosition);


static void HandleShowingOfEnemiesWithMilitiaOn(void)
{
	SGPSector sSector(1, 1, iCurrentMapSectorZ);
	for (sSector.x = 1; sSector.x < MAP_WORLD_X - 1; ++sSector.x)
	{
		for (sSector.y = 1; sSector.y < MAP_WORLD_Y - 1; ++sSector.y)
		{
			HandleShowingOfEnemyForcesInSector(sSector, CountAllMilitiaInSector(sSector));
		}
	}
}


static void BlitMineGridMarkers(void);
static void BlitMineIcon(const SGPSector& sMap);
static void BlitMineText(UINT8 mine_idx, const SGPSector& sMap);
static void BlitTownGridMarkers(void);
static void DisplayLevelString(void);
static void DrawBullseye(void);
static void DrawSecretSite(const StrategicMapSecretModel*);
static void DrawTownMilitiaForcesOnMap();
static void HandleLowerLevelMapBlit(void);
static void ShadeMapElem(const SGPSector& sMap, INT32 iColor);
static void ShowItemsOnMap(void);
static void ShowSAMSitesOnStrategicMap();
static void ShowTeamAndVehicles();
static void ShowTownText(void);


void DrawMap(void)
{
	if (!iCurrentMapSectorZ)
	{
		BltVideoSurfaceHalf(guiSAVEBUFFER, guiBIGMAP, MAP_VIEW_START_X + 1, MAP_VIEW_START_Y, NULL);

		// shade map sectors (must be done after Tixa/Orta/Mine icons have been blitted, but before icons!)
		SGPSector sSector(1, 1, iCurrentMapSectorZ);
		for (sSector.x = 1; sSector.x < MAP_WORLD_X - 1; ++sSector.x)
		{
			for (sSector.y = 1; sSector.y < MAP_WORLD_Y - 1; ++sSector.y)
			{
				if (!GetSectorFlagStatus(sSector, SF_ALREADY_VISITED))
				{
					INT32 color;
					if (fShowAircraftFlag)
					{
						if (!StrategicMap[sSector.AsStrategicIndex()].fEnemyAirControlled)
						{
							// sector not visited, not air controlled
							color = MAP_SHADE_DK_GREEN;
						}
						else
						{
							// sector not visited, controlled and air not
							color = MAP_SHADE_DK_RED;
						}
					}
					else
					{
						// not visited
						color = MAP_SHADE_BLACK;
					}
					ShadeMapElem(sSector, color);
				}
				else
				{
					if (fShowAircraftFlag)
					{
						INT32 color;
						if (!StrategicMap[sSector.AsStrategicIndex()].fEnemyAirControlled)
						{
							// sector visited and air controlled
							color = MAP_SHADE_LT_GREEN;
						}
						else
						{
							// sector visited but not air controlled
							color = MAP_SHADE_LT_RED;
						}
						ShadeMapElem(sSector, color);
					}
				}
			}
		}

		/* unfortunately, we can't shade these icons as part of shading the map,
		 * because for airspace, the shade function doesn't merely shade the
		 * existing map surface, but instead grabs the original graphics from
		 * bigmap, and changes their palette.  blitting icons prior to shading would
		 * mean they don't show up in airspace view at all. */

		for (auto s : GCM->getMapSecrets())
		{
			if (!s->isSAMSite		 // we have other handling for SAM sites
				&& IsSecretFoundAt(s->sectorID))
			{
				DrawSecretSite(s);
			}
		}

		ShowSAMSitesOnStrategicMap();

		// draw mine icons and descriptive text
		auto && mines{ GCM->getMines() };
		for (UINT32 i = 0; i < mines.size(); ++i)
		{
			SGPSector sMap(mines[i]->entranceSector);
			BlitMineIcon(sMap);
			// show mine name/production text
			if (fShowMineFlag) BlitMineText(i, sMap);
		}

		// draw towns names & loyalty ratings, and grey town limit borders
		if (fShowTownFlag)
		{
			BlitTownGridMarkers();
			ShowTownText();
		}

		if (fShowMilitia) DrawTownMilitiaForcesOnMap();

		if (fShowAircraftFlag && !gfInChangeArrivalSectorMode) DrawBullseye();
	}
	else
	{
		HandleLowerLevelMapBlit();
	}

	/* show mine outlines even when viewing underground sublevels - they indicate
		* where the mine entrances are */
	if (fShowMineFlag) BlitMineGridMarkers();

	if (fShowTeamFlag)
	{
		ShowTeamAndVehicles();
	}
	else if (fShowMilitia)
	{
		HandleShowingOfEnemiesWithMilitiaOn();
	}

	if (fShowItemsFlag) ShowItemsOnMap();

	DisplayLevelString();
}


void GetScreenXYFromMapXY(const SGPSector& sMap, INT16 *psX, INT16 *psY)
{
	*psX = (sMap.x * MAP_GRID_X) + MAP_VIEW_START_X;
	*psY = (sMap.y * MAP_GRID_Y) + MAP_VIEW_START_Y;
}

// display the town names and loyalty on the screen
static void ShowTownText(void)
{
	SetFont(MAP_FONT);
	SetFontBackground(FONT_MCOLOR_BLACK);
	SetFontDestBuffer(guiSAVEBUFFER, MapScreenRect.iLeft + 2, MapScreenRect.iTop, MapScreenRect.iRight, MapScreenRect.iBottom);
	ClipBlitsToMapViewRegion();

	for (INT8 town = FIRST_TOWN; town < NUM_TOWNS; ++town)
	{
		// skip Orta/Tixa until found
		if (!IsTownFound(town)) continue;

		UINT16 x = MAP_VIEW_START_X + MAP_GRID_X;
		UINT16 y = MAP_VIEW_START_Y + MAP_GRID_Y;
		x += pTownPoints[town].iX * MAP_GRID_X / 10;
		y += pTownPoints[town].iY * MAP_GRID_Y / 10 + 1;

		// if within view region...render, else don't
		if (x < MAP_VIEW_START_X || MAP_VIEW_START_X + MAP_VIEW_WIDTH  < x) continue;
		if (y < MAP_VIEW_START_Y || MAP_VIEW_START_Y + MAP_VIEW_HEIGHT < y) continue;

		// don't show loyalty string until loyalty tracking for that town has been started
		if (gTownLoyalty[town].fStarted && gfTownUsesLoyalty[town])
		{
			// if loyalty is too low to train militia, and militia training is allowed here
			UINT8 const colour =
				gTownLoyalty[town].ubRating < MIN_RATING_TO_TRAIN_TOWN &&
				MilitiaTrainingAllowedInTown(town) ?
					FONT_MCOLOR_RED : FONT_MCOLOR_LTGREEN;
			SetFontForeground(colour);

			ST::string loyalty_str = st_format_printf(gsLoyalString, gTownLoyalty[town].ubRating);

			INT16 loyalty_x = x - StringPixLength(loyalty_str, MAP_FONT) / 2;

			// make sure we don't go past left edge (Grumm)
			INT16 const min_x = MAP_VIEW_START_X + 23;
			if (loyalty_x < min_x) loyalty_x = min_x;

			GDirtyPrint(loyalty_x, y + GetFontHeight(MAP_FONT), loyalty_str);
		}
		else
		{
			SetFontForeground(FONT_MCOLOR_LTGREEN);
		}

		ST::string name = GCM->getTownName(town);
		INT16          const name_x = x - StringPixLength(name, MAP_FONT) / 2;
		GDirtyPrint(name_x, y, name);
	}

	RestoreClipRegionToFullScreen();
}


static void DrawMapBoxIcon(cache_key_t, UINT16 icon, const SGPSector& sMap, UINT8 icon_pos);


// "on duty" includes mercs inside vehicles
static INT32 ShowOnDutyTeam(const SGPSector& sSector)
{
	UINT8 icon_pos = 0;
	CFOR_EACH_IN_CHAR_LIST(c)
	{
		SOLDIERTYPE const& s = *c->merc;
		if (s.uiStatusFlags & SOLDIER_VEHICLE)                    continue;
		if (s.sSector != sSector)                                 continue;
		if (s.bLife    <= 0)                                      continue;
		if (s.bAssignment >= ON_DUTY && s.bAssignment != VEHICLE) continue;
		if (InHelicopter(s))                                      continue;
		if (PlayerIDGroupInMotion(s.ubGroupID))                   continue;
		DrawMapBoxIcon(guiCHARICONS, SMALL_YELLOW_BOX, sSector, icon_pos++);
	}
	return icon_pos;
}


static INT32 ShowAssignedTeam(const SGPSector& sSector, INT32 icon_pos)
{
	CFOR_EACH_IN_CHAR_LIST(c)
	{
		SOLDIERTYPE const& s = *c->merc;
		// given number of on duty members, find number of assigned chars
		// start at beginning of list, look for people who are in sector and assigned
		if (s.uiStatusFlags & SOLDIER_VEHICLE)  continue;
		if (s.sSector != sSector)               continue;
		if (s.bAssignment <  ON_DUTY)           continue;
		if (s.bAssignment == VEHICLE)           continue;
		if (s.bAssignment == IN_TRANSIT)        continue;
		if (s.bAssignment == ASSIGNMENT_POW)    continue;
		if (s.bLife       <= 0)                 continue;
		if (PlayerIDGroupInMotion(s.ubGroupID)) continue;

		DrawMapBoxIcon(guiCHARICONS, SMALL_DULL_YELLOW_BOX, sSector, icon_pos++);
	}
	return icon_pos;
}


static INT32 ShowVehicles(const SGPSector& sSector, INT32 icon_pos)
{
	CFOR_EACH_VEHICLE(v)
	{
		// skip the chopper, it has its own icon and displays in airspace mode
		if (IsHelicopter(v))                          continue;
		if (v.sSector != sSector)                     continue;
		if (PlayerIDGroupInMotion(v.ubMovementGroup)) continue;

		SOLDIERTYPE const& vs = GetSoldierStructureForVehicle(v);
		if (vs.bTeam != OUR_TEAM) continue;

		DrawMapBoxIcon(guiCHARICONS, SMALL_WHITE_BOX, sSector, icon_pos++);
	}
	return icon_pos;
}


// Same idea as DisplayDestinationOfHelicopter() (which handles the one,
// singleton helicopter) but for ground vehicles, of which there can be
// several moving at once -- draws a persistent marker at each player ground
// vehicle's confirmed destination sector, which naturally disappears once
// the vehicle arrives (its path then has length <= 1). Called once (not
// per-sector) from ShowTeamAndVehicles(), redrawn into guiSAVEBUFFER
// whenever the map panel is dirty (same as the other per-sector icons in
// this file), so unlike the helicopter's version this needs no manual
// old-position bookkeeping even with multiple vehicles moving at once.
static void ShowVehicleDestinations(void)
{
	if (iCurrentMapSectorZ != 0) return; // ground vehicles only travel on the surface

	CFOR_EACH_VEHICLE(v)
	{
		if (IsHelicopter(v)) continue; // has its own destination marker, see DisplayDestinationOfHelicopter()

		SOLDIERTYPE const& vs = GetSoldierStructureForVehicle(v);
		if (vs.bTeam != OUR_TEAM) continue;

		if (GetLengthOfPath(v.pMercPath) <= 1) continue; // not going anywhere

		INT16 sLastSectorId = v.sSector.AsStrategicIndex();
		for (PathSt const* pNode = v.pMercPath; pNode != NULL; pNode = pNode->pNext)
		{
			sLastSectorId = (INT16)pNode->uiSectorId;
		}
		SGPSector const sDest = SGPSector::FromStrategicIndex(sLastSectorId);

		// same +1/+3 offset as DisplayDestinationOfHelicopter() uses for its
		// own marker, for visual consistency
		INT16 const x = MAP_VIEW_START_X + sDest.x * MAP_GRID_X + 1;
		INT16 const y = MAP_VIEW_START_Y + sDest.y * MAP_GRID_Y + 3;
		BltVideoObject(guiSAVEBUFFER, guiVehicleCursorIcon, VEHICLE_DESTINATION_ICON, x, y);
		InvalidateRegion(x, y, x + DMAP_GRID_X, y + DMAP_GRID_Y);
	}
}


// Same idea as ShowVehicleDestinations() above, but for player groups
// travelling on foot -- draws a persistent marker at each walking group's
// confirmed destination sector, which naturally disappears once the group
// arrives (its path then has length <= 1). Iterates groups rather than
// individual soldiers so a whole squad travelling together only gets one
// marker, using GetGroupMercPathPtr() to get at the right merc's pMercPath
// regardless of which soldier in the group actually holds it. Vehicle
// groups are skipped -- they already have their own marker above.
static void ShowMercDestinations(void)
{
	if (iCurrentMapSectorZ != 0) return; // mercs only travel on the surface

	CFOR_EACH_PLAYER_GROUP(g)
	{
		if (g->fVehicle) continue; // has its own destination marker, see ShowVehicleDestinations()

		PathSt* const pMercPath = GetGroupMercPathPtr(*g);
		if (GetLengthOfPath(pMercPath) <= 1) continue; // not going anywhere

		INT16 sLastSectorId = g->ubSector.AsStrategicIndex();
		for (PathSt const* pNode = pMercPath; pNode != NULL; pNode = pNode->pNext)
		{
			sLastSectorId = (INT16)pNode->uiSectorId;
		}
		SGPSector const sDest = SGPSector::FromStrategicIndex(sLastSectorId);

		// same +1/+3 offset as DisplayDestinationOfHelicopter()/
		// ShowVehicleDestinations() use for their own markers, for visual consistency
		INT16 const x = MAP_VIEW_START_X + sDest.x * MAP_GRID_X + 1;
		INT16 const y = MAP_VIEW_START_Y + sDest.y * MAP_GRID_Y + 3;
		BltVideoObject(guiSAVEBUFFER, guiWalkingCursorIcon, MERC_DESTINATION_ICON, x, y);
		InvalidateRegion(x, y, x + DMAP_GRID_X, y + DMAP_GRID_Y);
	}
}


static void ShowEnemiesInSector(const SGPSector& sMap, INT16 n_enemies, UINT8 icon_pos)
{
	while (n_enemies-- != 0)
	{
		DrawMapBoxIcon(guiCHARICONS, SMALL_RED_BOX, sMap, icon_pos++);
	}
}


static void ShowUncertainNumberEnemiesInSector(INT16 const sec_x, INT16 const sec_y)
{
	INT16 const x = MAP_VIEW_START_X + sec_x * MAP_GRID_X + MAP_X_ICON_OFFSET;
	INT16 const y = MAP_VIEW_START_Y + sec_y * MAP_GRID_Y - 1;
	BltVideoObject(guiSAVEBUFFER, guiCHARICONS, SMALL_QUESTION_MARK, x, y);
	InvalidateRegion(x, y, x + DMAP_GRID_X, y + DMAP_GRID_Y);
}


static void ShowPeopleInMotion(const SGPSector& sSector);


static void ShowTeamAndVehicles()
{
	// Go through each sector, display the on duty, assigned, and vehicles
	INT32              icon_pos = 0;
	GROUP const* const g        = gfDisplayPotentialRetreatPaths ? gpBattleGroup : 0;
	SGPSector sector(1, 1, iCurrentMapSectorZ);
	for (sector.x = 1; sector.x != MAP_WORLD_X - 1; ++sector.x)
	{
		for (sector.y = 1; sector.y != MAP_WORLD_Y - 1; ++sector.y)
		{
			/* Don't show mercs/vehicles currently in this sector if player is
				* contemplating retreating from THIS sector */
			if (!g || sector != g->ubSector)
			{
				icon_pos = ShowOnDutyTeam(sector);
				icon_pos = ShowAssignedTeam(sector, icon_pos);
				icon_pos = ShowVehicles(sector, icon_pos);
			}

			HandleShowingOfEnemyForcesInSector(sector, icon_pos);
			ShowPeopleInMotion(sector);
		}
	}

	ShowVehicleDestinations();
	ShowMercDestinations();
}


static void ShadeMapElem(const SGPSector& sMap, const INT32 iColor)
{
	INT16 sScreenX;
	INT16 sScreenY;
	GetScreenXYFromMapXY(sMap, &sScreenX, &sScreenY);

	// compensate for original BIG_MAP blit being done at MAP_VIEW_START_X + 1
	sScreenX += 1;

	SGPBox const clip =
	{
		(UINT16)(2 * (sScreenX - (MAP_VIEW_START_X + 1))),
		(UINT16)(2 * (sScreenY - MAP_VIEW_START_Y)),
		2 * MAP_GRID_X,
		2 * MAP_GRID_Y
	};

	// non-airspace
	if (iColor == MAP_SHADE_BLACK) sScreenY -= 1;

	UINT16* pal;
	switch (iColor)
	{
	case MAP_SHADE_BLACK:
		// simply shade darker
		guiSAVEBUFFER->ShadowRect(sScreenX, sScreenY, sScreenX + MAP_GRID_X - 1, sScreenY + MAP_GRID_Y - 1);
		return;

	case MAP_SHADE_LT_GREEN: pal = pMapLTGreenPalette; break;
	case MAP_SHADE_DK_GREEN: pal = pMapDKGreenPalette; break;
	case MAP_SHADE_LT_RED:   pal = pMapLTRedPalette;   break;
	case MAP_SHADE_DK_RED:   pal = pMapDKRedPalette;   break;

	default: return;
	}

	// get original video surface palette
	SGPVSurface* const map = guiBIGMAP;

	UINT16* const org_pal = map->p16BPPPalette;
	map->p16BPPPalette = pal;
	BltVideoSurfaceHalf(guiSAVEBUFFER, guiBIGMAP, sScreenX, sScreenY, &clip);
	map->p16BPPPalette = org_pal;
}


static void InitializePalettesForMap(SGPPaletteEntry const * const pal)
{
	if (pMapDKGreenPalette) return;

	pMapLTRedPalette   = Create16BPPPaletteShaded(pal, 400,   0, 0, TRUE);
	pMapDKRedPalette   = Create16BPPPaletteShaded(pal, 200,   0, 0, TRUE);
	pMapLTGreenPalette = Create16BPPPaletteShaded(pal,   0, 400, 0, TRUE);
	pMapDKGreenPalette = Create16BPPPaletteShaded(pal,   0, 200, 0, TRUE);
}


static void ShutDownPalettesForMap()
{
	delete[] pMapLTRedPalette;
	delete[] pMapDKRedPalette;
	delete[] pMapLTGreenPalette;
	delete[] pMapDKGreenPalette;

	pMapLTRedPalette   = NULL;
	pMapDKRedPalette   = NULL;
	pMapLTGreenPalette = NULL;
	pMapDKGreenPalette = NULL;
}


static void CopyPathToCharactersSquadIfInOne(SOLDIERTYPE* pCharacter);


void PlotPathForCharacter(SOLDIERTYPE& s, const SGPSector& sector, bool const tactical_traversal)
{
	// Don't build path, if cursor isn't allowed here
	if (!IsTheCursorAllowedToHighLightThisSector(sector)) return;
	// Leave if the character is in transit
	if (s.bAssignment == IN_TRANSIT) return;

	if (s.sSector.z != 0)
	{ /* Not on the surface, character won't move until they reach surface, inform
		* player of this fact */
		ST::string who =
			s.bAssignment >= ON_DUTY ? s.name :
			pLongAssignmentStrings[s.bAssignment];
		MapScreenMessage(FONT_MCOLOR_DKRED, MSG_INTERFACE, ST::format("{} {}", who, gsUndergroundString));
		return;
	}

	bool const vehicle = s.bAssignment == VEHICLE || s.uiStatusFlags & SOLDIER_VEHICLE;
	if (vehicle) SetUpMvtGroupForVehicle(&s);

	/* Plot a path from current position to x, y: Get last sector in characters
		* list, build new path, remove tail section, and append onto old list */
	INT16   const start = GetLastSectorIdInCharactersPath(&s);
	INT16   const end   = sector.AsStrategicIndex();
	PathSt* const path  = BuildAStrategicPath(start, end, *GetSoldierGroup(s), tactical_traversal);
	s.pMercPath = AppendStrategicPath(path, s.pMercPath);

	if (vehicle)
	{
		MoveCharactersPathToVehicle(&s);
	}
	else
	{
		CopyPathToCharactersSquadIfInOne(&s);
	}
}


void PlotATemporaryPathForCharacter(const SOLDIERTYPE* const pCharacter, const SGPSector& sector)
{
	// clear old temp path
	pTempCharacterPath = ClearStrategicPathList( pTempCharacterPath, -1 );

	// is cursor allowed here?..if not..don't build temp path
	if (!IsTheCursorAllowedToHighLightThisSector(sector))
	{
		return;
	}

	pTempCharacterPath = BuildAStrategicPath(GetLastSectorIdInCharactersPath(pCharacter), sector.AsStrategicIndex(), *GetSoldierGroup(*pCharacter), FALSE);
}



// clear out character path list, after and including this sector
UINT32 ClearPathAfterThisSectorForCharacter(SOLDIERTYPE *pCharacter, const SGPSector& sMap)
{
	INT32 iOrigLength = 0;
	VEHICLETYPE *pVehicle = NULL;


	iOrigLength = GetLengthOfMercPath( pCharacter );

	if( !iOrigLength )
	{
		// no previous path, nothing to do
		return( ABORT_PLOTTING );
	}


	// if we're clearing everything beyond the current sector, that's quite different.  Since we're basically cancelling
	// his movement completely, we must also make sure his next X,Y are changed and he officially "returns" to his sector
	if (sMap == pCharacter->sSector)
	{
		// if we're in confirm map move mode, cancel that (before new UI messages are issued)
		EndConfirmMapMoveMode( );

		CancelPathsOfAllSelectedCharacters();
		return( PATH_CLEARED );
	}
	else	// click not in the current sector
	{
		// if the clicked sector is along current route, this will repath only as far as it.  If not, the entire path will
		// be canceled.

		// if a vehicle
		if( pCharacter->uiStatusFlags & SOLDIER_VEHICLE )
		{
			pVehicle = &( pVehicleList[ pCharacter->bVehicleID ] );
		}
		// or in a vehicle
		else if( pCharacter->bAssignment == VEHICLE )
		{
			pVehicle = &( pVehicleList[ pCharacter->iVehicleId ] );
		}
		else
		{
			// foot soldier
			pCharacter->pMercPath = ClearStrategicPathListAfterThisSector(pCharacter->pMercPath, sMap, pCharacter->ubGroupID);
		}

		// if there's an associated vehicle structure
		if ( pVehicle != NULL )
		{
			// do it for the vehicle, too
			pVehicle->pMercPath = ClearStrategicPathListAfterThisSector(pVehicle->pMercPath, sMap, pVehicle->ubMovementGroup);
		}

		if( GetLengthOfMercPath( pCharacter ) < iOrigLength )
		{
			CopyPathToAllSelectedCharacters( pCharacter->pMercPath );
			// path WAS actually shortened
			return( PATH_SHORTENED );
		}
		else
		{
			// same path as before - it's not any shorter
			return ( ABORT_PLOTTING );
		}
	}
}


void CancelPathForCharacter( SOLDIERTYPE *pCharacter )
{
	// clear out character's entire path list, he and his squad will stay/return to his current sector.
	pCharacter->pMercPath = ClearStrategicPathList( pCharacter->pMercPath, pCharacter->ubGroupID );
	// NOTE: This automatically calls RemoveGroupWaypoints() internally for valid movement groups

	// This causes the group to effectively reverse directions (even if they've never actually left), so handle that.
	// They are going to return to their current X,Y sector.
	RebuildWayPointsForGroupPath(pCharacter->pMercPath, *GetGroup(pCharacter->ubGroupID));
//	GroupReversingDirectionsBetweenSectors(GetGroup(pCharacter->ubGroupID), pCharacter->sSector, FALSE);


	// if he's in a vehicle, clear out the vehicle, too
	if( pCharacter->bAssignment == VEHICLE )
	{
		CancelPathForVehicle(pVehicleList[pCharacter->iVehicleId], TRUE);
	}
	else
	{
		// display "travel route canceled" message
		BeginMapUIMessage(0, pMapPlotStrings[3]);
	}


	CopyPathToCharactersSquadIfInOne( pCharacter );

	fMapPanelDirty = TRUE;
	fTeamPanelDirty = TRUE;
	fCharacterInfoPanelDirty = TRUE;		// to update ETA
}


void CancelPathForVehicle(VEHICLETYPE& v, BOOLEAN const fAlreadyReversed)
{
	// we're clearing everything beyond the *current* sector, that's quite different.  Since we're basically cancelling
	// his movement completely, we must also make sure his next X,Y are changed and he officially "returns" to his sector
	v.pMercPath = ClearStrategicPathList(v.pMercPath, v.ubMovementGroup);
	// NOTE: This automatically calls RemoveGroupWaypoints() internally for valid movement groups

	// if we already reversed one of the passengers, flag will be TRUE,
	// don't do it again or we're headed back where we came from!
	if ( !fAlreadyReversed )
	{
		// This causes the group to effectively reverse directions (even if they've never actually left), so handle that.
		// They are going to return to their current X,Y sector.
		RebuildWayPointsForGroupPath(v.pMercPath, *GetGroup(v.ubMovementGroup));
	}

	// display "travel route canceled" message
	BeginMapUIMessage(0, pMapPlotStrings[3]);

	// turn the helicopter flag off here, this prevents the "route aborted" msg from coming up
	fPlotForHelicopter = FALSE;

	fTeamPanelDirty = TRUE;
	fMapPanelDirty = TRUE;
	fCharacterInfoPanelDirty = TRUE;		// to update ETA
}


static void CopyPathToCharactersSquadIfInOne(SOLDIERTYPE* const s)
{
	// check if on a squad, if so, do same thing for all characters

	// check to see if character is on a squad, if so, copy path to squad
	if (s->bAssignment < ON_DUTY)
	{
		CopyPathOfCharacterToSquad(s, s->bAssignment);
	}
}


static void AnimateRoute(PathSt* pPath);
static void TracePathRoute(PathSt*);


void DisplaySoldierPath( SOLDIERTYPE *pCharacter )
{
	PathSt* const pPath = GetSoldierMercPathPtr(pCharacter);
	// trace real route
	TracePathRoute(pPath);
	AnimateRoute( pPath );
}


void DisplaySoldierTempPath(void)
{
	// now render temp route
	TracePathRoute(pTempCharacterPath);
}



void DisplayHelicopterPath( void )
{
	// clip to map
	ClipBlitsToMapViewRegion( );

	VEHICLETYPE const& v = GetHelicopter();
	// trace both lists..temp is conditional if cursor has sat in same sector grid long enough
	TracePathRoute(v.pMercPath);
	AnimateRoute(v.pMercPath);

	// restore
	RestoreClipRegionToFullScreen( );
}


void DisplayHelicopterTempPath( void )
{
	//should we draw temp path?
	if (fDrawTempHeliPath) TracePathRoute(pTempHelicopterPath);
}


void PlotPathForHelicopter(const SGPSector& sector)
{
	// will plot the path for the helicopter

	// no heli...go back
	if (!fShowAircraftFlag || iHelicopterVehicleId == -1) return;

	// is cursor allowed here?..if not..don't build path
	if (!IsTheCursorAllowedToHighLightThisSector(sector)) return;

	// set up mvt group for helicopter
	SetUpHelicopterForMovement();

	VEHICLETYPE& v = GetHelicopter();
	// will plot a path from current position to sX, sY
	// get last sector in helicopters list, build new path, remove tail section, move to beginning of list, and append onto old list
	v.pMercPath = AppendStrategicPath(BuildAStrategicPath(GetLastSectorOfHelicoptersPath(), sector.AsStrategicIndex(), *GetGroup(v.ubMovementGroup), FALSE), v.pMercPath);

	fMapPanelDirty = TRUE;
}


void PlotATemporaryPathForHelicopter(const SGPSector& sector)
{
	// clear old temp path
	pTempHelicopterPath = ClearStrategicPathList(pTempHelicopterPath, 0);

	// is cursor allowed here?..if not..don't build temp path
	if (!IsTheCursorAllowedToHighLightThisSector(sector))
	{
		return;
	}

	// build path
	pTempHelicopterPath = BuildAStrategicPath(GetLastSectorOfHelicoptersPath(), sector.AsStrategicIndex(), *GetGroup(GetHelicopter().ubMovementGroup), FALSE);
}


// clear out helicopter path list, after and including this sector
UINT32 ClearPathAfterThisSectorForHelicopter(const SGPSector& sMap)
{
	INT32 iOrigLength = 0;

	// clear out helicopter path list, after and including this sector
	if( iHelicopterVehicleId == -1 ||  !CanHelicopterFly() )
	{
		// abort plotting, shouldn't even be here
		return( ABORT_PLOTTING );
	}

	VEHICLETYPE& v = GetHelicopter();

	iOrigLength = GetLengthOfPath(v.pMercPath);
	if( !iOrigLength )
	{
		// no previous path, nothing to do, and we didn't shorten it
		return( ABORT_PLOTTING );
	}


	// are we clearing everything beyond the helicopter's CURRENT sector?
	if (sMap == v.sSector)
	{
		// if we're in confirm map move mode, cancel that (before new UI messages are issued)
		EndConfirmMapMoveMode( );

		CancelPathForVehicle(v, FALSE);
		return( PATH_CLEARED );
	}
	else	// click not in the current sector
	{
		// if the clicked sector is along current route, this will repath only as far as it.  If not, the entire path will
		// be canceled.
		v.pMercPath = ClearStrategicPathListAfterThisSector(v.pMercPath, sMap, v.ubMovementGroup);

		if (GetLengthOfPath(v.pMercPath) < iOrigLength)
		{
			// really shortened!
			return( PATH_SHORTENED );
		}
		else
		{
			// same path as before - it's not any shorter
			return ( ABORT_PLOTTING );
		}
	}
}



INT16 GetLastSectorOfHelicoptersPath( void )
{
	VEHICLETYPE const& v = GetHelicopter();
	// will return the last sector of the helicopter's current path
	INT16 sLastSector = v.sSector.AsStrategicIndex();
	PathSt* pNode = v.pMercPath;

	while( pNode )
	{
		sLastSector = ( INT16 ) ( pNode->uiSectorId );
		pNode = pNode->pNext;
	}

	return sLastSector;
}


// trace a route for a passed path...doesn't require dest char - most more general
static void TracePathRoute(PathSt* const pPath)
{
	if (pPath == NULL) return;
	Assert(pPath->pPrev == NULL);

	INT32 iDirection = -1;
	INT32 iArrow     = -1;
	const PathSt* prev = NULL;
	const PathSt* next;
	for (const PathSt* node = pPath; node != NULL; prev = node, node = next)
	{
		next = node->pNext;

		BOOLEAN fUTurnFlag = FALSE;

		SGPSector iSector;
		iSector = SGPSector::FromStrategicIndex(node->uiSectorId);
		iSector.x = iSector.x * MAP_GRID_X + MAP_VIEW_START_X;
		iSector.y = iSector.y * MAP_GRID_Y + MAP_VIEW_START_Y;
		SGPSector iArrowSector = iSector;

		if (prev && next)
		{
			const INT32 iDeltaA = (INT16)node->uiSectorId - (INT16)prev->uiSectorId;
			const INT32 iDeltaB = (INT16)node->uiSectorId - (INT16)next->uiSectorId;
			if (iDeltaA == 0) return;

			if (prev->pPrev && next->pNext)
			{
				// check to see if out-of sector U-turn
				// for placement of arrows
				const INT32 iDeltaB1 = next->uiSectorId - next->pNext->uiSectorId;
				fUTurnFlag =
					(iDeltaB1 == -WORLD_MAP_X && iDeltaA == -WORLD_MAP_X && iDeltaB == -1)           ||
					(iDeltaB1 == -WORLD_MAP_X && iDeltaA == -WORLD_MAP_X && iDeltaB ==  1)           ||
					(iDeltaB1 ==  WORLD_MAP_X && iDeltaA ==  WORLD_MAP_X && iDeltaB == -1)           ||
					(iDeltaB1 ==  WORLD_MAP_X && iDeltaA ==  WORLD_MAP_X && iDeltaB ==  1)           ||
					(iDeltaB1 == -1           && iDeltaA == -1           && iDeltaB == -WORLD_MAP_X) ||
					(iDeltaB1 == -1           && iDeltaA == -1           && iDeltaB ==  WORLD_MAP_X) ||
					(iDeltaB1 ==  1           && iDeltaA ==  1           && iDeltaB == -WORLD_MAP_X) ||
					(iDeltaB1 ==  1           && iDeltaA ==  1           && iDeltaB ==  WORLD_MAP_X);
			}

			if (prev->uiSectorId == next->uiSectorId)
			{
				if (prev->uiSectorId + WORLD_MAP_X == node->uiSectorId)
				{
					iDirection = S_TO_N_LINE;
					iArrow = NORTH_ARROW;
					iArrowSector += northOffset;
				}
				else if (prev->uiSectorId - WORLD_MAP_X == node->uiSectorId)
				{
					iDirection = N_TO_S_LINE;
					iArrow = SOUTH_ARROW;
					iArrowSector += southOffset;
				}
				else if (prev->uiSectorId + 1 == node->uiSectorId)
				{
					iDirection = E_TO_W_LINE;
					iArrow = WEST_ARROW;
					iArrowSector += westOffset;
				}
				else
				{
					iDirection = W_TO_E_LINE;
					iArrow = EAST_ARROW;
					iArrowSector += eastOffset;
				}
			}
			else
			{
				if (iDeltaA == -1 && iDeltaB == 1)
				{
					iDirection = WEST_LINE;
					iArrow = WEST_ARROW;
					iArrowSector += westOffset;
				}
				else if (iDeltaA == 1 && iDeltaB == -1)
				{
					iDirection = EAST_LINE;
					iArrow = EAST_ARROW;
					iArrowSector += eastOffset;
				}
				else if (iDeltaA == -WORLD_MAP_X && iDeltaB == WORLD_MAP_X)
				{
					iDirection = NORTH_LINE;
					iArrow = NORTH_ARROW;
					iArrowSector += northOffset;
				}
				else if (iDeltaA == WORLD_MAP_X && iDeltaB == -WORLD_MAP_X)
				{
					iDirection = SOUTH_LINE;
					iArrow = SOUTH_ARROW;
					iArrowSector += southOffset;
				}
				else if (iDeltaA == -WORLD_MAP_X && iDeltaB == -1)
				{
					iDirection = N_TO_E_LINE;
					iArrow = EAST_ARROW;
					iArrowSector += eastOffset;
				}
				else if (iDeltaA == WORLD_MAP_X && iDeltaB == 1)
				{
					iDirection = S_TO_W_LINE;
					iArrow = WEST_ARROW;
					iArrowSector += westOffset;
				}
				else if (iDeltaA == 1 && iDeltaB == -WORLD_MAP_X)
				{
					iDirection = E_TO_S_LINE;
					iArrow = SOUTH_ARROW;
					iArrowSector += southOffset;
				}
				else if (iDeltaA == -1 && iDeltaB == WORLD_MAP_X)
				{
					iDirection = W_TO_N_LINE;
					iArrow = NORTH_ARROW;
					iArrowSector += northOffset;
				}
				else if (iDeltaA == -1 && iDeltaB == -WORLD_MAP_X)
				{
					iDirection = W_TO_S_LINE;
					iArrow = SOUTH_ARROW;
					iArrowSector += southOffset;
					iArrowSector.y += WEST_TO_SOUTH_OFFSET_Y;
				}
				else if (iDeltaA == -WORLD_MAP_X && iDeltaB == 1)
				{
					iDirection = N_TO_W_LINE;
					iArrow = WEST_ARROW;
					iArrowSector += westOffset;
				}
				else if (iDeltaA == WORLD_MAP_X && iDeltaB == -1)
				{
					iDirection  = S_TO_E_LINE;
					iArrow      = EAST_ARROW;
					iArrowSector += eastOffset;
				}
				else if (iDeltaA == 1 && iDeltaB == WORLD_MAP_X)
				{
					iDirection  = E_TO_N_LINE;
					iArrow      = NORTH_ARROW;
					iArrowSector += northOffset;
					iArrowSector.y += EAST_TO_NORTH_OFFSET_Y;
				}
			}
		}
		else
		{
			// display enter and exit 'X's
			if (prev)
			{
				fUTurnFlag = TRUE;
				const INT32 iDeltaA = (INT16)node->uiSectorId - (INT16)prev->uiSectorId;
				if (iDeltaA == -1)
				{
					iDirection = RED_X_WEST;
				}
				else if (iDeltaA == 1)
				{
					iDirection = RED_X_EAST;
				}
				else if (iDeltaA == -WORLD_MAP_X)
				{
					iDirection = RED_X_NORTH;
				}
				else
				{
					iDirection = RED_X_SOUTH;
				}
			}
			if (next)
			{
				fUTurnFlag = FALSE;
				const INT32 iDeltaB = (INT16)node->uiSectorId - (INT16)next->uiSectorId;
				if (iDeltaB == -1)
				{
					iDirection  = GREEN_X_EAST;
					iArrow      = EAST_ARROW;
					iArrowSector += eastOffset;
				}
				else if (iDeltaB == 1)
				{
					iDirection  = GREEN_X_WEST;
					iArrow      = WEST_ARROW;
					iArrowSector += westOffset;
				}
				else if (iDeltaB == WORLD_MAP_X)
				{
					iDirection  = GREEN_X_NORTH;
					iArrow      = NORTH_ARROW;
					iArrowSector += northOffset;
				}
				else
				{
					iDirection  = GREEN_X_SOUTH;
					iArrow      = SOUTH_ARROW;
					iArrowSector += southOffset;
				}
			}
		}

		if (iDirection == -1) continue;

		if (MAP_VIEW_START_X < iSector.x && iSector.x < SCREEN_WIDTH     - MAP_GRID_X * 2 &&
			MAP_VIEW_START_Y < iSector.y && iSector.y < MAP_VIEW_START_Y + MAP_VIEW_HEIGHT
			)
		{
			BltVideoObject(FRAME_BUFFER, guiMAPCURSORS, iDirection, iSector.x, iSector.y);

			if (!fUTurnFlag)
			{
				BltVideoObject(FRAME_BUFFER, guiMAPCURSORS, (UINT16)iArrow, iArrowSector.x, iArrowSector.y);
				InvalidateRegion(iArrowSector.x, iArrowSector.y, iArrowSector.x + 2 * MAP_GRID_X, iArrowSector.y + 2 * MAP_GRID_Y);
			}

			InvalidateRegion(iSector.x, iSector.y, iSector.x + 2 * MAP_GRID_X, iSector.y + 2 * MAP_GRID_Y);
		}
	}
}


static BOOLEAN TraceCharAnimatedRoute(PathSt* pPath, BOOLEAN fForceUpDate);


static void AnimateRoute(PathSt* pPath)
{
	SetFontDestBuffer(FRAME_BUFFER);

	// the animated path
	if (TraceCharAnimatedRoute(pPath, FALSE))
	{
		// ARM? Huh?  Why the same thing twice more?
		TraceCharAnimatedRoute(pPath, TRUE);
		TraceCharAnimatedRoute(pPath, TRUE);
	}
}


static BOOLEAN TraceCharAnimatedRoute(PathSt* const pPath, const BOOLEAN fForceUpDate)
{
	static PathSt* pCurrentNode = NULL;
	static BOOLEAN fUpDateFlag=FALSE;
	static BOOLEAN fPauseFlag=TRUE;
	static UINT8 ubCounter=1;

	INT32 iArrow=-1;
	SGPSector iSector;
	SGPSector iArrowSector;
	INT32 iDeltaA, iDeltaB, iDeltaB1;
	INT32 iDirection = -1;
	BOOLEAN fUTurnFlag=FALSE;
	BOOLEAN fNextNode=FALSE;
	PathSt* pTempNode = NULL;
	PathSt* pNode     = NULL;
	PathSt* pPastNode = NULL;
	PathSt* pNextNode = NULL;


	// must be plotting movement
	if (bSelectedDestChar == -1 && !fPlotForHelicopter)
	{
		return FALSE;
	}

	// if any nodes have been deleted, reset current node to beginning of the list
	if( fDeletedNode )
	{
		fDeletedNode = FALSE;
		pCurrentNode = NULL;
	}


	// Valid path?
	if ( pPath == NULL )
	{
		return FALSE;
	}
	else
	{
		if(pCurrentNode==NULL)
		{
			pCurrentNode = pPath;
		}
	}

	// Check Timer
	if (guiAnimateRouteBaseTime==0)
	{
		guiAnimateRouteBaseTime=GetJA2Clock();
		return FALSE;
	}

	// check difference in time
	UINT32 uiDifference = GetJA2Clock() - guiAnimateRouteBaseTime;

	// if pause flag, check time, if time passed, reset, continue on, else return
	if(fPauseFlag)
	{
		if (uiDifference < PAUSE_DELAY)
		{
			return FALSE;
		}
		else
		{
			fPauseFlag=FALSE;
		guiAnimateRouteBaseTime=GetJA2Clock();
		}
	}


	// if is checkflag and change in status, return TRUE;
	if(!fForceUpDate)
	{
		if (uiDifference < ARROW_DELAY)
		{
			if (!fUpDateFlag)
				return FALSE;
		}
		else
		{
			// sufficient time, update base time
			guiAnimateRouteBaseTime=GetJA2Clock();
			fUpDateFlag=!fUpDateFlag;

			fNextNode=TRUE;
		}
	}

	// check to see if Current node has not been deleted
	pTempNode = pPath;

	while(pTempNode)
	{
		if(pTempNode==pCurrentNode)
		{
			//not deleted
			//reset pause flag
			break;
		}
		else
			pTempNode=pTempNode->pNext;
	}

	// if deleted, restart at beginnning
	if(pTempNode==NULL)
	{
		pCurrentNode = pPath;
	}

	// Handle drawing of arrow
	pNode=pCurrentNode;
	if((!pNode->pPrev)&&(ubCounter==1)&&(fForceUpDate))
	{
		ubCounter=0;
		return FALSE;
	}
	else if((ubCounter==1)&&(fForceUpDate))
	{
		pNode=pCurrentNode->pPrev;
	}
	if (pNode->pNext)
		pNextNode=pNode->pNext;
	else
		pNextNode=NULL;

	if (pNode->pPrev)
		pPastNode=pNode->pPrev;
	else
		pPastNode=NULL;

	// go through characters list and display arrows for path
	iSector = SGPSector::FromStrategicIndex(pNode->uiSectorId);
	iSector.x = iSector.x * MAP_GRID_X + MAP_VIEW_START_X;
	iSector.y = iSector.y * MAP_GRID_Y + MAP_VIEW_START_Y;
	iArrowSector = iSector;
	fUTurnFlag=FALSE;
	if ((pPastNode)&&(pNextNode))
	{
		iDeltaA=(INT16)pNode->uiSectorId-(INT16)pPastNode->uiSectorId;
		iDeltaB=(INT16)pNode->uiSectorId-(INT16)pNextNode->uiSectorId;
		if (iDeltaA ==0)
			return FALSE;

		if ((pPastNode->pPrev)&&(pNextNode->pNext))
		{
			fUTurnFlag=FALSE;
			// check to see if out-of sector U-turn
			// for placement of arrows
			iDeltaB1=pNextNode->uiSectorId-pNextNode->pNext->uiSectorId;
			if ((iDeltaB1==-WORLD_MAP_X)&&(iDeltaA==-WORLD_MAP_X)&&(iDeltaB==-1))
			{
				fUTurnFlag=TRUE;
			}
			else if((iDeltaB1==-WORLD_MAP_X)&&(iDeltaA==-WORLD_MAP_X)&&(iDeltaB==1))
			{
				fUTurnFlag=TRUE;
			}
			else if((iDeltaB1==WORLD_MAP_X)&&(iDeltaA==WORLD_MAP_X)&&(iDeltaB==-1))
			{
				fUTurnFlag=TRUE;
			}
			else if((iDeltaB1==WORLD_MAP_X)&&(iDeltaA==WORLD_MAP_X)&&(iDeltaB==1))
			{
				fUTurnFlag=TRUE;
			}
			else if((iDeltaB1==-1)&&(iDeltaA==-1)&&(iDeltaB==-WORLD_MAP_X))
			{
				fUTurnFlag=TRUE;
			}
			else if((iDeltaB1==-1)&&(iDeltaA==-1)&&(iDeltaB==WORLD_MAP_X))
			{
				fUTurnFlag=TRUE;
			}
			else if((iDeltaB1==1)&&(iDeltaA==1)&&(iDeltaB==-WORLD_MAP_X))
			{
				fUTurnFlag=TRUE;
			}
			else if((iDeltaB1==1)&&(iDeltaA==1)&&(iDeltaB==WORLD_MAP_X))
			{
				fUTurnFlag=TRUE;
			}
			else
				fUTurnFlag=FALSE;
		}


		if (pPastNode->uiSectorId==pNextNode->uiSectorId)
		{
			if (pPastNode->uiSectorId + WORLD_MAP_X == pNode->uiSectorId)
			{
				iDirection = S_TO_N_LINE;
				if (!ubCounter)
					iArrow = W_NORTH_ARROW;
				else
					iArrow = NORTH_ARROW;

				iArrowSector += northOffset;
			}
			else if(pPastNode->uiSectorId-WORLD_MAP_X==pNode->uiSectorId)
			{
				iDirection=N_TO_S_LINE;
				if(!ubCounter)
					iArrow=W_SOUTH_ARROW;
				else
					iArrow=SOUTH_ARROW;
				iArrowSector += southOffset;
			}
			else if (pPastNode->uiSectorId+1==pNode->uiSectorId)
			{
				iDirection=E_TO_W_LINE;
				if(!ubCounter)
					iArrow=W_WEST_ARROW;
				else
					iArrow=WEST_ARROW;
				iArrowSector += westOffset;
			}
			else
			{
				iDirection=W_TO_E_LINE;
				if(!ubCounter)
					iArrow=W_EAST_ARROW;
				else
					iArrow=EAST_ARROW;
				iArrowSector += eastOffset;
			}
		}
		else
		{
			if ((iDeltaA==-1)&&(iDeltaB==1))
			{
				iDirection = WEST_LINE;
				if (!ubCounter)
					iArrow = W_WEST_ARROW;
				else
					iArrow = WEST_ARROW;
				iArrowSector += westOffset;
			}
			else if((iDeltaA==1)&&(iDeltaB==-1))
			{
				iDirection = EAST_LINE;
				if (!ubCounter)
					iArrow = W_EAST_ARROW;
				else
					iArrow = EAST_ARROW;
				iArrowSector += eastOffset;
			}
			else if((iDeltaA==-WORLD_MAP_X)&&(iDeltaB==WORLD_MAP_X))
			{
				iDirection=NORTH_LINE;
				if(!ubCounter)
					iArrow=W_NORTH_ARROW;
				else
					iArrow=NORTH_ARROW;
				iArrowSector += northOffset;
			}
			else if((iDeltaA==WORLD_MAP_X)&&(iDeltaB==-WORLD_MAP_X))
			{
				iDirection=SOUTH_LINE;
				if(!ubCounter)
					iArrow=W_SOUTH_ARROW;
				else
					iArrow=SOUTH_ARROW;

				iArrowSector += southOffset;
			}
			else if((iDeltaA==-WORLD_MAP_X)&&(iDeltaB==-1))
			{
				iDirection=N_TO_E_LINE;
				if(!ubCounter)
					iArrow=W_EAST_ARROW;
				else
				iArrow=EAST_ARROW;
				iArrowSector += eastOffset;
			}
			else if((iDeltaA==WORLD_MAP_X)&&(iDeltaB==1))
			{
				iDirection=S_TO_W_LINE;
				if(!ubCounter)
					iArrow=W_WEST_ARROW;
				else
				iArrow=WEST_ARROW;
				iArrowSector += westOffset;
			}
			else if((iDeltaA==1)&&(iDeltaB==-WORLD_MAP_X))
			{
				iDirection=E_TO_S_LINE;
				if(!ubCounter)
					iArrow=W_SOUTH_ARROW;
				else
				iArrow=SOUTH_ARROW;
				iArrowSector += southOffset;
			}
			else if ((iDeltaA==-1)&&(iDeltaB==WORLD_MAP_X))
			{
				iDirection=W_TO_N_LINE;
				if(!ubCounter)
					iArrow=W_NORTH_ARROW;
				else
					iArrow=NORTH_ARROW;
				iArrowSector += northOffset;
			}
			else if ((iDeltaA==-1)&&(iDeltaB==-WORLD_MAP_X))
			{
				iDirection=W_TO_S_LINE;
				if(!ubCounter)
					iArrow=W_SOUTH_ARROW;
				else
					iArrow=SOUTH_ARROW;
				iArrowSector += southOffset;
				iArrowSector.y += WEST_TO_SOUTH_OFFSET_Y;
			}
			else if ((iDeltaA==-WORLD_MAP_X)&&(iDeltaB==1))
			{
				iDirection=N_TO_W_LINE;
				if(!ubCounter)
					iArrow=W_WEST_ARROW;
				else
					iArrow=WEST_ARROW;
				iArrowSector += westOffset;
			}
			else if ((iDeltaA==WORLD_MAP_X)&&(iDeltaB==-1))
			{
				iDirection=S_TO_E_LINE;
				if(!ubCounter)
					iArrow=W_EAST_ARROW;
				else
					iArrow=EAST_ARROW;
				iArrowSector += eastOffset;
			}
			else if ((iDeltaA==1)&&(iDeltaB==WORLD_MAP_X))
			{
				iDirection=E_TO_N_LINE;
				if(!ubCounter)
					iArrow=W_NORTH_ARROW;
				else
					iArrow=NORTH_ARROW;
				iArrowSector += northOffset;
				iArrowSector.y += EAST_TO_NORTH_OFFSET_Y;
			}
		}
	}
	else
	{
		// display enter and exit 'X's
		if (pPastNode)
		{
			// red 'X'
			fUTurnFlag=TRUE;
			iDeltaA=(INT16)pNode->uiSectorId-(INT16)pPastNode->uiSectorId;
			if (iDeltaA==-1)
			{
				iDirection=RED_X_WEST;
				//iSector.x+=RED_WEST_OFF_X;
			}
			else if (iDeltaA==1)
			{
				iDirection=RED_X_EAST;
				//iSector.x+=RED_EAST_OFF_X;
			}
			else if(iDeltaA==-WORLD_MAP_X)
			{
				iDirection=RED_X_NORTH;
				//iSector.y+=RED_NORTH_OFF_Y;
			}
			else
			{
				iDirection=RED_X_SOUTH;
				//iSector.y+=RED_SOUTH_OFF_Y;
			}
		}
		if (pNextNode)
		{
			fUTurnFlag=FALSE;
			iDeltaB=(INT16)pNode->uiSectorId-(INT16)pNextNode->uiSectorId;
			if (iDeltaB==-1)
			{
				iDirection=GREEN_X_EAST;
				if(!ubCounter)
					iArrow=W_EAST_ARROW;
				else
					iArrow=EAST_ARROW;
				iArrowSector += eastOffset;
				//iSector.x+=RED_EAST_OFF_X;
			}
			else if (iDeltaB==1)
			{
				iDirection=GREEN_X_WEST;
				if(!ubCounter)
					iArrow=W_WEST_ARROW;
				else
					iArrow=WEST_ARROW;
				iArrowSector += westOffset;
				//iSector.x+=RED_WEST_OFF_X;
			}
			else if(iDeltaB==WORLD_MAP_X)
			{
				iDirection=GREEN_X_NORTH;
				if(!ubCounter)
					iArrow=W_NORTH_ARROW;
				else
					iArrow=NORTH_ARROW;
				iArrowSector += northOffset;
				//iSector.y+=RED_NORTH_OFF_Y;
			}
			else
			{
				iDirection=GREEN_X_SOUTH;
				if(!ubCounter)
					iArrow=W_SOUTH_ARROW;
				else
					iArrow=SOUTH_ARROW;
				iArrowSector += southOffset;
				//iSector.y+=RED_SOUTH_OFF_Y;
			}


		}
	}
	if(fNextNode)
	{
		if(!ubCounter)
		{
			pCurrentNode=pCurrentNode->pNext;
			if(!pCurrentNode)
				fPauseFlag=TRUE;
		}
	}
	if ((iDirection !=-1)&&(iArrow!=-1))
	{

		if(!fUTurnFlag)
		{
			if ((MAP_VIEW_START_X < iSector.x && iSector.x < SCREEN_WIDTH - MAP_GRID_X * 2 && MAP_VIEW_START_Y < iSector.y && iSector.y < MAP_VIEW_START_Y + MAP_VIEW_HEIGHT))
			{
				if( pNode != pPath )
				{
					BltVideoObject(FRAME_BUFFER, guiMAPCURSORS, (UINT16)iArrow, iArrowSector.x, iArrowSector.y);
					InvalidateRegion( iArrowSector.x, iArrowSector.y, iArrowSector.x + 2 * MAP_GRID_X, iArrowSector.y + 2 * MAP_GRID_Y );
				}
			}
			if(ubCounter==1)
				ubCounter=0;
			else
				ubCounter=1;
			return TRUE;
		}
		if(ubCounter==1)
			ubCounter=0;
		else
			ubCounter=1;


	}
	// move to next arrow


	//ARM who knows what it should return here?
	return FALSE;
}


// display potential path, yes or no?
void DisplayThePotentialPathForHelicopter(const SGPSector& sMap)
{
	// simply check if we want to refresh the screen to display path
	static BOOLEAN fOldShowAirCraft = FALSE;
	static SGPSector sOldMap;

	if( fOldShowAirCraft != fShowAircraftFlag )
	{
		fOldShowAirCraft = fShowAircraftFlag;
		guiPotHeliPathBaseTime = GetJA2Clock( );

		sOldMap = sMap;
		fTempPathAlreadyDrawn = FALSE;
		fDrawTempHeliPath = FALSE;

	}

	if (sMap != sOldMap)
	{
		guiPotHeliPathBaseTime = GetJA2Clock( );
		sOldMap = sMap;

		// path was plotted and we moved, re draw map..to clean up mess
		if( fTempPathAlreadyDrawn )
		{
			fMapPanelDirty = TRUE;
		}

		fTempPathAlreadyDrawn = FALSE;
		fDrawTempHeliPath = FALSE;
	}

	UINT32 uiDifference = GetJA2Clock() - guiPotHeliPathBaseTime;

	if( fTempPathAlreadyDrawn )
	{
		return;
	}

	if (uiDifference > MIN_WAIT_TIME_FOR_TEMP_PATH)
	{
		fDrawTempHeliPath = TRUE;
		guiPotHeliPathBaseTime = GetJA2Clock( );
		fTempPathAlreadyDrawn = TRUE;
	}
}


bool IsTheCursorAllowedToHighLightThisSector(const SGPSector& sMap)
{
	// It is not strictly necessary to construct a new SGPSector above because
	// AsByte() would ignore the z member anyway. It is done here to avoid
	// possible confusion; accessing the SectorInfo array with only covers
	// ground level sectors with an underground SGPSector may look wrong.
	return
		sMap.IsValid() &&
		(SectorInfo[SGPSector{ sMap.x, sMap.y, 0 }.AsByte()]
			.ubTraversability[THROUGH_STRATEGIC_MOVE] != EDGEOFWORLD);
}


void RestoreBackgroundForMapGrid(const SGPSector& sMap)
{
	// screen values
	INT16 sX = (sMap.x * MAP_GRID_X) + MAP_VIEW_START_X;
	INT16 sY = (sMap.y * MAP_GRID_Y) + MAP_VIEW_START_Y;

	// restore background
	RestoreExternBackgroundRect(sX, sY, DMAP_GRID_X, DMAP_GRID_Y);
}


void ClipBlitsToMapViewRegion( void )
{
	SetClippingRect(MapScreenRect);
	gOldClipRect = std::exchange(gDirtyClipRect, MapScreenRect);
}

void RestoreClipRegionToFullScreen( void )
{
	SGPRect FullScreenRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	SetClippingRect(FullScreenRect);
	gDirtyClipRect = gOldClipRect;
}


void ClipBlitsToMapViewRegionForRectangleAndABit( UINT32 uiDestPitchBYTES )
{
	// clip blits to map view region
	// because MC's map coordinates system is so screwy, these had to be hand-tuned to work right...  ARM
	SetClippingRegionAndImageWidth( uiDestPitchBYTES, MapScreenRect.iLeft - 1, MapScreenRect.iTop - 1, MapScreenRect.iRight - MapScreenRect.iLeft + 3, MapScreenRect.iBottom - MapScreenRect.iTop + 2 );
}

void RestoreClipRegionToFullScreenForRectangle( UINT32 uiDestPitchBYTES )
{
	// clip blits to map view region
	SetClippingRegionAndImageWidth(uiDestPitchBYTES, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}


// mvt icon offset defines
#define SOUTH_Y_MVT_OFFSET +10
#define SOUTH_X_MVT_OFFSET 0
#define NORTH_Y_MVT_OFFSET -10
#define NORTH_X_MVT_OFFSET  +10
#define NORTH_SOUTH_CENTER_OFFSET +5


#define EAST_Y_MVT_OFFSET + 8
#define EAST_X_MVT_OFFSET 15
#define WEST_Y_MVT_OFFSET -2
#define WEST_X_MVT_OFFSET -8
#define EAST_WEST_CENTER_OFFSET +2

#define NORTH_TEXT_X_OFFSET +1 + 14 + 1 + 1
#define NORTH_TEXT_Y_OFFSET +4 + 5 + 9 - 2
#define SOUTH_TEXT_X_OFFSET +1 + 9
#define SOUTH_TEXT_Y_OFFSET +2 + 14 - 9

#define EAST_TEXT_X_OFFSET + 2 + 18 - 11 + 1
#define EAST_TEXT_Y_OFFSET 0 + 10
#define WEST_TEXT_X_OFFSET + 4 + 4 + 10 + 1
#define WEST_TEXT_Y_OFFSET 0 + 10


#define ICON_WIDTH 8

// Total pixel displacement the transit arrow (and its merc-count number)
// slides in the direction of travel over the course of one leg (one sector
// to the next), reaching this exactly at arrival regardless of how long the
// leg actually takes -- see PlayersBetweenTheseSectors()'s
// transit_fraction_enter output. Matches MERC_BETWEEN_SECTOR_ICONS.STI's
// own width/height (18px, per user measurement).
#define MVT_ANIM_TOTAL_PIXELS 18


// show the icons for people in motion
static void ShowPeopleInMotion(const SGPSector& sSector)
{
	if (sSector.z != 0) return;

	// show the icons for people in motion from this sector to the next guy over
	INT16 const sSource = sSector.AsStrategicIndex();
	for (INT32 dir = 0; dir != 4; ++dir)
	{ // find how many people are coming and going in this sector
		INT16 sDest = sSource;
		switch (dir)
		{
			// note that these limit tests are 1 sector stricter than sSector.IsValid()
			case 0: if (sSector.y <= 1)               continue; sDest += NORTH_MOVE; break;
			case 1: if (sSector.x >= MAP_WORLD_X - 2) continue; sDest += EAST_MOVE;  break;
			case 2: if (sSector.y >= MAP_WORLD_Y - 2) continue; sDest += SOUTH_MOVE; break;
			case 3: if (sSector.x <= 1)               continue; sDest += WEST_MOVE;  break;

			default: abort();
		}

		INT32       sExiting;
		INT32       sEntering;
		BOOLEAN     fAboutToEnter;
		float       flTransitFraction;
		INT16 const sec_src = SGPSector::FromStrategicIndex(sSource).AsByte();
		INT16 const sec_dst = SGPSector::FromStrategicIndex(sDest).AsByte();
		if (!PlayersBetweenTheseSectors(sec_src, sec_dst, &sExiting, &sEntering, &fAboutToEnter, &flTransitFraction)) continue;
		// someone is leaving

		// now find position
		INT16 sOffsetX;
		INT16 sOffsetY;
		if (dir % 2 == 0)
		{ // guys going north or south
			if (sEntering > 0)
			{ // more than one coming in, offset from middle
				sOffsetX = dir == 0 ?
					(NORTH_X_MVT_OFFSET) :
					(SOUTH_X_MVT_OFFSET);
			}
			else
			{
				sOffsetX = NORTH_SOUTH_CENTER_OFFSET;
			}

			if (dir == 0)
			{ // going north
				sOffsetY = NORTH_Y_MVT_OFFSET;
			}
			else
			{ // going south
				sOffsetY = SOUTH_Y_MVT_OFFSET;
			}
		}
		else
		{ // going east/west
			if (sEntering > 0)
			{ // people also entering, offset from middle
				sOffsetY = dir == 1 ?
					(EAST_Y_MVT_OFFSET) :
					(WEST_Y_MVT_OFFSET);
			}
			else
			{
				sOffsetY = EAST_WEST_CENTER_OFFSET;
			}

			if (dir == 1)
			{ // going east
				sOffsetX = EAST_X_MVT_OFFSET;
			}
			else
			{ // going west
				sOffsetX = WEST_X_MVT_OFFSET;
			}
		}

		INT16 sTextXOffset;
		INT16 sTextYOffset;
		switch (dir)
		{
			case 0: sTextXOffset = NORTH_TEXT_X_OFFSET; sTextYOffset = NORTH_TEXT_Y_OFFSET; break;
			case 1: sTextXOffset =  EAST_TEXT_X_OFFSET; sTextYOffset =  EAST_TEXT_Y_OFFSET; break;
			case 2: sTextXOffset = SOUTH_TEXT_X_OFFSET; sTextYOffset = SOUTH_TEXT_Y_OFFSET; break;
			case 3: sTextXOffset =  WEST_TEXT_X_OFFSET; sTextYOffset =  WEST_TEXT_Y_OFFSET; break;

			default: abort();
		}

		// if about to enter, draw yellow arrows, blue otherwise
		auto * const hIconHandle = fAboutToEnter ? guiCHARBETWEENSECTORICONSCLOSE : guiCHARBETWEENSECTORICONS;

		INT16 iX = MAP_VIEW_START_X                     + sSector.x * MAP_GRID_X + sOffsetX;
		INT16 iY = MAP_Y_ICON_OFFSET + MAP_VIEW_START_Y + sSector.y * MAP_GRID_Y + sOffsetY;

		// Slide the arrow (and, since the text position below is derived from
		// iX/iY, its merc-count number too) smoothly in the direction of
		// travel over the course of this leg. Reaches exactly
		// MVT_ANIM_TOTAL_PIXELS by the time the leg completes, regardless of
		// how long it took -- kept in sync every frame by MapScreen.cc
		// forcing a redraw while AnyPlayerGroupInMotion() is true.
		INT16 const sAnimOffset = (INT16)(flTransitFraction * MVT_ANIM_TOTAL_PIXELS + 0.5f);
		switch (dir)
		{
			case 0: iY -= sAnimOffset; break; // north: moving up
			case 1: iX += sAnimOffset; break; // east: moving right
			case 2: iY += sAnimOffset; break; // south: moving down
			case 3: iX -= sAnimOffset; break; // west: moving left
		}

		BltVideoObject(guiSAVEBUFFER, hIconHandle, dir, iX, iY);

		// blit the text
		// Always white text on a (default, dark) shadow, on both icons --
		// per user request, matching how it already looks on
		// guiCHARBETWEENSECTORICONS. The old fAboutToEnter-based switch to
		// black text broke down because DEFAULT_SHADOW (a dark index in
		// MAP_MVT_TRANSIT_FONT's own palette) disappears behind black text.
		SetFontAttributes(MAP_MVT_TRANSIT_FONT, FONT_WHITE);
		SetFontDestBuffer(guiSAVEBUFFER);

		ST::string buf = ST::format("{}", sExiting);

		INT16 usX;
		INT16 usY;
		// Centering width must be measured with the same font the text is
		// actually drawn in (MAP_MVT_TRANSIT_FONT) -- see MAP_MVT_TRANSIT_FONT's
		// own comment above for why this is a separate font from
		// MAP_MVT_ICON_FONT.
		FindFontCenterCoordinates(iX + sTextXOffset, 0, ICON_WIDTH, 0, buf, MAP_MVT_TRANSIT_FONT, &usX, &usY);
		MPrint(usX, iY + sTextYOffset, buf);

		INT32 iWidth;
		INT32 iHeight;
		if (dir % 2 == 0)
		{ // north south
			iWidth  = 10;
			iHeight = 12;
		}
		else
		{ // east west
			iWidth  = 12;
			iHeight =  7;
		}

		// error correction for scrolling with people on the move
		if (iX < 0)
		{
			iWidth += iX;
			iX      = 0;
		}

		if (iY < 0)
		{
			iHeight += iY;
			iY       = 0;
		}

		if (iWidth > 0 && iHeight > 0)
		{
			RestoreExternBackgroundRect(iX, iY, (INT16)iWidth, (INT16)iHeight);
		}
	}

	// restore buffer
	SetFontDestBuffer(FRAME_BUFFER);
}


/* calculate the distance travelled, the proposed distance, and total distance
 * one can go and display these on screen */
// Old behavior: which of the two heli ETA popups (pos2_first.sti/
// pos2_second.sti) is shown was decided by the row of the currently
// highlighted map sector (>= row 13 -> show the "upper" one instead, so the
// popup didn't run off the bottom of the screen). Deactivated (not removed)
// per user request, replaced by the cursor-hover-based swap below. Flip
// back to true to restore the old behavior.
constexpr bool ENABLE_ROW_BASED_HELI_POPUP_SWITCH = false;

void DisplayDistancesForHelicopter()
{
	static INT16 sOldXPosition = 0;
	static INT16 sOldYPosition = 0;
	// Persists across frames: which popup is currently being shown (false =
	// pos2_first.sti, true = pos2_second.sti). Only used by the new
	// hover-based swap logic below.
	static bool fShowSecondHeliPopup = false;

	bool fUpper;
	if (ENABLE_ROW_BASED_HELI_POPUP_SWITCH)
	{
		fUpper = gsHighlightSector.IsValid() && gsHighlightSector.y >= 13;
	}
	else
	{
		// New behavior, per user request: swap to the OTHER popup (and hide
		// this one) whenever the mouse cursor (with the helicopter icon,
		// while plotting a heli route) is hovering directly over the popup
		// that's currently being shown.
		INT16 const sCurX = fShowSecondHeliPopup ? MAP_HELICOPTER_UPPER_ETA_POPUP_X : MAP_HELICOPTER_ETA_POPUP_X;
		INT16 const sCurY = fShowSecondHeliPopup ? MAP_HELICOPTER_UPPER_ETA_POPUP_Y : MAP_HELICOPTER_ETA_POPUP_Y;
		if (gusMouseXPos >= (UINT16)sCurX && gusMouseXPos < (UINT16)(sCurX + MAP_HELICOPTER_ETA_POPUP_WIDTH) &&
			gusMouseYPos >= (UINT16)sCurY && gusMouseYPos < (UINT16)(sCurY + MAP_HELICOPTER_ETA_POPUP_HEIGHT))
		{
			fShowSecondHeliPopup = !fShowSecondHeliPopup;
		}
		fUpper = fShowSecondHeliPopup;
	}

	INT16 const sXPosition = fUpper ? MAP_HELICOPTER_UPPER_ETA_POPUP_X : MAP_HELICOPTER_ETA_POPUP_X;
	INT16 const sYPosition = fUpper ? MAP_HELICOPTER_UPPER_ETA_POPUP_Y : MAP_HELICOPTER_ETA_POPUP_Y;

	if (sOldYPosition != 0 && (sOldXPosition != sXPosition || sOldYPosition != sYPosition))
	{
		RestoreExternBackgroundRect(sOldXPosition, sOldYPosition, MAP_HELICOPTER_ETA_POPUP_WIDTH + 20, MAP_HELICOPTER_ETA_POPUP_HEIGHT);
	}
	sOldXPosition = sXPosition;
	sOldYPosition = sYPosition;

	BltVideoObject(FRAME_BUFFER, fUpper ? guiMapBorderHeliSectorsSecond : guiMapBorderHeliSectorsFirst, 0, sXPosition, sYPosition);

	SetFontAttributes(MAP_FONT, FONT_LTGREEN);

	// Independent text-layout constants per location -- see their
	// definitions above.
	INT32 const x = fUpper ? MAP_HELICOPTER_UPPER_ETA_TEXT_X       : MAP_HELICOPTER_ETA_TEXT_X;
	INT32       y = fUpper ? MAP_HELICOPTER_UPPER_ETA_TEXT_Y       : MAP_HELICOPTER_ETA_TEXT_Y;
	INT32 const w = fUpper ? MAP_HELICOPTER_UPPER_ETA_VALUE_MARGIN : MAP_HELICOPTER_ETA_VALUE_MARGIN;
	INT32 const h = GetFontHeight(MAP_FONT);
	ST::string sString;
	INT16       sX;
	INT16       sY;

	MPrint(x, y, pHelicopterEtaStrings[0]);
	INT32 const total_distance = DistanceOfIntendedHelicopterPath();
	sString = ST::format("{}", total_distance);
	FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT,  &sX, &sY);
	MPrint(sX, y, sString);
	y += h;

	MPrint(x, y, pHelicopterEtaStrings[1]);
	INT16 const n_safe_sectors = GetNumSafeSectorsInPath();
	sString = ST::format("{}", n_safe_sectors);
	FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
	MPrint(sX, y, sString);
	y += h;

	MPrint(x, y, pHelicopterEtaStrings[2]);
	INT16 const n_unsafe_sectors = GetNumUnSafeSectorsInPath();
	sString = ST::format("{}", n_unsafe_sectors);
	FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
	MPrint(sX, y, sString);
	y += h;

	// calculate the cost of the trip based on the number of safe and unsafe sectors it will pass through
	MPrint(x, y, pHelicopterEtaStrings[3]);
	UINT32 const uiTripCost = n_safe_sectors * COST_AIRSPACE_SAFE + n_unsafe_sectors * COST_AIRSPACE_UNSAFE;
	sString = SPrintMoney(uiTripCost);
	FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
	MPrint(sX, y, sString);
	y += h;

	MPrint(x, y, pHelicopterEtaStrings[4]);
	// get travel time for the last path segment
	INT32 iTime = GetPathTravelTimeDuringPlotting(pTempHelicopterPath);
	// add travel time for any prior path segments (stored in the helicopter's mercpath, but waypoints aren't built)
	iTime += GetPathTravelTimeDuringPlotting(GetHelicopter().pMercPath);
	sString = ST::format("{}{} {}{}", iTime / 60, gsTimeStrings[0], iTime % 60, gsTimeStrings[1]);
	FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
	MPrint(sX, y, sString);
	y += h;

	// show # of passengers aboard the chopper
	MPrint(x, y, pHelicopterEtaStrings[6]);
	sString = ST::format("{}", GetNumberInVehicle(GetHelicopter()));
	FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
	MPrint(sX, y, sString);

	InvalidateRegion(sXPosition, sYPosition, sXPosition + MAP_HELICOPTER_ETA_POPUP_WIDTH + 20, sYPosition + MAP_HELICOPTER_ETA_POPUP_HEIGHT);
}


static void DisplayDestinationOfHelicopter(void);


// grab position of helicopter and blt to screen
void DisplayPositionOfHelicopter( void )
{
	static INT16 sOldMapX = 0, sOldMapY = 0;
	//INT16 sX =0, sY = 0;
	FLOAT flRatio = 0.0;
	UINT32 x,y;
	UINT16 minX, minY, maxX, maxY;

	AssertMsg(0 <= sOldMapX && sOldMapX < SCREEN_WIDTH,  ST::format("DisplayPositionOfHelicopter: Invalid sOldMapX = {}", sOldMapX));
	AssertMsg(0 <= sOldMapY && sOldMapY < SCREEN_HEIGHT, ST::format("DisplayPositionOfHelicopter: Invalid sOldMapY = {}", sOldMapY));

	// restore background on map where it is
	if( sOldMapX != 0 )
	{
		RestoreExternBackgroundRect( sOldMapX, sOldMapY, HELI_ICON_WIDTH, HELI_ICON_HEIGHT );
		sOldMapX = 0;
	}


	if( iHelicopterVehicleId != -1 )
	{
		// draw the destination icon first, so when they overlap, the real one is on top!
		DisplayDestinationOfHelicopter( );

		VEHICLETYPE const& v = GetHelicopter();
		// check if mvt group
		if (v.ubMovementGroup != 0)
		{
			const GROUP* const pGroup = GetGroup(v.ubMovementGroup);

			if (pGroup->uiTraverseTime > 0 && pGroup->uiTraverseTime != TRAVERSE_TIME_IMPOSSIBLE)
			{
				flRatio = ( ( pGroup->uiTraverseTime + GetWorldTotalMin() ) - pGroup->uiArrivalTime ) / ( float ) pGroup->uiTraverseTime;
			}

			/*
			AssertMsg((flRatio >= 0) && (flRatio <= 100), ST::format("DisplayPositionOfHelicopter: Invalid flRatio = {}.2f, trav {}, arr {}, time {}",
					flRatio, pGroup->uiTraverseTime, pGroup->uiArrivalTime, GetWorldTotalMin() ) );*/

			if ( flRatio < 0 )
			{
				flRatio = 0;
			}
			else if ( flRatio > 100 )
			{
				flRatio = 100;
			}

			// grab min and max locations to interpolate sub sector position
			minX = MAP_VIEW_START_X + MAP_GRID_X * pGroup->ubSector.x;
			maxX = MAP_VIEW_START_X + MAP_GRID_X * pGroup->ubNext.x;
			minY = MAP_VIEW_START_Y + MAP_GRID_Y * pGroup->ubSector.y;
			maxY = MAP_VIEW_START_Y + MAP_GRID_Y * pGroup->ubNext.y;

			AssertMsg(minX < SCREEN_WIDTH, ST::format("DisplayPositionOfHelicopter: Invalid minX = {}", minX));
			AssertMsg(maxX < SCREEN_WIDTH, ST::format("DisplayPositionOfHelicopter: Invalid maxX = {}", maxX));
			AssertMsg(minY < SCREEN_HEIGHT, ST::format("DisplayPositionOfHelicopter: Invalid minY = {}", minY));
			AssertMsg(maxY < SCREEN_HEIGHT, ST::format("DisplayPositionOfHelicopter: Invalid maxY = {}", maxY));

			// IMPORTANT: Since min can easily be larger than max, we gotta cast to as signed value
			x = ( UINT32 )( minX + flRatio * ( ( INT16 ) maxX - ( INT16 ) minX ) );
			y = ( UINT32 )( minY + flRatio * ( ( INT16 ) maxY - ( INT16 ) minY ) );
			x += 1;
			y += 3;

			AssertMsg(x < SCREEN_WIDTH, ST::format("DisplayPositionOfHelicopter: Invalid x = {}.  At {},{}.  Next {},{}.  Min/Max X = {}/{}",
					x, pGroup->ubSector.x, pGroup->ubSector.y, pGroup->ubNext.x, pGroup->ubNext.y, minX, maxX));

			AssertMsg(y < SCREEN_HEIGHT, ST::format("DisplayPositionOfHelicopter: Invalid y = {}.  At {},{}.  Next {},{}.  Min/Max Y = {}/{}",
					y, pGroup->ubSector.x, pGroup->ubSector.y, pGroup->ubNext.x, pGroup->ubNext.y, minY, maxY));


			// clip blits to mapscreen region
			ClipBlitsToMapViewRegion( );

			BltVideoObject(FRAME_BUFFER, guiHelicopterIcon, HELI_ICON, x, y);

			SetFontAttributes(MAP_MVT_ICON_FONT, FONT_WHITE);
			MPrint(x + 5, y + 1, ST::format("{}", GetNumberInVehicle(v)));

			InvalidateRegion( x, y, x + HELI_ICON_WIDTH, y + HELI_ICON_HEIGHT );

			RestoreClipRegionToFullScreen( );

			// now store the old stuff
			sOldMapX = ( INT16 ) x;
			sOldMapY = ( INT16 ) y;
		}
	}
}


static void DisplayDestinationOfHelicopter(void)
{
	static INT16 sOldMapX = 0, sOldMapY = 0;

	AssertMsg(0 <= sOldMapX && sOldMapX < SCREEN_WIDTH, ST::format("DisplayDestinationOfHelicopter: Invalid sOldMapX = {}", sOldMapX));
	AssertMsg(0 <= sOldMapY && sOldMapY < SCREEN_HEIGHT, ST::format("DisplayDestinationOfHelicopter: Invalid sOldMapY = {}", sOldMapY));

	// restore background on map where it is
	if( sOldMapX != 0 )
	{
		RestoreExternBackgroundRect( sOldMapX, sOldMapY, HELI_SHADOW_ICON_WIDTH, HELI_SHADOW_ICON_HEIGHT );
		sOldMapX = 0;
	}

	// if helicopter is going somewhere
	if (GetLengthOfPath(GetHelicopter().pMercPath) > 1)
	{
		// get destination
		const INT16 sSector = GetLastSectorOfHelicoptersPath();
		SGPSector sMap = SGPSector::FromStrategicIndex(sSector);

		UINT32 x = MAP_VIEW_START_X + ( MAP_GRID_X * sMap.x ) + 1;
		UINT32 y = MAP_VIEW_START_Y + ( MAP_GRID_Y * sMap.y ) + 3;

		AssertMsg(x < SCREEN_WIDTH, ST::format("DisplayDestinationOfHelicopter: Invalid x = {}.  Dest {}", x, sMap));
		AssertMsg(y < SCREEN_HEIGHT, ST::format("DisplayDestinationOfHelicopter: Invalid y = {}.  Dest {}", y, sMap));

		// clip blits to mapscreen region
		ClipBlitsToMapViewRegion( );

		BltVideoObject(FRAME_BUFFER, guiHelicopterIcon, HELI_SHADOW_ICON, x, y);
		InvalidateRegion( x, y, x + HELI_SHADOW_ICON_WIDTH, y + HELI_SHADOW_ICON_HEIGHT );

		RestoreClipRegionToFullScreen( );

		// now store the old stuff
		sOldMapX = ( INT16 ) x;
		sOldMapY = ( INT16 ) y;
	}
}



BOOLEAN CheckForClickOverHelicopterIcon(const SGPSector& sClicked)
{
	BOOLEAN fHelicopterOverNextSector = FALSE;
	FLOAT flRatio = 0.0;

	if (!fShowAircraftFlag)         return FALSE;
	if (iHelicopterVehicleId == -1) return FALSE;

	VEHICLETYPE const& v = GetHelicopter();

	// figure out over which sector the helicopter APPEARS to be to the player (because we slide it smoothly across the
	// map, unlike groups travelling on the ground, it can appear over its next sector while it's not there yet.
	GROUP const* const pGroup = GetGroup(v.ubMovementGroup);
	Assert( pGroup );

	if ( pGroup->fBetweenSectors )
	{
		if (pGroup->uiTraverseTime > 0 && pGroup->uiTraverseTime != TRAVERSE_TIME_IMPOSSIBLE)
		{
			flRatio = ( pGroup->uiTraverseTime - pGroup->uiArrivalTime + GetWorldTotalMin() ) / ( float ) pGroup->uiTraverseTime;
		}

		// if more than halfway there, the chopper appears more over the next sector, not over its current one(!)
		if ( flRatio > 0.5 )
		{
			fHelicopterOverNextSector = TRUE;
		}
	}


	SGPSector sSector;
	if ( fHelicopterOverNextSector )
	{
		// use the next sector's coordinates
		sSector = pGroup->ubNext;
	}
	else
	{
		// use current sector's coordinates
		sSector = v.sSector;
	}
	sSector.z = 0;

	// check if helicopter appears where he clicked
	return sSector == sClicked;
}


static void DrawSite(const SGPSector& sMap, const SGPVObject* const icon)
{
	INT16  x;
	INT16  y;
	UINT16 max_w;
	UINT16 max_h;
	UINT8  vo_idx;

	GetScreenXYFromMapXY(sMap, &x, &y);
	++x;
	max_w = MAP_GRID_X - 1;
	max_h = MAP_GRID_Y - 1;
	vo_idx = 1;

	ETRLEObject const& ETRLEProps = icon->SubregionProperties(vo_idx);
	UINT16      const  w          = ETRLEProps.usWidth;
	UINT16      const  h          = ETRLEProps.usHeight;
	x += (max_w - w) / 2;
	/* If the icon is higher than a map cell, align with the bottom of the cell */
	y += (h > max_h ? max_h - h : (max_h - h) / 2);

	BltVideoObject(guiSAVEBUFFER, icon, vo_idx, x, y);
}


static void BlitMineIcon(const SGPSector& sMap)
{
	DrawSite(sMap, GetVObject(guiMINEICON));
}


static void PrintStringCenteredBoxed(INT32 x, INT32 y, const ST::string& string)
{
	x -= StringPixLength(string, MAP_FONT) / 2;
	if (x < MAP_VIEW_START_X + 23) x = MAP_VIEW_START_X + 23;

	MPrint(x, y, string);
}


static void BlitMineText(UINT8 const mine_idx, const SGPSector& sMap)
{
	// set coordinates for start of mine text
	INT16 sScreenX;
	INT16 sScreenY;

	GetScreenXYFromMapXY(sMap, &sScreenX, &sScreenY);
	sScreenX += MAP_GRID_X / 2; // centered around middle of mine square
	sScreenY += MAP_GRID_Y + 1; // slightly below

	// show detailed mine info (name, production rate, daily production)

	SetFontDestBuffer(guiSAVEBUFFER, MAP_VIEW_START_X, MAP_VIEW_START_Y, MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X, MAP_VIEW_START_Y + MAP_VIEW_HEIGHT + 7);
	SetFontAttributes(MAP_FONT, FONT_LTGREEN);

	INT32 const x = sScreenX;
	INT32       y = sScreenY;
	INT32 const h = GetFontHeight(MAP_FONT);
	ST::string buf;

	// display associated town name, followed by "mine"
	buf = ST::format("{} {}", GCM->getTownName(GetTownAssociatedWithMine(mine_idx)), pwMineStrings[0]);
	PrintStringCenteredBoxed(x, y, buf);
	y += h;

	// check if mine is empty (abandoned) or running out
	if (gMineStatus[mine_idx].fEmpty)
	{
		PrintStringCenteredBoxed(x, y, pwMineStrings[5]);
		y += h;
	}
	else if (gMineStatus[mine_idx].fShutDown)
	{
		PrintStringCenteredBoxed(x, y, pwMineStrings[6]);
		y += h;
	}
	else if (gMineStatus[mine_idx].fRunningOut)
	{
		PrintStringCenteredBoxed(x, y, pwMineStrings[7]);
		y += h;
	}

	// only show production if player controls it and it's actually producing
	if (PlayerControlsMine(mine_idx) && !gMineStatus[mine_idx].fEmpty)
	{
		// show current production
		buf = SPrintMoney(PredictDailyIncomeFromAMine(mine_idx));

		// if potential is not nil, show percentage of the two
		UINT32 maxIncome = GetMaxDailyRemovalFromMine(mine_idx);
		if (maxIncome > 0)
		{
			UINT32 predictedIncome = PredictDailyIncomeFromAMine(mine_idx);
			buf += ST::format(" ({}%)", 100 * predictedIncome / maxIncome);
		}

		PrintStringCenteredBoxed(x, y, buf);
	}

	SetFontDestBuffer(FRAME_BUFFER, MAP_VIEW_START_X, MAP_VIEW_START_Y, MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X, MAP_VIEW_START_Y + MAP_VIEW_HEIGHT + 7);
}


// Each town's grey border ("Show Towns") can be nudged independently on top
// of its sector-grid position (GetScreenXYFromMapXY()), to line up with the
// map graphic without moving the grid, sector icons or mouse regions: the
// town's optional "borderOffset1024" (height 768+, b_map_1024.pcx) /
// "borderOffset1280" (height 720-767, b_map_1280.pcx) in
// strategic-map-towns.json -- see TownModel. Still clipped to MapScreenRect.
static void BlitTownGridMarkers(void)
{
	SGPVSurface::Lock l(guiSAVEBUFFER);
	UINT16* const buf   = l.Buffer<UINT16>();
	UINT32  const pitch = l.Pitch();

	ClipBlitsToMapViewRegionForRectangleAndABit(pitch);

	// Go through list of towns and place on screen
	UINT16 const color = Get16BPPColor(FROMRGB(100, 100, 100));
	FOR_EACH_TOWN_SECTOR(i)
	{
		// skip Orta/Tixa until found
		if (!IsTownFound(i->town)) continue;

		INT16       x;
		INT16       y;
		INT16       w;
		INT16       h;
		// Get location on screen
		SGPSector sMap(i->sector);
		GetScreenXYFromMapXY(sMap, &x, &y);
		w  = MAP_GRID_X - 1;
		h  = MAP_GRID_Y;
		x += 2;

		TownModel const* const town = GCM->getTown(i->town);
		if (town)
		{
			TownBorderOffset const& offset = g_ui.isCompactStrategicScreen() ? town->borderOffset1280 : town->borderOffset1024;
			x += offset.x;
			y += offset.y;
		}

		INT32 const loc = sMap.AsStrategicIndex();
		if (StrategicMap[loc - MAP_WORLD_X].bNameId == BLANK_SECTOR)
		{
			LineDraw(TRUE, x - 1, y - 1, x + w - 1, y - 1, color, buf);
		}

		if (StrategicMap[loc + MAP_WORLD_X].bNameId == BLANK_SECTOR)
		{
			LineDraw(TRUE, x - 1, y + h - 1, x + w - 1, y + h - 1, color, buf);
		}

		if (StrategicMap[loc - 1].bNameId == BLANK_SECTOR)
		{
			LineDraw(TRUE, x - 2, y - 1, x - 2, y + h - 1, color, buf);
		}

		if (StrategicMap[loc + 1].bNameId == BLANK_SECTOR)
		{
			LineDraw(TRUE, x + w - 1, y - 1, x + w - 1, y + h - 1, color, buf);
		}
	}

	RestoreClipRegionToFullScreenForRectangle(pitch);
}


static void BlitMineGridMarkers(void)
{
	SGPVSurface::Lock l(guiSAVEBUFFER);
	UINT32 const pitch = l.Pitch();

	ClipBlitsToMapViewRegionForRectangleAndABit(pitch);

	UINT16 const color = Get16BPPColor(FROMRGB(100, 100, 100));
	for (auto m : GCM->getMines())
	{
		INT16                     x;
		INT16                     y;
		INT16                     w;
		INT16                     h;
		SGPSector sMap(m->entranceSector);

		// Get location on screen
		GetScreenXYFromMapXY(sMap, &x, &y);
		w = MAP_GRID_X;
		h = MAP_GRID_Y;

		RectangleDraw(TRUE, x, y - 1, x + w, y + h - 1, color, l.Buffer<UINT16>());
	}

	RestoreClipRegionToFullScreenForRectangle(pitch);
}


static void DisplayLevelString(void)
{
	// given the current level being displayed on the map, show a sub level message

	// at the surface
	if( !iCurrentMapSectorZ )
	{
		return;
	}

	// otherwise we will have to display the string with the level number

	SetFontDestBuffer(guiSAVEBUFFER, MAP_VIEW_START_X, MAP_VIEW_START_Y, MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X, MAP_VIEW_START_Y + MAP_VIEW_HEIGHT + 7);
	SetFontAttributes(MAP_FONT, MAP_INDEX_COLOR);
	MPrint(MAP_LEVEL_STRING_X, MAP_LEVEL_STRING_Y, ST::format("{} {}", sMapLevelString, iCurrentMapSectorZ));
	SetFontDestBuffer(FRAME_BUFFER);
}


static INT16& GetPickedUpMilitia(UINT8 const type)
{
	switch (type)
	{
		case GREEN_MILITIA:   return sGreensOnCursor;
		case REGULAR_MILITIA: return sRegularsOnCursor;
		case ELITE_MILITIA:   return sElitesOnCursor;
		case ELITE_HEAVY_MILITIA: return sHeaviesOnCursor;
		default: throw std::logic_error("invalid militia type");
	}
}


// The unassigned militia ("on the cursor") per rank
static UINT8 const (&CursorMilitia())[MAX_MILITIA_LEVELS]
{
	static UINT8 counts[MAX_MILITIA_LEVELS];
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		counts[rank] = (UINT8)std::clamp<INT16>(GetPickedUpMilitia(rank), 0, 255);
	}
	return counts;
}


static UINT32 CursorMilitiaTotal()
{
	return sGreensOnCursor + sRegularsOnCursor + sElitesOnCursor + sHeaviesOnCursor;
}


// D5.2: the sector of the battle loaded in tactical, when the arrival box has
// it: militia put there join the battle when the box is closed with Done
static bool IsReinforceSector(INT16 const sector)
{
	return g_arrival_order != 0 && gfWorldLoaded && gWorldSector.z == 0 &&
		gTacticalStatus.fEnemyInSector && gWorldSector.AsByte() == sector &&
		MilitiaBoxHasSector(sSelectedMilitiaTown, sector);
}


// The militia put into the battle's sector for it, of this rank
static UINT8 Reinforcing(INT16 const sector, UINT8 const rank)
{
	return IsReinforceSector(sector) ? g_reinforce[rank] : 0;
}


static UINT32 ReinforceTotal()
{
	UINT32 n = 0;
	for (UINT8 const c : g_reinforce) n += c;
	return n;
}


// The arrival order's militia: the unassigned ones and those for the battle
static UINT8 const (&ArrivalOrderMilitia())[MAX_MILITIA_LEVELS]
{
	static UINT8 counts[MAX_MILITIA_LEVELS];
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		counts[rank] = (UINT8)std::min(CursorMilitia()[rank] + g_reinforce[rank], 255);
	}
	return counts;
}


// The unassigned militia go to the Deployment Center as a waiting order
static void SendCursorMilitiaToDeployment()
{
	if (g_arrival_order == 0 || CursorMilitiaTotal() == 0) return;
	SetMilitiaOrderCounts(g_arrival_order, CursorMilitia());
	SendMilitiaOrderToDeployment(g_arrival_order, CursorMilitia());
	g_arrival_sent += CursorMilitiaTotal();
	sGreensOnCursor = sRegularsOnCursor = sElitesOnCursor = sHeaviesOnCursor = 0;
	fMapPanelDirty = TRUE;
}


bool OpenMilitiaArrivalBox(UINT32 const order_id, INT16 const town, UINT8 const counts[])
{
	if (sSelectedMilitiaTown != 0 || gfMilitiaPopupCreated) return false; // the box is open
	if (fShowMapInventoryPool)                               return false;
	if (IsMilitiaRecruitPanelOpen() || IsMilitiaDeploymentPanelOpen()) return false;

	if (!fShowMilitia) ToggleShowMilitiaMode();
	sGreensOnCursor   = counts[GREEN_MILITIA];
	sRegularsOnCursor = counts[REGULAR_MILITIA];
	sElitesOnCursor   = counts[ELITE_MILITIA];
	sHeaviesOnCursor  = counts[ELITE_HEAVY_MILITIA];
	g_arrival_order      = order_id;
	g_arrival_back_to_tactical = g_arrival_jump_pending;
	g_arrival_jump_pending     = false;
	g_arrival_total      = CursorMilitiaTotal();
	g_arrival_sent       = 0;
	g_arrival_room       = 0;
	g_arrival_ours       = false;
	ForEachMilitiaBoxSector(town, [&](UINT8 const sector)
	{
		if (!SectorOursAndPeaceful(SGPSector(sector))) return;
		g_arrival_ours = true;
		UINT8 const n = CountAllMilitiaInSector(SGPSector(sector));
		if (n < MAX_ALLOWABLE_MILITIA_PER_SECTOR) g_arrival_room += MAX_ALLOWABLE_MILITIA_PER_SECTOR - n;
	});
	sSelectedMilitiaTown = town;
	fMapPanelDirty       = TRUE;
	return true;
}


INT16 MilitiaBoxKeyForSector(const SGPSector& sector)
{
	if (sector.z != 0) return 0;
	INT8 const town = GetTownIdForSector(sector);
	if (town != BLANK_SECTOR) return town;
	if (IsThisSectorASAMSector(sector) &&
			GetVObject(guiMilitiaMaps)->SubregionCount() > MILITIA_BOX_SAM_MAP)
	{
		return MILITIA_BOX_SAM_KEY + sector.AsByte();
	}
	return 0;
}


INT16 MilitiaArrivalBoxTown()
{
	return g_arrival_order != 0 ? sSelectedMilitiaTown : 0;
}


UINT32 MilitiaArrivalBoxOrder()
{
	return g_arrival_order;
}


void AddToMilitiaArrivalBox(UINT8 const counts[])
{
	if (g_arrival_order == 0) return;
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		GetPickedUpMilitia(rank) += counts[rank];
		g_arrival_total          += counts[rank];
	}
	SetMilitiaOrderCounts(g_arrival_order, ArrivalOrderMilitia());
	fMapPanelDirty = TRUE;
}


void RequestMilitiaArrivalBoxFromTactical()
{
	if (g_arrival_jump_pending || gfEnteringMapScreen) return;
	g_arrival_jump_pending = true;
	gfEnteringMapScreen    = TRUE;
}


void ResetMilitiaArrivalBox()
{
	g_arrival_jump_pending     = false;
	g_arrival_back_to_tactical = false;
	std::fill(std::begin(g_reinforce), std::end(g_reinforce), 0);
	g_arrival_order = 0;
	g_arrival_total = 0;
	g_arrival_sent  = 0;
}


// A town sector held by the enemy the militia box's militia can attack: put
// there, they attack it when the box is closed with Done (an autoresolve
// battle, as the militia orders' attack)
static bool MilitiaBoxSectorAttackable(UINT8 const sector)
{
	SGPSector const s(sector);
	if (NumEnemiesInSector(s) == 0)  return false;
	if (gfPreBattleInterfaceActive)  return false;
	// a battle in the loaded sector
	if (gfWorldLoaded && gWorldSector == s && gTacticalStatus.fEnemyInSector) return false;
	return true;
}


// The town sector the militia box's militia were put in to attack it, -1: none
static INT16 MilitiaBoxAttackSector(INT16 const town)
{
	INT16 attacked = -1;
	ForEachMilitiaBoxSector(town, [&](UINT8 const sector)
	{
		if (attacked != -1) return;
		if (SectorOursAndPeaceful(SGPSector(sector))) return;
		// the battle loaded in tactical (its militia are fighting there)
		if (gfWorldLoaded && gWorldSector.z == 0 && gWorldSector.AsByte() == sector && gTacticalStatus.fEnemyInSector) return;
		if (NumEnemiesInSector(SGPSector(sector)) == 0) return;
		if (CountAllMilitiaInSector(SGPSector(sector)) != 0) attacked = sector;
	});
	return attacked;
}


// Militia can be put in and taken from the town's sectors of ours with no
// enemy, and from those the enemy holds the militia can attack
static bool MilitiaBoxSectorUsable(UINT8 const sector)
{
	return SectorOursAndPeaceful(SGPSector(sector)) || MilitiaBoxSectorAttackable(sector) || IsReinforceSector(sector);
}


// function to manipulate the number of towns people on the cursor
static void PickUpATownPersonFromSector(UINT8 const type, UINT8 const sector)
{
	// Are they in the same town as they were picked up from?
	if (!MilitiaBoxHasSector(sSelectedMilitiaTown, sector)) return;

	if (!MilitiaBoxSectorUsable(sector)) return;

	// the battle's sector: only the militia put there for it (D5.2)
	if (IsReinforceSector(sector))
	{
		if (g_reinforce[type] == 0) return;
		--g_reinforce[type];
		++GetPickedUpMilitia(type);
		fMapPanelDirty = TRUE;
		return;
	}

	UINT8& n_type = SectorInfo[sector].ubNumberOfCivsAtLevel[type];
	// See if there are any militia of this type in this sector
	if (n_type == 0) return;

	--n_type;                   // Reduce number in this sector
	++GetPickedUpMilitia(type); // Pick this guy up
	fMapPanelDirty = TRUE;
	if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
}


static void DropAPersonInASector(UINT8 const type, UINT8 const sector)
{
	// Are they in the same town as they were picked up from?
	if (!MilitiaBoxHasSector(sSelectedMilitiaTown, sector)) return;

	if (!MilitiaBoxSectorUsable(sector)) return;

	// the battle's sector: as many as the battle can take (D5.2)
	if (IsReinforceSector(sector))
	{
		if (ReinforceTotal() >= MilitiaReinforcementRoom()) return;
		INT16& n_type = GetPickedUpMilitia(type);
		if (n_type == 0) return;
		--n_type;
		++g_reinforce[type];
		fMapPanelDirty = TRUE;
		return;
	}

	// one enemy-held sector attacked at a time
	if (!SectorOursAndPeaceful(SGPSector(sector)))
	{
		INT16 const attacked = MilitiaBoxAttackSector(sSelectedMilitiaTown);
		if (attacked != -1 && attacked != sector) return;
	}

	UINT8 (&n_milita)[MAX_MILITIA_LEVELS] = SectorInfo[sector].ubNumberOfCivsAtLevel;
	if (CountAllMilitiaInSector(SGPSector(sector)) >= MAX_ALLOWABLE_MILITIA_PER_SECTOR) return;

	// Drop the guy into this sector
	INT16& n_type = GetPickedUpMilitia(type);
	if (n_type == 0) return;

	--n_type;
	++n_milita[type]; // Up the number in this sector of this type of militia
	fMapPanelDirty = TRUE;
	if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
}


void LoadMapScreenInterfaceMapGraphics()
{
	// Suffix convention: _1280 for the compact strategic-screen tier (height
	// 720-767), _1024 for the large tier (height 768+) -- see
	// GetCharListGraphicsFilename() in MapScreen.cc for the same pattern.
	char const* const bigMapFile = g_ui.isCompactStrategicScreen()
	                                      ? INTERFACEDIR "/b_map_1280.pcx"
	                                      : INTERFACEDIR "/b_map_1024.pcx";
	guiBIGMAP                      = AddVideoSurfaceFromFile(bigMapFile);
	// The map is drawn shaded and halved through its palette (BltVideoSurfaceHalf(),
	// InitializePalettesForMap()), so a replacement PNG must be palettised too.
	if (guiBIGMAP->BPP() != 8)
	{
		throw std::runtime_error(ST::format("{}: the strategic map needs an 8 bit palettised image, a PNG replacing it must be palettised", bigMapFile).to_std_string());
	}

	for (auto s : GCM->getMapSecrets())
	{
		const ST::string& path = s->secretMapIcon;
		if (!path.empty() && gSecretSiteIcons.find(path) == gSecretSiteIcons.end())
		{
			gSecretSiteIcons[path] = AddVideoObjectFromFile(path);
		}
	}

	InitializePalettesForMap(guiBIGMAP->GetPalette());
}


void DeleteMapScreenInterfaceMapGraphics()
{
	DeleteVideoSurface(guiBIGMAP);
	RemoveVObject(guiBULLSEYE);
	RemoveVObject(guiSAMICON);
	RemoveVObject(guiCHARBETWEENSECTORICONS);
	RemoveVObject(guiCHARBETWEENSECTORICONSCLOSE);
	RemoveVObject(guiCHARICONS);
	RemoveVObject(guiHelicopterIcon);
	RemoveVObject(guiVehicleCursorIcon);
	RemoveVObject(guiWalkingCursorIcon);
	RemoveVObject(guiMAPCURSORS);
	RemoveVObject(guiMINEICON);
	RemoveVObject(guiMapBorderHeliSectorsFirst);
	RemoveVObject(guiMapBorderHeliSectorsSecond);
	RemoveVObject(guiMilitia);
	RemoveVObject(guiMilitiaMaps);
	RemoveVObject(guiMilitiaSectorHighLight);
	RemoveVObject(guiMilitiaSectorOutline);
	RemoveVObject(GetMineLevelGraphicsFilename(1));
	RemoveVObject(GetMineLevelGraphicsFilename(2));
	RemoveVObject(GetMineLevelGraphicsFilename(3));

	for (auto& pair : gSecretSiteIcons)
	{
		DeleteVideoObject(pair.second);
	}
	gSecretSiteIcons.clear();

	ShutDownPalettesForMap();
}


static void CheckAndUpdateStatesOfSelectedMilitiaSectorButtons();
static void DisplayUnallocatedMilitia(void);
static void DrawMilitiaArrivalText();
static void DrawTownMilitiaName();
static void RenderIconsPerSectorForSelectedTown(void);
static void RenderShadingForUnControlledSectors(void);
static void SetMilitiaMapButtonsText(void);
static void ShowHighLightedSectorOnMilitiaMap(void);


void DrawMilitiaPopUpBox()
{
	if( !fShowMilitia )
	{
		sSelectedMilitiaTown = 0;
	}

	// create buttons
	CreateDestroyMilitiaSectorButtons( );

	// create mouse regions if we need to
	CreateDestroyMilitiaPopUPRegions( );

	if (!sSelectedMilitiaTown) return;

	// the arrival mode: the order holds exactly the unassigned militia
	if (g_arrival_order != 0) SetMilitiaOrderCounts(g_arrival_order, ArrivalOrderMilitia());

	// update states of militia selected sector buttons
	CheckAndUpdateStatesOfSelectedMilitiaSectorButtons();

	BltVideoObject(FRAME_BUFFER, guiMilitia,     MilitiaBox().panel,       MAP_MILITIA_BOX_POS_X,                     MAP_MILITIA_BOX_POS_Y);
	UINT16 const map_frame = IsSamBoxKey(sSelectedMilitiaTown) ? MILITIA_BOX_SAM_MAP : sSelectedMilitiaTown - 1;
	BltVideoObject(FRAME_BUFFER, guiMilitiaMaps, map_frame,                MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X, MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y);

	// set font color for labels and "total militia" counts
	SetFontForeground(MilitiaBox().colour);

	// draw name of town, and the "unassigned" label
	DrawTownMilitiaName();

	// render the icons for each sector in the town
	RenderIconsPerSectorForSelectedTown( );

	// shade any sectors not under our control
	RenderShadingForUnControlledSectors( );

	// display anyone picked up
	DisplayUnallocatedMilitia( );

	// the arrival mode's text box
	DrawMilitiaArrivalText();

	// draw the highlight last
	ShowHighLightedSectorOnMilitiaMap( );

	ETRLEObject const& pTrav = GetVObject(guiMilitia)->SubregionProperties(MilitiaBox().panel);
	InvalidateRegion(MAP_MILITIA_BOX_POS_X, MAP_MILITIA_BOX_POS_Y, MAP_MILITIA_BOX_POS_X + pTrav.usWidth, MAP_MILITIA_BOX_POS_Y + pTrav.usHeight);

	// set the text for the militia map sector info buttons
	SetMilitiaMapButtonsText( );
	if (fMilitiaMapButtonsCreated)
	{
		for (INT32 i = 0; i != MAX_MILITIA_LEVELS; ++i) ReleaseButtonPressedLook(giMapMilitiaButton[i]);
	}

	// render buttons
	MarkButtonsDirty( );
}


static void CreateMilitiaPanelBottomButton(void);
static void DeleteMilitiaPanelBottomButton(void);
static void HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor(INT16 sTownValue);
static void MilitiaRegionClickCallbackPrimary(MOUSE_REGION*, UINT32 reason);
static void MilitiaRegionClickCallbackSecondary(MOUSE_REGION*, UINT32 reason);
static void MilitiaRegionMoveCallback(MOUSE_REGION*, UINT32 reason);


void CreateDestroyMilitiaPopUPRegions(void)
{
	static INT16 sOldTown = 0;

	// create destroy militia pop up regions for mapscreen militia pop up box
	if (sSelectedMilitiaTown != 0)
	{
		sOldTown = sSelectedMilitiaTown;
	}

	if (fShowMilitia && sSelectedMilitiaTown && !gfMilitiaPopupCreated)
	{
		{
			ETRLEObject const& panel = GetVObject(guiMilitia)->SubregionProperties(MilitiaBox().panel);
			UINT16 const x = MAP_MILITIA_BOX_POS_X;
			UINT16 const y = MAP_MILITIA_BOX_POS_Y;
			MSYS_DefineRegion(&gMapScreenMilitiaBoxBlanket, x, y, x + panel.usWidth, y + panel.usHeight, MSYS_PRIORITY_HIGHEST - 3, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);
		}
		for (INT32 i = 0; i < 9; ++i)
		{
			MOUSE_REGION* const r = &gMapScreenMilitiaBoxRegions[i];
			UINT16        const x = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + i % MILITIA_BOX_ROWS * MILITIA_BOX_BOX_WIDTH;
			UINT16        const y = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + i / MILITIA_BOX_ROWS * MILITIA_BOX_BOX_HEIGHT;
			MSYS_DefineRegion(r, x, y, x + MILITIA_BOX_BOX_WIDTH, y + MILITIA_BOX_BOX_HEIGHT, MSYS_PRIORITY_HIGHEST - 2, MSYS_NO_CURSOR, MilitiaRegionMoveCallback, MouseCallbackPrimarySecondary(MilitiaRegionClickCallbackPrimary, MilitiaRegionClickCallbackSecondary));
			MSYS_SetRegionUserData(r, 0, i);
		}

		CreateMilitiaPanelBottomButton();
		gfMilitiaPopupCreated = TRUE;
	}
	else if (gfMilitiaPopupCreated  && (!fShowMilitia || !sSelectedMilitiaTown))
	{
		for (INT32 i = 0; i < 9; ++i)
		{
			MSYS_RemoveRegion(&gMapScreenMilitiaBoxRegions[i]);
		}
		MSYS_RemoveRegion(&gMapScreenMilitiaBoxBlanket);

		// militia put in an enemy-held sector: closed with Done, they attack it;
		// closed any other way (or the attack not possible any more), they go
		// back to the cursor and from there over the town's sectors of ours
		INT16 const attacked = MilitiaBoxAttackSector(sOldTown);
		bool  const fDone    = g_militia_box_done;
		bool  const fAttack  = attacked != -1 && fDone && MilitiaBoxSectorAttackable(attacked);
		g_militia_box_done = false;
		if (attacked != -1 && !fAttack)
		{
			UINT8 (&n)[MAX_MILITIA_LEVELS] = SectorInfo[attacked].ubNumberOfCivsAtLevel;
			for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
			{
				GetPickedUpMilitia(rank) += n[rank];
				n[rank] = 0;
			}
		}

		// the arrival mode: what is unassigned goes to the Deployment Center
		// (closed other than with Done, D3), the order is done
		bool back_to_tactical = false;
		if (g_arrival_order != 0)
		{
			// militia for the battle in tactical (D5.2): closed with Done they
			// join it back on the tactical screen (their own waiting order, the
			// loyalty counted then); otherwise they go to the Deployment Center
			if (ReinforceTotal() != 0)
			{
				SetMilitiaOrderCounts(g_arrival_order, ArrivalOrderMilitia());
				UINT32 const id = SendMilitiaOrderToDeployment(g_arrival_order, g_reinforce, !fDone);
				if (fDone && id != 0) JoinMilitiaOrderToBattle(id);
				g_arrival_sent += ReinforceTotal();
				std::fill(std::begin(g_reinforce), std::end(g_reinforce), 0);
			}
			SendCursorMilitiaToDeployment();
			FinishMilitiaArrivalOrder(g_arrival_order, g_arrival_total > g_arrival_sent ? g_arrival_total - g_arrival_sent : 0);
			back_to_tactical = g_arrival_back_to_tactical;
			ResetMilitiaArrivalBox();
		}

		// handle the shutdown of the panel...there maybe people on the cursor, distribute them evenly over all the sectors
		HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor(sOldTown);

		DeleteMilitiaPanelBottomButton();
		gfMilitiaPopupCreated = FALSE;

		if (fAttack)
		{
			gfStrategicMilitiaChangesMade = TRUE;
			StartMilitiaAttackBattle(SGPSector(attacked));
		}
		else if (back_to_tactical && guiCurrentScreen == MAP_SCREEN)
		{
			// the box opened for an arrival on the tactical screen (D5.1)
			RequestTriggerExitFromMapscreen(MAP_EXIT_TO_TACTICAL);
		}
	}
}


static INT16 GetBaseSectorForCurrentTown(void);


static void RenderIconsPerSectorForSelectedTown(void)
{
	// render icons for map
	INT16 const sBaseSectorValue = GetBaseSectorForCurrentTown();
	for (INT32 iCounter = 0; iCounter != 9; ++iCounter)
	{
		INT32 const dx     = iCounter % MILITIA_BOX_ROWS;
		INT32 const dy     = iCounter / MILITIA_BOX_ROWS;
		INT16 const sector = sBaseSectorValue + dx + dy * 16;

		// skip sectors not in the selected town (nearby other towns or wilderness SAM Sites)
		if (!MilitiaBoxHasSector(sSelectedMilitiaTown, sector)) continue;

		// get number of each
		SECTORINFO const& si         = SectorInfo[sector];
		INT32      const  n_greens   = si.ubNumberOfCivsAtLevel[GREEN_MILITIA]       + Reinforcing(sector, GREEN_MILITIA);
		INT32      const  n_regulars = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA]     + Reinforcing(sector, REGULAR_MILITIA);
		INT32      const  n_elites   = si.ubNumberOfCivsAtLevel[ELITE_MILITIA]       + Reinforcing(sector, ELITE_MILITIA);
		INT32      const  n_heavies  = si.ubNumberOfCivsAtLevel[ELITE_HEAVY_MILITIA] + Reinforcing(sector, ELITE_HEAVY_MILITIA);
		INT32      const  n_total    = n_greens + n_regulars + n_elites + n_heavies;

		StrategicMapElement const& e = StrategicMap[SGPSector(sector).AsStrategicIndex()];
		MilitiaBoxLayout const& box = MilitiaBox();
		if (!e.fEnemyControlled || n_total != 0)
		{
			// print number of troops (the small layout's rank buttons cover the
			// selected sector's right side)
			SetFont(box.font);
			ST::string sString = ST::format("{}", n_total);
			INT16       sX;
			INT16       sY;
			INT16 const x  = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + dx * MILITIA_BOX_BOX_WIDTH;
			INT16 const y  = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + dy * MILITIA_BOX_BOX_HEIGHT;
			if (box.big)
			{
				FindFontRightCoordinates(x, y, MILITIA_BOX_BOX_WIDTH - 4, 0, sString, box.font, &sX, &sY);
				MPrint(sX, y + MILITIA_BOX_BOX_HEIGHT - 4 - GetFontHeight(box.font), sString);
			}
			else
			{
				FindFontRightCoordinates(x, y, MILITIA_BOX_BOX_WIDTH, 0, sString, FONT10ARIAL, &sX, &sY);
				MPrint(sSectorMilitiaMapSector != iCounter ? sX : sX - 15, sY + MILITIA_BOX_BOX_HEIGHT - 5, sString);
			}
		}

		// now display
		for (INT32 i = 0; i < n_total; ++i)
		{
			// get screen x and y coords
			INT16  x = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + dx * MILITIA_BOX_BOX_WIDTH;
			INT16  y = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + dy * MILITIA_BOX_BOX_HEIGHT;
			if (box.big)
			{
				// the 11x11 squares, also in the selected sector
				x += i % POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING + 4;
				y += i / POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING + 4;
				UINT16 const big_icon =
					i < n_greens                         ? BIG_MILITIA_ICON_FIRST + GREEN_MILITIA   :
					i < n_greens + n_regulars            ? BIG_MILITIA_ICON_FIRST + REGULAR_MILITIA :
					i < n_greens + n_regulars + n_elites ? BIG_MILITIA_ICON_FIRST + ELITE_MILITIA   :
					BIG_MILITIA_ICON_FIRST + ELITE_HEAVY_MILITIA;
				BltVideoObject(FRAME_BUFFER, guiMilitia, big_icon, x, y);
				continue;
			}
			UINT16 icon_base;
			if (sSectorMilitiaMapSector == iCounter)
			{
				x         += i % POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING       + 2;
				y         += i / POPUP_MILITIA_ICONS_PER_ROW * (MEDIUM_MILITIA_ICON_SPACING - 1) + 3;
				icon_base  = 5;
			}
			else
			{
				x         += i % POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING + 3;
				y         += i / POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING + 3;
				icon_base  = 8;
			}

			UINT16 const icon =
				i < n_greens                         ? icon_base     :
				i < n_greens + n_regulars            ? icon_base + 1 :
				i < n_greens + n_regulars + n_elites ? icon_base + 2 :
				HeavyMilitiaIcon(icon_base + 2);
			BltVideoObject(FRAME_BUFFER, guiMilitia, icon, x, y);
		}
	}
}


// Get the sector value for the upper left corner
static INT16 GetBaseSectorForCurrentTown(void)
{
	INT16 sBaseSector = 0;

	// is the current town
	if (IsSamBoxKey(sSelectedMilitiaTown))
	{
		// the SAM site in the middle
		sBaseSector = SamBoxSector(sSelectedMilitiaTown) - 1 - 16;
	}
	else if( sSelectedMilitiaTown != 0 )
	{
		sBaseSector = sBaseSectorList[sSelectedMilitiaTown - 1].AsByte();
	}

	// return the current sector value
	return( sBaseSector );
}


static void ShowHighLightedSectorOnMilitiaMap(void)
{
	// show the highlighted sector on the militia map
	INT16 sX = 0, sY = 0;

	// the big layout's sectors are larger than the outline graphics: as them,
	// the selected sector yellow, the one under the mouse white, drawn 2 px wide
	bool const big = MilitiaBox().big;
	auto const frame = [](INT16 const x, INT16 const y, INT16 const w, INT16 const h, UINT32 const rgb)
	{
		UINT16 const c = Get16BPPColor(rgb);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x,         y,         x + w,     y + 2,     c);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x,         y + h - 2, x + w,     y + h,     c);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x,         y,         x + 2,     y + h,     c);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x + w - 2, y,         x + w,     y + h,     c);
	};

	if( sSectorMilitiaMapSector != -1 )
	{
		sX = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + ( ( sSectorMilitiaMapSector % MILITIA_BOX_ROWS ) * MILITIA_BOX_BOX_WIDTH );
		sY = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + ( ( sSectorMilitiaMapSector / MILITIA_BOX_ROWS ) * MILITIA_BOX_BOX_HEIGHT );
		if (big) frame(sX, sY, MILITIA_BOX_BOX_WIDTH, MILITIA_BOX_BOX_HEIGHT, FROMRGB(241, 255, 11));
		else     BltVideoObject(FRAME_BUFFER, guiMilitiaSectorHighLight, 0, sX, sY);
	}

	if( sSectorMilitiaMapSectorOutline != -1 )
	{
		sX = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + ( ( sSectorMilitiaMapSectorOutline % MILITIA_BOX_ROWS ) * MILITIA_BOX_BOX_WIDTH );
		sY = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + ( ( sSectorMilitiaMapSectorOutline / MILITIA_BOX_ROWS ) * MILITIA_BOX_BOX_HEIGHT );
		if (big) frame(sX, sY, MILITIA_BOX_BOX_WIDTH, MILITIA_BOX_BOX_HEIGHT, FROMRGB(255, 255, 255));
		else     BltVideoObject(FRAME_BUFFER, guiMilitiaSectorOutline, 0, sX, sY);
	}
}


static bool IsThisMilitiaTownSectorAllowable(INT16 sSectorIndexValue);


static void MilitiaRegionClickCallbackPrimary(MOUSE_REGION* const r, UINT32 const reason)
{
	INT16 const val = MSYS_GetRegionUserData(r, 0);
	sSectorMilitiaMapSector =
		!IsThisMilitiaTownSectorAllowable(val) ? -1 :
		sSectorMilitiaMapSector == val         ? -1 :
		val;
}

static void MilitiaRegionClickCallbackSecondary(MOUSE_REGION* const r, UINT32 const reason)
{
	sSectorMilitiaMapSector = -1;
}


static void MilitiaRegionMoveCallback(MOUSE_REGION* const r, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE)
	{
		INT16 const val = MSYS_GetRegionUserData(r, 0);
		sSectorMilitiaMapSectorOutline =
			!IsThisMilitiaTownSectorAllowable(val) ? -1 :
			val;
	}
	else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		sSectorMilitiaMapSectorOutline = -1;
	}
}


static void MilitiaButtonCallbackPrimary(GUI_BUTTON* btn, UINT32 reason);
static void MilitiaButtonCallbackSecondary(GUI_BUTTON* btn, UINT32 reason);
static void MilitiaUpArrowCallback(GUI_BUTTON* btn, UINT32 reason);
static void MilitiaDownArrowCallback(GUI_BUTTON* btn, UINT32 reason);


void CreateDestroyMilitiaSectorButtons()
{
	static INT16 sOldSectorValue = -1;

	if (!fMilitiaMapButtonsCreated && sOldSectorValue == sSectorMilitiaMapSector && fShowMilitia && sSelectedMilitiaTown && sSectorMilitiaMapSector != -1)
	{
		// given sector..place down the 4 buttons: the small layout's one under the
		// other in the sector (the 4th, ELITE_HEAVY, reaches below the sector box),
		// the big layout's in a row under the unassigned militia
		MilitiaBoxLayout const& box = MilitiaBox();
		INT16 x = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + sSectorMilitiaMapSector % MILITIA_BOX_ROWS * MILITIA_BOX_BOX_WIDTH  + MILITIA_BTN_OFFSET_X;
		INT16 y = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + sSectorMilitiaMapSector / MILITIA_BOX_ROWS * MILITIA_BOX_BOX_HEIGHT + 2;
		if (box.big)
		{
			x = MAP_MILITIA_BOX_POS_X + box.rank_btn_x;
			y = MAP_MILITIA_BOX_POS_Y + box.rank_btn_y;
		}
		for (INT32 i = 0; i != MAX_MILITIA_LEVELS; ++i)
		{
			GUIButtonRef b = QuickCreateButtonImg(INTERFACEDIR "/militia.sti", 3, 4, x, y, MSYS_PRIORITY_HIGHEST - 1,
				ButtonCallbackPrimarySecondary(MilitiaButtonCallbackPrimary, MilitiaButtonCallbackSecondary, ShowButtonPressedOnRightClick));
			if (box.big) x += box.rank_btn_dx;
			else         y += MILITIA_BTN_HEIGHT;
			giMapMilitiaButton[i] = b;
			b->SetUserData(i);
			// the rank's colour
			b->SpecifyGeneralTextAttributes({}, box.font, box.big ? gsBigMilitiaSectorButtonColors[i] : gsMilitiaSectorButtonColors[i], FONT_BLACK);
			b->SpecifyTextSubOffsets(0, 0, TRUE);
			b->fShiftText = FALSE;
			if (box.arrow_up_y != 0)
			{
				// the arrows move the militia: the rank button only shows the
				// sector's number -- clicks do nothing (nor go through it), it
				// doesn't look pressed, no help text
				b->ClickCallback = [](GUI_BUTTON*, UINT32) {}; // not empty: an empty one toggles the pressed look
				b->MoveCallback  = GUI_CALLBACK();

				INT16 const ax = b->X() + box.arrow_dx;
				GUIButtonRef const up = QuickCreateButtonImg(MILITIA_ARROWS_FILE, 0, 1, ax, MAP_MILITIA_BOX_POS_Y + box.arrow_up_y, MSYS_PRIORITY_HIGHEST - 1, MilitiaUpArrowCallback);
				up->SetUserData(i);
				g_militia_up_arrow[i] = up;
				GUIButtonRef const down = QuickCreateButtonImg(MILITIA_ARROWS_FILE, 2, 3, ax, MAP_MILITIA_BOX_POS_Y + box.arrow_down_y, MSYS_PRIORITY_HIGHEST - 1, MilitiaDownArrowCallback);
				down->SetUserData(i);
				g_militia_down_arrow[i] = down;
			}
			else
			{
				b->SetFastHelpText(i == ELITE_HEAVY_MILITIA ? pMilitiaHeavyStrings[0] : pMilitiaButtonsHelpText[i]);
			}
		}

		CreateScreenMaskForMoveBox();

		// Set the fact that the buttons are created
		fMilitiaMapButtonsCreated = true;
	}
	else if (fMilitiaMapButtonsCreated && (sOldSectorValue != sSectorMilitiaMapSector || !fShowMilitia || !sSelectedMilitiaTown || sSectorMilitiaMapSector == -1))
	{
		sOldSectorValue = sSectorMilitiaMapSector;

		// get rid of the buttons
		for (INT32 i = 0; i != MAX_MILITIA_LEVELS; ++i)
		{
			RemoveButton(giMapMilitiaButton[i]);
			if (g_militia_up_arrow[i])   RemoveButton(g_militia_up_arrow[i]);
			if (g_militia_down_arrow[i]) RemoveButton(g_militia_down_arrow[i]);
		}

		if (!fShowMilitia || !sSelectedMilitiaTown)
		{
			sSectorMilitiaMapSector = -1;
			sSelectedMilitiaTown    =  0;
		}

		RemoveScreenMaskForMoveBox();

		// Set the fact that the buttons are destroyed
		fMilitiaMapButtonsCreated = false;
	}

	sOldSectorValue = sSectorMilitiaMapSector;
}


static void SetMilitiaMapButtonsText()
{
	if (!fMilitiaMapButtonsCreated) return;

	// grab the appropriate global sector value in the world
	INT16      const  base_sector = GetBaseSectorForCurrentTown();
	INT16      const  sector      = base_sector + (sSectorMilitiaMapSector % MILITIA_BOX_ROWS + sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16);
	SECTORINFO const& si          = SectorInfo[sector];
	ST::string buf;

	// the greens in this sector
	buf = ST::format("{}", si.ubNumberOfCivsAtLevel[GREEN_MILITIA] + Reinforcing(sector, GREEN_MILITIA));
	giMapMilitiaButton[0]->SpecifyText(buf);

	// the regulars in this sector
	buf = ST::format("{}", si.ubNumberOfCivsAtLevel[REGULAR_MILITIA] + Reinforcing(sector, REGULAR_MILITIA));
	giMapMilitiaButton[1]->SpecifyText(buf);

	// the number of elites in this sector
	buf = ST::format("{}", si.ubNumberOfCivsAtLevel[ELITE_MILITIA] + Reinforcing(sector, ELITE_MILITIA));
	giMapMilitiaButton[2]->SpecifyText(buf);

	// the number of elite heavies in this sector
	buf = ST::format("{}", si.ubNumberOfCivsAtLevel[ELITE_HEAVY_MILITIA] + Reinforcing(sector, ELITE_HEAVY_MILITIA));
	giMapMilitiaButton[3]->SpecifyText(buf);
}


static void MilitiaButtonCallbackPrimary(GUI_BUTTON *btn, UINT32 reason)
{
	INT32 const iValue = btn->GetUserData();

	// get the sector value for the upper left corner
	INT16 sBaseSectorValue = GetBaseSectorForCurrentTown();
	INT16 sGlobalMapSector = sBaseSectorValue + sSectorMilitiaMapSector % MILITIA_BOX_ROWS + sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16;

	DropAPersonInASector(iValue, sGlobalMapSector);
}

static void MilitiaButtonCallbackSecondary(GUI_BUTTON *btn, UINT32 reason)
{
	INT32 const iValue = btn->GetUserData();

	// get the sector value for the upper left corner
	INT16 sBaseSectorValue = GetBaseSectorForCurrentTown();
	INT16 sGlobalMapSector = sBaseSectorValue + sSectorMilitiaMapSector % MILITIA_BOX_ROWS + sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16;

	PickUpATownPersonFromSector(iValue, sGlobalMapSector);
}


// the big box's arrows: up puts one of the rank from the cursor into the
// selected sector (as the rank button's left click), down takes one out onto
// the cursor (its right click)
static void MilitiaUpArrowCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	INT16 const sector = GetBaseSectorForCurrentTown() + sSectorMilitiaMapSector % MILITIA_BOX_ROWS + sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16;
	DropAPersonInASector(btn->GetUserData(), sector);
}


static void MilitiaDownArrowCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	INT16 const sector = GetBaseSectorForCurrentTown() + sSectorMilitiaMapSector % MILITIA_BOX_ROWS + sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16;
	PickUpATownPersonFromSector(btn->GetUserData(), sector);
}


static void DisplayUnallocatedMilitia(void)
{
	// show the nunber on the cursor
	INT32 iTotalNumberOfTroops =0, iNumberOfGreens = 0, iNumberOfRegulars =0, iNumberOfElites = 0, iNumberOfHeavies = 0, iCurrentTroopIcon = 0;
	INT32 iCurrentIcon = 0;
	INT16 sX = 0, sY = 0;

	// get number of each
	iNumberOfGreens = sGreensOnCursor;
	iNumberOfRegulars = sRegularsOnCursor;
	iNumberOfElites = sElitesOnCursor;
	iNumberOfHeavies = sHeaviesOnCursor;

	// get total
	iTotalNumberOfTroops = iNumberOfGreens + iNumberOfRegulars + iNumberOfElites + iNumberOfHeavies;

	// now display
	MilitiaBoxLayout const& box = MilitiaBox();
	for( iCurrentTroopIcon = 0; iCurrentTroopIcon < iTotalNumberOfTroops; iCurrentTroopIcon++ )
	{
		// get screen x and y coords
		sX =  ( iCurrentTroopIcon % NUMBER_OF_MILITIA_ICONS_PER_LOWER_ROW ) * MEDIUM_MILITIA_ICON_SPACING + MAP_MILITIA_BOX_POS_X + box.lower_x;
		sY =  ( iCurrentTroopIcon / NUMBER_OF_MILITIA_ICONS_PER_LOWER_ROW ) * MEDIUM_MILITIA_ICON_SPACING + MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_LOWER_ROW_Y;

		if( iCurrentTroopIcon < iNumberOfGreens )
		{
			iCurrentIcon = 8;
		}
		else if( iCurrentTroopIcon < iNumberOfGreens + iNumberOfRegulars )
		{
			iCurrentIcon = 9;
		}
		else if( iCurrentTroopIcon < iNumberOfGreens + iNumberOfRegulars + iNumberOfElites )
		{
			iCurrentIcon = 10;
		}
		else
		{
			iCurrentIcon = HeavyMilitiaIcon(10);
		}
		// the big layout's squares: 15-18 instead of 8-10, 14
		if (box.big)
		{
			iCurrentIcon =
				iCurrentTroopIcon < iNumberOfGreens                                        ? BIG_MILITIA_ICON_FIRST + GREEN_MILITIA   :
				iCurrentTroopIcon < iNumberOfGreens + iNumberOfRegulars                    ? BIG_MILITIA_ICON_FIRST + REGULAR_MILITIA :
				iCurrentTroopIcon < iNumberOfGreens + iNumberOfRegulars + iNumberOfElites  ? BIG_MILITIA_ICON_FIRST + ELITE_MILITIA   :
				BIG_MILITIA_ICON_FIRST + ELITE_HEAVY_MILITIA;
		}

		BltVideoObject(FRAME_BUFFER, guiMilitia, iCurrentIcon, sX, sY);
	}

	// the big box with arrows: the unassigned militia's numbers per rank, on
	// the rank button's graphic, in the rank's colour, as the rank buttons
	if (box.counter_y != 0)
	{
		ETRLEObject const& g = GetVObject(guiMilitia)->SubregionProperties(MILITIA_RANK_BUTTON_GRAPHIC);
		SetFont(box.font);
		SetFontBackground(FONT_MCOLOR_BLACK);
		SetFontShadow(NO_SHADOW);
		for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
		{
			INT16 const x = MAP_MILITIA_BOX_POS_X + box.rank_btn_x + rank * box.rank_btn_dx;
			INT16 const y = MAP_MILITIA_BOX_POS_Y + box.counter_y;
			BltVideoObject(FRAME_BUFFER, guiMilitia, MILITIA_RANK_BUTTON_GRAPHIC, x, y);
			ST::string const n = ST::format("{}", GetPickedUpMilitia(rank));
			FindFontCenterCoordinates(x, y, g.usWidth, g.usHeight, n, box.font, &sX, &sY);
			SetFontForeground(gsBigMilitiaSectorButtonColors[rank]);
			MPrint(sX, sY, n);
		}
		SetFontShadow(DEFAULT_SHADOW);
		SetFontForeground(box.colour);
	}
}


// Is this sector allowed to be clicked on?
static bool IsThisMilitiaTownSectorAllowable(INT16 const sSectorIndexValue)
{
	INT16 const base_sector = GetBaseSectorForCurrentTown();
	INT16 const sector      = base_sector + sSectorIndexValue % MILITIA_BOX_ROWS + sSectorIndexValue / MILITIA_BOX_ROWS * 16;
	SGPSector sec(sector);
	return MilitiaBoxHasSector(sSelectedMilitiaTown, sector) &&
		MilitiaBoxSectorUsable(sector);
}


// The box's name: the town's, or "SAM Site D15"
static ST::string MilitiaBoxName()
{
	return IsSamBoxKey(sSelectedMilitiaTown) ?
		ST::format("{} {}", pRepairStrings[1], SGPSector(SamBoxSector(sSelectedMilitiaTown)).AsShortString()) :
		GCM->getTownName(sSelectedMilitiaTown);
}


// The arrival mode's text box (docs/militia-arrival-box-plan.md, D1, D1b):
// the header after the room when the box opened, "Choose which militia:",
// then the choices there are -- available sector(s) with a sector of ours,
// recapture with an enemy-held sector the militia can attack (the mercs
// liberated it once), the Deployment Center always
static void DrawMilitiaArrivalText()
{
	MilitiaBoxLayout const& box = MilitiaBox();
	if (g_arrival_order == 0 || box.deploy_y == 0) return;
	if (GetVObject(guiMilitia)->SubregionProperties(box.panel).usHeight < MILITIA_TEXT_Y + MILITIA_TEXT_H) return;

	bool const sam = IsSamBoxKey(sSelectedMilitiaTown);
	bool recapture = false;
	ForEachMilitiaBoxSector(sSelectedMilitiaTown, [&](UINT8 const sector)
	{
		if (SectorOursAndPeaceful(SGPSector(sector))) return;
		if (MilitiaBoxSectorAttackable(sector) && SectorInfo[sector].fSurfaceWasEverPlayerControlled) recapture = true;
	});

	ST::string const name = MilitiaBoxName();
	ST::string header;
	if (!g_arrival_ours && sam)                header = st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ARRIVAL_HEADER_OCCUPIED], name);
	else if (sam || g_arrival_room >= g_arrival_total) header = st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ARRIVAL_HEADER], name);
	else                                       header = st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ARRIVAL_HEADER_ROOM], name, int(g_arrival_room), int(g_arrival_total));

	std::vector<ST::string> lines;
	for (ST::utf32_buffer const& l : LineWrap(MILITIA_TEXT_FONT, MILITIA_TEXT_W, header)) lines.push_back(ST::string::from_utf32(l));
	lines.push_back(pMilitiaRecruitStrings[MILITIA_STR_ARRIVAL_CHOOSE]);
	if (g_arrival_ours) lines.push_back(pMilitiaRecruitStrings[sam ? MILITIA_STR_ARRIVAL_SECTOR : MILITIA_STR_ARRIVAL_SECTORS]);
	if (recapture)      lines.push_back(pMilitiaRecruitStrings[MILITIA_STR_ARRIVAL_RECAPTURE]);
	lines.push_back(pMilitiaRecruitStrings[MILITIA_STR_ARRIVAL_DEPLOYMENT]);

	INT16 const block_h = INT16((lines.size() - 1) * MILITIA_TEXT_PITCH + GetFontHeight(MILITIA_TEXT_FONT));
	INT16       y       = MAP_MILITIA_BOX_POS_Y + MILITIA_TEXT_Y + std::max<INT16>(0, (MILITIA_TEXT_H - block_h) / 2);
	SetFont(MILITIA_TEXT_FONT);
	SetFontForeground(box.colour);
	SetFontBackground(FONT_MCOLOR_BLACK);
	SetFontShadow(DEFAULT_SHADOW);
	for (ST::string const& l : lines)
	{
		MPrint(MAP_MILITIA_BOX_POS_X + MILITIA_TEXT_X, y, l);
		y += MILITIA_TEXT_PITCH;
	}
}


static void DrawTownMilitiaName()
{
	ST::string town = MilitiaBoxName();
	MilitiaBoxLayout const& box = MilitiaBox();
	INT16          const x    = MAP_MILITIA_BOX_POS_X + box.title_x;
	INT16          const y    = MAP_MILITIA_BOX_POS_Y;
	INT16          const w    = MILITIA_BOX_WIDTH;
	ST::string buf;
	INT16                sX;
	INT16                sY;

	// get the name for the current militia town
	SetFont(box.font);
	buf = ST::format("{} {}", town, pMilitiaString[0]);
	FindFontCenterCoordinates(x, y + MILITIA_BOX_TEXT_OFFSET_Y, w, MILITIA_BOX_TEXT_TITLE_HEIGHT, buf, box.font, &sX, &sY);
	MPrint(sX, sY, buf);

	// might as well show the unassigned string (the big layout: next to the
	// rank buttons, the town is in the title)
	buf = box.big ? pMilitiaString[1] : ST::format("{} {}", town, pMilitiaString[1]);
	INT16 const uh = box.unassigned_h ? box.unassigned_h : GetFontHeight(box.font);
	FindFontCenterCoordinates(MAP_MILITIA_BOX_POS_X + box.unassigned_x, y + box.unassigned_y, box.unassigned_w, uh, buf, box.font, &sX, &sY);
	SetFontShadow(DEFAULT_SHADOW); // with a shadow
	MPrint(sX, sY, buf);
}


static void HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor(INT16 const town)
{
	// check if anyone still on the cursor
	if (sGreensOnCursor == 0 && sRegularsOnCursor == 0 && sElitesOnCursor == 0 && sHeaviesOnCursor == 0) return;

	std::vector<UINT8> box_sectors;
	ForEachMilitiaBoxSector(town, [&](UINT8 const sector) { box_sectors.push_back(sector); });
	for (UINT8 const sector : box_sectors)
	{
		if (!SectorOursAndPeaceful(SGPSector(sector))) continue;
		SECTORINFO& si        = SectorInfo[sector];
		UINT8&      n_green   = si.ubNumberOfCivsAtLevel[GREEN_MILITIA];
		UINT8&      n_regular = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA];
		UINT8&      n_elite   = si.ubNumberOfCivsAtLevel[ELITE_MILITIA];
		UINT8&      n_heavy   = si.ubNumberOfCivsAtLevel[ELITE_HEAVY_MILITIA];
		INT32       n         = MAX_ALLOWABLE_MILITIA_PER_SECTOR - n_green - n_regular - n_elite - n_heavy;
		if (n == 0) continue;

		while (sGreensOnCursor != 0 || sRegularsOnCursor != 0 || sElitesOnCursor != 0 || sHeaviesOnCursor != 0)
		{
			if (sGreensOnCursor != 0)
			{
				++n_green;
				--sGreensOnCursor;
				if (--n == 0) break;
			}

			if (sRegularsOnCursor != 0)
			{
				++n_regular;
				--sRegularsOnCursor;
				if (--n == 0) break;
			}

			if (sElitesOnCursor != 0)
			{
				++n_elite;
				--sElitesOnCursor;
				if (--n == 0) break;
			}

			if (sHeaviesOnCursor != 0)
			{
				++n_heavy;
				--sHeaviesOnCursor;
				if (--n == 0) break;
			}
		}

		if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
	}

	Assert(sGreensOnCursor   == 0);
	Assert(sRegularsOnCursor == 0);
	Assert(sElitesOnCursor   == 0);
	Assert(sHeaviesOnCursor  == 0);
}


// Even out troops among the town: all the militia of the town's sectors of
// ours with no enemy and the unassigned ones, dealt out one at a time to
// those sectors in turn, rank by rank, up to MAX_ALLOWABLE_MILITIA_PER_SECTOR
// each; what doesn't fit stays unassigned (the arrival mode's cursor can hold
// more than the room)
static void HandleEveningOutOfTroopsAmongstSectors()
{
	INT16 const town = sSelectedMilitiaTown;

	std::vector<UINT8> sectors;
	ForEachMilitiaBoxSector(town, [&](UINT8 const sector)
	{
		if (SectorOursAndPeaceful(SGPSector(sector))) sectors.push_back(sector);
	});
	if (sectors.empty()) return;

	INT32 n[MAX_MILITIA_LEVELS];
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank) n[rank] = GetPickedUpMilitia(rank);
	for (UINT8 const sector : sectors)
	{
		for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
		{
			n[rank] += SectorInfo[sector].ubNumberOfCivsAtLevel[rank];
			SectorInfo[sector].ubNumberOfCivsAtLevel[rank] = 0;
		}
		if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
	}

	size_t next = 0;
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		while (n[rank] != 0)
		{
			// the next sector with room
			size_t tried = 0;
			while (tried != sectors.size() && CountAllMilitiaInSector(SGPSector(sectors[next])) >= MAX_ALLOWABLE_MILITIA_PER_SECTOR)
			{
				next = (next + 1) % sectors.size();
				++tried;
			}
			if (tried == sectors.size()) break; // all full

			++SectorInfo[sectors[next]].ubNumberOfCivsAtLevel[rank];
			--n[rank];
			next = (next + 1) % sectors.size();
		}
	}

	// what didn't fit
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank) GetPickedUpMilitia(rank) = n[rank];
}


static void MakeButton(UINT idx, INT16 x, GUI_CALLBACK click, const ST::string& text)
{
	GUIButtonRef const btn = QuickCreateButtonImg(INTERFACEDIR "/militia.sti", 1, 2, x, MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_BOX_AUTO_BOX_Y, MSYS_PRIORITY_HIGHEST - 1, click);
	giMapMilitiaButton[idx] = btn;
	MilitiaBoxLayout const& box = MilitiaBox();
	btn->SpecifyGeneralTextAttributes(text, box.font, box.big ? box.colour : FONT_BLACK, box.big ? DEFAULT_SHADOW : FONT_BLACK);
}


static bool CanMilitiaAutoDistribute();
static void MilitiaAutoButtonCallback(GUI_BUTTON* btn, UINT32 reason);
static void MilitiaDoneButtonCallback(GUI_BUTTON* btn, UINT32 reason);


static void MilitiaDeployButtonCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) SendCursorMilitiaToDeployment(); // the box stays open (D8)
}


static void CreateMilitiaPanelBottomButton(void)
{
	// the arrival mode: Send to Deployment Center
	MilitiaBoxLayout const& box = MilitiaBox();
	if (g_arrival_order != 0 && box.deploy_y != 0 &&
			GetVObject(guiMilitia)->SubregionCount() > MILITIA_DEPLOY_BUTTON_PRESSED)
	{
		GUIButtonRef const b = QuickCreateButtonImg(INTERFACEDIR "/militia.sti", MILITIA_DEPLOY_BUTTON_NORMAL, MILITIA_DEPLOY_BUTTON_PRESSED,
			MAP_MILITIA_BOX_POS_X + box.deploy_x, MAP_MILITIA_BOX_POS_Y + box.deploy_y, MSYS_PRIORITY_HIGHEST - 1, MilitiaDeployButtonCallback);
		b->SpecifyGeneralTextAttributes(pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_SEND], box.font, box.colour, DEFAULT_SHADOW);
		b->SetFastHelpText(pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_SEND_HELP]);
		g_militia_deploy_button = b;
	}

	MakeButton(MILITIA_AUTO_BUTTON, MAP_MILITIA_BOX_POS_X + MAP_MILITIA_BOX_AUTO_BOX_X, MilitiaAutoButtonCallback, pMilitiaButtonString[0]);
	MakeButton(MILITIA_DONE_BUTTON, MAP_MILITIA_BOX_POS_X + MAP_MILITIA_BOX_DONE_BOX_X, MilitiaDoneButtonCallback, pMilitiaButtonString[1]);

	// AUTO button help
	giMapMilitiaButton[MILITIA_AUTO_BUTTON]->SetFastHelpText(pMilitiaButtonsHelpText[3]);


	// if auto-distribution is not possible
	if ( !CanMilitiaAutoDistribute( ) )
	{
		// disable the AUTO button
		DisableButton( giMapMilitiaButton[ MILITIA_AUTO_BUTTON ] );
	}
}


static void DeleteMilitiaPanelBottomButton(void)
{

	// delete militia panel bottom
	RemoveButton( giMapMilitiaButton[ MILITIA_AUTO_BUTTON ] );
	RemoveButton( giMapMilitiaButton[ MILITIA_DONE_BUTTON ] );
	if (g_militia_deploy_button) RemoveButton(g_militia_deploy_button);

	if( sSelectedMilitiaTown != 0 && g_arrival_order == 0 )
	{
		HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor( sSelectedMilitiaTown );
	}

	// redraw the map
	fMapPanelDirty = TRUE;

}


static void MilitiaAutoButtonCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		// distribute troops over all the sectors under control
		HandleEveningOutOfTroopsAmongstSectors();
		fMapPanelDirty = TRUE;
	}
}


static void MilitiaDoneButtonCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		// militia put in an enemy-held sector the mercs never liberated can't
		// attack it: the player is told, the box stays open
		INT16 const attacked = MilitiaBoxAttackSector(sSelectedMilitiaTown);
		if (attacked != -1 && !SectorInfo[attacked].fSurfaceWasEverPlayerControlled)
		{
			DoMapMessageBox(MSG_BOX_BASIC_STYLE,
				st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_LIBERATE_FIRST], SGPSector(attacked).AsShortString()),
				MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
			return;
		}

		// militia put in an enemy-held sector attack it
		g_militia_box_done = true;

		// reset fact we are in the box
		sSelectedMilitiaTown = 0;
		fMapPanelDirty = TRUE;
	}
}


static void RenderShadingForUnControlledSectors(void)
{
	// now render shading over any uncontrolled sectors
	INT16 const sBaseSectorValue = GetBaseSectorForCurrentTown();
	SGPSector sBase(sBaseSectorValue);
	SGPSector delta;
	for (delta.y = 0; delta.y != 3; ++delta.y)
	{
		for (delta.x = 0; delta.x != 3; ++delta.x)
		{
			SGPSector sMap = sBase + delta;
			StrategicMapElement const& e = StrategicMap[sMap.AsStrategicIndex()];
			if (!MilitiaBoxHasSector(sSelectedMilitiaTown, sMap.AsByte())) continue;
			if (!e.fEnemyControlled && NumHostilesInSector(sMap) == 0) continue;

			// shade this sector, not under our control
			INT16 const sX = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + delta.x * MILITIA_BOX_BOX_WIDTH;
			INT16 const sY = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + delta.y * MILITIA_BOX_BOX_HEIGHT;
			FRAME_BUFFER->ShadowRect(sX, sY, sX + MILITIA_BOX_BOX_WIDTH - 1, sY + MILITIA_BOX_BOX_HEIGHT - 1);
		}
	}
}


static void DrawMilitiaForcesForSector(INT32 const sector)
{
	SGPSector sMap(sector);
	if (StrategicMap[sMap.AsStrategicIndex()].fEnemyControlled) return;

	/* Large/small icon offset in the .sti */
	INT32      const  icon = 5;
	INT32             pos  = 0;
	SECTORINFO const& si   = SectorInfo[sector];
	for (INT32 i = si.ubNumberOfCivsAtLevel[GREEN_MILITIA]; i != 0; --i)
	{
		DrawMapBoxIcon(guiMilitia, icon, sMap, pos++);
	}
	for (INT32 i = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA]; i != 0; --i)
	{
		DrawMapBoxIcon(guiMilitia, icon + 1, sMap, pos++);
	}
	for (INT32 i = si.ubNumberOfCivsAtLevel[ELITE_MILITIA]; i != 0; --i)
	{
		DrawMapBoxIcon(guiMilitia, icon + 2, sMap, pos++);
	}
	for (INT32 i = si.ubNumberOfCivsAtLevel[ELITE_HEAVY_MILITIA]; i != 0; --i)
	{
		DrawMapBoxIcon(guiMilitia, HeavyMilitiaIcon(icon + 2), sMap, pos++);
	}
}


static void DrawTownMilitiaForcesOnMap()
{
	ClipBlitsToMapViewRegion();

	FOR_EACH_TOWN_SECTOR(i)
	{
		DrawMilitiaForcesForSector(i->sector);
	}

	// now handle militia for sam sectors
	for(auto s : GCM->getSamSites())
	{
		DrawMilitiaForcesForSector(s->sectorId);
	}

	RestoreClipRegionToFullScreen();
}


static void CheckAndUpdateStatesOfSelectedMilitiaSectorButtons()
{
	// the arrival mode: Done only with nothing unassigned (D4), Send to
	// Deployment Center with something
	if (g_militia_deploy_button) EnableButton(g_militia_deploy_button, CursorMilitiaTotal() != 0);
	if (g_arrival_order != 0) EnableButton(giMapMilitiaButton[MILITIA_AUTO_BUTTON], CanMilitiaAutoDistribute());
	if (!fMilitiaMapButtonsCreated)
	{
		EnableButton(giMapMilitiaButton[MILITIA_DONE_BUTTON], g_arrival_order == 0 || CursorMilitiaTotal() == 0);
		return;
	}

	INT16      const  base_sector = GetBaseSectorForCurrentTown();
	INT16      const  sector      = base_sector + (sSectorMilitiaMapSector % MILITIA_BOX_ROWS + sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16);
	SECTORINFO const& si          = SectorInfo[sector];
	INT32      const  n_green     = si.ubNumberOfCivsAtLevel[GREEN_MILITIA]   + sGreensOnCursor;
	INT32      const  n_regular   = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA] + sRegularsOnCursor;
	INT32      const  n_elite     = si.ubNumberOfCivsAtLevel[ELITE_MILITIA]   + sElitesOnCursor;
	INT32      const  n_heavy     = si.ubNumberOfCivsAtLevel[ELITE_HEAVY_MILITIA] + sHeaviesOnCursor;
	EnableButton(giMapMilitiaButton[MILITIA_DONE_BUTTON], sGreensOnCursor + sRegularsOnCursor + sElitesOnCursor + sHeaviesOnCursor == 0); // Done
	EnableButton(giMapMilitiaButton[0], n_green   != 0); // greens button
	EnableButton(giMapMilitiaButton[1], n_regular != 0); // regulars button
	EnableButton(giMapMilitiaButton[2], n_elite   != 0); // elites button
	EnableButton(giMapMilitiaButton[3], n_heavy   != 0); // elite heavies button

	// the arrows: up with some of the rank on the cursor, down with some in the sector
	if (MilitiaBox().arrow_up_y != 0)
	{
		for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
		{
			EnableButton(g_militia_up_arrow[rank],   GetPickedUpMilitia(rank) != 0);
			// the battle's sector: only those put there for it (D5.2)
			UINT8 const takeable = IsReinforceSector(sector) ? g_reinforce[rank] : si.ubNumberOfCivsAtLevel[rank];
			EnableButton(g_militia_down_arrow[rank], takeable != 0);
		}
	}
}


static void HideExistenceOfUndergroundMapSector(const SGPSector& sSector);


static void ShadeSubLevelsNotVisited(void)
{
	// Obtain the 16-bit version of the same color used in the mine STIs
	gusUndergroundNearBlack = Get16BPPColor(FROMRGB(2, 2, 0));

	// Run through all (real & possible) underground sectors
	for (UNDERGROUND_SECTORINFO const* i = gpUndergroundSectorInfoHead; i; i = i->next)
	{
		if (i->ubSector.z != (UINT8)iCurrentMapSectorZ) continue;
		if (i->uiFlags & SF_ALREADY_VISITED)           continue;
		/* The sector is on the currently displayed sublevel and has never been
			* visited.  Remove that portion of the "mine" graphics from view. */
		HideExistenceOfUndergroundMapSector(i->ubSector);
	}
}


static void HandleLowerLevelMapBlit(void)
{
	// blits the sub level maps
	cache_key_t const vo = GetMineLevelGraphicsFilename(iCurrentMapSectorZ);

	// Mine graphics (both tiers) are offset +21 X / +17 Y from
	// MAP_VIEW_START_X/Y, per user request. MAP_VIEW_START_X/Y themselves must
	// stay untouched -- they also anchor the terrain (B_MAP.PCX) blit and the
	// sector grid math above.
	INT16 const sBltX = MAP_VIEW_START_X + 21 + 22 - 1;
	INT16 const sBltY = MAP_VIEW_START_Y + 17 + 16 + 1;

	// handle the blt of the sublevel
	BltVideoObject(guiSAVEBUFFER, vo, 0, sBltX, sBltY);

	// handle shading of sublevels
	ShadeSubLevelsNotVisited( );
}


INT32 GetNumberOfMilitiaInSector(const SGPSector& sSector)
{
	INT32 iNumberInSector = 0;

	if (!sSector.z)
	{
		auto sector = sSector.AsByte();
		iNumberInSector = SectorInfo[sector].ubNumberOfCivsAtLevel[ GREEN_MILITIA ]
			+  SectorInfo[sector].ubNumberOfCivsAtLevel[ REGULAR_MILITIA ]
			+  SectorInfo[sector].ubNumberOfCivsAtLevel[ ELITE_MILITIA ]
			+  SectorInfo[sector].ubNumberOfCivsAtLevel[ ELITE_HEAVY_MILITIA ];
	}

	return( iNumberInSector );
}


//There is a special case flag used when players encounter enemies in a sector, then retreat.  The number of enemies
//will display on mapscreen until time is compressed.  When time is compressed, the flag is cleared, and
//a question mark is displayed to reflect that the player no longer knows.  This is the function that clears that
//flag.
void ClearAnySectorsFlashingNumberOfEnemies()
{
	INT32 i;
	for( i = 0; i < 256; i++ )
	{
		SectorInfo[ i ].uiFlags &= ~SF_PLAYER_KNOWS_ENEMIES_ARE_HERE;
	}

	// redraw map
	fMapPanelDirty = TRUE;
}


static BOOLEAN CanMercsScoutThisSector(const SGPSector& sSector);


UINT32 WhatPlayerKnowsAboutEnemiesInSector(const SGPSector& sSector)
{
	UINT32 uiSectorFlags = SectorInfo[sSector.AsByte()].uiFlags;

	// if player has militia close enough to scout this sector out, if there are mercs who can scout here, OR
	//Special case flag used when players encounter enemies in a sector, then retreat.  The number of enemies
	//will display on mapscreen until time is compressed.  When time is compressed, the flag is cleared, and
	//a question mark is displayed to reflect that the player no longer knows.
	if (CanMercsScoutThisSector(sSector) ||
		CanNearbyMilitiaScoutThisSector(sSector) ||
		( uiSectorFlags & SF_PLAYER_KNOWS_ENEMIES_ARE_HERE ) )
	{
		// if the enemies are stationary (i.e. mercs attacking a garrison)
		if (NumStationaryEnemiesInSector(sSector) > 0)
		{
			// inside a garrison - hide their # (show question mark) to match what the PBI is showing
			return KNOWS_THEYRE_THERE;
		}
		else
		{
			// other situations - show exactly how many there are
			return KNOWS_HOW_MANY;
		}
	}

	// if the player has visited the sector during this game
	if (GetSectorFlagStatus(sSector, SF_ALREADY_VISITED))
	{
		// then he always knows about any enemy presence for the remainder of the game, but not exact numbers
		return KNOWS_THEYRE_THERE;
	}

	// if Skyrider noticed the enemis in the sector recently
	if ( uiSectorFlags & SF_SKYRIDER_NOTICED_ENEMIES_HERE )
	{
		// and Skyrider is still in this sector, flying
		if (IsSkyriderFlyingInSector(sSector))
		{
			// player remains aware of them as long as Skyrider remains in the sector
			return KNOWS_THEYRE_THERE;
		}
		else
		{
			// Skyrider is gone, reset the flag that he noticed enemies here
			SectorInfo[sSector.AsByte()].uiFlags &= ~SF_SKYRIDER_NOTICED_ENEMIES_HERE;
		}
	}


	// no information available
	return KNOWS_NOTHING;
}


static BOOLEAN CanMercsScoutThisSector(const SGPSector& sSector)
{
	CFOR_EACH_IN_TEAM(pSoldier, OUR_TEAM)
	{
		// vehicles can't scout!
		if ( pSoldier->uiStatusFlags & SOLDIER_VEHICLE )
		{
			continue;
		}

		// POWs, dead guys, guys in transit, sleeping, and really hurt guys can't scout!
		if ( ( pSoldier->bAssignment == IN_TRANSIT ) ||
			( pSoldier->bAssignment == ASSIGNMENT_POW ) ||
			( pSoldier->bAssignment == ASSIGNMENT_DEAD ) ||
			pSoldier->fMercAsleep ||
			( pSoldier->bLife < OKLIFE ) )
		{
			continue;
		}

		// don't count mercs aboard Skyrider
		if (InHelicopter(*pSoldier)) continue;

		// mercs on the move can't scout
		if ( pSoldier->fBetweenSectors )
		{
			continue;
		}

		// is he here?
		if (pSoldier->sSector == sSector)
		{
			return( TRUE );
		}
	}

	// none here who can scout
	return( FALSE );
}


static void HandleShowingOfEnemyForcesInSector(const SGPSector& sSector, UINT8 const icon_pos)
{
	// ATE: If game has just started, don't do it
	if (DidGameJustStart()) return;

	// Never display enemies underground - sector info doesn't have support for it
	if (sSector.z != 0) return;

	INT16 const n_enemies = NumEnemiesInSector(sSector);
	if (n_enemies == 0) return; // No enemies here, display nothing

	switch (WhatPlayerKnowsAboutEnemiesInSector(sSector))
	{
		case KNOWS_NOTHING: // Display nothing
			break;

		case KNOWS_THEYRE_THERE: // Display a question mark
			ShowUncertainNumberEnemiesInSector(sSector.x, sSector.y);
			break;

		case KNOWS_HOW_MANY:
			/* Display individual icons for each enemy, starting at the received icon
				* position index */
			ShowEnemiesInSector(sSector, n_enemies, icon_pos);
			break;
	}
}


static void BlitSAMGridMarkers();


static void ShowSAMSitesOnStrategicMap()
{
	if (fShowAircraftFlag) BlitSAMGridMarkers();

	for (auto s : GCM->getSamSites())
	{
		SGPSector sMap(s->sectorId);

		// Has the sam site here been found?
		auto secret = GetMapSecretBySectorID(s->sectorId);
		if (secret && !IsSecretFoundAt(s->sectorId)) continue;

		DrawSite(sMap, GetVObject(guiSAMICON));

		if (fShowAircraftFlag)
		{ // write "SAM Site" centered underneath
			INT16 x;
			INT16 y;
			GetScreenXYFromMapXY(sMap, &x, &y);
			x += 11;
			y += 19;

			const ST::string& sam_site = GCM->getLandTypeString(SAM_SITE);

			// Center the first string around x.
			x -= StringPixLength(sam_site, MAP_FONT) / 2;

			if (x < MAP_VIEW_START_X || MAP_VIEW_START_X + MAP_VIEW_WIDTH  < x) continue;
			if (y < MAP_VIEW_START_Y || MAP_VIEW_START_Y + MAP_VIEW_HEIGHT < y) continue;
			// Within view, render.

			SetFontDestBuffer(guiSAVEBUFFER, MapScreenRect.iLeft + 2, MapScreenRect.iTop, MapScreenRect.iRight, MapScreenRect.iBottom);

			ClipBlitsToMapViewRegion();

			// Green on green does not contrast well, use yellow.
			SetFontAttributes(MAP_FONT, FONT_MCOLOR_LTYELLOW);
			GDirtyPrint(x, y, sam_site);

			RestoreClipRegionToFullScreen();
		}
	}
}


static void BlitSAMGridMarkers()
{
	UINT16 const colour = Get16BPPColor(FROMRGB(100, 100, 100));

	SGPVSurface::Lock l(guiSAVEBUFFER);
	UINT32 const uiDestPitchBYTES = l.Pitch();

	ClipBlitsToMapViewRegionForRectangleAndABit(uiDestPitchBYTES);

	for (auto s : GCM->getSamSites())
	{
		// Has the sam site here been found?
		if (!IsSecretFoundAt(s->sectorId)) continue;

		INT16 x;
		INT16 y;
		INT16 w;
		INT16 h;
		SGPSector sMap(s->sectorId);

		GetScreenXYFromMapXY(sMap, &x, &y);
		w = MAP_GRID_X;
		h = MAP_GRID_Y;

		RectangleDraw(TRUE, x, y - 1, x + w, y + h - 1, colour, l.Buffer<UINT16>());
	}

	RestoreClipRegionToFullScreenForRectangle(uiDestPitchBYTES);
}


static bool CanMilitiaAutoDistribute()
{
	INT16 const town = sSelectedMilitiaTown;

	// Cannot auto-distribute, if we don't have a town (or SAM site) selected
	if (town == BLANK_SECTOR) return false;

	bool can = false;
	ForEachMilitiaBoxSector(town, [&](UINT8 const sector)
	{
		if (!SectorOursAndPeaceful(SGPSector(sector))) return;
		// unassigned militia and a sector of ours with no enemy to put them
		// in, or militia there
		if (CursorMilitiaTotal() != 0 || CountAllMilitiaInSector(SGPSector(sector)) != 0) can = true;
	});
	return can;
}


static void ShowItemsOnMap(void)
{
	ClipBlitsToMapViewRegion();
	SetFontDestBuffer(guiSAVEBUFFER, MapScreenRect.iLeft + 2, MapScreenRect.iTop, MapScreenRect.iRight, MapScreenRect.iBottom);
	SetFontAttributes(MAP_FONT, FONT_MCOLOR_LTGREEN);

	// run through sectors
	SGPSector sector(1, 1, iCurrentMapSectorZ);
	for (sector.x = 1; sector.x < MAP_WORLD_X - 1; ++sector.x)
	{
		for (sector.y = 1; sector.y < MAP_WORLD_Y - 1; ++sector.y)
		{
			// to speed this up, only look at sector that player has visited
			if (!GetSectorFlagStatus(sector, SF_ALREADY_VISITED)) continue;

			UINT32 const n_items = GetNumberOfVisibleWorldItemsFromSectorStructureForSector(sector);
			if (n_items == 0) continue;

			INT16       usXPos;
			INT16       usYPos;
			INT16 const sXCorner = MAP_VIEW_START_X + sector.x * MAP_GRID_X;
			INT16 const sYCorner = MAP_VIEW_START_Y + sector.y * MAP_GRID_Y;
			ST::string sString = ST::format("{}", n_items);
			FindFontCenterCoordinates(sXCorner, sYCorner, MAP_GRID_X, MAP_GRID_Y, sString, MAP_FONT, &usXPos, &usYPos);
			GDirtyPrint(usXPos, usYPos, sString);
		}
	}

	RestoreClipRegionToFullScreen();
}


static void DrawMapBoxIcon(cache_key_t const vo, UINT16 const icon, const SGPSector& sMap, UINT8 const icon_pos)
{
	/* Don't show any more icons than will fit into one sector, to keep them from
		* spilling into sector(s) beneath */
	if (icon_pos >= MERC_ICONS_PER_LINE * ROWS_PER_SECTOR) return;

	INT32 const col = icon_pos % MERC_ICONS_PER_LINE;
	INT32 const row = icon_pos / MERC_ICONS_PER_LINE;

	// Spacing step was 3 (matching the old 3x3 icon size in boxes.sti/
	// militia.sti); doubled to 6 to match the new 6x6 icon size, per user
	// request -- otherwise consecutive icons would overlap by half their
	// width/height.
	INT32 const x = MAP_VIEW_START_X + sMap.x * MAP_GRID_X + MAP_X_ICON_OFFSET + 6 * col;
	INT32 const y = MAP_VIEW_START_Y + sMap.y * MAP_GRID_Y + MAP_Y_ICON_OFFSET + 6 * row;
	BltVideoObject(guiSAVEBUFFER, vo, icon, x, y);
	InvalidateRegion(x, y, x + DMAP_GRID_X, y + DMAP_GRID_Y);
}

void DrawSecretSite(const StrategicMapSecretModel* secret)
{
	if (!secret->secretMapIcon.empty())
	{
		const SGPVObject *const icon = gSecretSiteIcons.at(secret->secretMapIcon);
		DrawSite(SGPSector(secret->sectorID), icon);
	}
}


static void DrawBullseye(void)
{
	INT16 sX, sY;

	GetScreenXYFromMapXY(g_merc_arrive_sector, &sX, &sY);
	sY -= 2;

	BltVideoObject(guiSAVEBUFFER, guiBULLSEYE, 0, sX, sY);
}


static void HideExistenceOfUndergroundMapSector(const SGPSector& sSector)
{
	INT16 sScreenX;
	INT16 sScreenY;

	GetScreenXYFromMapXY(sSector, &sScreenX, &sScreenY);

	// fill it with near black
	ColorFillVideoSurfaceArea( guiSAVEBUFFER, sScreenX + 1, sScreenY, sScreenX + MAP_GRID_X,	sScreenY + MAP_GRID_Y - 1, gusUndergroundNearBlack );
}


BOOLEAN CanRedistributeMilitiaInSector(INT16 bClickedTownId)
{
	INT32 iCounter = 0;
	INT16 sBaseSectorValue = 0, sCurrentSectorValue = 0;

	// if no world is loaded, we can't be in combat (PBI/Auto-resolve locks out normal mapscreen interface for this)
	if( !gfWorldLoaded )
	{
		// ok to redistribute
		return( TRUE );
	}

	// if tactically not in combat, hostile sector, or air-raid
	if (!(gTacticalStatus.uiFlags & INCOMBAT) && !(gTacticalStatus.fEnemyInSector))
	{
		// ok to redistribute
		return( TRUE );
	}

	// if the fight is underground
	if (gWorldSector.z != 0)
	{
		// ok to redistribute
		return( TRUE );
	}


	// currently loaded surface sector IS hostile - so we must check if it's also one of the sectors in this "militia map"

	// a SAM site's box: its one sector
	if (IsSamBoxKey(bClickedTownId))
	{
		return !(SGPSector(SamBoxSector(bClickedTownId)) == gWorldSector);
	}

	// get the sector value for the upper left corner
	sBaseSectorValue = sBaseSectorList[bClickedTownId - 1].AsByte();

	// render icons for map
	for( iCounter = 0; iCounter < 9; iCounter++ )
	{
		// grab current sector value
		sCurrentSectorValue = sBaseSectorValue + ( ( iCounter % MILITIA_BOX_ROWS ) + ( iCounter / MILITIA_BOX_ROWS ) * ( 16 ) );
		SGPSector sSector(sCurrentSectorValue);

		// not in the same town?
		if (StrategicMap[sSector.AsStrategicIndex()].bNameId != bClickedTownId)
		{
			continue;
		}

		// if this is the loaded sector that is currently hostile
		if (sSector == gWorldSector)
		{
			// the fight is within this town!  Can't redistribute.
			return( FALSE );
		}
	}


	// the fight is elsewhere - ok to redistribute
	return( TRUE );
}
