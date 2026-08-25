#ifndef LOADSAVEOBJECTTYPE_H
#define LOADSAVEOBJECTTYPE_H

#include "Item_Types.h"
#include "LoadSaveData.h"


void ExtractObject(DataReader& d, OBJECTTYPE* o);

void InjectObject(DataWriter& d, const OBJECTTYPE* o);

// Parses/writes the original (vanilla) 36-byte, 4-attachment-slot OBJECTTYPE
// layout. Frozen forever - used only to read/write sector map files (the ones
// shipped with the base game, and any authored with the in-game map editor).
// ExtractLegacyObject() defaults attachment slots beyond the legacy 4 to
// NOTHING/0; InjectLegacyObject() silently drops any attachment in those slots,
// since this format has nowhere on disk to put them.
void ExtractLegacyObject(DataReader& d, OBJECTTYPE* o);
void InjectLegacyObject(DataWriter& d, const OBJECTTYPE* o);

#endif
