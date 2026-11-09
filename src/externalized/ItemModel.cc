#include "ItemModel.h"
#include "Exceptions.h"
#include "Logger.h"
#include <algorithm>
#include <functional>
#include <cstdio>
#include <iterator>
#include <set>
#include <utility>


namespace {
auto deserializeHelper(ItemModel::InitData const& initData,
	char const * propertyName,
	decltype(&BinaryData::getItemName) getterMethod)
{
	auto result{ initData.json.getOptionalString(propertyName) };
	if (result.empty()) {
		result = std::invoke(getterMethod, initData.strings,
		                     initData.json.GetUInt("itemIndex"));
	}
	return result;
}
}

ItemModel::ItemModel(uint16_t itemIndex,
			ST::string&& internalName,
			uint32_t usItemClass,
			uint8_t classIndex,
			ItemCursor cursor) :
	inventoryGraphics(InventoryGraphicsModel(GraphicModel("interface/mdguns.sti", 0), GraphicModel("bigitems/gun00.sti", 0))),
	tileGraphic(TilesetTileIndexModel(TileTypeDefines::GUNS, 1))
{
	this->itemIndex             = itemIndex;
	this->internalName          = std::move(internalName);
	this->usItemClass           = usItemClass;
	this->ubClassIndex          = classIndex;
	this->ubCursor              = cursor;
	this->ubWeight              = 0;
	this->ubPerPocket           = 0;
	this->usPrice               = 0;
	this->ubCoolness            = 0;
	this->bReliability          = 0;
	this->bRepairEase           = 0;
	this->fFlags                = 0;
}

ItemModel::ItemModel(uint16_t   itemIndex,
			ST::string&& internalName,
			ST::string&& shortName,
			ST::string&& name,
			ST::string&& description,
			uint32_t   usItemClass,
			uint8_t    ubClassIndex,
			ItemCursor ubCursor,
			InventoryGraphicsModel&& inventoryGraphics_,
			TilesetTileIndexModel&& tileGraphic_,
			uint8_t    ubWeight,
			uint8_t    ubPerPocket,
			uint16_t   usPrice,
			uint8_t    ubCoolness,
			int8_t     bReliability,
			int8_t     bRepairEase,
			uint16_t   fFlags) : inventoryGraphics(std::move(inventoryGraphics_)), tileGraphic(tileGraphic_)
{
	this->itemIndex             = itemIndex;
	this->internalName          = std::move(internalName);
	this->shortName             = std::move(shortName);
	this->name                  = std::move(name);
	this->description           = std::move(description);
	this->usItemClass           = usItemClass;
	this->ubClassIndex          = ubClassIndex;
	this->ubCursor              = ubCursor;
	this->ubWeight              = ubWeight;
	this->ubPerPocket           = ubPerPocket;
	this->usPrice               = usPrice;
	this->ubCoolness            = ubCoolness;
	this->bReliability          = bReliability;
	this->bRepairEase           = bRepairEase;
	this->fFlags                = fFlags;
}

const ST::string& ItemModel::getInternalName() const   { return internalName;          }

const ST::string& ItemModel::getShortName() const      { return shortName; }
const ST::string& ItemModel::getName() const           { return name; }
const ST::string& ItemModel::getDescription() const     { return description; }

void ItemModel::setTexts(ST::string const* const s, ST::string const* const n, ST::string const* const d)
{
	if (s) shortName   = *s;
	if (n) name        = *n;
	if (d) description = *d;
}

