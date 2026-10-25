#include "Militia_Deployment_Panel.h"
#include "Button_System.h"
#include "Directories.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "GameScreen.h"
#include "HImage.h"
#include "JAScreens.h"
#include "MapScreen.h"
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Bottom.h"
#include "Militia_Recruit_Panel.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "ScreenIDs.h"
#include "Text.h"
#include "Town_Militia.h"
#include "UILayout.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"

#include <string_theory/format>
#include <string_theory/string>

#include <algorithm>
#include <vector>


namespace {

cache_key_t const guiDeploymentPanel{ INTERFACEDIR "/MILITIA_DEPLOYMENT_PANEL_1366x768.png" };
// The close button: as the recruitment panel's (mapinv_done_buttons.sti 2/3, 50x50)
char const* const DEPLOY_CLOSE_BUTTON_GFX = INTERFACEDIR "/mapinv_done_buttons.sti";
constexpr INT16   DEPLOY_CLOSE_BUTTON_SIZE   = 50;
constexpr INT16   DEPLOY_CLOSE_BUTTON_MARGIN = 10;
// A waiting order's Done button: SECTOR_INVENTORY_DONE_BUTTONS.STI 2/3, 41x16
char const* const DEPLOY_DONE_BUTTON_GFX = INTERFACEDIR "/SECTOR_INVENTORY_DONE_BUTTONS.STI";
constexpr INT16   DEPLOY_DONE_BUTTON_W   = 41;
constexpr INT16   DEPLOY_DONE_BUTTON_H   = 16;

// The recruitment panel's font and colour. Positions inside the panel graphic
// (761x647); every box has a 3 px frame.
#define DEPLOY_FONT   FONT12ARIAL
#define DEPLOY_COLOUR FONT_BEIGE

// title bar (180,16 402x31), the two headers (180,68 / 386,68 196x56): the
// header's name at the top, the column labels under it
constexpr INT16 DEPLOY_TITLE_X  = 180, DEPLOY_TITLE_Y  = 16, DEPLOY_TITLE_W  = 402, DEPLOY_TITLE_H  = 31;
constexpr INT16 DEPLOY_HEADER_Y = 78;  // the header's name
constexpr INT16 DEPLOY_LABELS_Y = 104; // the column labels
constexpr INT16 DEPLOY_HEADER_W = 196;
constexpr INT16 DEPLOY_INCOMING_X = 180; // the header and the rank rows' Incoming boxes
constexpr INT16 DEPLOY_WAITING_X  = 386; // ... and Waiting boxes
constexpr INT16 DEPLOY_TOTAL_X    = 592; // the rank rows' TOTAL boxes, 127 wide
constexpr INT16 DEPLOY_TOTAL_W    = 127;
// the rank rows' boxes (66 high), 76 px apart; the portrait and the rank's
// name in one box (44,y 126x66), the name right of the portrait (107-167),
// a name of two words in two lines
constexpr INT16 DEPLOY_ROW_FIRST_Y = 134;
constexpr INT16 DEPLOY_ROW_PITCH   = 76;
constexpr INT16 DEPLOY_ROW_H       = 66;
constexpr INT16 DEPLOY_RANK_X      = 107;
constexpr INT16 DEPLOY_RANK_W      = 60;
constexpr INT16 DEPLOY_RANK_PITCH  = 15;
// three lines in a box, 19 px apart, the first 4 px under the box's inside
constexpr INT16 DEPLOY_LINES       = 3;
constexpr INT16 DEPLOY_LINE_FIRST  = 3 + 4;
constexpr INT16 DEPLOY_LINE_PITCH  = 19;
constexpr INT16 DEPLOY_LINE_H      = 16;
// the columns of a line, from the box's left edge: count (centred), town,
// hours (right-aligned) or the Done button
constexpr INT16 DEPLOY_COUNT_DX = 9,   DEPLOY_COUNT_W = 28;
constexpr INT16 DEPLOY_TOWN_DX  = 38,  DEPLOY_TOWN_W  = 100;
constexpr INT16 DEPLOY_ETA_DX   = 140, DEPLOY_ETA_W   = 48;
constexpr INT16 DEPLOY_DONE_DX  = 196 - 3 - 4 - DEPLOY_DONE_BUTTON_W; // 4 px from the inside's right
// the TOTAL box: header, then Incoming / Waiting (label left, value right)
constexpr INT16 DEPLOY_TOTAL_LABEL_DX = 8, DEPLOY_TOTAL_VALUE_W = 127 - 16;

bool         g_open    = false;
bool         g_pending = false;   // asked for on the tactical screen, opens on the map screen
MOUSE_REGION g_blanket;           // the whole panel, so that clicks don't reach the map under it
GUIButtonRef g_close_button;
GUIButtonRef g_done_buttons[MAX_MILITIA_LEVELS][DEPLOY_LINES];

// The orders listed in a rank row's box: the ones with militia of the rank,
// on their way (by arrival) or waiting (by order)
std::vector<MilitiaOrder const*> RankOrders(UINT8 const rank, bool const waiting)
{
	std::vector<MilitiaOrder const*> list;
	for (MilitiaOrder const& o : GetMilitiaOrders())
	{
		if (o.counts[rank] != 0 && o.waiting == waiting) list.push_back(&o);
	}
	if (!waiting)
	{
		std::stable_sort(list.begin(), list.end(),
			[](MilitiaOrder const* a, MilitiaOrder const* b) { return a->arrival_min < b->arrival_min; });
	}
	return list;
}

}


