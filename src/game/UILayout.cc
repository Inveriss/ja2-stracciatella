#include "UILayout.h"

#include "ContentManager.h"
#include "Directories.h"
#include "GameInstance.h"
#include "GamePolicy.h"
#include "Interface.h"
#include "JAScreens.h"
#include "MapScreen.h"
#include "ScreenIDs.h"
#include "Soldier_Control.h"
#include <algorithm>
#include <stdexcept>
#include <string_theory/string>

#define MIN_INTERFACE_WIDTH             1024  // absolute minimum width, both strategic-screen asset variants
#define MIN_INTERFACE_HEIGHT            720   // absolute minimum height == height of the compact strategic-screen asset variant
#define LARGE_STRATEGIC_SCREEN_HEIGHT   768   // screens at/above this height use the large strategic-screen asset variant instead of the compact one
// WIDE_STRATEGIC_SCREEN_WIDTH / EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH /
// MAP_LEGACY_CANVAS_WIDTH live in UILayout.h, since the strategic screen
// code needs them too.

/**
 * Default screen layout.
 * It might be changed later when the window size is known for sure. */
UILayout g_ui(MIN_INTERFACE_WIDTH, MIN_INTERFACE_HEIGHT);


/** Constructor. */
UILayout::UILayout(UINT16 screenWidth, UINT16 screenHeight)
	:m_mapScreenWidth(MIN_INTERFACE_WIDTH), m_mapScreenHeight(MIN_INTERFACE_HEIGHT),
	m_screenWidth(screenWidth), m_screenHeight(screenHeight), m_bigTeamPanel(false), m_lowTeamPanel(false), m_watchBar(false), m_bigSMPanel(false), m_smLargeIcons(true)
{
}


void UILayout::setScreenSize(UINT16 width, UINT16 height)
{
	if (width < MIN_INTERFACE_WIDTH || height < MIN_INTERFACE_HEIGHT)
	{
		ST::string err = ST::format("Failed to set screen resolution {} x {}", width, height);
		throw std::runtime_error(err.to_std_string());
	}
	m_screenWidth = width;
	m_screenHeight = height;
}


/** Check if the screen is bigger than the currently active strategic-screen canvas size (m_mapScreenWidth x m_mapScreenHeight). */
bool UILayout::isBigScreen() const
{
	return (m_screenWidth > m_mapScreenWidth) || (m_screenHeight > m_mapScreenHeight);
}


/** True when the active resolution uses the compact (720px-tall) strategic screen asset set instead of the large (768px-tall) one. */
bool UILayout::isCompactStrategicScreen() const
{
	return m_mapScreenHeight < LARGE_STRATEGIC_SCREEN_HEIGHT;
}


/** True when the active resolution uses the wide (1280px) strategic screen canvas -- left column left-aligned, map/MBS block shifted right by MAP_RIGHT_BLOCK_SHIFT. */
bool UILayout::isWideStrategicScreen() const
{
	return m_mapScreenWidth >= WIDE_STRATEGIC_SCREEN_WIDTH;
}


/** True when the active resolution uses the 1366px wide strategic screen canvas (the 1366x768 base). */
bool UILayout::isExtraWideStrategicScreen() const
{
	return m_mapScreenWidth >= EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH;
}


