#pragma once

#include <string_theory/string>

struct ItemModel;

// The data file an item is defined in; each one has its own block of new item
// indices (see Item_Types.h and docs/item-limits-plan.md).
enum class ItemDataFile
{
	ITEMS,      // items.json
	MAGAZINES,  // magazines.json
	WEAPONS,    // weapons.json
	EXPLOSIVES, // explosives.json
	ARMOURS     // armours.json
};

const char* ItemDataFileName(ItemDataFile file);

// Internal name of the placeholder item at an index no data file defines.
ST::string UnusedItemIndexName(unsigned index);
// Is this item a placeholder (an index no data file defines)?
bool IsUnusedItemIndex(const ItemModel& item);

// Checks the index of an item against the block of the file it is defined in.
// Returns an empty string when the index is allowed, the reason (naming the
// file, the item and the allowed range) otherwise. Does not check duplicates.
ST::string CheckItemIndexBlock(const ItemModel& item, ItemDataFile file);
