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

#endif
