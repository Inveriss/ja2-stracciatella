#ifndef _UI_LAYOUT_H_
#define _UI_LAYOUT_H_

#include "Types.h"

/////////////////////////////////////////////////////////////
// defines
/////////////////////////////////////////////////////////////

#define NUM_INVENTORY_SLOTS     (39)

/* Following defines allow us to not change the old code too much.
 * It will help to preserve original Stracciatella codebase. */

#define SCREEN_HEIGHT                   (g_ui.m_screenHeight)
#define SCREEN_WIDTH                    (g_ui.m_screenWidth)
#define INV_INTERFACE_START_Y           (g_ui.get_INV_INTERFACE_START_Y())
// #define INV_INTERFACE_HEIGHT         (140)                                 // height of the bottom bar single-merc inventory panel
#define INV_INTERFACE_HEIGHT            (g_ui.isBigSMPanel() ? BIG_SMPANEL_HEIGHT : 197) // NEW POSITION; the 1366x768 one is taller

// The 1366x768 interface's single merc (SM) panel,
// inventory_bottom_panel_big_1366x768.png (isBigSMPanel()): one fixed piece at
// the screen's left edge (not centred), its slots laid out in UILayout.cc's
// m_invSlotPositionTac and Interface_Items.cc's gSMInvDataTacBig.
#define BIG_SMPANEL_FILE                "inventory_bottom_panel_big_1366x768.png"
#define BIG_SMPANEL_WIDTH               (1366)
#define BIG_SMPANEL_HEIGHT              (284)  // its top 37 px: the bookmark buttons' strip
#define BIG_SMPANEL_TOP                 (37)
// The helicopter's own SM panel on the 1366x768 interface, same size and
// left block as the big one, its 8 cargo slots in m_invSlotPositionTacHeli
#define HELI_SMPANEL_FILE               "heli_tactical_inv_1366x768.png"
// ... and its own merc inventory panel on the 1366x768 strategic screen
// ("Show Large Icons" only), 604x514 at PLAYER_INFO_X/Y, slots in
// m_invSlotPositionMapHeli
#define HELI_MAPINV_FILE                "heli_strategic_inv_1366x768.png"
// position of a slot a layout has no place for: off the screen
#define INV_SLOT_HIDDEN_POS             (60000)

