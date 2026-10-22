#include "Militia_Recruit_Panel.h"
#include "Militia_Deployment_Panel.h"
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
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Bottom.h"
#include "MessageBoxScreen.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "ScreenIDs.h"
#include "Soldier_Control.h"
#include "Text.h"
#include "StrategicMap.h"
#include "Strategic_Town_Loyalty.h"
#include "Town_Militia.h"
#include "UILayout.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"

#include "LaptopSave.h"

#include <string_theory/format>
#include <string_theory/string>

#include <algorithm>
#include <iterator>


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
// the count / cost box of a row (363,y+8 237x66): - and + buttons, then the
// "Count" and "Cost" columns (label over value) with "=" between the values
char const* const RECRUIT_BUTTONS_GFX = INTERFACEDIR "/MILITIA_RECRUIT_BUTTONS.STI"; // 39x33: 0/1 -, 2/3 +, 4/5 done
constexpr INT16 RECRUIT_MINUS_X      = 372;
constexpr INT16 RECRUIT_PLUS_X       = 416;
constexpr INT16 RECRUIT_BUTTON_DY    = 23; // from the row frame's top
constexpr INT16 RECRUIT_LABEL_DY     = 23;
constexpr INT16 RECRUIT_VALUE_DY     = 40;
constexpr INT16 RECRUIT_COUNT_X      = 459;
constexpr INT16 RECRUIT_COUNT_W      = 44;
constexpr INT16 RECRUIT_EQUALS_X     = 503;
constexpr INT16 RECRUIT_EQUALS_W     = 16;
constexpr INT16 RECRUIT_COST_X       = 519;
constexpr INT16 RECRUIT_COST_W       = 72;
// total cost box (618,331 127x66), the buy button 10 px under it, centred
constexpr RecruitTextPos RECRUIT_TOTAL_LABEL = { 621, 348, 121 };
constexpr RecruitTextPos RECRUIT_TOTAL_VALUE = { 621, 369, 121 };
constexpr INT16 RECRUIT_BUY_X = 662;
constexpr INT16 RECRUIT_BUY_Y = 407;
// arrival time box (618,239 127x66): header, then a line per option -- check
// box (the sector inventory's check box sheet, its 15x15 images), name, hours
// right-aligned -- in the smaller font
#define RECRUIT_SMALL_FONT FONT10ARIAL
constexpr RecruitTextPos RECRUIT_ARRIVAL_HEADER = { 621, 244, 121 };
char const* const RECRUIT_CHECKBOX_GFX = INTERFACEDIR "/Sector_Inventory_PopupCheck.sti";
constexpr INT16 RECRUIT_CHECKBOX_X      = 624;
constexpr INT16 RECRUIT_ARRIVAL_FIRST_Y = 257;
constexpr INT16 RECRUIT_ARRIVAL_PITCH   = 15;
constexpr INT16 RECRUIT_ARRIVAL_NAME_X  = 643;
constexpr INT16 RECRUIT_ARRIVAL_W       = 95; // name .. right edge of the hours
// current balance box (618,147 127x66), at the height of the first rank row's boxes
constexpr RecruitTextPos RECRUIT_BALANCE_LABEL = { 621, 164, 121 };
constexpr RecruitTextPos RECRUIT_BALANCE_VALUE = { 621, 185, 121 };

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
	RECRUIT_STR_COST,
	RECRUIT_STR_NO_MILITIA, // the map's militia view without militia (Map_Screen_Interface_Border.cc)
	RECRUIT_STR_CURRENT_BALANCE,
	RECRUIT_STR_ARRIVAL_TIME,
	RECRUIT_STR_ARRIVAL_FIRST, // Express, Fast, Standard (MilitiaArrivalOption)
	RECRUIT_STR_HOURS_FMT = RECRUIT_STR_ARRIVAL_FIRST + NUM_MILITIA_ARRIVAL_OPTIONS,
	RECRUIT_STR_ARRIVAL_HELP_FMT,
	RECRUIT_STR_MENU_RECRUITMENT, // the Militia menu of Assign > Train (Assignments.cc)
	RECRUIT_STR_MENU_DEPLOYMENT,
	RECRUIT_STR_ORDER_COMING,     // the militia orders (Town_Militia.cc)
	RECRUIT_STR_ORDER_ARRIVED,
	RECRUIT_STR_ORDER_WAITING,
	RECRUIT_STR_ORDER_PARTIAL,
	RECRUIT_STR_ORDER_ATTACK,
	RECRUIT_STR_ORDER_JOIN,
	RECRUIT_STR_DEPLOY_TITLE, // the deployment panel (Militia_Deployment_Panel.cc)
	RECRUIT_STR_DEPLOY_INCOMING,
	RECRUIT_STR_DEPLOY_WAITING,
	RECRUIT_STR_DEPLOY_TOTAL,
	RECRUIT_STR_DEPLOY_INCOMING_LABEL,
	RECRUIT_STR_DEPLOY_WAITING_LABEL,
	RECRUIT_STR_DEPLOY_RETRY_FMT
};
static_assert(RECRUIT_STR_MENU_RECRUITMENT == MILITIA_MENU_STR_RECRUITMENT);
static_assert(RECRUIT_STR_MENU_DEPLOYMENT  == MILITIA_MENU_STR_DEPLOYMENT);
static_assert(RECRUIT_STR_ORDER_COMING     == MILITIA_STR_ORDER_COMING);
static_assert(RECRUIT_STR_ORDER_ARRIVED    == MILITIA_STR_ORDER_ARRIVED);
static_assert(RECRUIT_STR_ORDER_WAITING    == MILITIA_STR_ORDER_WAITING);
static_assert(RECRUIT_STR_ORDER_PARTIAL    == MILITIA_STR_ORDER_PARTIAL);
static_assert(RECRUIT_STR_ORDER_ATTACK     == MILITIA_STR_ORDER_ATTACK);
static_assert(RECRUIT_STR_ORDER_JOIN       == MILITIA_STR_ORDER_JOIN);
static_assert(RECRUIT_STR_HOURS_FMT        == MILITIA_STR_HOURS_FMT);
static_assert(RECRUIT_STR_DEPLOY_TITLE     == MILITIA_STR_DEPLOY_TITLE);
static_assert(RECRUIT_STR_DEPLOY_RETRY_FMT == MILITIA_STR_DEPLOY_RETRY_FMT);

