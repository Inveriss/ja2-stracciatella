#ifdef WITH_UNITTESTS

// calibres.json: the table by index and the name lists (checked at load)

#include "CalibreModel.h"
#include "Exceptions.h"
#include "Json.h"

#include "gtest/gtest.h"

#include <string_theory/format>

#include <memory>
#include <vector>


namespace
{
	struct CalibreTable
	{
		std::vector<std::unique_ptr<CalibreModel>> owned;
		std::vector<const CalibreModel*> table;

		void add(int const index, char const* const name)
		{
			ST::string const json = ST::format(
				R"({{"index": {}, "internalName": "{}", "showInHelpText": true, "monsterWeapon": false}})",
				index, name);
			owned.emplace_back(CalibreModel::deserialize(JsonValue::deserialize(json)));
			AddCalibreToTable(table, owned.back().get());
		}
	};
}


TEST(Calibres, tableWithoutGapsAndNamesForAll)
{
	CalibreTable t;
	t.add(0, "NOAMMO");
	t.add(2, "AMMO9");
	t.add(1, "AMMO38");
	EXPECT_NO_THROW(CheckCalibreTable(t.table));
	EXPECT_NO_THROW(CheckCalibreNames(t.table, 3, "names", 3, "bobby"));
	EXPECT_NO_THROW(CheckCalibreNames(t.table, 5, "names", 4, "bobby")); // more names: fine
}


TEST(Calibres, mistakesStopTheGame)
{
	CalibreTable gap;
	gap.add(0, "NOAMMO");
	gap.add(2, "AMMO9");
	EXPECT_THROW(CheckCalibreTable(gap.table), DataError);

	CalibreTable sameIndex;
	sameIndex.add(0, "NOAMMO");
	sameIndex.add(1, "AMMO38");
	EXPECT_THROW(sameIndex.add(1, "AMMO9"), DataError);

	CalibreTable sameName;
	sameName.add(0, "NOAMMO");
	sameName.add(1, "AMMO38");
	sameName.add(2, "AMMO38");
	EXPECT_THROW(CheckCalibreTable(sameName.table), DataError);

	CalibreTable names;
	names.add(0, "NOAMMO");
	names.add(1, "AMMO38");
	names.add(2, "AMMO9");
	EXPECT_THROW(CheckCalibreNames(names.table, 2, "names", 3, "bobby"), DataError);
	EXPECT_THROW(CheckCalibreNames(names.table, 3, "names", 2, "bobby"), DataError);
}

#endif