uint16_t        ItemModel::getItemIndex() const        { return itemIndex;             }
uint32_t        ItemModel::getItemClass() const        { return usItemClass;           }
uint8_t         ItemModel::getClassIndex() const       { return ubClassIndex;          }
ItemCursor      ItemModel::getCursor() const           { return ubCursor;              }
uint8_t         ItemModel::getWeight() const           { return ubWeight;              }
uint8_t         ItemModel::getPerPocket() const        { return ubPerPocket;           }
std::optional<uint8_t> ItemModel::getSmallPerPocket() const { return ubSmallPerPocket; }
std::optional<uint8_t> ItemModel::getBigPerPocket() const   { return ubBigPerPocket;   }
uint16_t        ItemModel::getPrice() const            { return usPrice;               }
uint8_t         ItemModel::getCoolness() const         { return ubCoolness;            }
int8_t          ItemModel::getReliability() const      { return bReliability;          }
int8_t          ItemModel::getRepairEase() const       { return bRepairEase;           }
uint16_t        ItemModel::getFlags() const            { return fFlags;                }
const GraphicModel& ItemModel::getInventoryGraphicSmall() const { return inventoryGraphics.small; }
const GraphicModel& ItemModel::getInventoryGraphicBig() const { return inventoryGraphics.big; }
const TilesetTileIndexModel& ItemModel::getTileGraphic() const { return tileGraphic; }

bool ItemModel::isAmmo() const           { return usItemClass == IC_AMMO; }
bool ItemModel::isArmour() const         { return usItemClass == IC_ARMOUR; }
bool ItemModel::isBlade() const          { return usItemClass == IC_BLADE; }
bool ItemModel::isBomb() const           { return usItemClass == IC_BOMB; }
bool ItemModel::isExplosive() const      { return usItemClass & (IC_GRENADE | IC_BOMB); }
bool ItemModel::isFace() const           { return usItemClass == IC_FACE; }
bool ItemModel::isGrenade() const        { return usItemClass == IC_GRENADE; }
bool ItemModel::isGun() const            { return usItemClass == IC_GUN; }
bool ItemModel::isKey() const            { return usItemClass == IC_KEY; }
bool ItemModel::isKit() const            { return usItemClass == IC_KIT; }
bool ItemModel::isLauncher() const       { return usItemClass == IC_LAUNCHER; }
bool ItemModel::isMedkit() const         { return usItemClass == IC_MEDKIT; }
bool ItemModel::isMisc() const           { return usItemClass == IC_MISC; }
bool ItemModel::isMoney() const          { return usItemClass == IC_MONEY; }
bool ItemModel::isPunch() const          { return usItemClass == IC_PUNCH; }
bool ItemModel::isTentacles() const      { return usItemClass == IC_TENTACLES; }
bool ItemModel::isThrowingKnife() const  { return usItemClass == IC_THROWING_KNIFE; }
bool ItemModel::isThrown() const         { return usItemClass == IC_THROWN; }
bool ItemModel::isWeapon() const         { return usItemClass & (IC_GUN | IC_BLADE | IC_THROWING_KNIFE | IC_LAUNCHER | IC_TENTACLES); }

bool ItemModel::isTwoHanded() const      { return fFlags & ITEM_TWO_HANDED;             }
bool ItemModel::isInBigGunList() const   { return fFlags & ITEM_BIGGUNLIST;             }

void ItemModel::serializeFlags(JsonObject &obj) const
{
	if(getFlags() & ITEM_DAMAGEABLE)          { obj.set("bDamageable", true);     }
	if(getFlags() & ITEM_REPAIRABLE)          { obj.set("bRepairable", true);     }
	if(getFlags() & ITEM_WATER_DAMAGES)       { obj.set("bWaterDamages", true);   }
	if(getFlags() & ITEM_METAL)               { obj.set("bMetal", true);          }
	if(getFlags() & ITEM_SINKS)               { obj.set("bSinks", true);          }
	if(getFlags() & ITEM_SHOW_STATUS)         { obj.set("bShowStatus", true);     }
	if(getFlags() & ITEM_HIDDEN_ADDON)        { obj.set("bHiddenAddon", true);    }
	if(getFlags() & ITEM_TWO_HANDED)          { obj.set("bTwoHanded", true);      }
	if(getFlags() & ITEM_NOT_BUYABLE)         { obj.set("bNotBuyable", true);     }
	if(getFlags() & ITEM_ATTACHMENT)          { obj.set("bAttachment", true);     }
	if(getFlags() & ITEM_BIGGUNLIST)          { obj.set("bBigGunList", true);     }
	if(getFlags() & ITEM_NOT_EDITOR)          { obj.set("bNotEditor", true);      }
	if(getFlags() & ITEM_DEFAULT_UNDROPPABLE) { obj.set("bDefaultUndroppable", true); }
	if(getFlags() & ITEM_UNAERODYNAMIC)       { obj.set("bUnaerodynamic", true);  }
	if(getFlags() & ITEM_ELECTRONIC)          { obj.set("bElectronic", true);     }
	if(getFlags() & ITEM_INSEPARABLE)         { obj.set("bInseparable", true);    }
}

