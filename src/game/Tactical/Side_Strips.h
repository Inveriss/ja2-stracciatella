#ifndef SIDE_STRIPS_H
#define SIDE_STRIPS_H

#include "Types.h"

// The 1366x768 tactical screen's vertical button strips at the screen's left
// (x 0) and right (x 1310) edges, over the world: SIDE_STRIP_LEFT/RIGHT_1366x768.png
// (56x480, nine 50x50 slots). The bottom slot holds the button hiding / showing
// its strip (then only that button is left); the others are empty for now.
// Hidden or not is kept in Ja2.set (gGameSettings).

// With the tactical panel (InitializeCurrentPanel() / ShutdownCurrentPanel()).
void CreateTacticalSideStrips();
void RemoveTacticalSideStrips();

// Every frame, over the world (RenderTopmostTacticalInterface()) -- and from
// the overhead map, where they are off.
void RenderTacticalSideStrips();

// The width the left strip takes from the screen's left edge (0 when it is
// not shown): the tactical messages start right of it.
INT16 TacticalSideStripLeftWidth();

// The world's edge-scroll / exit-cursor zones at the left and right screen
// edges, moved inside, next to the strips where these are shown.
bool InTacticalLeftEdgeZone(INT32 x, INT32 y);
bool InTacticalRightEdgeZone(INT32 x, INT32 y);

#endif
