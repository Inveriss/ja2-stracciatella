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
#include "Cursors.h"
#include "EditorDefines.h"
#include "Cursor_Control.h"

#include "RenderWorld.h"

#include <algorithm>
#include <memory>
#include <vector>
#include <string_theory/format>


// Factory default size -- must stay in sync with Radar_Screen.h's own
// RADAR_WINDOW_WIDTH/HEIGHT (the in-game display size, blitted 1:1 with no
// runtime scaling). Confirmed working at 250x125 in a live test, per user
// request, then reverted back to this default.
#define MINIMAP_X_SIZE		88
#define MINIMAP_Y_SIZE		44

// Second, larger minimap set generated alongside the factory-default one
// above, for the strategic screen's sector-inventory "big minimap"
// (Radar_Screen.h's RADAR_WINDOW_BIG_WIDTH/HEIGHT) -- written as
// "<mapname>.big.sti" next to each map's own "<mapname>.sti", same
// directory, same per-map loop, sampled from the same already-rendered
// overhead-map framebuffer content (see the second pass below).
// 580x290 per user request (was 238x119, the size the game's big minimap --
// RADAR_WINDOW_BIG_WIDTH/HEIGHT -- still shows).
#define RADAR_BIG_X_SIZE	580
#define RADAR_BIG_Y_SIZE	290

// Third set, for the tactical placement's minimap (1366x768 panels,
// Data/RadarMaps_Overhead) -- "<mapname>.overhead.sti", made the same way.
// 800x400 per user request (was 640x320, the overhead map's own size, before
// that 352x176): bigger than the overhead map, so it is not made from it but
// from a picture of the sector drawn 1:1 with the real tiles (3200x1600, see
// CaptureWorldPicture()), each pixel the average of a 4x4 block of it.
#define RADAR_OVERHEAD_X_SIZE	800
#define RADAR_OVERHEAD_Y_SIZE	400

// The part of the world the overhead map shows, in the renderer's screen
// coordinates (FromCellToScreenCoordinates() of a cell is the top corner of
// its tile's diamond): the overhead map starts with the tile at map position
// (0, WORLD_ROWS / 2), its 8x4 picture at (0, 0) -- the 40x20 one's left edge
// is 20 px left of that corner -- and is the world scaled 5:1.
#define WORLD_PICTURE_X	(-(WORLD_ROWS / 2) * (WORLD_TILE_X / 2) - WORLD_TILE_X / 2)     // -1620
#define WORLD_PICTURE_Y	((WORLD_ROWS / 2) * CELL_Y_SIZE)                               // 800
#define WORLD_PICTURE_W	(OVERHEAD_MAP_RENDER_WIDTH * 5)                                // 3200
#define WORLD_PICTURE_H	(320 * 5)                                                      // 1600
static_assert(WORLD_PICTURE_X == -1620 && WORLD_PICTURE_Y == 800, "the overhead map's origin in the world");

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

