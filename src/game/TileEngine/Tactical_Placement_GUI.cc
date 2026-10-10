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
#include "Viewport_Zoom.h"
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
#include "ContentManager.h"
#include "FileMan.h"
#include "GameInstance.h"
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
#include <cmath>
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
// Where the 1366x768 panel is, by the entry edges (see InitTacticalPlacementGUI()):
// at the bottom; at the top (the south edge stays free); at the bottom and
// right (west entry) or left (east entry) aligned, narrower, the west / east
// edge's bottom free.
enum PlacementPanelSide { PANEL_BOTTOM, PANEL_TOP, PANEL_WEST, PANEL_EAST };
static PlacementPanelSide g_placement_panel   = PANEL_BOTTOM;
static INT16              g_placement_panel_w = 1366;
// How the panel graphic in use is built (image coordinates): the 200 px
// panel and the box of the minimap (Data/RadarMaps_Overhead).
//   overheadinterface_north / west / east_1366x768.png (533 px tall): the box
//     (648x333, its 640x320 window at (4, 6)) above the panel, at the image's
//     right edge -- the east one's at the left edge;
//   overheadinterface_south_1366x768.png: the panel at the top, the box below
//     it at the right edge, its window at (4, 7);
//   the older north_south / west_east ones (391 px tall): a centred box for a
//     352x176 minimap above the panel, drawn below it when the panel is at the
//     top of the screen (two_parts).
struct PanelGfx
{
	INT16 panel_y;                    // the panel's first row
	INT16 box_x, box_y, box_w, box_h; // the minimap's box; box_h 0: none
	INT16 map_dx, map_dy;             // the minimap in the box
	INT16 map_w, map_h;               // and its place's size
	bool  two_parts;
};
static PanelGfx    g_gfx;
static SGPVObject* g_placement_minimap = 0;
// the minimap's place in the box, by the panel graphic
static INT32 MinimapPlaceH() { return g_gfx.map_h; }
static INT32 MinimapPlaceW() { return g_gfx.map_w; }
// The minimap graphic's own size: the scale of the view's rectangle and of
// the clicks. Normally the place's size; a map of another size is cut to it.
static INT32 g_minimap_w = 352;
static INT32 g_minimap_h = 176;

// The tactical viewport ends above the game's bottom panel; with the
// placement panel at the top the world is shown down to the screen's bottom.
struct SavedViewport
{
	UINT16  end_y;
	UINT16  window_end_y;
	UINT16  center_y;
	SGPRect clip;
};
static SavedViewport g_saved_viewport;
static bool          g_viewport_extended = false;

static void ExtendViewportToScreenBottom()
{
	g_saved_viewport = { g_ui.m_VIEWPORT_END_Y, g_ui.m_VIEWPORT_WINDOW_END_Y, g_ui.m_tacticalMapCenterY, g_ui.m_worldClippingRect };
	g_ui.m_VIEWPORT_END_Y            = SCREEN_HEIGHT;
	g_ui.m_VIEWPORT_WINDOW_END_Y     = SCREEN_HEIGHT;
	g_ui.m_tacticalMapCenterY        = (SCREEN_HEIGHT - g_ui.m_VIEWPORT_START_Y) / 2;
	g_ui.m_worldClippingRect.iBottom = SCREEN_HEIGHT;
	g_viewport_extended = true;
	SetRenderFlags(RENDER_FLAG_FULL);
}

static void RestoreViewport()
{
	if (!g_viewport_extended) return;
	g_ui.m_VIEWPORT_END_Y        = g_saved_viewport.end_y;
	g_ui.m_VIEWPORT_WINDOW_END_Y = g_saved_viewport.window_end_y;
	g_ui.m_tacticalMapCenterY    = g_saved_viewport.center_y;
	g_ui.m_worldClippingRect     = g_saved_viewport.clip;
	g_viewport_extended = false;
	SetRenderFlags(RENDER_FLAG_FULL);
}

