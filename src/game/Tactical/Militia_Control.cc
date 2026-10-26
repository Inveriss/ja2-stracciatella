#include "Militia_Control.h"
#include "Campaign_Types.h"
#include "Overhead.h"
#include "Overhead_Types.h"
#include "Soldier_Control.h"
#include "Soldier_Create.h"
#include "Soldier_Init_List.h"
#include "StrategicMap.h"
#include "Town_Militia.h"
#include "Isometric_Utils.h"
#include "MapScreen.h"
#include "Strategic.h"
#include "Map_Edgepoints.h"
#include "OppList.h"
#include "Random.h"
#include "WorldDef.h"
#include <stdexcept>


BOOLEAN gfStrategicMilitiaChangesMade = FALSE;


static void RemoveMilitiaFromTactical(void);


void ResetMilitia()
{
	if( gfStrategicMilitiaChangesMade || gTacticalStatus.uiFlags & LOADING_SAVED_GAME )
	{
		gfStrategicMilitiaChangesMade = FALSE;
		RemoveMilitiaFromTactical();
		PrepareMilitiaForTactical();
	}
}


static void RemoveMilitiaFromTactical(void)
{
	TrashAllSoldiers(MILITIA_TEAM);
	FOR_EACH_SOLDIERINITNODE(curr)
	{
		if( curr->pBasicPlacement->bTeam == MILITIA_TEAM )
		{
			curr->pSoldier = NULL;
		}
	}
}

void PrepareMilitiaForTactical()
{
	SECTORINFO *pSector;
	//INT32 i;
	UINT8 ubGreen, ubRegs, ubElites, ubHeavies;
	if (gWorldSector.z > 0)
		return;

	// Do we have a loaded sector?
	if (gWorldSector.x == 0 && gWorldSector.y == 0)
		return;

	pSector = &SectorInfo[gWorldSector.AsByte()];
	ubGreen = pSector->ubNumberOfCivsAtLevel[ GREEN_MILITIA ];
	ubRegs = pSector->ubNumberOfCivsAtLevel[ REGULAR_MILITIA ];
	ubElites = pSector->ubNumberOfCivsAtLevel[ ELITE_MILITIA ];
	ubHeavies = pSector->ubNumberOfCivsAtLevel[ ELITE_HEAVY_MILITIA ];
	AddSoldierInitListMilitia( ubGreen, ubRegs, ubElites, ubHeavies );
}


UINT8 MilitiaReinforcementRoom()
{
	if (gWorldSector.z != 0) return 0;
	UINT8 const in_sector = CountAllMilitiaInSector(gWorldSector);
	UINT8 const room      = in_sector < MAX_ALLOWABLE_MILITIA_PER_SECTOR ? MAX_ALLOWABLE_MILITIA_PER_SECTOR - in_sector : 0;
	UINT8 slots = 0;
	for (INT32 i = gTacticalStatus.Team[MILITIA_TEAM].bFirstID; i <= gTacticalStatus.Team[MILITIA_TEAM].bLastID; ++i)
	{
		if (!GetMan(i).bActive) ++slots;
	}
	return std::min(room, slots);
}


// The edge the reinforcements come in from: the one facing a neighbouring
// sector of the same town, else the one farthest from the enemies (the
// shortest distance from its first edgepoint to an enemy); 0: no edgepoints
static UINT8 MilitiaReinforcementEdge()
{
	struct Dir { INT8 dx, dy; UINT8 code; };
	static Dir const dirs[] =
	{
		{  0, -1, INSERTION_CODE_NORTH },
		{  1,  0, INSERTION_CODE_EAST  },
		{  0,  1, INSERTION_CODE_SOUTH },
		{ -1,  0, INSERTION_CODE_WEST  },
	};
	INT8 const town = GetTownIdForSector(gWorldSector);

	UINT8 best       = 0;
	bool  best_town  = false;
	INT32 best_score = -1;
	for (Dir const& d : dirs)
	{
		MAPEDGEPOINTINFO info;
		ChooseMapEdgepoints(&info, d.code, 1);
		if (info.ubNumPoints == 0) continue;

		INT16 const nx = gWorldSector.x + d.dx;
		INT16 const ny = gWorldSector.y + d.dy;
		bool const same_town = town != BLANK_SECTOR && 1 <= nx && nx <= 16 && 1 <= ny && ny <= 16 &&
			GetTownIdForSector(SGPSector(nx, ny)) == town;

		INT32 score = 10000; // no enemy
		CFOR_EACH_IN_TEAM(e, ENEMY_TEAM)
		{
			if (!e->bInSector || e->bLife <= 0) continue;
			score = std::min<INT32>(score, PythSpacesAway(info.sGridNo[0], e->sGridNo));
		}

		if ((same_town && !best_town) || (same_town == best_town && score > best_score))
		{
			best       = d.code;
			best_town  = same_town;
			best_score = score;
		}
	}
	return best;
}


