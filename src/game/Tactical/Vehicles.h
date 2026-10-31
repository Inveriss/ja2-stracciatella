#ifndef _VEHICLES_H
#define _VEHICLES_H

#include "JA2Types.h"
#include "Strategic_Movement.h"

#include <vector>


#define MAX_VEHICLES 10
#define MAX_PASSENGERS_IN_VEHICLE 10

// type of vehicles
enum{
	ELDORADO_CAR = 0,
	HUMMER,
	ICE_CREAM_TRUCK,
	JEEP_CAR,
	TANK_CAR,
	HELICOPTER,
	NUMBER_OF_TYPES_OF_VEHICLES,
};


// struct for vehicles
struct VEHICLETYPE
{
	PathSt  *pMercPath; // vehicle's stategic path list
	UINT8   ubMovementGroup; // the movement group this vehicle belongs to
	UINT8   ubVehicleType; // type of vehicle
	SGPSector sSector; // position on the Stategic Map
	BOOLEAN fBetweenSectors; // between sectors?
	INT16   sGridNo; // location in tactical
	SOLDIERTYPE *pPassengers[MAX_PASSENGERS_IN_VEHICLE];
	BOOLEAN fDestroyed;
	UINT32  uiMovementSoundID;
	BOOLEAN fValid;
};

#define CFOR_EACH_PASSENGER(v, iter)                                                   \
	for (SOLDIERTYPE* const* iter = (v).pPassengers,                        \
		* const* const end__##iter = (v).pPassengers + GetVehicleSeats((v)); \
		iter != end__##iter;                                                            \
		++iter)                                                                         \
		if (!*iter) continue; else


// the list of vehicles
extern std::vector<VEHICLETYPE> pVehicleList;

#define VEHICLE2ID(v) static_cast<INT32>(&(v) - pVehicleList.data())

#define BASE_FOR_EACH_VEHICLE(type, iter) \
	for (type& iter : pVehicleList) \
		if (!iter.fValid) continue; else
#define FOR_EACH_VEHICLE( iter) BASE_FOR_EACH_VEHICLE(      VEHICLETYPE, iter)
#define CFOR_EACH_VEHICLE(iter) BASE_FOR_EACH_VEHICLE(const VEHICLETYPE, iter)


void SetVehicleValuesIntoSoldierType( SOLDIERTYPE *pVehicle );

// add vehicle to list and return id value
INT32 AddVehicleToList(const SGPSector& sMap, INT16 sGridNo, UINT8 ubType);

// remove this vehicle from the list
void RemoveVehicleFromList(VEHICLETYPE&);

// clear out the vehicle list
void ClearOutVehicleList( void );

bool AnyAccessibleVehiclesInSoldiersSector(SOLDIERTYPE const&);

// is this vehicle in the same sector (not between sectors), and accesible
bool IsThisVehicleAccessibleToSoldier(SOLDIERTYPE const&, VEHICLETYPE const&);

// strategic mvt stuff
// move character path to the vehicle
BOOLEAN MoveCharactersPathToVehicle( SOLDIERTYPE *pSoldier );

// Return the vehicle, iff the vehicle ID is valid, NULL otherwise
VEHICLETYPE& GetVehicle(INT32 vehicle_id);

// Given this grunt, find out if asscoiated vehicle has a mvt group, if so,
// set this grunts mvt group to the vehicle.  For pathing purposes, will be
// reset to zero in copying of path
void SetUpMvtGroupForVehicle(SOLDIERTYPE*);

// find vehicle id of group with this vehicle
VEHICLETYPE& GetVehicleFromMvtGroup(GROUP const&);

// kill everyone in vehicle
BOOLEAN KillAllInVehicle(VEHICLETYPE const&);

// grab number of occupants in vehicles
INT32 GetNumberInVehicle(VEHICLETYPE const&);

// grab # in vehicle skipping EPCs (who aren't allowed to drive :-)
INT32 GetNumberOfNonEPCsInVehicle( INT32 iId );

BOOLEAN ExitVehicle( SOLDIERTYPE *pSoldier );
// one aboard the helicopter in the loaded sector gets out in tactical, at this
// spot, onto a squad of his own (the checks: CanExitHelicopterInTactical())
bool ExitHelicopterAt(SOLDIERTYPE&, GridNo);


void VehicleTakeDamage(UINT8 ubID, UINT8 ubReason, INT16 sDamage, INT16 sGridNo, SOLDIERTYPE* att);

// the soldiertype containing this tactical incarnation of this vehicle
SOLDIERTYPE& GetSoldierStructureForVehicle(VEHICLETYPE const&);

// does it need fixing?
bool DoesVehicleNeedAnyRepairs(VEHICLETYPE const&);