// by the entry edge; the older two as fallbacks
static char const* const g_panel_north = INTERFACEDIR "/overheadinterface_north_1366x768.png";
static char const* const g_panel_south = INTERFACEDIR "/overheadinterface_south_1366x768.png";
static char const* const g_panel_west  = INTERFACEDIR "/overheadinterface_west_1366x768.png";
static char const* const g_panel_east  = INTERFACEDIR "/overheadinterface_east_1366x768.png";
static char const* const g_full_view_panel      = INTERFACEDIR "/overheadinterface_north_south_1366x768.png";
static char const* const g_full_view_panel_side = INTERFACEDIR "/overheadinterface_west_east_1366x768.png";

// The first of the two panel graphics that can be used, none: 0.
static char const* UsablePanel(char const* const first, char const* const second)
{
	char const* const original = INTERFACEDIR "/overheadinterface.sti";
	char const* const file     = FirstUsableInterfaceAsset({ first, second, original });
	return file == original ? 0 : file;
}

static bool IsNewPanel(char const* const file)
{
	return file == g_panel_north || file == g_panel_south || file == g_panel_west || file == g_panel_east;
}

bool TacticalPlacementFullView()
{
	return gfPlacementFullView;
}

// top left corner of the panel (640x160, 1366x200 or 1207x200)
static INT32 PanelX()
{
	if (!gfPlacementFullView) return STD_SCREEN_X;
	return g_placement_panel == PANEL_WEST ? SCREEN_WIDTH - g_placement_panel_w : 0;
}
static INT32 PanelY()
{
	if (!gfPlacementFullView) return STD_SCREEN_Y + 320;
	return g_placement_panel == PANEL_TOP ? 0 : SCREEN_HEIGHT - TACTICAL_PLACEMENT_PANEL_HEIGHT;
}

INT16 TacticalPlacementViewTop()
{
	return g_placement_panel == PANEL_TOP ? TACTICAL_PLACEMENT_PANEL_HEIGHT : 0;
}

INT16 TacticalPlacementViewBottom()
{
	// beside a narrower panel the world is shown down to the screen's bottom
	return g_placement_panel == PANEL_BOTTOM ? SCREEN_HEIGHT - TACTICAL_PLACEMENT_PANEL_HEIGHT : SCREEN_HEIGHT;
}

bool TacticalPlacementPanelBox(SGPBox& box)
{
	if (!gfPlacementFullView) return false;
	box = { (UINT16)PanelX(), (UINT16)PanelY(), (UINT16)g_placement_panel_w, (UINT16)TACTICAL_PLACEMENT_PANEL_HEIGHT };
	return true;
}

// The minimap's box: above the panel, below it when the panel is at the top
// of the screen.
bool TacticalPlacementMapBox(SGPBox& box)
{
	if (!gfPlacementFullView || g_gfx.box_h == 0) return false;
	INT32 const x = PanelX() + g_gfx.box_x;
	INT32 const y = g_gfx.two_parts && g_placement_panel == PANEL_TOP ?
		TACTICAL_PLACEMENT_PANEL_HEIGHT :
		PanelY() - g_gfx.panel_y + g_gfx.box_y;
	box = { (UINT16)x, (UINT16)y, (UINT16)g_gfx.box_w, (UINT16)g_gfx.box_h };
	return true;
}

static bool MouseInBox(SGPBox const& b)
{
	return b.x <= gusMouseXPos && gusMouseXPos < b.x + b.w && b.y <= gusMouseYPos && gusMouseYPos < b.y + b.h;
}

bool TacticalPlacementMouseOverPanel()
{
	SGPBox b;
	if (TacticalPlacementPanelBox(b) && MouseInBox(b)) return true;
	if (TacticalPlacementMapBox(b)   && MouseInBox(b)) return true;
	return false;
}


