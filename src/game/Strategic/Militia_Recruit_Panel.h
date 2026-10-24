#ifndef MILITIA_RECRUIT_PANEL_H
#define MILITIA_RECRUIT_PANEL_H

// The militia recruitment panel (game.json "militia_recruitment"): opened by
// Assign > Train > Militia instead of the militia training, for the town (or
// SAM site) where the merc stands. Lives on the strategic map screen; asked
// for from the tactical screen it opens once the map screen is entered.

#include "JA2Types.h"

// Opens the panel for this merc's town (going to the map screen first if needed)
void OpenMilitiaRecruitPanel(SOLDIERTYPE const&);

// Assign > Train > Militia opens a menu (Assignments.cc) of these two
// pMilitiaRecruitStrings: the recruitment panel and the deployment panel
#define MILITIA_MENU_STR_RECRUITMENT 23
#define MILITIA_MENU_STR_DEPLOYMENT  24

// pMilitiaRecruitStrings of the militia orders (Town_Militia.cc): bought,
// arrived, waiting
#define MILITIA_STR_ORDER_COMING     25
#define MILITIA_STR_ORDER_ARRIVED    26
#define MILITIA_STR_ORDER_WAITING    27
#define MILITIA_STR_ORDER_PARTIAL    28 // only some can enter: let them?
#define MILITIA_STR_ORDER_ATTACK     29 // the enemy holds the sector: attack?
#define MILITIA_STR_ORDER_JOIN       30 // a battle there: join it?

// pMilitiaRecruitStrings of the deployment panel (Militia_Deployment_Panel.cc)
#define MILITIA_STR_HOURS_FMT              21 // "%d h"
#define MILITIA_STR_DEPLOY_TITLE           31
#define MILITIA_STR_DEPLOY_INCOMING        32
#define MILITIA_STR_DEPLOY_WAITING         33
#define MILITIA_STR_DEPLOY_TOTAL           34
#define MILITIA_STR_DEPLOY_INCOMING_LABEL  35
#define MILITIA_STR_DEPLOY_WAITING_LABEL   36
#define MILITIA_STR_DEPLOY_RETRY_FMT       37 // the Done button's help
// "%d green militia have arrived in %s.", then regular, elite, elite heavy
#define MILITIA_STR_ORDER_ARRIVED_RANK_FIRST 38
// the militia box's Done: "Your mercenaries must liberate %s first. ..."
#define MILITIA_STR_LIBERATE_FIRST           42
// the militia box's arrival mode: the Send to Deployment Center button, its help
#define MILITIA_STR_DEPLOY_SEND              43
#define MILITIA_STR_DEPLOY_SEND_HELP         44

void CloseMilitiaRecruitPanel();

bool IsMilitiaRecruitPanelOpen();

// Every map screen frame: opens a panel asked for from the tactical screen and
// draws the open one
void HandleMilitiaRecruitPanel();

#endif
