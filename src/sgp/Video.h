#ifndef VIDEO_H
#define VIDEO_H

#include "Types.h"
#include "RustInterface.h"
#include "SDL.h"


#define VIDEO_DEFAULT_TO_NO_CURSOR 0xFFFE // VIDEO_DEFAULT_TO_NO_CURSOR is equal to VIDEO_NO_CURSOR unless always_show_cursor_in_tactical is true
#define VIDEO_NO_CURSOR 0xFFFF
#define GAME_WINDOW g_game_window

extern INT16 gsMouseSizeYModifier;
extern SDL_Window* g_game_window;

using VideoScaleQuality = ScalingQuality;

void         VideoSetFullScreen(BOOLEAN enable);
// stretchToFill: scale the game image to the whole window/screen, X and Y
// independently, ignoring its aspect ratio (no black bars).
void         InitializeVideoManager(VideoScaleQuality quality, bool stretchToFill, int32_t targetFPS);
bool         VideoIsStretchedToFill();
// While stretching, `provider` is asked on every screen refresh which part of
// the frame should fill the screen ({0,0,0,0}: the whole frame) -- lets a
// screen with a smaller fixed canvas (strategic map, laptop) zoom that canvas
// instead of the whole frame.
void         VideoSetStretchRegionProvider(SGPBox (*provider)());
// The part of the frame currently filling the screen while stretching (the
// whole frame when not zoomed).
SGPBox       VideoGetStretchRegion();

// Viewport zoom (tactical screen, mouse wheel): the frame is still rendered
// 1:1, then at refresh time its `src` part is shown scaled onto the screen
// box `dst`. The rest of the frame stays 1:1, and so do the `keep` boxes
// inside `dst` (fixed-position HUD drawn over the world, e.g. the message
// log). Mouse positions inside `dst` (outside `keep`) are mapped back into
// `src`, so the game keeps working in frame coordinates.
#define VIDEO_ZOOM_MAX_KEEP 8
struct VideoZoom
{
	bool   active;
	SGPBox src;
	SGPBox dst;
	UINT8  numKeep;
	SGPBox keep[VIDEO_ZOOM_MAX_KEEP];
};
// `provider` is asked on every screen refresh for the zoom to show.
void             VideoSetZoomProvider(void (*provider)(VideoZoom&));
VideoZoom const& VideoGetZoom();
// Screen position -> frame position through the current zoom, and back
// (identity while not zoomed and outside the zoomed part).
SGPPoint         VideoZoomScreenToFrame(int x, int y);
SGPPoint         VideoZoomFrameToScreen(int x, int y);
void         ShutdownVideoManager(void);
void         InvalidateRegion(INT32 iLeft, INT32 iTop, INT32 iRight, INT32 iBottom);
void         InvalidateScreen(void);

void VideoSetBrightness(float brightness);

/* Toggle between fullscreen and window mode after initialising the video
 * manager */
void VideoToggleFullScreen(void);

void SetMouseCursorProperties(INT16 sOffsetX, INT16 sOffsetY, UINT16 usCursorHeight, UINT16 usCursorWidth);

void InvalidateRegionEx(INT32 iLeft, INT32 iTop, INT32 iRight, INT32 iBottom);

void RefreshScreen(void);

// Creates a list to contain video Surfaces
void InitializeVideoSurfaceManager(void);

// Deletes any video Surface placed into list
void ShutdownVideoSurfaceManager(void);

#endif
