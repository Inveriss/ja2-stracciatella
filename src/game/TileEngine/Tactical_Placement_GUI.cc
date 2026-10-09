#include "Directories.h"
#include "Font.h"
#include "HImage.h"
#include "Isometric_Utils.h"
#include "MercPortrait.h"
#include "Types.h"
#include "MouseSystem.h"
#include "Button_System.h"
#include "Input.h"
#include "Debug.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"
#include "VObject_Blitters.h"
#include "Line.h"
#include "Tactical_Placement_GUI.h"
#include "Overhead_Map.h"
#include "Interface.h"
#include "Font_Control.h"
#include "Overhead.h"
#include "Render_Dirty.h"
#include "SysUtil.h"
#include "PreBattle_Interface.h"
#include "Soldier_Profile.h"
#include "Map_Edgepoints.h"
#include "Strategic.h"
#include "StrategicMap.h"
#include "GameLoop.h"
#include "Message.h"
#include "Map_Information.h"
#include "Soldier_Add.h"
#include "Cursors.h"
#include "Cursor_Control.h"
#include "MessageBoxScreen.h"
#include "Assignments.h"
#include "Text.h"
#include "Game_Clock.h"
#include "JAScreens.h"
#include "Turn_Based_Input.h"
#include "Interface_Utils.h"
#include "Object_Cache.h"
#include "UILayout.h"
#include "MapScreen.h"
#include "RenderWorld.h"

#include <string_theory/format>
#include <string_theory/string>
#include <algorithm>
#include <memory>
#include <stdexcept>


struct MERCPLACEMENT
{
	SOLDIERTYPE		*pSoldier;
	SGPVObject*   uiVObjectID;
	MOUSE_REGION	region;
	UINT8					ubStrategicInsertionCode;
	BOOLEAN				fPlaced;
};

static std::unique_ptr<MERCPLACEMENT[]> gMercPlacement;
static INT32 giPlacements;

#define FOR_EACH_MERC_PLACEMENT(iter) \
	for (MERCPLACEMENT* iter = &gMercPlacement[0], * const iter##__end = &gMercPlacement[giPlacements]; iter != iter##__end; ++iter)

enum
{
	DONE_BUTTON,
	SPREAD_BUTTON,
	GROUP_BUTTON,
	CLEAR_BUTTON,
	NUM_TP_BUTTONS
};
GUIButtonRef iTPButtons[NUM_TP_BUTTONS];

UINT8	gubDefaultButton = CLEAR_BUTTON;
BOOLEAN gfTacticalPlacementGUIActive = FALSE;
BOOLEAN gfTacticalPlacementFirstTime = FALSE;
BOOLEAN gfEnterTacticalPlacementGUI = FALSE;
BOOLEAN gfKillTacticalGUI = FALSE;
static SGPVObject* giOverheadPanelImage;
static BUTTON_PICS* giOverheadButtonImages[NUM_TP_BUTTONS];
SGPVObject* giMercPanelImage = 0;
BOOLEAN gfTacticalPlacementGUIDirty = FALSE;
BOOLEAN gfValidLocationsChanged = FALSE;
BOOLEAN gfValidCursor = FALSE;
BOOLEAN gfEveryonePlaced = FALSE;

UINT8	gubSelectedGroupID = 0;
UINT8	gubHilightedGroupID = 0;
UINT8 gubCursorGroupID = 0;
INT8	gbSelectedMercID = -1;
INT8	gbHilightedMercID = -1;
INT8  gbCursorMercID = -1;
SOLDIERTYPE *gpTacticalPlacementSelectedSoldier = NULL;
SOLDIERTYPE *gpTacticalPlacementHilightedSoldier = NULL;

// 1366x768: placement in the 1:1 tactical view above a full width panel
static bool gfPlacementFullView = false;
static BOOLEAN gfPlacementOldVideoScroll;
// the panel at the top of the screen: the south edge stays free
static bool gfPlacementPanelTop = false;

static char const* const g_full_view_panel = INTERFACEDIR "/overheadinterface_1366x768.png";

bool TacticalPlacementFullView()
{
	return gfPlacementFullView;
}

// top left corner of the bottom panel (640x160 or 1366x200)
static INT32 PanelX() { return gfPlacementFullView ? 0 : STD_SCREEN_X; }
static INT32 PanelY()
{
	if (!gfPlacementFullView) return STD_SCREEN_Y + 320;
	return gfPlacementPanelTop ? 0 : SCREEN_HEIGHT - TACTICAL_PLACEMENT_PANEL_HEIGHT;
}

INT16 TacticalPlacementViewTop()
{
	return gfPlacementPanelTop ? TACTICAL_PLACEMENT_PANEL_HEIGHT : 0;
}

INT16 TacticalPlacementViewBottom()
{
	return gfPlacementPanelTop ? SCREEN_HEIGHT : SCREEN_HEIGHT - TACTICAL_PLACEMENT_PANEL_HEIGHT;
}

// The 1366x768 panel (panel coordinates): the big portraits, 9 a row, one
// row shown at a time, scrolled by the mouse wheel and the arrows. A block
// is a 2 px black frame around the 106x122 portrait and the 3 bars (3 px).
#define FV_BUTTONS_DY    40 // the buttons are lower than on the old panel
#define FV_BLOCK_X      144
#define FV_BLOCK_Y       47
#define FV_BLOCK_W      123
#define FV_BLOCK_H      126
#define FV_BLOCK_STEP   133 // 10 px between the blocks
#define FV_PER_ROW        9
#define FV_PORTRAIT_W   106
#define FV_PORTRAIT_H   122
#define FV_BAR_DX       110 // life, breath, morale every 4 px
#define FV_NAME_Y       175
#define FV_NAME_H        13
#define FV_ARROW_X      101
#define FV_ARROW_UP_Y    49
#define FV_ARROW_DOWN_Y 158
#define FV_TRACK_Y       76 // the slider's track, down to y 157
#define FV_TRACK_H       82

static char const* const g_full_view_arrows = INTERFACEDIR "/mapinv_done_buttons.sti";
static GUIButtonRef g_placement_arrows[2];
static MOUSE_REGION g_placement_panel_region; // the mouse wheel over the portraits
static INT32        giPlacementRow = 0;       // the row shown

static INT32 PlacementRows() { return std::max<INT32>(1, (giPlacements + FV_PER_ROW - 1) / FV_PER_ROW); }
static bool  MercShown(INT32 const i) { return !gfPlacementFullView || i / FV_PER_ROW == giPlacementRow; }

// a merc's region in the panel (portrait and name)
static INT32 MercRegionX(INT32 const i) { return gfPlacementFullView ? PanelX() + FV_BLOCK_X + i % FV_PER_ROW * FV_BLOCK_STEP : PanelX() + 91 + i / 2 * 54; }
static INT32 MercRegionY(INT32 const i) { return gfPlacementFullView ? PanelY() + FV_BLOCK_Y : PanelY() + 41 + i % 2 * 51; }
static INT32 MercRegionW() { return gfPlacementFullView ? FV_BLOCK_W : 54; }
static INT32 MercRegionH() { return gfPlacementFullView ? FV_NAME_Y + FV_NAME_H - FV_BLOCK_Y : 62; }
// the old panel's portrait
static INT32 MercPortraitX(INT32 const i) { return PanelX() + 95 + i / 2 * 54; }
static INT32 MercPortraitY(INT32 const i) { return PanelY() + 51 + i % 2 * 51; }

static bool gfNorth;
static bool gfEast;
static bool gfSouth;
static bool gfWest;


static void MakeButton(UINT idx, INT16 y, GUI_CALLBACK click, const ST::string& text, const ST::string& help)
{
	INT16 const dy = gfPlacementFullView ? FV_BUTTONS_DY : 0;
	GUIButtonRef const btn = QuickCreateButton(giOverheadButtonImages[idx], PanelX() + 11, PanelY() + y + dy, MSYS_PRIORITY_HIGH, click);
	iTPButtons[idx] = btn;
	btn->SpecifyGeneralTextAttributes(text, BLOCKFONT, FONT_BEIGE, 141);
	btn->SetFastHelpText(help);
	btn->SpecifyHilitedTextColors(FONT_WHITE, FONT_NEARBLACK);
}


static void ClearPlacementsCallback(GUI_BUTTON* btn, UINT32 reason);
static void DoneOverheadPlacementClickCallback(GUI_BUTTON* btn, UINT32 reason);
static void GroupPlacementsCallback(GUI_BUTTON* btn, UINT32 reason);
static void MercClickCallback(MOUSE_REGION* reg, UINT32 reason);
static void MercMoveCallback(MOUSE_REGION* reg, UINT32 reason);
static void PlaceMercs(void);
static void SetCursorMerc(INT8 placement);
static void SpreadPlacementsCallback(GUI_BUTTON* btn, UINT32 reason);


// 1366x768: shows the row `row` of the portraits (clamped)
static void SetPlacementRow(INT32 row)
{
	if (!gfPlacementFullView) return;
	row = std::clamp<INT32>(row, 0, PlacementRows() - 1);
	giPlacementRow = row;
	for (INT32 i = 0; i != giPlacements; ++i)
	{
		MOUSE_REGION& r = gMercPlacement[i].region;
		if (MercShown(i)) r.Enable(); else r.Disable();
	}
	if (g_placement_arrows[0])
	{
		EnableButton(g_placement_arrows[0], row > 0);
		EnableButton(g_placement_arrows[1], row < PlacementRows() - 1);
	}
	// the highlighted merc may be gone from the panel
	gbHilightedMercID   = -1;
	gubHilightedGroupID =  0;
	gpTacticalPlacementHilightedSoldier = 0;
	gfTacticalPlacementGUIDirty = TRUE;
}


// 1366x768: the row of this merc is shown
static void ShowPlacementMerc(INT32 const i)
{
	if (gfPlacementFullView && i >= 0 && !MercShown(i)) SetPlacementRow(i / FV_PER_ROW);
}


static void PlacementWheel(UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_WHEEL_UP)   SetPlacementRow(giPlacementRow - 1);
	if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN) SetPlacementRow(giPlacementRow + 1);
}


