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
#include "Soldier_Create.h"
#include "StrategicMap.h"
#include "Items.h"
#include "MagazineModel.h"
#include "AmmoTypeModel.h"
#include "WeaponModels.h"

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


// --- docs/enemy-elite-heavy-plan.md, section 3: Elite Heavy count ---

TEST(EnemyEliteHeavy, formulaTable)
{
	struct { UINT8 elites; UINT8 novice, experienced, expert; } const rows[] =
	{
		{  0, 0, 0, 0 }, {  4, 0, 0, 0 },
		{  5, 0, 1, 2 }, {  9, 0, 1, 2 },
		{ 10, 1, 2, 3 }, { 14, 1, 2, 3 },
		{ 15, 2, 3, 4 }, { 19, 2, 3, 4 },
		{ 20, 3, 4, 5 },
	};
	for (auto const& r : rows)
	{
		EXPECT_EQ(EnemyEliteHeavies(r.elites, 5, -1), r.novice)      << int(r.elites);
		EXPECT_EQ(EnemyEliteHeavies(r.elites, 5,  0), r.experienced) << int(r.elites);
		EXPECT_EQ(EnemyEliteHeavies(r.elites, 5,  1), r.expert)      << int(r.elites);
	}
}


TEST(EnemyEliteHeavy, neverBelowZeroNorAboveTheElites)
{
	EXPECT_EQ(EnemyEliteHeavies(5, 5, -3), 0);
	EXPECT_EQ(EnemyEliteHeavies(5, 5, 10), 5);
	EXPECT_EQ(EnemyEliteHeavies(1, 1, 3), 1);
	EXPECT_EQ(EnemyEliteHeavies(3, 0, 1), 0); // no divisor: none
}


namespace {

// a batch of new elites under a running count: how many became Elite Heavy
int Batch(int const elites)
{
	int heavies = 0;
	for (int i = 0; i < elites; ++i) heavies += TakeEnemyEliteHeavySlot() ? 1 : 0;
	return heavies;
}

struct Difficulty
{
	UINT8 const old = gGameOptions.ubDifficultyLevel;
	explicit Difficulty(UINT8 const d) { gGameOptions.ubDifficultyLevel = d; }
	~Difficulty() { gGameOptions.ubDifficultyLevel = old; EndEnemyEliteHeavyCount(); }
};

}


TEST_F(EnemyEquipment, eliteHeavyOnInGameJson)
{
	EXPECT_TRUE(gamepolicy(enemy_elite_heavy_enabled));
	EXPECT_EQ(gamepolicy(enemy_elite_heavy_elites_per_heavy), 5);
	EXPECT_EQ(gamepolicy(enemy_elite_heavy_difficulty_modifiers)[0], -1);
	EXPECT_EQ(gamepolicy(enemy_elite_heavy_difficulty_modifiers)[1],  0);
	EXPECT_EQ(gamepolicy(enemy_elite_heavy_difficulty_modifiers)[2],  1);
}


TEST_F(EnemyEquipment, eliteHeavyRunningCount)
{
	Difficulty const d(DIF_LEVEL_MEDIUM);

	// no count running: nobody is converted
	EndEnemyEliteHeavyCount();
	EXPECT_FALSE(TakeEnemyEliteHeavySlot());

	// defenders: 12 elites -> 2 Elite Heavy; 3 more arrive (15) -> 1 more (N8)
	BeginEnemyEliteHeavyCount(0, 0);
	EXPECT_EQ(Batch(12), 2);
	EndEnemyEliteHeavyCount();
	BeginEnemyEliteHeavyCount(12, 2);
	EXPECT_EQ(Batch(3), 1);
	EndEnemyEliteHeavyCount();

	// existing soldiers count too (autoresolve reuses the tactical ones)
	BeginEnemyEliteHeavyCount(0, 0);
	for (int i = 0; i < 5; ++i) CountExistingEnemyElite(i == 0);
	EXPECT_EQ(Batch(4), 0); // 9 elites: 1 Elite Heavy, already there
	EXPECT_EQ(Batch(1), 1); // 10 elites: 2
	EndEnemyEliteHeavyCount();
}


