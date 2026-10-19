#ifndef MILITIA_WATCH_H
#define MILITIA_WATCH_H

// Watching a militia battle: from the autoresolve screen of a battle where only
// militia fight on the player's side, the sector is loaded and the battle is
// fought in tactical by the AI (militia against the enemy), the player only
// watches. The player's turns end by themselves; when the battle is over the
// game goes back to the map screen.

#include "JA2Types.h"

// The loaded sector's battle is being watched
extern BOOLEAN gfWatchingMilitiaBattle;

// Asked for on the autoresolve screen: the sector is loaded once the map
// screen is reached
void RequestWatchMilitiaBattle(const SGPSector&);

// Every map screen frame: loads the sector of a requested watched battle
void HandleWatchMilitiaBattleRequest();

// Every tactical frame: ends the player's (empty) turns of a watched battle
void HandleWatchedMilitiaBattleTurn();

// The watched battle was won: back to the map screen after a moment
void WatchedMilitiaBattleWon();

// The sector of the watched battle is unloaded: the watching ends
void EndWatchedMilitiaBattle();

#endif
