#include "Viewport_Zoom.h"

#include "Dialogue_Control.h"
#include "Game_Clock.h"
#include "Input.h"
#include "Interface.h"
#include "Interface_Dialogue.h"
#include "Interface_Items.h"
#include "JAScreens.h"
#include "Map_Information.h"
#include "Message.h"
#include "Overhead_Map.h"
#include "RenderWorld.h"
#include "ScreenIDs.h"
#include "Strategic_Exit_GUI.h"
#include "Sys_Globals.h"
#include "Tactical_Placement_GUI.h"
#include "Timer.h"
#include "UILayout.h"
#include "Video.h"

#include <algorithm>
#include <cmath>
#include <iterator>


// Index 0 is no zoom.
static UINT16 const g_zoom_levels[] = { 100, 125, 150, 200 };

// Wheel steps closer to each other than this are ignored (ms).
#define ZOOM_STEP_DELAY 90

static UINT8  g_zoom_level = 0;
// Wanted crop center, in frame coordinates; negative: the tactical map
// center. The crop is moved inside the zone when it does not fit there
// (e.g. while the inventory panel is open), without changing this.
static double g_zoom_center_x = -1;
static double g_zoom_center_y = -1;
static UINT32 g_zoom_last_step_time = 0;


ViewportZoomGeometry ComputeViewportZoomGeometry(UINT16 const percent, UINT16 const screenWidth,
	UINT16 const zoneTop, UINT16 const zoneBottom, double const centerX, double const centerY)
{
	ViewportZoomGeometry g;
	UINT16 const zoneHeight = zoneBottom > zoneTop ? zoneBottom - zoneTop : 1;
	g.zone = { 0, zoneTop, screenWidth, zoneHeight };

	UINT16 const w = std::max(1, (int)std::lround(screenWidth * 100.0 / percent));
	UINT16 const h = std::max(1, (int)std::lround(zoneHeight  * 100.0 / percent));
	int const x = (int)std::lround(centerX - w / 2.0);
	int const y = (int)std::lround(centerY - h / 2.0);
	g.crop = {
		(UINT16)std::clamp(x, 0, screenWidth - w),
		(UINT16)std::clamp(y, (int)zoneTop, zoneTop + zoneHeight - h),
		w, h
	};
	return g;
}


void ViewportZoomReset()
{
	g_zoom_level    = 0;
	g_zoom_center_x = -1;
	g_zoom_center_y = -1;
}


UINT16 ViewportZoomPercent()
{
	return g_zoom_levels[g_zoom_level];
}


// Things drawn 1:1 over the viewport that the zoom would cut or move away
// from what they belong to: back to 100% while they are open.
static bool ZoomSuppressed()
{
	// the tactical placement's 1:1 view zooms like the game
	if (!gfEditMode && gfTacticalPlacementGUIActive && TacticalPlacementFullView()) return false;

	return
		gfEditMode                   ||
		InOverheadMap()              ||
		gfTacticalPlacementGUIActive ||
		gfInTalkPanel                ||
		gfInSectorExitMenu           ||
		InItemDescriptionBox()       ||
		InItemStackPopup()           ||
		InKeyRingPopup();
}


// The tactical screen can be zoomed now (at whatever level).
static bool ZoomPossible()
{
	return guiCurrentScreen == GAME_SCREEN && gfWorldLoaded && !ZoomSuppressed();
}


bool ViewportZoomIsActive()
{
	return g_zoom_level != 0 && ZoomPossible();
}


// The part of the screen showing the world: below the top message bar (when
// it is shown) and above the bottom panel.
static void GetZoomZone(UINT16& top, UINT16& bottom)
{
	top    = gsVIEWPORT_WINDOW_START_Y;
	bottom = gsVIEWPORT_WINDOW_END_Y;
	if (gsCurInterfacePanel == SM_PANEL) bottom = std::min(bottom, INV_INTERFACE_START_Y);
	if (gfTacticalPlacementGUIActive && TacticalPlacementFullView())
	{
		// beside the placement panel
		top    = TacticalPlacementViewTop();
		bottom = TacticalPlacementViewBottom();
	}
}


static ViewportZoomGeometry ZoomGeometryAt(UINT8 const level)
{
	if (g_zoom_center_x < 0 || g_zoom_center_y < 0)
	{
		g_zoom_center_x = g_ui.m_tacticalMapCenterX;
		g_zoom_center_y = g_ui.m_tacticalMapCenterY;
	}
	UINT16 top;
	UINT16 bottom;
	GetZoomZone(top, bottom);
	return ComputeViewportZoomGeometry(g_zoom_levels[level], SCREEN_WIDTH, top, bottom, g_zoom_center_x, g_zoom_center_y);
}