static void PlacementPanelRegionCallback(MOUSE_REGION*, UINT32 const reason)
{
	PlacementWheel(reason);
}


static void PlacementArrowCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	SetPlacementRow(giPlacementRow + (btn == g_placement_arrows[0] ? -1 : +1));
}


// 1366x768: the arrows (mapinv_done_buttons.sti 8/9 up, 10/11 down, 12 the
// slider) and the mouse wheel over the portraits
static void CreateFullViewScrolling()
{
	giPlacementRow = 0;
	g_placement_arrows[0] = GUIButtonRef();
	g_placement_arrows[1] = GUIButtonRef();
	bool arrows = false;
	try
	{
		arrows = GetVObject(g_full_view_arrows)->SubregionCount() >= 13;
	}
	catch (std::exception const& e)
	{
		SLOGE("No placement arrows: {}", e.what());
	}
	if (arrows)
	{
		INT16 const x = PanelX() + FV_ARROW_X;
		g_placement_arrows[0] = QuickCreateButtonImg(g_full_view_arrows,  8,  9, x, PanelY() + FV_ARROW_UP_Y,   MSYS_PRIORITY_HIGH, PlacementArrowCallback);
		g_placement_arrows[1] = QuickCreateButtonImg(g_full_view_arrows, 10, 11, x, PanelY() + FV_ARROW_DOWN_Y, MSYS_PRIORITY_HIGH, PlacementArrowCallback);
	}
	else
	{
		SLOGW("{} has no sub-images 8-12, the portraits scroll by the mouse wheel only", g_full_view_arrows);
	}

	INT16 const x = PanelX() + FV_BLOCK_X - 4;
	INT16 const y = PanelY() + FV_BLOCK_Y - 4;
	MSYS_DefineRegion(&g_placement_panel_region, x, y, SCREEN_WIDTH, SCREEN_HEIGHT, MSYS_PRIORITY_HIGH + 1, 0, MSYS_NO_CALLBACK, PlacementPanelRegionCallback);
	SetPlacementRow(0);
}


static void RemoveFullViewScrolling()
{
	for (GUIButtonRef& b : g_placement_arrows)
	{
		if (b) RemoveButton(b);
		b = GUIButtonRef();
	}
	MSYS_RemoveRegion(&g_placement_panel_region);
}


// a merc's big portrait (the 65 one if there is none)
static SGPVObject* LoadPlacementPortrait(MERCPROFILESTRUCT const& p)
{
	if (gfPlacementFullView)
	{
		try
		{
			return LoadBigPortrait(p);
		}
		catch (std::exception const& e)
		{
			SLOGE("No big portrait, using the small one: {}", e.what());
		}
	}
	return Load65Portrait(p);
}


