#ifndef __RADAR_SCREEN_H
#define __RADAR_SCREEN_H

#include "Types.h"


void LoadRadarScreenBitmap(const ST::string&);

// RADAR WINDOW DEFINES
// WIDTH/HEIGHT: factory default size -- must stay in sync with
// MapUtility.cc's own MINIMAP_X_SIZE/Y_SIZE (the generator), since this is
// blitted 1:1 with no runtime scaling (RenderRadarScreen(), Radar_Screen.cc).
// Confirmed working at 250x125 in a live test, per user request, then
// reverted back to this default.
#define RADAR_WINDOW_X		(g_ui.get_RADAR_WINDOW_X())
#define RADAR_WINDOW_TM_Y	(g_ui.get_RADAR_WINDOW_TM_Y())
#define RADAR_WINDOW_WIDTH	88
#define RADAR_WINDOW_HEIGHT	44

void InitRadarScreen(void);
void RenderRadarScreen(void);
void MoveRadarScreen(void);

// toggle rendering flag of radar screen
void ToggleRadarScreenRender( void );

// clear out the video object for the radar map
void ClearOutRadarMapImage( void );

// do we render the radar screen?..or the squad list?
extern BOOLEAN   fRenderRadarScreen;

// BIG RADAR WINDOW -- the sector-inventory "big minimap" (bottom-left corner
// of the strategic screen), shown only while the sector-inventory window is
// open, hovered by the cursor, and no item is on the cursor -- see
// RenderBigRadarScreenIfVisible() (Radar_Screen.cc) and
// IsCursorOverSectorInventoryWindow() (Map_Screen_Interface_Map_Inventory.cc).
// WIDTH/HEIGHT must stay in sync with MapUtility.cc's own
// RADAR_BIG_X_SIZE/Y_SIZE (the generator), since this is blitted 1:1 with no
// runtime scaling, same convention as RADAR_WINDOW_WIDTH/HEIGHT above.
// Position/frame per user request -- one shared frame graphic for both
// screen-height tiers (g_ui.isCompactStrategicScreen()), only Y differs.
#define RADAR_WINDOW_BIG_WIDTH		238
#define RADAR_WINDOW_BIG_HEIGHT		119
#define RADAR_WINDOW_BIG_FRAME_X	0
#define RADAR_WINDOW_BIG_FRAME_Y	(g_ui.isCompactStrategicScreen() ? 573 : 621)
#define RADAR_WINDOW_BIG_X			12
#define RADAR_WINDOW_BIG_Y			(g_ui.isCompactStrategicScreen() ? 591 : 639)

void LoadBigRadarScreenBitmap(const ST::string&);
void ClearOutBigRadarMapImage(void);
void RenderBigRadarScreenIfVisible(void);

#endif
