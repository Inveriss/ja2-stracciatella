#include "MapScreen.h"
#include "MessageBoxScreen.h"
#include "SAM_Sites.h"
#include "Town_Militia.h"
#include "Militia_Control.h"
#include "Campaign_Types.h"
#include "StrategicMap.h"
#include "Overhead.h"
#include "Strategic_Town_Loyalty.h"
#include "Random.h"
#include "Text.h"
#include "Map_Screen_Interface.h"
#include "LaptopSave.h"
#include "Finances.h"
#include "Game_Clock.h"
#include "Assignments.h"
#include "Dialogue_Control.h"
#include "Queen_Command.h"
#include "PreBattle_Interface.h"
#include "Map_Screen_Interface_Border.h"
#include "Map_Screen_Interface_Map.h"
#include "JAScreens.h"
#include "Debug.h"
#include "ScreenIDs.h"
#include "UILayout.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "TownModel.h"
#include "GamePolicy.h"
#include "Game_Event_Hook.h"
#include "Militia_Recruit_Panel.h"
#include "WordWrap.h"
#include "Militia_Watch.h"
#include "Strategic_Movement.h"
#include "Map_Information.h"
#include "Militia_Control.h"
#include "Handle_UI.h"
#include "ScreenIDs.h"
#include "Message.h"
#include "SaveLoadGameStates.h"

#include <string_theory/format>
#include <string_theory/string>

#include <algorithm>
#include <iterator>

#define SIZE_OF_MILITIA_COMPLETED_TRAINING_LIST PLAYER_TEAM_SIZE

// temporary local global variables
UINT8 gubTownSectorServerTownId = BLANK_SECTOR;
SGPSector gsTownSectorServerSkip(-1, -1);
UINT8 gubTownSectorServerIndex = 0;
BOOLEAN gfYesNoPromptIsForContinue = FALSE; // this flag remembers whether we're starting new training, or continuing
INT32 giTotalCostOfTraining = 0;


//the completed list of sector soldiers for training militia
static SOLDIERTYPE* g_list_of_merc_in_sectors_completed_militia_training[SIZE_OF_MILITIA_COMPLETED_TRAINING_LIST];
SOLDIERTYPE *pMilitiaTrainerSoldier = NULL;

// note that these sector values are STRATEGIC INDEXES, not 0-255!
static INT16 gsUnpaidStrategicSector[MAX_CHARACTER_COUNT];


static void HandleCompletionOfTownTrainingByGroupWithTrainer(SOLDIERTYPE* pTrainer);
static void InitFriendlyTownSectorServer(UINT8 ubTownId, INT16 sSkipSectorX, INT16 sSkipSectorY);
static bool ServeNextFriendlySectorInTown(SGPSector& neighbour);
static void StrategicPromoteMilitiaInSector(const SGPSector& sMap, UINT8 current_rank, UINT8 n);


void TownMilitiaTrainingCompleted(SOLDIERTYPE *pTrainer, const SGPSector& sector)
{
	UINT8 ubMilitiaTrained = 0;
	BOOLEAN fFoundOne;
	UINT8 ubTownId;


	// get town index
	ubTownId = StrategicMap[sector.AsStrategicIndex()].bNameId;

	if( ubTownId == BLANK_SECTOR )
	{
		Assert(IsThisSectorASAMSector(sector));
	}


	// force tactical to update militia status
	gfStrategicMilitiaChangesMade = TRUE;

	// ok, so what do we do with all this training?  Well, in order of decreasing priority:
	// 1) If there's room in training sector, create new GREEN militia guys there
	// 2) If not enough room there, create new GREEN militia guys in friendly sectors of the same town
	// 3) If not enough room anywhere in town, promote a number of GREENs in this sector into regulars
	// 4) If not enough GREENS there to promote, promote GREENs in other sectors.
	// 5) If no GREENs anywhere, promote REGULARs in this sector into elites
	// 6) If not enough REGULARs there to promote, promote REGULARs in other sectors.
	// 7) If all friendly sectors of this town are completely filled with ELITE (or ELITE_HEAVY) militia, then training effect is wasted
	//    (ELITE_HEAVY is never created by training, only by combat promotions)

	SGPSector sNeighbour;
	while (ubMilitiaTrained < MILITIA_TRAINING_SQUAD_SIZE)
	{
		// is there room for another militia in the training sector itself?
		if (CountAllMilitiaInSector(sector) < MAX_ALLOWABLE_MILITIA_PER_SECTOR)
		{
			// great! Create a new GREEN militia guy in the training sector
			StrategicAddMilitiaToSector(sector, GREEN_MILITIA, 1);
		}
		else
		{
			fFoundOne = FALSE;

			if( ubTownId != BLANK_SECTOR )
			{
				InitFriendlyTownSectorServer(ubTownId, sector.x, sector.y);

				// check other eligible sectors in this town for room for another militia
				while (ServeNextFriendlySectorInTown(sNeighbour))
				{
					// is there room for another militia in this neighbouring sector ?
					if (CountAllMilitiaInSector(sNeighbour) < MAX_ALLOWABLE_MILITIA_PER_SECTOR)
					{
						// great! Create a new GREEN militia guy in the neighbouring sector
						StrategicAddMilitiaToSector(sNeighbour, GREEN_MILITIA, 1);

						fFoundOne = TRUE;
						break;
					}
				}
			}

			// if we still haven't been able to train anyone
			if (!fFoundOne)
			{
				// alrighty, then.  We'll have to *promote* guys instead.

				// are there any GREEN militia men in the training sector itself?
				if (MilitiaInSectorOfRank(sector, GREEN_MILITIA) > 0)
				{
					// great! Promote a GREEN militia guy in the training sector to a REGULAR
					StrategicPromoteMilitiaInSector(sector, GREEN_MILITIA, 1);
				}
				else
				{
					if( ubTownId != BLANK_SECTOR )
					{
						// dammit! Last chance - try to find other eligible sectors in the same town with a Green guy to be promoted
						InitFriendlyTownSectorServer(ubTownId, sector.x, sector.y);

						// check other eligible sectors in this town for room for another militia
						while (ServeNextFriendlySectorInTown(sNeighbour))
						{
							// are there any GREEN militia men in the neighbouring sector ?
							if (MilitiaInSectorOfRank(sNeighbour, GREEN_MILITIA) > 0)
							{
								// great! Promote a GREEN militia guy in the neighbouring sector to a REGULAR
								StrategicPromoteMilitiaInSector(sNeighbour, GREEN_MILITIA, 1);

								fFoundOne = TRUE;
								break;
							}
						}
					}

					// no GREENs left anywhere - promote a REGULAR to an ELITE instead
					if (!fFoundOne)
					{
						if (MilitiaInSectorOfRank(sector, REGULAR_MILITIA) > 0)
						{
							StrategicPromoteMilitiaInSector(sector, REGULAR_MILITIA, 1);
							fFoundOne = TRUE;
						}
						else if (ubTownId != BLANK_SECTOR)
						{
							InitFriendlyTownSectorServer(ubTownId, sector.x, sector.y);

							while (ServeNextFriendlySectorInTown(sNeighbour))
							{
								if (MilitiaInSectorOfRank(sNeighbour, REGULAR_MILITIA) > 0)
								{
									StrategicPromoteMilitiaInSector(sNeighbour, REGULAR_MILITIA, 1);

									fFoundOne = TRUE;
									break;
								}
							}
						}
					}

					// if we still haven't been able to train anyone
					if (!fFoundOne)
					{
						// Well, that's it.  All eligible sectors of this town are full of ELITEs (or ELITE_HEAVYs).
						// The training goes to waste in this situation.
						break; // the main while loop
					}
				}
			}
		}

		// next, please!
		ubMilitiaTrained++;
	}


	// if anyone actually got trained
	if (ubMilitiaTrained > 0)
	{
		// update the screen display
		fMapPanelDirty = TRUE;

		if( ubTownId != BLANK_SECTOR )
		{
			// loyalty in this town increases a bit because we obviously care about them...
			IncrementTownLoyalty( ubTownId, LOYALTY_BONUS_FOR_TOWN_TRAINING );
		}

	}


	// the trainer announces to player that he's finished his assignment.  Make his sector flash!
	AssignmentDone( pTrainer, TRUE, FALSE );

	// handle completion of town by training group
	HandleCompletionOfTownTrainingByGroupWithTrainer( pTrainer );

}


