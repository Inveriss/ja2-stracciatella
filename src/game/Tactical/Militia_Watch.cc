#include "Militia_Watch.h"
#include "JAScreens.h"
#include "Overhead.h"
#include "ScreenIDs.h"
#include "StrategicMap.h"
#include "TeamTurns.h"
#include "Timer_Control.h"

#include <chrono>

using namespace std::chrono_literals;


BOOLEAN gfWatchingMilitiaBattle = FALSE;

static bool      g_watch_requested = false;
static SGPSector g_watch_sector;


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
	SetCurrentWorldSector(g_watch_sector);
}


void HandleWatchedMilitiaBattleTurn()
{
	if (!gfWatchingMilitiaBattle)                       return;
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


void WatchedMilitiaBattleWon()
{
	if (!gfWatchingMilitiaBattle) return;
	// a moment for the victory music
	SetCustomizableTimerCallbackAndDelay(5s, WatchedBattleOverCallback, true);
}
