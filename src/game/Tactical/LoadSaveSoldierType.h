#ifndef LOADSAVESOLDIERTYPE_H
#define LOADSAVESOLDIERTYPE_H

#include "JA2Types.h"
#include "Overhead_Types.h"

// Bytes of one saved SOLDIERTYPE (InjectSoldierType()/ExtractSoldierType()):
// 8580 (8604 in the Stracciatella Linux format) with the original 148
// soldiers, plus one byte per soldier more -- bOppList has an entry for
// every soldier (MAX_NUM_SOLDIERS, grown with PLAYER_TEAM_SIZE).
#define SOLDIER_TYPE_SAVED_SIZE              (8580 + (MAX_NUM_SOLDIERS - 148))
#define SOLDIER_TYPE_SAVED_SIZE_STRAC_LINUX  (8604 + (MAX_NUM_SOLDIERS - 148))


void ExtractSoldierType(const BYTE* Src, SOLDIERTYPE* Soldier, bool stracLinuxFormat, UINT32 uiSavedGameVersion);

void InjectSoldierType(BYTE* Dst, const SOLDIERTYPE* Soldier);

#endif
