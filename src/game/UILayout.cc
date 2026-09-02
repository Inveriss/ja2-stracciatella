#include "UILayout.h"

#include "ContentManager.h"
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

#define MIN_INTERFACE_WIDTH       640
#define MIN_INTERFACE_HEIGHT      480

/**
 * Default screen layout.
 * It might be changed later when the window size is known for sure. */
UILayout g_ui(MIN_INTERFACE_WIDTH, MIN_INTERFACE_HEIGHT);


/** Constructor. */
UILayout::UILayout(UINT16 screenWidth, UINT16 screenHeight)
	:m_mapScreenWidth(MIN_INTERFACE_WIDTH), m_mapScreenHeight(MIN_INTERFACE_HEIGHT),
	m_screenWidth(screenWidth), m_screenHeight(screenHeight)
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


/** Check if the screen is bigger than original 640x480. */
bool UILayout::isBigScreen() const
{
	return (m_screenWidth > 640) || (m_screenHeight > 480);
}


UINT16 UILayout::currentHeight() const             { return fInMapMode ? (STD_SCREEN_Y + m_mapScreenHeight) : m_screenHeight; }
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
	if (fInMapMode) return STD_SCREEN_X + 554;
	// SM_PANEL offset shifted 3px right (86 -> 83) to match
	// inventory_bottom_panel.sti's latest graphic; TEAM_PANEL (bottom_bar.sti)
	// is untouched.
	if (gsCurInterfacePanel == SM_PANEL) return m_teamPanelPosition.iX + m_smPanelWidth - 83;
	return m_teamPanelPosition.iX + m_teamPanelWidth - 86;
}
UINT16 UILayout::get_CLOCK_Y() const               { return currentHeight() - 23;                                  }
UINT16 UILayout::get_RADAR_WINDOW_X() const
{
	if (fInMapMode) return STD_SCREEN_X + 543;
	// SM_PANEL offset shifted 3px right (97 -> 94) -- see get_CLOCK_X().
	if (gsCurInterfacePanel == SM_PANEL) return m_teamPanelPosition.iX + m_smPanelWidth - 94;
	return m_teamPanelPosition.iX + m_teamPanelWidth - 97;
}
UINT16 UILayout::get_RADAR_WINDOW_TM_Y() const     { return currentHeight() - 107;                                 }
UINT16 UILayout::get_INV_INTERFACE_START_Y() const { return m_screenHeight - INV_INTERFACE_HEIGHT;                                  }
UINT16 UILayout::get_ITEMDESC_PANEL_START_Y() const { return m_screenHeight - ITEMDESC_PANEL_HEIGHT;                                 }


