#ifndef __TACTICAL_PLACEMENT_GUI_H
#define __TACTICAL_PLACEMENT_GUI_H

#include "JA2Types.h"


void InitTacticalPlacementGUI();
void TacticalPlacementHandle(void);

void HandleTacticalPlacementClicksInOverheadMap(INT32 reason);

// 1366x768 with overheadinterface_1366x768.png: the mercs are placed in the
// 1:1 tactical view (scrollable) above a full width panel instead of on the
// overhead map. Only meaningful while gfTacticalPlacementGUIActive.
bool TacticalPlacementFullView();
#define TACTICAL_PLACEMENT_PANEL_HEIGHT 200

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
