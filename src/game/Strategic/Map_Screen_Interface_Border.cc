#include "Map_Screen_Interface_Border.h"
#include "Assignments.h"
#include "Campaign_Types.h"
#include "Debug.h"
#include "Directories.h"
#include "Interface.h"
#include "Map_Screen_Helicopter.h"
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Map.h"
#include "Map_Screen_Interface_Map_Inventory.h"
#include "MapScreen.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "SysUtil.h"
#include "Text.h"
#include "UILayout.h"
#include "Video.h"
#include "VObject.h"
#include "VSurface.h"
#include <string_theory/string>

struct BUTTON_PICS;

// Large-tier (map canvas height 768+) coordinates for the six Show-* buttons,
// their shared row Y, and the map-level marker/regions -- these are the
// values already tuned via prior requests (X net +191/+192, Y net +1 from
// the original bottom-anchored positions).
#define BTN_TOWN_X_LARGE           (MAP_SCREEN_X + 490)
#define BTN_MINE_X_LARGE           (MAP_SCREEN_X + 533)
#define BTN_TEAMS_X_LARGE          (MAP_SCREEN_X + 576)
#define BTN_MILITIA_X_LARGE        (MAP_SCREEN_X + 619)
#define BTN_AIR_X_LARGE            (MAP_SCREEN_X + 662)
#define BTN_ITEM_X_LARGE           (MAP_SCREEN_X + 705)
#define BTN_ROW_Y_LARGE            (MAP_SCREEN_BOTTOM - 156)
#define MAP_LEVEL_MARKER_X_LARGE   (MAP_SCREEN_X + 757)

// Compact-tier (map canvas height 720-767) coordinates -- a second,
// independent set from the large tier above, per user request. Initialized
// as the large-tier values shifted -97 X / +55 Y (Y+55 down = 55 less
// distance from the bottom edge, i.e. 156-55=101); tune freely from here
// without affecting the large tier.
#define BTN_TOWN_X_COMPACT         (MAP_SCREEN_X + 393)
#define BTN_MINE_X_COMPACT         (MAP_SCREEN_X + 436)
#define BTN_TEAMS_X_COMPACT        (MAP_SCREEN_X + 479)
#define BTN_MILITIA_X_COMPACT      (MAP_SCREEN_X + 522)
#define BTN_AIR_X_COMPACT          (MAP_SCREEN_X + 565)
#define BTN_ITEM_X_COMPACT         (MAP_SCREEN_X + 608)
#define BTN_ROW_Y_COMPACT          (MAP_SCREEN_BOTTOM - 101)
// X shifted -9 per user request.
#define MAP_LEVEL_MARKER_X_COMPACT (MAP_SCREEN_X + 660 - 9)

// Resolved at runtime (not static-init time) via
// UILayout::isCompactStrategicScreen(), same reasoning as
// GetMapBorderGraphicsFilename() below: which set applies depends on the
// active resolution.
#define BTN_TOWN_X      (g_ui.isCompactStrategicScreen() ? BTN_TOWN_X_COMPACT    : BTN_TOWN_X_LARGE)
#define BTN_MINE_X      (g_ui.isCompactStrategicScreen() ? BTN_MINE_X_COMPACT    : BTN_MINE_X_LARGE)
#define BTN_TEAMS_X     (g_ui.isCompactStrategicScreen() ? BTN_TEAMS_X_COMPACT   : BTN_TEAMS_X_LARGE)
#define BTN_MILITIA_X   (g_ui.isCompactStrategicScreen() ? BTN_MILITIA_X_COMPACT : BTN_MILITIA_X_LARGE)
#define BTN_AIR_X       (g_ui.isCompactStrategicScreen() ? BTN_AIR_X_COMPACT     : BTN_AIR_X_LARGE)
#define BTN_ITEM_X      (g_ui.isCompactStrategicScreen() ? BTN_ITEM_X_COMPACT    : BTN_ITEM_X_LARGE)

