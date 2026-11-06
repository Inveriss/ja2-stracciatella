#include "ItemIndexBlocks.h"

#include "ItemModel.h"
#include "Item_Types.h"

#include <string_theory/format>


const char* ItemDataFileName(ItemDataFile const file)
{
	switch (file)
	{
		case ItemDataFile::ITEMS:      return "items.json";
		case ItemDataFile::MAGAZINES:  return "magazines.json";
		case ItemDataFile::WEAPONS:    return "weapons.json";
		case ItemDataFile::EXPLOSIVES: return "explosives.json";
		case ItemDataFile::ARMOURS:    return "armours.json";
	}
	return "?";
}


ST::string UnusedItemIndexName(unsigned const index)
{
	return ST::format("__ITEM_{}", index);
}


bool IsUnusedItemIndex(const ItemModel& item)
{
	return item.getInternalName() == UnusedItemIndexName(item.getItemIndex());
}


namespace {

struct ItemIndexBlock
{
	UINT16      first;
	UINT16      last;
	char const* what;
};

ItemIndexBlock NewItemBlock(const ItemModel& item, ItemDataFile const file)
{
	switch (file)
	{
		case ItemDataFile::WEAPONS:    return { FIRST_NEW_WEAPON,    LAST_NEW_WEAPON,    "a weapon" };
		case ItemDataFile::MAGAZINES:  return { FIRST_NEW_AMMO,      LAST_NEW_AMMO,      "a magazine" };
		case ItemDataFile::EXPLOSIVES: return { FIRST_NEW_EXPLOSIVE, LAST_NEW_EXPLOSIVE, "an explosive" };
		case ItemDataFile::ARMOURS:    return { FIRST_NEW_ARMOUR,    LAST_NEW_ARMOUR,    "an armour" };
		case ItemDataFile::ITEMS:      break;
	}
	if (item.getItemClass() == IC_KEY)
	{
		return { FIRST_NEW_KEY, LAST_NEW_KEY, "a key" };
	}
	if (item.getFlags() & ITEM_ATTACHMENT)
	{
		return { FIRST_NEW_WEAPON_ATTACHMENT, LAST_NEW_WEAPON_ATTACHMENT, "an attachment" };
	}
	return { FIRST_NEW_MISC, LAST_NEW_MISC, "a misc item" };
}

}


ST::string CheckItemIndexBlock(const ItemModel& item, ItemDataFile const file)
{
	UINT16      const idx  = item.getItemIndex();
	char const* const name = ItemDataFileName(file);

	if (idx >= MAXITEMS)
	{
		return ST::format("{}: item '{}' has index {}, the highest allowed index is {}",
			name, item.getInternalName(), idx, MAXITEMS - 1);
	}

	if (idx < VANILLA_MAXITEMS)
	{
		// the indices of the original game keep their old rules
		if (file == ItemDataFile::ITEMS && idx <= MAX_WEAPONS)
		{
			return ST::format("{}: item '{}' has index {}, items.json may only use the indices {}-{} and from {} on",
				name, item.getInternalName(), idx, MAX_WEAPONS + 1, VANILLA_MAXITEMS - 1, FIRST_NEW_ITEM);
		}
		return {};
	}

	if (idx < FIRST_NEW_ITEM)
	{
		return ST::format("{}: item '{}' has index {}, the indices {}-{} are reserved",
			name, item.getInternalName(), idx, VANILLA_MAXITEMS, FIRST_NEW_ITEM - 1);
	}

	ItemIndexBlock const block = NewItemBlock(item, file);
	if (idx < block.first || block.last < idx)
	{
		return ST::format("{}: item '{}' has index {}, {} must have an index in {}-{}",
			name, item.getInternalName(), idx, block.what, block.first, block.last);
	}
	return {};
}
