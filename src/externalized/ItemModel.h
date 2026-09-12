#pragma once

#include "GamePolicy.h"
#include "Item_Types.h"
#include "ItemStrings.h"
#include "InventoryGraphicsModel.h"
#include "Json.h"
#include "TilesetTileIndexModel.h"

#include <string_theory/string>
#include <optional>

class JsonObject;
class JsonObject;
struct ArmourModel;
struct ExplosiveModel;
struct MagazineModel;
struct WeaponModel;

struct ItemModel
{
	ItemModel(
		uint16_t itemIndex,
		ST::string&& internalName,
		uint32_t usItemClass,
		uint8_t classIndex=0,
		ItemCursor cursor=INVALIDCURS);

	ItemModel(
		uint16_t   itemIndex,
		ST::string&& internalName,
		ST::string&& shortName,
		ST::string&& name,
		ST::string&& description,
		uint32_t   usItemClass,
		uint8_t    ubClassIndex,
		ItemCursor ubCursor,
		InventoryGraphicsModel&& inventoryGraphics,
		TilesetTileIndexModel&& tileGraphic,
		uint8_t    ubWeight, // In hectogram, so f.e. ubWeight==4 means 400 grams.
		uint8_t    ubPerPocket,
		uint16_t   usPrice,
		uint8_t    ubCoolness,
		int8_t     bReliability,
		int8_t     bRepairEase,
		uint16_t   fFlags);

	virtual ~ItemModel() = default;

	const virtual ST::string& getInternalName() const;
	const virtual ST::string& getShortName() const;
	const virtual ST::string& getName() const;
	const virtual ST::string& getDescription() const;

	virtual uint16_t        getItemIndex() const;
	virtual uint32_t        getItemClass() const;
	virtual uint8_t         getClassIndex() const;
	virtual ItemCursor      getCursor() const;
	virtual const GraphicModel& getInventoryGraphicSmall() const;
	virtual const GraphicModel& getInventoryGraphicBig() const;
	virtual const TilesetTileIndexModel& getTileGraphic() const;
	virtual uint8_t         getWeight() const;
	virtual uint8_t         getPerPocket() const;
	// Independent, opt-in override of how many fit in a SMALL pocket
	// specifically (SMALLPOCK1-20) -- ItemSlotLimit() (Items.cc) falls back
	// to halving getPerPocket() when this is unset (empty optional), same
	// as it always has. A value of 0 here means "does not fit in a small
	// pocket at all", independent of ubPerPocket's own (big-pocket) value --
	// see ubSmallPerPocket below.
	virtual std::optional<uint8_t> getSmallPerPocket() const;
	virtual uint16_t        getPrice() const;
	virtual uint8_t         getCoolness() const;
	virtual int8_t          getReliability() const;
	virtual int8_t          getRepairEase() const;
	virtual uint16_t        getFlags() const;

	virtual bool isAmmo() const;
	virtual bool isArmour() const;
	virtual bool isBlade() const;
	virtual bool isBomb() const;
	virtual bool isExplosive() const;
	virtual bool isFace() const;
	virtual bool isGrenade() const;
	virtual bool isGun() const;
	virtual bool isKey() const;
	virtual bool isKit() const;
	virtual bool isLauncher() const;
	virtual bool isMedkit() const;
	virtual bool isMisc() const;
	virtual bool isMoney() const;
	virtual bool isPunch() const;
	virtual bool isTentacles() const;
	virtual bool isThrowingKnife() const;
	virtual bool isThrown() const;
	virtual bool isWeapon() const;

	virtual bool isTwoHanded() const;
	virtual bool isInBigGunList() const;

	virtual const WeaponModel* asWeapon() const   { return NULL; }
	virtual const MagazineModel* asAmmo() const   { return NULL; }
	virtual const ArmourModel* asArmour() const   { return NULL; }
	virtual const ExplosiveModel* asExplosive() const   { return NULL; }

	/** Check if the given attachment can be attached to the item. */
	virtual bool canBeAttached(const GamePolicy* policy, const ItemModel* attachment) const;

	struct InitData
	{
		JsonObject const& json;
		BinaryData const& strings;
	};

	virtual JsonValue serialize() const;
	static const ItemModel* deserialize(const JsonValue &json, const BinaryData& vanillaItemStrings);

protected:
	static ST::string deserializeShortName(InitData const&);
	static ST::string deserializeName(InitData const&);
	static ST::string deserializeDescription(InitData const&);

	uint16_t   itemIndex;
	ST::string internalName;
	ST::string shortName;
	ST::string name;
	ST::string description;
	uint32_t   usItemClass;
	uint8_t    ubClassIndex;
	ItemCursor ubCursor;
	InventoryGraphicsModel inventoryGraphics;
	TilesetTileIndexModel tileGraphic;
	uint8_t    ubWeight; //2 units per kilogram; roughly 1 unit per pound
	uint8_t    ubPerPocket;
	// Unset (empty optional) unless the item's own JSON explicitly sets
	// "ubSmallPerPocket" -- see getSmallPerPocket() above. Deliberately not
	// a plain uint8_t: 0 is itself a meaningful value ("doesn't fit"), so a
	// magic-number sentinel for "unset" would collide with it.
	std::optional<uint8_t> ubSmallPerPocket;
	uint16_t   usPrice;
	uint8_t    ubCoolness;
	int8_t     bReliability;
	int8_t     bRepairEase;
	uint16_t   fFlags;

	void serializeFlags(JsonObject &obj) const;
	static uint16_t deserializeFlags(const JsonObject &obj);

	void serializeSmallPerPocket(JsonObject &obj) const;
	static std::optional<uint8_t> deserializeSmallPerPocket(const JsonObject &obj);
};
