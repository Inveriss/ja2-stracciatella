#ifndef MILITIA_RECRUIT_PANEL_H
#define MILITIA_RECRUIT_PANEL_H

// The militia recruitment panel (game.json "militia_recruitment"): opened by
// Assign > Train > Militia instead of the militia training, for the town (or
// SAM site) where the merc stands. Lives on the strategic map screen; asked
// for from the tactical screen it opens once the map screen is entered.

#include "JA2Types.h"

// Opens the panel for this merc's town (going to the map screen first if needed)
void OpenMilitiaRecruitPanel(SOLDIERTYPE const&);

void CloseMilitiaRecruitPanel();

bool IsMilitiaRecruitPanelOpen();

// Every map screen frame: opens a panel asked for from the tactical screen and
// draws the open one
void HandleMilitiaRecruitPanel();

#endif