// The part of the map shown, a green rectangle on the minimap at (mx, my) --
// as RenderRadarScreen() on the tactical radar, without the part of the view
// under the panel.
static void DrawPlacementMinimapView(SGPVSurface* const buf, INT32 const mx, INT32 const my)
{
	INT32 const w = g_minimap_w;
	INT32 const h = g_minimap_h;
	double const scale_x = double(w) / (gsRightX  - gsLeftX);
	double const scale_y = double(h) / (gsBottomY - gsTopY);

	INT16 left;
	INT16 top;
	INT16 right;
	INT16 bottom;
	ViewportZoomGetVisibleWorldRect(left, top, right, bottom);
	if (!ViewportZoomIsActive())
	{
		top    = gsTopLeftWorldY + TacticalPlacementViewTop();
		bottom = gsTopLeftWorldY + TacticalPlacementViewBottom();
	}

	SGPVSurface::Lock l(buf);
	SetClippingRegionAndImageWidth(l.Pitch(), mx, my, std::min(w, MinimapPlaceW()), std::min(h, MinimapPlaceH()));
	RectangleDraw(TRUE,
		mx + std::max(0.0, std::round((left - SCROLL_LEFT_PADDING) * scale_x)),
		my + std::max(0.0, std::round((top  - SCROLL_TOP_PADDING)  * scale_y)),
		mx + std::min(std::round((right  - SCROLL_RIGHT_PADDING  - SCROLL_LEFT_PADDING) * scale_x - 1.0), double(w - 1)),
		my + std::min(std::round((bottom - SCROLL_BOTTOM_PADDING - SCROLL_TOP_PADDING)  * scale_y - 1.0), double(h - 1)),
		Get16BPPColor(FROMRGB(0, 255, 0)), l.Buffer<UINT16>());
}


// The mouse on the minimap: the view goes there, as on the tactical radar --
// the clicked place in the middle of the free part of the view.
static MOUSE_REGION g_placement_minimap_region;
static bool         g_placement_minimap_region_made = false;

static void MovePlacementViewFromMinimap(MOUSE_REGION const& r)
{
	double const scale_x = double(g_minimap_w) / (gsRightX  - gsLeftX);
	double const scale_y = double(g_minimap_h) / (gsBottomY - gsTopY);

	// from the minimap's middle to screen coordinates from the map's middle
	INT32 x = (INT32)((r.RelativeXPos - g_minimap_w / 2) / scale_x);
	INT32 y = (INT32)((r.RelativeYPos - g_minimap_h / 2) / scale_y);
	// the render centre is the whole viewport's middle, the panel covers a part of it
	y += g_ui.m_tacticalMapCenterY - (TacticalPlacementViewTop() + TacticalPlacementViewBottom()) / 2;

	// multiples of the scroll steps, as AdjustWorldCenterFromRadarCoords()
	x = x / WORLD_TILE_X * WORLD_TILE_X;
	y = y / (WORLD_TILE_Y * 2) * (WORLD_TILE_Y * 2);

	INT16 cell_x;
	INT16 cell_y;
	FromScreenToCellCoordinates((INT16)x, (INT16)y, &cell_x, &cell_y);
	SetRenderCenter(gCenterWorldX + cell_x, gCenterWorldY + cell_y);
	SetRenderFlags(RENDER_FLAG_FULL);
}

static void PlacementMinimapMoveCallback(MOUSE_REGION* const r, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_MOVE && r->ButtonState & MSYS_LEFT_BUTTON) MovePlacementViewFromMinimap(*r);
}

static void PlacementMinimapClickCallback(MOUSE_REGION* const r, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN) MovePlacementViewFromMinimap(*r);
}

static void CreatePlacementMinimapRegion()
{
	SGPBox box;
	if (!g_placement_minimap || !TacticalPlacementMapBox(box)) return;
	INT16 const x = box.x + g_gfx.map_dx;
	INT16 const y = box.y + g_gfx.map_dy;
	MSYS_DefineRegion(&g_placement_minimap_region, x, y, x + std::min(g_minimap_w, MinimapPlaceW()), y + std::min(g_minimap_h, MinimapPlaceH()), MSYS_PRIORITY_HIGH + 3, 0, PlacementMinimapMoveCallback, PlacementMinimapClickCallback);
	g_placement_minimap_region_made = true;
}

