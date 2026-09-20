#include "Directories.h"
#include "MercPortrait.h"
#include "MercProfile.h"
#include "Soldier_Profile.h"
#include "IMP_Compile_Character.h"
#include "Soldier_Profile_Type.h"
#include "VObject.h"


bool IsImpPortrait(ProfileID const id)
{
	MERCPROFILESTRUCT const& p = GetProfile(id);
	return p.ubFaceIndex >= IMP_PORTRAIT_FIRST &&
		p.ubFaceIndex < IMP_PORTRAIT_FIRST + NUMBER_OF_PLAYER_PORTRAITS &&
		MercProfile(id).isIMPMerc();
}


ST::string ImpPortraitFilePath(char const* const subdir, int const portrait)
{
	return ST::format(FACESDIR "/imp/{}{02d}.sti", subdir, IMP_PORTRAIT_FILE_FIRST + portrait);
}


ST::string PortraitFilePath(ProfileID const id, char const* const subdir, char const* const prefix)
{
	MERCPROFILESTRUCT const& p = GetProfile(id);
	if (IsImpPortrait(id))
	{
		int const portrait = p.ubFaceIndex - IMP_PORTRAIT_FIRST;
		return ST::format(FACESDIR "/imp/{}{}{02d}.sti", subdir, prefix, IMP_PORTRAIT_FILE_FIRST + portrait);
	}
	return ST::format(FACESDIR "/{}{}{02d}.sti", subdir, prefix, p.ubFaceIndex);
}


static SGPVObject* LoadPortrait(MERCPROFILESTRUCT const& p, char const* const subdir)
{
	ST::string filename = ST::format(FACESDIR "/{}{02d}.sti", subdir, p.ubFaceIndex);
	if (&p >= gMercProfiles && &p < gMercProfiles + NUM_PROFILES)
	{
		filename = PortraitFilePath(static_cast<ProfileID>(&p - gMercProfiles), subdir);
	}
	return AddVideoObjectFromFile(filename);
}


SGPVObject* Load33Portrait(   MERCPROFILESTRUCT const& p) { return LoadPortrait(p, "33face/");   }
SGPVObject* Load65Portrait(   MERCPROFILESTRUCT const& p) { return LoadPortrait(p, "65face/");   }
SGPVObject* LoadBigPortrait(  MERCPROFILESTRUCT const& p) { return LoadPortrait(p, "bigfaces/"); }
SGPVObject* LoadSmallPortrait(MERCPROFILESTRUCT const& p) { return LoadPortrait(p, "");          }
