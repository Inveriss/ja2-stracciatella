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