static void RemovePlacementMinimapRegion()
{
	if (!g_placement_minimap_region_made) return;
	MSYS_RemoveRegion(&g_placement_minimap_region);
	g_placement_minimap_region_made = false;
}


// The panel graphic: its panel at PanelY(), the minimap's box where the
// graphic has it. An older graphic (the box above the panel) with the panel
// at the top of the screen: in two parts, clipped, the box below the panel.
static void DrawPlacementPanel(SGPVSurface* const buf)
{
	if (!g_gfx.two_parts || g_placement_panel != PANEL_TOP)
	{
		BltVideoObject(buf, giOverheadPanelImage, 0, PanelX(), PanelY() - g_gfx.panel_y);
	}
	else
	{
		INT32 const h = TACTICAL_PLACEMENT_PANEL_HEIGHT;
		SGPRect const old = SetClippingRect(SGPRect{ 0, 0, (UINT16)SCREEN_WIDTH, (UINT16)h });
		BltVideoObject(buf, giOverheadPanelImage, 0, PanelX(), PanelY() - g_gfx.panel_y);
		SetClippingRect(SGPRect{ 0, (UINT16)h, (UINT16)SCREEN_WIDTH, (UINT16)(h + g_gfx.box_h) });
		BltVideoObject(buf, giOverheadPanelImage, 0, PanelX(), h);
		SetClippingRect(old);
	}

	SGPBox box;
	if (g_placement_minimap && TacticalPlacementMapBox(box))
	{
		INT32 const mx = box.x + g_gfx.map_dx;
		INT32 const my = box.y + g_gfx.map_dy;
		// cut to its place: a map bigger than the panel graphic's box stays inside
		SGPRect const old = SetClippingRect(SGPRect{ (UINT16)mx, (UINT16)my, (UINT16)(mx + MinimapPlaceW()), (UINT16)(my + MinimapPlaceH()) });
		BltVideoObject(buf, g_placement_minimap, 0, mx, my);
		SetClippingRect(old);
		DrawPlacementMinimapView(buf, mx, my);
	}
}


// the minimap of the sector (352x176, 640x320), none if it is missing
static SGPVObject* LoadPlacementMinimap()
{
	ST::string const name = FileMan::replaceExtension(FileMan::getFileName(GetMapFileName(gWorldSector, TRUE)), "sti");
	try
	{
		return AddVideoObjectFromFile(GCM->getRadarMapOverheadResourceName(name));
	}
	catch (std::exception const& e)
	{
		SLOGW("No placement minimap: {}", e.what());
		return 0;
	}
}

// The 1366x768 panel (panel coordinates): the big portraits, 9 a row (8 on
// the narrower west / east panel), one row shown at a time, scrolled by the
// mouse wheel and the arrows. A block is a 2 px black frame around the
// 106x122 portrait and the 3 bars (3 px).
#define FV_BUTTONS_DY    40 // the buttons are lower than on the old panel
#define FV_BLOCK_X      144
#define FV_BLOCK_Y       47
#define FV_BLOCK_W      123
#define FV_BLOCK_H      126
#define FV_BLOCK_STEP   133 // 10 px between the blocks
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

static INT32 PlacementPerRow() { return g_placement_panel == PANEL_WEST || g_placement_panel == PANEL_EAST ? 8 : 9; }
static INT32 PlacementRows() { return std::max<INT32>(1, (giPlacements + PlacementPerRow() - 1) / PlacementPerRow()); }
static bool  MercShown(INT32 const i) { return !gfPlacementFullView || i / PlacementPerRow() == giPlacementRow; }

