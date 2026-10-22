#ifndef MILITIA_DEPLOYMENT_PANEL_H
#define MILITIA_DEPLOYMENT_PANEL_H

// The militia deployment panel (MILITIA_DEPLOYMENT_PANEL_1366x768.png):
// opened by Assign > Train > Militia > Militia Deployment Center. A row per
// militia rank lists the recruitment orders on their way (count, town, hours
// to arrival) and those waiting for orders (count, town, a Done button that
// tries again to enter the town), and their totals. Lives on the strategic
// map screen; asked for from the tactical screen it opens once the map screen
// is entered. See docs/militia-arrival-time-plan.md (stage 5).

void OpenMilitiaDeploymentPanel();

void CloseMilitiaDeploymentPanel();

bool IsMilitiaDeploymentPanelOpen();

// Every map screen frame: opens a panel asked for from the tactical screen and
// draws the open one
void HandleMilitiaDeploymentPanel();

#endif
