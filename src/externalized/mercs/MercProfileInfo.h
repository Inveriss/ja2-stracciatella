#pragma once

#include "JA2Types.h"
#include "Json.h"

#include <map>

enum class MercType : int8_t
{
	NOT_USED,
	AIM,
	MERC,
	IMP,
	RPC,
	NPC,
	VEHICLE
};

// Read-only data supplementary to the merc profiles
class MercProfileInfo
{
public:
	// Creates an empty instance, of ID NO_PROFILE and type NOT_USED
	MercProfileInfo();

	const ST::string internalName;
	const uint8_t profileID;
	const MercType mercType;
	const uint8_t weaponSaleModifier;
	// A.I.M. description and additional information (shown on the mercenary's file page). Only
	// needed for A.I.M. mercs added to the game: the original ones have them in aimbios.edt.
	const ST::string biography;
	const ST::string additionalInfo;
	// The number of the portrait files (faces/NNN.sti and its 33face, 65face, bigfaces versions) of
	// the profile; it is the profile ID unless the profile says otherwise ("faceIndex").
	const uint8_t faceIndex;

	// A function to provide MercProfileInfo by given ProfileIDs. This is to
	// avoid a circular reference to ContentManager.
	// This function must be initialized at init
	static std::function<const MercProfileInfo *(ProfileID)> load;
	// The description texts of the names file of the game language (if any) take precedence
	// over the ones of the JSON object.
	static MercProfileInfo* deserialize(const JsonObject& json, const ST::string& biographyOverride = ST::string(), const ST::string& additionalInfoOverride = ST::string());
	static void validateData(const std::map<uint8_t, const MercProfileInfo*>& models);

protected:
	MercProfileInfo(uint8_t profileID_, ST::string internalName_, MercType mercType_, uint8_t weaponSaleModifier_, ST::string biography_, ST::string additionalInfo_, uint8_t faceIndex_);
};
