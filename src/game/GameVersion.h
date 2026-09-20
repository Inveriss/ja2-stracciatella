#ifndef _GAME_VERSION_H_
#define _GAME_VERSION_H_

#include "Types.h"


//
//	Keeps track of the game version
//

extern const char g_version_label[];
extern const char g_version_number[16];


//
//		Keeps track of the saved game version.  Increment the saved game version whenever
//	you will invalidate the saved game file
//

// 105: 200 instead of 170 profiles, each with a bMercOpinion table of 200 entries.
// 104: the nickname field of a saved merc profile grew from 10 to 13 chars
// (IMP nicknames up to 12 characters).
// Bumped for the OBJECTTYPE/WORLDITEM layout change (MAX_OBJECTS_PER_SLOT
// 8 -> 100, Item_Types.h) -- old saves have a different byte layout for
// every stored item and must not be loaded against this build.
// Bumped again for sSpreadLocations going from 10 to 100 entries (+180 bytes per soldier).
constexpr UINT32 SAVE_GAME_VERSION = 106;

#endif