// a merc's region in the panel (portrait and name)
static INT32 MercRegionX(INT32 const i) { return gfPlacementFullView ? PanelX() + FV_BLOCK_X + i % PlacementPerRow() * FV_BLOCK_STEP : PanelX() + 91 + i / 2 * 54; }
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
	// the 1366x768 panels: every text in FONT14ARIAL
	SGPFont const font = gfPlacementFullView ? FONT14ARIAL : BLOCKFONT;
	UINT8 const colour = gfPlacementFullView ? FONT_WHITE : FONT_BEIGE;
	btn->SpecifyGeneralTextAttributes(text, font, colour, 141);
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
	if (gfPlacementFullView && i >= 0 && !MercShown(i)) SetPlacementRow(i / PlacementPerRow());
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
	INT16 const bottom = PanelY() + TACTICAL_PLACEMENT_PANEL_HEIGHT; // the map is not covered
	MSYS_DefineRegion(&g_placement_panel_region, x, y, PanelX() + g_placement_panel_w, bottom, MSYS_PRIORITY_HIGH + 1, 0, MSYS_NO_CALLBACK, PlacementPanelRegionCallback);
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

	char const* panel = g_ui.isExtraWideStrategicScreen() ? UsablePanel(g_panel_north, g_full_view_panel) : 0;
	gfPlacementFullView = panel != 0;
	if (!panel) panel = INTERFACEDIR "/overheadinterface.sti";
	if (gfPlacementFullView)
	{
		// scrolling renders the whole world again (shading drawn over it)
		gfPlacementOldVideoScroll = gfDoVideoScroll;
		gfDoVideoScroll           = FALSE;
	}

	// The panel by the entry edges. Mercs entering from the south: the panel
	// goes to the top of the screen, over the north edge, else it would cover
	// most of their entry area. Only from the west (and the north) / only from
	// the east (and the north): the narrower panel, right / left aligned, keeps
	// that edge's bottom free. Else (north only, west and east) at the bottom.
	g_placement_panel   = PANEL_BOTTOM;
	g_placement_panel_w = 1366;
	bool east  = false;
	bool south = false;
	bool west  = false;
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
			switch (code)
			{
				case INSERTION_CODE_EAST:  east  = true; break;
				case INSERTION_CODE_SOUTH: south = true; break;
				case INSERTION_CODE_WEST:  west  = true; break;
			}
		}

		char const* const side_panel = west != east ? UsablePanel(west ? g_panel_west : g_panel_east, g_full_view_panel_side) : 0;
		if (south)
		{
			g_placement_panel = PANEL_TOP;
			if (char const* const top = UsablePanel(g_panel_south, g_full_view_panel)) panel = top;
		}
		else if (side_panel && side_panel != g_full_view_panel)
		{
			g_placement_panel = west ? PANEL_WEST : PANEL_EAST;
			panel             = side_panel;
		}
	}
	if (g_placement_panel != PANEL_BOTTOM) ExtendViewportToScreenBottom();
	if (gfPlacementFullView)
	{
		// the world scrolls out from under the panel: every edge of the map
		// can be brought into the free part of the view
		gsScrollTopExtra    = TacticalPlacementViewTop();
		gsScrollBottomExtra = std::max(0, gsVIEWPORT_END_Y - TacticalPlacementViewBottom());
	}

	GoIntoOverheadMap();

	giOverheadPanelImage = AddVideoObjectFromFile(panel);
	// 1366, the west / east ones 1207; taller with the minimap's box
	g_gfx               = PanelGfx{};
	g_placement_minimap = 0;
	if (gfPlacementFullView)
	{
		ETRLEObject const& panel_size = giOverheadPanelImage->SubregionProperties(0);
		INT16 const box_rows = panel_size.usHeight - TACTICAL_PLACEMENT_PANEL_HEIGHT;
		g_placement_panel_w = panel_size.usWidth;
		if (IsNewPanel(panel))
		{ // see PanelGfx
			bool const top = panel == g_panel_south;
			g_gfx.panel_y = top ? 0 : box_rows;
			g_gfx.box_w   = 648;
			g_gfx.box_h   = box_rows;
			g_gfx.box_x   = panel == g_panel_east ? 0 : panel_size.usWidth - g_gfx.box_w;
			g_gfx.box_y   = top ? TACTICAL_PLACEMENT_PANEL_HEIGHT : 0;
			g_gfx.map_dx  = 4;
			g_gfx.map_dy  = top ? 7 : 6;
			g_gfx.map_w   = 640;
			g_gfx.map_h   = 320;
		}
		else if (box_rows > 15)
		{ // an older one: the map 1 px inside its window at (4, 6), 8 px of frame below
			g_gfx.two_parts = true;
			g_gfx.panel_y   = box_rows;
			g_gfx.box_h     = box_rows;
			g_gfx.map_h     = box_rows - 15;
			g_gfx.map_w     = g_gfx.map_h * 2;
			g_gfx.box_w     = g_gfx.map_w + 10;
			g_gfx.box_x     = (panel_size.usWidth - g_gfx.box_w) / 2;
			g_gfx.map_dx    = 5;
			g_gfx.map_dy    = 7;
		}
		if (g_gfx.box_h != 0)
		{
			g_placement_minimap = LoadPlacementMinimap();
			if (g_placement_minimap)
			{
				ETRLEObject const& map = g_placement_minimap->SubregionProperties(0);
				g_minimap_w = map.usWidth;
				g_minimap_h = map.usHeight;
				if (g_minimap_w != MinimapPlaceW() || g_minimap_h != MinimapPlaceH())
				{
					SLOGW("The placement minimap is {}x{}, the panel's box holds {}x{}: cut to it", g_minimap_w, g_minimap_h, MinimapPlaceW(), MinimapPlaceH());
				}
			}
		}
	}
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

	if (gfPlacementFullView)
	{
		CreateFullViewScrolling();
		CreatePlacementMinimapRegion();
	}

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


