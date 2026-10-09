#include "HImage.h"
#include "Cursors.h"
#include "Directories.h"
#include "Font.h"
#include "Input.h"
#include "Button_System.h"
#include "Map_Screen_Interface_Map.h"
#include "MouseSystem.h"
#include "Map_Screen_Interface.h"
#include "JAScreens.h"
#include "GameScreen.h"
#include "PreBattle_Interface.h"
#include "StrategicMap.h"
#include "Game_Clock.h"
#include "Music_Control.h"
#include "ContentMusic.h"
#include "SysUtil.h"
#include "Font_Control.h"
#include "Timer.h"
#include "Queen_Command.h"
#include "Strategic_Movement.h"
#include "Strategic_Pathing.h"
#include "Text.h"
#include "PopUpBox.h"
#include "Auto_Resolve.h"
#include "Sound_Control.h"
#include "English.h"
#include "Map_Screen_Interface_Bottom.h"
#include "Overhead.h"
#include "Tactical_Placement_GUI.h"
#include "Timer_Control.h"
#include "Town_Militia.h"
#include "Campaign.h"
#include "GameSettings.h"
#include "Random.h"
#include "Creature_Spreading.h"
#include "GameRes.h"
#include "Map_Screen_Helicopter.h"
#include "MapScreen.h"
#include "Quests.h"
#include "Map_Screen_Interface_Border.h"
#include "Cheats.h"
#include "SAM_Sites.h"
#include "Soldier_Macros.h"
#include "Squads.h"
#include "Strategic_Status.h"
#include "Strategic_Town_Loyalty.h"
#include "Assignments.h"
#include "History.h"
#include "VObject.h"
#include "Vehicles.h"
#include "Video.h"
#include "Debug.h"
#include "ScreenIDs.h"
#include "Render_Dirty.h"
#include "VSurface.h"
#include "UILayout.h"
#include <optional>
#include <utility>
#include <string_theory/format>
#include <string_theory/string>


extern BOOLEAN gfDelayAutoResolveStart;


BOOLEAN gfTacticalTraversal = FALSE;
GROUP *gpTacticalTraversalGroup = NULL;
SOLDIERTYPE *gpTacticalTraversalChosenSoldier = NULL;


BOOLEAN gfAutomaticallyStartAutoResolve = FALSE;
BOOLEAN gfAutoAmbush = FALSE;
BOOLEAN gfHighPotentialForAmbush = FALSE;
BOOLEAN gfGotoSectorTransition = FALSE;
BOOLEAN gfEnterAutoResolveMode = FALSE;
BOOLEAN gfEnteringMapScreenToEnterPreBattleInterface = FALSE;
BOOLEAN gfIgnoreAllInput = TRUE;

enum //GraphicIDs for the panel
{
	MAINPANEL,
	TITLE_BAR_PIECE,
	TOP_COLUMN,
	BOTTOM_COLUMN,
	UNINVOLVED_HEADER
};

//The start of the black space
#define TOP_Y							(MAP_SCREEN_Y + 113)
//The end of the black space
#define BOTTOM_Y					(MAP_SCREEN_Y + 349)
//The internal height of the uninvolved panel
#define INTERNAL_HEIGHT		27
//The actual height of the uninvolved panel
#define ACTUAL_HEIGHT			34
//The height of each row
#define ROW_HEIGHT				10

bool gfDisplayPotentialRetreatPaths = false;

// The panel's layout (panel coordinates): the original 261x359 one and the
// 392x539 prebattlepanel.png (the original x1.5: no uninvolved list, the
// participants' table down to the bottom, 98x45 buttons), chosen by the
// panel graphic's width when the interface comes up.
struct PBLayout
{
	INT16 w, h;                   // the panel
	INT16 title_centre, title_y;
	INT16 location_label_right, location_x, location_top, location_h;
	INT16 count_label_right[3], count_label_y;
	INT16 count_x[3], count_w[3], count_top, count_h;
	INT16 button_x[3], button_y, button_w, button_h, button_text_x, button_text_y, button_text_w;
	INT16 rows_top, rows_bottom, row_h; // the participants' table
	INT16 col_x[5], col_w[5];           // name, assignment, condition, HP, EP
	bool  uninvolved;                   // the uninvolved list at the bottom
};

static PBLayout const g_pb_layout_small =
{
	261, 359,
	130, 4,
	65, 70, 17, 0,
	{ 54, 139, 224 }, 38,
	{ 57, 142, 227 }, { 27, 27, 27 }, 36, 0,
	{ 27, 98, 169 }, 54, 65, 30, 8, 7, 51,
	113, 349, 10,
	{ 17, 72, 129, 189, 217 }, { 52, 45, 58, 25, 25 },
	true
};

static PBLayout const g_pb_layout_big =
{
	392, 539,
	196, 6,
	97, 105, 25, 13,
	{ 81, 208, 336 }, 57,
	{ 86, 214, 344 }, { 40, 39, 37 }, 47, 29,
	{ 40, 147, 253 }, 81, 98, 45, 8, 11, 82,
	169, 525, 15,
	{ 25, 110, 193, 287, 329 }, { 77, 77, 86, 34, 34 },
	false
};

static PBLayout const* g_pb = &g_pb_layout_small;
static bool PBBig() { return g_pb == &g_pb_layout_big; }

GROUP *gpBattleGroup = NULL;


static MOUSE_REGION PBInterfaceBlanket;
BOOLEAN gfPreBattleInterfaceActive = FALSE;
static GUIButtonRef iPBButton[3];
static BUTTON_PICS* iPBButtonImage[3];
static SGPVObject*  uiInterfaceImages;
BOOLEAN gfRenderPBInterface;
static BOOLEAN      gfPBButtonsHidden;
BOOLEAN fDisableMapInterfaceDueToBattle = FALSE;

static BOOLEAN gfBlinkHeader;

static UINT32 guiNumInvolved;
static UINT32 guiNumUninvolved;

//SAVE START

//Using the ESC key in the PBI will get rid of the PBI and go back to mapscreen, but
//only if the PBI isn't persistant (!gfPersistantPBI).
BOOLEAN gfPersistantPBI = FALSE;

//Contains general information about the type of encounter the player is faced with.  This
//determines whether or not you can autoresolve the battle or even retreat.  This code
//dictates the header that is used at the top of the PBI.
UINT8 gubEnemyEncounterCode = NO_ENCOUNTER_CODE;

//The autoresolve during tactical battle option needs more detailed information than the
//gubEnemyEncounterCode can provide.  The explicit version contains possibly unique codes
//for reasons not normally used in the PBI.  For example, if we were fighting the enemy
//in a normal situation, then shot at a civilian, the civilians associated with the victim
//would turn hostile, which would disable the ability to autoresolve the battle.
BOOLEAN gubExplicitEnemyEncounterCode = NO_ENCOUNTER_CODE;

//Location of the current battle (determines where the animated icon is blitted) and if the
//icon is to be blitted.
BOOLEAN gfBlitBattleSectorLocator = FALSE;

SGPSector gubPBSector;

BOOLEAN gfCantRetreatInPBI = FALSE;
//SAVE END

BOOLEAN gfUsePersistantPBI;

static void MakeButton(UINT idx, const ST::string& text, GUI_CALLBACK click)
{
	GUIButtonRef const btn = QuickCreateButton(iPBButtonImage[idx], MAP_SCREEN_X + g_pb->button_x[idx],
		MAP_SCREEN_Y + g_pb->button_y, MSYS_PRIORITY_HIGHEST - 2, std::move(click));
	iPBButton[idx] = btn;

	SGPFont const font = PBBig() ? StrategicGeneralFont() : BLOCKFONT;
	btn->SpecifyGeneralTextAttributes(text, font, FONT_BEIGE, 141);
	btn->SpecifyHilitedTextColors(FONT_WHITE, FONT_NEARBLACK);
	btn->SpecifyTextOffsets(g_pb->button_text_x, g_pb->button_text_y, TRUE);
	btn->SpecifyTextWrappedWidth(g_pb->button_text_w);
	btn->AllowDisabledFastHelp();
	btn->Hide();
}


