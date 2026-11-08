#ifndef __INVENTORY_CHOOSING_H
#define __INVENTORY_CHOOSING_H

#include "Types.h"
#include "Soldier_Create.h"
#include "GamePolicy.h"

#include <vector>

#define MORTAR_GRENADE_CLASS	100


enum
{
	//administrator ratings (1-5)
	BAD_ADMINISTRATOR_EQUIPMENT_RATING = 1,
	POOR_ADMINISTRATOR_EQUIPMENT_RATING,
	AVERAGE_ADMINISTRATOR_EQUIPMENT_RATING,
	GOOD_ADMINISTRATOR_EQUIPMENT_RATING,
	GREAT_ADMINISTRATOR_EQUIPMENT_RATING,
	//army ratings (3-7)
	BAD_ARMY_EQUIPMENT_RATING = AVERAGE_ADMINISTRATOR_EQUIPMENT_RATING,
	POOR_ARMY_EQUIPMENT_RATING,
	AVERAGE_ARMY_EQUIPMENT_RATING,
	GOOD_ARMY_EQUIPMENT_RATING,
	GREAT_ARMY_EQUIPMENT_RATING,
	//elite ratings (5-9)
	BAD_ELITE_EQUIPMENT_RATING = AVERAGE_ARMY_EQUIPMENT_RATING,
	POOR_ELITE_EQUIPMENT_RATING,
	AVERAGE_ELITE_EQUIPMENT_RATING,
	GOOD_ELITE_EQUIPMENT_RATING,
	GREAT_ELITE_EQUIPMENT_RATING,

	// militia ratings (1-5)
	BAD_MILITIA_EQUIPMENT_RATING = BAD_ADMINISTRATOR_EQUIPMENT_RATING,
	POOR_MILITIA_EQUIPMENT_RATING,
	AVERAGE_MILITIA_EQUIPMENT_RATING,
	GOOD_MILITIA_EQUIPMENT_RATING,
	GREAT_MILITIA_EQUIPMENT_RATING,
};

#define MIN_EQUIPMENT_CLASS	1
#define MAX_EQUIPMENT_CLASS	11


// Selects at the start of the game the set of guns the Queen's army will use during this game
void InitArmyGunTypes(void);

//Chooses equipment based on the equipment level (1-11) with best being 11.  It allocates a range
//of equipment to choose from.
void GenerateRandomEquipment( SOLDIERCREATE_STRUCT *pp, INT8 bSoldierClass, INT8 bEquipmentRating);

void AssignCreatureInventory( SOLDIERTYPE *pSoldier );

void ReplaceExtendedGuns( SOLDIERCREATE_STRUCT *pp, INT8 bSoldierClass );

void ResetMortarsOnTeamCount( void );

// An enemy soldier keeps his armour and other gear but loses every weapon, ammo
// and explosive; instead he gets the heavy weapon (ROCKET_LAUNCHER or MORTAR) in
// the hand and its ammunition (more LAWs, mortar shells) in his empty big
// pockets, as much as his strength carries (100%).  Everything at 100%.
void GiveEnemyOnlyHeavyWeapon(SOLDIERTYPE& s, UINT16 usWeapon);


// --- Enemy equipment by game progress (docs/enemy-equipment-progress-plan.md) ---

// bRelativeEquipmentLevel of a strategically created soldier (no editor level, E8)
#define NO_EQUIPMENT_LEVEL (-1)

// game.json enemy_equipment.by_progress
bool EnemyEquipmentByProgress();

// The random offset range of a soldier: by editor equipment level 0-4 (E7), else by
// soldier type (E8; militia: green = admin, regular = army, elite = elite, E10)
GamePolicy::OffsetRange EnemyEquipmentOffsetRange(INT8 bSoldierClass, INT8 bEquipmentLevel);

// The difficulty offset (E9): enemies only, 0 for militia
INT8 EnemyEquipmentDifficultyOffset(INT8 bSoldierClass, UINT8 ubDifficultyLevel);

// P = progress + offset + difficulty offset, clamped to 0..100 (E6)
UINT8 EnemyEquipmentProgress(UINT8 ubGameProgress, INT8 iOffset, INT8 iDifficultyOffset);

// The class 1..11 that counts and chances keep using (E13)
INT8 EnemyEquipmentClass(UINT8 ubProgress);

// Tier rule (E12): the highest tier <= progress, else the lowest tier; -1 when there is none
int ChooseEnemyEquipmentTier(std::vector<UINT8> const& tiers, int progress);

// Stage 3: a random gun of the tier chosen for this progress, among the guns with
// minEnemyProgress; NOTHING when no gun has one
UINT16 SelectEnemyGunByProgress(int progress);

// Stage 3 (O3): the guns of the lowest gun tier, marked as already dropped at the start
// of a game so that the crappiest guns get no first-drop bonus
std::vector<UINT16> LowestEnemyGunTier();

#endif