// The compressed .sti format (STIConvert.cc's ETRLE writer, TCI == 0x00)
// always treats palette index 0 as fully transparent, regardless of what
// color actually sits there -- and QuantizeImage()'s (Quantize.cc) octree
// traversal happens to assign index 0 to whatever real color cluster it
// visits first, not a reserved value (confirmed by user report: it's a
// different, non-black color for every map). Moving index 0's real content
// to another, genuinely unused index means nothing in the actual minimap
// image points at index 0 anymore, so it can never accidentally mask out
// real pixels that happen to quantize to whatever color the quantizer put
// there.
//
// The target index must be one MapPalette() (Quantize.cc) never actually
// assigned to any pixel -- found here by scanning for the highest index
// actually in use and picking the next one up. The two QuantizeImage() call
// sites below pass sMaxColors == 254 (not the default 255), which GUARANTEES
// index 254 is always free regardless of how many distinct colors a given
// map's minimap actually has -- confirmed necessary by user report: these
// heavily-downscaled/averaged minimaps routinely use close to the full
// 255-color budget, so relying on "probably some high index is unused"
// left plenty of maps unfixed.
//
// Index 255 specifically must be avoided as the target: it's WI in
// STIConvert.cc, an entirely separate convention from TCI meaning
// "subimage/wall boundary", checked by DetermineSubImageSize() while
// scanning row 0/column 0 for where the image's used area ends. An earlier
// attempt swapped index 0's content there, which planted real 255-valued
// pixels in the image; every map's background-colored top-right corner
// (always outside the isometric diamond) then read as a "wall" 1 pixel
// early, truncating every generated minimap's detected width by 1 -- per
// user report. The sMaxColors == 254 cap keeps this function's own result
// (maxUsed + 1) at 254 at most, safely below that.
static void ReserveTransparentPaletteIndex(UINT8* const pData, SGPPaletteEntry* const pPalette, const INT32 pixelCount)
{
	UINT8 maxUsed = 0;
	for (INT32 i = 0; i < pixelCount; ++i)
	{
		if (pData[i] > maxUsed) maxUsed = pData[i];
	}

	// Defensive only -- unreachable as long as callers pass sMaxColors <= 254.
	if (maxUsed >= 254) return;

	UINT8 const freeIndex = maxUsed + 1;
	pPalette[freeIndex] = pPalette[0];
	pPalette[0]         = SGPPaletteEntry{};
	for (INT32 i = 0; i < pixelCount; ++i)
	{
		if (pData[i] == 0) pData[i] = freeIndex;
	}
}

static float     gdXStep;
static float     gdYStep;

// Utililty file for sub-sampling/creating our radar screen maps
// Loops though our maps directory and reads all .map files, subsamples an area, color
// quantizes it into an 8-bit image ans writes it to an sti file in radarmaps.


/* Draws the world's rectangle (x, y, w, h) -- renderer's screen coordinates,
 * see WORLD_PICTURE_X -- 1:1 into the top left corner of `picture`: one
 * viewport after the other, each copied to its place. A frame pixel (px, py)
 * shows the world's screen point
 *   (Sx + px - centre x, Sy + py - centre y + 10),
 * (Sx, Sy) being the render centre's (the +10 as in GetMouseWorldCoords());
 * the renderer snaps the centre to a tile, so the place of every viewport is
 * taken from where it really went, and they overlap by more than a snap. The
 * scroll limits are moved out of the way meanwhile: the picture reaches the
 * map's very edges. */
static void CaptureWorldPicture(SGPVSurface* const picture, INT32 const x, INT32 const y, INT32 const w, INT32 const h)
{
	INT16 const old_left   = gsLeftX;
	INT16 const old_top    = gsTopY;
	INT16 const old_right  = gsRightX;
	INT16 const old_bottom = gsBottomY;
	gsLeftX   -= 200;
	gsTopY    -= 200;
	gsRightX  += 200;
	gsBottomY += 200;

	INT32 const centre_x = g_ui.m_tacticalMapCenterX;
	INT32 const centre_y = g_ui.m_tacticalMapCenterY;
	INT32 const vx0      = gsVIEWPORT_START_X;
	INT32 const vx1      = gsVIEWPORT_END_X;
	INT32 const vy0      = std::max<INT32>(gsVIEWPORT_START_Y, gsVIEWPORT_WINDOW_START_Y);
	INT32 const vy1      = std::min<INT32>(gsVIEWPORT_END_Y,   gsVIEWPORT_WINDOW_END_Y);
	INT32 const overlap  = 80;
	INT32 const margin   = 30;
	INT32 const step_x   = std::max<INT32>(40, vx1 - vx0 - overlap);
	INT32 const step_y   = std::max<INT32>(20, vy1 - vy0 - overlap);

	picture->Fill(0);
	for (INT32 wy = y - margin; wy < y + h; wy += step_y)
	{
		for (INT32 wx = x - margin; wx < x + w; wx += step_x)
		{
			// the centre that shows (wx, wy) in the viewport's top left corner
			INT32 const sx = wx - vx0 + centre_x;
			INT32 const sy = wy - vy0 + centre_y - 10;
			FRAME_BUFFER->Fill(0);
			RenderWorldForSnapshot((INT16)((sx + 2 * sy) / 4), (INT16)((2 * sy - sx) / 4));

			// the world's point in the frame's top left corner, as it was drawn
			INT32 const ax = 2 * gsRenderCenterX - 2 * gsRenderCenterY - centre_x;
			INT32 const ay = gsRenderCenterX + gsRenderCenterY - centre_y + 10;

			INT32 const px0 = std::max(vx0, x - ax);
			INT32 const px1 = std::min(vx1, x + w - ax);
			if (px0 >= px1) continue;

			SGPVSurface::Lock lsrc(FRAME_BUFFER);
			SGPVSurface::Lock ldst(picture);
			UINT16 const* const src       = lsrc.Buffer<UINT16>();
			UINT16*       const dst       = ldst.Buffer<UINT16>();
			UINT32        const src_pitch = lsrc.Pitch() / 2;
			UINT32        const dst_pitch = ldst.Pitch() / 2;
			for (INT32 py = vy0; py != vy1; ++py)
			{
				INT32 const dy = ay + py - y;
				if (dy < 0 || h <= dy) continue;
				std::copy_n(src + py * src_pitch + px0, px1 - px0, dst + dy * dst_pitch + (ax + px0 - x));
			}
		}
	}

	gsLeftX   = old_left;
	gsTopY    = old_top;
	gsRightX  = old_right;
	gsBottomY = old_bottom;
}


