#ifndef _MERC_ENTRING_H
#define _MERC_ENTRING_H

#include "JA2Types.h"

void ResetHeliSeats(void);
void AddMercToHeli(SOLDIERTYPE* s);

void StartHelicopterRun( INT16 sGridNoSweetSpot );
// A run that puts its mercs (AddMercToHeli()) into `sector` by
// `insertion_code` -- StartHelicopterRun() is the game's start: its start
// sector, north. INSERTION_CODE_GRIDNO: each at his own
// usStrategicInsertionData. level 1: onto a roof.
void StartHelicopterRunAt(INT16 sGridNoSweetSpot, SGPSector const& sector, UINT8 insertion_code, INT8 level = 0);
bool IsHelicopterRunActive();


void HandleHeliDrop(void);


extern BOOLEAN gfIngagedInDrop;

#endif
