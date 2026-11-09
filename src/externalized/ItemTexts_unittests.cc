#ifdef WITH_UNITTESTS

// docs/item-texts-json-plan.md: strings/item-texts-<language>.json

#include "DefaultContentManagerUT.h"
#include "Exceptions.h"
#include "ItemModel.h"
#include "Item_Types.h"

#include "gtest/gtest.h"

#include <map>
#include <memory>
#include <string>

namespace {

using ItemTextsData = DefaultContentManagerUT::BaseTest;

std::unique_ptr<ItemModel> MakeItem(char const* internalName)
{
	return std::make_unique<ItemModel>(
		7000, ST::string(internalName), ST::string("short"), ST::string("name"), ST::string("description"),
		IC_GUN, 0, INVALIDCURS,
		InventoryGraphicsModel(GraphicModel("interface/mdguns.sti", 0), GraphicModel("bigitems/gun00.sti", 0)),
		TilesetTileIndexModel(TileTypeDefines::GUNS, 1),
		0, 0, 0, 0, 0, 0, 0);
}

struct Items
{
	std::map<ST::string, std::unique_ptr<ItemModel>> items;
	ItemModel& add(char const* name) { return *(items[name] = MakeItem(name)); }
	std::vector<ST::string> apply(char const* json)
	{
		return ApplyItemTexts(JsonValue::deserialize(json), "item-texts-test.json",
			[this](ST::string const& n) -> ItemModel* { auto it = items.find(n); return it == items.end() ? nullptr : it->second.get(); });
	}
};

}


TEST(ItemTexts, onlyTheGivenFieldsChange)
{
	Items t;
	ItemModel& a = t.add("A");
	ItemModel& b = t.add("B");
	auto const warnings = t.apply(R"([
		{ "internalName": "A", "description": "new A description" },
		{ "internalName": "B", "shortName": "B s", "name": "B n", "bobbyRayName": "B br" }
	])");
	EXPECT_TRUE(warnings.empty());

	EXPECT_EQ(a.getShortName(), "short");
	EXPECT_EQ(a.getName(), "name");
	EXPECT_EQ(a.getDescription(), "new A description");
	EXPECT_TRUE(a.getBobbyRayName().empty());

	EXPECT_EQ(b.getShortName(), "B s");
	EXPECT_EQ(b.getName(), "B n");
	EXPECT_EQ(b.getDescription(), "description");
	EXPECT_EQ(b.getBobbyRayName(), "B br");
	EXPECT_TRUE(b.getBobbyRayDescription().empty());
}


TEST(ItemTexts, unknownOrDuplicateItemIsAnError)
{
	Items t;
	t.add("A");
	EXPECT_THROW(t.apply(R"([ { "internalName": "NO_SUCH_ITEM", "name": "x" } ])"), DataError);
	EXPECT_THROW(t.apply(R"([ { "internalName": "A", "name": "x" }, { "internalName": "A", "name": "y" } ])"), DataError);
}


TEST(ItemTexts, warnsAboveTheOldSizes)
{
	Items t;
	t.add("A");
	std::string const long240(240, 'x');
	std::string const long241(241, 'x');
	std::string const long321(321, 'x');
	std::string const json = R"([ { "internalName": "A", "description": ")" + long240 + R"(" } ])";
	EXPECT_TRUE(t.apply(json.c_str()).empty());

	std::string const json2 = R"([ { "internalName": "A", "description": ")" + long241 +
		R"(", "bobbyRayDescription": ")" + long321 + R"(" } ])";
	auto const warnings = t.apply(json2.c_str());
	ASSERT_EQ(warnings.size(), 2u);
	EXPECT_NE(warnings[0].find("description has 241"), -1);
	EXPECT_NE(warnings[1].find("bobbyRayDescription has 321"), -1);
}


TEST(ItemTexts, exportSkipsEmptyItemsAndEscapes)
{
	std::vector<ItemTextsEntry> entries(3);
	entries[0].internalName = "NOTHING";                    // no text at all: left out (T8)
	entries[1].internalName = "GUN";
	entries[1].name         = "Gun \"Big\"";
	entries[1].description  = "Line\\one";
	entries[2].internalName = "KIT";
	entries[2].bobbyRayDescription = "Only Bobby Ray";

	std::string const text = ItemTextsFileText(entries);
	EXPECT_EQ(text.find("NOTHING"), std::string::npos);
	std::string const expectedName = "\"name\": \"Gun \\\"Big\\\"\"";       // "name": "Gun \"Big\""
	std::string const expectedDescription = "\"description\": \"Line\\\\one\""; // "description": "Line\\one"
	EXPECT_NE(text.find(expectedName), std::string::npos);
	EXPECT_NE(text.find(expectedDescription), std::string::npos);
	EXPECT_EQ(text.find(R"("shortName")"), std::string::npos);    // empty fields left out
	EXPECT_NE(text.find(R"("bobbyRayDescription": "Only Bobby Ray")"), std::string::npos);

	// the result reads back as JSON with the two entries
	auto const list = JsonValue::deserialize(ST::string(text.substr(text.find('[')).c_str())).toVec();
	ASSERT_EQ(list.size(), 2u);
	EXPECT_EQ(list[0].toObject().GetString("internalName"), "GUN");
	EXPECT_EQ(list[0].toObject().GetString("name"), "Gun \"Big\"");
}


TEST_F(ItemTextsData, englishFileOfTheRepository)
{
	// loaded by the content manager of the test (English): names and descriptions are there
	for (char const* const name : { "GLOCK_17", "MORTAR", "FIRSTAIDKIT", "SPECTRA_VEST" })
	{
		ItemModel const* const item = cm->getItemByName(name);
		ASSERT_NE(item, nullptr) << name;
		EXPECT_FALSE(item->getName().empty()) << name;
		EXPECT_FALSE(item->getDescription().empty()) << name;
	}
}

#endif
