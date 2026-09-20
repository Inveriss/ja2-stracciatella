#ifndef MERCPORTRAIT_H
#define MERCPORTRAIT_H

#include "JA2Types.h"
#include <string_theory/string>

SGPVObject* Load33Portrait(MERCPROFILESTRUCT const&);
SGPVObject* Load65Portrait(MERCPROFILESTRUCT const&);
SGPVObject* LoadBigPortrait(MERCPROFILESTRUCT const&);
SGPVObject* LoadSmallPortrait(MERCPROFILESTRUCT const&);

// The portraits of the IMP characters are in faces/imp/ (with the same sub folders as faces/), so
// that the numbers of faces/ can be used by other profiles. The ubFaceIndex of an IMP character is
// IMP_PORTRAIT_FIRST + the number of the portrait (0-15), the files of the portraits are named
// IMP_PORTRAIT_FILE_FIRST + the number of the portrait.
#define IMP_PORTRAIT_FIRST      200
#define IMP_PORTRAIT_FILE_FIRST 51

// Is this profile an IMP character with one of the IMP portraits?
bool IsImpPortrait(ProfileID id);

// The path of the portrait file of a profile: subdir is "", "33face/", "65face/" or "bigfaces/",
// prefix "" or "b" (big face of the tactical dialogues). An IMP portrait is only taken from
// faces/imp/; if the file is not there, it is a missing file like any other.
ST::string PortraitFilePath(ProfileID id, char const* subdir, char const* prefix = "");

// The path of the file of IMP portrait number 0-15 in faces/imp/ (subdir as above)
ST::string ImpPortraitFilePath(char const* subdir, int portrait);

#endif