void InitTacticalPlacementGUI()
{
	gfTacticalPlacementGUIActive = TRUE;
	gfTacticalPlacementGUIDirty  = TRUE;
	gfValidLocationsChanged      = TRUE;
	gfTacticalPlacementFirstTime = TRUE;

	char const* const panel = g_ui.isExtraWideStrategicScreen() ?
		FirstUsableInterfaceAsset({ g_full_view_panel, INTERFACEDIR "/overheadinterface.sti" }) :
		INTERFACEDIR "/overheadinterface.sti";
	gfPlacementFullView = panel == g_full_view_panel;
	if (gfPlacementFullView)
	{
		// scrolling renders the whole world again (shading drawn over it)
		gfPlacementOldVideoScroll = gfDoVideoScroll;
		gfDoVideoScroll           = FALSE;
	}

	// Mercs entering from the south: the panel goes to the top of the screen,
	// over the north edge, else it would cover most of their entry area.
	gfPlacementPanelTop = false;
	if (gfPlacementFullView)
	{
		GROUP const& bg = *gpBattleGroup;
		CFOR_EACH_IN_TEAM(s, OUR_TEAM)
		{
			if (s->bLife == 0)                      continue;
			if (s->fBetweenSectors)                 continue;
			if (s->sSector != bg.ubSector)          continue;
			if (s->uiStatusFlags & SOLDIER_VEHICLE) continue;
			if (s->bAssignment == ASSIGNMENT_POW)   continue;
			if (s->bAssignment == IN_TRANSIT)       continue;
			if (s->sSector.z != 0)                  continue;
			// as the insertion code is taken below
			UINT8 const code =
				s->ubStrategicInsertionCode == INSERTION_CODE_PRIMARY_EDGEINDEX ||
				s->ubStrategicInsertionCode == INSERTION_CODE_SECONDARY_EDGEINDEX ?
				(UINT8)s->usStrategicInsertionData : s->ubStrategicInsertionCode;
			if (code == INSERTION_CODE_SOUTH) gfPlacementPanelTop = true;
		}
	}

	GoIntoOverheadMap();

	giOverheadPanelImage = AddVideoObjectFromFile(panel);
	giMercPanelImage     = AddVideoObjectFromFile(INTERFACEDIR "/panels.sti");

	BUTTON_PICS* const img = LoadButtonImage(INTERFACEDIR "/overheaduibuttons.sti", 0, 1);
	giOverheadButtonImages[DONE_BUTTON]   = img;
	giOverheadButtonImages[SPREAD_BUTTON] = UseLoadedButtonImage(img, 0, 1);
	giOverheadButtonImages[GROUP_BUTTON]  = UseLoadedButtonImage(img, 0, 1);
	giOverheadButtonImages[CLEAR_BUTTON]  = UseLoadedButtonImage(img, 0, 1);

	// Create the buttons which provide automatic placements.
	MakeButton(CLEAR_BUTTON,   12, ClearPlacementsCallback,            gpStrategicString[STR_TP_CLEAR],  gpStrategicString[STR_TP_CLEARHELP]);
	MakeButton(SPREAD_BUTTON,  47, SpreadPlacementsCallback,           gpStrategicString[STR_TP_SPREAD], gpStrategicString[STR_TP_SPREADHELP]);
	MakeButton(GROUP_BUTTON,   82, GroupPlacementsCallback,            gpStrategicString[STR_TP_GROUP],  gpStrategicString[STR_TP_GROUPHELP]);
	MakeButton(DONE_BUTTON,   117, DoneOverheadPlacementClickCallback, gpStrategicString[STR_TP_DONE],   gpStrategicString[STR_TP_DONEHELP]);
	iTPButtons[DONE_BUTTON]->AllowDisabledFastHelp();

	GROUP const& bg = *gpBattleGroup;
	/* First pass: Count the number of mercs that are going to be placed by the
	 * player. This determines the size of the array we will allocate. */
	size_t n = 0;
	CFOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		if (s->fBetweenSectors)                 continue;
		if (s->sSector != bg.ubSector)          continue;
		if (s->uiStatusFlags & SOLDIER_VEHICLE) continue; // ATE Ignore vehicles
		if (s->bAssignment == ASSIGNMENT_POW)   continue;
		if (s->bAssignment == IN_TRANSIT)       continue;
		if (s->sSector.z != 0)                  continue;
		++n;
	}
	// Allocate the array based on how many mercs there are.
	gMercPlacement.reset(new MERCPLACEMENT[n]);

	// Second pass: Assign the mercs to their respective slots.
	giPlacements = 0;
	gfNorth      = false;
	gfEast       = false;
	gfSouth      = false;
	gfWest       = false;
	FOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		if (s->bLife == 0)                      continue;
		if (s->fBetweenSectors)                 continue;
		if (s->sSector != bg.ubSector)          continue;
		if (s->uiStatusFlags & SOLDIER_VEHICLE) continue; // ATE Ignore vehicles
		if (s->bAssignment == ASSIGNMENT_POW)   continue;
		if (s->bAssignment == IN_TRANSIT)       continue;
		if (s->sSector.z != 0)                  continue;

		if (s->ubStrategicInsertionCode == INSERTION_CODE_PRIMARY_EDGEINDEX ||
				s->ubStrategicInsertionCode == INSERTION_CODE_SECONDARY_EDGEINDEX)
		{
			s->ubStrategicInsertionCode = (UINT8)s->usStrategicInsertionData;
		}

		UINT32 const   i = giPlacements++;
		MERCPLACEMENT& m = gMercPlacement[i];
		m.pSoldier                 = s;
		m.ubStrategicInsertionCode = s->ubStrategicInsertionCode;
		m.fPlaced                  = FALSE;
		m.uiVObjectID              = LoadPlacementPortrait(GetProfile(m.pSoldier->ubProfile));
		INT32 const x = MercRegionX(i);
		INT32 const y = MercRegionY(i);
		// 1366x768: above the panel's wheel region
		INT16 const priority = gfPlacementFullView ? MSYS_PRIORITY_HIGH + 2 : MSYS_PRIORITY_HIGH;
		MSYS_DefineRegion(&m.region, x, y, x + MercRegionW(), y + MercRegionH(), priority, 0, MercMoveCallback, MercClickCallback);

		switch (s->ubStrategicInsertionCode)
		{
			case INSERTION_CODE_NORTH: gfNorth = true; break;
			case INSERTION_CODE_EAST:  gfEast  = true; break;
			case INSERTION_CODE_SOUTH: gfSouth = true; break;
			case INSERTION_CODE_WEST:  gfWest  = true; break;
		}
	}

	if (gfPlacementFullView) CreateFullViewScrolling();

	PlaceMercs();

	if (gubDefaultButton == GROUP_BUTTON)
	{
		iTPButtons[GROUP_BUTTON]->uiFlags |= BUTTON_CLICKED_ON;
		for (INT32 i = 0; i != giPlacements; ++i)
		{
			MERCPLACEMENT const& m = gMercPlacement[i];
			if (m.fPlaced) continue;
			// Found an unplaced merc. Select him.
			gbSelectedMercID                   = i;
			gubSelectedGroupID                 = m.pSoldier->ubGroupID;
			gpTacticalPlacementSelectedSoldier = m.pSoldier;
			SetCursorMerc(i);
			ShowPlacementMerc(i);
			break;
		}
	}
}


