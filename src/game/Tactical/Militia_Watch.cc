#include "Militia_Watch.h"
#include "Button_System.h"
#include "Directories.h"
#include "Font_Control.h"
#include "Handle_UI.h"
#include "Interface.h"
#include "Interface_Control.h"
#include "Text.h"
#include "UILayout.h"
#include "JAScreens.h"
#include "Overhead.h"
#include "ScreenIDs.h"
#include "StrategicMap.h"
#include "TeamTurns.h"
#include "Timer_Control.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "GamePolicy.h"

#include <chrono>

using namespace std::chrono_literals;


BOOLEAN gfWatchingMilitiaBattle = FALSE;


bool IsWatchedBattleInRealtime()
{
	return gfWatchingMilitiaBattle && gamepolicy(militia_watch_realtime);
}


UINT16 WatchedBattleAIDelay()
{
	return gamepolicy(militia_watch_ai_delay_ms);
}

static bool      g_watch_requested = false;
static SGPSector g_watch_sector;
static bool      g_saved_low_team_panel = false; // the team panel's form before the watching
static bool      g_low_team_panel_forced = false;
static bool      g_exit_requested = false;

// EXIT (MILITIA_RECRUIT_BUTTONS.STI 6 ready, 7 pressed, 120x33): over the team
// panel at the screen's right edge
static GUIButtonRef  g_exit_button;
static char const* const WATCH_EXIT_GFX    = INTERFACEDIR "/MILITIA_RECRUIT_BUTTONS.STI";
static INT16 const       WATCH_EXIT_W      = 120;
static INT16 const       WATCH_EXIT_H      = 33;
static INT16 const       WATCH_EXIT_MARGIN = 8;


void RequestWatchMilitiaBattle(const SGPSector& sector)
{
	g_watch_requested = true;
	g_watch_sector    = sector;
	g_watch_sector.z  = 0;
}


void HandleWatchMilitiaBattleRequest()
{
	if (!g_watch_requested || guiCurrentScreen != MAP_SCREEN) return;
	g_watch_requested = false;

	// as the pre-battle interface's "Go to sector", but nobody of the player's
	// team is there: the militia fight, the player watches
	gfWatchingMilitiaBattle = TRUE;

	// always the low team panel (bottom_bar_low_big_1366x768.png): there is no merc
	if (!g_low_team_panel_forced) g_saved_low_team_panel = g_ui.isLowTeamPanel();
	g_low_team_panel_forced = true;
	g_ui.setLowTeamPanel(true);
	RebuildTacticalTeamPanel();

	SetCurrentWorldSector(g_watch_sector);
}


static void ExitButtonCallback(GUI_BUTTON*, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	// handled next frame (the button must not be removed in its own callback)
	g_exit_requested = true;
}


// The EXIT button exists while a battle is watched on the tactical screen.
// The turns of the AI (all of them here) lock the interface with a screen-wide
// region of the highest priority (the wait cursor); the button has that
// priority too and is made again whenever such a lock begins, so that it is
// the newer region and gets the clicks.
static void HandleExitButton()
{
	static bool locked_before = false;
	bool const  locked        = gfDisableRegionActive || gfUserTurnRegionActive;
	bool const  want          = gfWatchingMilitiaBattle && guiCurrentScreen == GAME_SCREEN;
	if (g_exit_button && (!want || (locked && !locked_before)))
	{
		RemoveButton(g_exit_button);
	}
	locked_before = locked;
	if (want && !g_exit_button)
	{
		INT16 const x = SCREEN_WIDTH - WATCH_EXIT_MARGIN - WATCH_EXIT_W;
		INT16 const y = SCREEN_HEIGHT - BIG_TEAMPANEL_HEIGHT - WATCH_EXIT_MARGIN - WATCH_EXIT_H;
		g_exit_button = QuickCreateButtonImg(WATCH_EXIT_GFX, 6, 7, x, y, MSYS_PRIORITY_HIGHEST, ExitButtonCallback);
		g_exit_button->SpecifyGeneralTextAttributes(pMilitiaWatchStrings[2], FONT12ARIAL, 169, FONT_NEARBLACK);
	}
}


void HandleWatchedMilitiaBattleTurn()
{
	if (g_exit_requested && gTacticalStatus.ubAttackBusyCount == 0)
	{
		// stop watching: leave the sector, back to the map screen; a battle still
		// going on is finished by autoresolve (nobody of the player's team is here).
		// First the AI's turn ends and the interface lock goes, as at the end of a
		// battle, or the lock would stay over the map screen.
		g_exit_requested        = false;
		gfWatchingMilitiaBattle = FALSE;
		if (gTacticalStatus.ubCurrentTeam != OUR_TEAM) EndAllAITurns();
		UIHandleLUIEndLock(NULL);
		CheckAndHandleUnloadingOfCurrentWorld();
	}
	HandleExitButton();
	if (!gfWatchingMilitiaBattle)                       return;

	// real time: no interface lock (wait cursor) stays from before the watching
	// or from anything that set one up anyway; the player keeps the cursor
	if (IsWatchedBattleInRealtime())
	{
		if (gfDisableRegionActive)  UIHandleLUIEndLock(NULL);
		if (gfUserTurnRegionActive) guiPendingOverrideEvent = LA_ENDUIOUTURNLOCK;
		// ...and no turn or interrupt bar at the top of the screen
		if (gTacticalStatus.fInTopMessage) EndTopMessage();
	}
	if (!(gTacticalStatus.uiFlags & INCOMBAT))           return;
	if (gTacticalStatus.ubCurrentTeam != OUR_TEAM)       return;
	if (gTacticalStatus.ubAttackBusyCount > 0)           return;
	if (INTERRUPT_QUEUED)                                return;

	// the player has nobody here: the turn ends at once
	EndTurn(OUR_TEAM + 1);
}


static void WatchedBattleOverCallback()
{
	// leave the sector: back to the map screen (the militia stay)
	CheckAndHandleUnloadingOfCurrentWorld();
}


void EndWatchedMilitiaBattle()
{
	if (g_exit_button) RemoveButton(g_exit_button);
	g_exit_requested = false;
	if (g_low_team_panel_forced)
	{
		// the team panel's form from before the watching
		g_low_team_panel_forced = false;
		g_ui.setLowTeamPanel(g_saved_low_team_panel);
		RebuildTacticalTeamPanel();
	}
	gfWatchingMilitiaBattle = FALSE;
}


void WatchedMilitiaBattleWon()
{
	if (!gfWatchingMilitiaBattle) return;
	// a moment for the victory music
	SetCustomizableTimerCallbackAndDelay(5s, WatchedBattleOverCallback, true);
}