bool         g_open = false;
SGPSector    g_sector;                 // the recruiting merc's sector
SOLDIERTYPE const* g_pending = NULL;   // asked for on the tactical screen, opens on the map screen
MOUSE_REGION g_blanket;                // the whole panel, so that clicks don't reach the map under it
GUIButtonRef g_close_button;
GUIButtonRef g_minus_buttons[MAX_MILITIA_LEVELS];
GUIButtonRef g_plus_buttons[MAX_MILITIA_LEVELS];
GUIButtonRef g_buy_button;
GUIButtonRef g_arrival_buttons[NUM_MILITIA_ARRIVAL_OPTIONS];
UINT8        g_counts[MAX_MILITIA_LEVELS]; // militia picked to buy, per rank
UINT8        g_arrival = MILITIA_ARRIVAL_STANDARD; // the arrival option checked

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


static UINT32 PickedTotal()
{
	UINT32 n = 0;
	for (UINT8 const c : g_counts) n += c;
	return n;
}


static INT32 PickedCost()
{
	INT32 cost = 0;
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		cost += g_counts[rank] * MilitiaRecruitUnitPrice(rank, g_sector);
	}
	return cost;
}


// What the purchase costs: the militia picked and the arrival option's extra
static INT32 TotalCost()
{
	return PickedCost() + MilitiaArrivalExtraCost(g_arrival);
}