uint16_t ItemModel::deserializeFlags(const JsonObject &obj)
{
	uint32_t flags = 0;
	if(obj.getOptionalBool("bDamageable"))        { flags |= ITEM_DAMAGEABLE;             }
	if(obj.getOptionalBool("bRepairable"))        { flags |= ITEM_REPAIRABLE;             }
	if(obj.getOptionalBool("bWaterDamages"))      { flags |= ITEM_WATER_DAMAGES;          }
	if(obj.getOptionalBool("bMetal"))             { flags |= ITEM_METAL;                  }
	if(obj.getOptionalBool("bSinks"))             { flags |= ITEM_SINKS;                  }
	if(obj.getOptionalBool("bShowStatus"))        { flags |= ITEM_SHOW_STATUS;            }
	if(obj.getOptionalBool("bHiddenAddon"))       { flags |= ITEM_HIDDEN_ADDON;           }
	if(obj.getOptionalBool("bTwoHanded"))         { flags |= ITEM_TWO_HANDED;             }
	if(obj.getOptionalBool("bNotBuyable"))        { flags |= ITEM_NOT_BUYABLE;            }
	if(obj.getOptionalBool("bAttachment"))        { flags |= ITEM_ATTACHMENT;             }
	if(obj.getOptionalBool("bBigGunList"))        { flags |= ITEM_BIGGUNLIST;             }
	if(obj.getOptionalBool("bNotEditor"))         { flags |= ITEM_NOT_EDITOR;             }
	if(obj.getOptionalBool("bDefaultUndroppable")){ flags |= ITEM_DEFAULT_UNDROPPABLE;    }
	if(obj.getOptionalBool("bUnaerodynamic"))     { flags |= ITEM_UNAERODYNAMIC;          }
	if(obj.getOptionalBool("bElectronic"))        { flags |= ITEM_ELECTRONIC;             }
	if(obj.getOptionalBool("bInseparable"))       { flags |= ITEM_INSEPARABLE;            }
	return flags;
}

void ItemModel::serializeSmallPerPocket(JsonObject &obj) const
{
	// Only written when explicitly set -- omitting it (rather than always
	// writing ubPerPocket/2 or similar) keeps every item that never opted
	// into this free of a redundant field, and preserves the "unset ->
	// derive from ubPerPocket" fallback in ItemSlotLimit() (Items.cc) on
	// the next load.
	if (ubSmallPerPocket) { obj.set("ubSmallPerPocket", *ubSmallPerPocket); }
}

std::optional<uint8_t> ItemModel::deserializeSmallPerPocket(const JsonObject &obj)
{
	if (!obj.has("ubSmallPerPocket")) return std::nullopt;
	return static_cast<uint8_t>(obj.GetUInt("ubSmallPerPocket"));
}

void ItemModel::serializeBigPerPocket(JsonObject &obj) const
{
	if (ubBigPerPocket) { obj.set("ubBigPerPocket", *ubBigPerPocket); }
}

std::optional<uint8_t> ItemModel::deserializeBigPerPocket(const JsonObject &obj)
{
	if (!obj.has("ubBigPerPocket")) return std::nullopt;
	return static_cast<uint8_t>(obj.GetUInt("ubBigPerPocket"));
}