// Infobox.sti (the weapon/item description popup) is blitted straight to the
// screen buffer, independently of guiSMPanel/inventory_bottom_panel.sti --
// see ITEMDESC_START_Y in Interface_Items.cc. This is its OWN vertical
// anchor, deliberately NOT reusing INV_INTERFACE_HEIGHT, so resizing the
// inventory panel doesn't silently move the description box (and vice
// versa). Currently set to the same value (302) purely to preserve today's
// on-screen position -- tune independently as needed.
//
// Infobox_money.sti/Infobox_items.sti (see fIsMoney/fIsWeapon in
// InternalInitItemDescriptionBox()/RenderItemDescriptionBox(),
// Interface_Items.cc) get their own, equally independent copies below --
// all three currently equal, preserving today's shared on-screen position.
#define ITEMDESC_PANEL_START_Y          (g_ui.get_ITEMDESC_PANEL_START_Y())
#define ITEMDESC_PANEL_HEIGHT           (304)                                 // Infobox.sti (weapon)
#define ITEMDESC_PANEL_START_Y_MONEY    (g_ui.get_ITEMDESC_PANEL_START_Y_MONEY())
#define ITEMDESC_PANEL_HEIGHT_MONEY     (197)                                 // Infobox_money.sti
#define ITEMDESC_PANEL_START_Y_ITEMS    (g_ui.get_ITEMDESC_PANEL_START_Y_ITEMS())
#define ITEMDESC_PANEL_HEIGHT_ITEMS     (197)                                 // Infobox_items.sti
#define INTERFACE_START_X               (g_ui.m_teamPanelPosition.iX)
#define INTERFACE_START_Y               (g_ui.m_teamPanelPosition.iY)
#define gsVIEWPORT_START_X              (g_ui.m_VIEWPORT_START_X)
#define gsVIEWPORT_START_Y              (g_ui.m_VIEWPORT_START_Y)
#define gsVIEWPORT_WINDOW_START_Y       (g_ui.m_VIEWPORT_WINDOW_START_Y)
#define gsVIEWPORT_END_X                (g_ui.m_VIEWPORT_END_X)
#define gsVIEWPORT_END_Y                (g_ui.m_VIEWPORT_END_Y)
#define gsVIEWPORT_WINDOW_END_Y         (g_ui.m_VIEWPORT_WINDOW_END_Y)
// STD_SCREEN_X/Y centers the legacy (STD_SCREEN_WIDTH x STD_SCREEN_HEIGHT,
// still 640x480) window used by every screen except the strategic map --
// splash, options, save/load, credits, initial game settings, the laptop
// and all of its sub-screens (including the laptop's own on-screen
// footprint used by its map<->laptop transition animation).
#define STD_SCREEN_X                    (g_ui.m_stdScreenOffsetX)
#define STD_SCREEN_Y                    (g_ui.m_stdScreenOffsetY)
#define STD_SCREEN_WIDTH                (640)
#define STD_SCREEN_HEIGHT               (480)
// MAP_SCREEN_X/Y centers the strategic map's own, larger canvas
// (MAP_SCREEN_WIDTH x MAP_SCREEN_HEIGHT, 1024, 1280 or 1366 x 768/720) -- use this (not
// STD_SCREEN_X/Y) for anything drawn as part of the strategic map screen
// itself.
#define MAP_SCREEN_X                    (g_ui.m_mapScreenOffsetX)
#define MAP_SCREEN_Y                    (g_ui.m_mapScreenOffsetY)
#define MAP_SCREEN_WIDTH                (g_ui.m_mapScreenWidth)
#define MAP_SCREEN_HEIGHT               (g_ui.m_mapScreenHeight)
// Right/bottom edge of the strategic map's own canvas -- use these (not a
// fresh MAP_SCREEN_X/Y + hand-tuned pixel offset) for anything that should
// hug the far edge of the map screen, so it tracks MAP_SCREEN_WIDTH/HEIGHT
// automatically across both strategic-screen size tiers.
#define MAP_SCREEN_RIGHT                (MAP_SCREEN_X + MAP_SCREEN_WIDTH)
#define MAP_SCREEN_BOTTOM               (MAP_SCREEN_Y + MAP_SCREEN_HEIGHT)

// Wide strategic screen (screen width >= WIDE_STRATEGIC_SCREEN_WIDTH): the
// map canvas is 1280px wide, or 1366px wide on the 1366x768 base (screen at
// least EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH wide and LARGE tier tall -- see
// isExtraWideStrategicScreen()); centered on anything wider. The left
// column (charinfo/newgoldpiece3/mapinv/iteminfoc, pre-battle panel) and
// the left part of map_screen_bottom stay on MAP_SCREEN_X; the map itself,
// MBS and everything drawn on it (border buttons of the large tier, ETA,
// map view, sector inventory, map popups) use MAP_SCREEN_RIGHT_BLOCK_X,
// which is shifted right by MAP_RIGHT_BLOCK_SHIFT (canvas width - 1024:
// 256 on the 1280 canvas, 342 on the 1366 one), leaving a free space
// between the two (filled with background_middle_wide_*.sti). On the legacy
// 1024px canvas MAP_SCREEN_RIGHT_BLOCK_X == MAP_SCREEN_X.
#define WIDE_STRATEGIC_SCREEN_WIDTH       (1280)
#define EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH (1366)
#define MAP_LEGACY_CANVAS_WIDTH           (1024)   // width the right block's own coordinates were laid out for
#define MAP_RIGHT_BLOCK_SHIFT             (g_ui.m_mapRightBlockShift)   // 0, 256 or 342
#define MAP_SCREEN_RIGHT_BLOCK_X          (g_ui.m_mapRightBlockOffsetX)
// The free space's own column (X relative to MAP_SCREEN_X) -- starts under
// the left column's last pixel column, the same 1px overlap MBS has always
// had with it; the left column is drawn on top.
#define MAP_MIDDLE_BACKGROUND_X         (MAP_SCREEN_X + 261)
#define MAP_MIDDLE_BACKGROUND_WIDTH     (MAP_RIGHT_BLOCK_SHIFT)

