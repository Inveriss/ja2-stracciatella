#pragma once
#include "Observable.h"
#include "Types.h"
class DataReader;
class DataWriter;
class StrategicMapSecretModel;

extern Observable<UINT8> OnMapSecretFound;

// Reset the state of all map secrets
void InitMapSecrets();

// TEMPORARY FOR TESTING: every surface sector visited, every map secret found
// (called for a new game and after loading a game while this is enabled)
#define REVEAL_WHOLE_MAP_FOR_TESTING 1
void RevealWholeMapForTesting();

// TEMPORARY FOR TESTING: on Expert the counterattack after the capture of the
// D15 SAM site is 20 elites instead of 6 (+ progress bonus); in tactical 4 of
// them carry only LAWs and 3 only a mortar with shells (Strategic_AI.cc,
// Queen_Command.cc, Inventory_Choosing.cc)
#define SAM_D15_COUNTERATTACK_FOR_TESTING 1

// Whether the town was found by the player. Returns TRUE if the town is not hidden (i.e. not defined in map secrets)
BOOLEAN IsTownFound(INT8 bTownID);

// Whether the map secret was known to the player. Returns TRUE with a warning, if there is no secret at the sector ID.
BOOLEAN IsSecretFoundAt(UINT8 sectorID);

// Marks the town's base sector as a found secret
void SetTownAsFound(INT8 bTownID, BOOLEAN fFound = TRUE);

// set this SAM site as being found by the player
void SetSAMSiteAsFound(UINT8 uiSamIndex);

// Marks a sector's secret as found
void SetSectorSecretAsFound(UINT8 ubSectorID, BOOLEAN fFound = TRUE);

const StrategicMapSecretModel* GetMapSecretBySectorID(UINT8 ubSectorID);

// writes the map secret states of all SAM sites
void InjectSAMSitesFoundToSavedFile(DataWriter& d);

// reads the map secret states for all SAM sites
void ExtractSAMSitesFoundFromSavedFile(DataReader& d);

// write the state of a non-SAM-site map secret at the given index
void InjectMapSecretStateToSave(DataWriter& d, unsigned int index);

// Gets a non-SAM-site map secret state by index
BOOLEAN GetMapSecretStateForSave(unsigned int index);

// reads a non-SAM-site map secret state from save
void ExtractMapSecretStateFromSave(DataReader & d, unsigned int index);

// Sets a non-SAM-site map secret state by index
void SetMapSecretStateFromSave(unsigned int index, BOOLEAN fFound);