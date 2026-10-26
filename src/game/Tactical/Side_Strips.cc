#include "Side_Strips.h"
#include "Button_System.h"
#include "ContentManager.h"
#include "Cursors.h"
#include "Directories.h"
#include "GameInstance.h"
#include "GameSettings.h"
#include "Interface_Items.h"
#include "Interface_Panels.h"
#include "JAScreens.h"
#include "MapScreen.h"
#include "Militia_Watch.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "Overhead_Map.h"
#include "RenderWorld.h"
#include "ScreenIDs.h"
#include "UILayout.h"
#include "VObject.h"
#include "VObject_Blitters.h"
#include "VSurface.h"
#include "Video.h"


namespace {

cache_key_t const guiSideStripLeft{  INTERFACEDIR "/SIDE_STRIP_LEFT_1366x768.png"  };
cache_key_t const guiSideStripRight{ INTERFACEDIR "/SIDE_STRIP_RIGHT_1366x768.png" };
// For now the hide / show button is the map inventory's big Done button
// (mapinv_done_buttons.sti sub-images 2 = ready, 3 = pressed, 50x50).
cache_key_t const guiSideStripToggle{ INTERFACEDIR "/mapinv_done_buttons.sti" };

// The strips' graphic: 56x480 at the screen's top, nine 50x50 slots
// 3 px apart (and from the strip's edges), the toggle in the bottom one.
constexpr INT16 SIDE_STRIP_W      = SIDE_STRIP_WIDTH;
constexpr INT16 SIDE_STRIP_H      = 480;
constexpr INT16 SIDE_STRIP_Y      = 0;
constexpr INT16 SIDE_STRIP_SLOT   = 50;
constexpr INT16 SIDE_STRIP_GAP    = 3;
constexpr INT16 SIDE_STRIP_SLOTS  = 9;
constexpr INT16 SIDE_STRIP_SLOT_Y(INT16 const i) { return SIDE_STRIP_Y + SIDE_STRIP_GAP + i * (SIDE_STRIP_SLOT + SIDE_STRIP_GAP); }
constexpr INT16 SIDE_STRIP_TOGGLE_Y = SIDE_STRIP_SLOT_Y(SIDE_STRIP_SLOTS - 1); // 427
// a hidden strip: only its bottom slot with the frame around it
constexpr INT16 SIDE_STRIP_HIDDEN_TOP = SIDE_STRIP_TOGGLE_Y - SIDE_STRIP_GAP; // 424

struct SideStrip
{
	cache_key_t  gfx;
	GUIButtonRef toggle;
	MOUSE_REGION region; // the whole strip, so that clicks on it don't reach the world
};

SideStrip g_side_strips[2] = { { guiSideStripLeft, {}, {} }, { guiSideStripRight, {}, {} } };
// the buttons other code placed in the slots
GUIButtonRef g_side_strip_buttons[2][SIDE_STRIP_BUTTON_SLOTS];
bool      g_side_strips_created = false;
bool      g_side_strips_off     = true; // off, e.g. under the overhead map, as of the last frame

// The strips' own buttons, independent of the tactical panels: Statistics
// (the SM panel bookmark row's picture, inventory_bottom_panel_bookmarks.sti
// 22 / 23) in the right strip's top slot.
BUTTON_PICS* g_side_strip_stats_image = NULL;
GUIButtonRef g_side_strip_stats_button;

}


static INT16 SideStripX(UINT const i)
{
	return i == 0 ? 0 : SCREEN_WIDTH - SIDE_STRIP_W; // 1310
}


static BOOLEAN& SideStripHidden(UINT const i)
{
	return i == 0 ? gGameSettings.fSideStripLeftHidden : gGameSettings.fSideStripRightHidden;
}


// Created, but nothing to show here: not the tactical screen, the overhead
// map, the 1366x768 weapon box over the screen's whole width, or a watched
// militia battle (nobody of the player's team is there).
static bool SideStripsSuppressed()
{
	return fInMapMode || guiCurrentScreen != GAME_SCREEN || InOverheadMap() || InTacBigWeaponBox() ||
		gfWatchingMilitiaBattle;
}