std::optional<UINT8> SoldierClassToMilitiaRank(UINT8 const soldier_class)
{
	switch (soldier_class)
	{
		case SOLDIER_CLASS_GREEN_MILITIA: return GREEN_MILITIA;
		case SOLDIER_CLASS_REG_MILITIA:   return REGULAR_MILITIA;
		case SOLDIER_CLASS_ELITE_MILITIA: return ELITE_MILITIA;
		case SOLDIER_CLASS_ELITE_HEAVY_MILITIA: return ELITE_HEAVY_MILITIA;
		default:                          return {};
	}
}


// add militias of a certain rank
void StrategicAddMilitiaToSector(const SGPSector& sMap, UINT8 ubRank, UINT8 ubHowMany)
{
	SECTORINFO *pSectorInfo = &(SectorInfo[sMap.AsByte()]);

	pSectorInfo->ubNumberOfCivsAtLevel[ ubRank ] += ubHowMany;

	// update the screen display
	fMapPanelDirty = TRUE;
}


// Promote militias of a certain rank
static void StrategicPromoteMilitiaInSector(const SGPSector& sMap, UINT8 const current_rank, UINT8 const n)
{
	SECTORINFO& si = SectorInfo[sMap.AsByte()];

	Assert(si.ubNumberOfCivsAtLevel[current_rank] >= n);
	//KM : July 21, 1999 patch fix
	if (si.ubNumberOfCivsAtLevel[current_rank] < n) return;

	si.ubNumberOfCivsAtLevel[current_rank    ] -= n;
	si.ubNumberOfCivsAtLevel[current_rank + 1] += n;

	fMapPanelDirty = TRUE;
}


void StrategicRemoveMilitiaFromSector(const SGPSector& sMap, UINT8 ubRank, UINT8 ubHowMany)
{
	SECTORINFO *pSectorInfo = &(SectorInfo[sMap.AsByte()]);

	// damn well better have that many around to remove!
	Assert(pSectorInfo->ubNumberOfCivsAtLevel[ ubRank ] >= ubHowMany);

	//KM : July 21, 1999 patch fix
	if( pSectorInfo->ubNumberOfCivsAtLevel[ ubRank ] < ubHowMany )
	{
		return;
	}

	pSectorInfo->ubNumberOfCivsAtLevel[ ubRank ] -= ubHowMany;

	// update the screen display
	fMapPanelDirty = TRUE;
}


// kill pts are (2 * kills) + assists
UINT8 CheckOneMilitiaForPromotion(const SGPSector& sMap, UINT8 &current_rank, UINT8 kill_points)
{
	// since the awarding is potentially significantly delayed, make sure they
	// weren't all promoted already by regular training or killed;
	// if we can't find them, we try higher ranks and make sure the caller follows up
	SECTORINFO& si = SectorInfo[sMap.AsByte()];
	while (si.ubNumberOfCivsAtLevel[current_rank] == 0) {
		if (current_rank == ELITE_HEAVY_MILITIA) return 0;
		current_rank++;
	}

	UINT8 n_promotions = 0;
	switch (current_rank)
	{
		case GREEN_MILITIA:
			// 2 kill points minimum, 25% chance per kill point
			if (kill_points < 2)           break;
			if (!Chance(25 * kill_points)) break;
			StrategicPromoteMilitiaInSector(sMap, GREEN_MILITIA, 1);
			++n_promotions;
			// Attempt another level up
			kill_points -= 2;
			/* FALLTHROUGH */
		case REGULAR_MILITIA:
			// 5 kill points minimum, 10% chance per kill point
			if (kill_points < 5)           break;
			if (!Chance(10 * kill_points)) break;
			StrategicPromoteMilitiaInSector(sMap, REGULAR_MILITIA, 1);
			++n_promotions;
			break;
		case ELITE_MILITIA:
			// only militia that already were ELITE can become ELITE_HEAVY (no chain from lower ranks)
			// 10 kill points minimum, 5% chance per kill point
			if (kill_points < 10)          break;
			if (!Chance(5 * kill_points))  break;
			StrategicPromoteMilitiaInSector(sMap, ELITE_MILITIA, 1);
			++n_promotions;
			break;
	}
	return n_promotions;
}


// call this if the player attacks his own militia
static void HandleMilitiaDefections(const SGPSector& sMap)
{
	UINT8 ubRank;
	UINT8 ubMilitiaCnt;
	UINT8 ubCount;
	UINT32 uiChanceToDefect;

	for( ubRank = 0; ubRank < MAX_MILITIA_LEVELS; ubRank++ )
	{
		ubMilitiaCnt = MilitiaInSectorOfRank(sMap, ubRank);

		// check each guy at each rank to see if he defects
		for (ubCount = 0; ubCount < ubMilitiaCnt; ubCount++)
		{
			switch( ubRank )
			{
				case GREEN_MILITIA:
					uiChanceToDefect = 50;
					break;
				case REGULAR_MILITIA:
					uiChanceToDefect = 75;
					break;
				case ELITE_MILITIA:
				case ELITE_HEAVY_MILITIA:
					uiChanceToDefect = 90;
					break;
				default:
					SLOGA("HandleMilitiaDefections: invalid Rank");
					return;
			}

			// roll the bones; should I stay or should I go now?  (for you music fans out there)
			if (Random(100) < uiChanceToDefect)
			{
				//B'bye!  (for you SNL fans out there)
				StrategicRemoveMilitiaFromSector(sMap, ubRank, 1);
			}
		}
	}
}


UINT8 CountAllMilitiaInSector(const SGPSector& sMap)
{
	UINT8 ubMilitiaTotal = 0;
	UINT8 ubRank;

	// find out if there are any town militia in this SECTOR (don't care about other sectors in same town)
	for( ubRank = 0; ubRank < MAX_MILITIA_LEVELS; ubRank++ )
	{
		ubMilitiaTotal += MilitiaInSectorOfRank(sMap, ubRank);
	}

	return(ubMilitiaTotal);
}


UINT8 MilitiaInSectorOfRank(const SGPSector& sMap, UINT8 ubRank)
{
	return SectorInfo[sMap.AsByte()].ubNumberOfCivsAtLevel[ubRank];
}


BOOLEAN SectorOursAndPeaceful(const SGPSector& sector)
{
	// if this sector is currently loaded
	if (sector == gWorldSector)
	{
		// and either there are enemies prowling this sector, or combat is in progress
		if ( gTacticalStatus.fEnemyInSector || ( gTacticalStatus.uiFlags & INCOMBAT ) )
		{
			return FALSE;
		}
	}

	// if sector is controlled by enemies, it's not ours (duh!)
	if (!sector.z && StrategicMap[sector.AsStrategicIndex()].fEnemyControlled)
	{
		return FALSE;
	}

	if (NumHostilesInSector(sector))
	{
		return FALSE;
	}

	// safe & secure, s'far as we can tell
	return(TRUE);
}


static void InitFriendlyTownSectorServer(UINT8 ubTownId, INT16 sSkipSectorX, INT16 sSkipSectorY)
{
	// reset globals
	gubTownSectorServerTownId = ubTownId;
	gsTownSectorServerSkip.x = sSkipSectorX;
	gsTownSectorServerSkip.y = sSkipSectorY;

	gubTownSectorServerIndex = 0;
}


/* Fetch the X,Y of the next town sector on the town list for the town specified
 * at initialization. It will skip an entry that matches the skip X/Y value, if
 * one was specified at initialization.
 * MUST CALL InitFriendlyTownSectorServer() before using! */
