#ifdef WITH_UNITTESTS

// docs/enemy-equipment-progress-plan.md, stages 2-5: effective progress P, the tier rule, guns, other equipment and grenades by tiers

#include "DefaultContentManagerUT.h"
#include "GameSettings.h"
#include "Inventory_Choosing.h"
#include "ItemModel.h"
#include "Item_Types.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "Soldier_Control.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <initializer_list>
#include <set>
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
	for (bool const lowestWhenBelow : { true, false })
	{
		EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 10, lowestWhenBelow), 10);
		EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 35, lowestWhenBelow), 20);
		EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 40, lowestWhenBelow), 40);
		EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 100, lowestWhenBelow), 70); // too high: the first lower tier
	}
}


TEST(EnemyEquipmentTier, belowTheLowestTierTakesTheLowest)
{
	// guns and launcher ammunition (docs/enemy-equipment-below-lowest-tier-plan.md, B1/B2)
	std::vector<UINT8> const tiers{ 30, 50 };
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 0, true), 30);
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 25, true), 30);
}


TEST(EnemyEquipmentTier, belowTheLowestTierGivesNone)
{
	// all other equipment (variant A)
	std::vector<UINT8> const tiers{ 30, 50 };
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 0, false), -1);
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 25, false), -1);
	EXPECT_EQ(ChooseEnemyEquipmentTier(tiers, 30, false), 30);
}


TEST(EnemyEquipmentTier, noTiers)
{
	EXPECT_EQ(ChooseEnemyEquipmentTier({}, 50, true), -1);
	EXPECT_EQ(ChooseEnemyEquipmentTier({}, 50, false), -1);
}


// --- stage 3: guns by tiers ---

namespace {

UINT16 Id(char const* name)
{
	return GCM->getItemByName(name)->getItemIndex();
}

std::set<UINT16> Ids(std::initializer_list<char const*> names)
{
	std::set<UINT16> ids;
	for (char const* n : names) ids.insert(Id(n));
	return ids;
}

std::set<UINT16> Draw(int const progress)
{
	std::set<UINT16> guns;
	for (int i = 0; i < 300; ++i) guns.insert(SelectEnemyGunByProgress(progress));
	return guns;
}

}


TEST_F(EnemyEquipment, gunsOfTheChosenTierOnly)
{
	EXPECT_EQ(Draw(0),   Ids({ "SW38", "BARRACUDA", "DESERTEAGLE" }));      // below the lowest: lowest tier 10
	EXPECT_EQ(Draw(35),  Ids({ "MP5K", "THOMPSON", "MP53", "TYPE85", "M870" })); // tier 30
	EXPECT_EQ(Draw(100), Ids({ "MINIMI", "RPK74", "HK21E", "ROCKET_RIFLE" })); // tier 100
}


TEST_F(EnemyEquipment, gunsWithoutMinEnemyProgressNeverChosen)
{
	for (int p = 0; p <= 100; p += 5)
	{
		for (UINT16 const gun : Draw(p))
		{
			EXPECT_TRUE(GCM->getItem(gun)->hasMinEnemyProgress()) << p;
			EXPECT_LE(GCM->getItem(gun)->getMinEnemyProgress(), std::max(p, 10)) << p;
		}
	}
}


TEST_F(EnemyEquipment, lowestGunTierHasNoFirstDropBonus)
{
	std::vector<UINT16> const lowest = LowestEnemyGunTier();
	EXPECT_EQ(std::set<UINT16>(lowest.begin(), lowest.end()), Ids({ "SW38", "BARRACUDA", "DESERTEAGLE" }));
}


// --- stage 4: other equipment by tiers ---

namespace {

std::set<UINT16> DrawItems(std::vector<UINT16> const& candidates, int const progress, bool const lowestWhenBelow = false)
{
	std::set<UINT16> items;
	for (int i = 0; i < 300; ++i) items.insert(PickEnemyItemByProgress(candidates, progress, lowestWhenBelow));
	return items;
}

std::vector<UINT16> Vec(std::initializer_list<char const*> names)
{
	std::vector<UINT16> v;
	for (char const* n : names) v.push_back(Id(n));
	return v;
}

}


TEST_F(EnemyEquipment, vestsByTiers)
{
	auto const vests = Vec({ "FLAK_JACKET", "KEVLAR_VEST", "KEVLAR2_VEST", "SPECTRA_VEST", "TSHIRT", "LEATHER_JACKET" });
	EXPECT_EQ(DrawItems(vests, 0),   std::set<UINT16>{ NOTHING }); // below the lowest tier 20: none
	EXPECT_EQ(DrawItems(vests, 20),  Ids({ "FLAK_JACKET" }));  // lowest tier 20; t-shirts never
	EXPECT_EQ(DrawItems(vests, 45),  Ids({ "KEVLAR_VEST" }));  // tier 40
	EXPECT_EQ(DrawItems(vests, 100), Ids({ "SPECTRA_VEST" })); // tier 80
}