static bool SideStripsShown()
{
	return g_side_strips_created && !SideStripsSuppressed();
}


bool TacticalSideStripsAvailable()
{
	return g_ui.isBigTeamPanel() &&
		GCM->doesGameResExists(guiSideStripLeft)  &&
		GCM->doesGameResExists(guiSideStripRight) &&
		GCM->doesGameResExists(guiSideStripToggle);
}


void TacticalSideStripSlotPos(SideStripKind const strip, UINT const slot, INT16& x, INT16& y)
{
	x = SideStripX(strip) + SIDE_STRIP_GAP;
	y = SIDE_STRIP_SLOT_Y(slot);
}


void SetTacticalSideStripButton(SideStripKind const strip, UINT const slot, GUIButtonRef const button)
{
	if (slot >= SIDE_STRIP_BUTTON_SLOTS) return;
	g_side_strip_buttons[strip][slot] = button;
}


// The slots' buttons shown with their strip, hidden otherwise -- also after
// their owner showed them (e.g. the SM panel's bookmark row).
static void ApplySideStripButtons()
{
	for (UINT i = 0; i != lengthof(g_side_strips); ++i)
	{
		bool const shown = !g_side_strips_off && !SideStripHidden(i);
		for (GUIButtonRef const b : g_side_strip_buttons[i])
		{
			if (!b) continue;
			bool const visible = (b->Area.uiFlags & MSYS_REGION_ENABLED) != 0;
			if (visible != shown)
			{
				if (shown) ShowButton(b); else HideButton(b);
			}
			if (shown) MarkAButtonDirty(b);
		}
	}
}


// The buttons and regions as the strips' state says.
static void ApplySideStripsState()
{
	for (UINT i = 0; i != lengthof(g_side_strips); ++i)
	{
		SideStrip& s = g_side_strips[i];
		if (g_side_strips_off)
		{
			HideButton(s.toggle);
			s.region.Disable();
		}
		else
		{
			ShowButton(s.toggle);
			if (SideStripHidden(i)) s.region.Disable(); else s.region.Enable();
		}
	}
	ApplySideStripButtons();
}


static void SideStripToggleCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	BOOLEAN& hidden = SideStripHidden(btn->GetUserData());
	hidden = !hidden;
	SaveGameSettings();
	ApplySideStripsState();
	// the world under the strip again
	if (hidden) SetRenderFlags(RENDER_FLAG_FULL);
}


static void SideStripStatsCallback(GUI_BUTTON*, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ToggleTacticalStatsPopup();
}


// A strip button with this picture in the middle of a slot.
static GUIButtonRef CreateSideStripButton(SideStripKind const strip, UINT const slot, BUTTON_PICS* const image, GUI_CALLBACK click, ST::string const& help)
{
	INT16 x;
	INT16 y;
	TacticalSideStripSlotPos(strip, slot, x, y);
	ButtonDimensions const* const d = GetDimensionsOfButtonPic(image);
	x += (SIDE_STRIP_SLOT - (INT16)d->w) / 2;
	y += (SIDE_STRIP_SLOT - (INT16)d->h) / 2;
	GUIButtonRef const b = QuickCreateButton(image, x, y, MSYS_PRIORITY_HIGH, std::move(click));
	b->SetFastHelpText(help);
	SetTacticalSideStripButton(strip, slot, b);
	return b;
}