static ViewportZoomGeometry CurrentZoomGeometry()
{
	return ZoomGeometryAt(g_zoom_level);
}


static void SetZoomCenterFromCrop(SGPBox const& crop)
{
	g_zoom_center_x = crop.x + crop.w / 2.0;
	g_zoom_center_y = crop.y + crop.h / 2.0;
}


// Clips `box` to `zone`, false when nothing is left.
static bool ClipZoomBox(SGPBox& box, SGPBox const& zone)
{
	int const left   = std::max<int>(box.x, zone.x);
	int const top    = std::max<int>(box.y, zone.y);
	int const right  = std::min<int>(box.x + box.w, zone.x + zone.w);
	int const bottom = std::min<int>(box.y + box.h, zone.y + zone.h);
	if (right <= left || bottom <= top) return false;
	box = { (UINT16)left, (UINT16)top, (UINT16)(right - left), (UINT16)(bottom - top) };
	return true;
}


static void ProvideViewportZoom(VideoZoom& zoom)
{
	if (!ViewportZoomIsActive()) return;

	ViewportZoomGeometry const g = CurrentZoomGeometry();
	zoom.active = true;
	zoom.src    = g.crop;
	zoom.dst    = g.zone;

	// Fixed-position HUD over the world stays 1:1, where it is: the message
	// log, the subtitle box, the "game paused" box.
	SGPBox boxes[VIDEO_ZOOM_MAX_KEEP];
	UINT8 n = GetTacticalMessageBoxes(boxes, VIDEO_ZOOM_MAX_KEEP - 2);
	if (GetTacticalTextBox(boxes[n])) ++n;
	if (GetPausedGameBox(boxes[n]))   ++n;
	// the tactical placement's panel beside the world (west / east entry)
	if (n < VIDEO_ZOOM_MAX_KEEP && TacticalPlacementPanelBox(boxes[n])) ++n;
	if (n < VIDEO_ZOOM_MAX_KEEP && TacticalPlacementMapBox(boxes[n]))   ++n; // its minimap

	zoom.numKeep = 0;
	for (UINT8 i = 0; i != n; ++i)
	{
		SGPBox box = boxes[i];
		if (ClipZoomBox(box, g.zone)) zoom.keep[zoom.numKeep++] = box;
	}
}


void InitViewportZoom()
{
	VideoSetZoomProvider(ProvideViewportZoom);
}


void ViewportZoomHandleWheel(bool const zoomIn)
{
	if (!ZoomPossible()) return;

	UINT8 const last  = (UINT8)(std::size(g_zoom_levels) - 1);
	UINT8 const level = zoomIn ?
		(g_zoom_level < last ? g_zoom_level + 1 : last) :
		(g_zoom_level > 0    ? g_zoom_level - 1 : 0);
	if (level == g_zoom_level) return;

	UINT32 const now = GetClock();
	if (now - g_zoom_last_step_time < ZOOM_STEP_DELAY) return;
	g_zoom_last_step_time = now;

	// The mouse, inside the zone.
	UINT16 top;
	UINT16 bottom;
	GetZoomZone(top, bottom);
	SGPPoint const mouse = GetPhysicalMousePos();
	double const mx = std::clamp<int>(mouse.iX, 0, SCREEN_WIDTH - 1);
	double const my = std::clamp<int>(mouse.iY, top, std::max<int>(top, bottom - 1));

	// The frame position under the mouse now...
	double fx = mx;
	double fy = my;
	if (g_zoom_level != 0)
	{
		ViewportZoomGeometry const g = CurrentZoomGeometry();
		fx = g.crop.x + (mx - g.zone.x) * g.crop.w / g.zone.w;
		fy = g.crop.y + (my - g.zone.y) * g.crop.h / g.zone.h;
	}

	g_zoom_level = level;
	if (level == 0) return;

	// ... stays under it.
	ViewportZoomGeometry const g = CurrentZoomGeometry();
	double const left = fx - (mx - g.zone.x) * g.crop.w / g.zone.w;
	double const up   = fy - (my - g.zone.y) * g.crop.h / g.zone.h;
	g_zoom_center_x = left + g.crop.w / 2.0;
	g_zoom_center_y = up   + g.crop.h / 2.0;
	SetZoomCenterFromCrop(CurrentZoomGeometry().crop);
}