// The strategic screen's merc inventory panel (mapinv) is laid out for
// MAP_SCREEN_Y + 107; on the 1366x768 interface (isExtraWideStrategicScreen())
// it and everything on it sit MAP_INV_SHIFT_Y lower (y 133, below the
// character info panel's top part).
#define MAP_INV_SHIFT_Y                 (g_ui.isExtraWideStrategicScreen() ? 26 : 0)

#define SM_BODYINV_X                    (INTERFACE_START_X + 324)
#define SM_BODYINV_Y                    (INV_INTERFACE_START_Y + 6)
#define SM_INVINTERFACE_WIDTH           (920)    // width of the single-merc inventory panel excluding the right-side buttons and minimap

// Total width of inventory_bottom_panel.sti (SM_INVINTERFACE_WIDTH + the
// 105px right-side buttons/minimap segment). Used wherever code needs to
// know the actual pixel width of the loaded graphic asset itself, as
// opposed to just the usable inventory portion.
#define INVENTORY_BOTTOM_PANEL_WIDTH    (1025)

#define EDITOR_TASKBAR_HEIGHT           (120)
#define EDITOR_TASKBAR_POS_Y            (UINT16)(SCREEN_HEIGHT - EDITOR_TASKBAR_HEIGHT)

#define DEFAULT_EXTERN_PANEL_X_POS      (STD_SCREEN_X + 320)
#define DEFAULT_EXTERN_PANEL_Y_POS      (STD_SCREEN_Y + 40)
// Same speech-bubble default, but for the strategic map screen (where the
// panel must align with the map's own, larger canvas) -- see
// Strategic_Mines.cc / Map_Screen_Helicopter.cc.
#define DEFAULT_MAP_EXTERN_PANEL_X_POS  (MAP_SCREEN_RIGHT_BLOCK_X + 320)
#define DEFAULT_MAP_EXTERN_PANEL_Y_POS  (MAP_SCREEN_Y + 40)

#define TEAMPANEL_SLOT_WIDTH            (83)     // width of one slot in the bottom team panel
#define TEAMPANEL_BUTTONSBOX_WIDTH      (142)    // width of the container of the buttons on the right of team panel
#define TEAMPANEL_HEIGHT                (120)    // height of the bottom bar team panel

// The 1366x768 interface's own team panel (bottom_bar_big_1366x768.png,
// isBigTeamPanel()): 7 slots shown, 146 px apart (a squad over 7 scrolls
// through them), left-aligned (not centred), with
// the buttons, sector name, clock and minimap block on its right; its texts
// are FONT14ARIAL (BIG_TEAMPANEL_FONT). The single
// merc (SM) panel is not affected -- it keeps INTERFACE_START_X/Y and the
// squad-size-driven m_teamPanelWidth/m_smPanelWidth.
#define BIG_TEAMPANEL_FILE              "bottom_bar_big_1366x768.png"
#define BIG_TEAMPANEL_WIDTH             (1366)
// The big team panel's two heights: the full one, and the low one
// (bottom_bar_low_big_1366x768.png, isLowTeamPanel()) without the second hand,
// toggled by its sixth button.
#define BIG_TEAMPANEL_FULL_HEIGHT       (200)
#define BIG_TEAMPANEL_LOW_HEIGHT        (150)
#define BIG_TEAMPANEL_LOW_FILE          "bottom_bar_low_big_1366x768.png"
// The bar of a watched militia battle (isWatchBar()): the radar, the sector
// and the clock, the EXIT and PAUSE buttons; the world shown down to it;
// 1366x768 only. Its parts (bar coordinates) inside the 5 px frame:
#define WATCH_BAR_FILE                  "MILITIA_BOTTOM_BAR_1366x768.png"
#define WATCH_BAR_HEIGHT                (110)
#define WATCH_BAR_INNER_Y               (5)      // the frame's inside, 100 high
#define WATCH_BAR_INNER_HEIGHT          (100)
#define WATCH_BAR_RADAR_X               (1161)   // radar window, 200x100
#define WATCH_BAR_RADAR_Y               (5)
#define WATCH_BAR_BOX_HEIGHT            (20)     // sector and clock boxes
#define WATCH_BAR_SECTOR_X              (1003)   // sector box, 130x20
#define WATCH_BAR_SECTOR_Y              (27)
#define WATCH_BAR_SECTOR_WIDTH          (130)
#define WATCH_BAR_CLOCK_X               (1018)   // clock box, 100x20
#define WATCH_BAR_CLOCK_Y               (63)
#define WATCH_BAR_CLOCK_WIDTH           (100)
#define BIG_TEAMPANEL_HEIGHT            (g_ui.isWatchBar() ? WATCH_BAR_HEIGHT : g_ui.isLowTeamPanel() ? BIG_TEAMPANEL_LOW_HEIGHT : BIG_TEAMPANEL_FULL_HEIGHT)
#define BIG_TEAMPANEL_NUM_SLOTS         (7)      // slots shown; a bigger squad scrolls through them
#define BIG_TEAMPANEL_SLOT_WIDTH        (146)
#define BIG_TEAMPANEL_FONT              FONT14ARIAL
// Team panel position/size for whichever team panel is in use -- use these
// (not INTERFACE_START_X/Y, which the SM panel shares) for the team panel.
#define TEAM_PANEL_X                    (g_ui.isBigTeamPanel() ? 0 : INTERFACE_START_X)
#define TEAM_PANEL_Y                    (g_ui.isBigTeamPanel() ? SCREEN_HEIGHT - BIG_TEAMPANEL_HEIGHT : INTERFACE_START_Y)
#define TEAM_PANEL_WIDTH                (g_ui.isBigTeamPanel() ? BIG_TEAMPANEL_WIDTH : g_ui.m_teamPanelWidth)
#define TEAM_PANEL_HEIGHT               (g_ui.isBigTeamPanel() ? BIG_TEAMPANEL_HEIGHT : TEAMPANEL_HEIGHT)


