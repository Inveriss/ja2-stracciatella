#ifndef __MILITIA_CONTROL_H
#define __MILITIA_CONTROL_H

#include "Types.h"
#include "Campaign_Types.h"

void PrepareMilitiaForTactical(void);
void ResetMilitia(void);
void HandleMilitiaPromotions();

// Militia reinforcements for a battle in the loaded sector (a recruitment
// order joining it, docs/militia-arrival-time-plan.md 4c): how many more
// militia the battle can take -- the sector's room and the militia team's
// free slots
UINT8 MilitiaReinforcementRoom();

// Up to n of counts[rank] militia (decremented) enter the loaded sector's
// battle from a map edge -- the one facing a neighbouring sector of the same
// town, else the one farthest from the enemies -- the best ranks first, and
// go for the enemy; they count in the sector (strategic). The number added.
UINT8 AddMilitiaReinforcementsToBattle(UINT8 (&counts)[MAX_MILITIA_LEVELS], UINT8 n);

extern BOOLEAN gfStrategicMilitiaChangesMade;

#endif
