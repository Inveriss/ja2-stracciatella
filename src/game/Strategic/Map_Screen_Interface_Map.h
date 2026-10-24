#ifndef _MAP_SCREEN_INTERFACE_MAP_H
#define _MAP_SCREEN_INTERFACE_MAP_H

#include "JA2Types.h"
#include "UILayout.h"
#include <string_theory/string>


void InitMapScreenInterfaceMap();

// functions
void DrawMapIndexBigMap( BOOLEAN fSelectedCursorIsYellow );
//void DrawMapIndexSmallMap( BOOLEAN fSelectedCursorIsYellow );

void DrawMap(void);

void GetScreenXYFromMapXY(const SGPSector& sMap, INT16 *psX, INT16 *psY);

// plot path for helicopter
void PlotPathForHelicopter(const SGPSector& sector);

// the temp path, where the helicopter could go
void PlotATemporaryPathForHelicopter(const SGPSector& sector);

// show arrows for this char
void DisplayPathArrows(UINT16 usCharNum, HVOBJECT hMapHandle);

// build path for character
void PlotPathForCharacter(SOLDIERTYPE&, const SGPSector& sector, bool tactical_traversal);

// build temp path for character
void PlotATemporaryPathForCharacter(const SOLDIERTYPE* s, const SGPSector& sector);


// display current/temp paths
void DisplaySoldierPath( SOLDIERTYPE *pCharacter );
void DisplaySoldierTempPath(void);
void DisplayHelicopterPath( void );
void DisplayHelicopterTempPath( void );


// clear path after this sector
UINT32 ClearPathAfterThisSectorForCharacter(SOLDIERTYPE *pCharacter, const SGPSector& sMap);

// cancel path : clear the path completely and gives player feedback message that the route was canceled
void CancelPathForCharacter( SOLDIERTYPE *pCharacter );
void CancelPathForVehicle(VEHICLETYPE&, BOOLEAN fAlreadyReversed);

// check if we have waited long enought o update temp path
void DisplayThePotentialPathForHelicopter(const SGPSector& sector);

// clear out helicopter list after this sector
UINT32 ClearPathAfterThisSectorForHelicopter(const SGPSector& sMap);


// check to see if sector is highlightable
bool IsTheCursorAllowedToHighLightThisSector(const SGPSector& sMap);

// restore background for map grids
void RestoreBackgroundForMapGrid(const SGPSector& sMap);

// clip blits to map view region
void ClipBlitsToMapViewRegion( void );
void ClipBlitsToMapViewRegionForRectangleAndABit( UINT32 uiDestPitchBYTES );

// clip blits to full screen....restore after use of ClipBlitsToMapViewRegion( )
void RestoreClipRegionToFullScreen( void );
void RestoreClipRegionToFullScreenForRectangle( UINT32 uiDestPitchBYTES );

// last sector in helicopter's path
INT16 GetLastSectorOfHelicoptersPath( void );

// display info about helicopter path
void DisplayDistancesForHelicopter( void );

// display where hei is
void DisplayPositionOfHelicopter( void );

// check for click
BOOLEAN CheckForClickOverHelicopterIcon(const SGPSector& sMap);

void LoadMapScreenInterfaceMapGraphics();
void DeleteMapScreenInterfaceMapGraphics();


// grab the total number of militia in sector
INT32 GetNumberOfMilitiaInSector(const SGPSector& sector);

// create destroy
void CreateDestroyMilitiaPopUPRegions( void );

// draw the militia box
void DrawMilitiaPopUpBox();


//Returns true if the player knows how many enemies are in the sector if that number is greater than 0.
//Returns false for all other cases.
UINT32 WhatPlayerKnowsAboutEnemiesInSector(const SGPSector& sSector);

//There is a special case flag used when players encounter enemies in a sector, then retreat.  The number of enemies
//will display on mapscreen until time is compressed.  When time is compressed, the flag is cleared, and
//a question mark is displayed to reflect that the player no longer knows.  This is the function that clears that
//flag.
void ClearAnySectorsFlashingNumberOfEnemies(void);