uint8_t ItemModel::deserializeAPCostPerThrow(const JsonObject &obj, const ST::string& internalName)
{
	if (!obj.has("ubAPCostPerThrow"))
	{
		SLOGW("Throwable item '{}' has no ubAPCostPerThrow, using {} AP", internalName, DEFAULT_AP_COST_PER_THROW);
		return DEFAULT_AP_COST_PER_THROW;
	}
	return static_cast<uint8_t>(std::clamp(obj.GetInt("ubAPCostPerThrow"), 1, 255));
}

/** Check if the given attachment can be attached to the item. */
bool ItemModel::canBeAttached(const GamePolicy* policy, const ItemModel* attachment) const
{
	return false;
}

JsonValue ItemModel::serialize() const
{
	JsonObject obj;
    obj.set("itemIndex", itemIndex);
    obj.set("internalName", internalName);
    obj.set("usItemClass", (uint32_t)getItemClass());
    obj.set("ubClassIndex", getClassIndex());
    obj.set("ubCursor",  getCursor());
    obj.set("inventoryGraphics", inventoryGraphics.serialize());
	obj.set("tileGraphic", tileGraphic.serialize());
    obj.set("ubWeight", getWeight());
    obj.set("ubPerPocket", getPerPocket());
    obj.set("usPrice", getPrice());
    obj.set("ubCoolness", getCoolness());
    obj.set("bReliability", getReliability());
    obj.set("bRepairEase", getRepairEase());

    serializeFlags(obj);
    serializeSmallPerPocket(obj);
    serializeBigPerPocket(obj);

	return obj.toValue();
}

ST::string ItemModel::deserializeShortName(InitData const& initData)
{
	return deserializeHelper(initData, "shortName", &BinaryData::getItemShortName);
}

ST::string ItemModel::deserializeName(InitData const& initData)
{
	return deserializeHelper(initData, "name", &BinaryData::getItemName);
}

ST::string ItemModel::deserializeDescription(InitData const& initData)
{
	return deserializeHelper(initData, "description", &BinaryData::getItemDescription);
}

const ItemModel* ItemModel::deserialize(const JsonValue &json, const BinaryData& vanillaItemStrings)
{
	auto obj = json.toObject();
	InitData const initData{ obj, vanillaItemStrings };
	uint16_t itemIndex = obj.GetUInt("itemIndex");
	ST::string internalName = obj.GetString("internalName");
	auto inventoryGraphics = InventoryGraphicsModel::deserialize(obj["inventoryGraphics"]);
	auto tileGraphic = TilesetTileIndexModel::deserialize(obj["tileGraphic"]);

	auto shortName = ItemModel::deserializeShortName(initData);
	auto name = ItemModel::deserializeName(initData);
	auto description = ItemModel::deserializeDescription(initData);
	auto flags = ItemModel::deserializeFlags(obj);

	ItemModel* const item = new ItemModel(
		itemIndex,
		std::move(internalName),
		std::move(shortName),
		std::move(name),
		std::move(description),
		obj.GetUInt("usItemClass"),
		obj.GetUInt("ubClassIndex"),
		(ItemCursor)obj.GetUInt("ubCursor"),
		std::move(inventoryGraphics),
		std::move(tileGraphic),
		obj.GetUInt("ubWeight"),
		obj.GetUInt("ubPerPocket"),
		obj.GetUInt("usPrice"),
		obj.GetUInt("ubCoolness"),
		obj.GetInt("bReliability"),
		obj.GetInt("bRepairEase"),
		flags
	);
	item->ubSmallPerPocket = ItemModel::deserializeSmallPerPocket(obj);
	item->ubBigPerPocket   = ItemModel::deserializeBigPerPocket(obj);
	item->deserializeAttachmentFamily(obj);
	return item;
}

