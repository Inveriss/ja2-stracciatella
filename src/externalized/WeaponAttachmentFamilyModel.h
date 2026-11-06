#pragma once

#include "Json.h"

#include <string_theory/string>

#include <map>
#include <set>
#include <stdint.h>
#include <vector>

struct CalibreModel;
struct WeaponModel;

// A family of weapon attachments (all quality levels of one kind of
// attachment) and the rule which weapons take it, from
// weapon-attachment-families.json (docs/weapon-attachments-plan.md, W2/W4).
struct WeaponAttachmentFamilyModel
{
	ST::string            name;
	std::set<ST::string>  weaponTypes; // WeaponModel::internalType values
	std::set<ST::string>  calibres;    // calibre internal names, empty = any
	std::set<ST::string>  excludes;    // families that can't be on the same weapon
	int8_t                skillCheck = 0;    // SkillChecks value for attaching it
	int8_t                skillCheckModifier = 0;

	// Does the family rule (weapon type and calibre) take this weapon? An empty
	// weaponTypes list takes no weapon: those families only attach through the
	// weapons' own allow lists (or the legacy attachment_Xxx flags).
	bool fitsWeapon(WeaponModel const& weapon) const;

	// Do the two families exclude each other (either one lists the other)?
	bool excludesFamily(WeaponAttachmentFamilyModel const& other) const;

	// Throws DataError on unknown weapon types, calibres or skill checks.
	static WeaponAttachmentFamilyModel* deserialize(JsonValue const& json,
		std::map<ST::string, const CalibreModel*> const& calibreMap);

	// The weapon types a family rule may name (weapons.json internalType values)
	static std::set<ST::string> const& knownWeaponTypes();
};
