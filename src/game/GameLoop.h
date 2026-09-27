#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "MessageBoxScreen.h"
#include "ScreenIDs.h"


void InitializeGame(void);
void ShutdownGame(void);
void GameLoop(void);

// handle exit from game due to shortcut key
void HandleShortCutExitState();

void SetPendingNewScreen(ScreenID);

// Registers which part of the frame fills the screen while the "Stretch"
// option is on (see VideoSetStretchRegionProvider()): the strategic map's own
// canvas on the map screen, the laptop's 640x480 canvas on the laptop screen
// if `stretchLaptop` ("Stretch Laptop"), the whole frame everywhere else.
void InitStretchRegion(bool stretchLaptop);

extern ScreenID guiPendingScreen;

void NextLoopCheckForEnoughFreeHardDriveSpace(void);

// callback to confirm game is over
void EndGameMessageBoxCallBack(MessageBoxReturnValue);

#endif