// Shared by the six Show-* buttons above and MAP_LEVEL_MARKER_Y below (the
// current-level highlight rides on the same row) -- bottom-anchored so both
// stay flush with the bottom edge of the map canvas instead of a fixed
// offset tuned for the old 640x480 canvas.
#define BTN_ROW_Y             (g_ui.isCompactStrategicScreen() ? BTN_ROW_Y_COMPACT : BTN_ROW_Y_LARGE)

#define MAP_LEVEL_MARKER_X    (g_ui.isCompactStrategicScreen() ? MAP_LEVEL_MARKER_X_COMPACT : MAP_LEVEL_MARKER_X_LARGE)
#define MAP_LEVEL_MARKER_Y     BTN_ROW_Y
#define MAP_LEVEL_MARKER_DELTA   8
#define MAP_LEVEL_MARKER_WIDTH  55


#define MAP_BORDER_X (MAP_SCREEN_X + 261)
#define MAP_BORDER_Y (MAP_SCREEN_Y + 0)

#define MAP_BORDER_CORNER_X (MAP_SCREEN_X + 584)
#define MAP_BORDER_CORNER_Y (MAP_SCREEN_Y + 279)


// mouse levels
static MOUSE_REGION LevelMouseRegions[4];

// graphics
namespace {
// the white rectangle highlighting the current level on the map border
cache_key_t const guiLEVELMARKER{ INTERFACEDIR "/greenarr.sti" };
 // the map border eta pop up
cache_key_t const guiMapBorderEtaPopUp{ INTERFACEDIR "/eta_pop_up.sti" };

// Not a plain cache_key_t constant like the others above: which file this is
// depends on the active resolution (see UILayout::isCompactStrategicScreen()),
// which isn't known yet at static-initialization time, so the choice has to
// be resolved at runtime, on every call. Suffix convention: _1280 for the
// compact strategic-screen tier (height 720-767), _1024 for the large tier
// (height 768+) -- see GetCharListGraphicsFilename() in MapScreen.cc for the
// same pattern.
cache_key_t GetMapBorderGraphicsFilename()
{
	return g_ui.isCompactStrategicScreen()
		? INTERFACEDIR "/mbs_1280.sti"
		: INTERFACEDIR "/mbs_1024.sti";
}
}

// scroll direction
INT32 giScrollButtonState = -1;

// flags
BOOLEAN fShowTownFlag = FALSE;
BOOLEAN fShowMineFlag = FALSE;
BOOLEAN fShowTeamFlag = FALSE;
BOOLEAN fShowMilitia = FALSE;
BOOLEAN fShowAircraftFlag = FALSE;
BOOLEAN fShowItemsFlag = FALSE;


// buttons & button images
GUIButtonRef giMapBorderButtons[6];
static BUTTON_PICS* giMapBorderButtonsImage[6];

extern void CancelMapUIMessage( void );


void DeleteMapBorderGraphics( void )
{
	// procedure will delete graphics loaded for map border
	RemoveVObject(guiLEVELMARKER);
	RemoveVObject(GetMapBorderGraphicsFilename());
	RemoveVObject(guiMapBorderEtaPopUp);
}


static void DisplayCurrentLevelMarker(void);


void RenderMapBorder( void )
{
	if( fShowMapInventoryPool )
	{
		// render background, then leave
		BlitInventoryPoolGraphic( );
		return;
	}

	BltVideoObject(guiSAVEBUFFER, GetMapBorderGraphicsFilename(), 0, MAP_BORDER_X, MAP_BORDER_Y);

	// show the level marker
	DisplayCurrentLevelMarker( );
}