static void DrawBar(SGPVSurface* const buf, INT32 const x, INT32 const y, INT32 const h, UINT32 const colour1, UINT32 const colour2)
{
	ColorFillVideoSurfaceArea(buf, x,     y - h, x + 1, y, Get16BPPColor(colour1));
	ColorFillVideoSurfaceArea(buf, x + 1, y - h, x + 2, y, Get16BPPColor(colour2));
}


static UINT16 PlacementHatchColour()
{
	return
		DayTime() ? 0 :                     // 6AM to 9PM is black
		Get16BPPColor(FROMRGB(63, 31, 31)); // 9PM to 6AM is gray (black is too dark to distinguish)
}


// The overhead map's placement edges (its own coordinates, as in the overhead
// branch below) carried over to the 1:1 view: everything but the edges where
// the mercs may enter gets the hatch.
static void ShadeFullViewInvalidArea()
{
	INT16 const view_top    = TacticalPlacementViewTop();
	INT16 const view_bottom = TacticalPlacementViewBottom();
	INT16 top    = view_top;
	INT16 left   = 0;
	INT16 bottom = view_bottom;
	INT16 right  = SCREEN_WIDTH;

	bool north = false;
	bool east  = false;
	bool south = false;
	bool west  = false;
	if (gbCursorMercID == -1)
	{
		north = gfNorth;
		east  = gfEast;
		south = gfSouth;
		west  = gfWest;
	}
	else switch (gMercPlacement[gbCursorMercID].ubStrategicInsertionCode)
	{
		case INSERTION_CODE_NORTH: north = true; break;
		case INSERTION_CODE_EAST:  east  = true; break;
		case INSERTION_CODE_SOUTH: south = true; break;
		case INSERTION_CODE_WEST:  west  = true; break;
	}

	INT16 x;
	INT16 y;
	OverheadToViewportXY(30, 30, &x, &y);
	if (north) top  = std::max(top,  y);
	if (west)  left = std::max(left, x);
	OverheadToViewportXY(610, 290, &x, &y);
	if (south) bottom = std::min(bottom, y);
	if (east)  right  = std::min(right,  x);
	if (top >= bottom || left >= right) return;

	UINT16 const hatch_colour = PlacementHatchColour();
	{
		SGPRect clip = { (UINT16)left, (UINT16)top, (UINT16)right, (UINT16)bottom };
		SGPVSurface::Lock l(FRAME_BUFFER);
		Blt16BPPBufferLooseHatchRectWithColor(l.Buffer<UINT16>(), l.Pitch(), &clip, hatch_colour);
	}

	// the edge of the entry area: a 5 px line on the hatched side of each
	// edge inside the view
	INT16 const w = 5;
	SGPVSurface* const buf = FRAME_BUFFER;
	if (top    > view_top)     ColorFillVideoSurfaceArea(buf, left,      top,        right,    std::min<INT16>(top + w, bottom), hatch_colour);
	if (bottom < view_bottom)  ColorFillVideoSurfaceArea(buf, left,      std::max<INT16>(bottom - w, top), right, bottom, hatch_colour);
	if (left   > 0)            ColorFillVideoSurfaceArea(buf, left,      top,        std::min<INT16>(left + w, right), bottom, hatch_colour);
	if (right  < SCREEN_WIDTH) ColorFillVideoSurfaceArea(buf, std::max<INT16>(right - w, left), top, right, bottom, hatch_colour);
}


// 1366x768: the portraits of the row shown, their bars and the slider
static void RenderFullViewMercs(SGPVSurface* const buf)
{
	for (INT32 i = 0; i != giPlacements; ++i)
	{
		if (!MercShown(i)) continue;
		INT32 const x = MercRegionX(i);
		INT32 const y = MercRegionY(i);
		ColorFillVideoSurfaceArea(buf, x, y, x + FV_BLOCK_W, y + FV_BLOCK_H, 0);
		BltVideoObject(buf, gMercPlacement[i].uiVObjectID, 0, x + 2, y + 2);
	}

	// DrawSoldierUIBarsTall() erases its windows from the saved background
	BlitBufferToBuffer(buf, guiSAVEBUFFER, PanelX(), PanelY(), SCREEN_WIDTH - PanelX(), TACTICAL_PLACEMENT_PANEL_HEIGHT);
	for (INT32 i = 0; i != giPlacements; ++i)
	{
		if (!MercShown(i)) continue;
		INT16 const x = MercRegionX(i) + FV_BAR_DX;
		INT16 const y = MercRegionY(i) + 2;
		DrawSoldierUIBarsTall(*gMercPlacement[i].pSoldier, x, x + 4, x + 8, y, 3, FV_PORTRAIT_H, buf);
	}

	if (g_placement_arrows[0])
	{
		SGPVObject* const vo = GetVObject(g_full_view_arrows);
		ETRLEObject const& thumb = vo->SubregionProperties(12);
		INT32 const rows   = PlacementRows();
		INT32 const travel = FV_TRACK_H - thumb.usHeight;
		INT32 const y      = PanelY() + FV_TRACK_Y + (rows > 1 ? travel * giPlacementRow / (rows - 1) : 0);
		INT32 const x      = PanelX() + FV_ARROW_X + (27 - thumb.usWidth) / 2;
		BltVideoObject(buf, vo, 12, x, y);
	}
}


