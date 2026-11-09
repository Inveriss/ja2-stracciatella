#ifdef WITH_UNITTESTS

// docs/enemy-equipment-progress-plan.md, stage 1: the minEnemyProgress field

#include "DefaultContentManagerUT.h"
#include "ItemModel.h"
#include "Item_Types.h"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

using EnemyEquipmentProgress = DefaultContentManagerUT::BaseTest;

std::unique_ptr<ItemModel> MakeItem(uint32_t const itemClass, int const minEnemyProgress)
{
	auto item = std::make_unique<ItemModel>(
		7000, ST::string("TEST_ITEM"), ST::string(), ST::string(), ST::string(),
		itemClass, 0, INVALIDCURS,
		InventoryGraphicsModel(GraphicModel("interface/mdguns.sti", 0), GraphicModel("bigitems/gun00.sti", 0)),
		TilesetTileIndexModel(TileTypeDefines::GUNS, 1),
		0, 0, 0, 0, 0, 0, 0);
	if (minEnemyProgress >= 0) item->setMinEnemyProgress(static_cast<uint8_t>(minEnemyProgress));
	return item;
}

ST::string Apply(ItemModel& item, char const* json)
{
	return ApplyMinEnemyProgress(item, JsonValue::deserialize(json).toObject());
}

}


TEST(EnemyEquipmentProgressField, leftOutMeansNeverForEnemies)
{
	auto item = MakeItem(IC_GUN, 30);
	EXPECT_TRUE(Apply(*item, "{}").empty());
	EXPECT_FALSE(item->hasMinEnemyProgress());
}


TEST(EnemyEquipmentProgressField, acceptsStepsOfFive)
{
	auto item = MakeItem(IC_GUN, -1);
	for (int const v : { 0, 5, 55, 100 })
	{
		EXPECT_TRUE(Apply(*item, ST::format("{{\"minEnemyProgress\": {}}}", v).c_str()).empty()) << v;
		EXPECT_TRUE(item->hasMinEnemyProgress());
		EXPECT_EQ(item->getMinEnemyProgress(), v);
	}
}


TEST(EnemyEquipmentProgressField, rejectsOtherValues)
{
	auto item = MakeItem(IC_GUN, -1);
	for (int const v : { -5, 7, 99, 105 })
	{
		EXPECT_FALSE(Apply(*item, ST::format("{{\"minEnemyProgress\": {}}}", v).c_str()).empty()) << v;
	}
}


TEST(EnemyEquipmentProgressField, warnsAboutMissingLowTiers)
{
	auto gun   = MakeItem(IC_GUN, 20);
	auto vest  = MakeItem(IC_ARMOUR, 25);
	auto knife = MakeItem(IC_BLADE, 0);
	auto nade  = MakeItem(IC_GRENADE, 10);
	auto bomb  = MakeItem(IC_BOMB, 10);
	auto kit   = MakeItem(IC_MEDKIT, -1);   // no tier: kits get a warning too
	std::vector<const ItemModel*> const items{ gun.get(), nullptr, vest.get(), knife.get(), nade.get(), bomb.get(), kit.get() };

	auto const warnings = MissingLowEnemyTiers(items);
	ASSERT_EQ(warnings.size(), 2u);
	EXPECT_NE(warnings[0].find("armour"), -1);
	EXPECT_NE(warnings[0].find("soldiers below it get none"), -1); // B5: not a gun
	EXPECT_NE(warnings[1].find("no kits"), -1);

	// guns above the limit: weaker soldiers still get the lowest tier
	auto const highGun = MakeItem(IC_GUN, 30);
	std::vector<const ItemModel*> const guns{ highGun.get(), vest.get(), knife.get(), nade.get(), bomb.get() };
	auto const gunWarnings = MissingLowEnemyTiers(guns);
	ASSERT_GE(gunWarnings.size(), 2u);
	EXPECT_NE(gunWarnings[0].find("guns"), -1);
	EXPECT_NE(gunWarnings[0].find("weaker soldiers get that tier"), -1);
}


TEST_F(EnemyEquipmentProgress, baseDataForTodaysEnemyItems)
{
	// copied from minDropProgress (rounded to 5); ROCKET_RIFLE has no drop limit, so its table row
	struct { char const* name; int progress; } const expected[] =
	{
		{ "GLOCK_17", 20 }, { "SW38", 10 }, { "FNFAL", 80 }, { "ROCKET_RIFLE", 100 },
		{ "COMBAT_KNIFE", 20 }, { "BRASS_KNUCKLES", 20 },
		{ "FLAK_JACKET", 20 }, { "SPECTRA_HELMET", 70 },
		{ "HAND_GRENADE", 70 }, { "BREAK_LIGHT", 30 }, { "MORTAR_SHELL", 90 }, { "TNT", 60 },
		{ "FIRSTAIDKIT", 10 }, { "LASERSCOPE", 80 }, { "CANTEEN", 10 },
	};
	for (auto const& e : expected)
	{
		ItemModel const* const item = cm->getItemByName(e.name);
		ASSERT_NE(item, nullptr) << e.name;
		ASSERT_TRUE(item->hasMinEnemyProgress()) << e.name;
		EXPECT_EQ(item->getMinEnemyProgress(), e.progress) << e.name;
	}
	// never given to enemies: treated armour (coolness 0), toolkits, detonators, quest guns
	for (char const* const name : { "FLAK_JACKET_18", "TOOLKIT", "DETONATOR", "ROCKET_LAUNCHER", "TANK_CANNON" })
	{
		ItemModel const* const item = cm->getItemByName(name);
		ASSERT_NE(item, nullptr) << name;
		EXPECT_FALSE(item->hasMinEnemyProgress()) << name;
	}
}


// docs/enemy-drop-progress-plan.md (D13): "minDropProgress": "blocked"

namespace {

ST::string ApplyDrop(ItemModel& item, char const* json)
{
	return ApplyMinDropProgress(item, JsonValue::deserialize(json).toObject());
}

}


TEST(MinDropProgressField, numbersAndBlocked)
{
	auto item = MakeItem(IC_GUN, -1);
	EXPECT_TRUE(ApplyDrop(*item, "{}").empty());
	EXPECT_EQ(item->getMinDropProgress(), 0);
	EXPECT_FALSE(item->isEnemyDropBlocked());

	EXPECT_TRUE(ApplyDrop(*item, "{\"minDropProgress\": 1}").empty());
	EXPECT_EQ(item->getMinDropProgress(), 1);
	EXPECT_FALSE(item->isEnemyDropBlocked());

	EXPECT_TRUE(ApplyDrop(*item, "{\"minDropProgress\": \"blocked\"}").empty());
	EXPECT_EQ(item->getMinDropProgress(), 0);
	EXPECT_TRUE(item->isEnemyDropBlocked());

	EXPECT_TRUE(ApplyDrop(*item, "{\"minDropProgress\": 100}").empty());
	EXPECT_EQ(item->getMinDropProgress(), 100);
	EXPECT_FALSE(item->isEnemyDropBlocked());
}


TEST(MinDropProgressField, rejectsOtherValues)
{
	auto item = MakeItem(IC_GUN, -1);
	for (char const* const json : { "{\"minDropProgress\": 101}", "{\"minDropProgress\": -1}",
		"{\"minDropProgress\": \"never\"}", "{\"minDropProgress\": false}" })
	{
		EXPECT_FALSE(ApplyDrop(*item, json).empty()) << json;
	}
}

#endif
