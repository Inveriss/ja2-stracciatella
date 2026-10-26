#ifndef _DISPLAY_COVER__H_
#define _DISPLAY_COVER__H_

#include "JA2Types.h"


void DisplayCoverOfSelectedGridNo(void);
void RemoveCoverOfSelectedGridNo(void);

void DisplayRangeToTarget(SOLDIERTYPE* pSoldier, INT16 sTargetGridNo);


void RemoveVisibleGridNoAtSelectedGridNo(void);
void DisplayGridNoVisibleToSoldierGrid(void);

void ChangeSizeOfDisplayCover( INT32 iNewSize );

void ChangeSizeOfLOS( INT32 iNewSize );

// Every frame: two rings around each merc of ours lying down with camouflage,
// at the distances an unaware enemy sees him from lying still and crawling
void HandleCamouflageRings(void);
// The world is going: forget the rings (their tiles go with it)
void ResetCamouflageRings(void);

#endif