static void RenderTacticalPlacementGUI()
{
	if (gfTacticalPlacementFirstTime)
	{
		gfTacticalPlacementFirstTime = FALSE;
		DisableScrollMessages();
	}

	/* Check to make sure that if we have a hilighted merc (not selected) and the
	 * mouse has moved out of its region, then we will clear the hilighted ID, and
	 * refresh the display. */
	if (!gfTacticalPlacementGUIDirty && gbHilightedMercID != -1)
	{
		INT32 const x = MercRegionX(gbHilightedMercID);
		INT32 const y = MercRegionY(gbHilightedMercID);
		if (gusMouseXPos < x || x + MercRegionW() < gusMouseXPos ||
				gusMouseYPos < y || y + MercRegionH() < gusMouseYPos)
		{
			gbHilightedMercID   = -1;
			gubHilightedGroupID =  0;
			SetCursorMerc(gbSelectedMercID);
			gpTacticalPlacementHilightedSoldier = 0;
		}
	}

	if (gfPlacementFullView)
	{
		// the world was rendered again this frame: everything over it again
		gfTacticalPlacementGUIDirty = TRUE;
		gfValidLocationsChanged     = TRUE;
	}

	SGPVSurface* const buf = FRAME_BUFFER;
	// If the display is dirty render the entire panel.
	if (gfTacticalPlacementGUIDirty)
	{
		BltVideoObject(buf, giOverheadPanelImage, 0, PanelX(), PanelY());
		InvalidateRegion(STD_SCREEN_X + 0, STD_SCREEN_Y + 0, STD_SCREEN_X + 320, STD_SCREEN_Y + 480);
		gfTacticalPlacementGUIDirty = FALSE;
		MarkButtonsDirty();
		if (gfPlacementFullView) RenderFullViewMercs(buf);
		else for (INT32 i = 0; i != giPlacements; ++i)
		{ // Render the mercs
			MERCPLACEMENT const& m = gMercPlacement[i];
			INT32         const  x = MercPortraitX(i);
			INT32         const  y = MercPortraitY(i);
			ColorFillVideoSurfaceArea(buf, x + 36, y + 2, x + 44, y + 30, 0);
			BltVideoObject(buf, giMercPanelImage, 0, x,     y);
			BltVideoObject(buf, m.uiVObjectID,    0, x + 2, y + 2);

			SOLDIERTYPE const& s = *m.pSoldier;
			if (s.bLife == 0) continue;

			DrawBar(buf, x + 36, y + 29, s.bLifeMax   * 27 / 100, FROMRGB(107, 107,  57), FROMRGB(222, 181, 115)); // Yellow one for bleeding
			DrawBar(buf, x + 36, y + 29, s.bBleeding  * 27 / 100, FROMRGB(156,  57,  57), FROMRGB(222, 132, 132)); // Pink one for bandaged
			DrawBar(buf, x + 36, y + 29, s.bLife      * 27 / 100, FROMRGB(107,   8,   8), FROMRGB(206,   0,   0)); // Red one for actual health
			DrawBar(buf, x + 39, y + 29, s.bBreathMax * 27 / 100, FROMRGB(  8,   8, 132), FROMRGB(  8,   8, 107)); // Breath bar
			DrawBar(buf, x + 42, y + 29, s.bMorale    * 27 / 100, FROMRGB(  8, 156,   8), FROMRGB(  8, 107,   8)); // Morale bar
		}

		SetFontAttributes(BLOCKFONT, FONT_BEIGE);
		ST::string str = GetSectorIDString(gubPBSector, TRUE);
		MPrint(PanelX() + 120, PanelY() + 15, ST::format("{} {} -- {}...", gpStrategicString[STR_TP_SECTOR], str, gpStrategicString[STR_TP_CHOOSEENTRYPOSITIONS]));

		// Shade out the part of the tactical map that isn't considered placable.
		if (!gfPlacementFullView) BlitBufferToBuffer(buf, guiSAVEBUFFER, STD_SCREEN_X + 0, STD_SCREEN_Y + 320, 640, 160);
	}

	if (gfValidLocationsChanged && gfPlacementFullView)
	{
		gfValidLocationsChanged = FALSE;
		ShadeFullViewInvalidArea();
	}
	else if (gfValidLocationsChanged)
	{
		gfValidLocationsChanged = FALSE;
		BlitBufferToBuffer(guiSAVEBUFFER, buf, STD_SCREEN_X + 4, STD_SCREEN_Y + 4, 636, 320);
		InvalidateRegion(STD_SCREEN_X + 4, STD_SCREEN_Y + 4, STD_SCREEN_X + 636, STD_SCREEN_Y + 320);

		UINT16 const hatch_colour = PlacementHatchColour();
		SGPRect clip = { (UINT16)(STD_SCREEN_X + 4), (UINT16)(STD_SCREEN_Y + 4), (UINT16)(STD_SCREEN_X + 636), (UINT16)(STD_SCREEN_Y + 320) };
		if (gbCursorMercID == -1)
		{
			if (gfNorth) clip.iTop    = STD_SCREEN_Y +  30;
			if (gfEast)  clip.iRight  = STD_SCREEN_X + 610;
			if (gfSouth) clip.iBottom = STD_SCREEN_Y + 290;
			if (gfWest)  clip.iLeft   = STD_SCREEN_X +  30;
		}
		else
		{
			switch (gMercPlacement[gbCursorMercID].ubStrategicInsertionCode)
			{
				case INSERTION_CODE_NORTH: clip.iTop    = STD_SCREEN_Y +  30; break;
				case INSERTION_CODE_EAST:  clip.iRight  = STD_SCREEN_X + 610; break;
				case INSERTION_CODE_SOUTH: clip.iBottom = STD_SCREEN_Y + 290; break;
				case INSERTION_CODE_WEST:  clip.iLeft   = STD_SCREEN_X +  30; break;
			}
		}
		SGPVSurface::Lock l(buf);
		UINT16* const pDestBuf         = l.Buffer<UINT16>();
		UINT32  const uiDestPitchBYTES = l.Pitch();
		Blt16BPPBufferLooseHatchRectWithColor(pDestBuf, uiDestPitchBYTES, &clip, hatch_colour);
		SetClippingRegionAndImageWidth(uiDestPitchBYTES, STD_SCREEN_X + 0, STD_SCREEN_Y + 0, 640, 480);
		RectangleDraw(TRUE, clip.iLeft, clip.iTop, clip.iRight, clip.iBottom, hatch_colour, pDestBuf);
	}

	bool const is_group = gubDefaultButton == GROUP_BUTTON;
	for (INT32 i = 0; i != giPlacements; ++i)
	{ // Render the merc's names
		if (!MercShown(i)) continue;
		INT32 const x = gfPlacementFullView ? MercRegionX(i) : MercPortraitX(i);
		INT32 const y = gfPlacementFullView ? MercRegionY(i) : MercPortraitY(i);

		MERCPLACEMENT const& m     = gMercPlacement[i];
		SOLDIERTYPE   const& s     = *m.pSoldier;
		UINT8         const colour =
			(is_group ? s.ubGroupID == gubSelectedGroupID  : i == gbSelectedMercID)  ? FONT_YELLOW :
			(is_group ? s.ubGroupID == gubHilightedGroupID : i == gbHilightedMercID) ? FONT_WHITE  :
			FONT_GRAY3;
		SGPFont const font = gfPlacementFullView ? StrategicGeneralFont() : BLOCKFONT;
		SetFontAttributes(font, colour);
		INT32 const w  = StringPixLength(s.name, font);
		INT32 const nx = gfPlacementFullView ? x + (FV_BLOCK_W - w) / 2 : x + (48 - w) / 2;
		INT32 const ny = gfPlacementFullView ? PanelY() + FV_NAME_Y : y + 33;
		MPrint(nx, ny, s.name);
		InvalidateRegion(nx, ny, nx + w, ny + w);

		// Render a question mark over the face, if the merc hasn't yet been placed.
		INT32 const qx = gfPlacementFullView ? x + 2 + FV_PORTRAIT_W / 2 - 4 : x + 16;
		INT32 const qy = gfPlacementFullView ? y + 2 + FV_PORTRAIT_H / 2 - 4 : y + 14;
		if (m.fPlaced)
		{
			RegisterBackgroundRect(BGND_FLAG_SINGLE, qx, qy, 8, 8);
		}
		else
		{
			SetFont(FONT10ARIALBOLD);
			MPrint(qx, qy, "?");
			InvalidateRegion(qx, qy, qx + 8, qy + 8);
		}
	}
}