enum {
	ABORT_PLOTTING = 0,
	PATH_CLEARED,
	PATH_SHORTENED,
};

// what the player knows about the enemies in a given sector
enum {
	KNOWS_NOTHING = 0,
	KNOWS_THEYRE_THERE,
	KNOWS_HOW_MANY,
};


// size of squares on the map
#define MAP_GRID_X		43
#define MAP_GRID_Y		35


// map view region
#define MAP_VIEW_START_X	(MAP_SCREEN_RIGHT_BLOCK_X + 270)
#define MAP_VIEW_START_Y	(MAP_SCREEN_Y + 10)
// 16x16 visible sector grid (MAP_WORLD_X/Y is 18, with a 1-sector border on
// each side excluded from view) x MAP_GRID_X/Y, per user request.
#define MAP_VIEW_WIDTH		688
#define MAP_VIEW_HEIGHT		560


// number of units wide
#define WORLD_MAP_X		18

// dirty regions for the map
#define DMAP_GRID_X		( MAP_GRID_X + 1 )
#define DMAP_GRID_Y		( MAP_GRID_Y + 1 )


// Tixa position on the map
#define TIXA_SECTOR_X		9
#define TIXA_SECTOR_Y		10


// wait time until temp path is drawn, from placing cursor on a map grid
#define MIN_WAIT_TIME_FOR_TEMP_PATH 200


// the number of militia on the cursor
extern INT16 sGreensOnCursor;
extern INT16 sRegularsOnCursor;
extern INT16 sElitesOnCursor;
extern INT16 sHeaviesOnCursor;

// highlighted sectors
extern SGPSector gsHighlightSector;

// the viewable map bound region
extern SGPRect MapScreenRect;

// draw temp path
extern BOOLEAN fDrawTempHeliPath;

// selected destination char
extern INT8 bSelectedDestChar;

// current assignment character
extern INT8 bSelectedAssignChar;

// the contract char
extern INT8 bSelectedContractChar;

// has temp path for character path or helicopter been already drawn
extern BOOLEAN  fTempPathAlreadyDrawn;

// the currently selected town militia
extern INT16 sSelectedMilitiaTown;

// the selected sectors
extern SGPSector sSelMap;


void    CreateDestroyMilitiaSectorButtons(void);
BOOLEAN CanRedistributeMilitiaInSector(INT16 bClickedTownId);

// The militia box's arrival mode (docs/militia-arrival-box-plan.md): opens
// the box for the town with an arrived order's militia (counts per rank) as
// the unassigned ones; false when it can't now (another box or panel, the
// map inventory, combat in the town)
bool OpenMilitiaArrivalBox(UINT32 order_id, INT16 town, UINT8 const counts[]);
// The box could open now (no other box or panel, no map inventory)
bool CanOpenMilitiaArrivalBox();
// The message shown before the box opens for an arrived order of total
// militia: the header after the room, "Choose which militia:" and the
// choices there are (docs/militia-arrival-box-plan.md, D1, D1b)
ST::string MilitiaArrivalMessage(INT16 key, UINT32 total);
// The militia box's key of a sector (sSelectedMilitiaTown): its town id, a
// SAM site's own key outside the towns, 0: no box
INT16 MilitiaBoxKeyForSector(const SGPSector&);
// The town of the box open in the arrival mode, 0: none
INT16 MilitiaArrivalBoxTown();
// Its order, 0: none
UINT32 MilitiaArrivalBoxOrder();
// Another order for the same town: its militia join the unassigned ones
void AddToMilitiaArrivalBox(UINT8 const counts[]);
// A new or loaded game: no box in the arrival mode
void ResetMilitiaArrivalBox();
// An order arrived on the tactical screen (D5.1): the map screen is entered,
// the box opens there and its closing goes back to the tactical screen
void RequestMilitiaArrivalBoxFromTactical();

#endif