// The sizes to write, chosen in a prompt before the first map: any of them,
// at least one.
enum { SIZE_SMALL, SIZE_BIG, SIZE_OVERHEAD, NUM_RADAR_SIZES };
static bool g_write_size[NUM_RADAR_SIZES] = { true, true, true };
static bool g_sizes_chosen = false;

enum PromptResult { PROMPT_OPEN, PROMPT_START, PROMPT_CANCEL };

// Draws the prompt and handles its input: the keys 1-3 or a click on a line
// toggle a size, A takes all, Enter or a click on Start begins, Esc or a click
// on Cancel goes back to the editor.
static PromptResult RadarMapSizePrompt()
{
	static bool button_was_down = true; // the click that started the utility is not one of ours

	struct { char const* text; } const sizes[NUM_RADAR_SIZES] =
	{
		{ "88x44   <map>.sti   (tactical radar)" },
		{ "580x290   <map>.big.sti" },
		{ "800x400   <map>.overhead.sti   (tactical placement)" }
	};

	INT32 const x      = 60;
	INT32 const y      = 80;
	INT32 const line_h = 26;
	INT32 const w      = 520;
	auto const line_y  = [&](INT32 const i) { return y + 50 + i * line_h; };
	INT32 const start_y  = line_y(NUM_RADAR_SIZES) + 20;
	INT32 const cancel_y = start_y + line_h;

	bool any = false;
	for (bool const b : g_write_size) any |= b;

	PromptResult result = PROMPT_OPEN;

	InputAtom e;
	while (DequeueEvent(&e))
	{
		if (e.usEvent != KEY_DOWN) continue;
		switch (e.usParam)
		{
			case '1': g_write_size[SIZE_SMALL]    = !g_write_size[SIZE_SMALL];    break;
			case '2': g_write_size[SIZE_BIG]      = !g_write_size[SIZE_BIG];      break;
			case '3': g_write_size[SIZE_OVERHEAD] = !g_write_size[SIZE_OVERHEAD]; break;
			case 'a': for (bool& b : g_write_size) b = true;                      break;
			case SDLK_RETURN: if (any) result = PROMPT_START;                     break;
			case SDLK_ESCAPE: result = PROMPT_CANCEL;                             break;
		}
	}

	// a click: the button going down over a line
	bool const down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
	if (down && !button_was_down && x <= gusMouseXPos && gusMouseXPos < x + w)
	{
		for (INT32 i = 0; i != NUM_RADAR_SIZES; ++i)
		{
			if (line_y(i) <= gusMouseYPos && gusMouseYPos < line_y(i) + line_h) g_write_size[i] = !g_write_size[i];
		}
		if (start_y  <= gusMouseYPos && gusMouseYPos < start_y  + line_h && any) result = PROMPT_START;
		if (cancel_y <= gusMouseYPos && gusMouseYPos < cancel_y + line_h)        result = PROMPT_CANCEL;
	}
	button_was_down = down;

	FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
	SetFontDestBuffer(FRAME_BUFFER);
	SetFontAttributes(FONT14ARIAL, FONT_WHITE);
	MPrint(x, y,      "Radar maps: which sizes to create for every map?");
	MPrint(x, y + 20, "Click a line or press its key; A = all.");
	for (INT32 i = 0; i != NUM_RADAR_SIZES; ++i)
	{
		SetFontForeground(g_write_size[i] ? FONT_YELLOW : FONT_GRAY2);
		MPrint(x, line_y(i), ST::format("{}   [{}]   {}", i + 1, g_write_size[i] ? "X" : "  ", sizes[i].text));
	}
	any = false;
	for (bool const b : g_write_size) any |= b;
	SetFontForeground(any ? FONT_LTGREEN : FONT_GRAY2);
	MPrint(x, start_y,  "Enter   Start");
	SetFontForeground(FONT_WHITE);
	MPrint(x, cancel_y, "Esc   Cancel (back to the editor)");

	SetCurrentCursorFromDatabase(CURSOR_NORMAL);
	InvalidateScreen();
	RefreshScreen();

	if (result != PROMPT_OPEN) button_was_down = true; // for the next time
	return result;
}