static void EnsureDoneButtonStatus(void)
{
	bool enable = true;
	FOR_EACH_MERC_PLACEMENT(i)
	{
		if (i->fPlaced) continue;
		enable = false;
		break;
	}
	GUI_BUTTON& b = *iTPButtons[DONE_BUTTON];
	// Only enable it when it is disabled, otherwise the button will stay down
	if (b.Enabled() == enable) return;
	EnableButton(&b, enable);
	b.SetFastHelpText(enable ? gpStrategicString[STR_TP_DONEHELP] : gpStrategicString[STR_TP_DISABLED_DONEHELP]);
}


static void KillTacticalPlacementGUI(void);


void TacticalPlacementHandle()
{
	InputAtom InputEvent;

	EnsureDoneButtonStatus();

	RenderTacticalPlacementGUI();

	if( IsMouseButtonDown(MOUSE_BUTTON_RIGHT) )
	{
		gbSelectedMercID = -1;
		gubSelectedGroupID = 0;
		gpTacticalPlacementSelectedSoldier = NULL;
	}

	while( DequeueSpecificEvent(&InputEvent, KEYBOARD_EVENTS) )
	{
		if( InputEvent.usEvent == KEY_DOWN )
		{
			switch( InputEvent.usParam )
			{
				case SDLK_RETURN:
					if (iTPButtons[DONE_BUTTON]->Enabled())
					{
						KillTacticalPlacementGUI();
					}
					break;
				case 'c':
					ClearPlacementsCallback(iTPButtons[CLEAR_BUTTON], MSYS_CALLBACK_REASON_POINTER_UP);
					break;
				case 'g':
					GroupPlacementsCallback(iTPButtons[GROUP_BUTTON], MSYS_CALLBACK_REASON_POINTER_UP);
					break;
				case 's':
					SpreadPlacementsCallback(iTPButtons[SPREAD_BUTTON], MSYS_CALLBACK_REASON_POINTER_UP);
					break;
				case 't':
					// the 1:1 view shows the tree tops, as in the game
					if (gfPlacementFullView) ToggleTreeTops();
					break;
				case 'x':
					if( InputEvent.usKeyState & ALT_DOWN )
					{
						HandleShortCutExitState();
					}
					break;
			}
		}
	}
	gfValidCursor = FALSE;
	bool const mouse_in_map = gfPlacementFullView ?
		TacticalPlacementViewTop() <= gusMouseYPos && gusMouseYPos < TacticalPlacementViewBottom() :
		(gusMouseYPos >= STD_SCREEN_Y) && (gusMouseYPos < STD_SCREEN_Y + 320) &&
		(gusMouseXPos >= STD_SCREEN_X) && (gusMouseXPos < STD_SCREEN_X + 640);
	if (gbSelectedMercID != -1 && mouse_in_map)
	{
		// the mouse in the overhead map's own coordinates
		INT16 mx = gusMouseXPos - STD_SCREEN_X;
		INT16 my = gusMouseYPos - STD_SCREEN_Y;
		if (gfPlacementFullView) ViewportToOverheadXY(gusMouseXPos, gusMouseYPos, &mx, &my);
		switch( gMercPlacement[ gbCursorMercID ].ubStrategicInsertionCode )
		{
			case INSERTION_CODE_NORTH:
				if (my <= 40)
					gfValidCursor = TRUE;
				break;
			case INSERTION_CODE_EAST:
				if (mx >= 600)
					gfValidCursor = TRUE;
				break;
			case INSERTION_CODE_SOUTH:
				if (my >= 280)
					gfValidCursor = TRUE;
				break;
			case INSERTION_CODE_WEST:
				if (mx <= 40)
					gfValidCursor = TRUE;
				break;
		}
		if( gubDefaultButton == GROUP_BUTTON )
		{
			if( gfValidCursor )
			{
				SetCurrentCursorFromDatabase( CURSOR_PLACEGROUP );
			}
			else
			{
				SetCurrentCursorFromDatabase( CURSOR_DPLACEGROUP );
			}
		}
		else
		{
			if( gfValidCursor )
			{
				SetCurrentCursorFromDatabase( CURSOR_PLACEMERC );
			}
			else
			{
				SetCurrentCursorFromDatabase( CURSOR_DPLACEMERC );
			}
		}
	}
	else
	{
		SetCurrentCursorFromDatabase( CURSOR_NORMAL );
	}
	if( gfKillTacticalGUI == 1 )
	{
		KillTacticalPlacementGUI();
	}
	else if( gfKillTacticalGUI == 2 )
	{
		gfKillTacticalGUI = 1;
	}
}


static void PickUpMercPiece(MERCPLACEMENT&);


static void KillTacticalPlacementGUI(void)
{
	gbHilightedMercID = -1;
	gbSelectedMercID = -1;
	gubSelectedGroupID = 0;
	gubHilightedGroupID = 0;
	gbCursorMercID = -1;
	gpTacticalPlacementHilightedSoldier = NULL;
	gpTacticalPlacementSelectedSoldier = NULL;

	//Destroy the tactical placement gui.
	gfEnterTacticalPlacementGUI = FALSE;
	gfTacticalPlacementGUIActive = FALSE;
	gfKillTacticalGUI = FALSE;
	//Delete video objects
	DeleteVideoObject(giOverheadPanelImage);
	DeleteVideoObject(giMercPanelImage);
	//Delete buttons
	for (INT32 i = 0; i < NUM_TP_BUTTONS; ++i)
	{
		UnloadButtonImage( giOverheadButtonImages[ i ] );
		RemoveButton( iTPButtons[ i ] );
	}
	if (gfPlacementFullView) RemoveFullViewScrolling();
	//Delete faces and regions
	FOR_EACH_MERC_PLACEMENT(i)
	{
		MERCPLACEMENT& m = *i;
		DeleteVideoObject(m.uiVObjectID);
		MSYS_RemoveRegion(&m.region);
	}

	if( gsCurInterfacePanel >= NUM_UI_PANELS )
		gsCurInterfacePanel = TEAM_PANEL;

	SetCurrentInterfacePanel(gsCurInterfacePanel);

	//Leave the overhead map.
	if (gfPlacementFullView)
	{
		gfDoVideoScroll     = gfPlacementOldVideoScroll;
		gfPlacementFullView = false;
	}
	KillOverheadMap();
	//Recreate the tactical panel.
	gRadarRegion.Enable();
	SetCurrentInterfacePanel( TEAM_PANEL );
	//Initialize the rest of the map (AI, enemies, civs, etc.)

	FOR_EACH_MERC_PLACEMENT(i) PickUpMercPiece(*i);

	gMercPlacement.reset();
	PrepareLoadedSector();
	EnableScrollMessages();
}