void CreateTacticalSideStrips()
{
	if (g_side_strips_created || !TacticalSideStripsAvailable()) return;

	for (UINT i = 0; i != lengthof(g_side_strips); ++i)
	{
		SideStrip&  s = g_side_strips[i];
		INT16 const x = SideStripX(i);
		MSYS_DefineRegion(&s.region, x, SIDE_STRIP_Y, x + SIDE_STRIP_W, SIDE_STRIP_Y + SIDE_STRIP_H,
			MSYS_PRIORITY_HIGH - 1, CURSOR_NORMAL, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);
		s.toggle = QuickCreateButtonImg(guiSideStripToggle, 2, 3, x + SIDE_STRIP_GAP, SIDE_STRIP_TOGGLE_Y,
			MSYS_PRIORITY_HIGH, SideStripToggleCallback);
		s.toggle->SetUserData(i);
	}

	g_side_strip_stats_image  = LoadButtonImage(INTERFACEDIR "/inventory_bottom_panel_bookmarks.sti", 22, 23);
	g_side_strip_stats_button = CreateSideStripButton(SIDE_STRIP_RIGHT, 0, g_side_strip_stats_image, SideStripStatsCallback, "Statistics");

	g_side_strips_created = true;
	g_side_strips_off     = SideStripsSuppressed();
	ApplySideStripsState();
}


void RemoveTacticalSideStrips()
{
	if (!g_side_strips_created) return;

	SetTacticalSideStripButton(SIDE_STRIP_RIGHT, 0, GUIButtonRef::NoButton());
	RemoveButton(g_side_strip_stats_button);
	UnloadButtonImage(g_side_strip_stats_image);
	g_side_strip_stats_image = NULL;

	for (SideStrip& s : g_side_strips)
	{
		RemoveButton(s.toggle);
		MSYS_RemoveRegion(&s.region);
	}
	g_side_strips_created = false;
	g_side_strips_off     = true;
}


void RenderTacticalSideStrips()
{
	if (!g_side_strips_created) return;

	bool const off = SideStripsSuppressed();
	if (off != g_side_strips_off)
	{
		g_side_strips_off = off;
		ApplySideStripsState();
		// e.g. the weapon box: the world where the strips were
		if (off) SetRenderFlags(RENDER_FLAG_FULL);
	}
	if (off)
	{
		ApplySideStripButtons();
		return;
	}

	// Drawn every frame over the world (its slots are see-through), like the
	// world's own overlays.
	SGPRect const old_clip = ClippingRect;
	for (UINT i = 0; i != lengthof(g_side_strips); ++i)
	{
		SideStrip&  s   = g_side_strips[i];
		INT16 const x   = SideStripX(i);
		INT16 const top = SideStripHidden(i) ? SIDE_STRIP_HIDDEN_TOP : SIDE_STRIP_Y;
		ClippingRect.iLeft   = x;
		ClippingRect.iTop    = top;
		ClippingRect.iRight  = x + SIDE_STRIP_W;
		ClippingRect.iBottom = SIDE_STRIP_Y + SIDE_STRIP_H;
		BltVideoObject(FRAME_BUFFER, GetVObject(s.gfx), 0, x, SIDE_STRIP_Y);
		InvalidateRegion(x, top, x + SIDE_STRIP_W, SIDE_STRIP_Y + SIDE_STRIP_H);
		MarkAButtonDirty(s.toggle);
	}
	ClippingRect = old_clip;
	ApplySideStripButtons();
}


INT16 TacticalSideStripLeftWidth()
{
	return SideStripsShown() ? SIDE_STRIP_W : 0;
}


// How far the strip at this y covers the screen from its edge.
static INT32 SideStripCover(UINT const i, INT32 const y)
{
	if (!SideStripsShown()) return 0;
	INT16 const top = SideStripHidden(i) ? SIDE_STRIP_HIDDEN_TOP : SIDE_STRIP_Y;
	return top <= y && y < SIDE_STRIP_Y + SIDE_STRIP_H ? SIDE_STRIP_W : 0;
}


bool InTacticalLeftEdgeZone(INT32 const x, INT32 const y)
{
	INT32 const edge = SideStripCover(0, y);
	return edge <= x && x < edge + NO_PX_SHOW_EXIT_CURS;
}


bool InTacticalRightEdgeZone(INT32 const x, INT32 const y)
{
	INT32 const edge = SCREEN_WIDTH - SideStripCover(1, y);
	return edge - NO_PX_SHOW_EXIT_CURS <= x && x < edge;
}