static bool ServeNextFriendlySectorInTown(SGPSector& neighbour)
{
	while (gubTownSectorServerIndex < g_town_sectors.size() && g_town_sectors[gubTownSectorServerIndex].town != BLANK_SECTOR)
	{
		INT32 const sector = g_town_sectors[gubTownSectorServerIndex++].sector;

		// if this sector is in the town we're looking for
		if (StrategicMap[SGPSector(sector).AsStrategicIndex()].bNameId != gubTownSectorServerTownId) continue;

		SGPSector sec(sector);
		// Make sure we're not supposed to skip it
		if (sec == gsTownSectorServerSkip) continue;

		// check if it's "friendly" - not enemy controlled, no enemies in it, no combat in progress
		if (!SectorOursAndPeaceful(sec)) continue;

		neighbour = sec;
		return true;
	}
	return false;
}


static void CantTrainMilitiaOkBoxCallback(MessageBoxReturnValue);
static INT32 GetNumberOfUnpaidTrainableSectors();
static void PayMilitiaTrainingYesNoBoxCallback(MessageBoxReturnValue);


void HandleInterfaceMessageForCostOfTrainingMilitia( SOLDIERTYPE *pSoldier )
{
	ST::string sString;
	INT32 iNumberOfSectors = 0;

	pMilitiaTrainerSoldier = pSoldier;

	// grab total number of sectors
	iNumberOfSectors = GetNumberOfUnpaidTrainableSectors( );
	Assert( iNumberOfSectors > 0 );

	// get total cost
	giTotalCostOfTraining = MILITIA_TRAINING_COST * iNumberOfSectors;
	Assert( giTotalCostOfTraining > 0 );

	gfYesNoPromptIsForContinue = FALSE;

	if( LaptopSaveInfo.iCurrentBalance < giTotalCostOfTraining )
	{
		sString = st_format_printf(pMilitiaConfirmStrings[7], giTotalCostOfTraining);
		DoScreenIndependantMessageBox( sString, MSG_BOX_FLAG_OK, CantTrainMilitiaOkBoxCallback );
		return;
	}

	if( iNumberOfSectors > 1 )
	{
		sString = st_format_printf(pMilitiaConfirmStrings[6], iNumberOfSectors, giTotalCostOfTraining, pMilitiaConfirmStrings[1]);
	}
	else
	{
		sString = ST::format("{}{}. {}", pMilitiaConfirmStrings[ 0 ], giTotalCostOfTraining, pMilitiaConfirmStrings[ 1 ]);
	}

	// if we are in mapscreen, make a pop up
	if( guiCurrentScreen == MAP_SCREEN )
	{
		DoMapMessageBox( MSG_BOX_BASIC_STYLE, sString, MAP_SCREEN, MSG_BOX_FLAG_YESNO, PayMilitiaTrainingYesNoBoxCallback );
	}
	else
	{
		SGPBox const centering_rect = { 0, 0, SCREEN_WIDTH, INV_INTERFACE_START_Y };
		DoMessageBox(MSG_BOX_BASIC_STYLE, sString, GAME_SCREEN, MSG_BOX_FLAG_YESNO, PayMilitiaTrainingYesNoBoxCallback, &centering_rect);
	}
}

// continue training?
static void HandleInterfaceMessageForContinuingTrainingMilitia(SOLDIERTYPE* const pSoldier)
{
	ST::string sString;
	ST::string sStringB;

	UINT8 const sector = pSoldier->sSector.AsByte();

	Assert(!SectorInfo[sector].fMilitiaTrainingPaid);

	pMilitiaTrainerSoldier = pSoldier;

	gfYesNoPromptIsForContinue = TRUE;

	// is there enough loyalty to continue training
	if (!DoesSectorMercIsInHaveSufficientLoyaltyToTrainMilitia(pSoldier))
	{
		// loyalty too low to continue training
		sString = st_format_printf(pMilitiaConfirmStrings[8], GCM->getTownName(GetTownIdForSector(sector)), MIN_RATING_TO_TRAIN_TOWN);
		DoScreenIndependantMessageBox( sString, MSG_BOX_FLAG_OK, CantTrainMilitiaOkBoxCallback );
		return;
	}

	if (IsAreaFullOfMilitia(pSoldier->sSector))
	{
		// we're full!!! go home!
		UINT8 const bTownId = GetTownIdForSector(sector);
		if ( bTownId == BLANK_SECTOR )
		{
			// wilderness SAM site
			sStringB = GetSectorIDString(pSoldier->sSector, TRUE);
			sString = st_format_printf(pMilitiaConfirmStrings[9], sStringB);
		}
		else
		{
			// town
			sString = st_format_printf(pMilitiaConfirmStrings[9], GCM->getTownName(bTownId), MIN_RATING_TO_TRAIN_TOWN);
		}
		DoScreenIndependantMessageBox( sString, MSG_BOX_FLAG_OK, CantTrainMilitiaOkBoxCallback );
		return;
	}

	// continue training always handles just one sector at a time
	giTotalCostOfTraining = MILITIA_TRAINING_COST;

	// can player afford to continue training?
	if( LaptopSaveInfo.iCurrentBalance < giTotalCostOfTraining )
	{
		// can't afford to continue training
		sString = st_format_printf(pMilitiaConfirmStrings[7], giTotalCostOfTraining);
		DoScreenIndependantMessageBox( sString, MSG_BOX_FLAG_OK, CantTrainMilitiaOkBoxCallback );
		return;
	}

	// ok to continue, ask player

	sStringB = GetSectorIDString(pSoldier->sSector, TRUE);
	sString = st_format_printf(pMilitiaConfirmStrings[ 3 ], sStringB, pMilitiaConfirmStrings[ 4 ], giTotalCostOfTraining);

	// ask player whether he'd like to continue training
	DoMapMessageBox( MSG_BOX_BASIC_STYLE, sString, MAP_SCREEN, MSG_BOX_FLAG_YESNO, PayMilitiaTrainingYesNoBoxCallback );
}


static void ContinueTrainingInThisSector(void);
static void MilitiaTrainingRejected(void);
static void StartTrainingInAllUnpaidTrainableSectors();


// IMPORTANT: This same callback is used both for initial training and for continue training prompt
// use 'gfYesNoPromptIsForContinue' flag to tell them apart
static void PayMilitiaTrainingYesNoBoxCallback(MessageBoxReturnValue const bExitValue)
{
	Assert( giTotalCostOfTraining > 0 );

	// yes
	if( bExitValue == MSG_BOX_RETURN_YES )
	{
		// does the player have enough
		if( LaptopSaveInfo.iCurrentBalance >= giTotalCostOfTraining )
		{
			if( gfYesNoPromptIsForContinue )
			{
				ContinueTrainingInThisSector();
			}
			else
			{
				StartTrainingInAllUnpaidTrainableSectors();
			}

			// this completes the training prompt sequence
			pMilitiaTrainerSoldier = NULL;
		}
		else	// can't afford it
		{
			StopTimeCompression();
			DoMapMessageBox(MSG_BOX_BASIC_STYLE, pMilitiaConfirmStrings[2], MAP_SCREEN, MSG_BOX_FLAG_OK, CantTrainMilitiaOkBoxCallback);
		}
	}
	else if( bExitValue == MSG_BOX_RETURN_NO )
	{
		StopTimeCompression();

		MilitiaTrainingRejected();
	}
}


static void CantTrainMilitiaOkBoxCallback(MessageBoxReturnValue const bExitValue)
{
	MilitiaTrainingRejected();
}


