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
#include "AI.h"
#include "Animation_Control.h"
#include "Bullets.h"
#include "Explosion_Control.h"
#include "OppList.h"
#include "Physics.h"
#include "RenderWorld.h"
#include "Soldier_Control.h"
#include "Soldier_Functions.h"
#include "Animation_Data.h"
#include "Logger.h"

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
static bool      g_show_all_set = false; // SHOW_ALL_MERCS set by the watching

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

	// there is no merc: at 1366x768 the watched battle's bar (WATCH_BAR_FILE,
	// only the EXIT button on it), otherwise the low team panel
	// (bottom_bar_low_big_1366x768.png)
	if (!g_low_team_panel_forced) g_saved_low_team_panel = g_ui.isLowTeamPanel();
	g_low_team_panel_forced = true;
	g_ui.setLowTeamPanel(true);
	g_ui.setWatchBar(true);
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
		// in the middle of the watched battle's bar, or over the low team panel
		// at the screen's right edge
		bool  const bar = g_ui.isWatchBar();
		INT16 const x   = bar ? (SCREEN_WIDTH - WATCH_EXIT_W) / 2 :
			SCREEN_WIDTH - WATCH_EXIT_MARGIN - WATCH_EXIT_W;
		INT16 const y   = bar ? SCREEN_HEIGHT - WATCH_BAR_HEIGHT + (WATCH_BAR_HEIGHT - WATCH_EXIT_H) / 2 :
			SCREEN_HEIGHT - BIG_TEAMPANEL_HEIGHT - WATCH_EXIT_MARGIN - WATCH_EXIT_H;
		g_exit_button = QuickCreateButtonImg(WATCH_EXIT_GFX, 6, 7, x, y, MSYS_PRIORITY_HIGHEST, ExitButtonCallback);
		g_exit_button->SpecifyGeneralTextAttributes(pMilitiaWatchStrings[2], FONT12ARIAL, 169, FONT_NEARBLACK);
	}
}


// Real time: nothing of an attack is on its way any more (bullets, thrown or
// launched objects, explosions waiting to go off); the attack busy count is not
// asked, several soldiers attack at once and it seldom drops to 0
static bool NoAttackInFlight()
{
	return !BulletInFlight(NULL) && !ObjectInFlight(NULL) && gubElementsOnExplosionQueue == 0;
}


// The end of an attack (the AI soldier free for his next action, his target
// under fire and noticing him, the muzzle flash off) is handled where the attack
// busy count drops to 0, for the last attacker only.  In a watched battle in real
// time several soldiers attack at once, so every AI attacker whose attack is over
// (nothing of his on its way, no firing animation) for ATTACK_OVER_DELAY gets it
// here.
#define ATTACK_OVER_DELAY 1000 // ms

static bool IsAttackAction(INT8 const action)
{
	switch (action)
	{
		case AI_ACTION_FIRE_GUN:
		case AI_ACTION_TOSS_PROJECTILE:
		case AI_ACTION_THROW_KNIFE:
		case AI_ACTION_KNIFE_STAB:
			return true;
		default:
			return false;
	}
}

static bool OwnAttackUnderway(SOLDIERTYPE const& s)
{
	UINT32 const anim_flags = gAnimControl[s.usAnimState].uiFlags;
	return
		s.bBulletsLeft > 0                                    || // a burst goes on
		s.pTempObject != NULL                                 || // something about to be thrown
		s.bDesiredDirection != s.bDirection                   || // turning to the target
		s.usPendingAnimation != NO_PENDING_ANIMATION          ||
		anim_flags & (ANIM_FIRE | ANIM_RAISE_WEAPON | ANIM_MOVING) ||
		BulletInFlight(&s)                                    ||
		ObjectInFlight(&s);
}

static void EndOwnAttack(SOLDIERTYPE& s)
{
	SOLDIERTYPE* const tgt = s.target;
	if (tgt && tgt->bActive && tgt->bInSector && (s.uiStatusFlags & SOLDIER_ATTACK_NOTICED) &&
		tgt->ubBodyType != CROW && (s.bTeam != OUR_TEAM || tgt->bTeam != OUR_TEAM))
	{
		if (tgt->bOppList[s.ubID] != SEEN_CURRENTLY) NoticeUnseenAttacker(&s, tgt, 0);
		tgt->bUnderFire = 2;
	}
	s.uiStatusFlags   &= ~SOLDIER_ATTACK_NOTICED;
	s.ubAttackingHand  = HANDPOS;
	if (s.bWeaponMode == WM_ATTACHED) s.bWeaponMode = WM_NORMAL;
	s.sLastTarget      = s.sTargetGridNo;
	EndMuzzleFlash(&s);
	FreeUpNPCFromAttacking(&s);
}