// The entry areas in the 1:1 view: 166 px wide bands along the map's
// visible edges (where the scrolling stops, SCROLL_*_PADDING), the same
// for every edge (the screen's width beside the west / east panel + 7). Their
// inner edges in frame coordinates, from the render center as ApplyScrolling()
// places the view.
#define FV_ENTRY_BAND 166

struct FullViewEntryEdges
{
	INT16 north; // the north area ends above this row
	INT16 east;  // the east area starts at this column
	INT16 south; // the south area starts at this row
	INT16 west;  // the west area ends left of this column
};

static FullViewEntryEdges GetFullViewEntryEdges()
{
	INT32 const left = 2 * gsRenderCenterX - 2 * gsRenderCenterY - g_ui.m_tacticalMapCenterX; // the view's top left corner
	INT32 const top  = gsRenderCenterX + gsRenderCenterY - 10   - g_ui.m_tacticalMapCenterY;
	FullViewEntryEdges e;
	e.north = (INT16)(gsTopY    + SCROLL_TOP_PADDING    + FV_ENTRY_BAND - top);
	e.east  = (INT16)(gsRightX  + SCROLL_RIGHT_PADDING  - FV_ENTRY_BAND - left);
	e.south = (INT16)(gsBottomY + SCROLL_BOTTOM_PADDING - FV_ENTRY_BAND - top);
	e.west  = (INT16)(gsLeftX   + SCROLL_LEFT_PADDING   + FV_ENTRY_BAND - left);
	return e;
}


static UINT16 PlacementHatchColour()
{
	return
		DayTime() ? 0 :                     // 6AM to 9PM is black
		Get16BPPColor(FROMRGB(63, 31, 31)); // 9PM to 6AM is gray (black is too dark to distinguish)
}


