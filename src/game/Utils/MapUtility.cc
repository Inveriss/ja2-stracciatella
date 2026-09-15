#include "Input.h"
#include "Overhead_Types.h"
#include "SDL_keycode.h"
#include "SDL_timer.h"
#include "SGP.h"
#include "Font.h"
#include "HImage.h"
#include "Local.h"
#include "VObject.h"
#include "VSurface.h"
#include "WorldDef.h"
#include "FileMan.h"
#include "Overhead_Map.h"
#include "VObject_Blitters.h"
#include "STIConvert.h"
#include "Font_Control.h"
#include "WorldDat.h"
#include "Map_Information.h"
#include "Line.h"
#include "ScreenIDs.h"
#include "Video.h"
#include "Quantize.h"
#include "UILayout.h"

#include <memory>
#include <string_theory/format>


#define MINIMAP_X_SIZE		88
#define MINIMAP_Y_SIZE		44

// The overhead map's own natural render width -- a fixed, classic-engine
// constant (see RenderOverheadMap()'s other caller, Overhead_Map.cc's own
// tactical-screen "overhead view" toggle: STD_SCREEN_X + 640, and its
// OverheadRegion/OverheadBackgroundRegion mouse regions, all hardcoded to
// exactly 640 regardless of the current screen resolution), NOT the live
// SCREEN_WIDTH -- per user report. Using SCREEN_WIDTH here (a modern
// window's actual width, often 1024+, well past where the isometric render
// actually stops drawing map tiles) left everything past the map's true
// 640px-wide extent sampling whatever default "off the edge of the
// diamond" border tile the renderer draws there instead -- the same
// brown/black triangular pattern on every map, since that border tile
// doesn't depend on the specific map's own content.
#define OVERHEAD_MAP_RENDER_WIDTH 640

#define WINDOW_SIZE		2

static float     gdXStep;
static float     gdYStep;

// Utililty file for sub-sampling/creating our radar screen maps
// Loops though our maps directory and reads all .map files, subsamples an area, color
// quantizes it into an 8-bit image ans writes it to an sti file in radarmaps.