void UILayout::recalculatePositions()
{
	m_teamPanelSlotsTotalWidth = getTeamPanelNumSlots() * TEAMPANEL_SLOT_WIDTH;
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

	UINT16 startInvY = get_INV_INTERFACE_START_Y();
	UINT16 startX    = INTERFACE_START_X;

	m_stdScreenOffsetX            = (m_screenWidth - MIN_INTERFACE_WIDTH) / 2;
	m_stdScreenOffsetY            = (m_screenHeight - MIN_INTERFACE_HEIGHT) / 2;

	// tactical screen inventory position
	m_invSlotPositionTac[HELMETPOS           ].set(startX + 430, startInvY +   8);
	m_invSlotPositionTac[VESTPOS             ].set(startX + 430, startInvY +  47);
	m_invSlotPositionTac[LEGPOS              ].set(startX + 430, startInvY +  116);
	m_invSlotPositionTac[HEAD1POS            ].set(startX + 230, startInvY +   8);
	m_invSlotPositionTac[HEAD2POS            ].set(startX + 230, startInvY +  43);
	m_invSlotPositionTac[HANDPOS             ].set(startX + 303, startInvY +  83);
	m_invSlotPositionTac[SECONDHANDPOS       ].set(startX + 303, startInvY + 118);
	m_invSlotPositionTac[BIGPOCK1POS         ].set(startX + 678, startInvY +   80);
	m_invSlotPositionTac[BIGPOCK2POS         ].set(startX + 678, startInvY +  116);
	m_invSlotPositionTac[BIGPOCK3POS         ].set(startX + 771, startInvY +  8);
	m_invSlotPositionTac[BIGPOCK4POS         ].set(startX + 771, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK1POS       ].set(startX + 490, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK2POS       ].set(startX + 490, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK3POS       ].set(startX + 490, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK4POS       ].set(startX + 490, startInvY +  116);
	m_invSlotPositionTac[SMALLPOCK5POS       ].set(startX + 537, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK6POS       ].set(startX + 537, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK7POS       ].set(startX + 537, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK8POS       ].set(startX + 537, startInvY +  116);

	// TODO: placeholder positions for the 20 new slots (HEAD3/4, BIGPOCK5-10,
	// SMALLPOCK9-20) added for the inventory expansion -- replace with real
	// coordinates once the redesigned inventory_bottom_panel.sti layout is final.
	m_invSlotPositionTac[HEAD3POS            ].set(startX + 278, startInvY + 8);
	m_invSlotPositionTac[HEAD4POS            ].set(startX + 278, startInvY + 43);
	m_invSlotPositionTac[BIGPOCK5POS         ].set(startX + 771, startInvY + 80);
	m_invSlotPositionTac[BIGPOCK6POS         ].set(startX + 771, startInvY + 116);
	m_invSlotPositionTac[BIGPOCK7POS         ].set(startX + 847, startInvY + 8);
	m_invSlotPositionTac[BIGPOCK8POS         ].set(startX + 847, startInvY + 44);
	m_invSlotPositionTac[BIGPOCK9POS         ].set(startX + 847, startInvY + 80);
	m_invSlotPositionTac[BIGPOCK10POS        ].set(startX + 847, startInvY + 116);
	m_invSlotPositionTac[SMALLPOCK9POS       ].set(startX + 584, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK10POS      ].set(startX + 584, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK11POS      ].set(startX + 584, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK12POS      ].set(startX + 584, startInvY +  116);
	m_invSlotPositionTac[SMALLPOCK13POS      ].set(startX + 631, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK14POS      ].set(startX + 631, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK15POS      ].set(startX + 631, startInvY +  80);
	m_invSlotPositionTac[SMALLPOCK16POS      ].set(startX + 631, startInvY +  116);
	m_invSlotPositionTac[SMALLPOCK17POS      ].set(startX + 678, startInvY +   8);
	m_invSlotPositionTac[SMALLPOCK18POS      ].set(startX + 678, startInvY +  44);
	m_invSlotPositionTac[SMALLPOCK19POS      ].set(startX + 725, startInvY +  8);
	m_invSlotPositionTac[SMALLPOCK20POS      ].set(startX + 725, startInvY +  44);

	// map screen inventory position
	m_invSlotPositionMap[HELMETPOS           ].set(m_stdScreenOffsetX + 204, m_stdScreenOffsetY + 116);
	m_invSlotPositionMap[VESTPOS             ].set(m_stdScreenOffsetX + 204, m_stdScreenOffsetY + 145);
	m_invSlotPositionMap[LEGPOS              ].set(m_stdScreenOffsetX + 204, m_stdScreenOffsetY + 205);
	m_invSlotPositionMap[HEAD1POS            ].set(m_stdScreenOffsetX +  21, m_stdScreenOffsetY + 116);
	m_invSlotPositionMap[HEAD2POS            ].set(m_stdScreenOffsetX +  21, m_stdScreenOffsetY + 140);
	m_invSlotPositionMap[HANDPOS             ].set(m_stdScreenOffsetX +  21, m_stdScreenOffsetY + 194);
	m_invSlotPositionMap[SECONDHANDPOS       ].set(m_stdScreenOffsetX +  21, m_stdScreenOffsetY + 218);
	m_invSlotPositionMap[BIGPOCK1POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 251);
	m_invSlotPositionMap[BIGPOCK2POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 275);
	m_invSlotPositionMap[BIGPOCK3POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 299);
	m_invSlotPositionMap[BIGPOCK4POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 323);
	m_invSlotPositionMap[SMALLPOCK1POS       ].set(m_stdScreenOffsetX +  22, m_stdScreenOffsetY + 251);
	m_invSlotPositionMap[SMALLPOCK2POS       ].set(m_stdScreenOffsetX +  22, m_stdScreenOffsetY + 275);
	m_invSlotPositionMap[SMALLPOCK3POS       ].set(m_stdScreenOffsetX +  22, m_stdScreenOffsetY + 299);
	m_invSlotPositionMap[SMALLPOCK4POS       ].set(m_stdScreenOffsetX +  22, m_stdScreenOffsetY + 323);
	m_invSlotPositionMap[SMALLPOCK5POS       ].set(m_stdScreenOffsetX +  60, m_stdScreenOffsetY + 251);
	m_invSlotPositionMap[SMALLPOCK6POS       ].set(m_stdScreenOffsetX +  60, m_stdScreenOffsetY + 275);
	m_invSlotPositionMap[SMALLPOCK7POS       ].set(m_stdScreenOffsetX +  60, m_stdScreenOffsetY + 299);
	m_invSlotPositionMap[SMALLPOCK8POS       ].set(m_stdScreenOffsetX +  60, m_stdScreenOffsetY + 323);

	// TODO: placeholder positions for the 20 new slots (HEAD3/4, BIGPOCK5-10,
	// SMALLPOCK9-20) added for the inventory expansion -- replace with real
	// coordinates once the redesigned mapinv.sti layout is final.
	m_invSlotPositionMap[HEAD3POS            ].set(m_stdScreenOffsetX +  21, m_stdScreenOffsetY + 164);
	m_invSlotPositionMap[HEAD4POS            ].set(m_stdScreenOffsetX +  21, m_stdScreenOffsetY + 188);
	m_invSlotPositionMap[BIGPOCK5POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 347);
	m_invSlotPositionMap[BIGPOCK6POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 371);
	m_invSlotPositionMap[BIGPOCK7POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 395);
	m_invSlotPositionMap[BIGPOCK8POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 419);
	m_invSlotPositionMap[BIGPOCK9POS         ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 443);
	m_invSlotPositionMap[BIGPOCK10POS        ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 467);
	m_invSlotPositionMap[SMALLPOCK9POS       ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 251);
	m_invSlotPositionMap[SMALLPOCK10POS      ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 275);
	m_invSlotPositionMap[SMALLPOCK11POS      ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 299);
	m_invSlotPositionMap[SMALLPOCK12POS      ].set(m_stdScreenOffsetX +  98, m_stdScreenOffsetY + 323);
	m_invSlotPositionMap[SMALLPOCK13POS      ].set(m_stdScreenOffsetX + 136, m_stdScreenOffsetY + 251);
	m_invSlotPositionMap[SMALLPOCK14POS      ].set(m_stdScreenOffsetX + 136, m_stdScreenOffsetY + 275);
	m_invSlotPositionMap[SMALLPOCK15POS      ].set(m_stdScreenOffsetX + 136, m_stdScreenOffsetY + 299);
	m_invSlotPositionMap[SMALLPOCK16POS      ].set(m_stdScreenOffsetX + 136, m_stdScreenOffsetY + 323);
	m_invSlotPositionMap[SMALLPOCK17POS      ].set(m_stdScreenOffsetX + 174, m_stdScreenOffsetY + 251);
	m_invSlotPositionMap[SMALLPOCK18POS      ].set(m_stdScreenOffsetX + 174, m_stdScreenOffsetY + 275);
	m_invSlotPositionMap[SMALLPOCK19POS      ].set(m_stdScreenOffsetX + 174, m_stdScreenOffsetY + 299);
	m_invSlotPositionMap[SMALLPOCK20POS      ].set(m_stdScreenOffsetX + 174, m_stdScreenOffsetY + 323);

	m_invCamoRegion.set(SM_BODYINV_X, SM_BODYINV_Y);

	m_progress_bar_box.set(STD_SCREEN_X + 5, 2, MIN_INTERFACE_WIDTH - 10, 12);
	// Uses ITEMDESC_PANEL_START_Y (not startInvY/INV_INTERFACE_START_Y): the
	// money buttons are an overlay drawn inside the Infobox.sti popup (see
	// gMoneyButtonLoc in Interface_Items.cc), which is itself anchored to
	// ITEMDESC_PANEL_START_Y since the Infobox.sti/inventory_bottom_panel.sti
	// decoupling. Anchoring this to the wrong one left the buttons ~147px
	// (ITEMDESC_PANEL_HEIGHT - INV_INTERFACE_HEIGHT) below where the popup
	// actually is.
	m_moneyButtonLoc.set(startX + 343, get_ITEMDESC_PANEL_START_Y() + 11);
	m_MoneyButtonLocMap.set(m_stdScreenOffsetX + 174, m_stdScreenOffsetY + 115);

	m_VIEWPORT_START_X            = 0;
	m_VIEWPORT_START_Y            = 0;
	m_VIEWPORT_WINDOW_START_Y     = 0;
	m_VIEWPORT_END_X              = m_screenWidth;
	m_VIEWPORT_END_Y              = m_screenHeight - 120;
	m_VIEWPORT_WINDOW_END_Y       = m_screenHeight - 120;
	m_tacticalMapCenterX          = (m_VIEWPORT_END_X - m_VIEWPORT_START_X) / 2;
	m_tacticalMapCenterY          = (m_VIEWPORT_END_Y - m_VIEWPORT_START_Y) / 2;

	m_worldClippingRect.set(0, 0, m_screenWidth, m_screenHeight - 120);

	m_contractPosition.set(       m_stdScreenOffsetX + 120, m_stdScreenOffsetY +  50);
	m_attributePosition.set(      m_stdScreenOffsetX + 220, m_stdScreenOffsetY + 150);
	m_trainPosition.set(          m_stdScreenOffsetX + 160, m_stdScreenOffsetY + 150);
	m_vehiclePosition.set(        m_stdScreenOffsetX + 160, m_stdScreenOffsetY + 150);
	m_repairPosition.set(         m_stdScreenOffsetX + 160, m_stdScreenOffsetY + 150);
	m_assignmentPosition.set(     m_stdScreenOffsetX + 120, m_stdScreenOffsetY + 150);
	m_squadPosition.set(          m_stdScreenOffsetX + 160, m_stdScreenOffsetY + 150);
	m_versionPosition.set(        10, m_screenHeight - 15);
}

/** Get X position of tactical textbox. */
UINT16 UILayout::getTacticalTextBoxX() const
{

	if ( guiCurrentScreen == MAP_SCREEN )
	{
		return STD_SCREEN_X + 110;
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
		return DEFAULT_EXTERN_PANEL_Y_POS;
	}
	else
	{
		return 20;
	}
}

UINT16 UILayout::getTeamPanelNumSlots() const
{
	if (!GCM || !GCM->getGamePolicy())
	{
		throw std::runtime_error("ContentManager is not initialized yet. Unable to determine num of team slots");
	}

	int numSlots = std::min({(int)gamepolicy(squad_size), (m_screenWidth - TEAMPANEL_BUTTONSBOX_WIDTH) / TEAMPANEL_SLOT_WIDTH, 12});
	numSlots = std::max((int)numSlots, 6);
	return numSlots;
}