UINT16 UILayout::currentHeight() const             { return fInMapMode ? (MAP_SCREEN_Y + m_mapScreenHeight) : m_screenHeight; }
// Tactical (non-map) branch anchored to the right edge of whichever of the
// two bottom-bar panels is actually on screen right now: m_smPanelWidth
// (floored to fit inventory_bottom_panel.sti) while the single-merc panel is
// shown, m_teamPanelWidth (purely squad-size-driven, matching
// bottom_bar.sti's own tiling) while the team panel is shown. The clock and
// minimap are shared, single widgets drawn regardless of which panel is
// active, so their position must follow whichever panel is currently
// rendered -- see the comment above SM_DONE_X in Interface_Panels.cc.
// Equivalent to the old formula for squads >= ~11 (where m_smPanelWidth ==
// m_teamPanelWidth, since neither is floored there): 142 - 56 = 86, 142 - 45 = 97.
UINT16 UILayout::get_CLOCK_X() const
{
	if (fInMapMode) return MAP_SCREEN_RIGHT - 86;
	// the watched battle's bar: its clock box
	if (isWatchBarShown()) return WATCH_BAR_CLOCK_X;
	// SM_PANEL offset shifted 3px right (86 -> 83) to match
	// inventory_bottom_panel.sti's latest graphic; TEAM_PANEL (bottom_bar.sti)
	// is untouched.
	// the big SM panel's time box: x 11, 74 wide
	if (gsCurInterfacePanel == SM_PANEL && m_bigSMPanel) return m_teamPanelPosition.iX + 11 + (74 - get_CLOCK_WIDTH()) / 2;
	if (gsCurInterfacePanel == SM_PANEL) return m_teamPanelPosition.iX + m_smPanelWidth - 83;
	// centered on the big team panel's time box (x 1034, 72 wide; 73 on the low one)
	if (m_bigTeamPanel) return 1034 + (72 - get_CLOCK_WIDTH()) / 2;
	return m_teamPanelPosition.iX + m_teamPanelWidth - 86;
}
UINT16 UILayout::get_CLOCK_Y() const
{
	// the watched battle's bar: its clock box, 20 high (clock 13)
	if (isWatchBarShown()) return m_screenHeight - WATCH_BAR_HEIGHT + WATCH_BAR_CLOCK_Y + (WATCH_BAR_BOX_HEIGHT - 13) / 2;
	// the big team panel's time box: y 163 (113 on the low one), 22 high (clock 13)
	if (isBigTeamPanelShown()) return m_screenHeight - BIG_TEAMPANEL_HEIGHT + (isLowTeamPanel() ? 113 : 163) + (22 - 13) / 2;
	// the big SM panel's time box: y 175 (under its top strip), 22 high
	if (isBigSMPanelShown()) return get_INV_INTERFACE_START_Y() + BIG_SMPANEL_TOP + 138 + (22 - 13) / 2;
	return currentHeight() - 23;
}
// The big team panel's clock (FONT12ARIAL, "Day 31, 23:59" is 68 px) fills
// its 72 px box.
UINT16 UILayout::get_CLOCK_WIDTH() const           { return isWatchBarShown() ? WATCH_BAR_CLOCK_WIDTH : isBigTeamPanelShown() || isBigSMPanelShown() ? 72 : 66; }
// Big team panel: the radar window (x 1121, y 76 -- 26 on the low one --, 240x120) -- the 240x120
// map, or the 88x44 one in its top left corner when the sector has no big
// one (Radar_Screen.h's RadarUsesPanelMap()).
UINT16 UILayout::get_RADAR_WINDOW_X() const
{
	if (fInMapMode) return MAP_SCREEN_RIGHT - 97;
	// SM_PANEL offset shifted 3px right (97 -> 94) -- see get_CLOCK_X().
	// the big SM panel has no radar window: its radar map opens from a button
	// (the overhead map) -- off the screen
	if (gsCurInterfacePanel == SM_PANEL && m_bigSMPanel) return 60000;
	if (gsCurInterfacePanel == SM_PANEL) return m_teamPanelPosition.iX + m_smPanelWidth - 94;
	if (isWatchBar()) return WATCH_BAR_RADAR_X;
	if (m_bigTeamPanel) return 1121;
	return m_teamPanelPosition.iX + m_teamPanelWidth - 97;
}
UINT16 UILayout::get_RADAR_WINDOW_TM_Y() const
{
	if (isWatchBarShown()) return m_screenHeight - WATCH_BAR_HEIGHT + WATCH_BAR_RADAR_Y;
	if (isBigTeamPanelShown()) return m_screenHeight - BIG_TEAMPANEL_HEIGHT + (isLowTeamPanel() ? 26 : 76);
	return currentHeight() - 107;
}
bool UILayout::isBigTeamPanelShown() const         { return !fInMapMode && m_bigTeamPanel && gsCurInterfacePanel != SM_PANEL; }
bool UILayout::isBigSMPanelShown() const           { return !fInMapMode && m_bigSMPanel && gsCurInterfacePanel == SM_PANEL; }
void UILayout::setSMLargeIcons(bool const on)
{
	m_smLargeIcons = on;
	recalculatePositions();
}
void UILayout::setLowTeamPanel(bool const low)
{
	m_lowTeamPanel = low && GCM && GCM->doesGameResExists(INTERFACEDIR "/" BIG_TEAMPANEL_LOW_FILE);
}
void UILayout::setWatchBar(bool const on)
{
	bool const want = on && m_screenWidth == 1366 && m_screenHeight == 768 &&
		GCM && GCM->doesGameResExists(INTERFACEDIR "/" WATCH_BAR_FILE);
	if (want == m_watchBar) return;
	m_watchBar = want;
	recalculatePositions(); // the viewport's bottom
}
UINT16 UILayout::get_INV_INTERFACE_START_Y() const { return m_screenHeight - INV_INTERFACE_HEIGHT;                                  }
UINT16 UILayout::get_ITEMDESC_PANEL_START_Y() const { return m_screenHeight - ITEMDESC_PANEL_HEIGHT;                                 }
UINT16 UILayout::get_ITEMDESC_PANEL_START_Y_MONEY() const { return m_screenHeight - ITEMDESC_PANEL_HEIGHT_MONEY;                       }
UINT16 UILayout::get_ITEMDESC_PANEL_START_Y_ITEMS() const { return m_screenHeight - ITEMDESC_PANEL_HEIGHT_ITEMS;                       }