// The 1:1 view: everything but the 166 px bands along the edges where the
// mercs may enter gets the hatch.
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

	FullViewEntryEdges const e = GetFullViewEntryEdges();
	if (north) top    = std::max(top,    e.north);
	if (west)  left   = std::max(left,   e.west);
	if (south) bottom = std::min(bottom, e.south);
	if (east)  right  = std::min(right,  e.east);
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
	BlitBufferToBuffer(buf, guiSAVEBUFFER, PanelX(), PanelY(), g_placement_panel_w, TACTICAL_PLACEMENT_PANEL_HEIGHT);
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
		// the world was rendered again this frame: everything over it again,
		// the shading first, under the panel
		gfTacticalPlacementGUIDirty = TRUE;
		gfValidLocationsChanged     = FALSE;
		ShadeFullViewInvalidArea();
	}

	SGPVSurface* const buf = FRAME_BUFFER;
	// If the display is dirty render the entire panel.
	if (gfTacticalPlacementGUIDirty)
	{
		if (gfPlacementFullView) DrawPlacementPanel(buf);
		else BltVideoObject(buf, giOverheadPanelImage, 0, PanelX(), PanelY());
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

		// the 1366x768 panels: FONT14ARIAL, white
		if (gfPlacementFullView) SetFontAttributes(FONT14ARIAL, FONT_WHITE);
		else                     SetFontAttributes(BLOCKFONT, FONT_BEIGE);
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
			gfPlacementFullView ? FONT_WHITE : FONT_GRAY3; // the 1366x768 panels: white
		SGPFont const font = gfPlacementFullView ? FONT14ARIAL : BLOCKFONT;
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
			if (gfPlacementFullView)
			{ // FONT14ARIAL like the other texts, in the portrait's middle
				SetFont(FONT14ARIAL);
				INT32 const fx = x + 2 + (FV_PORTRAIT_W - StringPixLength("?", FONT14ARIAL)) / 2;
				INT32 const fy = y + 2 + (FV_PORTRAIT_H - GetFontHeight(FONT14ARIAL)) / 2;
				MPrint(fx, fy, "?");
				InvalidateRegion(fx, fy, fx + 12, fy + 16);
			}
			else
			{
				SetFont(FONT10ARIALBOLD);
				MPrint(qx, qy, "?");
				InvalidateRegion(qx, qy, qx + 8, qy + 8);
			}
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
		TacticalPlacementViewTop() <= gusMouseYPos && gusMouseYPos < TacticalPlacementViewBottom() &&
		!TacticalPlacementMouseOverPanel() :
		(gusMouseYPos >= STD_SCREEN_Y) && (gusMouseYPos < STD_SCREEN_Y + 320) &&
		(gusMouseXPos >= STD_SCREEN_X) && (gusMouseXPos < STD_SCREEN_X + 640);
	if (gbSelectedMercID != -1 && mouse_in_map)
	{
		// the mouse in the overhead map's own coordinates
		INT16 const mx = gusMouseXPos - STD_SCREEN_X;
		INT16 const my = gusMouseYPos - STD_SCREEN_Y;
		UINT8 const code = gMercPlacement[gbCursorMercID].ubStrategicInsertionCode;
		if (gfPlacementFullView)
		{
			// the 166 px bands (see GetFullViewEntryEdges())
			FullViewEntryEdges const e = GetFullViewEntryEdges();
			switch (code)
			{
				case INSERTION_CODE_NORTH: gfValidCursor = gusMouseYPos <  e.north; break;
				case INSERTION_CODE_EAST:  gfValidCursor = gusMouseXPos >= e.east;  break;
				case INSERTION_CODE_SOUTH: gfValidCursor = gusMouseYPos >= e.south; break;
				case INSERTION_CODE_WEST:  gfValidCursor = gusMouseXPos <  e.west;  break;
			}
		}
		else switch (code)
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
	// before the tactical panel sets its viewport again
	RestoreViewport();
	gsScrollTopExtra    = 0;
	gsScrollBottomExtra = 0;

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
	if (g_placement_minimap)
	{
		DeleteVideoObject(g_placement_minimap);
		g_placement_minimap = 0;
	}
	DeleteVideoObject(giMercPanelImage);
	//Delete buttons
	for (INT32 i = 0; i < NUM_TP_BUTTONS; ++i)
	{
		UnloadButtonImage( giOverheadButtonImages[ i ] );
		RemoveButton( iTPButtons[ i ] );
	}
	if (gfPlacementFullView) RemoveFullViewScrolling();
	RemovePlacementMinimapRegion();
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
	// beside a narrower panel the map's region is under the panel too
	if (TacticalPlacementMouseOverPanel()) return;

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
