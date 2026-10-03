#pragma once

#include "Types.h"

// Tactical viewport zoom (mouse wheel over the viewport): 100%, 125%, 150%
// and 200%. The world is still rendered into the frame 1:1; a part of the
// frame (the "crop") is shown scaled onto the viewport when the screen is
// refreshed -- see VideoSetZoomProvider() in Video.h. Mouse positions are
// mapped back, so the game keeps working in frame coordinates.

// The video manager asks this module for the zoom on every screen refresh.
void InitViewportZoom();

// Back to 100%, with the crop centered (new sector, loaded game).
void ViewportZoomReset();

// The zoom level, in percent: 100 is no zoom.
UINT16 ViewportZoomPercent();

// The viewport is zoomed right now: a level above 100% and nothing that
// needs the whole viewport 1:1 is open (e.g. a dialogue, the overhead map).
bool ViewportZoomIsActive();

// One zoom level up (zoomIn) or down, keeping the frame position under the
// mouse under the mouse. Steps closer to each other than a short delay are
// ignored, so a touchpad does not run through all the levels at once.
void ViewportZoomHandleWheel(bool zoomIn);

// Centers the crop on the tactical map center, i.e. on the render center --
// where the game puts what it locates.
void ViewportZoomCenterCrop();

// Moves the crop by (dx, dy) frame pixels, as far as it goes.
void ViewportZoomPan(INT32 dx, INT32 dy);

// The crop reaches the frame's edge in the given SCROLL_* direction (always
// true while not zoomed): further scrolling that way has to move the world.
bool ViewportZoomCropAtEdge(UINT32 scrollFlag);

// The visible part of the world, in the coordinates of gsTopLeftWorldX/Y and
// gsBottomRightWorldX/Y (which it equals while not zoomed).
void ViewportZoomGetVisibleWorldRect(INT16& left, INT16& top, INT16& right, INT16& bottom);


// Zoom geometry, independent of the game state (unit tested).
struct ViewportZoomGeometry
{
	SGPBox zone; // the part of the screen showing the zoomed world
	SGPBox crop; // the part of the frame shown there
};

// percent: above 100. The zone is the full screen width between zoneTop and
// zoneBottom. The crop is centered on (centerX, centerY), moved inside the
// zone if that does not fit.
ViewportZoomGeometry ComputeViewportZoomGeometry(UINT16 percent, UINT16 screenWidth,
	UINT16 zoneTop, UINT16 zoneBottom, double centerX, double centerY);