static void AutoResolveBattleCallback(GUI_BUTTON* btn, UINT32 reason);
static void CheckForRobotAndIfItsControlled(void);
static void DoTransitionFromMapscreenToPreBattleInterface(void);
static void GoToSectorCallback(GUI_BUTTON* btn, UINT32 reason);
static void RetreatMercsCallback(GUI_BUTTON* btn, UINT32 reason);


void InitPreBattleInterface(GROUP* const battle_group, bool const persistent_pbi)
{
	// ARM: Feb01/98 - Cancel out of mapscreen movement plotting if PBI subscreen is coming up
	if (bSelectedDestChar != -1 || fPlotForHelicopter)
	{
		AbortMovementPlottingMode();
	}

	if (gfPreBattleInterfaceActive) return;

	gfPersistantPBI = persistent_pbi;
	if (persistent_pbi)
	{
		gfBlitBattleSectorLocator = TRUE;
		gfBlinkHeader = FALSE;

		//	InitializeTacticalStatusAtBattleStart();
		// CJC, Oct 5 98: this is all we should need from InitializeTacticalStatusAtBattleStart()
		if (gubEnemyEncounterCode != BLOODCAT_AMBUSH_CODE        &&
				gubEnemyEncounterCode != ENTERING_BLOODCAT_LAIR_CODE &&
				!CheckFact(FACT_FIRST_BATTLE_FOUGHT, 0))
		{
			SetFactTrue(FACT_FIRST_BATTLE_BEING_FOUGHT);
		}

		// ATE: Added check for persistent_pbi if !battle_group
		// Searched code and saw that this condition only happens for creatures
		if (guiCurrentScreen == GAME_SCREEN && (battle_group || persistent_pbi))
		{
			gpBattleGroup                                = battle_group;
			gfEnteringMapScreen                          = TRUE;
			gfEnteringMapScreenToEnterPreBattleInterface = TRUE;
			gfUsePersistantPBI                           = TRUE;
			return;
		}

		if (gfTacticalTraversal && (battle_group == gpTacticalTraversalGroup || gWorldSector.z > 0))
		{
			return;
		}

		// Reset the help text for mouse regions
		gMapStatusBarsRegion.SetFastHelpText({});

		gfDisplayPotentialRetreatPaths = false;

		gpBattleGroup = battle_group;

		// Calc sector values
		if (battle_group)
		{
			gubPBSector = battle_group->ubSector;
			fMapPanelDirty = TRUE;
		}
		else
		{
			gubPBSector = SGPSector(gubSectorIDOfCreatureAttack);
		}
	}
	else
	{ // Calculate the non-persistent situation
		gfBlinkHeader = TRUE;

		if (HostileCiviliansPresent())
		{ // There are hostile civilians, so no autoresolve allowed.
			gubExplicitEnemyEncounterCode = HOSTILE_CIVILIANS_CODE;
		}
		else if (HostileBloodcatsPresent())
		{ // There are bloodcats in the sector, so no autoresolve allowed
			gubExplicitEnemyEncounterCode = HOSTILE_BLOODCATS_CODE;
		}
		else if (gWorldSector.z != 0)
		{ // We are underground, so no autoresolve allowed
			SECTORINFO const& sector = SectorInfo[gubPBSector.AsByte()]; // XXX Why check surface info when underground?
			if (sector.ubCreaturesInBattle != 0)
			{
				gubExplicitEnemyEncounterCode = FIGHTING_CREATURES_CODE;
			}
			else if (sector.ubAdminsInBattle != 0 || sector.ubTroopsInBattle != 0 || sector.ubElitesInBattle != 0)
			{
				gubExplicitEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
			}
		}
		else if (gubEnemyEncounterCode == ENTERING_ENEMY_SECTOR_CODE ||
				gubEnemyEncounterCode == ENEMY_ENCOUNTER_CODE            ||
				gubEnemyEncounterCode == ENEMY_AMBUSH_CODE               ||
				gubEnemyEncounterCode == ENEMY_INVASION_CODE             ||
				gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE            ||
				gubEnemyEncounterCode == ENTERING_BLOODCAT_LAIR_CODE     ||
				gubEnemyEncounterCode == CREATURE_ATTACK_CODE)
		{ // Use same code
			gubExplicitEnemyEncounterCode = gubEnemyEncounterCode;
		}
		else
		{
			gfBlitBattleSectorLocator = FALSE;
			return;
		}
	}

	fMapScreenBottomDirty = TRUE;
	SGPSector sSector = gubPBSector;
	ChangeSelectedMapSector(sSector);
	RenderMapScreenInterfaceBottom();

	/* If we are currently in tactical, then set the flag to automatically bring
	 * up the mapscreen. */
	if (guiCurrentScreen == GAME_SCREEN) gfEnteringMapScreen = TRUE;

	if (!fShowTeamFlag) ToggleShowTeamsMode();

	/* Define the blanket region to cover all of the other regions used underneath
	 * the panel. */
	// Bottom edge was hardcoded to the old canvas boundary (MAP_SCREEN_Y + 359)
	// -- same bug as the RestoreExternBackgroundRect calls above, just for
	// click-blocking instead of rendering. Extend to the real (now bigger)
	// canvas bottom so clicks can't leak through the gap below the old boundary.
	// Create the panel; its size picks the layout
	uiInterfaceImages = AddVideoObjectFromFile(MLG_PREBATTLEPANEL);
	g_pb = uiInterfaceImages->SubregionProperties(MAINPANEL).usWidth >= g_pb_layout_big.w ?
		&g_pb_layout_big : &g_pb_layout_small;

	MSYS_DefineRegion(&PBInterfaceBlanket, MAP_SCREEN_X + 0, MAP_SCREEN_Y + 0, MAP_SCREEN_X + g_pb->w, MAP_SCREEN_BOTTOM, MSYS_PRIORITY_HIGHEST - 5, 0, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

	// Create the 3 buttons
	iPBButtonImage[0] = LoadButtonImage(INTERFACEDIR "/prebattlebutton.sti", 0, 1);
	iPBButtonImage[1] = UseLoadedButtonImage(iPBButtonImage[0], 0, 1);
	iPBButtonImage[2] = UseLoadedButtonImage(iPBButtonImage[0], 0, 1);
	MakeButton(0, gpStrategicString[STR_PB_AUTORESOLVE_BTN],  AutoResolveBattleCallback);
	MakeButton(1, gpStrategicString[STR_PB_GOTOSECTOR_BTN],   GoToSectorCallback);
	MakeButton(2, gpStrategicString[STR_PB_RETREATMERCS_BTN], RetreatMercsCallback);

	gfPBButtonsHidden = TRUE;

	/* ARM: This must now be set before any calls utilizing the
	 * GetCurrentBattleSectorXYZ() function */
	gfPreBattleInterfaceActive = TRUE;

	CheckForRobotAndIfItsControlled();

	WakeUpAllMercsInSectorUnderAttack();

	// Count the number of players involved or not involved in this battle
	guiNumUninvolved = 0;
	guiNumInvolved   = 0;
	UINT8 group_id           = 0;
	INT8  best_exp_level     = 0;
	bool  use_plural_version = false;
	CFOR_EACH_IN_TEAM(i, OUR_TEAM)
	{
		SOLDIERTYPE const& s = *i;
		if (s.bLife == 0 || s.uiStatusFlags & SOLDIER_VEHICLE) continue;

		if (PlayerMercInvolvedInThisCombat(s))
		{
			if (group_id == 0)
			{ /* Record the first groupID. If there is more than one group in this
				 * battle, we can detect it by comparing the first value with future
				 * values. If we do, then we set a flag which determines whether to use
				 * the singular help text or plural version for the retreat button. */
				group_id = s.ubGroupID;
				if (!gpBattleGroup) gpBattleGroup = GetGroup(group_id);
				if (best_exp_level > s.bExpLevel) best_exp_level = s.bExpLevel; // XXX Determines minimum, not maximum, i.e. stays at 0
				if (s.ubPrevSectorID == 255)
				{ //Not able to retreat (calculate it for group)
					GROUP* const g = GetGroup(group_id);
					Assert(g);
					CalculateGroupRetreatSector(g);
				}
			}
			else if (group_id != s.ubGroupID)
			{
				use_plural_version = true;
			}
			++guiNumInvolved;
		}
		else
		{
			++guiNumUninvolved;
		}
	}

	if (gfPersistantPBI)
	{
		if (!battle_group)
		{ // Creatures are attacking
			gubEnemyEncounterCode = CREATURE_ATTACK_CODE;
		}
		else if (gpBattleGroup->fPlayer)
		{
			if (gubEnemyEncounterCode != BLOODCAT_AMBUSH_CODE &&
					gubEnemyEncounterCode != ENTERING_BLOODCAT_LAIR_CODE)
			{
				UINT8 const n_stationary_enemies = NumStationaryEnemiesInSector(sSector);
				if (n_stationary_enemies != 0)
				{
					gubEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
				}
				else
				{
					gubEnemyEncounterCode = ENEMY_ENCOUNTER_CODE;

					//Don't consider ambushes until the player has reached 25% (normal) progress
					if (gfHighPotentialForAmbush)
					{
						if (Chance(90)) gubEnemyEncounterCode = ENEMY_AMBUSH_CODE;
					}
					else
					{
						UINT8 const n_mobile_enemies = NumMobileEnemiesInSector(sSector);
						UINT8 const n_mercs          = PlayerMercsInSector(sSector);
						if (gfAutoAmbush && n_mobile_enemies > n_mercs)
						{
							gubEnemyEncounterCode = ENEMY_AMBUSH_CODE;
						}
						else if (WhatPlayerKnowsAboutEnemiesInSector(sSector) == KNOWS_NOTHING &&
								CurrentPlayerProgressPercentage() >= 30 - gGameOptions.ubDifficultyLevel * 5)
						{ /* If the enemy outnumbers the players, then there is a small chance
							 * of the enemies ambushing the group */
							if (n_mobile_enemies > n_mercs)
							{
								SECTORINFO const& sector = SectorInfo[sSector.AsByte()];
								if (!(sector.uiFlags & SF_ALREADY_VISITED))
								{
									INT32 chance = (UINT8)(4 - best_exp_level + 2 * gGameOptions.ubDifficultyLevel + CurrentPlayerProgressPercentage() / 10);
									if (sector.uiFlags & SF_ENEMY_AMBUSH_LOCATION) chance += 20;
									if (gfCantRetreatInPBI)                        chance += 20;
									if ((INT32)PreRandom(100) < chance)
									{
										gubEnemyEncounterCode = ENEMY_AMBUSH_CODE;
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{ // Are enemies invading a town, or just encountered the player.
		  // SAM sites not in towns will also be considered to be important.
			gubEnemyEncounterCode =
				((GetTownIdForSector(sSector) != BLANK_SECTOR)
				  || IsThisSectorASAMSector(sSector))
				? ENEMY_INVASION_CODE : ENEMY_ENCOUNTER_CODE;
		}
	}

	gfHighPotentialForAmbush = FALSE;

	if (gfAutomaticallyStartAutoResolve)
	{
		DisableButton(iPBButton[1]);
		DisableButton(iPBButton[2]);
	}

	gfRenderPBInterface = TRUE;
	MSYS_SetCurrentCursor(CURSOR_NORMAL);
	StopTimeCompression();

	HideAllBoxes();
	fShowAssignmentMenu = FALSE;
	fShowContractMenu   = FALSE;
	DisableTeamInfoPanels();
	if (giMapContractButton) giMapContractButton->Hide();
	if (giCharInfoButton[0]) giCharInfoButton[0]->Hide();
	if (giCharInfoButton[1]) giCharInfoButton[1]->Hide();

	if (gubEnemyEncounterCode == ENEMY_ENCOUNTER_CODE)
	{ /* We know how many enemies are here, so until we leave the sector, we will
		 * continue to display the value. The flag will get cleared when time
		 * advances after the fEnemyInSector flag is clear. */
		SECTORINFO& sector = SectorInfo[sSector.AsByte()];

		/* Always use these 2 statements together. Without setting the boolean, the
		 * flag will never be cleaned up */
		sector.uiFlags |= SF_PLAYER_KNOWS_ENEMIES_ARE_HERE;
		gfResetAllPlayerKnowsEnemiesFlags = TRUE;
	}

	/* Set up fast help for buttons depending on the state of the button, and
	 * disable buttons when necessary. */
	if (gfPersistantPBI)
	{
		ST::string autoresolve_help;
		switch (gubEnemyEncounterCode)
		{
			case ENTERING_ENEMY_SECTOR_CODE:
			case ENTERING_BLOODCAT_LAIR_CODE:
				// Don't allow autoresolve for player initiated invasion battle types
				DisableButton(iPBButton[0]);
				autoresolve_help = gpStrategicString[STR_PB_DISABLED_AUTORESOLVE_FASTHELP];
				break;

			case ENEMY_AMBUSH_CODE:
			case BLOODCAT_AMBUSH_CODE:
				// Don't allow autoresolve for ambushes
				DisableButton(iPBButton[0]);
				autoresolve_help = gzNonPersistantPBIText[3];
				break;

			default:
				autoresolve_help = gpStrategicString[STR_PB_AUTORESOLVE_FASTHELP];
				break;
		}
		iPBButton[0]->SetFastHelpText(autoresolve_help);
		iPBButton[1]->SetFastHelpText(gpStrategicString[STR_PB_GOTOSECTOR_FASTHELP]);
		if (gfAutomaticallyStartAutoResolve) DisableButton(iPBButton[1]);

		ST::string retreat_help;
		if (gfAutomaticallyStartAutoResolve               ||
				gfCantRetreatInPBI                            ||
				gubEnemyEncounterCode == ENEMY_AMBUSH_CODE    ||
				gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE ||
				gubEnemyEncounterCode == CREATURE_ATTACK_CODE)
		{
			gfCantRetreatInPBI = FALSE;
			DisableButton(iPBButton[2]);
			retreat_help = gzNonPersistantPBIText[9];
		}
		else
		{
			retreat_help =
				use_plural_version ? gpStrategicString[STR_BP_RETREATPLURAL_FASTHELP] :
				gpStrategicString[STR_BP_RETREATSINGLE_FASTHELP];
		}
		iPBButton[2]->SetFastHelpText(retreat_help);
	}
	else
	{ /* Use the explicit encounter code to determine what gets disable and the
		 * associated help text that is used. */

		/* First of all, the retreat button is always disabled seeing a battle is in
		 * progress. */
		DisableButton(iPBButton[2]);
		iPBButton[2]->SetFastHelpText(gzNonPersistantPBIText[0]);
		iPBButton[1]->SetFastHelpText(gzNonPersistantPBIText[1]);
		ST::string help;
		switch (gubExplicitEnemyEncounterCode)
		{
			case CREATURE_ATTACK_CODE:
			case ENEMY_ENCOUNTER_CODE:
			case ENEMY_INVASION_CODE:         help = gzNonPersistantPBIText[2]; goto set_help;
			case ENTERING_ENEMY_SECTOR_CODE:  help = gzNonPersistantPBIText[3]; goto disable_set_help;
			case ENEMY_AMBUSH_CODE:           help = gzNonPersistantPBIText[4]; goto disable_set_help;
			case FIGHTING_CREATURES_CODE:     help = gzNonPersistantPBIText[5]; goto disable_set_help;
			case HOSTILE_CIVILIANS_CODE:      help = gzNonPersistantPBIText[6]; goto disable_set_help;
			case HOSTILE_BLOODCATS_CODE:
			case BLOODCAT_AMBUSH_CODE:
			case ENTERING_BLOODCAT_LAIR_CODE: help = gzNonPersistantPBIText[7]; goto disable_set_help;

disable_set_help:
				DisableButton(iPBButton[0]);
set_help:
				iPBButton[0]->SetFastHelpText(help);
		}
	}

	// Disable the options button when the auto resolve screen comes up
	EnableDisAbleMapScreenOptionsButton(FALSE);

	SetMusicMode(MUSIC_TACTICAL_ENEMYPRESENT);

	DoTransitionFromMapscreenToPreBattleInterface();
}


static void DoTransitionFromMapscreenToPreBattleInterface(void)
{
	UINT32 uiStartTime, uiCurrTime;
	INT32 iPercentage, iFactor;
	UINT32 uiTimeRange;
	INT16 sStartLeft, sEndLeft, sStartTop, sEndTop;
	INT32 iLeft, iTop, iWidth, iHeight;
	BOOLEAN fEnterAutoResolveMode = FALSE;

	PauseTime( FALSE );

	iWidth  = g_pb->w;
	iHeight = g_pb->h;

	uiTimeRange = 1000;
	iPercentage = 0;
	uiStartTime = GetClock();

	GetScreenXYFromMapXY(gubPBSector, &sStartLeft, &sStartTop);
	sStartLeft += MAP_GRID_X / 2;
	sStartTop += MAP_GRID_Y / 2;
	// The last frame ends at sEnd - 1 (see iLeft / iTop below): size / 2 + 1
	// puts it at the panel's corner (130 + 1, 179 + 1 for the 261x359 panel).
	sEndLeft = MAP_SCREEN_X + iWidth  / 2 + 1;
	sEndTop  = MAP_SCREEN_Y + iHeight / 2 + 1;

	//save the mapscreen buffer
	BltVideoSurface(guiEXTRABUFFER, FRAME_BUFFER, 0, 0, NULL);

	if( gfEnterAutoResolveMode )
	{ //If we are intending on immediately entering autoresolve, change the global flag so that it will actually
		//render the interface once.  If gfEnterAutoResolveMode is clear, then RenderPreBattleInterface() won't do
		//anything.
		fEnterAutoResolveMode = TRUE;
		gfEnterAutoResolveMode = FALSE;
	}
	//render the prebattle interface
	RenderPreBattleInterface();

	gfIgnoreAllInput = TRUE;

	if( fEnterAutoResolveMode )
	{ //Change it back
		gfEnterAutoResolveMode = TRUE;
	}

	{ // the buttons' row
		INT16 const bx = MAP_SCREEN_X + g_pb->button_x[0];
		INT16 const by = MAP_SCREEN_Y + g_pb->button_y;
		INT16 const bw = g_pb->button_x[2] + g_pb->button_w - g_pb->button_x[0];
		INT16 const bh = g_pb->button_h + 2;
		BlitBufferToBuffer( guiSAVEBUFFER, FRAME_BUFFER, bx, by, bw, bh );
		RenderButtons();
		BlitBufferToBuffer( FRAME_BUFFER, guiSAVEBUFFER, bx, by, bw, bh );
	}
	gfRenderPBInterface = TRUE;

	//hide the prebattle interface
	BlitBufferToBuffer( guiEXTRABUFFER, FRAME_BUFFER, MAP_SCREEN_X, MAP_SCREEN_Y, iWidth, iHeight );
	PlayJA2SampleFromFile(SOUNDSDIR "/laptop power up (8-11).wav", HIGHVOLUME, 1, MIDDLEPAN);
	InvalidateScreen();

	SGPBox const PBIRect = { MAP_SCREEN_X, MAP_SCREEN_Y, (UINT16)iWidth, (UINT16)iHeight };
	while( iPercentage < 100  )
	{
		uiCurrTime = GetClock();
		iPercentage = (uiCurrTime-uiStartTime) * 100 / uiTimeRange;
		iPercentage = std::min(iPercentage, 100);

		//Factor the percentage so that it is modified by a gravity falling acceleration effect.
		iFactor = (iPercentage - 50) * 2;
		if( iPercentage < 50 )
			iPercentage = (UINT32)(iPercentage + iPercentage * iFactor * 0.01 + 0.5);
		else
			iPercentage = (UINT32)(iPercentage + (100-iPercentage) * iFactor * 0.01 + 0.05);

		//Calculate the center point.
		iLeft = sStartLeft - (sStartLeft-sEndLeft+1) * iPercentage / 100;
		if( sStartTop > sEndTop )
			iTop = sStartTop - (sStartTop-sEndTop+1) * iPercentage / 100;
		else
			iTop = sStartTop + (sEndTop-sStartTop+1) * iPercentage / 100;

		// Never left of / above the screen: BltStretchVideoSurface() does not
		// clip, a negative x wrapped to 65535 drew the panel across the rows
		// (and the restore below skipped it, leaving a copy behind).
		SGPBox const DstRect =
		{
			(UINT16)(std::max(0, iLeft - iWidth  * iPercentage / 200)),
			(UINT16)(std::max(0, iTop  - iHeight * iPercentage / 200)),
			(UINT16)(std::max(1, iWidth  * iPercentage / 100)),
			(UINT16)(std::max(1, iHeight * iPercentage / 100))
		};

		BltStretchVideoSurface(FRAME_BUFFER, guiSAVEBUFFER, &PBIRect, &DstRect);

		InvalidateScreen();
		RefreshScreen();

		//Restore the previous rect.
		BlitBufferToBuffer(guiEXTRABUFFER, FRAME_BUFFER, DstRect.x, DstRect.y, DstRect.w + 1, DstRect.h + 1);
	}
	BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);
}

void KillPreBattleInterface()
{
	if( !gfPreBattleInterfaceActive )
		return;

	fDisableMapInterfaceDueToBattle = FALSE;
	MSYS_RemoveRegion( &PBInterfaceBlanket );

	//The panel
	DeleteVideoObject(uiInterfaceImages);

	//The 3 buttons
	RemoveButton( iPBButton[0] );
	RemoveButton( iPBButton[1] );
	RemoveButton( iPBButton[2] );
	UnloadButtonImage( iPBButtonImage[0] );
	UnloadButtonImage( iPBButtonImage[1] );
	UnloadButtonImage( iPBButtonImage[2] );

	/*
	MSYS_RemoveRegion( &InvolvedRegion );
	if( guiNumUninvolved )
		MSYS_RemoveRegion( &UninvolvedRegion );
	*/

	gfPreBattleInterfaceActive = FALSE;

	//UpdateCharRegionHelpText( );

	// re draw affected regions
	fMapPanelDirty = TRUE;
	fTeamPanelDirty = TRUE;
	fMapScreenBottomDirty = TRUE;
	fCharacterInfoPanelDirty = TRUE;
	gfDisplayPotentialRetreatPaths = false;

	//Enable the options button when the auto resolve  screen comes up
	EnableDisAbleMapScreenOptionsButton( TRUE );

	ColorFillVideoSurfaceArea( guiSAVEBUFFER, 0, 0, g_pb->w, g_pb->h, 0 );

	EnableTeamInfoPanels();
	if (giMapContractButton) giMapContractButton->Show();
	if (giCharInfoButton[0]) giCharInfoButton[0]->Show();
	if (giCharInfoButton[1]) giCharInfoButton[1]->Show();
}


static void RenderPBHeader(INT32* piX, INT32* piWidth)
{
	INT32 x, width;
	UINT8 const foreground =
		!gfBlinkHeader             ? FONT_BEIGE :
		GetJA2Clock() % 1000 < 667 ? FONT_WHITE :
		FONT_LTRED;
	SGPFont const font = PBBig() ? FONT14ARIAL : FONT10ARIALBOLD;
	SetFontAttributes(font, foreground);
	ST::string str;
	if( !gfPersistantPBI )
	{
		str = gzNonPersistantPBIText[8];
	}
	else switch( gubEnemyEncounterCode )
	{
		case ENEMY_INVASION_CODE:
			str = gpStrategicString[STR_PB_ENEMYINVASION_HEADER];
			break;
		case ENEMY_ENCOUNTER_CODE:
			str = gpStrategicString[STR_PB_ENEMYENCOUNTER_HEADER];
			break;
		case ENEMY_AMBUSH_CODE:
			str = gpStrategicString[STR_PB_ENEMYAMBUSH_HEADER];
			gfBlinkHeader = TRUE;
			break;
		case ENTERING_ENEMY_SECTOR_CODE:
			str = gpStrategicString[STR_PB_ENTERINGENEMYSECTOR_HEADER];
			break;
		case CREATURE_ATTACK_CODE:
			str = gpStrategicString[STR_PB_CREATUREATTACK_HEADER];
			gfBlinkHeader = TRUE;
			break;
		case BLOODCAT_AMBUSH_CODE:
			str = gpStrategicString[STR_PB_BLOODCATAMBUSH_HEADER];
			gfBlinkHeader = TRUE;
			break;
		case ENTERING_BLOODCAT_LAIR_CODE:
			str = gpStrategicString[STR_PB_ENTERINGBLOODCATLAIR_HEADER];
			break;
	}
	width = StringPixLength( str, font );
	x = g_pb->title_centre - width / 2;
	MPrint(MAP_SCREEN_X + x, MAP_SCREEN_Y + g_pb->title_y, str);
	InvalidateRegion( MAP_SCREEN_X + 0, MAP_SCREEN_Y + 0, MAP_SCREEN_X + g_pb->w, MAP_SCREEN_Y + g_pb->title_y + GetFontHeight(font) + 2 );
	*piX = x;
	*piWidth = width;
}


static void PrintConfined(INT32 x, INT32 y, INT32 max_w, const ST::string& str)
{
	SGPFont  font  = PBBig() ? StrategicGeneralFont() : BLOCKFONT;
	INT32 w     = StringPixLength(str, font);
	if (w >= max_w && !PBBig())
	{
		font = BLOCKFONTNARROW;
		w    = StringPixLength(str, font);
	}
	SetFont(font);
	MPrint(MAP_SCREEN_X + x - w, MAP_SCREEN_Y + y, str);
}


static void MPrintCentered(INT32 x, INT32 y, INT32 w, const ST::string& str)
{
	x += (w - StringPixLength(str, FontDefault)) / 2;
	MPrint(MAP_SCREEN_X + x, MAP_SCREEN_Y + y, str);
}


static ST::string GetSoldierConditionInfo(const SOLDIERTYPE& s);


void RenderPreBattleInterface()
{
	ST::string str;

	/* If the cursor is inside the rectangle consisting of the rectangle button,
	 * then we set up the variables so that the retreat arrows get drawn in the
	 * mapscreen. */
	GUI_BUTTON const& retreat = *iPBButton[2];
	if (retreat.Enabled())
	{
		bool const mouse_in_reatread_button_area =
			retreat.X() <= gusMouseXPos && gusMouseXPos <= retreat.BottomRightX() &&
			retreat.Y() <= gusMouseYPos && gusMouseYPos <= retreat.BottomRightY();
		if (gfDisplayPotentialRetreatPaths != mouse_in_reatread_button_area)
		{
			gfDisplayPotentialRetreatPaths = mouse_in_reatread_button_area;
			fMapPanelDirty                 = TRUE;
		}
	}

	INT32 x;
	INT32 width;
	if (gfRenderPBInterface)
	{
		gfRenderPBInterface = FALSE;

		SGPVSurface* const dst = guiSAVEBUFFER;
		SetFontDestBuffer(dst);

		if (gfPBButtonsHidden)
		{
			gfPBButtonsHidden = FALSE;
			ShowButton(iPBButton[0]);
			ShowButton(iPBButton[1]);
			ShowButton(iPBButton[2]);
		}
		else
		{
			MarkAButtonDirty(iPBButton[0]);
			MarkAButtonDirty(iPBButton[1]);
			MarkAButtonDirty(iPBButton[2]);
		}

		SGPVObject const* const vo = uiInterfaceImages;
		// Main panel
		BltVideoObject(dst, vo, MAINPANEL, MAP_SCREEN_X + 0, MAP_SCREEN_Y + 0);
		PBLayout const& L = *g_pb;
		// Main title
		RenderPBHeader(&x, &width);

		// Of prebattlepanel.sti only the main panel (sub-image 0) is drawn, per
		// user request: not the title bar pieces (1), the participants' and the
		// uninvolved list's column lines (2, 3) and the "uninvolved" header (4).
#if 0
		// Draw the title bars up to the text
		for (INT32 i = x - 12; i > 20; i -= 10)
		{
			BltVideoObject(dst, vo, TITLE_BAR_PIECE, MAP_SCREEN_X + i, MAP_SCREEN_Y + 6);
		}
		for (INT32 i = x + width + 2; i < 231; i += 10)
		{
			BltVideoObject(dst, vo, TITLE_BAR_PIECE, MAP_SCREEN_X + i, MAP_SCREEN_Y + 6);
		}

		{ INT32 const y = BOTTOM_Y - ACTUAL_HEIGHT - ROW_HEIGHT * std::max(guiNumUninvolved, 1U);
			BltVideoObject(dst, vo, UNINVOLVED_HEADER, MAP_SCREEN_X + 8, y);
		}
#endif

		SetFontForeground(FONT_BEIGE);
		// big panel: the labels middled on the location bar / the counters
		INT16 const location_y = L.location_h ? L.location_top + (L.location_h - GetFontHeight(StrategicGeneralFont())) / 2 : L.location_top;
		PrintConfined(L.location_label_right, location_y, L.location_label_right - 1, gpStrategicString[STR_PB_LOCATION]);

		ST::string encounter =
			gubEnemyEncounterCode != CREATURE_ATTACK_CODE        ? gpStrategicString[STR_PB_ENEMIES] :
			gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE || // XXX case is unreachable, because of != above
			gubEnemyEncounterCode == ENTERING_BLOODCAT_LAIR_CODE ? gpStrategicString[STR_PB_BLOODCATS] :
			gpStrategicString[STR_PB_CREATURES];
		INT16 const label_y = L.count_h ? L.count_top + (L.count_h - GetFontHeight(StrategicGeneralFont())) / 2 : L.count_label_y;
		INT16 const label_w = PBBig() ? 78 : 52;
		PrintConfined(L.count_label_right[0], label_y, label_w, encounter);
		PrintConfined(L.count_label_right[1], label_y, label_w, gpStrategicString[STR_PB_MERCS]);
		PrintConfined(L.count_label_right[2], label_y, label_w, gpStrategicString[STR_PB_MILITIA]);

#if 0 // sub-images 2 and 3, see above
		// Draw the bottom columns
		for (INT32 i = 0; i < (INT32)std::max(guiNumUninvolved, 1U); ++i)
		{
			INT32 const y = BOTTOM_Y - ROW_HEIGHT * (i + 1) + 1;
			BltVideoObject(dst, vo, BOTTOM_COLUMN, MAP_SCREEN_X + 161, y);
		}

		for (INT32 i = 0; i < (INT32)(21 - std::max(guiNumUninvolved, 1U)); ++i)
		{
			INT32 const y = TOP_Y + ROW_HEIGHT * i;
			BltVideoObject(dst, vo, TOP_COLUMN, MAP_SCREEN_X + 186, y);
		}
#endif

		// Location
		SGPFont const location_font = PBBig() ? FONT12ARIAL : FONT10ARIAL;
		SetFontAttributes(location_font, FONT_YELLOW);
		ST::string sector_name = GetSectorIDString(gubPBSector, TRUE);
		INT16 const sector_y = L.location_h ? L.location_top + (L.location_h - GetFontHeight(location_font)) / 2 : L.location_top;
		MPrint(MAP_SCREEN_X + L.location_x, MAP_SCREEN_Y + sector_y, ST::format("{} {}", gpStrategicString[STR_PB_SECTOR], sector_name));

		SGPFont const count_font = PBBig() ? FONT16ARIAL : FONT14ARIAL;
		SetFont(count_font);
		INT16 const count_y = L.count_h ? L.count_top + (L.count_h - GetFontHeight(count_font)) / 2 : L.count_top;
		// Enemy
		ST::string enemies;
		if (gubEnemyEncounterCode == CREATURE_ATTACK_CODE        ||
			gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE        ||
			gubEnemyEncounterCode == ENTERING_BLOODCAT_LAIR_CODE ||
			WhatPlayerKnowsAboutEnemiesInSector(gubPBSector) != KNOWS_HOW_MANY)
		{ // Don't know how many
			enemies = "?";
		}
		else
		{ // Know exactly how many
			INT32 const n = NumEnemiesInSector(gubPBSector);
			str = ST::format("{}", n);
			enemies = str;
		}
		MPrintCentered(L.count_x[0], count_y, L.count_w[0], enemies);
		// Player
		str = ST::format("{}", guiNumInvolved);
		MPrintCentered(L.count_x[1], count_y, L.count_w[1], str);
		// Militia
		str = ST::format("{}", CountAllMilitiaInSector(gubPBSector));
		MPrintCentered(L.count_x[2], count_y, L.count_w[2], str);
		SetFontShadow(FONT_NEARBLACK);

		SetFont(StrategicGeneralFont());

		// Print the participants of the battle
		// |  NAME  | ASSIGN |  COND  |   HP   |   BP   |
		{ // big panel: the text middled in its 15 px row
			INT32 y = L.rows_top + 1 + (PBBig() ? (L.row_h - GetFontHeight(StrategicGeneralFont())) / 2 : 0);
			CFOR_EACH_IN_TEAM(i, OUR_TEAM)
			{
				SOLDIERTYPE const& s = *i;
				if (s.bLife == 0)                       continue;
				if (s.uiStatusFlags & SOLDIER_VEHICLE)  continue;
				if (!PlayerMercInvolvedInThisCombat(s)) continue;
				// the big panel's table ends at its bottom
				if (PBBig() && y + L.row_h > L.rows_bottom) break;

				// Name
				MPrintCentered(L.col_x[0], y, L.col_w[0], s.name);
				// Assignment
				str = GetMapscreenMercAssignmentString(s);
				MPrintCentered(L.col_x[1], y, L.col_w[1], str);
				// Condition
				MPrintCentered(L.col_x[2], y, L.col_w[2], GetSoldierConditionInfo(s));
				// HP
				str = ST::format("{}%", s.bLife * 100 / s.bLifeMax);
				MPrintCentered(L.col_x[3], y, L.col_w[3], str);
				// BP
				str = ST::format("{}%", s.bBreath);
				MPrintCentered(L.col_x[4], y, L.col_w[4], str);

				y += L.row_h;
			}
		}

		// Print the uninvolved members of the battle (not on the big panel)
		// |  NAME  | ASSIGN |  LOC   |  DEST  |  DEP   |
		if (!L.uninvolved)
		{
		}
		else if (guiNumUninvolved == 0)
		{
			MPrintCentered(17, BOTTOM_Y - MAP_SCREEN_Y - ROW_HEIGHT + 2, 52, gpStrategicString[STR_PB_NONE]);
		}
		else
		{
			INT32 y = BOTTOM_Y - MAP_SCREEN_Y - ROW_HEIGHT * guiNumUninvolved + 2;
			CFOR_EACH_IN_TEAM(i, OUR_TEAM)
			{
				SOLDIERTYPE const& s = *i;
				if (s.bLife == 0)                      continue;
				if (s.uiStatusFlags & SOLDIER_VEHICLE) continue;
				if (PlayerMercInvolvedInThisCombat(s)) continue;

				// Name
				MPrintCentered( 17, y, 52, s.name);
				// Assignment
				str = GetMapscreenMercAssignmentString(s);
				MPrintCentered( 72, y, 54, str);
				// Location
				str = GetMapscreenMercLocationString(s);
				MPrintCentered(128, y, 33, str);
				// Destination
				str = GetMapscreenMercDestinationString(s);
				if (!str.empty()) MPrintCentered(164, y, 41, str);
				// Departure
				str = GetMapscreenMercDepartureString(s, 0);
				MPrintCentered(208, y, 34, str);
				y += ROW_HEIGHT;
			}
		}

		MarkAllBoxesAsAltered();
		// Height was hardcoded to 359 (old 640x480 canvas boundary) -- same bug
		// as RenderMapRegionBackground()/RenderTeamRegionBackground() in
		// MapScreen.cc, clipping the restore before it reached the bottom of the
		// now-taller left column (mbs.sti / newgoldpiece3.sti). Restore the full
		// remaining canvas height instead.
		RestoreExternBackgroundRect(MAP_SCREEN_X, MAP_SCREEN_Y, L.w, MAP_SCREEN_HEIGHT);

		// Restore font destinanation buffer to the frame buffer
		SetFontDestBuffer(FRAME_BUFFER);
	}
	else if (gfBlinkHeader)
	{
		RenderPBHeader(&x, &width); // The text is important enough to blink
	}

	if (gfEnterAutoResolveMode)
	{
		gfEnterAutoResolveMode = FALSE;
		EnterAutoResolveMode(gubPBSector);
	}

	gfIgnoreAllInput = FALSE;
}


static void AutoResolveBattleCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if( !gfIgnoreAllInput )
	{
		if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
		{
				if( _KeyDown( ALT ) && CHEATER_CHEAT_LEVEL() )
				{
					if( !gfPersistantPBI )
					{
						return;
					}
					PlayJA2Sample(EXPLOSION_1, HIGHVOLUME, 1, MIDDLEPAN);
					gStrategicStatus.usPlayerKills += NumEnemiesInSector(gubPBSector);
					EliminateAllEnemies(gubPBSector);
					SetMusicMode( MUSIC_TACTICAL_VICTORY );
					btn->uiFlags &= ~BUTTON_CLICKED_ON;
					btn->Draw();
					InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
					RefreshScreen();
					KillPreBattleInterface();
					StopTimeCompression();
					SetMusicMode( MUSIC_TACTICAL_NOTHING );
					return;
				}
			gfEnterAutoResolveMode = TRUE;
		}
	}
}


static void ClearMovementForAllInvolvedPlayerGroups(void);
static void PutNonSquadMercsInBattleSectorOnSquads(BOOLEAN fExitVehicles);


static void GoToSectorCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if( !gfIgnoreAllInput )
	{
		if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
		{
				if( _KeyDown( ALT ) && CHEATER_CHEAT_LEVEL() )
				{
					if( !gfPersistantPBI )
					{
						return;
					}
					PlayJA2Sample(EXPLOSION_1, HIGHVOLUME, 1, MIDDLEPAN);
					gStrategicStatus.usPlayerKills += NumEnemiesInSector(gubPBSector);
					EliminateAllEnemies(gubPBSector);
					SetMusicMode( MUSIC_TACTICAL_VICTORY );
					btn->uiFlags &= ~BUTTON_CLICKED_ON;
					btn->Draw();
					InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
					RefreshScreen();
					KillPreBattleInterface();
					StopTimeCompression();
					SetMusicMode( MUSIC_TACTICAL_NOTHING );
					return;
				}
			if( gfPersistantPBI && gpBattleGroup && gpBattleGroup->fPlayer &&
					gubEnemyEncounterCode != ENEMY_AMBUSH_CODE &&
					gubEnemyEncounterCode != CREATURE_ATTACK_CODE &&
					gubEnemyEncounterCode != BLOODCAT_AMBUSH_CODE )
			{
				gfEnterTacticalPlacementGUI = TRUE;
			}
			btn->uiFlags &= ~BUTTON_CLICKED_ON;
			btn->Draw();
			InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
			RefreshScreen();
			SGPSector sector = gubPBSector;
			// NOTE: remove this zeroing if we ever want to support underground auto battle resolution
			sector.z = 0;
			if (sector == gWorldSector)
			{
				gfGotoSectorTransition = TRUE;
			}

			// first time going to the sector?
			if( gfPersistantPBI )
			{
				// put everyone on duty, and remove mercs from vehicles, too
				PutNonSquadMercsInBattleSectorOnSquads( TRUE );

				// we nuke the groups existing route & destination in advance
				ClearMovementForAllInvolvedPlayerGroups( );
			}
			else
			{ //Clear the battlegroup pointer.
				gpBattleGroup = NULL;
			}

			// must come AFTER anything that needs gpBattleGroup, as it wipes it out
			SetCurrentWorldSector(gubPBSector);

			KillPreBattleInterface();
		}
	}
}


static void RetreatMercsCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if( !gfIgnoreAllInput )
	{
		if( reason & MSYS_CALLBACK_REASON_POINTER_UP )
		{
			// get them outta here!
			RetreatAllInvolvedPlayerGroups();

			// NOTE: this code assumes you can never retreat while underground
			HandleLoyaltyImplicationsOfMercRetreat(RETREAT_PBI, gubPBSector);
			if (CountAllMilitiaInSector(gubPBSector))
			{ //Mercs retreat, but enemies still need to fight the militia
				gfEnterAutoResolveMode = TRUE;
				return;
			}

			//Warp time by 5 minutes so that player can't just go back into the sector he left.
			WarpGameTime( 300, WARPTIME_NO_PROCESSING_OF_EVENTS );
			ResetMovementForEnemyGroupsInLocation();

			btn->uiFlags &= ~BUTTON_CLICKED_ON;
			btn->Draw();
			InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
			RefreshScreen();
			KillPreBattleInterface();
			StopTimeCompression();
			gpBattleGroup = NULL;
			gfBlitBattleSectorLocator = FALSE;

			SetMusicMode( MUSIC_TACTICAL_NOTHING );
		}
	}
}

enum
{
	COND_EXCELLENT,
	COND_GOOD,
	COND_FAIR,
	COND_WOUNDED,
	COND_FATIGUED,
	COND_BLEEDING,
	COND_UNCONCIOUS,
	COND_DYING,
	COND_DEAD
};


static ST::string GetSoldierConditionInfo(const SOLDIERTYPE& s)
{
	// Go from the worst condition to the best
	return
		s.bLife == 0                         ? pConditionStrings[COND_DEAD]       :
		s.bLife < OKLIFE && s.bBleeding != 0 ? pConditionStrings[COND_DYING]      :
		s.bBreath < OKBREATH && s.bCollapsed ? pConditionStrings[COND_UNCONCIOUS] :
		s.bBleeding > MIN_BLEEDING_THRESHOLD ? pConditionStrings[COND_BLEEDING]   :
		s.bLife * 100 < s.bLifeMax * 50      ? pConditionStrings[COND_WOUNDED]    :
		s.bBreath < 50                       ? pConditionStrings[COND_FATIGUED]   :
		s.bLife * 100 < s.bLifeMax * 67      ? pConditionStrings[COND_FAIR]       :
		s.bLife * 100 < s.bLifeMax * 86      ? pConditionStrings[COND_GOOD]       :
		pConditionStrings[COND_EXCELLENT];
}


void ActivatePreBattleAutoresolveAction()
{
	if (iPBButton[0]->Enabled())
	{ //Feign call the autoresolve button using the callback
		AutoResolveBattleCallback(iPBButton[0], MSYS_CALLBACK_REASON_POINTER_UP);
	}
}

void ActivatePreBattleEnterSectorAction()
{
	if (iPBButton[1]->Enabled())
	{ //Feign call the enter sector button using the callback
		GoToSectorCallback(iPBButton[1], MSYS_CALLBACK_REASON_POINTER_UP);
	}
}

void ActivatePreBattleRetreatAction()
{
	if (iPBButton[2]->Enabled())
	{ //Feign call the retreat button using the callback
		RetreatMercsCallback(iPBButton[2], MSYS_CALLBACK_REASON_POINTER_UP);
	}
}


static void ActivateAutomaticAutoResolveStart()
{
	iPBButton[0]->uiFlags |= BUTTON_CLICKED_ON;
	gfIgnoreAllInput = FALSE;
	AutoResolveBattleCallback(iPBButton[0], MSYS_CALLBACK_REASON_POINTER_UP);
}


void CalculateNonPersistantPBIInfo(void)
{
	//We need to set up the non-persistant PBI
	if (!gfBlitBattleSectorLocator || gubPBSector != gWorldSector)
	{ //Either the locator isn't on or the locator info is in a different sector

		//Calculated the encounter type
		gubEnemyEncounterCode = NO_ENCOUNTER_CODE;
		gubExplicitEnemyEncounterCode = NO_ENCOUNTER_CODE;
		if( HostileCiviliansPresent() )
		{ //There are hostile civilians, so no autoresolve allowed.
			gubExplicitEnemyEncounterCode = HOSTILE_CIVILIANS_CODE;
		}
		else if( HostileBloodcatsPresent() )
		{ //There are bloodcats in the sector, so no autoresolve allowed
			gubExplicitEnemyEncounterCode = HOSTILE_BLOODCATS_CODE;
		}
		else if (gWorldSector.z)
		{
			UNDERGROUND_SECTORINFO *pSector = FindUnderGroundSector(gWorldSector);
			Assert( pSector );
			if( pSector->ubCreaturesInBattle )
			{
				gubExplicitEnemyEncounterCode = FIGHTING_CREATURES_CODE;
			}
			else if( pSector->ubAdminsInBattle || pSector->ubTroopsInBattle || pSector->ubElitesInBattle )
			{
				gubExplicitEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
				gubEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
			}
		}
		else
		{
			SECTORINFO *pSector = &SectorInfo[gWorldSector.AsByte()];
			Assert( pSector );
			if( pSector->ubCreaturesInBattle )
			{
				gubExplicitEnemyEncounterCode = FIGHTING_CREATURES_CODE;
			}
			else if( pSector->ubAdminsInBattle || pSector->ubTroopsInBattle || pSector->ubElitesInBattle )
			{
				gubExplicitEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
				gubEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
			}
		}
		if( gubExplicitEnemyEncounterCode != NO_ENCOUNTER_CODE )
		{	//Set up the location as well as turning on the blit flag.
			gubPBSector = gWorldSector;
			gfBlitBattleSectorLocator = TRUE;
		}
	}
}


static void PutNonSquadMercsInPlayerGroupOnSquads(GROUP* pGroup, BOOLEAN fExitVehicles);


static void PutNonSquadMercsInBattleSectorOnSquads(BOOLEAN fExitVehicles)
{
	// IMPORTANT: Have to do this by group, so everyone inside vehicles gets assigned to the same squad.  Needed for
	// the tactical placement interface to work in case of simultaneous multi-vehicle arrivals!
	FOR_EACH_GROUP_SAFE(i)
	{
		GROUP& g = *i;
		if (!PlayerGroupInvolvedInThisCombat(g)) continue;

		// the helicopter group CAN be involved, if it's on the ground, in which case everybody must get out of it
		if (IsGroupTheHelicopterGroup(g))
		{
			// only happens if chopper is on the ground...
			Assert( !fHelicopterIsAirBorne );

			// put anyone in it into movement group
			MoveAllInHelicopterToFootMovementGroup( );
		}
		else
		{
			PutNonSquadMercsInPlayerGroupOnSquads(&g, fExitVehicles);
		}
	}
}


static void PutNonSquadMercsInPlayerGroupOnSquads(GROUP* const pGroup, const BOOLEAN fExitVehicles)
{
	std::optional<INT8> bUniqueVehicleSquad;
	if (pGroup->fVehicle)
	{
		// put these guys on their own squad (we need to return their group ID, and can only return one, so they need a unique one
		bUniqueVehicleSquad = GetFirstEmptySquad();
	}

	PLAYERGROUP* next;
	for (PLAYERGROUP* p = pGroup->pPlayerList; p; p = next)
	{
		Assert(p->pSoldier);
		SOLDIERTYPE& s = *p->pSoldier;

		// store ptr to next soldier in group, once removed from group, his info will get memfree'd!
		next = p->next;

		if (!s.bActive || s.bLife == 0 || s.uiStatusFlags & SOLDIER_VEHICLE) continue;

		if (!PlayerMercInvolvedInThisCombat(s) || s.bAssignment < ON_DUTY) continue;
		// if involved, but off-duty (includes mercs inside vehicles!)

		// if in a vehicle, pull him out
		if (bUniqueVehicleSquad)
		{
			if (fExitVehicles)
			{
				TakeSoldierOutOfVehicle(&s);

				/* put them on the unique squad assigned to people leaving this vehicle.
				 * Can't add them to existing squads, because if this is a simultaneous
				 * group attack, the mercs could be coming from different sides, and the
				 * placement screen can't handle mercs on the same squad arriving from
				 * different edges! */
				BOOLEAN const fSuccess = AddCharacterToSquad(&s, *bUniqueVehicleSquad);
				Assert(fSuccess);
			}
		}
		else
		{
			// add him to ANY on duty foot squad
			AddCharacterToAnySquad(&s);
		}

		// stand him up
		MakeSoldiersTacticalAnimationReflectAssignment(&s);
	}
}


void WakeUpAllMercsInSectorUnderAttack()
{
	FOR_EACH_IN_TEAM(i, OUR_TEAM)
	{
		SOLDIERTYPE& s = *i;
		if (s.bLife == 0)                       continue;
		if (s.uiStatusFlags & SOLDIER_VEHICLE)  continue;
		if (!s.fMercAsleep)                     continue;
		if (!PlayerMercInvolvedInThisCombat(s)) continue;
		// Involved, but asleep, force him wake him up
		SetMercAwake(&s, FALSE, TRUE);
	}
}


// we are entering the sector, clear out all mvt orders for grunts
static void ClearMovementForAllInvolvedPlayerGroups(void)
{
	FOR_EACH_GROUP(i)
	{
		GROUP& g = *i;
		if (!PlayerGroupInvolvedInThisCombat(g)) continue;
		// clear their strategic movement (mercpaths and waypoints)
		ClearMercPathsAndWaypointsForAllInGroup(g);
	}
}

void RetreatAllInvolvedPlayerGroups( void )
{
	// make sure guys stop their off duty assignments, like militia training!
	// but don't exit vehicles - drive off in them!
	PutNonSquadMercsInBattleSectorOnSquads( FALSE );

	FOR_EACH_GROUP(i)
	{
		GROUP& g = *i;
		if (!PlayerGroupInvolvedInThisCombat(g)) continue;
		// Don't retreat empty vehicle groups!
		if (g.fVehicle && !DoesVehicleGroupHaveAnyPassengers(g)) continue;
		ClearMercPathsAndWaypointsForAllInGroup(g);
		RetreatGroupToPreviousSector(g);
	}
}


static BOOLEAN CurrentBattleSectorIs(const SGPSector& sSector);


bool PlayerMercInvolvedInThisCombat(SOLDIERTYPE const& s)
{
	Assert(s.bActive);
	return
		!s.fBetweenSectors                         &&
		s.bAssignment != IN_TRANSIT                &&
		s.bAssignment != ASSIGNMENT_POW            &&
		s.bAssignment != ASSIGNMENT_DEAD           &&
		!(s.uiStatusFlags & SOLDIER_VEHICLE)       &&
		// Robot is involved iff it has a valid controller with it
		(!AM_A_ROBOT(&s) || s.robot_remote_holder) &&
		!SoldierAboardAirborneHeli(s)              &&
		CurrentBattleSectorIs(s.sSector);
}


bool PlayerGroupInvolvedInThisCombat(GROUP const& g)
{
	/* Player group, non-empty, not between sectors, in the right sector, isn't a
	 * group of in transit, dead, or POW mercs, and either not the helicopter
	 * group, or the heli is on the ground */
	return
		g.fPlayer                                                 &&
		g.ubGroupSize != 0                                        &&
		!g.fBetweenSectors                                        &&
		!GroupHasInTransitDeadOrPOWMercs(g)                       &&
		(!IsGroupTheHelicopterGroup(g) || !fHelicopterIsAirBorne) &&
		CurrentBattleSectorIs(g.ubSector);
}

static BOOLEAN CurrentBattleSectorIs(const SGPSector& sSector)
{
	SGPSector sBattleSector;
	BOOLEAN fSuccess;

	fSuccess = GetCurrentBattleSectorXYZ(sBattleSector);
	Assert( fSuccess );

	return sSector == sBattleSector;
}

static void CheckForRobotAndIfItsControlled(void)
{
	// search for the robot on player's team
	FOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		if (s->bLife != 0 && AM_A_ROBOT(s))
		{
			// check whether it has a valid controller with it. This sets its robot_remote_holder field.
			UpdateRobotControllerGivenRobot(s);

			// if he has a controller, set controllers
			if (s->robot_remote_holder != NULL)
			{
				UpdateRobotControllerGivenController(s->robot_remote_holder);
			}

			break;
		}
	}
}


void LogBattleResults(const UINT8 ubVictoryCode)
{
	SGPSector sSector;
	GetCurrentBattleSectorXYZ(sSector);
	UINT8 code;
	if (ubVictoryCode == LOG_VICTORY)
	{
		switch (gubEnemyEncounterCode)
		{
			case ENEMY_INVASION_CODE:         code = HISTORY_DEFENDEDTOWNSECTOR;   break;
			case ENEMY_ENCOUNTER_CODE:        code = HISTORY_WONBATTLE;            break;
			case ENEMY_AMBUSH_CODE:           code = HISTORY_WIPEDOUTENEMYAMBUSH;  break;
			case ENTERING_ENEMY_SECTOR_CODE:  code = HISTORY_SUCCESSFULATTACK;     break;
			case CREATURE_ATTACK_CODE:        code = HISTORY_CREATURESATTACKED;    break;
			case BLOODCAT_AMBUSH_CODE:
			case ENTERING_BLOODCAT_LAIR_CODE: code = HISTORY_SLAUGHTEREDBLOODCATS; break;
			default:                          return;
		}
	}
	else
	{
		switch (gubEnemyEncounterCode)
		{
			case ENEMY_INVASION_CODE:         code = HISTORY_LOSTTOWNSECTOR;       break;
			case ENEMY_ENCOUNTER_CODE:        code = HISTORY_LOSTBATTLE;           break;
			case ENEMY_AMBUSH_CODE:           code = HISTORY_FATALAMBUSH;          break;
			case ENTERING_ENEMY_SECTOR_CODE:  code = HISTORY_UNSUCCESSFULATTACK;   break;
			case CREATURE_ATTACK_CODE:        code = HISTORY_CREATURESATTACKED;    break;
			case BLOODCAT_AMBUSH_CODE:
			case ENTERING_BLOODCAT_LAIR_CODE: code = HISTORY_KILLEDBYBLOODCATS;    break;
			default:                          return;
		}
	}
	AddHistoryToPlayersLog(code, 0, GetWorldTotalMin(), sSector);
}


void HandlePreBattleInterfaceStates()
{
	if( gfEnteringMapScreenToEnterPreBattleInterface && !gfEnteringMapScreen )
	{
		gfEnteringMapScreenToEnterPreBattleInterface = FALSE;
		if( !gfUsePersistantPBI )
		{
			InitPreBattleInterface(0, false);
			gfUsePersistantPBI = TRUE;
		}
		else
		{
			InitPreBattleInterface(gpBattleGroup, true);
		}
	}
	else if( gfDelayAutoResolveStart && gfPreBattleInterfaceActive )
	{
		gfDelayAutoResolveStart = FALSE;
		gfAutomaticallyStartAutoResolve = TRUE;
	}
	else if( gfAutomaticallyStartAutoResolve )
	{
		gfAutomaticallyStartAutoResolve = FALSE;
		ActivateAutomaticAutoResolveStart();
	}
}