// IMPORTANT: This same callback is used both for initial training and for continue training prompt
// use 'gfYesNoPromptIsForContinue' flag to tell them apart
static void MilitiaTrainingRejected(void)
{
	if( gfYesNoPromptIsForContinue )
	{
		// take all mercs in that sector off militia training
		ResetAssignmentOfMercsThatWereTrainingMilitiaInThisSector(pMilitiaTrainerSoldier->sSector);
	}
	else
	{
		// take all mercs in unpaid sectors EVERYWHERE off militia training
		ResetAssignmentsForMercsTrainingUnpaidSectorsInSelectedList();
	}

	// this completes the training prompt sequence
	pMilitiaTrainerSoldier = NULL;
}



void HandleMilitiaStatusInCurrentMapBeforeLoadingNewMap( void )
{
	if ( gTacticalStatus.Team[ MILITIA_TEAM ].bSide != 0 )
	{
		// handle militia defections and reset team to friendly
		HandleMilitiaDefections(gWorldSector);
		gTacticalStatus.Team[ MILITIA_TEAM ].bSide = 0;
	}
	else if( !gfAutomaticallyStartAutoResolve )
	{ //Don't promote militia if we are going directly to autoresolve to finish the current battle.
		HandleMilitiaPromotions();
	}
}


bool CanNearbyMilitiaScoutThisSector(const SGPSector& sMap)
{
	INT16 const scout_range = 1;
	INT16 const xstart      = sMap.x >      scout_range ? sMap.x - scout_range :  1;
	INT16 const ystart      = sMap.y >      scout_range ? sMap.y - scout_range :  1;
	INT16 const xend        = sMap.x < 16 - scout_range ? sMap.x + scout_range : 16;
	INT16 const yend        = sMap.y < 16 - scout_range ? sMap.y + scout_range : 16;
	SGPSector sSector;
	for (sSector.y = ystart; sSector.y <= yend; ++sSector.y)
	{
		for (sSector.x = xstart; sSector.x <= xend; ++sSector.x)
		{
			if (CountAllMilitiaInSector(sSector) != 0) return true;
		}
	}
	return false;
}


BOOLEAN IsAreaFullOfMilitia(const SGPSector& sector)
{
	if (sector.z != 0) return TRUE;

	if (IsMilitiaRecruitmentEnabled())
	{
		// recruitment buys any rank: full means no room left at all
		return MilitiaRecruitFreeSlots(sector) == 0;
	}

	UINT32 count_milita = 0;
	UINT32 max_milita   = 0;

	INT8 const town_id = GetTownIdForSector(sector);
	if (town_id != BLANK_SECTOR)
	{
		FOR_EACH_SECTOR_IN_TOWN(i, town_id)
		{
			SGPSector town(i->sector);
			if (SectorOursAndPeaceful(town))
			{
				// don't count GREEN and REGULAR militia, they can be trained into elites first
				count_milita += MilitiaInSectorOfRank(town, ELITE_MILITIA);
				count_milita += MilitiaInSectorOfRank(town, ELITE_HEAVY_MILITIA);
				max_milita   += MAX_ALLOWABLE_MILITIA_PER_SECTOR;
			}
		}
	}
	else if (IsThisSectorASAMSector(sector))
	{
		// don't count GREEN and REGULAR militia, they can be trained into elites first
		count_milita += MilitiaInSectorOfRank(sector, ELITE_MILITIA);
		count_milita += MilitiaInSectorOfRank(sector, ELITE_HEAVY_MILITIA);
		max_milita   += MAX_ALLOWABLE_MILITIA_PER_SECTOR;
	}

	return count_milita >= max_milita;
}


// handle completion of assignment by this soldier too and inform the player
static void HandleCompletionOfTownTrainingByGroupWithTrainer(SOLDIERTYPE* pTrainer)
{
	// get the sector values
	SGPSector sSector = pTrainer->sSector;

	CFOR_EACH_IN_CHAR_LIST(c)
	{
		SOLDIERTYPE* const pSoldier = c->merc;
		if (pSoldier->bAssignment == TRAIN_TOWN && pSoldier->sSector == sSector)
		{
			// done assignment
			AssignmentDone( pSoldier, FALSE, FALSE );
		}
	}
}


void AddSectorForSoldierToListOfSectorsThatCompletedMilitiaTraining(SOLDIERTYPE* pSoldier)
{
	INT32 iCounter = 0;
	INT16 sSector = 0, sCurrentSector = 0;

	// get the sector value
	sSector = pSoldier->sSector.AsStrategicIndex();

	while (g_list_of_merc_in_sectors_completed_militia_training[iCounter] != NULL)
	{
		// get the current soldier
		const SOLDIERTYPE* const pCurrentSoldier = g_list_of_merc_in_sectors_completed_militia_training[iCounter];

		// get the current sector value
		sCurrentSector = pCurrentSoldier->sSector.AsStrategicIndex();

		// is the merc's sector already in the list?
		if( sCurrentSector == sSector )
		{
			// already here
			return;
		}

		iCounter++;

		Assert( iCounter < SIZE_OF_MILITIA_COMPLETED_TRAINING_LIST );
	}

	// add merc to the list
	g_list_of_merc_in_sectors_completed_militia_training[iCounter] = pSoldier;
}


// clear out the list of training sectors...should be done once the list is posted
void ClearSectorListForCompletedTrainingOfMilitia()
{
	FOR_EACH(SOLDIERTYPE*, i, g_list_of_merc_in_sectors_completed_militia_training) *i = 0;
}


void HandleContinueOfTownTraining( void )
{
	INT32 iCounter = 0;
	BOOLEAN fContinueEventPosted = FALSE;


	while (g_list_of_merc_in_sectors_completed_militia_training[iCounter] != NULL)
	{
		// get the soldier
		SOLDIERTYPE& s = *g_list_of_merc_in_sectors_completed_militia_training[iCounter];
		if (s.bActive)
		{
			fContinueEventPosted = TRUE;

			class DialogueEventContinueTrainingMilitia : public CharacterDialogueEvent
			{
				public:
					DialogueEventContinueTrainingMilitia(SOLDIERTYPE& soldier) : CharacterDialogueEvent(soldier) {}

					bool Execute()
					{
						HandleInterfaceMessageForContinuingTrainingMilitia(&soldier_);
						return false;
					}
			};

			DialogueEvent::Add(new DialogueEventContinueTrainingMilitia(s));
		}

		// next entry
		iCounter++;
	}

	// now clear the list
	ClearSectorListForCompletedTrainingOfMilitia( );

	if( fContinueEventPosted )
	{
		// ATE: If this event happens in tactical mode we will be switching at some time to mapscreen...
		if ( guiCurrentScreen == GAME_SCREEN )
		{
			gfEnteringMapScreen = TRUE;
		}

		//If the militia view isn't currently active, then turn it on when prompting to continue training.
		if ( !fShowMilitia )
		{
			ToggleShowMilitiaMode();
		}
	}
}


static void AddIfTrainingUnpaidSector(SOLDIERTYPE const& s)
{
	if (!CanCharacterTrainMilitia(&s)) return;
	// Check if this sector is a town and needs equipment.
	if (SectorInfo[s.sSector.AsByte()].fMilitiaTrainingPaid) return;
	INT16 const sector = s.sSector.AsStrategicIndex();
	for (INT16* i = gsUnpaidStrategicSector;; ++i)
	{
		if (*i == 0)
		{
			*i = sector;
			break;
		}
		if (*i == sector) break; // Do not add duplicate
	}
}


static void BuildListOfUnpaidTrainableSectors()
{
	std::fill(std::begin(gsUnpaidStrategicSector), std::end(gsUnpaidStrategicSector), 0);

	if (fInMapMode)
	{
		for (INT32 i = 0; i != MAX_CHARACTER_COUNT; ++i)
		{
			MapScreenCharacterSt const& c = gCharactersList[i];
			if (!c.merc)                                 continue;
			if (!c.selected && i != bSelectedAssignChar) continue;
			AddIfTrainingUnpaidSector(*c.merc);
		}
	}
	else
	{ // Handle for tactical
		AddIfTrainingUnpaidSector(*pMilitiaTrainerSoldier);
	}
}