void RenderMapBorderEtaPopUp( void )
{
	if( fShowMapInventoryPool )
	{
		return;
	}

	if (fPlotForHelicopter)
	{
		DisplayDistancesForHelicopter( );
		return;
	}

	// Bottom-anchored per user request: 480-291=189, same distance from the
	// old 640x480 canvas' bottom edge as before. X shifted +136 per user
	// request.
	BltVideoObject(FRAME_BUFFER, guiMapBorderEtaPopUp, 0, MAP_BORDER_X + 215 + 136, MAP_SCREEN_BOTTOM - 189);

	InvalidateRegion( MAP_BORDER_X + 215 + 136, (MAP_SCREEN_BOTTOM - 189), MAP_BORDER_X + 215 + 136 + 100 , (MAP_SCREEN_BOTTOM - 189 + 19));
}


static void MakeButton(UINT idx, UINT gfx, INT16 x, GUI_CALLBACK click, const ST::string& help)
{
	BUTTON_PICS* const img = LoadButtonImage(INTERFACEDIR "/map_border_buttons.sti", gfx, gfx + 9);
	giMapBorderButtonsImage[idx] = img;
	GUIButtonRef const btn = QuickCreateButtonNoMove(img, x, BTN_ROW_Y, MSYS_PRIORITY_HIGH, click);
	giMapBorderButtons[idx] = btn;
	btn->SetFastHelpText(help);
	btn->SetCursor(MSYS_NO_CURSOR);
}


static void BtnAircraftCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnItemCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnMilitiaCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnMineCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnTeamCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnTownCallback(GUI_BUTTON* btn, UINT32 reason);
static void InitializeMapBorderButtonStates(void);


void CreateButtonsForMapBorder(void)
{
	// will create the buttons needed for the map screen border region

	MakeButton(MAP_BORDER_TOWN_BTN,     5, BTN_TOWN_X,    BtnTownCallback,     pMapScreenBorderButtonHelpText[0]); // towns
	MakeButton(MAP_BORDER_MINE_BTN,     4, BTN_MINE_X,    BtnMineCallback,     pMapScreenBorderButtonHelpText[1]); // mines
	MakeButton(MAP_BORDER_TEAMS_BTN,    3, BTN_TEAMS_X,   BtnTeamCallback,     pMapScreenBorderButtonHelpText[2]); // people
	MakeButton(MAP_BORDER_MILITIA_BTN,  8, BTN_MILITIA_X, BtnMilitiaCallback,  pMapScreenBorderButtonHelpText[5]); // militia
	MakeButton(MAP_BORDER_AIRSPACE_BTN, 2, BTN_AIR_X,     BtnAircraftCallback, pMapScreenBorderButtonHelpText[3]); // airspace
	MakeButton(MAP_BORDER_ITEM_BTN,     1, BTN_ITEM_X,    BtnItemCallback,     pMapScreenBorderButtonHelpText[4]); // items

	InitializeMapBorderButtonStates( );
}


void DeleteMapBorderButtons( void )
{
	UINT8 ubCnt;

	RemoveButton( giMapBorderButtons[ 0 ]);
	RemoveButton( giMapBorderButtons[ 1 ]);
	RemoveButton( giMapBorderButtons[ 2 ]);
	RemoveButton( giMapBorderButtons[ 3 ]);
	RemoveButton( giMapBorderButtons[ 4 ]);
	RemoveButton( giMapBorderButtons[ 5 ]);

	// images
	UnloadButtonImage( giMapBorderButtonsImage[ 0 ] );
	UnloadButtonImage( giMapBorderButtonsImage[ 1 ] );
	UnloadButtonImage( giMapBorderButtonsImage[ 2 ] );
	UnloadButtonImage( giMapBorderButtonsImage[ 3 ] );
	UnloadButtonImage( giMapBorderButtonsImage[ 4 ] );
	UnloadButtonImage( giMapBorderButtonsImage[ 5 ] );


	for ( ubCnt = 0; ubCnt < 6; ubCnt++ )
	{
		giMapBorderButtonsImage[ubCnt] = NULL;
	}
}