static void EndOverAttacks()
{
	static UINT32 attack_over_since[TOTAL_SOLDIERS]; // 0: underway / no attack
	UINT32 const now = GetJA2Clock();
	FOR_EACH_SOLDIER(s)
	{
		UINT32& since = attack_over_since[s->ubID];
		if (!s->bInSector || s->bTeam == OUR_TEAM || s->bLife < OKLIFE ||
			!s->bActionInProgress || !IsAttackAction(s->bAction) || OwnAttackUnderway(*s))
		{
			since = 0;
			continue;
		}
		if (since == 0 || now < since) { since = now; continue; }
		if (now - since < ATTACK_OVER_DELAY) continue;

		since = 0;
		SLOGD("Watched battle: attack of {} over, freeing him", s->ubID);
		EndOwnAttack(*s);
	}
}


// A dying soldier (below OKLIFE) still standing, crouching or in a firing pose:
// his fall was lost (an animation that couldn't be interrupted); it is started
// again
static void FixDyingSoldiersPoses()
{
	FOR_EACH_SOLDIER(s)
	{
		if (!s->bInSector || s->bLife <= 0 || s->bLife >= OKLIFE || !s->bCollapsed) continue;
		if (s->fInNonintAnim || s->usPendingAnimation != NO_PENDING_ANIMATION)    continue;
		ANIMCONTROLTYPE const& a = gAnimControl[s->usAnimState];
		if (a.ubEndHeight == ANIM_PRONE && !(a.uiFlags & (ANIM_FIRE | ANIM_FIREREADY))) continue;

		SLOGD("Watched battle: dying soldier {} not down, collapsing again", s->ubID);
		s->bCollapsed = FALSE;
		SoldierCollapse(s);
	}
}


// A watch on the attack busy count: when it stays above 0 for
// STUCK_BUSY_COUNT_DELAY while nothing is on its way and nobody is being hit, a
// part of an attack never ended (several at once in real time); the count goes
// back to 0 and the end of the battle is checked, which needs the count at 0.
#define STUCK_BUSY_COUNT_DELAY 3000 // ms

static void HandleStuckAttackBusyCount()
{
	static UINT8  last_count;
	static UINT32 last_change;
	UINT32 const now   = GetJA2Clock();
	UINT8  const count = gTacticalStatus.ubAttackBusyCount;
	if (count != last_count || now < last_change)
	{
		last_count  = count;
		last_change = now;
		return;
	}
	if (count == 0 || now - last_change < STUCK_BUSY_COUNT_DELAY) return;
	if (!NoAttackInFlight()) return;
	CFOR_EACH_SOLDIER(s)
	{
		if (s->bInSector && s->fGettingHit) return;
	}

	SLOGW("Watched battle: attack busy count {} stuck, set to 0", count);
	gTacticalStatus.ubAttackBusyCount = 0;
	last_count  = 0;
	last_change = now;
	CheckForEndOfBattle(FALSE);
}


void HandleWatchedMilitiaBattleTurn()
{
	bool const exit_now = gTacticalStatus.ubAttackBusyCount == 0 ||
		(IsWatchedBattleInRealtime() && NoAttackInFlight());
	if (g_exit_requested && exit_now)
	{
		// stop watching: leave the sector, back to the map screen; a battle still
		// going on is finished by autoresolve (nobody of the player's team is here).
		// First the AI's turn ends and the interface lock goes, as at the end of a
		// battle, or the lock would stay over the map screen.
		g_exit_requested        = false;
		// real time: attacks of others may still count (nothing is in flight)
		if (IsWatchedBattleInRealtime()) gTacticalStatus.ubAttackBusyCount = 0;
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

		// no fog of war for the watcher: every soldier is drawn (SHOW_ALL_MERCS
		// changes only what the screen shows; who sees whom stays as it is for
		// the militia and the enemy)
		if (!(gTacticalStatus.uiFlags & SHOW_ALL_MERCS))
		{
			gTacticalStatus.uiFlags |= SHOW_ALL_MERCS;
			g_show_all_set = true;
			SetRenderFlags(RENDER_FLAG_FULL);
		}

		// several soldiers attack at once
		EndOverAttacks();
		HandleStuckAttackBusyCount();
		FixDyingSoldiersPoses();
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
	if (g_show_all_set)
	{
		g_show_all_set = false;
		gTacticalStatus.uiFlags &= ~SHOW_ALL_MERCS;
	}
	if (g_exit_button) RemoveButton(g_exit_button);
	g_exit_requested = false;
	if (g_low_team_panel_forced)
	{
		// the team panel's form from before the watching
		g_low_team_panel_forced = false;
		g_ui.setWatchBar(false);
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