/////////////////////////////////////////////////////////////
// type definitions
/////////////////////////////////////////////////////////////

// USED TO SETUP REGION POSITIONS, ETC
struct INV_REGION_DESC
{
	UINT16     uX;
	UINT16     uY;

	void set(UINT16 x, UINT16 y)
	{
		uX = x;
		uY = y;
	}
};


struct MoneyLoc
{
	UINT16 x;
	UINT16 y;

	void set(UINT16 _x, UINT16 _y)
	{
		x = _x;
		y = _y;
	}
};


/** User Interface layout definition. */
struct UILayout
{
public:
	UINT16                m_mapScreenWidth;
	UINT16                m_mapScreenHeight;
	UINT16                m_screenWidth;
	UINT16                m_screenHeight;
	INV_REGION_DESC       m_invSlotPositionMap[NUM_INVENTORY_SLOTS];      /**< Map screen inventory slots positions  */
	INV_REGION_DESC       m_invSlotPositionMapBig[NUM_INVENTORY_SLOTS];   /**< Map screen inventory slots positions, "Show Large Icons" mode (mapinv_big_1280_720/768.sti) */
	INV_REGION_DESC       m_invSlotPositionTac[NUM_INVENTORY_SLOTS];      /**< Tactical screen Inventory slots positions */
	INV_REGION_DESC       m_invSlotPositionTacHeli[NUM_INVENTORY_SLOTS];  /**< The helicopter's big SM panel (HELI_SMPANEL_FILE): its 8 cargo slots, the rest INV_SLOT_HIDDEN_POS */
	INV_REGION_DESC       m_invSlotPositionMapHeli[NUM_INVENTORY_SLOTS];  /**< The helicopter's map inventory panel (HELI_MAPINV_FILE): its 8 cargo slots, the rest INV_SLOT_HIDDEN_POS */
	INV_REGION_DESC       m_invCamoRegion;                                /**< Camo (body) region in the inventory. */

	SGPBox                m_progress_bar_box;
	MoneyLoc              m_moneyButtonLoc;
	MoneyLoc              m_MoneyButtonLocMap;

	/** Viewport coordiantes.
	 * Viewport is the area of the screen where tactical map is displayed.
	 * For 640x480 it is (320, 180) */
	UINT16                m_VIEWPORT_START_X;
	UINT16                m_VIEWPORT_START_Y;
	UINT16                m_VIEWPORT_WINDOW_START_Y;

	UINT16                m_VIEWPORT_END_X;
	UINT16                m_VIEWPORT_END_Y;
	UINT16                m_tacticalMapCenterX;                           /**< Center of the tactical map (for 640x480 it is (320, 180)). */
	UINT16                m_tacticalMapCenterY;                           /**< Center of the tactical map (for 640x480 it is (320, 180)). */