static INT32 GetNumberOfUnpaidTrainableSectors()
{
	BuildListOfUnpaidTrainableSectors();
	INT32 n = 0;
	FOR_EACH(INT16 const, i, gsUnpaidStrategicSector)
	{
		if (*i != 0) ++n;
	}
	return n;
}


static void PayForTrainingInSector(UINT8 ubSector);


static void StartTrainingInAllUnpaidTrainableSectors()
{
	SetAssignmentForList(TRAIN_TOWN, 0);
	// Pay up in each sector
	BuildListOfUnpaidTrainableSectors();
	FOR_EACH(INT16 const, i, gsUnpaidStrategicSector)
	{
		if (*i == 0) continue;
		PayForTrainingInSector(SGPSector::FromStrategicIndex(*i).AsByte());
	}
}


static void ContinueTrainingInThisSector(void)
{
	UINT8 ubSector;

	Assert( pMilitiaTrainerSoldier );

	// pay up in the sector where pMilitiaTrainerSoldier is
	ubSector = pMilitiaTrainerSoldier->sSector.AsByte();
	PayForTrainingInSector( ubSector );
}


static void ResetDoneFlagForAllMilitiaTrainersInSector(UINT8 ubSector);


static void PayForTrainingInSector(UINT8 ubSector)
{
	Assert(!SectorInfo[ubSector].fMilitiaTrainingPaid);

	// spend the money
	AddTransactionToPlayersBook( TRAIN_TOWN_MILITIA, ubSector, GetWorldTotalMin(), -( MILITIA_TRAINING_COST ) );

	// mark this sector sectors as being paid up
	SectorInfo[ ubSector ].fMilitiaTrainingPaid = TRUE;

	// reset done flags
	ResetDoneFlagForAllMilitiaTrainersInSector( ubSector );
}


static void ResetDoneFlagForAllMilitiaTrainersInSector(UINT8 ubSector)
{
	FOR_EACH_IN_TEAM(pSoldier, OUR_TEAM)
	{
		if (pSoldier->bAssignment == TRAIN_TOWN &&
				pSoldier->sSector.AsByte() == ubSector &&
				pSoldier->sSector.z == 0)
		{
			pSoldier->fDoneAssignmentAndNothingToDoFlag = FALSE;
			pSoldier->usQuoteSaidExtFlags &= ~SOLDIER_QUOTE_SAID_DONE_ASSIGNMENT;
		}
	}
}


BOOLEAN MilitiaTrainingAllowedInSector(const SGPSector& sector)
{
	BOOLEAN fSamSitePresent = FALSE;

	if (sector.z != 0)
	{
		return( FALSE );
	}

	fSamSitePresent = IsThisSectorASAMSector(sector);

	if( fSamSitePresent )
	{
		// all SAM sites may have militia trained at them
		return(TRUE);
	}

	UINT8 const bTownId = GetTownIdForSector(sector);
	return( MilitiaTrainingAllowedInTown( bTownId ) );
}



BOOLEAN MilitiaTrainingAllowedInTown( INT8 bTownId )
{
	auto town = GCM->getTown(bTownId);
	if (town == NULL) {
		return false;
	}
	return town->isMilitiaTrainingAllowed;
}


static void PromoteMilitia(ST::string& str, INT8 count, const ST::string& singular, const ST::string& plural)
{
	if (count > 0)
	{
		if (!str.empty()) str += " ";
		if (count == 1)
		{
			str += singular;
		}
		else
		{
			str += st_format_printf(plural, count);
		}
	}
}


ST::string BuildMilitiaPromotionsString()
{
	ST::string str;

	if (gbMilitiaPromotions == 0) return {};

	PromoteMilitia(str, gbGreenToElitePromotions, gzLateLocalizedString[STR_LATE_29], gzLateLocalizedString[STR_LATE_22]);
	PromoteMilitia(str, gbGreenToRegPromotions,  gzLateLocalizedString[STR_LATE_30], gzLateLocalizedString[STR_LATE_23]);
	PromoteMilitia(str, gbRegToElitePromotions,  gzLateLocalizedString[STR_LATE_31], gzLateLocalizedString[STR_LATE_24]);
	PromoteMilitia(str, gbEliteToHeavyPromotions, pMilitiaHeavyStrings[1],           pMilitiaHeavyStrings[2]);

	// Clear the fields
	gbGreenToElitePromotions = 0;
	gbGreenToRegPromotions   = 0;
	gbRegToElitePromotions   = 0;
	gbEliteToHeavyPromotions = 0;
	gbMilitiaPromotions      = 0;

	return str;
}


// ---------------------------------------------------------------------------
// Militia recruitment
// ---------------------------------------------------------------------------

bool IsMilitiaRecruitmentEnabled()
{
	return gamepolicy(militia_recruitment);
}


// The sectors of the town (or the SAM site) of this sector that can take
// militia now: ours and peaceful
template<typename F> static void ForEachRecruitSector(const SGPSector& sector, F f)
{
	if (sector.z != 0) return;
	INT8 const town_id = GetTownIdForSector(sector);
	if (town_id != BLANK_SECTOR)
	{
		FOR_EACH_SECTOR_IN_TOWN(i, town_id)
		{
			SGPSector const town(i->sector);
			if (SectorOursAndPeaceful(town)) f(town);
		}
	}
	else if (IsThisSectorASAMSector(sector) && SectorOursAndPeaceful(sector))
	{
		f(sector);
	}
}


static std::vector<MilitiaOrder> g_militia_orders;


std::vector<MilitiaOrder> const& GetMilitiaOrders()
{
	return g_militia_orders;
}


// Both sectors belong to the same town, or are the same SAM site
static bool SameRecruitArea(const SGPSector& a, const SGPSector& b)
{
	INT8 const town_a = GetTownIdForSector(a);
	return town_a != BLANK_SECTOR ? town_a == GetTownIdForSector(b) : a == b;
}


static UINT32 OrderTotal(MilitiaOrder const& o)
{
	UINT32 n = 0;
	for (UINT8 const c : o.counts) n += c;
	return n;
}


// The room in the town's peaceful sectors of ours, before the orders
static UINT32 MilitiaRecruitRoom(const SGPSector& sector)
{
	UINT32 room = 0;
	ForEachRecruitSector(sector, [&](const SGPSector& s)
	{
		UINT8 const n = CountAllMilitiaInSector(s);
		if (n < MAX_ALLOWABLE_MILITIA_PER_SECTOR) room += MAX_ALLOWABLE_MILITIA_PER_SECTOR - n;
	});
	return room;
}


// The militia ordered for this town (or SAM site), on their way or waiting
static UINT32 MilitiaOrderedFor(const SGPSector& sector)
{
	UINT32 n = 0;
	for (MilitiaOrder const& o : g_militia_orders)
	{
		if (SameRecruitArea(o.sector, sector)) n += OrderTotal(o);
	}
	return n;
}


UINT32 MilitiaRecruitFreeSlots(const SGPSector& sector)
{
	// the ordered militia have their places already
	UINT32 const room    = MilitiaRecruitRoom(sector);
	UINT32 const ordered = MilitiaOrderedFor(sector);
	return room > ordered ? room - ordered : 0;
}


SOLDIERTYPE const* BestMilitiaRecruiter(const SGPSector& sector)
{
	if (sector.z != 0) return NULL;
	INT8 const town_id = GetTownIdForSector(sector);

	SOLDIERTYPE const* best = NULL;
	CFOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		if (s->bLife < OKLIFE)                         continue;
		if (s->uiStatusFlags & SOLDIER_VEHICLE)        continue;
		if (s->fBetweenSectors || s->sSector.z != 0)   continue;
		if (s->bAssignment == IN_TRANSIT ||
				s->bAssignment == ASSIGNMENT_DEAD ||
				s->bAssignment == ASSIGNMENT_POW)          continue;
		// in the same town, or for a SAM site in the same sector
		if (town_id != BLANK_SECTOR ? GetTownIdForSector(s->sSector) != town_id : s->sSector != sector) continue;
		if (!best || s->bLeadership > best->bLeadership) best = s;
	}
	return best;
}