// The arrival option can be paid with the militia picked
static bool CanAffordArrival(UINT8 const option)
{
	return PickedCost() + MilitiaArrivalExtraCost(option) <= LaptopSaveInfo.iCurrentBalance;
}


// The checked option no longer affordable: the most expensive one that is
// (Standard costs nothing extra)
static void ClampArrival()
{
	if (CanAffordArrival(g_arrival)) return;
	for (UINT8 option = 0; option != NUM_MILITIA_ARRIVAL_OPTIONS; ++option)
	{
		if (CanAffordArrival(option))
		{
			g_arrival = option;
			return;
		}
	}
	g_arrival = MILITIA_ARRIVAL_STANDARD;
}


// The picks never exceed the room in the town (it can shrink, e.g. after a buy)
static void ClampPicks()
{
	UINT32 const free_slots = MilitiaRecruitFreeSlots(g_sector);
	for (UINT8 rank = MAX_MILITIA_LEVELS; rank-- != 0;)
	{
		while (g_counts[rank] != 0 && PickedTotal() > free_slots) --g_counts[rank];
	}
}


// One more militia of this rank fits into the town and into the current
// balance (with the ones already picked)
static bool CanPickOneMore(UINT8 const rank)
{
	if (g_counts[rank] == 255)                              return false;
	if (PickedTotal() >= MilitiaRecruitFreeSlots(g_sector)) return false;
	return TotalCost() + MilitiaRecruitUnitPrice(rank, g_sector) <= LaptopSaveInfo.iCurrentBalance;
}


// Exactly one option checked: a click checks the clicked one
static void ArrivalButtonCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	UINT8 const option = btn->GetUserData();
	if (CanAffordArrival(option)) g_arrival = option;
}


// The check boxes show the checked option, greyed out when unaffordable
static void UpdateArrivalButtons()
{
	ClampArrival();
	for (UINT8 option = 0; option != NUM_MILITIA_ARRIVAL_OPTIONS; ++option)
	{
		GUIButtonRef const b = g_arrival_buttons[option];
		EnableButton(b, CanAffordArrival(option));
		if (option == g_arrival) b->uiFlags |=  BUTTON_CLICKED_ON;
		else                     b->uiFlags &= ~BUTTON_CLICKED_ON;
	}
}


// A right click on - or + takes or adds this many at once
#define RECRUIT_BIG_STEP 5

static void RemovePicks(GUI_BUTTON* const btn, UINT8 const step)
{
	UINT8& n = g_counts[btn->GetUserData()];
	n -= std::min(n, step);
}


// no more than the town has room for and the balance pays
static void AddPicks(GUI_BUTTON* const btn, UINT8 const step)
{
	UINT8 const rank = btn->GetUserData();
	for (UINT8 i = 0; i != step && CanPickOneMore(rank); ++i) ++g_counts[rank];
}


static void MinusButtonCallbackPrimary(GUI_BUTTON* const btn, UINT32)   { RemovePicks(btn, 1); }
static void MinusButtonCallbackSecondary(GUI_BUTTON* const btn, UINT32) { RemovePicks(btn, RECRUIT_BIG_STEP); }
static void PlusButtonCallbackPrimary(GUI_BUTTON* const btn, UINT32)    { AddPicks(btn, 1); }
static void PlusButtonCallbackSecondary(GUI_BUTTON* const btn, UINT32)  { AddPicks(btn, RECRUIT_BIG_STEP); }


