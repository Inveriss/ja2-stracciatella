#ifndef _TOWN_MILITIA_H
#define _TOWN_MILITIA_H

// header for town militia strategic control module

#include "JA2Types.h"
#include "Campaign_Types.h"

#include <string_theory/string>
#include <optional>
#include <vector>


// how many militia of all ranks can be in any one sector at once
#define MAX_ALLOWABLE_MILITIA_PER_SECTOR 20

// how many new green militia civilians are trained at a time
#define MILITIA_TRAINING_SQUAD_SIZE		10		// was 6

// cost of starting a new militia training assignment
#define MILITIA_TRAINING_COST		750

// minimum loyalty rating before training is allowed in a town
#define MIN_RATING_TO_TRAIN_TOWN 20



// this handles what happens when a new militia unit is finishes getting trained
void TownMilitiaTrainingCompleted(SOLDIERTYPE *pTrainer, const SGPSector& sector);

// Given a SOLDIER_CLASS_ returns a _MITILIA rank if it is militia
std::optional<UINT8> SoldierClassToMilitiaRank(UINT8 soldier_class);

// remove militias of a certain rank
void StrategicRemoveMilitiaFromSector(const SGPSector& sMap, UINT8 ubRank, UINT8 ubHowMany);

// Check for promotions and handle them
UINT8 CheckOneMilitiaForPromotion(const SGPSector& sMap, UINT8 &current_rank, UINT8 kill_points);

ST::string BuildMilitiaPromotionsString();

UINT8 CountAllMilitiaInSector(const SGPSector& sMap);
UINT8 MilitiaInSectorOfRank(const SGPSector& sMap, UINT8 ubRank);

// Returns TRUE if sector is under player control, has no enemies in it, and isn't currently in combat mode
BOOLEAN SectorOursAndPeaceful(const SGPSector& sector);

// tell player how much it will cost
void HandleInterfaceMessageForCostOfTrainingMilitia( SOLDIERTYPE *pSoldier );

// call this when the sector changes...
void HandleMilitiaStatusInCurrentMapBeforeLoadingNewMap( void );

// Is there a town with militia here or nearby?
bool CanNearbyMilitiaScoutThisSector(const SGPSector& sMap);

// Is the town or SAM site here full of milita?
BOOLEAN IsAreaFullOfMilitia(const SGPSector& sector);

// now that town training is complete, handle the continue boxes
void HandleContinueOfTownTraining( void );

// clear the list of training completed sectors
void ClearSectorListForCompletedTrainingOfMilitia();

BOOLEAN MilitiaTrainingAllowedInSector(const SGPSector& sMap);
BOOLEAN MilitiaTrainingAllowedInTown( INT8 bTownId );

void AddSectorForSoldierToListOfSectorsThatCompletedMilitiaTraining(SOLDIERTYPE* pSoldier);


// Militia recruitment (game.json "militia_recruitment"): militia are bought
// directly in the recruitment panel instead of being trained.

// Recruitment is on (and the militia training off)
bool IsMilitiaRecruitmentEnabled();

// How many more militia fit into the town (or SAM site) of this sector: the
// room left in its sectors that are ours and peaceful
UINT32 MilitiaRecruitFreeSlots(const SGPSector& sector);

// The merc with the best leadership in the town (or SAM site) of this sector;
// NULL if there is none
SOLDIERTYPE const* BestMilitiaRecruiter(const SGPSector& sector);

// The price modifier (percent of the base price) for this leadership
INT32 MilitiaRecruitPricePercent(INT8 leadership);

// The price of one militia of this rank, with the best recruiter's modifier
INT32 MilitiaRecruitUnitPrice(UINT8 rank, const SGPSector& sector);

// The arrival options of a purchase (docs/militia-arrival-time-plan.md): how
// soon the militia arrive and the extra cost of it, per purchase
enum MilitiaArrivalOption
{
	MILITIA_ARRIVAL_EXPRESS,  // 6 hours, + $1000
	MILITIA_ARRIVAL_FAST,     // 12 hours, + $500
	MILITIA_ARRIVAL_STANDARD, // 24 hours
	NUM_MILITIA_ARRIVAL_OPTIONS
};
UINT8 MilitiaArrivalHours(UINT8 option);
INT32 MilitiaArrivalExtraCost(UINT8 option);

// A purchase on its way: the militia arrive at arrival_min (the event
// EVENT_MILITIA_ORDER_ARRIVAL with the order's id). Waiting: they arrived but
// couldn't enter the town (enemy, no room); they wait for orders.
struct MilitiaOrder
{
	UINT32    id;
	SGPSector sector; // the sector the purchase was made for
	UINT8     counts[MAX_MILITIA_LEVELS];
	UINT32    arrival_min;
	UINT8     option; // MilitiaArrivalOption
	bool      waiting;
};
std::vector<MilitiaOrder> const& GetMilitiaOrders();

// Buys counts[rank] militia of each rank for the town (or SAM site) of this
// sector: checks money and room (the militia ordered before count as there),
// pays (with the arrival option's extra cost) and orders them -- they arrive
// after the option's hours (HandleMilitiaOrderArrival()). FALSE (nothing
// bought) when it can't.
BOOLEAN BuyMilitia(const SGPSector& sector, UINT8 const (&counts)[MAX_MILITIA_LEVELS], UINT8 arrival_option);

// EVENT_MILITIA_ORDER_ARRIVAL: the order's militia enter the town -- the
// ordered sector first, then the town's other sectors with room -- and the
// town loyalty bonus counts them. When only some fit, the player is asked
// whether they enter and the rest waits; when none fits, the order waits.
void HandleMilitiaOrderArrival(UINT32 order_id);

// Every map screen and tactical frame: asks the queued questions of arriving
// orders, one at a time, when nothing else is going on (no message box,
// battle or pre-battle panel)
void HandleMilitiaOrderQuestions();

// The orders in the saved game (g_gameStates); none in a new game
void InitMilitiaOrdersForNewGame();
void SaveMilitiaOrdersToSaveGameFile();
void LoadMilitiaOrdersFromSaveGameFile();

extern SOLDIERTYPE* pMilitiaTrainerSoldier;

#endif