// The panel at the map screen's right edge, from its top: over the map (as
// the recruitment panel)
static void PanelRect(INT16& x, INT16& y, INT16& w, INT16& h)
{
	ETRLEObject const& e = GetVObject(guiDeploymentPanel)->SubregionProperties(0);
	w = e.usWidth;
	h = e.usHeight;
	x = MAP_SCREEN_X + MAP_SCREEN_WIDTH - w;
	y = MAP_SCREEN_Y;
}


static void CloseButtonCallback(GUI_BUTTON*, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) CloseMilitiaDeploymentPanel();
}


// Done: the waiting order tries again to enter its town (as at its arrival,
// with the same questions)
static void DoneButtonCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	RetryMilitiaOrder(btn->GetUserData());
}


static void CreatePanel()
{
	INT16 x, y, w, h;
	PanelRect(x, y, w, h);

	MSYS_DefineRegion(&g_blanket, x, y, x + w, y + h, MSYS_PRIORITY_HIGHEST - 1, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

	INT16 const bx = x + w - DEPLOY_CLOSE_BUTTON_MARGIN - DEPLOY_CLOSE_BUTTON_SIZE;
	INT16 const by = y + h - DEPLOY_CLOSE_BUTTON_MARGIN - DEPLOY_CLOSE_BUTTON_SIZE;
	g_close_button = QuickCreateButtonImg(DEPLOY_CLOSE_BUTTON_GFX, 2, 3, bx, by, MSYS_PRIORITY_HIGHEST, CloseButtonCallback);
	g_close_button->SetFastHelpText("Close");

	// a Done button per waiting line, shown when the line has an order
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		for (INT16 line = 0; line != DEPLOY_LINES; ++line)
		{
			INT16 const ly = y + DEPLOY_ROW_FIRST_Y + rank * DEPLOY_ROW_PITCH + DEPLOY_LINE_FIRST + line * DEPLOY_LINE_PITCH;
			GUIButtonRef const b = QuickCreateButtonImg(DEPLOY_DONE_BUTTON_GFX, 2, 3,
				x + DEPLOY_WAITING_X + DEPLOY_DONE_DX, ly + (DEPLOY_LINE_H - DEPLOY_DONE_BUTTON_H) / 2,
				MSYS_PRIORITY_HIGHEST, DoneButtonCallback);
			b->Hide();
			g_done_buttons[rank][line] = b;
		}
	}

	g_open = true;
}


void OpenMilitiaDeploymentPanel()
{
	if (g_open) return;
	// one panel at a time
	CloseMilitiaRecruitPanel();
	if (guiCurrentScreen != MAP_SCREEN)
	{
		// the panel belongs to the map screen: go there, it opens on arrival
		g_pending           = true;
		gfEnteringMapScreen = TRUE;
		return;
	}
	g_pending = false;
	CreatePanel();
}


void CloseMilitiaDeploymentPanel()
{
	g_pending = false;
	if (!g_open) return;

	RemoveButton(g_close_button);
	for (auto& rank_buttons : g_done_buttons)
	{
		for (GUIButtonRef& b : rank_buttons) RemoveButton(b);
	}
	MSYS_RemoveRegion(&g_blanket);
	g_open = false;

	// what was under the panel
	fMapPanelDirty           = TRUE;
	fTeamPanelDirty          = TRUE;
	fCharacterInfoPanelDirty = TRUE;
	fMapScreenBottomDirty    = TRUE;
	MarkButtonsDirty();
}