void UILayout::recalculatePositions()
{
	// The 1366x768 base (same test as isExtraWideStrategicScreen(), which isn't
	// known yet here) gets its own team panel, when its graphic is there.
	m_bigTeamPanel = m_screenWidth >= EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH &&
		m_screenHeight >= LARGE_STRATEGIC_SCREEN_HEIGHT &&
		GCM && GCM->doesGameResExists(INTERFACEDIR "/" BIG_TEAMPANEL_FILE);

	// Everything below stays squad-size-driven, also with the big team panel:
	// the SM panel and its widgets keep their old positions.
	m_teamPanelSlotsTotalWidth = getOldTeamPanelNumSlots() * TEAMPANEL_SLOT_WIDTH;
	UINT16 tpXOffset = (m_screenWidth - m_teamPanelSlotsTotalWidth - TEAMPANEL_BUTTONSBOX_WIDTH) / 2;
	UINT16 tpYOffset = m_screenHeight - TEAMPANEL_HEIGHT;
	m_teamPanelPosition.set(tpXOffset, tpYOffset);
	// Purely squad-size-driven -- do NOT floor this to fit
	// inventory_bottom_panel.sti (see m_smPanelWidth below for that). This
	// field also drives bottom_bar.sti's own tiling (which already scales
	// correctly with squad size on its own) and the shared, squad-size-driven
	// widgets (minimap, clock, sector name) that must keep tracking real
	// squad size regardless of which of the two panels is currently shown.
	m_teamPanelWidth = m_teamPanelSlotsTotalWidth + TEAMPANEL_BUTTONSBOX_WIDTH;
	// Single-merc panel canvas width: must never be narrower than
	// inventory_bottom_panel.sti itself (INVENTORY_BOTTOM_PANEL_WIDTH) --
	// otherwise, for squads smaller than ~11 mercs, guiSMPanel's canvas
	// would be too narrow to show the new inventory slots at the right edge
	// of the graphic, clipping them off-canvas. Use this (not
	// m_teamPanelWidth) for anything specific to the single-merc panel's own
	// canvas/buttons.
	m_smPanelWidth = std::max<UINT16>(m_teamPanelWidth, INVENTORY_BOTTOM_PANEL_WIDTH);

	// The 1366x768 SM panel: one fixed 1366 px piece at the left edge, also
	// independent of the squad size -- unless its Show Large Icons toggle chose
	// the old panel: also at the left edge, 1366 px wide (the user's
	// inventory_bottom_panel.png is that wide; its right block -- Done, Map
	// Screen, radar map, sector name, clock -- stays anchored to the right
	// edge), also squad size independent.
	m_bigSMPanel = m_bigTeamPanel && m_smLargeIcons && GCM->doesGameResExists(INTERFACEDIR "/" BIG_SMPANEL_FILE);
	if (m_bigSMPanel)
	{
		m_teamPanelPosition.iX = 0;
		m_smPanelWidth         = BIG_SMPANEL_WIDTH;
	}
	else if (m_bigTeamPanel)
	{
		m_teamPanelPosition.iX = 0;
		m_smPanelWidth         = BIG_SMPANEL_WIDTH;
	}

	UINT16 startInvY = get_INV_INTERFACE_START_Y();
	UINT16 startX    = INTERFACE_START_X;

	// Resolve which strategic-screen asset variant is active for the current
	// resolution -- see isCompactStrategicScreen() / LARGE_STRATEGIC_SCREEN_HEIGHT.
	// Width is its own, independent axis: screens at least
	// WIDE_STRATEGIC_SCREEN_WIDTH wide get the 1280px wide canvas, and those
	// at least EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH wide on the large height tier
	// (the 1366x768 base) the 1366px wide one -- centered on anything wider,
	// like the 1024 one; everything narrower keeps the legacy 1024px canvas.
	// The compact tier (the 1280x720 base) stays on 1280 at any width.
	m_mapScreenHeight            = (m_screenHeight >= LARGE_STRATEGIC_SCREEN_HEIGHT)
	                                     ? LARGE_STRATEGIC_SCREEN_HEIGHT
	                                     : MIN_INTERFACE_HEIGHT;
	if (m_screenWidth >= EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH && !isCompactStrategicScreen())
	{
		m_mapScreenWidth = EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH;
	}
	else
	{
		m_mapScreenWidth = (m_screenWidth >= WIDE_STRATEGIC_SCREEN_WIDTH)
		                       ? WIDE_STRATEGIC_SCREEN_WIDTH
		                       : MAP_LEGACY_CANVAS_WIDTH;
	}

	// STD_SCREEN_X/Y centers the legacy 640x480 window (everything except the
	// strategic map -- see STD_SCREEN_X/Y / STD_SCREEN_WIDTH/HEIGHT in UILayout.h).
	m_stdScreenOffsetX            = (m_screenWidth - STD_SCREEN_WIDTH) / 2;
	m_stdScreenOffsetY            = (m_screenHeight - STD_SCREEN_HEIGHT) / 2;
	// MAP_SCREEN_X/Y centers the strategic map's own, larger canvas.
	m_mapScreenOffsetX            = (m_screenWidth - m_mapScreenWidth) / 2;
	m_mapScreenOffsetY            = (m_screenHeight - m_mapScreenHeight) / 2;
	// MAP_SCREEN_RIGHT_BLOCK_X: the map/MBS/sector-inventory block. Same as
	// MAP_SCREEN_X on the legacy 1024 canvas; shifted right on the wide one,
	// opening the free space between the left column and MBS: 256 px on the
	// 1280 canvas, 340 px (1366 - 262 - 764) on the 1366 one.
	m_mapRightBlockShift          = isWideStrategicScreen() ? m_mapScreenWidth - MAP_LEGACY_CANVAS_WIDTH : 0;
	m_mapRightBlockOffsetX        = m_mapScreenOffsetX + m_mapRightBlockShift;

	// tactical screen inventory position
	m_invSlotPositionTac[HELMETPOS           ].set(startX + 431, startInvY +   8);
	m_invSlotPositionTac[VESTPOS             ].set(startX + 431, startInvY +  47);
	m_invSlotPositionTac[LEGPOS              ].set(startX + 431, startInvY +  116);
	m_invSlotPositionTac[HEAD1POS            ].set(startX + 230, startInvY +   8);
	m_invSlotPositionTac[HEAD2POS            ].set(startX + 230, startInvY +  43);
	m_invSlotPositionTac[HANDPOS             ].set(startX + 303, startInvY +  83);
	m_invSlotPositionTac[SECONDHANDPOS       ].set(startX + 303, startInvY + 118);
	m_invSlotPositionTac[BIGPOCK1POS         ].set(startX + 679, startInvY +   80);
	m_invSlotPositionTac[BIGPOCK2POS         ].set(startX + 679, startInvY +  116);
	m_invSlotPositionTac[BIGPOCK3POS         ].set(startX + 772, startInvY +  8);
	m_invSlotPositionTac[BIGPOCK4POS         ].set(startX + 772, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK1POS       ].set(startX + 491, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK2POS       ].set(startX + 491, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK3POS       ].set(startX + 491, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK4POS       ].set(startX + 491, startInvY +  116);
	m_invSlotPositionTac[SMALLPOCK5POS       ].set(startX + 538, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK6POS       ].set(startX + 538, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK7POS       ].set(startX + 538, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK8POS       ].set(startX + 538, startInvY +  116);

	// TODO: placeholder positions for the 20 new slots (HEAD3/4, BIGPOCK5-10,
	// SMALLPOCK9-20) added for the inventory expansion -- replace with real
	// coordinates once the redesigned inventory_bottom_panel.sti layout is final.
	m_invSlotPositionTac[HEAD3POS            ].set(startX + 277, startInvY + 8);
	m_invSlotPositionTac[HEAD4POS            ].set(startX + 277, startInvY + 43);
	m_invSlotPositionTac[BIGPOCK5POS         ].set(startX + 772, startInvY + 80);
	m_invSlotPositionTac[BIGPOCK6POS         ].set(startX + 772, startInvY + 116);
	m_invSlotPositionTac[BIGPOCK7POS         ].set(startX + 848, startInvY + 8);
	m_invSlotPositionTac[BIGPOCK8POS         ].set(startX + 848, startInvY + 44);
	m_invSlotPositionTac[BIGPOCK9POS         ].set(startX + 848, startInvY + 80);
	m_invSlotPositionTac[BIGPOCK10POS        ].set(startX + 848, startInvY + 116);
	m_invSlotPositionTac[SMALLPOCK9POS       ].set(startX + 585, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK10POS      ].set(startX + 585, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK11POS      ].set(startX + 585, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK12POS      ].set(startX + 585, startInvY +  116);
	m_invSlotPositionTac[SMALLPOCK13POS      ].set(startX + 632, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK14POS      ].set(startX + 632, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK15POS      ].set(startX + 632, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK16POS      ].set(startX + 632, startInvY +  116);
	m_invSlotPositionTac[SMALLPOCK17POS      ].set(startX + 679, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK18POS      ].set(startX + 679, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK19POS      ].set(startX + 726, startInvY +  8);
	m_invSlotPositionTac[SMALLPOCK20POS      ].set(startX + 726, startInvY +  44);

	if (m_bigSMPanel)
	{
		// inventory_bottom_panel_big_1366x768.png: the old panel's layout, its
		// slots bigger (sizes: gSMInvDataTacBig, Interface_Items.cc)
		auto const at = [&](INT32 const slot, UINT16 const x, UINT16 const y) { m_invSlotPositionTac[slot].set(startX + x, startInvY + BIG_SMPANEL_TOP + y); };
		at(HEAD1POS,  257,  10); at(HEAD2POS,  257,  65);
		at(HEAD3POS,  330,  10); at(HEAD4POS,  330,  65);
		at(HANDPOS,   330, 125); at(SECONDHANDPOS, 330, 180);
		at(HELMETPOS, 516,  15); at(VESTPOS,   516,  75); at(LEGPOS, 516, 180);
		UINT16 const row[] = { 15, 70, 125, 180 };
		for (INT32 i = 0; i != 4; ++i)
		{
			at(SMALLPOCK1POS  + i, 596, row[i]);
			at(SMALLPOCK5POS  + i, 679, row[i]);
			at(SMALLPOCK9POS  + i, 762, row[i]);
			at(SMALLPOCK13POS + i, 845, row[i]);
			at(BIGPOCK3POS    + i, 1094, row[i]);
			at(BIGPOCK7POS    + i, 1227, row[i]);
		}
		at(SMALLPOCK17POS, 928, 15); at(SMALLPOCK18POS, 928, 70);
		at(SMALLPOCK19POS, 1011, 15); at(SMALLPOCK20POS, 1011, 70);
		at(BIGPOCK1POS,    928, 125); at(BIGPOCK2POS,    928, 180);
	}

	// map screen inventory position -- MAP_INV_SHIFT_Y lower on the 1366x768
	// interface (see UILayout.h)
	UINT16 const invY = m_mapScreenOffsetY + MAP_INV_SHIFT_Y;
	m_invSlotPositionMap[HELMETPOS           ].set(m_mapScreenOffsetX + 216, invY + 115);
	m_invSlotPositionMap[VESTPOS             ].set(m_mapScreenOffsetX + 216, invY + 154);
	m_invSlotPositionMap[LEGPOS              ].set(m_mapScreenOffsetX + 216, invY + 223);
	m_invSlotPositionMap[HEAD1POS            ].set(m_mapScreenOffsetX +  15, invY + 115);
	m_invSlotPositionMap[HEAD2POS            ].set(m_mapScreenOffsetX +  15, invY + 150);
	m_invSlotPositionMap[HANDPOS             ].set(m_mapScreenOffsetX +  88, invY + 190);
	m_invSlotPositionMap[SECONDHANDPOS       ].set(m_mapScreenOffsetX +  88, invY + 225);
	m_invSlotPositionMap[BIGPOCK1POS         ].set(m_mapScreenOffsetX +  15, invY + 486);
	m_invSlotPositionMap[BIGPOCK2POS         ].set(m_mapScreenOffsetX +  15, invY + 522);
	m_invSlotPositionMap[BIGPOCK3POS         ].set(m_mapScreenOffsetX +  109, invY + 414);
	m_invSlotPositionMap[BIGPOCK4POS         ].set(m_mapScreenOffsetX +  109, invY + 450);
	m_invSlotPositionMap[SMALLPOCK1POS       ].set(m_mapScreenOffsetX +  15, invY + 270);
	m_invSlotPositionMap[SMALLPOCK2POS       ].set(m_mapScreenOffsetX +  15, invY + 306);
	m_invSlotPositionMap[SMALLPOCK3POS       ].set(m_mapScreenOffsetX +  15, invY + 342);
	m_invSlotPositionMap[SMALLPOCK4POS       ].set(m_mapScreenOffsetX +  15, invY + 378);
	m_invSlotPositionMap[SMALLPOCK5POS       ].set(m_mapScreenOffsetX +  62, invY + 270);
	m_invSlotPositionMap[SMALLPOCK6POS       ].set(m_mapScreenOffsetX +  62, invY + 306);
	m_invSlotPositionMap[SMALLPOCK7POS       ].set(m_mapScreenOffsetX +  62, invY + 342);
	m_invSlotPositionMap[SMALLPOCK8POS       ].set(m_mapScreenOffsetX +  62, invY + 378);

	m_invSlotPositionMap[HEAD3POS            ].set(m_mapScreenOffsetX +  62, invY + 115);
	m_invSlotPositionMap[HEAD4POS            ].set(m_mapScreenOffsetX +  62, invY + 150);
	m_invSlotPositionMap[BIGPOCK5POS         ].set(m_mapScreenOffsetX +  109, invY + 486);
	m_invSlotPositionMap[BIGPOCK6POS         ].set(m_mapScreenOffsetX +  109, invY + 522);
	m_invSlotPositionMap[BIGPOCK7POS         ].set(m_mapScreenOffsetX +  185, invY + 414);
	m_invSlotPositionMap[BIGPOCK8POS         ].set(m_mapScreenOffsetX +  185, invY + 450);
	m_invSlotPositionMap[BIGPOCK9POS         ].set(m_mapScreenOffsetX +  185, invY + 486);
	m_invSlotPositionMap[BIGPOCK10POS        ].set(m_mapScreenOffsetX +  185, invY + 522);
	m_invSlotPositionMap[SMALLPOCK9POS       ].set(m_mapScreenOffsetX +  109, invY + 270);
	m_invSlotPositionMap[SMALLPOCK10POS      ].set(m_mapScreenOffsetX +  109, invY + 306);
	m_invSlotPositionMap[SMALLPOCK11POS      ].set(m_mapScreenOffsetX +  109, invY + 342);
	m_invSlotPositionMap[SMALLPOCK12POS      ].set(m_mapScreenOffsetX +  109, invY + 378);
	m_invSlotPositionMap[SMALLPOCK13POS      ].set(m_mapScreenOffsetX + 156, invY + 270);
	m_invSlotPositionMap[SMALLPOCK14POS      ].set(m_mapScreenOffsetX + 156, invY + 306);
	m_invSlotPositionMap[SMALLPOCK15POS      ].set(m_mapScreenOffsetX + 156, invY + 342);
	m_invSlotPositionMap[SMALLPOCK16POS      ].set(m_mapScreenOffsetX + 156, invY + 378);
	m_invSlotPositionMap[SMALLPOCK17POS      ].set(m_mapScreenOffsetX + 15, invY + 414);
	m_invSlotPositionMap[SMALLPOCK18POS      ].set(m_mapScreenOffsetX + 15, invY + 450);
	m_invSlotPositionMap[SMALLPOCK19POS      ].set(m_mapScreenOffsetX + 62, invY + 414);
	m_invSlotPositionMap[SMALLPOCK20POS      ].set(m_mapScreenOffsetX + 62, invY + 450);

	// map screen inventory position -- "Show Large Icons" mode
	// (MapInvBigImagesBtnCallback(), MapScreen.cc; background
	// mapinv_big_1280_720/768.sti, wide strategic screen only). Independent of the
	// table above; starts as a copy of it -- tune to the big graphic's slots.
	// Slot sizes for this mode: gSMInvDataMapBig[] in Interface_Items.cc.
	m_invSlotPositionMapBig[HELMETPOS           ].set(m_mapScreenOffsetX + 290, invY + 132);
	m_invSlotPositionMapBig[VESTPOS             ].set(m_mapScreenOffsetX + 290, invY + 194);
	m_invSlotPositionMapBig[LEGPOS              ].set(m_mapScreenOffsetX + 290, invY + 304);
	m_invSlotPositionMapBig[HEAD1POS            ].set(m_mapScreenOffsetX +  31, invY + 132);
	m_invSlotPositionMapBig[HEAD2POS            ].set(m_mapScreenOffsetX +  31, invY + 187);
	m_invSlotPositionMapBig[HANDPOS             ].set(m_mapScreenOffsetX +  104, invY + 249);
	m_invSlotPositionMapBig[SECONDHANDPOS       ].set(m_mapScreenOffsetX +  104, invY + 304);
	m_invSlotPositionMapBig[BIGPOCK1POS         ].set(m_mapScreenOffsetX +  197, invY + 595);
	m_invSlotPositionMapBig[BIGPOCK2POS         ].set(m_mapScreenOffsetX +  197, invY + 650);
	m_invSlotPositionMapBig[BIGPOCK3POS         ].set(m_mapScreenOffsetX +  372, invY + 256);
	m_invSlotPositionMapBig[BIGPOCK4POS         ].set(m_mapScreenOffsetX +  372, invY + 311);
	m_invSlotPositionMapBig[SMALLPOCK1POS       ].set(m_mapScreenOffsetX +  31, invY + 366);
	m_invSlotPositionMapBig[SMALLPOCK2POS       ].set(m_mapScreenOffsetX +  31, invY + 421);
	m_invSlotPositionMapBig[SMALLPOCK3POS       ].set(m_mapScreenOffsetX +  31, invY + 476);
	m_invSlotPositionMapBig[SMALLPOCK4POS       ].set(m_mapScreenOffsetX +  31, invY + 531);
	m_invSlotPositionMapBig[SMALLPOCK5POS       ].set(m_mapScreenOffsetX +  114, invY + 366);
	m_invSlotPositionMapBig[SMALLPOCK6POS       ].set(m_mapScreenOffsetX +  114, invY + 421);
	m_invSlotPositionMapBig[SMALLPOCK7POS       ].set(m_mapScreenOffsetX +  114, invY + 476);
	m_invSlotPositionMapBig[SMALLPOCK8POS       ].set(m_mapScreenOffsetX +  114, invY + 531);
	m_invSlotPositionMapBig[HEAD3POS            ].set(m_mapScreenOffsetX +  104, invY + 132);
	m_invSlotPositionMapBig[HEAD4POS            ].set(m_mapScreenOffsetX +  104, invY + 187);
	m_invSlotPositionMapBig[BIGPOCK5POS         ].set(m_mapScreenOffsetX +  372, invY + 366);
	m_invSlotPositionMapBig[BIGPOCK6POS         ].set(m_mapScreenOffsetX +  372, invY + 421);
	m_invSlotPositionMapBig[BIGPOCK7POS         ].set(m_mapScreenOffsetX +  372, invY + 485);
	m_invSlotPositionMapBig[BIGPOCK8POS         ].set(m_mapScreenOffsetX +  372, invY + 540);
	m_invSlotPositionMapBig[BIGPOCK9POS         ].set(m_mapScreenOffsetX +  372, invY + 595);
	m_invSlotPositionMapBig[BIGPOCK10POS        ].set(m_mapScreenOffsetX +  372, invY + 650);
	m_invSlotPositionMapBig[SMALLPOCK9POS       ].set(m_mapScreenOffsetX +  197, invY + 366);
	m_invSlotPositionMapBig[SMALLPOCK10POS      ].set(m_mapScreenOffsetX +  197, invY + 421);
	m_invSlotPositionMapBig[SMALLPOCK11POS      ].set(m_mapScreenOffsetX +  197, invY + 476);
	m_invSlotPositionMapBig[SMALLPOCK12POS      ].set(m_mapScreenOffsetX +  197, invY + 531);
	m_invSlotPositionMapBig[SMALLPOCK13POS      ].set(m_mapScreenOffsetX + 280, invY + 366);
	m_invSlotPositionMapBig[SMALLPOCK14POS      ].set(m_mapScreenOffsetX + 280, invY + 421);
	m_invSlotPositionMapBig[SMALLPOCK15POS      ].set(m_mapScreenOffsetX + 280, invY + 476);
	m_invSlotPositionMapBig[SMALLPOCK16POS      ].set(m_mapScreenOffsetX + 280, invY + 531);
	m_invSlotPositionMapBig[SMALLPOCK17POS      ].set(m_mapScreenOffsetX + 31, invY + 595);
	m_invSlotPositionMapBig[SMALLPOCK18POS      ].set(m_mapScreenOffsetX + 31, invY + 650);
	m_invSlotPositionMapBig[SMALLPOCK19POS      ].set(m_mapScreenOffsetX + 114, invY + 595);
	m_invSlotPositionMapBig[SMALLPOCK20POS      ].set(m_mapScreenOffsetX + 114, invY + 650);

	// The helicopter's panels: its 8 cargo slots (IsHelicopterCargoSlot(),
	// Vehicles.cc) -- small pockets 1-4 and big pockets 1-4, one column each,
	// row step 55 -- nothing else
	for (INT32 i = 0; i != NUM_INVENTORY_SLOTS; ++i)
	{
		m_invSlotPositionTacHeli[i].set(INV_SLOT_HIDDEN_POS, INV_SLOT_HIDDEN_POS);
		m_invSlotPositionMapHeli[i].set(INV_SLOT_HIDDEN_POS, INV_SLOT_HIDDEN_POS);
	}
	// heli_tactical_inv_1366x768.png: slot = the field (as the big SM panel)
	UINT16 const tacHeliY = startInvY + BIG_SMPANEL_TOP;
	// heli_strategic_inv_1366x768.png at PLAYER_INFO_Y (invY + 107): slot =
	// the field 1 px in (as mapinv_1366x768.png)
	UINT16 const mapHeliY = invY + 107;
	for (INT32 i = 0; i != 4; ++i)
	{
		m_invSlotPositionTacHeli[SMALLPOCK1POS + i].set(startX + 1144, tacHeliY + 15 + 55 * i);
		m_invSlotPositionTacHeli[BIGPOCK1POS   + i].set(startX + 1227, tacHeliY + 15 + 55 * i);
		m_invSlotPositionMapHeli[SMALLPOCK1POS + i].set(m_mapScreenOffsetX + 469, mapHeliY +  17 + 55 * i);
		m_invSlotPositionMapHeli[BIGPOCK1POS   + i].set(m_mapScreenOffsetX + 469, mapHeliY + 254 + 55 * i);
	}

	// the big SM panel has no place for the body figure yet: off the screen
	if (m_bigSMPanel) m_invCamoRegion.set(60000, 60000);
	else m_invCamoRegion.set(SM_BODYINV_X, SM_BODYINV_Y);

	m_progress_bar_box.set(STD_SCREEN_X + 5, 2, STD_SCREEN_WIDTH - 10, 12);
	// Uses ITEMDESC_PANEL_START_Y_MONEY (not startInvY/INV_INTERFACE_START_Y,
	// and not the weapon-only ITEMDESC_PANEL_START_Y): the money buttons are
	// an overlay drawn inside the Infobox_money.sti popup specifically (see
	// gMoneyButtonLoc in Interface_Items.cc), which now has its own,
	// independent vertical anchor -- see ITEMDESC_PANEL_START_Y_MONEY in
	// UILayout.h. Anchoring this to the wrong one (or the wrong panel's
	// height) leaves the buttons offset from wherever the popup actually is.
	m_moneyButtonLoc.set(startX + 343, get_ITEMDESC_PANEL_START_Y_MONEY() + 11);
	m_MoneyButtonLocMap.set(m_mapScreenOffsetX + 174, m_mapScreenOffsetY + MAP_INV_SHIFT_Y + 115);

	m_VIEWPORT_START_X            = 0;
	m_VIEWPORT_START_Y            = 0;
	m_VIEWPORT_WINDOW_START_Y     = 0;
	m_VIEWPORT_END_X              = m_screenWidth;
	// the watched battle's bar is low: the world is shown down to it
	UINT16 const viewport_end_y   = m_screenHeight - (isWatchBar() ? WATCH_BAR_HEIGHT : 120);
	m_VIEWPORT_END_Y              = viewport_end_y;
	m_VIEWPORT_WINDOW_END_Y       = viewport_end_y;
	m_tacticalMapCenterX          = (m_VIEWPORT_END_X - m_VIEWPORT_START_X) / 2;
	m_tacticalMapCenterY          = (m_VIEWPORT_END_Y - m_VIEWPORT_START_Y) / 2;

	m_worldClippingRect.set(0, 0, m_screenWidth, viewport_end_y);

	m_contractPosition.set(       m_mapScreenOffsetX + 120, m_mapScreenOffsetY +  50);
	m_attributePosition.set(      m_mapScreenOffsetX + 220, m_mapScreenOffsetY + 150);
	m_trainPosition.set(          m_mapScreenOffsetX + 160, m_mapScreenOffsetY + 150);
	m_vehiclePosition.set(        m_mapScreenOffsetX + 160, m_mapScreenOffsetY + 150);
	m_repairPosition.set(         m_mapScreenOffsetX + 160, m_mapScreenOffsetY + 150);
	// right of the team list's assignment column, 2 px from it like on the
	// original panel (67-118 -> 120): 1366x768 column 117-218 -> 221
	m_assignmentPosition.set(     m_mapScreenOffsetX + (isExtraWideStrategicScreen() ? 221 : 120), m_mapScreenOffsetY + 150);
	m_squadPosition.set(          m_mapScreenOffsetX + 160, m_mapScreenOffsetY + 150);
	m_versionPosition.set(        10, m_screenHeight - 15);
}