	UINT16                m_VIEWPORT_WINDOW_END_Y;

	SGPRect               m_worldClippingRect;

	// Map screen interface
	SGPPoint              m_versionPosition;
	SGPPoint              m_contractPosition;
	SGPPoint              m_attributePosition;
	SGPPoint              m_trainPosition;
	SGPPoint              m_vehiclePosition;
	SGPPoint              m_repairPosition;
	SGPPoint              m_assignmentPosition ;
	SGPPoint              m_squadPosition ;

	// Tactical screen bottom bar
	// It can be in the "team" (TEAM) or the "single merc inventory" (SM or INV_) mode. Both modes share the same
	// on-screen position, but the single-merc mode is slightly taller and (since inventory_bottom_panel.sti grew
	// past bottom_bar.sti's own scaling) can be wider too -- see m_smPanelWidth.
	SGPPoint              m_teamPanelPosition;              // offset position of the bottom bar
	UINT16                m_teamPanelSlotsTotalWidth;       // total width of all team slots in the bottom team panel
	UINT16                m_teamPanelWidth;                 // width of the entire team panel (bottom_bar.sti) including slots and buttons -- purely squad-size-driven, NOT floored to fit inventory_bottom_panel.sti. Also used to position squad-size-driven widgets shared by both panel modes (minimap, clock, sector name).
	UINT16                m_smPanelWidth;                   // width of the single-merc inventory panel (inventory_bottom_panel.sti) canvas -- max(m_teamPanelWidth, INVENTORY_BOTTOM_PANEL_WIDTH), since that graphic can be wider than what squad size alone would need. Use this (not m_teamPanelWidth) for anything specific to the single-merc panel's own canvas/buttons (SM_DONE_X, SM_MAPSCREEN_X).

	UINT16                m_stdScreenOffsetX;             /** Offset of the legacy (STD_SCREEN_WIDTH x STD_SCREEN_HEIGHT, 640x480) window used by every screen except the strategic map -- see STD_SCREEN_X/Y */
	UINT16                m_stdScreenOffsetY;             /** Offset of the legacy (STD_SCREEN_WIDTH x STD_SCREEN_HEIGHT, 640x480) window used by every screen except the strategic map -- see STD_SCREEN_X/Y */
	UINT16                m_mapScreenOffsetX;             /** Offset of the strategic map's own canvas (m_mapScreenWidth x m_mapScreenHeight) -- see MAP_SCREEN_X/Y */
	UINT16                m_mapScreenOffsetY;             /** Offset of the strategic map's own canvas (m_mapScreenWidth x m_mapScreenHeight) -- see MAP_SCREEN_X/Y */
	UINT16                m_mapRightBlockOffsetX;         /** Offset of the strategic map's right block (MBS, map, sector inventory) -- see MAP_SCREEN_RIGHT_BLOCK_X */
	UINT16                m_mapRightBlockShift;           /** How far the right block sits right of where the legacy 1024 canvas had it (m_mapScreenWidth - 1024 on the wide strategic screen, else 0) -- see MAP_RIGHT_BLOCK_SHIFT */
	bool                  m_bigTeamPanel;                 /** The tactical team panel is bottom_bar_big_1366x768.png -- see isBigTeamPanel() */
	bool                  m_lowTeamPanel;                 /** The big team panel is in its low form -- see isLowTeamPanel() */
	bool                  m_watchBar;                     /** The big team panel is the watched battle's bar -- see isWatchBar() */
	bool                  m_bigSMPanel;                   /** The tactical SM panel is inventory_bottom_panel_big_1366x768.png -- see isBigSMPanel() */
	bool                  m_smLargeIcons;                 /** The SM panels' Show Large Icons toggle: the big SM panel (on), or the old one (off) -- see setSMLargeIcons() */

	/** Constructor.
	 * @param screenWidth Screen width
	 * @param screenHeight Screen height */
	UILayout(UINT16 screenWidth, UINT16 screenHeight);

	/** Set new screen size. Element positions should be recalculated after setting this. @see UILayout::recalculatePositions */
	void setScreenSize(UINT16 width, UINT16 height);

	/** Check if the screen is bigger than the currently active strategic-screen canvas size. */
	bool isBigScreen() const;

