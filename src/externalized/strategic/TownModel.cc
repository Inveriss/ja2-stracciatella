#include "TownModel.h"

#include "JsonUtility.h"

TownModel::TownModel(int8_t townId_, ST::string&& internalName_, std::vector<uint8_t>&& sectorIDs_, SGPPoint townPoint_, bool isMilitiaTrainingAllowed_,
		TownBorderOffset borderOffset1024_, TownBorderOffset borderOffset1280_)
	: townId(townId_), internalName(std::move(internalName_)), sectorIDs(std::move(sectorIDs_)), townPoint(townPoint_), isMilitiaTrainingAllowed(isMilitiaTrainingAllowed_),
	  borderOffset1024(borderOffset1024_), borderOffset1280(borderOffset1280_) {}

// Optional { "x": .., "y": .. } object; 0/0 (or 0 for a missing coordinate)
// when absent.
static TownBorderOffset parseOptionalBorderOffset(const JsonObject& obj, const char* name)
{
	TownBorderOffset offset;
	if (obj.has(name))
	{
		auto o = obj[name].toObject();
		offset.x = static_cast<INT16>(o.getOptionalInt("x"));
		offset.y = static_cast<INT16>(o.getOptionalInt("y"));
	}
	return offset;
}

SGPSector TownModel::getBaseSector() const
{
	SGPSector min(99, 99);
	for ( auto sectorID : sectorIDs ) {
		SGPSector sector(sectorID);
		if (sector < min) min = sector;
	}
	return min;
}

TownModel* TownModel::deserialize(const JsonValue& json)
{
	std::vector<uint8_t> sectorIDs = JsonUtility::parseSectorList(json, "sectors");
	auto obj = json.toObject();

	auto tp = obj["townPoint"].toObject();
	SGPPoint townPoint = SGPPoint();
	townPoint.iX = tp.GetInt("x");
	townPoint.iY = tp.GetInt("y");

	return new TownModel(
		obj.GetInt("townId"),
		obj.getOptionalString("internalName"),
		std::move(sectorIDs),
		townPoint,
		obj.getOptionalBool("isMilitiaTrainingAllowed"),
		parseOptionalBorderOffset(obj, "borderOffset1024"),
		parseOptionalBorderOffset(obj, "borderOffset1280")
		);
}
