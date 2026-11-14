#include "CalibreModel.h"
#include "ContentManager.h"
#include "Exceptions.h"
#include "GameInstance.h"
#include "Json.h"
#include <string_theory/format>
#include <string_theory/string>
#include <stdexcept>
#include <utility>


JsonValue CalibreModel::serialize() const
{
	JsonObject obj;
	obj.set("index",              index);
	obj.set("internalName",       internalName);
	obj.set("sound",              sound);
	obj.set("burstSound",         burstSound);
	obj.set("silencedSound",      silencedSound);
	obj.set("silencedBurstSound", silencedBurstSound);
	obj.set("showInHelpText",     showInHelpText);
	obj.set("monsterWeapon",      monsterWeapon);
	return obj.toValue();
}

CalibreModel* CalibreModel::deserialize(const JsonValue &json)
{
	return new CalibreModel{ json.toObject() };
}

CalibreModel::CalibreModel(JsonObject const jsonObj) :
	index{ static_cast<uint16_t>(jsonObj.GetInt("index")) },
	internalName{ jsonObj.GetString("internalName") },
	sound{ jsonObj.getOptionalString("sound") },
	burstSound{ jsonObj.getOptionalString("burstSound") },
	silencedSound{ jsonObj.getOptionalString("silencedSound") },
	silencedBurstSound{ jsonObj.getOptionalString("silencedBurstSound") },
	showInHelpText{ jsonObj.GetBool("showInHelpText") },
	monsterWeapon{ jsonObj.GetBool("monsterWeapon") }
{
}

const ST::string* CalibreModel::getName() const
{
	return GCM->getCalibreName(index);
}

const CalibreModel* CalibreModel::getNoCalibreObject()
{
	static CalibreModel noCalibre;
	return &noCalibre;
}

const CalibreModel* getCalibre(const ST::string& calibreName,
				const std::map<ST::string, const CalibreModel*> &calibreMap)
{
	auto it = calibreMap.find(calibreName);
	if(it != calibreMap.end())
	{
		return it->second;
	}

	throw std::runtime_error(ST::format("calibre '{}' is not found", calibreName).to_std_string());
}


void AddCalibreToTable(std::vector<const CalibreModel*>& table, const CalibreModel* const calibre)
{
	if (table.size() <= calibre->index) table.resize(calibre->index + 1);

	const CalibreModel* const taken = table[calibre->index];
	if (taken != nullptr)
	{
		throw DataError(ST::format("calibres.json: index {} is used by '{}' and '{}'",
			calibre->index, taken->internalName, calibre->internalName));
	}
	table[calibre->index] = calibre;
}


void CheckCalibreTable(const std::vector<const CalibreModel*>& table)
{
	if (table.size() > 256)
	{
		throw DataError(ST::format("calibres.json: index {} is above 255", table.size() - 1));
	}

	std::set<ST::string> names;
	for (size_t i = 0; i != table.size(); ++i)
	{
		if (table[i] == nullptr)
		{
			throw DataError(ST::format("calibres.json: no calibre with index {} (the indices must go on without a gap)", i));
		}
		if (!names.insert(table[i]->internalName).second)
		{
			throw DataError(ST::format("calibres.json: internalName '{}' is used twice", table[i]->internalName));
		}
	}
}


void CheckCalibreNames(const std::vector<const CalibreModel*>& table,
	size_t const numNames, const ST::string& namesFile,
	size_t const numBobbyRayNames, const ST::string& bobbyRayNamesFile)
{
	if (numNames < table.size())
	{
		throw DataError(ST::format("{}: {} names for {} calibres; the name of calibre {} '{}' is missing (a name goes at the position of the calibre's index)",
			namesFile, numNames, table.size(), numNames, table[numNames]->internalName));
	}
	if (numBobbyRayNames < table.size())
	{
		throw DataError(ST::format("{}: {} names for {} calibres; the name of calibre {} '{}' is missing (a name goes at the position of the calibre's index)",
			bobbyRayNamesFile, numBobbyRayNames, table.size(), numBobbyRayNames, table[numBobbyRayNames]->internalName));
	}
}
