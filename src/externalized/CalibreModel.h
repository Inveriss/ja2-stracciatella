#pragma once

#include "Json.h"

#include <map>
#include <set>
#include <vector>
#include <stdint.h>
#include <string_theory/string>

struct CalibreModel
{
	static constexpr uint16_t NOAMMO = 0;

	[[nodiscard]] const ST::string* getName() const;

	[[nodiscard]] JsonValue serialize() const;
	[[nodiscard]] static CalibreModel* deserialize(const JsonValue &json);

	[[nodiscard]] static const CalibreModel* getNoCalibreObject();

	uint16_t index{ NOAMMO };
	ST::string internalName{ "NO_CALIBRE" };
	ST::string sound;
	ST::string burstSound;
	ST::string silencedSound;
	ST::string silencedBurstSound;
	bool showInHelpText{ false };
	bool monsterWeapon{ false };

private:
	CalibreModel() = default;
	CalibreModel(JsonObject jsonObj);
};

const CalibreModel* getCalibre(const ST::string& calibreName,
				const std::map<ST::string, const CalibreModel*> &calibreMap);

/** Puts a calibre of calibres.json into the table at its index. Throws DataError
 * when the index is taken already. */
void AddCalibreToTable(std::vector<const CalibreModel*>& table, const CalibreModel* calibre);

/** Checks the table of calibres once calibres.json is loaded: the indices go
 * 0, 1, 2 ... without a gap, at most 255 (the names are found by a uint8
 * index), no internalName twice. Throws DataError naming the problem. */
void CheckCalibreTable(const std::vector<const CalibreModel*>& table);

/** Both name lists (in the game, at Bobby Ray's) have a name for every
 * calibre, since a name is found by its position in the list (= the index).
 * Throws DataError naming the file and the calibre without a name. */
void CheckCalibreNames(const std::vector<const CalibreModel*>& table,
	size_t numNames, const ST::string& namesFile,
	size_t numBobbyRayNames, const ST::string& bobbyRayNamesFile);
