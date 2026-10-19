#include "Militia_Recruit_Panel.h"
#include "Button_System.h"
#include "Directories.h"
#include "GameScreen.h"
#include "HImage.h"
#include "JAScreens.h"
#include "MapScreen.h"
#include "Map_Screen_Interface_Bottom.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "ScreenIDs.h"
#include "Soldier_Control.h"
#include "UILayout.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"


namespace {

cache_key_t const guiRecruitPanel{ INTERFACEDIR "/MILITIA_RECRUIT_PANEL_1366x768.png" };
// The close button: the map inventory's big Done button (mapinv_done_buttons.sti
// sub-images 2 = ready, 3 = pressed, 50x50)
char const* const RECRUIT_CLOSE_BUTTON_GFX = INTERFACEDIR "/mapinv_done_buttons.sti";
constexpr INT16   RECRUIT_CLOSE_BUTTON_SIZE   = 50;
constexpr INT16   RECRUIT_CLOSE_BUTTON_MARGIN = 10; // from the panel's bottom right corner

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
	InvalidateRegion(x, y, x + w, y + h);
	MarkAButtonDirty(g_close_button);
}