// callbacks

static void CommonBtnCallbackBtnDownChecks(void);


static void BtnMilitiaCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowMilitiaMode( );
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnTeamCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowTeamsMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnTownCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowTownsMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnMineCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowMinesMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnAircraftCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();

		ToggleAirspaceMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnItemCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();

		ToggleItemsFilter();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}

static void MapBorderButtonOff(UINT8 ubBorderButtonIndex);
static void MapBorderButtonOn(UINT8 ubBorderButtonIndex);


void ToggleShowTownsMode( void )
{
	if (fShowTownFlag)
	{
		fShowTownFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
	}
	else
	{
		fShowTownFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_TOWN_BTN );

		if (fShowMineFlag)
		{
			fShowMineFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_MINE_BTN );
		}

		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}
	}

	fMapPanelDirty = TRUE;
}


void ToggleShowMinesMode( void )
{
	if (fShowMineFlag)
	{
		fShowMineFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_MINE_BTN );
	}
	else
	{
		fShowMineFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_MINE_BTN );

		if (fShowTownFlag)
		{
			fShowTownFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
		}

		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}
	}

	fMapPanelDirty = TRUE;
}


static bool DoesPlayerHaveAnyMilitia();


void ToggleShowMilitiaMode( void )
{
	if (fShowMilitia)
	{
		fShowMilitia = FALSE;
		MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
	}
	else
	{
		// toggle militia ON
		fShowMilitia = TRUE;
		MapBorderButtonOn( MAP_BORDER_MILITIA_BTN );

		// if Team is ON, turn it OFF
		if (fShowTeamFlag)
		{
			fShowTeamFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
		}

/*
		// if Airspace is ON, turn it OFF
		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}
*/

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}


		// check if player has any militia
		if (!DoesPlayerHaveAnyMilitia())
		{
			ST::string pwString;

			// no - so put up a message explaining how it works

			// if he's already training some
			if( IsAnyOneOnPlayersTeamOnThisAssignment( TRAIN_TOWN ) )
			{
				// say they'll show up when training is completed
				pwString = pMapErrorString[ 28 ];
			}
			else
			{
				// say you need to train them first
				pwString = zMarksMapScreenText[ 1 ];
			}

			BeginMapUIMessage(0, pwString);
		}
	}

	fMapPanelDirty = TRUE;
}


