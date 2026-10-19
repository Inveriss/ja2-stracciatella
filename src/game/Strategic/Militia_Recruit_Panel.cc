#include "Militia_Recruit_Panel.h"
#include "Button_System.h"
#include "Campaign_Types.h"
#include "ContentManager.h"
#include "Directories.h"
#include "Finances.h"
#include "Font.h"
#include "Font_Control.h"
#include "GameInstance.h"
#include "GamePolicy.h"
#include "GameScreen.h"
#include "HImage.h"
#include "JAScreens.h"
#include "MapScreen.h"
#include "Map_Screen_Interface_Bottom.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "ScreenIDs.h"
#include "Soldier_Control.h"
#include "Text.h"
#include "Town_Militia.h"
#include "UILayout.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"

#include <string_theory/format>
#include <string_theory/string>


namespace {

cache_key_t const guiRecruitPanel{ INTERFACEDIR "/MILITIA_RECRUIT_PANEL_1366x768.png" };
// The close button: the map inventory's big Done button (mapinv_done_buttons.sti
// sub-images 2 = ready, 3 = pressed, 50x50)
char const* const RECRUIT_CLOSE_BUTTON_GFX = INTERFACEDIR "/mapinv_done_buttons.sti";
constexpr INT16   RECRUIT_CLOSE_BUTTON_SIZE   = 50;
constexpr INT16   RECRUIT_CLOSE_BUTTON_MARGIN = 10; // from the panel's bottom right corner

// The texts: FONT12ARIAL in the colour of the big weapon box's MAINTENANCE /
// BALLISTICS headers. Positions inside the panel graphic (761x647); every box
// has a 3 px frame.
#define RECRUIT_FONT   FONT12ARIAL
#define RECRUIT_COLOUR FONT_BEIGE

// a text spot: x, y of the print position, w the width it is centred in /
// right-aligned to
struct RecruitTextPos { INT16 x, y, w; };

// base prices box (16,29 148x90): header, then a line per rank (name left,
// price right)
constexpr RecruitTextPos RECRUIT_BASE_HEADER = { 19, 36, 142 };
constexpr INT16 RECRUIT_LIST_FIRST_Y = 52; // the rank / bracket lines of both top boxes
constexpr INT16 RECRUIT_LIST_PITCH   = 16;
constexpr INT16 RECRUIT_BASE_X       = 25;
constexpr INT16 RECRUIT_BASE_W       = 131;
// title bar (184,29 392x31)
constexpr RecruitTextPos RECRUIT_TITLE = { 184, 40, 392 };
// the recruiter box (195,70 369x49): three lines
constexpr RecruitTextPos RECRUIT_RECRUITER[3] = { { 198, 76, 363 }, { 198, 89, 363 }, { 198, 102, 363 } };
// price modifiers box (597,29 148x90): header, then a line per leadership bracket
constexpr RecruitTextPos RECRUIT_MOD_HEADER = { 600, 36, 142 };
constexpr INT16 RECRUIT_MOD_X = 606;
constexpr INT16 RECRUIT_MOD_W = 131;
// the rank rows (frames at y 139, 231, 323, 415, 92 apart): rank name and
// "Your price" in the box at 197,y+8 156x66
constexpr INT16 RECRUIT_ROW_FIRST_Y = 139;
constexpr INT16 RECRUIT_ROW_PITCH   = 92;
constexpr INT16 RECRUIT_RANK_X      = 200;
constexpr INT16 RECRUIT_RANK_W      = 150;
constexpr INT16 RECRUIT_RANK_NAME_DY  = 25; // from the row frame's top
constexpr INT16 RECRUIT_RANK_PRICE_DY = 46;
// total cost box (618,286 127x66)
constexpr RecruitTextPos RECRUIT_TOTAL_LABEL = { 621, 303, 121 };
constexpr RecruitTextPos RECRUIT_TOTAL_VALUE = { 621, 324, 121 };

// pMilitiaRecruitStrings
enum
{
	RECRUIT_STR_TITLE,
	RECRUIT_STR_BASE_PRICES,
	RECRUIT_STR_PRICE_MODIFIERS,
	RECRUIT_STR_BEST_RECRUITER,
	RECRUIT_STR_RECRUITER_FMT,
	RECRUIT_STR_NO_RECRUITER,
	RECRUIT_STR_MODIFIER_FMT,
	RECRUIT_STR_YOUR_PRICE_FMT,
	RECRUIT_STR_TOTAL_COST,
	RECRUIT_STR_RANK_FIRST, // Green, Regular, Elite, Elite Heavy
	RECRUIT_STR_COUNT = RECRUIT_STR_RANK_FIRST + MAX_MILITIA_LEVELS,
	RECRUIT_STR_COST
};

bool         g_open = false;
SGPSector    g_sector;                 // the recruiting merc's sector
SOLDIERTYPE const* g_pending = NULL;   // asked for on the tactical screen, opens on the map screen
MOUSE_REGION g_blanket;                // the whole panel, so that clicks don't reach the map under it
GUIButtonRef g_close_button;

}


