#ifdef WITH_UNITTESTS

#include "DefaultContentManagerUT.h"
#include "Exceptions.h"
#include "ItemIndexBlocks.h"
#include "ItemModel.h"
#include "Item_Types.h"

#include "gtest/gtest.h"

#include <memory>

namespace {

std::unique_ptr<ItemModel> MakeItem(uint16_t const index, uint32_t const itemClass = IC_MISC, uint16_t const flags = 0)
{
	return std::make_unique<ItemModel>(
		index, ST::string("TEST_ITEM"), ST::string(), ST::string(), ST::string(),
		itemClass, 0, INVALIDCURS,
		InventoryGraphicsModel(GraphicModel("interface/mdguns.sti", 0), GraphicModel("bigitems/gun00.sti", 0)),
		TilesetTileIndexModel(TileTypeDefines::GUNS, 1),
		0, 0, 0, 0, 0, 0, flags);
}

bool Allowed(uint16_t const index, ItemDataFile const file, uint32_t const itemClass = IC_MISC, uint16_t const flags = 0)
{
	return CheckItemIndexBlock(*MakeItem(index, itemClass, flags), file).empty();
}

}

TEST(ItemIndexBlocks, vanillaIndicesKeepTheirRules)
{
	EXPECT_TRUE(Allowed(1,   ItemDataFile::WEAPONS, IC_GUN));
	EXPECT_TRUE(Allowed(100, ItemDataFile::MAGAZINES, IC_AMMO));
	EXPECT_TRUE(Allowed(331, ItemDataFile::WEAPONS, IC_GUN)); // a weapon in a former UNUSED slot
	EXPECT_TRUE(Allowed(350, ItemDataFile::ITEMS));
	EXPECT_TRUE(Allowed(MAX_WEAPONS + 1, ItemDataFile::ITEMS));
	EXPECT_FALSE(Allowed(MAX_WEAPONS, ItemDataFile::ITEMS));
	EXPECT_FALSE(Allowed(1, ItemDataFile::ITEMS));
}

TEST(ItemIndexBlocks, reserveIsRejected)
{
	EXPECT_FALSE(Allowed(VANILLA_MAXITEMS, ItemDataFile::WEAPONS, IC_GUN));
	EXPECT_FALSE(Allowed(999, ItemDataFile::ITEMS));
}

TEST(ItemIndexBlocks, everyFileHasItsBlock)
{
	EXPECT_TRUE(Allowed(1000, ItemDataFile::WEAPONS, IC_GUN));
	EXPECT_TRUE(Allowed(2999, ItemDataFile::WEAPONS, IC_GUN));
	EXPECT_FALSE(Allowed(3000, ItemDataFile::WEAPONS, IC_GUN));

	EXPECT_TRUE(Allowed(3000, ItemDataFile::MAGAZINES, IC_AMMO));
	EXPECT_TRUE(Allowed(3999, ItemDataFile::MAGAZINES, IC_AMMO));
	EXPECT_FALSE(Allowed(2999, ItemDataFile::MAGAZINES, IC_AMMO));

	EXPECT_TRUE(Allowed(4000, ItemDataFile::EXPLOSIVES, IC_GRENADE));
	EXPECT_FALSE(Allowed(5000, ItemDataFile::EXPLOSIVES, IC_GRENADE));

	EXPECT_TRUE(Allowed(5000, ItemDataFile::ARMOURS, IC_ARMOUR));
	EXPECT_TRUE(Allowed(5999, ItemDataFile::ARMOURS, IC_ARMOUR));
	EXPECT_FALSE(Allowed(6000, ItemDataFile::ARMOURS, IC_ARMOUR));
}

TEST(ItemIndexBlocks, itemsJsonSplitsByKind)
{
	EXPECT_TRUE(Allowed(6000, ItemDataFile::ITEMS));
	EXPECT_TRUE(Allowed(6999, ItemDataFile::ITEMS, IC_KIT));
	EXPECT_FALSE(Allowed(7000, ItemDataFile::ITEMS));

	EXPECT_TRUE(Allowed(7000, ItemDataFile::ITEMS, IC_MISC, ITEM_ATTACHMENT));
	EXPECT_TRUE(Allowed(7999, ItemDataFile::ITEMS, IC_MISC, ITEM_ATTACHMENT));
	EXPECT_FALSE(Allowed(6000, ItemDataFile::ITEMS, IC_MISC, ITEM_ATTACHMENT));

	EXPECT_TRUE(Allowed(8000, ItemDataFile::ITEMS, IC_KEY));
	EXPECT_TRUE(Allowed(8999, ItemDataFile::ITEMS, IC_KEY));
	EXPECT_FALSE(Allowed(7999, ItemDataFile::ITEMS, IC_KEY));
	EXPECT_FALSE(Allowed(8000, ItemDataFile::ITEMS));
}

TEST(ItemIndexBlocks, aboveTheLimitIsRejected)
{
	EXPECT_FALSE(Allowed(MAXITEMS, ItemDataFile::ITEMS, IC_KEY));
	EXPECT_FALSE(Allowed(UINT16_MAX, ItemDataFile::WEAPONS, IC_GUN));
}

TEST(ItemIndexBlocks, errorNamesFileItemAndRange)
{
	ST::string const err = CheckItemIndexBlock(*MakeItem(3500, IC_GUN), ItemDataFile::WEAPONS);
	EXPECT_NE(err.find("weapons.json"), -1);
	EXPECT_NE(err.find("TEST_ITEM"), -1);
	EXPECT_NE(err.find("1000-2999"), -1);
}

using ItemIndexLoading = DefaultContentManagerUT::BaseTest;

TEST_F(ItemIndexLoading, everyIndexHasAnItem)
{
	for (uint16_t i = 0; i < MAXITEMS; ++i)
	{
		ItemModel const* const item = GCM->getItem(i, ItemSystem::nothrow);
		ASSERT_NE(item, nullptr) << "index " << i;
		EXPECT_EQ(item->getItemIndex(), i);
	}
}

TEST_F(ItemIndexLoading, unusedIndicesArePlaceholders)
{
	for (uint16_t i = VANILLA_MAXITEMS; i < MAXITEMS; ++i)
	{
		ItemModel const* const item = GCM->getItem(i);
		EXPECT_EQ(item->getInternalName(), ST::format("__ITEM_{}", i));
		EXPECT_EQ(item->getItemClass(), uint32_t{IC_NONE});
		EXPECT_EQ(item->getFlags(), uint16_t{ITEM_NOT_BUYABLE | ITEM_NOT_EDITOR});
	}
	// the original items are all defined by the data
	for (uint16_t i = 0; i < VANILLA_MAXITEMS; ++i)
	{
		EXPECT_FALSE(GCM->getItem(i)->getInternalName().starts_with("__ITEM_")) << "index " << i;
	}
}

TEST_F(ItemIndexLoading, duplicateIndexIsRejected)
{
	EXPECT_THROW(cm->registerItemForTesting(MakeItem(GLOCK_17, IC_GUN).release(), ItemDataFile::WEAPONS), DataError);
}

TEST_F(ItemIndexLoading, wrongBlockIsRejected)
{
	EXPECT_THROW(cm->registerItemForTesting(MakeItem(500, IC_GUN).release(), ItemDataFile::WEAPONS), DataError);
	EXPECT_THROW(cm->registerItemForTesting(MakeItem(MAXITEMS, IC_GUN).release(), ItemDataFile::WEAPONS), DataError);
}

#endif
