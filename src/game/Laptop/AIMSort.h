#ifndef __AIMSORT_H_
#define __AIMSORT_H_

#include "Types.h"

extern UINT8 gubCurrentSortMode;
extern UINT8 gubCurrentListMode;


// sort modes 0-12 are the criteria (see str_aim_sort_list), then the order
#define AIM_SORT_NAME	6
#define AIM_ASCEND	13
#define AIM_DESCEND	14


void GameInitAimSort(void);
void EnterAimSort(void);
void ExitAimSort(void);
void RenderAimSort(void);

#endif
