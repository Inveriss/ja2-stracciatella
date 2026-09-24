#ifndef __AIMSORT_H_
#define __AIMSORT_H_

#include "JA2Types.h"
#include "Types.h"

extern UINT8 gubCurrentSortMode;
extern UINT8 gubCurrentListMode;

// TRUE if the merc matches the (up to 2) skill filters selected on the AIM Sort screen, or if
// none are selected. Used by ResetAimMercArray() to also filter AimMercArray by skill.
BOOLEAN MercMatchesAimSkillFilter(ProfileID id);


// sort modes 0-12 are the criteria (see str_aim_sort_list), then the order
#define AIM_SORT_NAME	6
#define AIM_ASCEND	13
#define AIM_DESCEND	14


// Sorts AimMercArray by the current criterion and order.
void SortAimMercArray(void);
void GameInitAimSort(void);
void EnterAimSort(void);
void ExitAimSort(void);
void RenderAimSort(void);

#endif
