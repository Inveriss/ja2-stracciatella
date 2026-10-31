#ifndef _MERC_ENTRING_H
#define _MERC_ENTRING_H

#include "JA2Types.h"

void ResetHeliSeats(void);
void AddMercToHeli(SOLDIERTYPE* s);

void StartHelicopterRun( INT16 sGridNoSweetSpot );
// A run that puts its mercs (AddMercToHeli()) into `sector` by
// `insertion_code` -- StartHelicopterRun() is the game's start: its start
// sector, north
void StartHelicopterRunAt(INT16 sGridNoSweetSpot, SGPSector const& sector, UINT8 insertion_code);
bool IsHelicopterRunActive();


void HandleHeliDrop(void);


extern BOOLEAN gfIngagedInDrop;

#endif