bool IsMilitiaDeploymentPanelOpen()
{
	return g_open;
}


static void PrintIn(INT16 const x, INT16 const y, INT16 const w, INT16 const h, ST::string const& str, bool const right = false)
{
	INT16 px;
	INT16 py;
	if (right) FindFontRightCoordinates( x, y, w, h, str, DEPLOY_FONT, &px, &py);
	else       FindFontCenterCoordinates(x, y, w, h, str, DEPLOY_FONT, &px, &py);
	MPrint(px, h != 0 ? py : y, str); // h 0: at y, only centred (right-aligned) across
}


// "Day 3, 14:20": when the order arrives, as the map screen's clock
static ST::string ArrivalString(MilitiaOrder const& o)
{
	UINT32 const day  = o.arrival_min / (24 * 60);
	UINT32 const hour = o.arrival_min / 60 % 24;
	UINT32 const min  = o.arrival_min % 60;
	return ST::format("{} {}, {02d}:{02d}", pDayStrings, day, hour, min);
}


static void RenderDeploymentTexts(INT16 const px, INT16 const py)
{
	SetFontDestBuffer(FRAME_BUFFER);
	SetFontAttributes(DEPLOY_FONT, DEPLOY_COLOUR);

	// title and the two headers: the name, the column labels over their
	// columns (count, town, hours / count, town, the Done buttons)
	PrintIn(px + DEPLOY_TITLE_X,    py + DEPLOY_TITLE_Y,  DEPLOY_TITLE_W,  DEPLOY_TITLE_H,  pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_TITLE]);
	PrintIn(px + DEPLOY_INCOMING_X, py + DEPLOY_HEADER_Y, DEPLOY_HEADER_W, 0, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_INCOMING]);
	PrintIn(px + DEPLOY_WAITING_X,  py + DEPLOY_HEADER_Y, DEPLOY_HEADER_W, 0, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_WAITING]);
	for (INT16 const bx : { INT16(px + DEPLOY_INCOMING_X), INT16(px + DEPLOY_WAITING_X) })
	{
		bool  const waiting_box = bx == px + DEPLOY_WAITING_X;
		INT16 const ly          = py + DEPLOY_LABELS_Y;
		PrintIn(bx + DEPLOY_COUNT_DX - 6, ly, DEPLOY_COUNT_W + 12, 0, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_COUNT]);
		PrintIn(bx + DEPLOY_TOWN_DX,      ly, DEPLOY_TOWN_W,       0, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_CITY]);
		if (waiting_box) PrintIn(bx + DEPLOY_DONE_DX - 10, ly, DEPLOY_DONE_BUTTON_W + 20, 0, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_DEPLOY]);
		else             PrintIn(bx + DEPLOY_ETA_DX - 52,  ly, DEPLOY_ETA_W + 52,         0, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_ARRIVAL], true);
	}

	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		INT16 const row_y = py + DEPLOY_ROW_FIRST_Y + rank * DEPLOY_ROW_PITCH;

		// the rank's name right of its portrait, a name of two words in two lines
		ST::string const name  = pMilitiaRecruitStrings[MILITIA_STR_RANK_FIRST + rank];
		ST_ssize_t const space = name.find_last(' ');
		if (space < 0)
		{
			PrintIn(px + DEPLOY_RANK_X, row_y, DEPLOY_RANK_W, DEPLOY_ROW_H, name);
		}
		else
		{
			INT16 const half = DEPLOY_RANK_PITCH / 2 + 1;
			PrintIn(px + DEPLOY_RANK_X, row_y - half, DEPLOY_RANK_W, DEPLOY_ROW_H, name.substr(0, space));
			PrintIn(px + DEPLOY_RANK_X, row_y + DEPLOY_RANK_PITCH - half, DEPLOY_RANK_W, DEPLOY_ROW_H, name.substr(space + 1));
		}

		// on their way: count, town, hours (values only)
		auto const incoming = RankOrders(rank, false);
		for (INT16 line = 0; line != DEPLOY_LINES && line < (INT16)incoming.size(); ++line)
		{
			MilitiaOrder const& o  = *incoming[line];
			INT16 const         ly = row_y + DEPLOY_LINE_FIRST + line * DEPLOY_LINE_PITCH;
			INT16 const         bx = px + DEPLOY_INCOMING_X;
			PrintIn(bx + DEPLOY_COUNT_DX, ly, DEPLOY_COUNT_W, DEPLOY_LINE_H, ST::format("{}", o.counts[rank]));
			PrintIn(bx + DEPLOY_TOWN_DX,  ly, DEPLOY_TOWN_W,  DEPLOY_LINE_H, MilitiaOrderAreaName(o));
			PrintIn(bx + DEPLOY_ETA_DX - 52, ly, DEPLOY_ETA_W + 52, DEPLOY_LINE_H, ArrivalString(o), true);
		}

		// waiting: count, town and the Done button
		auto const waiting = RankOrders(rank, true);
		for (INT16 line = 0; line != DEPLOY_LINES; ++line)
		{
			GUIButtonRef const b = g_done_buttons[rank][line];
			if (line >= (INT16)waiting.size())
			{
				b->Hide();
				continue;
			}
			MilitiaOrder const& o  = *waiting[line];
			INT16 const         ly = row_y + DEPLOY_LINE_FIRST + line * DEPLOY_LINE_PITCH;
			INT16 const         bx = px + DEPLOY_WAITING_X;
			PrintIn(bx + DEPLOY_COUNT_DX, ly, DEPLOY_COUNT_W, DEPLOY_LINE_H, ST::format("{}", o.counts[rank]));
			PrintIn(bx + DEPLOY_TOWN_DX,  ly, DEPLOY_TOWN_W,  DEPLOY_LINE_H, MilitiaOrderAreaName(o));
			if (b->GetUserData() != (INT32)o.id)
			{
				b->SetUserData(o.id);
				b->SetFastHelpText(st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_RETRY_FMT], MilitiaOrderAreaName(o)));
			}
			b->Show();
		}

		// TOTAL: header, then the rank's militia on their way and waiting
		UINT32 n_incoming = 0;
		UINT32 n_waiting  = 0;
		for (MilitiaOrder const* o : incoming) n_incoming += o->counts[rank];
		for (MilitiaOrder const* o : waiting)  n_waiting  += o->counts[rank];
		INT16 const tx = px + DEPLOY_TOTAL_X;
		INT16 const ty = row_y + DEPLOY_LINE_FIRST;
		PrintIn(tx, ty, DEPLOY_TOTAL_W, DEPLOY_LINE_H, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_TOTAL]);
		INT16 const ly1 = ty + DEPLOY_LINE_PITCH;
		INT16 const ly2 = ty + 2 * DEPLOY_LINE_PITCH;
		INT16 lx;
		INT16 lyy;
		FindFontCenterCoordinates(tx, ly1, 0, DEPLOY_LINE_H, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_INCOMING_LABEL], DEPLOY_FONT, &lx, &lyy);
		MPrint(tx + DEPLOY_TOTAL_LABEL_DX, lyy, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_INCOMING_LABEL]);
		FindFontCenterCoordinates(tx, ly2, 0, DEPLOY_LINE_H, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_WAITING_LABEL], DEPLOY_FONT, &lx, &lyy);
		MPrint(tx + DEPLOY_TOTAL_LABEL_DX, lyy, pMilitiaRecruitStrings[MILITIA_STR_DEPLOY_WAITING_LABEL]);
		PrintIn(tx + DEPLOY_TOTAL_LABEL_DX, ly1, DEPLOY_TOTAL_VALUE_W, DEPLOY_LINE_H, ST::format("{}", n_incoming), true);
		PrintIn(tx + DEPLOY_TOTAL_LABEL_DX, ly2, DEPLOY_TOTAL_VALUE_W, DEPLOY_LINE_H, ST::format("{}", n_waiting),  true);
	}
}


void HandleMilitiaDeploymentPanel()
{
	if (g_pending && guiCurrentScreen == MAP_SCREEN)
	{
		g_pending = false;
		CreatePanel();
	}
	if (!g_open) return;

	INT16 x, y, w, h;
	PanelRect(x, y, w, h);
	BltVideoObject(FRAME_BUFFER, guiDeploymentPanel, 0, x, y);
	RenderDeploymentTexts(x, y);
	InvalidateRegion(x, y, x + w, y + h);
	MarkAButtonDirty(g_close_button);
	for (auto const& rank_buttons : g_done_buttons)
	{
		for (GUIButtonRef const b : rank_buttons) MarkAButtonDirty(b);
	}
}