UINT8 AddMilitiaReinforcementsToBattle(UINT8 (&counts)[MAX_MILITIA_LEVELS], UINT8 n)
{
	static SoldierClass const rank_class[MAX_MILITIA_LEVELS] =
	{
		SOLDIER_CLASS_GREEN_MILITIA, SOLDIER_CLASS_REG_MILITIA, SOLDIER_CLASS_ELITE_MILITIA, SOLDIER_CLASS_ELITE_HEAVY_MILITIA
	};

	UINT8 const edge = MilitiaReinforcementEdge();
	if (edge == 0) return 0;
	UINT8 direction;
	switch (edge)
	{
		case INSERTION_CODE_NORTH: direction = SOUTHEAST; break;
		case INSERTION_CODE_EAST:  direction = SOUTHWEST; break;
		case INSERTION_CODE_SOUTH: direction = NORTHWEST; break;
		default:                   direction = NORTHEAST; break;
	}

	n = std::min(n, MilitiaReinforcementRoom());
	MAPEDGEPOINTINFO edgepoints;
	ChooseMapEdgepoints(&edgepoints, edge, n);

	bool const changes_before = gfStrategicMilitiaChangesMade;
	UINT8 added = 0;
	for (UINT8 rank = MAX_MILITIA_LEVELS; rank-- != 0 && added != n;)
	{
		while (counts[rank] != 0 && added != n)
		{
			if (MilitiaReinforcementRoom() == 0) break;
			SOLDIERTYPE* const s = TacticalCreateMilitia(rank_class[rank]);
			if (!s) break;
			// they come for a fight
			s->bOrders              = SEEKENEMY;
			s->bAlertStatus         = STATUS_RED;
			s->sNoiseGridno         = 13202; // the map's centre (as reinforcements of the enemy)
			s->ubNoiseVolume        = MAX_MISC_NOISE_DURATION;
			s->ubInsertionDirection = direction;
			if (added < edgepoints.ubNumPoints)
			{
				s->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
				s->usStrategicInsertionData = edgepoints.sGridNo[added];
			}
			else
			{
				s->ubStrategicInsertionCode = edge;
			}
			UpdateMercInSector(*s, gWorldSector);
			StrategicAddMilitiaToSector(gWorldSector, rank, 1);
			--counts[rank];
			++added;
		}
	}
	// the tactical militia are the strategic ones already: no reset for them
	gfStrategicMilitiaChangesMade = changes_before;
	return added;
}


void HandleMilitiaPromotions()
{
	gbGreenToElitePromotions = 0;
	gbGreenToRegPromotions   = 0;
	gbRegToElitePromotions   = 0;
	gbEliteToHeavyPromotions = 0;
	gbMilitiaPromotions      = 0;

	FOR_EACH_IN_TEAM(i, MILITIA_TEAM)
	{
		SOLDIERTYPE& s = *i;
		if (!s.bInSector)          continue;
		if (s.bLife <= 0)          continue;
		if (s.ubMilitiaKills == 0) continue;

		// Is this a regular militia or a "rebel" (the editor kind, not the
		// A10 basement kind? Only regular milita can be promoted.
		auto const rank = SoldierClassToMilitiaRank(s.ubSoldierClass);
		if (!rank) continue;
		auto militia_rank = *rank;

		UINT8 const promotions   = CheckOneMilitiaForPromotion(gWorldSector, militia_rank, s.ubMilitiaKills);
		if (promotions != 0)
		{
			++gbMilitiaPromotions;
			if (promotions == 2)
			{
				++gbGreenToElitePromotions;
			}
			else if (militia_rank == GREEN_MILITIA)
			{
				++gbGreenToRegPromotions;
			}
			else if (militia_rank == REGULAR_MILITIA)
			{
				++gbRegToElitePromotions;
			}
			else if (militia_rank == ELITE_MILITIA)
			{
				++gbEliteToHeavyPromotions;
			}

			s.ubMilitiaKills = 0;
		}
	}
}
