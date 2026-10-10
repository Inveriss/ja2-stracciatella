#include "Loading_Screen.h"

#include "Campaign_Types.h"
#include "ContentManager.h"
#include "CrashHandler.h"
#include "Debug.h"
#include "Directories.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "GameInstance.h"
#include "LoadingScreenModel.h"
#include "Random.h"
#include "SAM_Sites.h"
#include "Strategic_Movement.h"
#include "VSurface.h"
#include "Video.h"
#include "UILayout.h"

#include <SDL.h>
#include <string_theory/format>
#include <memory>


UINT8 gubLastLoadingScreenID = LOADINGSCREEN_NOTHING;

// the load screen picture on the screen, for the "press any key" line
static SGPBox g_load_screen_box;


UINT8 GetLoadScreenID(const SGPSector& sector)
{
	bool  const night     = NightTime();
	UINT8 const sector_id = sector.AsByte();
	
	if (DidGameJustStart()) return LOADINGSCREEN_HELI;

	const LoadingScreen* screen = GCM->getLoadingScreenForSector(sector_id, sector.z, night);
	if (screen != NULL)
	{
		return screen->index;
	}

	switch (sector.z)
	{
		case 0:
			if (IsThisSectorASAMSector(sector))
			{
				return night ? LOADINGSCREEN_NIGHTSAM : LOADINGSCREEN_DAYSAM;
			}

			switch (SectorInfo[sector_id].ubTraversability[THROUGH_STRATEGIC_MOVE])
			{
				case TOWN:
					return Random(2) == 0 ?
						(night ? LOADINGSCREEN_NIGHTTOWN1 : LOADINGSCREEN_DAYTOWN1) :
						(night ? LOADINGSCREEN_NIGHTTOWN2 : LOADINGSCREEN_DAYTOWN2);
				case SAND:
				case SAND_ROAD:
					return night ? LOADINGSCREEN_NIGHTDESERT : LOADINGSCREEN_DAYDESERT;
				case FARMLAND:
				case FARMLAND_ROAD:
				case ROAD:
					return night ? LOADINGSCREEN_NIGHTGENERIC : LOADINGSCREEN_DAYGENERIC;
				case PLAINS:
				case SPARSE:
				case HILLS:
				case PLAINS_ROAD:
				case SPARSE_ROAD:
				case HILLS_ROAD:
					return night ? LOADINGSCREEN_NIGHTWILD : LOADINGSCREEN_DAYWILD;
				case DENSE:
				case SWAMP:
				case SWAMP_ROAD:
				case DENSE_ROAD:
					return night ? LOADINGSCREEN_NIGHTFOREST : LOADINGSCREEN_DAYFOREST;
				case TROPICS:
				case TROPICS_ROAD:
				case WATER:
				case NS_RIVER:
				case EW_RIVER:
				case COASTAL:
				case COASTAL_ROAD:
					return night ? LOADINGSCREEN_NIGHTTROPICAL : LOADINGSCREEN_DAYTROPICAL;
				default:
					Assert(false);
					return night ? LOADINGSCREEN_NIGHTGENERIC : LOADINGSCREEN_DAYGENERIC;
			}

		case 1:
			// All sectors at this level, except mine sectors, are mapped to specific loading screens
			return LOADINGSCREEN_MINE;

		case 2:
		case 3:
			// All level 2 and 3 maps are caves.
			return LOADINGSCREEN_CAVE;

		default:
			Assert(false); // Shouldn't ever happen
			return night ? LOADINGSCREEN_NIGHTGENERIC : LOADINGSCREEN_DAYGENERIC;
	}
}


void DisplayLoadScreenWithID(UINT8 const id)
{
	const LoadingScreen* screen = GCM->getLoadingScreen(id);
	ST::string filename = LOADSCREENSDIR + screen->filename;

	g_load_screen_box = { (UINT16)STD_SCREEN_X, (UINT16)STD_SCREEN_Y, 640, 480 };
	try
	{ // Blit the background image.
		std::unique_ptr<SGPVSurface> src(AddVideoSurfaceFromFile(filename.c_str()));
		BltVideoSurface(FRAME_BUFFER, src.get(), STD_SCREEN_X, STD_SCREEN_Y, NULL);
		g_load_screen_box.w = src->Width();
		g_load_screen_box.h = src->Height();
	}
	catch (...)
	{ // Failed to load the file, so use a black screen and print out message.
		SetFontAttributes(FONT10ARIAL, FONT_YELLOW);
		FRAME_BUFFER->Fill(0);
		MPrint(5, 5, ST::format("{} loadscreen data file not found", filename));
	}

	gubLastLoadingScreenID = id;
	InvalidateScreen();
	RefreshScreen();
}


void WaitForKeyOnLoadScreen()
{
	// the line in the picture's bottom right corner
	ST::string const text = "Press Any Key to Continue";
	SGPFont const font = FONT14ARIAL;
	if (g_load_screen_box.w == 0) g_load_screen_box = { (UINT16)STD_SCREEN_X, (UINT16)STD_SCREEN_Y, 640, 480 };
	INT32 const right  = std::min<INT32>(g_load_screen_box.x + g_load_screen_box.w, SCREEN_WIDTH);
	INT32 const bottom = std::min<INT32>(g_load_screen_box.y + g_load_screen_box.h, SCREEN_HEIGHT);
	SetFontDestBuffer(FRAME_BUFFER);
	SetFontAttributes(font, FONT_WHITE);
	MPrint(right - 10 - StringPixLength(text, font), bottom - 10 - GetFontHeight(font), text);
	InvalidateScreen();
	RefreshScreen();

	/* Wait for a key or a mouse button: pressed, then released, so that the
	 * release does not reach the screen that comes next. Blocked on purpose:
	 * not a hang. */
	CrashHandlerPauseWatchdog(true);
	enum { NOTHING, KEY, BUTTON } pressed = NOTHING;
	for (bool done = false; !done;)
	{
		SDL_Event event;
		if (!SDL_WaitEventTimeout(&event, 50))
		{ // keeps the window drawn
			InvalidateScreen();
			RefreshScreen();
			continue;
		}
		switch (event.type)
		{
			case SDL_QUIT: // for the main loop
				SDL_PushEvent(&event);
				done = true;
				break;

			case SDL_KEYDOWN:         if (pressed == NOTHING) pressed = KEY;    break;
			case SDL_MOUSEBUTTONDOWN: if (pressed == NOTHING) pressed = BUTTON; break;
			case SDL_KEYUP:           done = pressed == KEY;    break;
			case SDL_MOUSEBUTTONUP:   done = pressed == BUTTON; break;
		}
	}
	CrashHandlerPauseWatchdog(false);
}