void ItemModel::deserializeAttachmentFamily(const JsonObject &obj)
{
	attachmentFamilyName = obj.getOptionalString("attachmentFamily");
	attachmentLevel      = static_cast<uint8_t>(obj.getOptionalUInt("attachmentLevel", 0));
	if (!obj.has("attachmentEffects")) return;

	JsonObject const e = obj["attachmentEffects"].toObject();
	auto const read = [&e](char const* const name, std::optional<int16_t>& value)
	{
		if (e.has(name)) value = static_cast<int16_t>(e.GetInt(name));
	};
	AttachmentEffects& a = attachmentEffects;
	read("rangeBonus",          a.rangeBonus);
	read("aimBonus",            a.aimBonus);
	read("aimedShotBonus",      a.aimedShotBonus);
	read("proneBonus",          a.proneBonus);
	read("attackVolumePercent", a.attackVolumePercent);
	read("apPerShotChange",     a.apPerShotChange);
	read("burstShotsBonus",     a.burstShotsBonus);
}


ST::string ApplyMinEnemyProgress(ItemModel& item, JsonObject const& json)
{
	if (!json.has("minEnemyProgress"))
	{
		item.setMinEnemyProgress(ItemModel::NO_MIN_ENEMY_PROGRESS);
		return {};
	}
	int const v = json.getOptionalInt("minEnemyProgress");
	if (v < 0 || 100 < v || v % 5 != 0)
	{
		return ST::format("minEnemyProgress of item '{}' must be 0-100 in steps of 5, not {}",
			item.getInternalName(), v);
	}
	item.setMinEnemyProgress(static_cast<uint8_t>(v));
	return {};
}


std::vector<ST::string> MissingLowEnemyTiers(std::vector<const ItemModel*> const& items)
{
	struct Category { char const* name; uint32_t classes; };
	static Category const categories[] =
	{
		{ "guns",     IC_GUN },
		{ "armour",   IC_ARMOUR },
		{ "knives",   IC_BLADE | IC_THROWING_KNIFE },
		{ "grenades", IC_GRENADE },
		{ "bombs",    IC_BOMB },
		{ "kits",     IC_KIT | IC_MEDKIT },
	};

	std::vector<ST::string> warnings;
	for (Category const& c : categories)
	{
		int lowest = -1;
		for (const ItemModel* const item : items)
		{
			if (!item || !(item->getItemClass() & c.classes) || !item->hasMinEnemyProgress()) continue;
			int const v = item->getMinEnemyProgress();
			if (lowest < 0 || v < lowest) lowest = v;
		}
		if (lowest < 0)
		{
			warnings.push_back(ST::format("Enemy equipment: no {} have minEnemyProgress, enemies get none", c.name));
		}
		else if (lowest > ENEMY_LOW_TIER_LIMIT)
		{
			// below the lowest tier guns take that tier, other equipment is none
			// (docs/enemy-equipment-below-lowest-tier-plan.md, B5)
			bool const guns = c.classes == IC_GUN;
			warnings.push_back(ST::format("Enemy equipment: the lowest minEnemyProgress of the {} is {} (above {}), {}",
				c.name, lowest, ENEMY_LOW_TIER_LIMIT,
				guns ? "weaker soldiers get that tier" : "soldiers below it get none"));
		}
	}
	return warnings;
}


ST::string ApplyMinDropProgress(ItemModel& item, JsonObject const& json)
{
	item.setMinDropProgress(0);
	item.setEnemyDropBlocked(false);
	if (!json.has("minDropProgress")) return {};

	JsonValue const v = json["minDropProgress"];
	if (v.isString())
	{
		if (v.toString() != "blocked")
		{
			return ST::format("minDropProgress of item '{}' must be 0-100 or \"blocked\", not \"{}\"",
				item.getInternalName(), v.toString());
		}
		item.setEnemyDropBlocked(true);
		return {};
	}
	int const progress = v.isInt() ? v.toInt() : -1;
	if (progress < 0 || 100 < progress)
	{
		return ST::format("minDropProgress of item '{}' must be 0-100 or \"blocked\", not {}",
			item.getInternalName(), v.serialize());
	}
	item.setMinDropProgress(static_cast<uint8_t>(progress));
	return {};
}


