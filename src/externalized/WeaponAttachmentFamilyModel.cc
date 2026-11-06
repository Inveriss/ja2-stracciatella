#include "WeaponAttachmentFamilyModel.h"

#include "CalibreModel.h"
#include "Exceptions.h"
#include "SkillCheck.h"
#include "WeaponModels.h"

#include <string_theory/format>


std::set<ST::string> const& WeaponAttachmentFamilyModel::knownWeaponTypes()
{
	// the internalType values of weapons.json (weapons.schema.yaml)
	static std::set<ST::string> const types{
		"PUNCH", "PISTOL", "M_PISTOL", "SMG", "RIFLE", "SN_RIFLE", "ASRIFLE", "SHOTGUN",
		"LMG", "BLADE", "THROWINGBLADE", "THROWN", "LAUNCHER", "MONSTSPIT", "PUNCHWEAPON",
		"LAW", "CANNON", "NOWEAPON"
	};
	return types;
}


bool WeaponAttachmentFamilyModel::fitsWeapon(WeaponModel const& weapon) const
{
	if (weaponTypes.count(weapon.internalType) == 0) return false;
	if (calibres.empty()) return true;
	return weapon.calibre && calibres.count(weapon.calibre->internalName) != 0;
}


bool WeaponAttachmentFamilyModel::excludesFamily(WeaponAttachmentFamilyModel const& other) const
{
	return excludes.count(other.name) != 0 || other.excludes.count(name) != 0;
}


static int8_t ParseSkillCheck(ST::string const& family, ST::string const& value)
{
	if (value.empty() || value == "NONE")         return NO_CHECK;
	if (value == "SPECIAL_ITEM")                  return ATTACHING_SPECIAL_ITEM_CHECK;
	if (value == "SPECIAL_ELECTRONIC_ITEM")       return ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK;
	throw DataError(ST::format("weapon-attachment-families.json: family '{}' has the unknown skillCheck '{}' (NONE, SPECIAL_ITEM or SPECIAL_ELECTRONIC_ITEM)", family, value));
}


static std::set<ST::string> ReadNames(JsonObject const& obj, char const* field)
{
	std::set<ST::string> names;
	if (!obj.has(field)) return names;
	for (JsonValue const& v : obj[field].toVec()) names.insert(v.toString());
	return names;
}


WeaponAttachmentFamilyModel* WeaponAttachmentFamilyModel::deserialize(JsonValue const& json,
	std::map<ST::string, const CalibreModel*> const& calibreMap)
{
	JsonObject const obj = json.toObject();
	auto* const f = new WeaponAttachmentFamilyModel{};
	f->name        = obj.GetString("family");
	f->weaponTypes = ReadNames(obj, "weaponTypes");
	f->calibres    = ReadNames(obj, "calibres");
	f->excludes    = ReadNames(obj, "excludes");
	f->skillCheckModifier = static_cast<int8_t>(obj.getOptionalInt("skillCheckModifier"));

	try
	{
		f->skillCheck = ParseSkillCheck(f->name, obj.getOptionalString("skillCheck"));
		for (ST::string const& type : f->weaponTypes)
		{
			if (knownWeaponTypes().count(type) == 0)
			{
				throw DataError(ST::format("weapon-attachment-families.json: family '{}' names the unknown weapon type '{}'", f->name, type));
			}
		}
		for (ST::string const& calibre : f->calibres)
		{
			if (calibreMap.count(calibre) == 0)
			{
				throw DataError(ST::format("weapon-attachment-families.json: family '{}' names the unknown calibre '{}'", f->name, calibre));
			}
		}
	}
	catch (...)
	{
		delete f;
		throw;
	}
	return f;
}