void ViewportZoomCenterCrop()
{
	g_zoom_center_x = g_ui.m_tacticalMapCenterX;
	g_zoom_center_y = g_ui.m_tacticalMapCenterY;
}


void ViewportZoomPan(INT32 const dx, INT32 const dy)
{
	if (!ViewportZoomIsActive() || (dx == 0 && dy == 0)) return;

	// From where the crop is shown, not from the wanted center.
	SGPBox const crop = CurrentZoomGeometry().crop;
	g_zoom_center_x = crop.x + crop.w / 2.0 + dx;
	g_zoom_center_y = crop.y + crop.h / 2.0 + dy;
	SetZoomCenterFromCrop(CurrentZoomGeometry().crop);
}


bool ViewportZoomCropAtEdge(UINT32 const scrollFlag)
{
	if (!ViewportZoomIsActive()) return true;

	ViewportZoomGeometry const g = CurrentZoomGeometry();
	switch (scrollFlag)
	{
		case SCROLL_LEFT:  return g.crop.x == 0;
		case SCROLL_RIGHT: return g.crop.x + g.crop.w >= g.zone.x + g.zone.w;
		case SCROLL_UP:    return g.crop.y <= g.zone.y;
		case SCROLL_DOWN:  return g.crop.y + g.crop.h >= g.zone.y + g.zone.h;
		default:           return true;
	}
}


void ViewportZoomGetVisibleWorldRect(INT16& left, INT16& top, INT16& right, INT16& bottom)
{
	left   = gsTopLeftWorldX;
	top    = gsTopLeftWorldY;
	right  = gsBottomRightWorldX;
	bottom = gsBottomRightWorldY;
	if (!ViewportZoomIsActive()) return;

	// gsTopLeftWorldX/Y is shown at the frame's top left corner.
	SGPBox const crop = CurrentZoomGeometry().crop;
	left   = gsTopLeftWorldX + crop.x;
	top    = gsTopLeftWorldY + crop.y;
	right  = left + crop.w;
	bottom = top  + crop.h;
}


#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"

TEST(Viewport_Zoom, GeometryCropSizes)
{
	// 1280x720, team panel: zone 1280x600
	struct { UINT16 percent, w, h; } const levels720[] = {
		{ 125, 1024, 480 }, { 150, 853, 400 }, { 200, 640, 300 }
	};
	for (auto const& l : levels720)
	{
		ViewportZoomGeometry const g = ComputeViewportZoomGeometry(l.percent, 1280, 0, 600, 640, 300);
		EXPECT_EQ(g.zone.x, 0);
		EXPECT_EQ(g.zone.y, 0);
		EXPECT_EQ(g.zone.w, 1280);
		EXPECT_EQ(g.zone.h, 600);
		EXPECT_EQ(g.crop.w, l.w);
		EXPECT_EQ(g.crop.h, l.h);
	}

	// 1366x768, inventory panel: zone 1366x571
	struct { UINT16 percent, w, h; } const levels768[] = {
		{ 125, 1093, 457 }, { 150, 911, 381 }, { 200, 683, 286 }
	};
	for (auto const& l : levels768)
	{
		ViewportZoomGeometry const g = ComputeViewportZoomGeometry(l.percent, 1366, 0, 571, 683, 285);
		EXPECT_EQ(g.crop.w, l.w);
		EXPECT_EQ(g.crop.h, l.h);
	}
}

TEST(Viewport_Zoom, GeometryCentersAndClampsCrop)
{
	// Centered
	ViewportZoomGeometry g = ComputeViewportZoomGeometry(200, 1280, 0, 600, 640, 300);
	EXPECT_EQ(g.crop.x, 320);
	EXPECT_EQ(g.crop.y, 150);

	// Moved inside the zone at every edge
	g = ComputeViewportZoomGeometry(200, 1280, 0, 600, 0, 0);
	EXPECT_EQ(g.crop.x, 0);
	EXPECT_EQ(g.crop.y, 0);
	g = ComputeViewportZoomGeometry(200, 1280, 0, 600, 5000, 5000);
	EXPECT_EQ(g.crop.x, 1280 - 640);
	EXPECT_EQ(g.crop.y, 600 - 300);

	// Below the top message bar
	g = ComputeViewportZoomGeometry(200, 1280, 20, 600, 640, 0);
	EXPECT_EQ(g.zone.y, 20);
	EXPECT_EQ(g.zone.h, 580);
	EXPECT_EQ(g.crop.y, 20);
}

#endif
