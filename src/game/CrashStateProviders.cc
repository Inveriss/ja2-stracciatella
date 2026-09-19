#include "CrashHandler.h"

#include "CharProfile.h"
#include "Game_Clock.h"
#include "GameSettings.h"
#include "IMP_MainPage.h"
#include "Interface_Panels.h"
#include "Laptop.h"
#include "Map_Information.h"
#include "Overhead.h"
#include "Soldier_Control.h"
#include "StrategicMap.h"

// Small read-only snapshots of the game state for crash reports (see
// CrashHandler.h). They run inside the crash handler, so they only read
// globals and never allocate.

namespace {

void GameSessionState(char* out, unsigned size)
{
	CrashStateAppend(out, size, "difficulty %d, save mode %d, day %u %02u:%02u",
		(int)gGameOptions.ubDifficultyLevel, (int)gGameOptions.ubGameSaveMode,
		(unsigned)GetWorldDay(), (unsigned)GetWorldHour(), (unsigned)(GetWorldMinutesInDay() % 60));
}

void SectorState(char* out, unsigned size)
{
	CrashStateAppend(out, size, "sector x=%d y=%d z=%d, world loaded: %s", (int)gWorldSector.x, (int)gWorldSector.y, (int)gWorldSector.z,
		gfWorldLoaded ? "yes" : "no");
}

void TacticalState(char* out, unsigned size)
{
	CrashStateAppend(out, size, "in combat: %s, current team %d, enemy in sector: %s",
		(gTacticalStatus.uiFlags & INCOMBAT) ? "yes" : "no", (int)gTacticalStatus.ubCurrentTeam,
		gTacticalStatus.fEnemyInSector ? "yes" : "no");

	SOLDIERTYPE const* const selected = g_selected_man;
	if (selected)
	{
		CrashStateAppend(out, size, ", selected merc '%s' (profile %d) at gridno %d", selected->name.c_str(), (int)selected->ubProfile, (int)selected->sGridNo);
	}
	else
	{
		CrashStateAppend(out, size, ", no merc selected");
	}
}

void LaptopState(char* out, unsigned size)
{
	CrashStateAppend(out, size, "laptop mode %d, IMP page %d, IMP profile mode %d", (int)guiCurrentLaptopMode, (int)iCurrentImpPage, (int)iCurrentProfileMode);
}

} // namespace

void RegisterGameCrashStateProviders()
{
	RegisterCrashStateProvider("Session", GameSessionState);
	RegisterCrashStateProvider("Strategic", SectorState);
	RegisterCrashStateProvider("Tactical", TacticalState);
	RegisterCrashStateProvider("Laptop", LaptopState);
}