INT32 MilitiaRecruitPricePercent(INT8 const leadership)
{
	for (int i = 0; i != 4; ++i)
	{
		if (leadership <= gamepolicy(militia_recruit_leadership_max[i])) return gamepolicy(militia_recruit_price_percent[i]);
	}
	return gamepolicy(militia_recruit_price_percent[3]);
}


INT32 MilitiaRecruitUnitPrice(UINT8 const rank, const SGPSector& sector)
{
	SOLDIERTYPE const* const recruiter = BestMilitiaRecruiter(sector);
	INT8  const leadership = recruiter ? recruiter->bLeadership : 0;
	INT32 const base       = gamepolicy(militia_recruit_price[rank]);
	return base * MilitiaRecruitPricePercent(leadership) / 100;
}


// One militia of this rank into the given sector if there is room, else into
// another sector of the same town; FALSE if there is no room anywhere
static BOOLEAN PlaceRecruitedMilitia(const SGPSector& sector, UINT8 const rank)
{
	if (SectorOursAndPeaceful(sector) && CountAllMilitiaInSector(sector) < MAX_ALLOWABLE_MILITIA_PER_SECTOR)
	{
		StrategicAddMilitiaToSector(sector, rank, 1);
		return TRUE;
	}

	BOOLEAN placed = FALSE;
	ForEachRecruitSector(sector, [&](const SGPSector& s)
	{
		if (placed) return;
		if (CountAllMilitiaInSector(s) >= MAX_ALLOWABLE_MILITIA_PER_SECTOR) return;
		StrategicAddMilitiaToSector(s, rank, 1);
		placed = TRUE;
	});
	return placed;
}


UINT8 MilitiaArrivalHours(UINT8 const option)
{
	switch (option)
	{
		case MILITIA_ARRIVAL_EXPRESS: return 6;
		case MILITIA_ARRIVAL_FAST:    return 12;
		default:                      return 24;
	}
}


INT32 MilitiaArrivalExtraCost(UINT8 const option)
{
	switch (option)
	{
		case MILITIA_ARRIVAL_EXPRESS: return 1000;
		case MILITIA_ARRIVAL_FAST:    return 500;
		default:                      return 0;
	}
}


// The town loyalty bonus of the militia training: one for every 10 militia
// that arrived in the town
static void AddMilitiaRecruitLoyalty(const SGPSector& sector, UINT32 const n)
{
	INT8 const town_id = GetTownIdForSector(sector);
	if (town_id == BLANK_SECTOR) return;
	auto const town = GCM->getTown(town_id);
	if (!town) return;

	UINT8& counter = SectorInfo[town->getBaseSector().AsByte()].ubMilitiaRecruitedForLoyalty;
	UINT32 recruited = counter + n;
	while (recruited >= MILITIA_TRAINING_SQUAD_SIZE)
	{
		IncrementTownLoyalty(town_id, LOYALTY_BONUS_FOR_TOWN_TRAINING);
		recruited -= MILITIA_TRAINING_SQUAD_SIZE;
	}
	counter = (UINT8) recruited;
}


// The town's name, or the SAM site's sector
static ST::string RecruitAreaName(const SGPSector& sector)
{
	INT8 const town_id = GetTownIdForSector(sector);
	return town_id != BLANK_SECTOR ? GCM->getTownName(town_id) : sector.AsShortString();
}


BOOLEAN BuyMilitia(const SGPSector& sector, UINT8 const (&counts)[MAX_MILITIA_LEVELS], UINT8 const arrival_option)
{
	UINT32 n_total = 0;
	INT32  cost    = MilitiaArrivalExtraCost(arrival_option);
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		n_total += counts[rank];
		cost    += counts[rank] * MilitiaRecruitUnitPrice(rank, sector);
	}
	if (n_total == 0)                                return FALSE;
	if (n_total > MilitiaRecruitFreeSlots(sector))   return FALSE;
	if (LaptopSaveInfo.iCurrentBalance < cost)      return FALSE;

	AddTransactionToPlayersBook(RECRUIT_TOWN_MILITIA, sector.AsByte(), GetWorldTotalMin(), -cost);

	// ordered: they arrive after the option's hours
	MilitiaOrder o{};
	o.id = 1;
	for (MilitiaOrder const& other : g_militia_orders) o.id = std::max(o.id, other.id + 1);
	o.sector      = sector;
	std::copy(std::begin(counts), std::end(counts), std::begin(o.counts));
	o.arrival_min = GetWorldTotalMin() + MilitiaArrivalHours(arrival_option) * 60;
	o.option      = arrival_option;
	o.waiting     = false;
	g_militia_orders.push_back(o);
	AddStrategicEvent(EVENT_MILITIA_ORDER_ARRIVAL, o.arrival_min, o.id);

	fMapPanelDirty = TRUE;
	return TRUE;
}


static std::vector<MilitiaOrder>::iterator FindMilitiaOrder(UINT32 const order_id)
{
	return std::find_if(g_militia_orders.begin(), g_militia_orders.end(),
		[&](MilitiaOrder const& o) { return o.id == order_id; });
}


// How many of the order's militia can enter the town now: its room, less the
// other orders'
static UINT32 MilitiaOrderRoom(MilitiaOrder const& o)
{
	UINT32 const room   = MilitiaRecruitRoom(o.sector);
	UINT32 const others = MilitiaOrderedFor(o.sector) - OrderTotal(o);
	return room > others ? room - others : 0;
}


// Up to n of the order's militia enter the town, the best ranks first; the
// order keeps the rest (and is removed when none is left)
// The arrival notices waiting to be shown (HandleMilitiaOrderQuestions())
static std::vector<ST::string> g_order_notices;


static void MilitiaOrderWaits(MilitiaOrder& o);


// "10 green militia have arrived in Drassen.": a line per rank in the
// message log
static void LogMilitiaArrival(MilitiaOrder const& o)
{
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		if (o.counts[rank] == 0) continue;
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_ARRIVED_RANK_FIRST + rank],
			int(o.counts[rank]), RecruitAreaName(o.sector)));
	}
}


void SetMilitiaOrderCounts(UINT32 const order_id, UINT8 const (&counts)[MAX_MILITIA_LEVELS])
{
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;
	std::copy(std::begin(counts), std::end(counts), std::begin(it->counts));
}


UINT32 SendMilitiaOrderToDeployment(UINT32 const order_id, UINT8 const (&counts)[MAX_MILITIA_LEVELS], bool const announce)
{
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return 0;

	MilitiaOrder sent = *it;
	sent.id = 1;
	for (MilitiaOrder const& other : g_militia_orders) sent.id = std::max(sent.id, other.id + 1);
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		UINT8 const n = std::min(counts[rank], it->counts[rank]);
		sent.counts[rank]  = n;
		it->counts[rank]  -= n;
	}
	if (OrderTotal(sent) == 0) return 0;
	sent.waiting = true;
	g_militia_orders.push_back(sent); // it is no longer valid
	if (announce) MilitiaOrderWaits(g_militia_orders.back());
	return sent.id;
}


void FinishMilitiaArrivalOrder(UINT32 const order_id, UINT32 const n_entered)
{
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;
	if (n_entered != 0) AddMilitiaRecruitLoyalty(it->sector, n_entered);
	g_militia_orders.erase(it);
	gfStrategicMilitiaChangesMade = TRUE;
	fMapPanelDirty = TRUE;
}