void ToggleShowTeamsMode( void )
{
	if (fShowTeamFlag)
	{
		// turn show teams OFF
		fShowTeamFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
	else
	{	// turn show teams ON
		TurnOnShowTeamsMode();
	}
}


void ToggleAirspaceMode( void )
{
	if (fShowAircraftFlag)
	{
		// turn airspace OFF
		fShowAircraftFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );

		if (fPlotForHelicopter)
		{
			AbortMovementPlottingMode( );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
	else
	{	// turn airspace ON
		TurnOnAirSpaceMode();
	}
}


static void TurnOnItemFilterMode(void);


void ToggleItemsFilter( void )
{
	if (fShowItemsFlag)
	{
		// turn items OFF
		fShowItemsFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_ITEM_BTN );

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
	else
	{
		// turn items ON
		TurnOnItemFilterMode();
	}
}

static void DisplayCurrentLevelMarker(void)
{
	// display the current level marker on the map border
/*
	if( fDisabledMapBorder )
	{
		return;
	}
*/

	// it's actually a white rectangle, not a green arrow!
	BltVideoObject(guiSAVEBUFFER, guiLEVELMARKER, 0, MAP_LEVEL_MARKER_X, MAP_LEVEL_MARKER_Y + MAP_LEVEL_MARKER_DELTA * iCurrentMapSectorZ);
}


void RenderMapLevelMarker(void)
{
	if (fShowMapInventoryPool) return;

	// Draws straight to FRAME_BUFFER (not guiSAVEBUFFER, unlike
	// DisplayCurrentLevelMarker() above) so it always ends up on top,
	// regardless of what else was blitted into guiSAVEBUFFER earlier in the
	// frame -- same pattern as CheckForAndRenderNewMailOverlay() in
	// MapScreen.cc, called at the same point in the per-frame render
	// sequence (after RenderButtons()). On the compact strategic-screen
	// tier, the marker's row now sits on map_screen_bottom.sti's own
	// territory (mbs.sti was shortened there); re-drawing into guiSAVEBUFFER
	// alone (tried first) wasn't enough to make it visible, since nothing
	// re-invalidated that exact screen rect afterwards.
	INT16 const x = MAP_LEVEL_MARKER_X;
	INT16 const y = MAP_LEVEL_MARKER_Y + MAP_LEVEL_MARKER_DELTA * iCurrentMapSectorZ;
	BltVideoObject(FRAME_BUFFER, guiLEVELMARKER, 0, x, y);
	InvalidateRegion(x, y, x + MAP_LEVEL_MARKER_WIDTH, y + MAP_LEVEL_MARKER_DELTA);
}


static void LevelMarkerBtnCallback(MOUSE_REGION* pRegion, UINT32 iReason);


void CreateMouseRegionsForLevelMarkers(void)
{
	for (UINT sCounter = 0; sCounter < 4 ; ++sCounter)
	{
		MOUSE_REGION* const r = &LevelMouseRegions[sCounter];
		const UINT16        x = MAP_LEVEL_MARKER_X;
		const UINT16        y = MAP_LEVEL_MARKER_Y + MAP_LEVEL_MARKER_DELTA * sCounter;
		const UINT16        w = MAP_LEVEL_MARKER_WIDTH;
		const UINT16        h = MAP_LEVEL_MARKER_DELTA;
		MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, LevelMarkerBtnCallback);

		MSYS_SetRegionUserData(r, 0, sCounter);

		ST::string sString = ST::format("{} {}", zMarksMapScreenText[0], sCounter + 1);
		r->SetFastHelpText(sString);
	}
}


void DeleteMouseRegionsForLevelMarkers()
{
	FOR_EACH(MOUSE_REGION, i, LevelMouseRegions) MSYS_RemoveRegion(&*i);
}


static void LevelMarkerBtnCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	// btn callback handler for assignment screen mask region
	INT32 iCounter = 0;

	iCounter = MSYS_GetRegionUserData( pRegion, 0 );

	if( ( iReason & MSYS_CALLBACK_REASON_POINTER_UP ) )
	{
		JumpToLevel( iCounter );
	}
}


/*
void DisableMapBorderRegion( void )
{
	// will shutdown map border region

	if( fDisabledMapBorder )
	{
		// checked, failed
		return;
	}

	// get rid of graphics and mouse regions
	DeleteMapBorderGraphics( );


	fDisabledMapBorder = TRUE;
}

void EnableMapBorderRegion( void )
{
	// will re-enable mapborder region

	if (!fDisabledMapBorder)
	{
		// checked, failed
		return;
	}

	// re load graphics and buttons
	LoadMapBorderGraphics( );

	fDisabledMapBorder = FALSE;

}
*/


void TurnOnShowTeamsMode( void )
{
	// if mode already on, leave, else set and redraw

	if (!fShowTeamFlag)
	{
		fShowTeamFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_TEAMS_BTN );

		if (fShowMilitia)
		{
			fShowMilitia = FALSE;
			MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
		}

/*
		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}
*/

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
}



void TurnOnAirSpaceMode( void )
{
	// if mode already on, leave, else set and redraw

	if (!fShowAircraftFlag)
	{
		fShowAircraftFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_AIRSPACE_BTN );


		// Turn off towns & mines (mostly because town/mine names overlap SAM site names)
		if (fShowTownFlag)
		{
			fShowTownFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
		}

		if (fShowMineFlag)
		{
			fShowMineFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_MINE_BTN );
		}

