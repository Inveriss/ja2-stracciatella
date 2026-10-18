#ifndef SIDE_STRIPS_H
#define SIDE_STRIPS_H

#include "Button_System.h"
#include "Types.h"

// The 1366x768 tactical screen's vertical button strips at the screen's left
// (x 0) and right (x 1310) edges, over the world: SIDE_STRIP_LEFT/RIGHT_1366x768.png
// (56x480, nine 50x50 slots). The bottom slot holds the button hiding / showing
// its strip (then only that button is left). Their buttons are their own,
// independent of the tactical panels: so far Statistics, the right strip's top.
// Hidden or not is kept in Ja2.set (gGameSettings).

enum SideStripKind { SIDE_STRIP_LEFT, SIDE_STRIP_RIGHT };
// the slots for other buttons, from the top (the bottom one is the toggle's)
#define SIDE_STRIP_BUTTON_SLOTS 8

// The strips are there (the 1366x768 screen, their graphics found): the panel
// buttons moved onto them go there, otherwise they stay in their old places.
bool TacticalSideStripsAvailable();

// Top left of a 50x50 slot (0 = the top one).
void TacticalSideStripSlotPos(SideStripKind strip, UINT slot, INT16& x, INT16& y);

// A button placed in a slot by its owner (e.g. the SM panel's Statistics);
// the strip shows / hides it with itself and redraws it over the strip. Set
// GUIButtonRef::NoButton() before removing the button.
void SetTacticalSideStripButton(SideStripKind strip, UINT slot, GUIButtonRef button);

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
