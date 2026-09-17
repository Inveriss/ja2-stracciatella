#ifndef __IMP_COMPILE_H
#define __IMP_COMPILE_H

#include "Types.h"

#define PLAYER_GENERATED_CHARACTER_ID 51
#define NUMBER_OF_PLAYER_PORTRAITS 16

// Number of player-generated (IMP) merc slots available in a single game,
// one per voice/gender combination (PLAYER_GENERATED_CHARACTER_ID..+5).
#define MAX_IMP_MERCS 6

void AddAnAttitudeToAttitudeList( INT8 bAttitude );
void CreateACharacterFromPlayerEnteredStats( void );
void CreatePlayersPersonalitySkillsAndAttitude( void );
void AddAPersonalityToPersonalityList( INT8 bPersonlity );
void AddSkillToSkillList( INT8 bSkill );
void ResetSkillsAttributesAndPersonality( void );
void HandleMercStatsForChangesInFace( void );

#endif