// repair the vehicle
INT8 RepairVehicle(VEHICLETYPE const&, INT8 bTotalPts, BOOLEAN* pfNothingToRepair);

// repair points that restore one condition point of this vehicle
UINT8 GetVehicleRepairPointsDivisor(VEHICLETYPE const&);


//Save all the vehicle information to the saved game file
void SaveVehicleInformationToSaveGameFile(HWFILE);

//Load all the vehicle information From the saved game file
void LoadVehicleInformationFromSavedGameFile(HWFILE, UINT32 uiSavedGameVersion);

// take soldier out of vehicle
BOOLEAN TakeSoldierOutOfVehicle( SOLDIERTYPE *pSoldier );

bool PutSoldierInVehicle(SOLDIERTYPE&, VEHICLETYPE&);

void SetVehicleSectorValues(VEHICLETYPE&, const SGPSector& sMap);

void UpdateAllVehiclePassengersGridNo( SOLDIERTYPE *pSoldier );

void LoadVehicleMovementInfoFromSavedGameFile(HWFILE);
void NewSaveVehicleMovementInfoToSavedGameFile(HWFILE);
void NewLoadVehicleMovementInfoFromSavedGameFile(HWFILE);

BOOLEAN OKUseVehicle( UINT8 ubProfile );

BOOLEAN IsRobotControllerInVehicle( INT32 iId );

void AddVehicleFuelToSave(void);

bool SoldierMustDriveVehicle(SOLDIERTYPE const&, bool trying_to_travel);

// may this vehicle drive on the tactical map now? (driving plan, stage 1:
// Hummer / ice cream truck, out of combat, fuel, someone aboard who can drive)
bool CanVehicleDriveInTactical(SOLDIERTYPE const& vs);
// is this vehicle driving on the tactical map now? (D13: nobody gets in or
// out and no items are passed then)
bool IsVehicleMoving(SOLDIERTYPE const& vs);
// the vehicle types that drive in tactical (D1: Hummer, ice cream truck)
bool VehicleDrivesInTactical(VEHICLETYPE const&);
// getting out in tactical (D15): out of combat always; in combat on our turn,
// his interrupt, and the AP (the message with explain) -- charged after
bool MayExitVehicleInTactical(SOLDIERTYPE const& s, bool explain);
void ChargeVehicleExit(SOLDIERTYPE& s);
// getting in or out of a vehicle in combat (D15, changed 2026-10-29)
#define AP_ENTER_EXIT_VEHICLE 5
void ChargeVehicleEntry(SOLDIERTYPE& s);
// the APs this merc needs to get into this vehicle: the walk up to it (his
// movement mode) plus AP_ENTER_EXIT_VEHICLE in combat; -1 if he can't get there
INT16 GetAPsToEnterVehicle(SOLDIERTYPE& s, SOLDIERTYPE const& vs, GridNo* action_pos);
// fuel one tile of driving to this gridno costs (vehicles.json
// tacticalFuelPerTile, twice off the road), and spending it (D11)
INT16 VehicleTacticalFuelCost(SOLDIERTYPE const& vs, GridNo);
void  SpendVehicleFuelInTactical(SOLDIERTYPE& vs, INT16 fuel);

bool IsEnoughSpaceInVehicle(VEHICLETYPE const&);

BOOLEAN IsSoldierInThisVehicleSquad(const SOLDIERTYPE* pSoldier, INT8 bSquadNumber);

SOLDIERTYPE*  PickRandomPassengerFromVehicle( SOLDIERTYPE *pSoldier );

bool DoesVehicleGroupHaveAnyPassengers(GROUP const&);

void SetSoldierExitHelicopterInsertionData(SOLDIERTYPE*);
// Takes one aboard the helicopter out onto a helicopter run's rope
// (INSERTION_CODE_CHOPPER -> AddMercToHeli(), placed by HandleHeliDrop()),
// into a squad of his own
bool PutHelicopterPassengerOnRope(SOLDIERTYPE&);

void HandleVehicleMovementSound(const SOLDIERTYPE*, BOOLEAN fOn);

UINT16 GetVehicleArmour(UINT8 vehicle_id);

UINT8 GetVehicleSeats(VEHICLETYPE const&);

// The helicopter's soldier: strategic only (team list row, condition), it never
// enters tactical
bool IsHelicopterSoldier(SOLDIERTYPE const&);
SOLDIERTYPE* GetHelicopterSoldier();
// The helicopter's cargo has 8 slots: small pockets 1-4 and big pockets 1-4
bool IsHelicopterCargoSlot(INT8 slot);

void InitVehicles(void);

#endif