template<> ScreenID HandleScreen<MAPUTILITY_SCREEN>()
{
	static auto p24BitValues{ std::make_unique<SGPPaletteEntry[]>(MINIMAP_X_SIZE * MINIMAP_Y_SIZE) };

	static SGPVSurface* giMiniMap{ AddVideoSurface(MINIMAP_X_SIZE, MINIMAP_Y_SIZE, PIXEL_DEPTH) };
	static SGPVSurface* gi8BitMiniMap{ AddVideoSurface(MINIMAP_X_SIZE, MINIMAP_Y_SIZE, 8) };

	// Get the names (full path) of all map files in the user's home directory.
	// recursive=true (6th arg) -- per user report: map .dat files live in a
	// subdirectory (e.g. Maps/), not directly in the Stracciatella home
	// root, and findFilesInDir() defaults to non-recursive when the
	// argument is omitted. Without it, this always found zero files and
	// fell straight into the "no more files" branch below, which calls
	// requestGameExit() -- silently closing the whole editor with no error
	// logged, since it's a clean SDL_QUIT request, not a crash. sortResults
	// (5th arg) set to true so multiple maps process in a stable, readable
	// order.
	static auto const mapFiles{ FileMan::findFilesInDir(
		RustPointer<char>{ EngineOptions_getStracciatellaHome() }.get(),
		"dat", true, false, true, true) };

	// Set the file iterator to the first file.
	static auto currentFile{ mapFiles.begin() };

	UINT32 uiRGBColor;

	UINT32 bR, bG, bB, bAvR, bAvG, bAvB;
	INT16 s16BPPSrc, sDest16BPPColor;

	INT16 sX1, sX2, sY1, sY2, sTop, sBottom, sLeft, sRight;


	FLOAT dX, dY, dStartX, dStartY;
	INT32 iX, iY, iSubX1, iSubY1, iSubX2, iSubY2, iWindowX, iWindowY, iCount;
	SGPPaletteEntry pPalette[ 256 ];


	sDest16BPPColor = -1;
	bAvR = bAvG = bAvB = 0;

	FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));

	//OK, we are here, now loop through files
	if (currentFile == mapFiles.end())
	{
		requestGameExit();
		return MAPUTILITY_SCREEN;
	}

	// OK, load maps and do overhead shrinkage of them...
	try { LoadWorldAbsolute(*currentFile); }
	catch (...) { return ERROR_SCREEN; }

	// Render small map
	InitNewOverheadDB(giCurrentTilesetID);

	gfOverheadMapDirty = TRUE;

	RenderOverheadMap(0, WORLD_COLS / 2, 0, 0, OVERHEAD_MAP_RENDER_WIDTH, 320, TRUE);

	TrashOverheadMap( );

	// OK, NOW PROCESS OVERHEAD MAP ( SHOUIDL BE ON THE FRAMEBUFFER )
	gdXStep	= OVERHEAD_MAP_RENDER_WIDTH / (float)MINIMAP_X_SIZE;
	gdYStep	= 320 / (float)MINIMAP_Y_SIZE;
	dStartX = dStartY = 0;

	// Adjust if we are using a restricted map...
	if ( gMapInformation.ubRestrictedScrollID != 0 )
	{

		CalculateRestrictedMapCoords(NORTH, &sX1,    &sY1,     &sX2,   &sTop, OVERHEAD_MAP_RENDER_WIDTH, 320);
		CalculateRestrictedMapCoords(SOUTH, &sX1,    &sBottom, &sX2,   &sY2,  OVERHEAD_MAP_RENDER_WIDTH, 320);
		CalculateRestrictedMapCoords(WEST,  &sX1,    &sY1,     &sLeft, &sY2,  OVERHEAD_MAP_RENDER_WIDTH, 320);
		CalculateRestrictedMapCoords(EAST,  &sRight, &sY1,     &sX2,   &sY2,  OVERHEAD_MAP_RENDER_WIDTH, 320);

		gdXStep	= (float)( sRight - sLeft )/(float)MINIMAP_X_SIZE;
		gdYStep	= (float)( sBottom - sTop )/(float)MINIMAP_Y_SIZE;

		dStartX = sLeft;
		dStartY = sTop;
	}

	//LOCK BUFFERS

	dX = dStartX;
	dY = dStartY;


	{ SGPVSurface::Lock lsrc(FRAME_BUFFER);
		SGPVSurface::Lock ldst(giMiniMap);
		UINT16* const pSrcBuf          = lsrc.Buffer<UINT16>();
		UINT32  const uiSrcPitchBYTES  = lsrc.Pitch();
		UINT16* const pDestBuf         = ldst.Buffer<UINT16>();
		UINT32  const uiDestPitchBYTES = ldst.Pitch();

		for ( iX = 0; iX < MINIMAP_X_SIZE; iX++ )
		{
			dY = dStartY;

			for ( iY = 0; iY < MINIMAP_Y_SIZE; iY++ )
			{
				// Reset per pixel -- per user report, when the sampling
				// window below finds zero valid source pixels (iCount stays
				// 0, e.g. dX has walked past the actually-rendered source
				// area for a run of columns), the code used to leave
				// sDest16BPPColor/bAvR/bAvG/bAvB at whatever the PREVIOUS
				// pixel computed, smearing/repeating that stale color
				// instead of falling back to a defined value. Black,
				// matching RenderOverheadMap()'s own initial fill color for
				// anything it didn't actually draw tiles over.
				sDest16BPPColor = Get16BPPColor(FROMRGB(0, 0, 0));
				bAvR = bAvG = bAvB = 0;

				//OK, AVERAGE PIXELS
				iSubX1 = (INT32)dX - WINDOW_SIZE;

				iSubX2 = (INT32)dX + WINDOW_SIZE;

				iSubY1 = (INT32)dY - WINDOW_SIZE;

				iSubY2 = (INT32)dY + WINDOW_SIZE;

				iCount = 0;
				bR = bG = bB = 0;

				for ( iWindowX = iSubX1; iWindowX < iSubX2; iWindowX++ )
				{
					for ( iWindowY = iSubY1; iWindowY < iSubY2; iWindowY++ )
					{
						if (0 <= iWindowX && iWindowX < OVERHEAD_MAP_RENDER_WIDTH &&
								0 <= iWindowY && iWindowY < 320)
						{
							s16BPPSrc = pSrcBuf[ ( iWindowY * (uiSrcPitchBYTES/2) ) + iWindowX ];

							uiRGBColor = GetRGBColor( s16BPPSrc );

							bR += SGPGetRValue( uiRGBColor );
							bG += SGPGetGValue( uiRGBColor );
							bB += SGPGetBValue( uiRGBColor );

							// Average!
							iCount++;
						}
					}

				}

				if ( iCount > 0 )
				{
					bAvR = bR / (UINT8)iCount;
					bAvG = bG / (UINT8)iCount;
					bAvB = bB / (UINT8)iCount;

					sDest16BPPColor = Get16BPPColor( FROMRGB( bAvR, bAvG, bAvB ) );
				}

				//Write into dest!
				pDestBuf[ ( iY * (uiDestPitchBYTES/2) ) + iX ] = sDest16BPPColor;

				// p24BitValues is a tightly-packed MINIMAP_X_SIZE x MINIMAP_Y_SIZE
				// buffer (allocated as such, and later read that way by
				// QuantizeImage()/ProcessImage()/MapPalette(), which just walk
				// width*height consecutive entries with no stride concept at
				// all) -- indexing it with uiDestPitchBYTES/2 (giMiniMap's own
				// video-surface row pitch, typically padded/aligned wider
				// than 88) wrote each row at the wrong offset, corrupting the
				// data QuantizeImage() later read back, per user report.
				SGPPaletteEntry* const dst = &p24BitValues[iY * MINIMAP_X_SIZE + iX];
				dst->r = bAvR;
				dst->g = bAvG;
				dst->b = bAvB;

				//Increment
				dY += gdYStep;

			}

			//Increment
			dX += gdXStep;
		}
	}

	// RENDER!
	BltVideoSurface(FRAME_BUFFER, giMiniMap, 20, 360, NULL);


	ST::string zFilename2;
	//QUantize!
	{ SGPVSurface::Lock lsrc(gi8BitMiniMap);
		UINT8* const pDataPtr = lsrc.Buffer<UINT8>();
		{ SGPVSurface::Lock ldst(FRAME_BUFFER);
			UINT16* const pDestBuf         = ldst.Buffer<UINT16>();
			UINT32  const uiDestPitchBYTES = ldst.Pitch();
			QuantizeImage(pDataPtr, p24BitValues.get(), MINIMAP_X_SIZE, MINIMAP_Y_SIZE, pPalette);
			gi8BitMiniMap->SetPalette(pPalette);
			// Blit!
			Blt8BPPDataTo16BPPBuffer(pDestBuf, uiDestPitchBYTES, gi8BitMiniMap, pDataPtr, 300, 360);

			// Write palette!
			{
				INT32 cnt;
				INT32 sX = 0, sY = 420;
				UINT16 usLineColor;

				SetClippingRegionAndImageWidth(uiDestPitchBYTES, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

				for ( cnt = 0; cnt < 256; cnt++ )
				{
					usLineColor = Get16BPPColor(FROMRGB(pPalette[cnt].r, pPalette[cnt].g, pPalette[cnt].b));
					LineDraw(FALSE, sX, sY, sX, sY + 10, usLineColor, pDestBuf);
					sX++;
					LineDraw(FALSE, sX, sY, sX, sY + 10, usLineColor, pDestBuf);
					sX++;
				}
			}
		}

		zFilename2 = FileMan::replaceExtension(*currentFile, "sti");
		WriteSTIFile(pDataPtr, pPalette, MINIMAP_X_SIZE, MINIMAP_Y_SIZE, zFilename2, CONVERT_ETRLE_COMPRESS, 0);
	}

	SetFontAttributes(TINYFONT1, FONT_MCOLOR_DKGRAY);
	MPrint(10, 340, ST::format("Writing radar image {}", zFilename2));
	MPrint(10, 350, ST::format("Using tileset {}", gTilesets[giCurrentTilesetID].zName));

	InvalidateScreen();
	RefreshScreen();

	InputAtom InputEvent;
	while (DequeueEvent(&InputEvent))
	{
		if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_ESCAPE)
		{ // Exit the program
			requestGameExit();
		}
	}

	if (_KeyDown(SDLK_SPACE))
	{
		// Wait two seconds to get the chance to see what is happening
		// for debugging purposes.
		SDL_Delay(2000);
	}

	// Set next
	++currentFile;

	return MAPUTILITY_SCREEN;
}
