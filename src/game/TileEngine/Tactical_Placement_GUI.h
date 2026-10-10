#ifndef __TACTICAL_PLACEMENT_GUI_H
#define __TACTICAL_PLACEMENT_GUI_H

#include "JA2Types.h"
#include "Types.h"


void InitTacticalPlacementGUI();
void TacticalPlacementHandle(void);

void HandleTacticalPlacementClicksInOverheadMap(INT32 reason);

// 1366x768 with overheadinterface_north_south_1366x768.png (and the narrower
// overheadinterface_west_east_1366x768.png): the mercs are placed in the
// 1:1 tactical view (scrollable) above a full width panel instead of on the
// overhead map. Only meaningful while gfTacticalPlacementGUIActive.
bool TacticalPlacementFullView();
#define TACTICAL_PLACEMENT_PANEL_HEIGHT 200
// The rows of the screen showing the world in the 1:1 view: below the
// panel when it is at the top (mercs entering from the south), else above it.
INT16 TacticalPlacementViewTop();
INT16 TacticalPlacementViewBottom();
// The placement panel's place in the 1:1 view (false without the view); the
// narrower west / east panel leaves the world visible beside it.
bool TacticalPlacementPanelBox(SGPBox& box);
// the minimap's box with the panel (above it, below it at the top)
bool TacticalPlacementMapBox(SGPBox& box);
bool TacticalPlacementMouseOverPanel();

extern BOOLEAN gfTacticalPlacementGUIActive;
extern BOOLEAN gfEnterTacticalPlacementGUI;

extern SOLDIERTYPE *gpTacticalPlacementSelectedSoldier;
extern SOLDIERTYPE *gpTacticalPlacementHilightedSoldier;

//Saved value.  Contains the last choice for future battles.
extern UINT8	gubDefaultButton;

extern BOOLEAN gfTacticalPlacementGUIDirty;
extern BOOLEAN gfValidLocationsChanged;
extern SGPVObject* giMercPanelImage;

#endif
