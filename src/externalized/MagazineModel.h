#pragma once

#include "ItemModel.h"
#include "ItemStrings.h"

#include <string_theory/string>

#include <map>
#include <stdint.h>
#include <vector>

class JsonObject;
class JsonObject;
struct AmmoTypeModel;
struct CalibreModel;

struct MagazineModel : ItemModel
{
	MagazineModel(uint16_t itemIndex,
			ST::string&& internalName,
			ST::string&& shortName,
			ST::string&& name,
			ST::string&& description,
			uint32_t  itemClass,
			const CalibreModel *calibre,
			uint16_t capacity,
			const AmmoTypeModel *ammoType,
			bool dontUseAsDefaultMagazine
	);

	virtual const MagazineModel* asAmmo() const   { return this; }

	virtual JsonValue serialize() const;

	static MagazineModel* deserialize(const JsonValue &json,
						const std::map<ST::string, const CalibreModel*> &calibreMap,
						const std::map<ST::string, const AmmoTypeModel*> &ammoTypeMap,
						const BinaryData& vanillaItemStrings);


	/** Get standard replacement ammo name. */
	virtual const ST::string & getStandardReplacement() const;

	/** One message for every magazine that has the calibre, capacity and ammo type
	 * of a magazine with a lower index: a gun loaded with another size of that ammo
	 * then always remembers the one with the lowest index
	 * (docs/magazine-size-variants-plan.md, M3). Magazines without a calibre are
	 * skipped. */
	static std::vector<ST::string> describeDuplicates(std::vector<const MagazineModel*> const& magazines);

	ST::string standardReplacement;
	const CalibreModel *calibre;
	const uint16_t capacity;
	const AmmoTypeModel *ammoType;
	const bool dontUseAsDefaultMagazine;

};