	/** True when the active resolution uses the compact (720px-tall) strategic screen asset set instead of the large (768px-tall) one. */
	bool isCompactStrategicScreen() const;

	/** True when the active resolution uses the wide (1280px) strategic screen canvas, i.e. screen width >= WIDE_STRATEGIC_SCREEN_WIDTH. */
	bool isWideStrategicScreen() const;

	/** True when the active resolution uses the 1366px wide strategic screen canvas (the 1366x768 base): screen at least EXTRA_WIDE_STRATEGIC_SCREEN_WIDTH wide and not compact. Implies isWideStrategicScreen(). */
	bool isExtraWideStrategicScreen() const;

	/** True when the tactical team panel is the 1366x768 interface's own one (BIG_TEAMPANEL_*): the 1366x768 base (as isExtraWideStrategicScreen()) and bottom_bar_big_1366x768.png present; otherwise the old bottom_bar.sti panel. */
	bool isBigTeamPanel() const { return m_bigTeamPanel; }

	/** True when the tactical SM (single merc inventory) panel is the 1366x768 interface's own one (BIG_SMPANEL_*): the big team panel's resolution and its graphic present. */
	bool isBigSMPanel() const { return m_bigSMPanel; }

	/** The big SM panel is the one on screen right now: tactical screen, SM panel. */
	bool isBigSMPanelShown() const;

	/** The SM panels' Show Large Icons toggle (1366x768): on = the big SM panel, off = the old inventory_bottom_panel.sti one. */
	bool isSMLargeIcons() const { return m_smLargeIcons; }

	/** Sets the Show Large Icons toggle and lays the SM panel out again (recalculatePositions()); the panel itself is rebuilt by the caller. */
	void setSMLargeIcons(bool on);

	/** True when the big team panel shows its low form (BIG_TEAMPANEL_LOW_*), chosen by the panel's toggle button. */
	bool isLowTeamPanel() const { return m_bigTeamPanel && m_lowTeamPanel; }

	/** Switches the big team panel's low form on or off -- off when its graphic is missing. The panel itself is rebuilt by the caller. */
	void setLowTeamPanel(bool low);

	/** True when the big team panel is the bar of a watched militia battle (WATCH_BAR_*): radar, sector, clock, EXIT and PAUSE. */
	bool isWatchBar() const { return m_bigTeamPanel && m_watchBar; }

	/** The watched battle's bar on screen right now (tactical, team panel). */
	bool isWatchBarShown() const { return isWatchBar() && isBigTeamPanelShown(); }

	/** Switches the watched battle's bar on or off -- on only at 1366x768 with its graphic present; the world viewport then reaches down to the bar. The panel itself is rebuilt by the caller. */
	void setWatchBar(bool on);

	UINT16 currentHeight() const;
	UINT16 get_CLOCK_X() const;
	UINT16 get_CLOCK_Y() const;
	UINT16 get_CLOCK_WIDTH() const;

	/** The big team panel is the one on screen right now: tactical screen, team (not SM) panel. */
	bool isBigTeamPanelShown() const;
	UINT16 get_INV_INTERFACE_START_Y() const;
	UINT16 get_ITEMDESC_PANEL_START_Y() const;
	UINT16 get_ITEMDESC_PANEL_START_Y_MONEY() const;
	UINT16 get_ITEMDESC_PANEL_START_Y_ITEMS() const;
	UINT16 get_RADAR_WINDOW_X() const;
	UINT16 get_RADAR_WINDOW_TM_Y() const;

	/** Get X position of tactical textbox. */
	UINT16 getTacticalTextBoxX() const;

	/** Get Y position of tactical textbox. */
	UINT16 getTacticalTextBoxY() const;

	/** Number of displayable slots in the team panel, based on the game policy and screen width. */
	UINT16 getTeamPanelNumSlots() const;

	/** Number of slots the old team panel (bottom_bar.sti) has, based on the game policy and screen width -- also sizes the SM panel's canvas. */
	UINT16 getOldTeamPanelNumSlots() const;

	/** Recalculate UI elements' positions after changing screen size.
	 *  This method requires the game data to be loaded, but it should be called before most other the application initialization is done.
	 */
	void recalculatePositions();
};

/////////////////////////////////////////////////////////////
// external declarations
/////////////////////////////////////////////////////////////

extern UILayout g_ui;

/////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////

#endif