template<> ScreenID HandleScreen<MAPUTILITY_SCREEN>()
{
	if (!g_sizes_chosen)
	{
		switch (RadarMapSizePrompt())
		{
			case PROMPT_OPEN:   return MAPUTILITY_SCREEN;
			case PROMPT_CANCEL:
				// the prompt painted over the editor: everything again
				gfRenderWorld   = TRUE;
				gfRenderTaskbar = TRUE;
				return EDIT_SCREEN;
			case PROMPT_START:  g_sizes_chosen = true; break;
		}
	}

	static auto p24BitValues{ std::make_unique<SGPPaletteEntry[]>(MINIMAP_X_SIZE * MINIMAP_Y_SIZE) };

	static SGPVSurface* giMiniMap{ AddVideoSurface(MINIMAP_X_SIZE, MINIMAP_Y_SIZE, PIXEL_DEPTH) };
	static SGPVSurface* gi8BitMiniMap{ AddVideoSurface(MINIMAP_X_SIZE, MINIMAP_Y_SIZE, 8) };

	// Big minimap set -- see RADAR_BIG_X_SIZE/Y_SIZE's own comment above.
	static auto p24BitValuesBig{ std::make_unique<SGPPaletteEntry[]>(RADAR_BIG_X_SIZE * RADAR_BIG_Y_SIZE) };

	static SGPVSurface* giMiniMapBig{ AddVideoSurface(RADAR_BIG_X_SIZE, RADAR_BIG_Y_SIZE, PIXEL_DEPTH) };
	static SGPVSurface* gi8BitMiniMapBig{ AddVideoSurface(RADAR_BIG_X_SIZE, RADAR_BIG_Y_SIZE, 8) };

	// Overhead minimap set -- see RADAR_OVERHEAD_X_SIZE/Y_SIZE above.
	static auto p24BitValuesOverhead{ std::make_unique<SGPPaletteEntry[]>(RADAR_OVERHEAD_X_SIZE * RADAR_OVERHEAD_Y_SIZE) };

	static SGPVSurface* giMiniMapOverhead{ AddVideoSurface(RADAR_OVERHEAD_X_SIZE, RADAR_OVERHEAD_Y_SIZE, PIXEL_DEPTH) };
	static SGPVSurface* gi8BitMiniMapOverhead{ AddVideoSurface(RADAR_OVERHEAD_X_SIZE, RADAR_OVERHEAD_Y_SIZE, 8) };
	// the sector 1:1, its source
	static SGPVSurface* giWorldPicture{ AddVideoSurface(WORLD_PICTURE_W, WORLD_PICTURE_H, PIXEL_DEPTH) };

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
				// anything it didn't actually draw tiles over. (Blue was
				// tried here per an earlier user request, then reverted --
				// see RenderOverheadMap()'s own "Black out" comment,
				// Overhead_Map.cc, for why: this fallback color's exact RGB
				// isn't actually what matters for the see-through-pixel bug
				// that caused -- it's whatever color QuantizeImage's octree
				// quantizer happens to assign to PALETTE INDEX 0, which the
				// .sti format always treats as transparent regardless of its
				// RGB value.)
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
			// sMaxColors capped to 254 (not the default 255) -- guarantees
			// index 254 is always free for ReserveTransparentPaletteIndex()
			// below to use, regardless of how many distinct colors this
			// particular map's minimap has. Confirmed necessary by user
			// report: many of these heavily-downscaled/averaged minimaps
			// actually do use close to the full 255-color budget, so relying
			// on "probably some high index is unused" left the original
			// index-0 masking bug unfixed for a large share of maps.
			QuantizeImage(pDataPtr, p24BitValues.get(), MINIMAP_X_SIZE, MINIMAP_Y_SIZE, pPalette, 254);
			ReserveTransparentPaletteIndex(pDataPtr, pPalette, MINIMAP_X_SIZE * MINIMAP_Y_SIZE);
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

		if (g_write_size[SIZE_SMALL])
		{
			zFilename2 = FileMan::replaceExtension(*currentFile, "sti");
			WriteSTIFile(pDataPtr, pPalette, MINIMAP_X_SIZE, MINIMAP_Y_SIZE, zFilename2, CONVERT_ETRLE_COMPRESS, 0);
		}
	}

	// Second and third passes: same already-rendered overhead-map framebuffer content
	// (RenderOverheadMap() above, still intact -- TrashOverheadMap() only
	// frees the overhead-map's own working data, not the pixels it already
	// blitted to FRAME_BUFFER), just resampled at RADAR_BIG_X_SIZE/Y_SIZE and
	// RADAR_OVERHEAD_X_SIZE/Y_SIZE instead of MINIMAP_X_SIZE/Y_SIZE. Mirrors the first pass's sampling
	// loop exactly (same averaging window, same per-pixel reset, same tight
	// p24BitValuesBig packing) -- see that pass's own comments above for why
	// each of those matters. dStartX/dStartY and, when restricted, sLeft/
	// sRight/sTop/sBottom are resolution-independent source-space bounds,
	// so they're reused as-is from the first pass above.
	std::vector<ST::string> written; // printed at the end
	auto const write_resampled = [&](INT32 const w, INT32 const h, SGPPaletteEntry* const values,
		SGPVSurface* const surf16, SGPVSurface* const surf8, char const* const extension, INT16 const text_y,
		INT32 const window) // the averaging window reaches this far from the sample: 2 -> 4x4 px, 1 -> 2x2 px, 0 -> the pixel itself
	{
		float const gdXStepBig = (gMapInformation.ubRestrictedScrollID != 0)
			? (float)(sRight - sLeft) / (float)w
			: OVERHEAD_MAP_RENDER_WIDTH / (float)w;
		float const gdYStepBig = (gMapInformation.ubRestrictedScrollID != 0)
			? (float)(sBottom - sTop) / (float)h
			: 320 / (float)h;

		FLOAT dXBig = dStartX;
		FLOAT dYBig;

		{ SGPVSurface::Lock lsrc(FRAME_BUFFER);
			SGPVSurface::Lock ldst(surf16);
			UINT16* const pSrcBuf          = lsrc.Buffer<UINT16>();
			UINT32  const uiSrcPitchBYTES  = lsrc.Pitch();
			UINT16* const pDestBuf         = ldst.Buffer<UINT16>();
			UINT32  const uiDestPitchBYTES = ldst.Pitch();

			for (INT32 iXBig = 0; iXBig < w; iXBig++)
			{
				dYBig = dStartY;

				for (INT32 iYBig = 0; iYBig < h; iYBig++)
				{
					// Black fallback, matching the small pass above -- see its
					// own comment for why (reverted from blue).
					INT16 sDestBig = Get16BPPColor(FROMRGB(0, 0, 0));
					UINT32 bAvRBig = 0, bAvGBig = 0, bAvBBig = 0;

					INT32 const iSubX1 = (INT32)dXBig - window;
					INT32 const iSubX2 = (INT32)dXBig + std::max(window, 1);
					INT32 const iSubY1 = (INT32)dYBig - window;
					INT32 const iSubY2 = (INT32)dYBig + std::max(window, 1);

					INT32 iCountBig = 0;
					UINT32 bRBig = 0, bGBig = 0, bBBig = 0;

					for (INT32 iWindowX = iSubX1; iWindowX < iSubX2; iWindowX++)
					{
						for (INT32 iWindowY = iSubY1; iWindowY < iSubY2; iWindowY++)
						{
							if (0 <= iWindowX && iWindowX < OVERHEAD_MAP_RENDER_WIDTH &&
									0 <= iWindowY && iWindowY < 320)
							{
								INT16 const s16BPPSrcBig = pSrcBuf[(iWindowY * (uiSrcPitchBYTES / 2)) + iWindowX];
								UINT32 const uiRGBColorBig = GetRGBColor(s16BPPSrcBig);

								bRBig += SGPGetRValue(uiRGBColorBig);
								bGBig += SGPGetGValue(uiRGBColorBig);
								bBBig += SGPGetBValue(uiRGBColorBig);

								iCountBig++;
							}
						}
					}

					if (iCountBig > 0)
					{
						bAvRBig = bRBig / (UINT8)iCountBig;
						bAvGBig = bGBig / (UINT8)iCountBig;
						bAvBBig = bBBig / (UINT8)iCountBig;

						sDestBig = Get16BPPColor(FROMRGB(bAvRBig, bAvGBig, bAvBBig));
					}

					pDestBuf[(iYBig * (uiDestPitchBYTES / 2)) + iXBig] = sDestBig;

					SGPPaletteEntry* const dstBig = &values[iYBig * w + iXBig];
					dstBig->r = bAvRBig;
					dstBig->g = bAvGBig;
					dstBig->b = bAvBBig;

					dYBig += gdYStepBig;
				}

				dXBig += gdXStepBig;
			}
		}

		SGPPaletteEntry pPaletteBig[256];
		ST::string zFilenameBig;
		{ SGPVSurface::Lock lsrc(surf8);
			UINT8* const pDataPtrBig = lsrc.Buffer<UINT8>();
			// sMaxColors capped to 254 -- see the small pass's own comment above.
			QuantizeImage(pDataPtrBig, values, w, h, pPaletteBig, 254);
			ReserveTransparentPaletteIndex(pDataPtrBig, pPaletteBig, w * h);
			surf8->SetPalette(pPaletteBig);

			zFilenameBig = FileMan::replaceExtension(*currentFile, extension);
			WriteSTIFile(pDataPtrBig, pPaletteBig, w, h, zFilenameBig, CONVERT_ETRLE_COMPRESS, 0);
		}

		(void)text_y;
		written.push_back(ST::format("Writing {}x{} radar image {}", w, h, zFilenameBig));
	};
	// 580x290 is close to the source's size (a sample every 1.1 px): a 2x2 px
	// window keeps it sharp, the 4x4 one blurred it.
	if (g_write_size[SIZE_BIG])      write_resampled(RADAR_BIG_X_SIZE, RADAR_BIG_Y_SIZE, p24BitValuesBig.get(), giMiniMapBig, gi8BitMiniMapBig, "big.sti", 330, 1);

	INT16 text_y = 330;
	if (g_write_size[SIZE_OVERHEAD])
	{
		/* The 800x400 map: from the sector drawn 1:1 -- the part the overhead
		 * map shows, its restricted part for a restricted map (the bounds above
		 * are overhead map pixels, 5 world pixels each) --, every pixel the
		 * average of its block of the picture (4x4 for a whole sector). */
		bool  const restricted = gMapInformation.ubRestrictedScrollID != 0;
		INT32 const wx = WORLD_PICTURE_X + (restricted ? sLeft * 5 : 0);
		INT32 const wy = WORLD_PICTURE_Y + (restricted ? sTop  * 5 : 0);
		INT32 const ww = restricted ? std::clamp((sRight  - sLeft) * 5, 5, int(WORLD_PICTURE_W)) : WORLD_PICTURE_W;
		INT32 const wh = restricted ? std::clamp((sBottom - sTop)  * 5, 5, int(WORLD_PICTURE_H)) : WORLD_PICTURE_H;
		CaptureWorldPicture(giWorldPicture, wx, wy, ww, wh);

		INT32 const w = RADAR_OVERHEAD_X_SIZE;
		INT32 const h = RADAR_OVERHEAD_Y_SIZE;
		{ SGPVSurface::Lock lsrc(giWorldPicture);
			SGPVSurface::Lock ldst(giMiniMapOverhead);
			UINT16 const* const src       = lsrc.Buffer<UINT16>();
			UINT16*       const dst       = ldst.Buffer<UINT16>();
			UINT32        const src_pitch = lsrc.Pitch() / 2;
			UINT32        const dst_pitch = ldst.Pitch() / 2;
			for (INT32 oy = 0; oy != h; ++oy)
			{
				INT32 const y0 = oy * wh / h;
				INT32 const y1 = std::max(y0 + 1, (oy + 1) * wh / h);
				for (INT32 ox = 0; ox != w; ++ox)
				{
					INT32 const x0 = ox * ww / w;
					INT32 const x1 = std::max(x0 + 1, (ox + 1) * ww / w);
					UINT32 r = 0;
					UINT32 g = 0;
					UINT32 b = 0;
					for (INT32 py = y0; py != y1; ++py)
					{
						for (INT32 px = x0; px != x1; ++px)
						{
							UINT32 const rgb = GetRGBColor(src[py * src_pitch + px]);
							r += SGPGetRValue(rgb);
							g += SGPGetGValue(rgb);
							b += SGPGetBValue(rgb);
						}
					}
					UINT32 const n = (y1 - y0) * (x1 - x0);
					SGPPaletteEntry& value = p24BitValuesOverhead[oy * w + ox];
					value.r = (UINT8)(r / n);
					value.g = (UINT8)(g / n);
					value.b = (UINT8)(b / n);
					dst[oy * dst_pitch + ox] = Get16BPPColor(FROMRGB(value.r, value.g, value.b));
				}
			}
		}

		SGPPaletteEntry palette[256];
		ST::string      filename;
		{ SGPVSurface::Lock l(gi8BitMiniMapOverhead);
			UINT8* const data = l.Buffer<UINT8>();
			// sMaxColors capped to 254 -- see the small pass's own comment above.
			QuantizeImage(data, p24BitValuesOverhead.get(), w, h, palette, 254);
			ReserveTransparentPaletteIndex(data, palette, w * h);
			gi8BitMiniMapOverhead->SetPalette(palette);
			filename = FileMan::replaceExtension(*currentFile, "overhead.sti");
			WriteSTIFile(data, palette, w, h, filename, CONVERT_ETRLE_COMPRESS, 0);
		}
		written.push_back(ST::format("Writing {}x{} radar image {}", w, h, filename));

		// The viewports were drawn over the frame: show the result instead.
		FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
		BltVideoSurface(FRAME_BUFFER, giMiniMapOverhead, 10, 10, NULL);
		text_y = 10 + h + 10;
	}

	SetFontDestBuffer(FRAME_BUFFER);
	SetFontAttributes(TINYFONT1, FONT_MCOLOR_DKGRAY);
	for (ST::string const& line : written)
	{
		MPrint(10, text_y, line);
		text_y += 10;
	}
	if (g_write_size[SIZE_SMALL])
	{
		MPrint(10, text_y, ST::format("Writing radar image {}", zFilename2));
		text_y += 10;
	}
	MPrint(10, text_y, ST::format("Using tileset {}", gTilesets[giCurrentTilesetID].zName));

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
