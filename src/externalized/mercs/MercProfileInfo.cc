#include "enums.h"
#include "MercProfileInfo.h"
#include "Soldier_Control.h"
#include "Soldier_Profile_Type.h"
#include <algorithm>
#include <utility>


std::function<const MercProfileInfo*(ProfileID)> MercProfileInfo::load = {};

MercProfileInfo::MercProfileInfo(uint8_t profileID_, ST::string internalName_, MercType mercType_, uint8_t weaponSaleModifier_, ST::string biography_, ST::string additionalInfo_)
	: internalName(std::move(internalName_)), profileID(profileID_), mercType(mercType_), weaponSaleModifier(weaponSaleModifier_), biography(std::move(biography_)), additionalInfo(std::move(additionalInfo_))
{
}

MercProfileInfo::MercProfileInfo()
	: internalName(""), profileID(NO_PROFILE), mercType(MercType::NOT_USED), weaponSaleModifier(100), biography(), additionalInfo()
{
}

MercProfileInfo *MercProfileInfo::deserialize(const JsonObject& json, const ST::string& biographyOverride, const ST::string& additionalInfoOverride)
{
	return new MercProfileInfo(
		json.GetUInt("profileID"),
		json.GetString("internalName"),
		Internals::getMercTypeEnumFromString(json.GetString("type")),
		json.getOptionalInt("weaponSaleModifier", 100),
		biographyOverride.empty() ? json.getOptionalString("biography") : biographyOverride,
		additionalInfoOverride.empty() ? json.getOptionalString("additionalInfo") : additionalInfoOverride
		);
}

void MercProfileInfo::validateData(const std::map<uint8_t, const MercProfileInfo*> &models)
{
	if (models.empty())
	{
		throw std::runtime_error("No merc profile info defined");
	}
	if (models.size() > NUM_PROFILES)
	{
		throw std::runtime_error("Too many merc profiles");
	}
	for (auto const& p : models)
	{
		if (p.first != p.second->profileID)
		{
			throw std::logic_error("profileID is not consistent");
		}
		if (p.first >= NUM_PROFILES || p.first == NO_PROFILE)
		{
			throw std::runtime_error("Invalid profileID");
		}
	}

	if (!MercProfileInfo::load)
	{
		throw std::runtime_error("MercProfileInfo::load has not been initialized");
	}
}