static void PutDownMercPiece(MERCPLACEMENT&);


static void ChooseRandomEdgepoints(void)
{
	FOR_EACH_MERC_PLACEMENT(i)
	{
		MERCPLACEMENT& m = *i;
		if (!(m.pSoldier->uiStatusFlags & SOLDIER_VEHICLE))
		{
			m.pSoldier->usStrategicInsertionData = ChooseMapEdgepoint(m.ubStrategicInsertionCode);
			if (m.pSoldier->usStrategicInsertionData != NOWHERE)
			{
				m.pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
			}
			else
			{
#if 0 /* XXX unsigned < 0 ? */
				Assert(0 <= m.pSoldier->usStrategicInsertionData && m.pSoldier->usStrategicInsertionData < WORLD_MAX);
#else
				Assert(m.pSoldier->usStrategicInsertionData < WORLD_MAX);
#endif
				m.pSoldier->ubStrategicInsertionCode = m.ubStrategicInsertionCode;
			}
		}

		PutDownMercPiece(m);
	}
	gfEveryonePlaced = TRUE;
}


static void PlaceMercs(void)
{
	switch( gubDefaultButton )
	{
		case SPREAD_BUTTON: //Place mercs randomly along their side using map edgepoints.
			ChooseRandomEdgepoints();
			break;
		case CLEAR_BUTTON:
			FOR_EACH_MERC_PLACEMENT(i) PickUpMercPiece(*i);
			gubSelectedGroupID = 0;
			gbSelectedMercID = 0;
			SetCursorMerc( 0 );
			gfEveryonePlaced = FALSE;
			break;
		default:
			return;
	}
	gfTacticalPlacementGUIDirty = TRUE;
}


static void DoneOverheadPlacementClickCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
	{
		gfKillTacticalGUI = 2;
	}
}


static void SpreadPlacementsCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
	{
		gubDefaultButton = SPREAD_BUTTON;
		iTPButtons[GROUP_BUTTON]->uiFlags &= ~BUTTON_CLICKED_ON;
		iTPButtons[GROUP_BUTTON]->uiFlags |= BUTTON_DIRTY;
		PlaceMercs();
		gubSelectedGroupID = 0;
		gbSelectedMercID = -1;
		SetCursorMerc( -1 );
	}
}


static void GroupPlacementsCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
	{
		if( gubDefaultButton == GROUP_BUTTON )
		{
			btn->uiFlags &= ~BUTTON_CLICKED_ON;
			btn->uiFlags |= BUTTON_DIRTY;
			gubDefaultButton = CLEAR_BUTTON;
			gubSelectedGroupID = 0;
		}
		else
		{
			btn->uiFlags |= BUTTON_CLICKED_ON | BUTTON_DIRTY;
			gubDefaultButton = GROUP_BUTTON;
			gbSelectedMercID = 0;
			SetCursorMerc( gbSelectedMercID );
			gubSelectedGroupID = gMercPlacement[ gbSelectedMercID ].pSoldier->ubGroupID;
		}
	}
}


static void ClearPlacementsCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
	{
		iTPButtons[GROUP_BUTTON]->uiFlags &= ~BUTTON_CLICKED_ON;
		iTPButtons[GROUP_BUTTON]->uiFlags |= BUTTON_DIRTY;
		gubDefaultButton = CLEAR_BUTTON;
		PlaceMercs();
	}
}


static void MercMoveCallback(MOUSE_REGION* reg, UINT32 reason)
{
	if( reg->uiFlags & MSYS_MOUSE_IN_AREA )
	{
		INT8 i;
		for( i = 0; i < giPlacements; i++ )
		{
			if( &gMercPlacement[ i ].region == reg )
			{
				if( gbHilightedMercID != i )
				{
					gbHilightedMercID = i;
					if( gubDefaultButton == GROUP_BUTTON )
						gubHilightedGroupID = gMercPlacement[ i ].pSoldier->ubGroupID;
					SetCursorMerc( i );
					gpTacticalPlacementHilightedSoldier = gMercPlacement[ i ].pSoldier;
				}
				return;
			}
		}
	}
}


static void MercClickCallback(MOUSE_REGION* reg, UINT32 reason)
{
	PlacementWheel(reason);

	if( reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		INT8 i;
		for( i = 0; i < giPlacements; i++ )
		{
			if( &gMercPlacement[ i ].region == reg )
			{
				if( gbSelectedMercID != i )
				{
					gbSelectedMercID = i;
					gpTacticalPlacementSelectedSoldier = gMercPlacement[ i ].pSoldier;
					if( gubDefaultButton == GROUP_BUTTON )
					{
						gubSelectedGroupID = gpTacticalPlacementSelectedSoldier->ubGroupID;
					}
				}
				return;
			}
		}
	}
}


static void SelectNextUnplacedUnit(void)
{
	INT32 i;
	if( gbSelectedMercID == -1 )
		return;
	for( i = gbSelectedMercID; i < giPlacements; i++ )
	{ //go from the currently selected soldier to the end
		if( !gMercPlacement[ i ].fPlaced )
		{ //Found an unplaced merc.  Select him.
			gbSelectedMercID = (INT8)i;
			if( gubDefaultButton == GROUP_BUTTON )
				gubSelectedGroupID = gMercPlacement[ i ].pSoldier->ubGroupID;
			gfTacticalPlacementGUIDirty = TRUE;
			SetCursorMerc( (INT8)i );
			gpTacticalPlacementSelectedSoldier = gMercPlacement[ i ].pSoldier;
			ShowPlacementMerc(i);
			return;
		}
	}
	for( i = 0; i < gbSelectedMercID; i++ )
	{ //go from the beginning to the currently selected soldier
		if( !gMercPlacement[ i ].fPlaced )
		{ //Found an unplaced merc.  Select him.
			gbSelectedMercID = (INT8)i;
			if( gubDefaultButton == GROUP_BUTTON )
				gubSelectedGroupID = gMercPlacement[ i ].pSoldier->ubGroupID;
			gfTacticalPlacementGUIDirty = TRUE;
			SetCursorMerc( (INT8)i );
			gpTacticalPlacementSelectedSoldier = gMercPlacement[ i ].pSoldier;
			ShowPlacementMerc(i);
			return;
		}
	}
	//checked the whole array, and everybody has been placed.  Select nobody.
	if( !gfEveryonePlaced )
	{
		gfEveryonePlaced = TRUE;
		SetCursorMerc( -1 );
		gbSelectedMercID = -1;
		gubSelectedGroupID = 0;
		gfTacticalPlacementGUIDirty = TRUE;
		gfValidLocationsChanged = TRUE;
		gpTacticalPlacementSelectedSoldier = gMercPlacement[ i ].pSoldier;
	}
}