/*
		// Turn off teams and militia
		if (fShowTeamFlag)
		{
			fShowTeamFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
		}

		if (fShowMilitia)
		{
			fShowMilitia = FALSE;
			MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
		}
*/

		// Turn off items
		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}

		if ( bSelectedDestChar != -1 )
		{
			AbortMovementPlottingMode( );
		}


		// if showing underground
		if ( iCurrentMapSectorZ != 0 )
		{
			// switch to the surface
			JumpToLevel( 0 );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
}


static void TurnOnItemFilterMode(void)
{
	// if mode already on, leave, else set and redraw

	if (!fShowItemsFlag)
	{
		fShowItemsFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_ITEM_BTN );


		// Turn off towns, mines, teams, militia & airspace if any are on
		if (fShowTownFlag)
		{
			fShowTownFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
		}

		if (fShowMineFlag)
		{
			fShowMineFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_MINE_BTN );
		}

		if (fShowTeamFlag)
		{
			fShowTeamFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
		}

		if (fShowMilitia)
		{
			fShowMilitia = FALSE;
			MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
		}

		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}

		if (bSelectedDestChar != -1 || fPlotForHelicopter)
		{
			AbortMovementPlottingMode( );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
}


// set button states to match map flags
static void InitializeMapBorderButtonStates(void)
{
	if( fShowItemsFlag )
	{
		MapBorderButtonOn( MAP_BORDER_ITEM_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
	}

	if( fShowTownFlag )
	{
		MapBorderButtonOn( MAP_BORDER_TOWN_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
	}

	if( fShowMineFlag )
	{
		MapBorderButtonOn( MAP_BORDER_MINE_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_MINE_BTN );
	}

	if( fShowTeamFlag )
	{
		MapBorderButtonOn( MAP_BORDER_TEAMS_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
	}

	if( fShowAircraftFlag )
	{
		MapBorderButtonOn( MAP_BORDER_AIRSPACE_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
	}

	if( fShowMilitia )
	{
		MapBorderButtonOn( MAP_BORDER_MILITIA_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
	}
}


static bool DoesPlayerHaveAnyMilitia()
{
	FOR_EACH(SECTORINFO const, i, SectorInfo)
	{
		UINT8 const (&n)[MAX_MILITIA_LEVELS] = i->ubNumberOfCivsAtLevel;
		if (n[GREEN_MILITIA] + n[REGULAR_MILITIA] + n[ELITE_MILITIA] != 0) return true;
	}
	return false;
}


static void CommonBtnCallbackBtnDownChecks(void)
{
	// any click cancels MAP UI messages, unless we're in confirm map move mode
	if (g_ui_message_overlay != NULL && !gfInConfirmMapMoveMode)
	{
		CancelMapUIMessage( );
	}
}



void InitMapScreenFlags( void )
{
	fShowTownFlag = TRUE;
	fShowMineFlag = FALSE;

	fShowTeamFlag = TRUE;
	fShowMilitia = FALSE;

	fShowAircraftFlag = FALSE;
	fShowItemsFlag = FALSE;
}


static void MapBorderButtonOff(UINT8 ubBorderButtonIndex)
{
	Assert( ubBorderButtonIndex < 6 );

	if( fShowMapInventoryPool )
	{
		return;
	}

	// if button doesn't exist, return
	GUIButtonRef const b = giMapBorderButtons[ubBorderButtonIndex];
	if (b) b->uiFlags &= ~BUTTON_CLICKED_ON;
}


static void MapBorderButtonOn(UINT8 ubBorderButtonIndex)
{
	Assert( ubBorderButtonIndex < 6 );

	if( fShowMapInventoryPool )
	{
		return;
	}

	GUIButtonRef const b = giMapBorderButtons[ubBorderButtonIndex];
	if (b) b->uiFlags |= BUTTON_CLICKED_ON;
}
