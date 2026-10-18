#ifndef __INTERFACE_CONTROL_H
#define __INTERFACE_CONTROL_H

#include "JA2Types.h"


void SetUpInterface();
void ResetInterface();
void RenderTopmostTacticalInterface(void);
void RenderTacticalInterface(void);

void RenderTacticalInterfaceWhileScrolling(void);

void EraseInterfaceMenus( BOOLEAN fIgnoreUIUnLock );

void ResetInterfaceAndUI(void);

bool AreWeInAUIMenu();

void HandleTacticalPanelSwitch();

bool InterfaceOKForMeanwhilePopup();

extern BOOLEAN gfRerenderInterfaceFromHelpText;

// If given a null pointer, show the team panel. Otherwise show the given merc
// in the single merc panel.
void SetNewPanel(SOLDIERTYPE*);

// Builds the team or SM panel again next frame (HandleTacticalPanelSwitch()).
void RebuildTacticalTeamPanel();

// Switches the SM panels' Show Large Icons (UILayout::setSMLargeIcons()) and
// builds the SM panel again, next frame.
void ToggleSMLargeIcons();

// Opens the overhead map next frame (HandleTacticalPanelSwitch()) -- for
// buttons of the SM panel, which opening it removes.
void RequestOverheadMap();

#endif