static void DialogRemoved(MessageBoxReturnValue);


// the middle of the map area
static SGPBox PlacementMessageBoxRect()
{
	if (gfPlacementFullView)
	{
		return SGPBox{ (UINT16)((SCREEN_WIDTH - 200) / 2), (UINT16)((TacticalPlacementViewTop() + TacticalPlacementViewBottom() - 80) / 2), 200, 80 };
	}
	return SGPBox{ (UINT16)(STD_SCREEN_X + 220), (UINT16)(STD_SCREEN_Y + 120), 200, 80 };
}


void HandleTacticalPlacementClicksInOverheadMap(INT32 reason)
{
	BOOLEAN fInvalidArea = FALSE;
	if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
	{ //if we have a selected merc, move him to the new closest map edgepoint of his side.
		if( gfValidCursor )
		{
			if( gbSelectedMercID != -1 )
			{
				const GridNo sGridNo = GetOverheadMouseGridNo();
				if (sGridNo != NOWHERE)
				{ //we have clicked within a valid part of the map.
					BeginMapEdgepointSearch();

					if( gubDefaultButton == GROUP_BUTTON )
					{ //We are placing a whole group.
						FOR_EACH_MERC_PLACEMENT(i)
						{ //Find locations of each member of the group, but don't place them yet.  If
							//one of the mercs can't be placed, then we won't place any, and tell the user
							//the problem.  If everything's okay, we will place them all.
							MERCPLACEMENT const& m = *i;
							if (m.pSoldier->ubGroupID == gubSelectedGroupID)
							{
								m.pSoldier->usStrategicInsertionData = SearchForClosestPrimaryMapEdgepoint(sGridNo, m.ubStrategicInsertionCode);
								if (m.pSoldier->usStrategicInsertionData == NOWHERE)
								{
									fInvalidArea = TRUE;
									break;
								}
							}
						}
						if( !fInvalidArea )
						{ //One or more of the mercs in the group didn't get gridno assignments, so we
							//report an error.
							FOR_EACH_MERC_PLACEMENT(i)
							{
								MERCPLACEMENT& m = *i;
								m.pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
								if (m.pSoldier->ubGroupID == gubSelectedGroupID)
								{
									PutDownMercPiece(m);
								}
							}
						}
					}
					else
					{ //This is a single merc placement.  If valid, then place him, else report error.
						MERCPLACEMENT& m = gMercPlacement[gbSelectedMercID];
						m.pSoldier->usStrategicInsertionData = SearchForClosestPrimaryMapEdgepoint(sGridNo, m.ubStrategicInsertionCode);
						if (m.pSoldier->usStrategicInsertionData != NOWHERE)
						{
							m.pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
							PutDownMercPiece(m);
						}
						else
						{
							fInvalidArea = TRUE;
						}

						//gbSelectedMercID++;
						//if( gbSelectedMercID == giPlacements )
						//	gbSelectedMercID = 0;
						//gpTacticalPlacementSelectedSoldier = gMercPlacement[ gbSelectedMercID ].pSoldier;
						gfTacticalPlacementGUIDirty = TRUE;
						//SetCursorMerc( gbSelectedMercID );
					}
					EndMapEdgepointSearch();

					if( fInvalidArea )
					{ //Report error due to invalid placement.
						SGPBox const CenterRect = PlacementMessageBoxRect();
						DoMessageBox(MSG_BOX_BASIC_STYLE, gpStrategicString[STR_TP_INACCESSIBLE_MESSAGE], guiCurrentScreen, MSG_BOX_FLAG_OK, DialogRemoved, &CenterRect);
					}
					else
					{ //Placement successful, so select the next unplaced unit (single or group).
						SelectNextUnplacedUnit();
					}
				}
			}
		}
		else
		{ //not a valid cursor location...
			if( gbCursorMercID != - 1 )
			{
				SGPBox const CenterRect = PlacementMessageBoxRect();
				DoMessageBox(MSG_BOX_BASIC_STYLE, gpStrategicString[STR_TP_INVALID_MESSAGE], guiCurrentScreen, MSG_BOX_FLAG_OK, DialogRemoved, &CenterRect);
			}
		}
	}
}


static void SetCursorMerc(INT8 const placement)
{
	if (gbCursorMercID == placement) return;

	if (gbCursorMercID == -1 ||
			placement      == -1 ||
			gMercPlacement[gbCursorMercID].ubStrategicInsertionCode != gMercPlacement[placement].ubStrategicInsertionCode)
	{
		gfValidLocationsChanged = TRUE;
	}
	gbCursorMercID = placement;
}


static void PutDownMercPiece(MERCPLACEMENT& m)
{
	SOLDIERTYPE& s = *m.pSoldier;
	GridNo       insertion_gridno;
	switch (s.ubStrategicInsertionCode)
	{
		case INSERTION_CODE_NORTH:  insertion_gridno = gMapInformation.sNorthGridNo; break;
		case INSERTION_CODE_SOUTH:  insertion_gridno = gMapInformation.sSouthGridNo; break;
		case INSERTION_CODE_EAST:   insertion_gridno = gMapInformation.sEastGridNo;  break;
		case INSERTION_CODE_WEST:   insertion_gridno = gMapInformation.sWestGridNo;  break;
		case INSERTION_CODE_GRIDNO: insertion_gridno = s.usStrategicInsertionData;   break;
		default: throw std::logic_error("invalid strategic insertion code");
	}
	s.sInsertionGridNo = insertion_gridno;
	if (m.fPlaced) PickUpMercPiece(m);
	GridNo const gridno = FindGridNoFromSweetSpot(&s, insertion_gridno, 4);
	if (gridno != NOWHERE)
	{
		EVENT_SetSoldierPositionNoCenter(&s, gridno, SSP_NONE);
		UINT8 const direction = GetDirectionToGridNoFromGridNo(gridno, CENTER_GRIDNO);
		EVENT_SetSoldierDirection(&s, direction);
		s.ubInsertionDirection = s.bDirection;
		m.fPlaced = TRUE;
		m.pSoldier->bInSector = TRUE;
	}
}


static void PickUpMercPiece(MERCPLACEMENT& m)
{
	m.fPlaced = FALSE;
	SOLDIERTYPE& s = *m.pSoldier;
	RemoveSoldierFromGridNo(s);
	s.bInSector = FALSE;
}


static void DialogRemoved(MessageBoxReturnValue const ubResult)
{
	gfTacticalPlacementGUIDirty = TRUE;
	gfValidLocationsChanged = TRUE;
}