TEST_F(EnemyEquipment, eliteHeavyRunningCountByDifficulty)
{
	{
		Difficulty const d(DIF_LEVEL_EASY);
		BeginEnemyEliteHeavyCount(0, 0);
		EXPECT_EQ(Batch(9), 0);
		EXPECT_EQ(Batch(1), 1); // 10 elites
	}
	{
		Difficulty const d(DIF_LEVEL_HARD);
		BeginEnemyEliteHeavyCount(0, 0);
		EXPECT_EQ(Batch(4), 0);
		EXPECT_EQ(Batch(5), 2); // 9 elites: 2
	}
}


TEST_F(EnemyEquipment, eliteHeavyBatchKnownInAdvance)
{
	{
		// Expert, exactly 5 elites: 2 Elite Heavy (the count jumps by two), the first two
		Difficulty const d(DIF_LEVEL_HARD);
		BeginEnemyEliteHeavyCount(0, 0, 5);
		EXPECT_TRUE(TakeEnemyEliteHeavySlot());
		EXPECT_TRUE(TakeEnemyEliteHeavySlot());
		EXPECT_EQ(Batch(3), 0);
	}
	{
		// Experienced, 8 elites: exactly 1 (already the first)
		Difficulty const d(DIF_LEVEL_MEDIUM);
		BeginEnemyEliteHeavyCount(0, 0, 8);
		EXPECT_TRUE(TakeEnemyEliteHeavySlot());
		EXPECT_EQ(Batch(7), 0);
	}
	{
		// 9 there with 1 Elite Heavy, 1 arrives: 10 = 2 for the sector, the arriving one
		Difficulty const d(DIF_LEVEL_MEDIUM);
		BeginEnemyEliteHeavyCount(9, 1, 1);
		EXPECT_EQ(Batch(1), 1);
	}
	{
		// more come than announced: the running formula still applies
		Difficulty const d(DIF_LEVEL_MEDIUM);
		BeginEnemyEliteHeavyCount(0, 0, 4);
		EXPECT_EQ(Batch(4), 0);
		EXPECT_EQ(Batch(1), 1); // 5th
	}
}


TEST_F(EnemyEquipment, eliteHeavyBlockedElitesStillCount)
{
	// 8 elites, 4 of them on editor placements (counted, never converted): still 1
	Difficulty const d(DIF_LEVEL_MEDIUM);
	BeginEnemyEliteHeavyCount(0, 0);
	for (int i = 0; i < 4; ++i) CountExistingEnemyElite(false);
	EXPECT_EQ(Batch(4), 1);
}


// --- docs/enemy-elite-heavy-plan.md, section 6: the fixed kit ---

namespace {

int CountOf(SOLDIERCREATE_STRUCT const& pp, UINT16 const item)
{
	int n = 0;
	for (OBJECTTYPE const& o : pp.Inv)
	{
		if (o.usItem == item) n += o.ubNumberOfObjects;
	}
	return n;
}

void ExpectCommonKit(SOLDIERCREATE_STRUCT const& pp)
{
	EXPECT_EQ(pp.Inv[HELMETPOS].usItem, SPECTRA_HELMET);
	EXPECT_EQ(pp.Inv[VESTPOS].usItem,   SPECTRA_VEST);
	EXPECT_EQ(pp.Inv[LEGPOS].usItem,    SPECTRA_LEGGINGS);
	EXPECT_EQ(pp.Inv[HEAD1POS].usItem,  GASMASK);
	EXPECT_EQ(pp.Inv[HEAD2POS].usItem,  UVGOGGLES);
	EXPECT_TRUE(FindAttachment(&pp.Inv[VESTPOS], CERAMIC_PLATES) != ITEM_NOT_FOUND);
	for (int i = 0; i < NUM_INV_SLOTS; ++i)
	{
		// O2; a magazine's status is its rounds, not a condition
		if (pp.Inv[i].usItem == NOTHING || GCM->getItem(pp.Inv[i].usItem)->getItemClass() == IC_AMMO) continue;
		EXPECT_EQ(pp.Inv[i].bStatus[0], 100) << i;
	}

	// the Minimi, loaded, with two magazines; after the heavy weapon's pockets
	EXPECT_EQ(CountOf(pp, MINIMI), 1);
	EXPECT_EQ(CountOf(pp, DefaultMagazine(MINIMI)), 2);
	for (int i = BIGPOCK1POS; i <= BIGPOCK10POS; ++i)
	{
		if (pp.Inv[i].usItem == MINIMI) EXPECT_GT(pp.Inv[i].ubGunShotsLeft, 0);
	}
}

}


