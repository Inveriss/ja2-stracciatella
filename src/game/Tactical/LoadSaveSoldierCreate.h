#ifndef LOADSAVESOLIDERCREATE_H
#define LOADSAVESOLIDERCREATE_H

#include "Soldier_Create.h"


UINT16 CalcSoldierCreateCheckSum(const SOLDIERCREATE_STRUCT* const s);

// Current format (OBJECTTYPE at the current MAX_ATTACHMENTS). Used only for our
// own save data (see Enemy_Soldier_Save.cc) - never for sector map files.
void ExtractSoldierCreateFromFile(HWFILE, SOLDIERCREATE_STRUCT*, bool stracLinuxFormat);

/**
* Load SOLDIERCREATE_STRUCT structure and checksum from the file and guess the
* format the structure was saved in (vanilla windows format or stracciatella linux format). */
void ExtractSoldierCreateFromFileWithChecksumAndGuess(HWFILE, SOLDIERCREATE_STRUCT*, UINT16 *checksum);

void InjectSoldierCreateIntoFile(HWFILE, SOLDIERCREATE_STRUCT const*);

// Frozen, original (vanilla) 4-attachment-slot format. Used only for reading/
// writing sector map files (the ones shipped with the base game, and any
// authored with the in-game map editor) - see ExtractLegacyObject().
void ExtractLegacySoldierCreateFromFile(HWFILE, SOLDIERCREATE_STRUCT*, bool stracLinuxFormat);
void InjectLegacySoldierCreateIntoFile(HWFILE, SOLDIERCREATE_STRUCT const*);

#endif