static void BuyButtonCallback(GUI_BUTTON*, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (PickedTotal() == 0) return;

	if (PickedTotal() > MilitiaRecruitFreeSlots(g_sector))
	{
		// "<town> is full of militia"
		INT8 const town_id = GetTownIdForSector(g_sector);
		ST::string const where = town_id != BLANK_SECTOR ? GCM->getTownName(town_id) : g_sector.AsShortString();
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, st_format_printf(zMarksMapScreenText[20], where), MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		ClampPicks();
		return;
	}
	if (LaptopSaveInfo.iCurrentBalance < TotalCost())
	{
		// "You can't afford it."
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, pMilitiaConfirmStrings[2], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	if (BuyMilitia(g_sector, g_counts, g_arrival))
	{
		// bought: start over (the panel stays open), and when they arrive
		std::fill(std::begin(g_counts), std::end(g_counts), 0);
		fMapScreenBottomDirty = TRUE; // the balance
		DoMapMessageBox(MSG_BOX_BASIC_STYLE,
			st_format_printf(pMilitiaRecruitStrings[RECRUIT_STR_ORDER_COMING], int(MilitiaArrivalHours(g_arrival))),
			MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
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

	// a - and a + button per rank row
	std::fill(std::begin(g_counts), std::end(g_counts), 0);
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		INT16 const row_y = y + RECRUIT_ROW_FIRST_Y + rank * RECRUIT_ROW_PITCH + RECRUIT_BUTTON_DY;
		// a left click changes the count by 1, a right click by RECRUIT_BIG_STEP
		g_minus_buttons[rank] = QuickCreateButtonImg(RECRUIT_BUTTONS_GFX, 0, 1, x + RECRUIT_MINUS_X, row_y, MSYS_PRIORITY_HIGHEST,
			ButtonCallbackPrimarySecondary(MinusButtonCallbackPrimary, MinusButtonCallbackSecondary));
		g_minus_buttons[rank]->SetUserData(rank);
		g_plus_buttons[rank]  = QuickCreateButtonImg(RECRUIT_BUTTONS_GFX, 2, 3, x + RECRUIT_PLUS_X,  row_y, MSYS_PRIORITY_HIGHEST,
			ButtonCallbackPrimarySecondary(PlusButtonCallbackPrimary, PlusButtonCallbackSecondary));
		g_plus_buttons[rank]->SetUserData(rank);
	}
	// buy (doesn't close the panel)
	g_buy_button = QuickCreateButtonImg(RECRUIT_BUTTONS_GFX, 4, 5, x + RECRUIT_BUY_X, y + RECRUIT_BUY_Y, MSYS_PRIORITY_HIGHEST, BuyButtonCallback);

	// the arrival options, Standard checked
	g_arrival = MILITIA_ARRIVAL_STANDARD;
	for (UINT8 option = 0; option != NUM_MILITIA_ARRIVAL_OPTIONS; ++option)
	{
		INT16 const oy = y + RECRUIT_ARRIVAL_FIRST_Y + option * RECRUIT_ARRIVAL_PITCH;
		GUIButtonRef const b = CreateCheckBoxButton(x + RECRUIT_CHECKBOX_X, oy, RECRUIT_CHECKBOX_GFX, MSYS_PRIORITY_HIGHEST, ArrivalButtonCallback, 0);
		b->SetUserData(option);
		b->SetFastHelpText(st_format_printf(pMilitiaRecruitStrings[RECRUIT_STR_ARRIVAL_HELP_FMT],
			int(MilitiaArrivalHours(option)), SPrintMoney(MilitiaArrivalExtraCost(option))));
		g_arrival_buttons[option] = b;
	}

	g_open = true;
}


void OpenMilitiaRecruitPanel(SOLDIERTYPE const& s)
{
	if (g_open) CloseMilitiaRecruitPanel();
	CloseMilitiaDeploymentPanel(); // one panel at a time

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
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		RemoveButton(g_minus_buttons[rank]);
		RemoveButton(g_plus_buttons[rank]);
	}
	RemoveButton(g_buy_button);
	for (GUIButtonRef& b : g_arrival_buttons) RemoveButton(b);
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
		INT32 const unit_price = MilitiaRecruitUnitPrice(rank, g_sector);
		PrintCentered(px, py, { RECRUIT_RANK_X, INT16(row_y + RECRUIT_RANK_PRICE_DY), RECRUIT_RANK_W }, st_format_printf(pMilitiaRecruitStrings[RECRUIT_STR_YOUR_PRICE_FMT], SPrintMoney(unit_price)));

		// count = cost
		PrintCentered(px, py, { RECRUIT_COUNT_X,  INT16(row_y + RECRUIT_LABEL_DY), RECRUIT_COUNT_W  }, pMilitiaRecruitStrings[RECRUIT_STR_COUNT]);
		PrintCentered(px, py, { RECRUIT_COUNT_X,  INT16(row_y + RECRUIT_VALUE_DY), RECRUIT_COUNT_W  }, ST::format("{}", g_counts[rank]));
		PrintCentered(px, py, { RECRUIT_EQUALS_X, INT16(row_y + RECRUIT_VALUE_DY), RECRUIT_EQUALS_W }, "=");
		PrintCentered(px, py, { RECRUIT_COST_X,   INT16(row_y + RECRUIT_LABEL_DY), RECRUIT_COST_W   }, pMilitiaRecruitStrings[RECRUIT_STR_COST]);
		PrintCentered(px, py, { RECRUIT_COST_X,   INT16(row_y + RECRUIT_VALUE_DY), RECRUIT_COST_W   }, SPrintMoney(g_counts[rank] * unit_price));
	}

	// current balance
	PrintCentered(px, py, RECRUIT_BALANCE_LABEL, pMilitiaRecruitStrings[RECRUIT_STR_CURRENT_BALANCE]);
	PrintCentered(px, py, RECRUIT_BALANCE_VALUE, SPrintMoney(LaptopSaveInfo.iCurrentBalance));

	// total cost: the militia and the arrival option's extra
	PrintCentered(px, py, RECRUIT_TOTAL_LABEL, pMilitiaRecruitStrings[RECRUIT_STR_TOTAL_COST]);
	PrintCentered(px, py, RECRUIT_TOTAL_VALUE, SPrintMoney(TotalCost()));

	// the arrival options: name, hours right-aligned
	PrintCentered(px, py, RECRUIT_ARRIVAL_HEADER, pMilitiaRecruitStrings[RECRUIT_STR_ARRIVAL_TIME]);
	SetFont(RECRUIT_SMALL_FONT);
	for (UINT8 option = 0; option != NUM_MILITIA_ARRIVAL_OPTIONS; ++option)
	{
		// the text's middle at the 15 px check box's
		INT16 const oy = RECRUIT_ARRIVAL_FIRST_Y + option * RECRUIT_ARRIVAL_PITCH + (15 - GetFontHeight(RECRUIT_SMALL_FONT)) / 2;
		MPrint(px + RECRUIT_ARRIVAL_NAME_X, py + oy, pMilitiaRecruitStrings[RECRUIT_STR_ARRIVAL_FIRST + option]);
		ST::string const hours = st_format_printf(pMilitiaRecruitStrings[RECRUIT_STR_HOURS_FMT], int(MilitiaArrivalHours(option)));
		INT16 hx;
		INT16 hy;
		FindFontRightCoordinates(px + RECRUIT_ARRIVAL_NAME_X, py + oy, RECRUIT_ARRIVAL_W, 0, hours, RECRUIT_SMALL_FONT, &hx, &hy);
		MPrint(hx, py + oy, hours);
	}
	SetFont(RECRUIT_FONT);
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
	ClampPicks();
	UpdateArrivalButtons();
	// + greyed out once one more wouldn't fit into the town or the balance
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		EnableButton(g_plus_buttons[rank], CanPickOneMore(rank));
	}
	RenderRecruitTexts(x, y);
	InvalidateRegion(x, y, x + w, y + h);
	MarkAButtonDirty(g_close_button);
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		MarkAButtonDirty(g_minus_buttons[rank]);
		MarkAButtonDirty(g_plus_buttons[rank]);
	}
	MarkAButtonDirty(g_buy_button);
	for (GUIButtonRef const b : g_arrival_buttons) MarkAButtonDirty(b);
}