// --- docs/item-texts-json-plan.md ---

namespace {

struct TextField
{
	char const* key;
	unsigned    size;
};

TextField const textFields[] =
{
	{ "shortName",           ITEM_TEXT_SHORT_NAME_SIZE },
	{ "name",                ITEM_TEXT_NAME_SIZE },
	{ "description",         ITEM_TEXT_DESCRIPTION_SIZE },
	{ "bobbyRayName",        ITEM_TEXT_BR_NAME_SIZE },
	{ "bobbyRayDescription", ITEM_TEXT_BR_DESCRIPTION_SIZE },
};

std::string JsonString(ST::string const& s)
{
	std::string out = "\"";
	for (char const c : std::string(s.c_str()))
	{
		switch (c)
		{
			case '"':  out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n";  break;
			case '\r': out += "\\r";  break;
			case '\t': out += "\\t";  break;
			default:
				if (static_cast<unsigned char>(c) < 0x20)
				{
					char buf[8];
					std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
					out += buf;
				}
				else
				{
					out += c; // UTF-8 bytes as they are
				}
		}
	}
	return out + "\"";
}

}


std::vector<ST::string> ApplyItemTexts(JsonValue const& list, ST::string const& fileName,
	std::function<ItemModel*(ST::string const&)> const& find)
{
	std::vector<ST::string> warnings;
	std::set<ST::string> seen;
	for (JsonValue const& element : list.toVec())
	{
		JsonObject const entry = element.toObject();
		ST::string const internalName = entry.GetString("internalName");
		if (!seen.insert(internalName).second)
		{
			throw DataError(ST::format("{}: item '{}' is listed twice", fileName, internalName));
		}
		ItemModel* const item = find(internalName);
		if (!item)
		{
			throw DataError(ST::format("{}: there is no item '{}'", fileName, internalName));
		}

		ST::string texts[std::size(textFields)];
		bool       has[std::size(textFields)] = {};
		for (size_t i = 0; i != std::size(textFields); ++i)
		{
			TextField const& f = textFields[i];
			if (!entry.has(f.key)) continue;
			has[i]   = true;
			texts[i] = entry.GetString(f.key);
			size_t const length = texts[i].to_utf32().size();
			if (length > f.size)
			{
				warnings.push_back(ST::format("{}: {} {} has {} characters (the old limit is {}, the text may be cut)",
					fileName, internalName, f.key, length, f.size));
			}
		}
		item->setTexts(has[0] ? &texts[0] : nullptr, has[1] ? &texts[1] : nullptr, has[2] ? &texts[2] : nullptr);
		if (has[3] || has[4])
		{
			item->setBobbyRayTexts(has[3] ? texts[3] : item->getBobbyRayName(),
				has[4] ? texts[4] : item->getBobbyRayDescription());
		}
	}
	return warnings;
}


std::string ItemTextsFileText(std::vector<ItemTextsEntry> const& entries)
{
	std::string out =
		"// Item names and descriptions of this language (docs/item-texts-json-plan.md), exported\n"
		"// from itemdesc.edt and braydesc.edt (JA2_EXPORT_ITEM_TEXTS). A field left out falls back to\n"
		"// the item file, then to the EDT files.\n"
		"[";
	bool first = true;
	for (ItemTextsEntry const& e : entries)
	{
		ST::string const* const values[] = { &e.shortName, &e.name, &e.description, &e.bobbyRayName, &e.bobbyRayDescription };
		bool any = false;
		for (ST::string const* v : values) any = any || !v->empty();
		if (!any) continue; // T8

		out += first ? "\n" : ",\n";
		first = false;
		out += "    {\n        \"internalName\": " + JsonString(e.internalName);
		for (size_t i = 0; i != std::size(textFields); ++i)
		{
			if (values[i]->empty()) continue;
			out += ",\n        \"" + std::string(textFields[i].key) + "\": " + JsonString(*values[i]);
		}
		out += "\n    }";
	}
	out += "\n]\n";
	return out;
}