/** Get X position of tactical textbox. */
UINT16 UILayout::getTacticalTextBoxX() const
{

	if ( guiCurrentScreen == MAP_SCREEN )
	{
		return MAP_SCREEN_X + 110;
	}
	else
	{
		return 110;
	}
}

/** Get Y position of tactical textbox. */
UINT16 UILayout::getTacticalTextBoxY() const
{
	if ( guiCurrentScreen == MAP_SCREEN )
	{
		return DEFAULT_MAP_EXTERN_PANEL_Y_POS;
	}
	else
	{
		return 20;
	}
}

UINT16 UILayout::getTeamPanelNumSlots() const
{
	// every member of a squad has a slot; BIG_TEAMPANEL_NUM_SLOTS of them show
	if (m_bigTeamPanel) return std::max<int>(BIG_TEAMPANEL_NUM_SLOTS, gamepolicy(squad_size));
	return getOldTeamPanelNumSlots();
}

UINT16 UILayout::getOldTeamPanelNumSlots() const
{
	if (!GCM || !GCM->getGamePolicy())
	{
		throw std::runtime_error("ContentManager is not initialized yet. Unable to determine num of team slots");
	}

	int numSlots = std::min({(int)gamepolicy(squad_size), (m_screenWidth - TEAMPANEL_BUTTONSBOX_WIDTH) / TEAMPANEL_SLOT_WIDTH, 12});
	numSlots = std::max((int)numSlots, 6);
	return numSlots;
}