static void EnterMilitiaOrder(UINT32 const order_id, UINT32 n)
{
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;
	MilitiaOrder& o = *it;

	UINT32 entered = 0;
	UINT32 entered_rank[MAX_MILITIA_LEVELS] = {};
	for (UINT8 rank = MAX_MILITIA_LEVELS; rank-- != 0 && n != 0;)
	{
		while (o.counts[rank] != 0 && n != 0)
		{
			if (!PlaceRecruitedMilitia(o.sector, rank)) { n = 0; break; } // no room after all
			--o.counts[rank];
			--n;
			++entered;
			++entered_rank[rank];
		}
	}
	if (entered == 0) return;

	AddMilitiaRecruitLoyalty(o.sector, entered);

	// "10 green militia have arrived in Drassen.": a line per rank, in the
	// message log and in a message box
	char32_t const newline[] = { TEXT_CODE_NEWLINE, 0 };
	ST::string     notice;
	for (UINT8 rank = 0; rank != MAX_MILITIA_LEVELS; ++rank)
	{
		if (entered_rank[rank] == 0) continue;
		ST::string const line = st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_ARRIVED_RANK_FIRST + rank],
			int(entered_rank[rank]), RecruitAreaName(o.sector));
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, line);
		if (!notice.empty()) notice += ST::string::from_utf32(newline);
		notice += line;
	}
	g_order_notices.push_back(notice);
	if (OrderTotal(o) == 0) g_militia_orders.erase(it);

	// force tactical to update militia status
	gfStrategicMilitiaChangesMade = TRUE;
	fMapPanelDirty = TRUE;
}


static void MilitiaOrderWaits(MilitiaOrder& o)
{
	o.waiting = true;
	ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_WAITING], RecruitAreaName(o.sector)));
}


// The orders waiting for the player's answer, in the order they arrived; the
// one asked about now
static std::vector<UINT32> g_order_questions;
static UINT32              g_order_asked = 0;
// The orders the militia box sent into the battle loaded in tactical (D5.2)
static std::vector<UINT32> g_order_joins;


// The order's sector held by the enemy, no battle there already (4b, 4d):
// the militia can attack it
static bool MilitiaOrderCanAttack(MilitiaOrder const& o)
{
	SGPSector const& s = o.sector;
	if (s.z != 0)                     return false;
	if (NumEnemiesInSector(s) == 0)   return false;
	// militia alone attack only a sector the mercs liberated once
	if (!SectorInfo[s.AsByte()].fSurfaceWasEverPlayerControlled) return false;
	if (gfPreBattleInterfaceActive)   return false;
	// a battle in the loaded sector: joining it is stage 4.3
	if (gfWorldLoaded && gWorldSector == s && gTacticalStatus.fEnemyInSector) return false;
	return CountAllMilitiaInSector(s) < MAX_ALLOWABLE_MILITIA_PER_SECTOR;
}


// A battle is going on in the order's sector, loaded in tactical, and it can
// take more militia (4c): they can join it from a map edge
static bool MilitiaOrderCanJoin(MilitiaOrder const& o)
{
	if (o.sector.z != 0 || !gfWorldLoaded || gWorldSector != o.sector) return false;
	if (!gTacticalStatus.fEnemyInSector)                               return false;
	return MilitiaReinforcementRoom() != 0;
}


// Joining a battle is asked on the tactical screen only, in real time or in
// the player's turn, between attacks
static bool CanAskToJoinNow()
{
	if (guiCurrentScreen != GAME_SCREEN)                           return false;
	if (gTacticalStatus.ubAttackBusyCount != 0)                     return false;
	if (gfDisableRegionActive || gfUserTurnRegionActive)            return false;
	if ((gTacticalStatus.uiFlags & INCOMBAT) &&
			gTacticalStatus.ubCurrentTeam != OUR_TEAM)                 return false;
	return true;
}


static void JoinCallback(MessageBoxReturnValue const ret)
{
	UINT32 const order_id = g_order_asked;
	g_order_asked = 0;
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;
	MilitiaOrder& o = *it;

	if (ret != MSG_BOX_RETURN_YES || !MilitiaOrderCanJoin(o) || guiCurrentScreen != GAME_SCREEN)
	{
		MilitiaOrderWaits(o);
		return;
	}

	// as many as the battle takes come in, the best ranks first; the rest waits
	SGPSector const sector = o.sector;
	UINT8 const entered = AddMilitiaReinforcementsToBattle(o.counts, (UINT8)std::min<UINT32>(OrderTotal(o), 255));
	if (entered == 0)
	{
		MilitiaOrderWaits(o);
		return;
	}
	AddMilitiaRecruitLoyalty(sector, entered);
	ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_ARRIVED], RecruitAreaName(sector)));
	if (OrderTotal(o) == 0) g_militia_orders.erase(it);
	else                    MilitiaOrderWaits(o);
	fMapPanelDirty = TRUE;
}


static void AttackCallback(MessageBoxReturnValue const ret)
{
	UINT32 const order_id = g_order_asked;
	g_order_asked = 0;
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;
	MilitiaOrder& o = *it;

	if (ret != MSG_BOX_RETURN_YES || !MilitiaOrderCanAttack(o))
	{
		MilitiaOrderWaits(o);
		return;
	}

	// as many as the sector holds enter it, the best ranks first; the rest
	// waits
	SGPSector const sector = o.sector;
	UINT32 n = MAX_ALLOWABLE_MILITIA_PER_SECTOR - CountAllMilitiaInSector(sector);
	UINT32 entered = 0;
	for (UINT8 rank = MAX_MILITIA_LEVELS; rank-- != 0 && n != 0;)
	{
		UINT8 const k = (UINT8)std::min<UINT32>(o.counts[rank], n);
		if (k == 0) continue;
		StrategicAddMilitiaToSector(sector, rank, k);
		o.counts[rank] -= k;
		n              -= k;
		entered        += k;
	}
	AddMilitiaRecruitLoyalty(sector, entered);
	if (OrderTotal(o) == 0) g_militia_orders.erase(it);
	else                    o.waiting = true;

	gfStrategicMilitiaChangesMade = TRUE;
	fMapPanelDirty = TRUE;
	StartMilitiaAttackBattle(sector);
}


static void PartialEntryCallback(MessageBoxReturnValue const ret)
{
	UINT32 const order_id = g_order_asked;
	g_order_asked = 0;
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;

	if (ret == MSG_BOX_RETURN_YES)
	{
		// those who fit enter, the rest waits
		EnterMilitiaOrder(order_id, MilitiaOrderRoom(*it));
		auto const rest = FindMilitiaOrder(order_id);
		if (rest != g_militia_orders.end()) MilitiaOrderWaits(*rest);
	}
	else
	{
		MilitiaOrderWaits(*it);
	}
}


// The militia box (its key: a town, a SAM site) that takes an order on its
// arrival, BLANK_SECTOR: none
static INT16 MilitiaOrderBoxTown(MilitiaOrder const& o)
{
	return MilitiaBoxKeyForSector(o.sector);
}


void HandleMilitiaOrderArrival(UINT32 const order_id)
{
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;
	MilitiaOrder& o = *it;

	// a town's order: the militia box, on the map screen (D1, D1b), queued
	// as the questions
	if (MilitiaOrderBoxTown(o) != BLANK_SECTOR)
	{
		g_order_questions.push_back(order_id);
		return;
	}

	UINT32 const total = OrderTotal(o);
	UINT32 const room  = MilitiaOrderRoom(o);
	if (room >= total)
	{
		// everybody enters
		EnterMilitiaOrder(order_id, total);
	}
	else if (room > 0 || MilitiaOrderCanJoin(o) || MilitiaOrderCanAttack(o))
	{
		// only some fit (4e), a battle in the loaded sector (4c), or the enemy
		// holds the sector (4b, 4d): the player is asked
		g_order_questions.push_back(order_id);
	}
	else
	{
		// no room: they wait for orders (the enemy holding a sector the mercs
		// never liberated: the player is told why there is no attack)
		if (o.sector.z == 0 && NumEnemiesInSector(o.sector) != 0 &&
				!SectorInfo[o.sector.AsByte()].fSurfaceWasEverPlayerControlled)
		{
			ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
				st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_LIBERATE_FIRST], o.sector.AsShortString()));
		}
		MilitiaOrderWaits(o);
	}
}