TEST_F(EnemyEquipment, repeatedCandidatesOfOneTier)
{
	// the misc list: CANTEEN x4, ALCOHOL x2 ... all tier 10 except the boosters, knuckles, watch
	auto const misc = Vec({ "CANTEEN", "CANTEEN", "CANTEEN", "CANTEEN", "ALCOHOL", "ALCOHOL",
		"ADRENALINE_BOOSTER", "ADRENALINE_BOOSTER", "BRASS_KNUCKLES", "CHEWING_GUM", "CIGARS", "GOLDWATCH" });
	EXPECT_EQ(DrawItems(misc, 15), Ids({ "CANTEEN", "ALCOHOL", "CHEWING_GUM", "CIGARS" }));
	EXPECT_EQ(DrawItems(misc, 25), Ids({ "BRASS_KNUCKLES", "GOLDWATCH" }));
	EXPECT_EQ(DrawItems(misc, 90), Ids({ "ADRENALINE_BOOSTER" }));
}


TEST_F(EnemyEquipment, candidatesWithoutMinEnemyProgressNeverPicked)
{
	EXPECT_EQ(PickEnemyItemByProgress(Vec({ "TOOLKIT", "TSHIRT", "DETONATOR" }), 50), NOTHING);
	EXPECT_EQ(PickEnemyItemByProgress({}, 50), NOTHING);
	EXPECT_EQ(DrawItems(Vec({ "TOOLKIT", "LOCKSMITHKIT", "CAMOUFLAGEKIT" }), 0), std::set<UINT16>{ NOTHING }); // lowest tier 30
	EXPECT_EQ(DrawItems(Vec({ "TOOLKIT", "LOCKSMITHKIT", "CAMOUFLAGEKIT" }), 30), Ids({ "LOCKSMITHKIT" }));
}


// --- stage 5: grenades by tiers ---

namespace {

// the candidates that can be picked at all (with minEnemyProgress)
std::set<UINT16> Pickable(std::vector<UINT16> const& candidates)
{
	std::set<UINT16> ids;
	for (UINT16 const c : candidates)
	{
		if (GCM->getItem(c)->hasMinEnemyProgress()) ids.insert(c);
	}
	return ids;
}

}


TEST_F(EnemyEquipment, grenadeCandidatesByLauncher)
{
	auto const hand = EnemyGrenadeCandidates(NOTHING);
	EXPECT_EQ(Pickable(hand), Ids({ "STUN_GRENADE", "TEARGAS_GRENADE",
		"MUSTARD_GRENADE", "MINI_GRENADE", "HAND_GRENADE", "BREAK_LIGHT", "SMOKE_GRENADE" }));

	auto const gl = EnemyGrenadeCandidates(Id("GLAUNCHER"));
	EXPECT_EQ(Pickable(gl), Ids({ "GL_HE_GRENADE", "GL_TEARGAS_GRENADE",
		"GL_STUN_GRENADE", "GL_SMOKE_GRENADE" }));

	auto const mortar = EnemyGrenadeCandidates(Id("MORTAR"));
	EXPECT_EQ(Pickable(mortar), Ids({ "MORTAR_SHELL" }));
}


TEST_F(EnemyEquipment, grenadesByTiers)
{
	auto const hand = EnemyGrenadeCandidates(NOTHING);
	EXPECT_EQ(DrawItems(hand, 0),   std::set<UINT16>{ NOTHING });              // below the lowest tier 30: none
	EXPECT_EQ(DrawItems(hand, 30),  Ids({ "BREAK_LIGHT" }));                   // lowest tier 30
	EXPECT_EQ(DrawItems(hand, 65),  Ids({ "STUN_GRENADE", "MINI_GRENADE" }));  // tier 60
	EXPECT_EQ(DrawItems(hand, 100), Ids({ "MUSTARD_GRENADE" }));               // tier 80

	// a grenade launcher always gets something to fire (B2)
	auto const gl = EnemyGrenadeCandidates(Id("GLAUNCHER"));
	EXPECT_EQ(DrawItems(gl, 0, true),   Ids({ "GL_STUN_GRENADE" }));           // lowest tier 50
	EXPECT_EQ(DrawItems(gl, 100, true), Ids({ "GL_HE_GRENADE" }));             // tier 80

	auto const mortar = EnemyGrenadeCandidates(Id("MORTAR"));
	EXPECT_EQ(DrawItems(mortar, 0, true), Ids({ "MORTAR_SHELL" }));            // tier 90
}

#endif
