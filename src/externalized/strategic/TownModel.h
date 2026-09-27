#pragma once

#include "Json.h"
#include "Types.h"
#include <vector>

// Pixel offset (may be negative) of a town's grey border on the strategic
// map ("Show Towns"), on top of its sector-grid position.
struct TownBorderOffset
{
	INT16 x = 0;
	INT16 y = 0;
};

class TownModel
{
public:
	TownModel(int8_t townId_, ST::string&& internalName_, std::vector<uint8_t>&& sectorIDs_, SGPPoint townPoint_, bool isMilitiaTrainingAllowed_,
		TownBorderOffset borderOffset1024_, TownBorderOffset borderOffset1280_);

	// Returns the top-left corner of the town on map. It may or may not belong to the town.
	SGPSector getBaseSector() const;
	static TownModel* deserialize(const JsonValue& obj);

	int8_t townId;
	ST::string internalName;
	std::vector<uint8_t> sectorIDs;
	SGPPoint townPoint;
	bool isMilitiaTrainingAllowed;
	// Border offsets per strategic-screen height tier -- optional
	// "borderOffset1024" (height 768+) / "borderOffset1280" (height 720-767)
	// in strategic-map-towns.json, 0/0 when absent.
	TownBorderOffset borderOffset1024;
	TownBorderOffset borderOffset1280;
};
