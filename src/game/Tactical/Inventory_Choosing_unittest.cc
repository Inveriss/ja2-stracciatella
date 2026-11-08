#ifdef WITH_UNITTESTS

// docs/enemy-equipment-progress-plan.md, stage 2: effective progress P and the tier rule

#include "DefaultContentManagerUT.h"
#include "GameSettings.h"
#include "Inventory_Choosing.h"
#include "Soldier_Control.h"

#include "gtest/gtest.h"

#include <vector>

namespace {

using EnemyEquipment = DefaultContentManagerUT::BaseTest;

void ExpectRange(GamePolicy::OffsetRange const r, int const min, int const max)
{
	EXPECT_EQ(r.min, min);
	EXPECT_EQ(r.max, max);
}

}


TEST_F(EnemyEquipment, onByDefault)
{
	EXPECT_TRUE(EnemyEquipmentByProgress());
}


TEST_F(EnemyEquipment, editorLevelOffsets)
{
	// E7: the editor level decides, whatever the soldier type
	for (INT8 const sc : { SOLDIER_CLASS_ADMINISTRATOR, SOLDIER_CLASS_ELITE, SOLDIER_CLASS_GREEN_MILITIA })
	{
		ExpectRange(EnemyEquipmentOffsetRange(sc, 0), -20, -10); // bad
		ExpectRange(EnemyEquipmentOffsetRange(sc, 1), -10,   0); // poor
		ExpectRange(EnemyEquipmentOffsetRange(sc, 2), -10,  10); // average
		ExpectRange(EnemyEquipmentOffsetRange(sc, 3),   0,  10); // good
		ExpectRange(EnemyEquipmentOffsetRange(sc, 4),  10,  20); // great
	}
}


TEST_F(EnemyEquipment, typeOffsetsWithoutEditorLevel)
{
	// E8, E10
	ExpectRange(EnemyEquipmentOffsetRange(SOLDIER_CLASS_ADMINISTRATOR, NO_EQUIPMENT_LEVEL), -20, -10);
	ExpectRange(EnemyEquipmentOffsetRange(SOLDIER_CLASS_ARMY,          NO_EQUIPMENT_LEVEL), -10,  10);
	ExpectRange(EnemyEquipmentOffsetRange(SOLDIER_CLASS_ELITE,         NO_EQUIPMENT_LEVEL),   0,  20);
	ExpectRange(EnemyEquipmentOffsetRange(SOLDIER_CLASS_GREEN_MILITIA, NO_EQUIPMENT_LEVEL), -20, -10);
	ExpectRange(EnemyEquipmentOffsetRange(SOLDIER_CLASS_REG_MILITIA,   NO_EQUIPMENT_LEVEL), -10,  10);
	ExpectRange(EnemyEquipmentOffsetRange(SOLDIER_CLASS_ELITE_MILITIA, NO_EQUIPMENT_LEVEL),   0,  20);
}


TEST_F(EnemyEquipment, difficultyOffsetForEnemiesOnly)
{
	// E9, E10
	EXPECT_EQ(EnemyEquipmentDifficultyOffset(SOLDIER_CLASS_ARMY, DIF_LEVEL_EASY),   -10);
	EXPECT_EQ(EnemyEquipmentDifficultyOffset(SOLDIER_CLASS_ARMY, DIF_LEVEL_MEDIUM),   0);
	EXPECT_EQ(EnemyEquipmentDifficultyOffset(SOLDIER_CLASS_ELITE, DIF_LEVEL_HARD),   10);
	EXPECT_EQ(EnemyEquipmentDifficultyOffset(SOLDIER_CLASS_ADMINISTRATOR, DIF_LEVEL_HARD), 10);
	for (INT8 const sc : { SOLDIER_CLASS_GREEN_MILITIA, SOLDIER_CLASS_REG_MILITIA, SOLDIER_CLASS_ELITE_MILITIA })
	{
		EXPECT_EQ(EnemyEquipmentDifficultyOffset(sc, DIF_LEVEL_EASY), 0);
		EXPECT_EQ(EnemyEquipmentDifficultyOffset(sc, DIF_LEVEL_HARD), 0);
	}
}


TEST(EnemyEquipmentProgressValue, sumIsClamped)
{
	// E6
	EXPECT_EQ(EnemyEquipmentProgress(40, 15, -10), 45);
	EXPECT_EQ(EnemyEquipmentProgress(5, -20, -10), 0);
	EXPECT_EQ(EnemyEquipmentProgress(95, 20, 10), 100);
}


TEST(EnemyEquipmentProgressValue, classForCountsAndChances)
{
	// E13
	EXPECT_EQ(EnemyEquipmentClass(0), 1);
	EXPECT_EQ(EnemyEquipmentClass(9), 1);
	EXPECT_EQ(EnemyEquipmentClass(10), 2);
	EXPECT_EQ(EnemyEquipmentClass(55), 6);
	EXPECT_EQ(EnemyEquipmentClass(100), 11);
}


TEST(EnemyEquipmentTier, highestTierAtOrBelowProgress)
{
	// E12
	std::vector<UINT8> const tiers{ 20, 10, 40, 40, 70 };
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 10), 10);
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 35), 20);
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 40), 40);
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 100), 70); // too high: the first lower tier
}


TEST(EnemyEquipmentTier, belowTheLowestTierTakesTheLowest)
{
	std::vector<UINT8> const tiers{ 30, 50 };
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 0), 30);
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 25), 30);
}


TEST(EnemyEquipmentTier, noTiers)
{
	EXPECT_EQ(ChooseEnemyEquipmentTier({}, 50), -1);
}

#endif