// The panel at the map screen's right edge, from its top: over the map
static void PanelRect(INT16& x, INT16& y, INT16& w, INT16& h)
{
	ETRLEObject const& e = GetVObject(guiRecruitPanel)->SubregionProperties(0);
	w = e.usWidth;
	h = e.usHeight;
	x = MAP_SCREEN_X + MAP_SCREEN_WIDTH - w;
	y = MAP_SCREEN_Y;
}


static void CloseButtonCallback(GUI_BUTTON*, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		CloseMilitiaRecruitPanel();
	}
}


static void CreatePanel()
{
	INT16 x, y, w, h;
	PanelRect(x, y, w, h);

	MSYS_DefineRegion(&g_blanket, x, y, x + w, y + h, MSYS_PRIORITY_HIGHEST - 1, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

	INT16 const bx = x + w - RECRUIT_CLOSE_BUTTON_MARGIN - RECRUIT_CLOSE_BUTTON_SIZE;
	INT16 const by = y + h - RECRUIT_CLOSE_BUTTON_MARGIN - RECRUIT_CLOSE_BUTTON_SIZE;
	g_close_button = QuickCreateButtonImg(RECRUIT_CLOSE_BUTTON_GFX, 2, 3, bx, by, MSYS_PRIORITY_HIGHEST, CloseButtonCallback);
	g_close_button->SetFastHelpText("Close");

	g_open = true;
}


void OpenMilitiaRecruitPanel(SOLDIERTYPE const& s)
{
	if (g_open) CloseMilitiaRecruitPanel();

	g_sector = s.sSector;
	if (guiCurrentScreen != MAP_SCREEN)
	{
		// the panel belongs to the map screen: go there, it opens on arrival
		g_pending          = &s;
		gfEnteringMapScreen = TRUE;
		return;
	}
	g_pending = NULL;
	CreatePanel();
}


void CloseMilitiaRecruitPanel()
{
	g_pending = NULL;
	if (!g_open) return;

	RemoveButton(g_close_button);
	MSYS_RemoveRegion(&g_blanket);
	g_open = false;

	// what was under the panel
	fMapPanelDirty           = TRUE;
	fTeamPanelDirty          = TRUE;
	fCharacterInfoPanelDirty = TRUE;
	fMapScreenBottomDirty    = TRUE;
	MarkButtonsDirty();
}


bool IsMilitiaRecruitPanelOpen()
{
	return g_open;
}


static void PrintCentered(INT16 const px, INT16 const py, RecruitTextPos const& p, ST::string const& str)
{
	INT16 x;
	INT16 y;
	FindFontCenterCoordinates(px + p.x, py + p.y, p.w, 0, str, RECRUIT_FONT, &x, &y);
	MPrint(x, py + p.y, str);
}


static void PrintRight(INT16 const px, INT16 const py, INT16 const x0, INT16 const y0, INT16 const w, ST::string const& str)
{
	INT16 x;
	INT16 y;
	FindFontRightCoordinates(px + x0, py + y0, w, 0, str, RECRUIT_FONT, &x, &y);
	MPrint(x, py + y0, str);
}


// "+25%" / "-10%" / "0%"
static ST::string ModifierString(INT32 const percent)
{
	INT32 const change = percent - 100;
	return change > 0 ? ST::format("+{}%", change) : ST::format("{}%", change);
}


static void RenderRecruitTexts(INT16 const px, INT16 const py)
{
	SetFontDestBuffer(FRAME_BUFFER);
	SetFontAttributes(RECRUIT_FONT, RECRUIT_COLOUR);

	// title
	PrintCentered(px, py, RECRUIT_TITLE, pMilitiaRecruitStrings[RECRUIT_STR_TITLE]);

	// base prices
	PrintCentered(px, py, RECRUIT_BASE_HEADER, pMilitiaRecruitStrings[RECRUIT_STR_BASE_PRICES]);
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		INT16 const y = RECRUIT_LIST_FIRST_Y + rank * RECRUIT_LIST_PITCH;
		MPrint(px + RECRUIT_BASE_X, py + y, pMilitiaRecruitStrings[RECRUIT_STR_RANK_FIRST + rank]);
		PrintRight(px, py, RECRUIT_BASE_X, y, RECRUIT_BASE_W, SPrintMoney(gamepolicy(militia_recruit_price[rank])));
	}

	// the price modifiers by leadership
	PrintCentered(px, py, RECRUIT_MOD_HEADER, pMilitiaRecruitStrings[RECRUIT_STR_PRICE_MODIFIERS]);
	INT32 low = 1;
	for (int i = 0; i != 4; ++i)
	{
		INT16 const y    = RECRUIT_LIST_FIRST_Y + i * RECRUIT_LIST_PITCH;
		INT32 const high = gamepolicy(militia_recruit_leadership_max[i]);
		MPrint(px + RECRUIT_MOD_X, py + y, ST::format("{} - {}", low, high));
		PrintRight(px, py, RECRUIT_MOD_X, y, RECRUIT_MOD_W, ModifierString(gamepolicy(militia_recruit_price_percent[i])));
		low = high + 1;
	}

	// the recruiter: the merc with the best leadership in this town
	SOLDIERTYPE const* const recruiter = BestMilitiaRecruiter(g_sector);
	PrintCentered(px, py, RECRUIT_RECRUITER[0], pMilitiaRecruitStrings[RECRUIT_STR_BEST_RECRUITER]);
	if (recruiter)
	{
		PrintCentered(px, py, RECRUIT_RECRUITER[1], st_format_printf(pMilitiaRecruitStrings[RECRUIT_STR_RECRUITER_FMT], recruiter->name, int(recruiter->bLeadership)));
		PrintCentered(px, py, RECRUIT_RECRUITER[2], st_format_printf(pMilitiaRecruitStrings[RECRUIT_STR_MODIFIER_FMT], ModifierString(MilitiaRecruitPricePercent(recruiter->bLeadership))));
	}
	else
	{
		PrintCentered(px, py, RECRUIT_RECRUITER[1], pMilitiaRecruitStrings[RECRUIT_STR_NO_RECRUITER]);
	}

	// a row per rank: its name and the price with the recruiter's modifier
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		INT16 const row_y = RECRUIT_ROW_FIRST_Y + rank * RECRUIT_ROW_PITCH;
		PrintCentered(px, py, { RECRUIT_RANK_X, INT16(row_y + RECRUIT_RANK_NAME_DY), RECRUIT_RANK_W }, pMilitiaRecruitStrings[RECRUIT_STR_RANK_FIRST + rank]);
		ST::string const price = SPrintMoney(MilitiaRecruitUnitPrice(rank, g_sector));
		PrintCentered(px, py, { RECRUIT_RANK_X, INT16(row_y + RECRUIT_RANK_PRICE_DY), RECRUIT_RANK_W }, st_format_printf(pMilitiaRecruitStrings[RECRUIT_STR_YOUR_PRICE_FMT], price));
	}

	// total cost (the counts come with the buy buttons)
	PrintCentered(px, py, RECRUIT_TOTAL_LABEL, pMilitiaRecruitStrings[RECRUIT_STR_TOTAL_COST]);
	PrintCentered(px, py, RECRUIT_TOTAL_VALUE, SPrintMoney(0));
}


void HandleMilitiaRecruitPanel()
{
	if (g_pending && guiCurrentScreen == MAP_SCREEN)
	{
		g_pending = NULL;
		CreatePanel();
	}
	if (!g_open) return;

	INT16 x, y, w, h;
	PanelRect(x, y, w, h);
	BltVideoObject(FRAME_BUFFER, guiRecruitPanel, 0, x, y);
	RenderRecruitTexts(x, y);
	InvalidateRegion(x, y, x + w, y + h);
	MarkAButtonDirty(g_close_button);
}