void RetryMilitiaOrder(UINT32 const order_id)
{
	// its militia are the militia box's unassigned ones
	if (order_id == MilitiaArrivalBoxOrder()) return;
	// already asked about, or queued
	if (g_order_asked == order_id) return;
	if (std::find(g_order_questions.begin(), g_order_questions.end(), order_id) != g_order_questions.end()) return;
	HandleMilitiaOrderArrival(order_id);
}


ST::string MilitiaOrderAreaName(MilitiaOrder const& o)
{
	return RecruitAreaName(o.sector);
}


void JoinMilitiaOrderToBattle(UINT32 const order_id)
{
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end()) return;
	it->sector = gWorldSector; // the battle's sector, of the same town
	g_order_joins.push_back(order_id);
}


void HandleMilitiaOrderQuestions()
{
	if (g_order_asked != 0) return;
	// no other message box or battle panel
	if (guiCurrentScreen != MAP_SCREEN && guiCurrentScreen != GAME_SCREEN) return;
	if (gfInMsgBox || gfPreBattleInterfaceActive)                           return;

	// militia the box sent into the battle: back on the tactical screen they
	// join it, between attacks (D5.2)
	if (!g_order_joins.empty() && CanAskToJoinNow())
	{
		UINT32 const order_id = g_order_joins.front();
		g_order_joins.erase(g_order_joins.begin());
		auto const it = FindMilitiaOrder(order_id);
		if (it == g_militia_orders.end()) return;
		MilitiaOrder& o = *it;
		if (MilitiaOrderCanJoin(o))
		{
			SGPSector const sector  = o.sector;
			UINT8     const entered = AddMilitiaReinforcementsToBattle(o.counts, (UINT8)std::min<UINT32>(OrderTotal(o), 255));
			if (entered != 0)
			{
				AddMilitiaRecruitLoyalty(sector, entered);
				ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_ARRIVED], RecruitAreaName(sector)));
			}
		}
		if (OrderTotal(o) == 0) g_militia_orders.erase(it);
		else                    MilitiaOrderWaits(o); // the battle took fewer, or is over
		fMapPanelDirty = TRUE;
		return;
	}

	// the militia that arrived, after any battle
	if (!g_order_notices.empty())
	{
		if ((gTacticalStatus.uiFlags & INCOMBAT) || gfWatchingMilitiaBattle) return;
		ST::string const notice = g_order_notices.front();
		g_order_notices.erase(g_order_notices.begin());
		DoScreenIndependantMessageBox(notice, MSG_BOX_FLAG_OK, NULL);
		return;
	}
	if (g_order_questions.empty()) return;

	UINT32 const order_id = g_order_questions.front();
	auto const it = FindMilitiaOrder(order_id);
	if (it == g_militia_orders.end())
	{
		g_order_questions.erase(g_order_questions.begin());
		return;
	}

	// a town's or a SAM site's order: the militia box with the order's
	// militia as the unassigned ones, on the map screen -- also during a
	// battle, whose sector (in the box) takes reinforcements (D5.2)
	INT16 const town = MilitiaOrderBoxTown(*it);
	if (town != BLANK_SECTOR)
	{
		if (guiCurrentScreen != MAP_SCREEN)
		{
			// the tactical screen (D5.1, D5.2): to the map screen, between
			// attacks; the box opens there, closing it comes back
			if (CanAskToJoinNow()) RequestMilitiaArrivalBoxFromTactical();
			return;
		}
		if (MilitiaArrivalBoxTown() == town)
		{
			// the town's box is open: they join its unassigned militia (D7)
			AddToMilitiaArrivalBox(it->counts);
			LogMilitiaArrival(*it);
			g_militia_orders.erase(it);
			g_order_questions.erase(g_order_questions.begin());
			return;
		}
		if (!OpenMilitiaArrivalBox(order_id, town, it->counts)) return;
		it->waiting = true; // saved in the box: back as a waiting order
		LogMilitiaArrival(*it);
		g_order_questions.erase(g_order_questions.begin());
		return;
	}

	// the situation may have changed while it was queued
	UINT32 const total = OrderTotal(*it);
	UINT32 const room  = MilitiaOrderRoom(*it);
	bool   const join  = room == 0 && MilitiaOrderCanJoin(*it);
	// a battle going on: joining it is asked between attacks, the other
	// questions after it
	bool const in_battle = (gTacticalStatus.uiFlags & INCOMBAT) || gfWatchingMilitiaBattle;
	if (join ? !CanAskToJoinNow() : (in_battle && room < total)) return;

	g_order_questions.erase(g_order_questions.begin());
	if (room >= total) { EnterMilitiaOrder(order_id, total); return; }

	g_order_asked = order_id;
	if (join)
	{
		DoScreenIndependantMessageBox(
			st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_JOIN], RecruitAreaName(it->sector), it->sector.AsShortString()),
			MSG_BOX_FLAG_YESNO, JoinCallback);
	}
	else if (room > 0)
	{
		DoScreenIndependantMessageBox(
			st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_PARTIAL], int(room), int(total), RecruitAreaName(it->sector)),
			MSG_BOX_FLAG_YESNO, PartialEntryCallback);
	}
	else if (MilitiaOrderCanAttack(*it))
	{
		DoScreenIndependantMessageBox(
			st_format_printf(pMilitiaRecruitStrings[MILITIA_STR_ORDER_ATTACK], RecruitAreaName(it->sector), it->sector.AsShortString()),
			MSG_BOX_FLAG_YESNO, AttackCallback);
	}
	else
	{
		g_order_asked = 0;
		MilitiaOrderWaits(*it);
	}
}


// Saved as one list of numbers, MILITIA_ORDER_FIELDS per order
static ST::string const gMilitiaOrdersStateKey{ "Militia::orders" };
#define MILITIA_ORDER_FIELDS (4 + MAX_MILITIA_LEVELS + 2)

void InitMilitiaOrdersForNewGame()
{
	g_militia_orders.clear();
	g_order_questions.clear();
	g_order_notices.clear();
	g_order_asked = 0;
	g_order_joins.clear();
	ResetMilitiaArrivalBox();
}


void SaveMilitiaOrdersToSaveGameFile()
{
	std::vector<int32_t> v;
	for (MilitiaOrder const& o : g_militia_orders)
	{
		if (OrderTotal(o) == 0) continue; // the militia box's, all placed
		v.push_back(o.id);
		v.push_back(o.sector.AsByte());
		v.push_back(o.arrival_min);
		v.push_back(o.option);
		for (UINT8 const c : o.counts) v.push_back(c);
		v.push_back(o.waiting ? 1 : 0);
		v.push_back(0); // spare
	}
	g_gameStates.SetVector(gMilitiaOrdersStateKey, std::move(v));
}


void LoadMilitiaOrdersFromSaveGameFile()
{
	g_militia_orders.clear();
	if (!g_gameStates.HasKey(gMilitiaOrdersStateKey)) return;
	auto const v = g_gameStates.GetVector<int32_t>(gMilitiaOrdersStateKey);
	for (size_t i = 0; i + MILITIA_ORDER_FIELDS <= v.size(); i += MILITIA_ORDER_FIELDS)
	{
		MilitiaOrder o{};
		o.id          = v[i];
		o.sector      = SGPSector::FromSectorID(UINT32(v[i + 1]), 0);
		o.arrival_min = v[i + 2];
		o.option      = v[i + 3];
		for (UINT8 r = 0; r != MAX_MILITIA_LEVELS; ++r) o.counts[r] = v[i + 4 + r];
		o.waiting     = v[i + 4 + MAX_MILITIA_LEVELS] != 0;
		// arrived, but its question wasn't answered: it waits
		if (o.arrival_min <= GetWorldTotalMin()) o.waiting = true;
		g_militia_orders.push_back(o);
	}
	g_order_questions.clear();
	g_order_notices.clear();
	g_order_asked = 0;
	g_order_joins.clear();
	ResetMilitiaArrivalBox();
}