TEST_F(EnemyEquipment, eliteHeavyLawKit)
{
	SOLDIERCREATE_STRUCT pp{};
	EquipEliteHeavyEnemy(&pp, false);
	ExpectCommonKit(pp);
	EXPECT_EQ(pp.Inv[HANDPOS].usItem, ROCKET_LAUNCHER);
	EXPECT_EQ(CountOf(pp, ROCKET_LAUNCHER), 3);
	EXPECT_EQ(CountOf(pp, MORTAR), 0);
	// the next LAWs come before the Minimi, so the AI takes them first
	EXPECT_EQ(pp.Inv[BIGPOCK1POS].usItem, ROCKET_LAUNCHER);
	EXPECT_EQ(pp.Inv[BIGPOCK2POS].usItem, ROCKET_LAUNCHER);
	EXPECT_EQ(pp.Inv[BIGPOCK3POS].usItem, MINIMI);
}


TEST_F(EnemyEquipment, eliteHeavyMortarKit)
{
	SOLDIERCREATE_STRUCT pp{};
	EquipEliteHeavyEnemy(&pp, true);
	ExpectCommonKit(pp);
	EXPECT_EQ(pp.Inv[HANDPOS].usItem, MORTAR);
	EXPECT_EQ(CountOf(pp, MORTAR_SHELL), 3);
	EXPECT_EQ(CountOf(pp, ROCKET_LAUNCHER), 0);
}


TEST_F(EnemyEquipment, eliteHeavyKitsAlternate)
{
	Difficulty const d(DIF_LEVEL_MEDIUM);
	SGPSector const old = gWorldSector;

	gWorldSector.z = 0;
	BeginEnemyEliteHeavyCount(0, 0);
	std::vector<bool> mortars;
	for (int i = 0; i < 20; ++i)
	{
		if (TakeEnemyEliteHeavySlot()) mortars.push_back(NextEnemyEliteHeavyGetsMortar());
	}
	EXPECT_EQ(mortars, (std::vector<bool>{ false, true, false, true })); // LAW, mortar, LAW, mortar

	// underground: LAWs only (N4)
	gWorldSector.z = 1;
	BeginEnemyEliteHeavyCount(0, 0);
	for (int i = 0; i < 20; ++i)
	{
		if (TakeEnemyEliteHeavySlot()) EXPECT_FALSE(NextEnemyEliteHeavyGetsMortar());
	}
	gWorldSector = old;
}

// docs/enemy-ammo-chances-plan.md, stage 2 (A5, A6): the loaded and the spare magazines
// of a generated soldier are of one type
TEST_F(EnemyEquipment, loadedAndSpareMagazinesOfOneType)
{
	int checked = 0;
	for (INT8 const cls : { SOLDIER_CLASS_ADMINISTRATOR, SOLDIER_CLASS_ARMY, SOLDIER_CLASS_ELITE })
	{
		for (int i = 0; i < 100; ++i)
		{
			SOLDIERCREATE_STRUCT pp{};
			GenerateRandomEquipment(&pp, cls, NO_EQUIPMENT_LEVEL);
			OBJECTTYPE const& gun = pp.Inv[HANDPOS];
			if (gun.usItem == NOTHING || !GCM->getItem(gun.usItem)->isGun() || gun.usGunAmmoItem == NOTHING) continue;
			auto const* const w = GCM->getItem(gun.usItem)->asWeapon();
			for (OBJECTTYPE const& o : pp.Inv)
			{
				if (o.usItem == NOTHING || !GCM->getItem(o.usItem)->isAmmo()) continue;
				if (!w->matches(GCM->getItem(o.usItem)->asAmmo())) continue;
				EXPECT_EQ(o.usItem, gun.usGunAmmoItem) << GCM->getItem(gun.usItem)->getInternalName().c_str();
				EXPECT_EQ(gun.ubGunAmmoType, GCM->getItem(o.usItem)->asAmmo()->ammoType->index);
				++checked;
			}
		}
	}
	EXPECT_GT(checked, 0);
}

#endif
