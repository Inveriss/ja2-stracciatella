#ifndef __AIM_H
#define __AIM_H

#include "Types.h"

#define AIMHISTORYFILE			BINARYDATADIR "/aimhist.edt"
#define AIM_HISTORY_LINE_SIZE		400


// Room for the 40 original A.I.M. mercs (profiles 0-39) and the 35 profiles 165-199 that are
// free for added characters (see NUM_PROFILES; profile 164 stands in for two vehicle types).
#define MAX_NUMBER_MERCS		75
// The number of A.I.M. mercs (profiles of type "AIM"), at most MAX_NUMBER_MERCS.
extern UINT8 gubNumAimMercs;
// The first mercs of the A.I.M. (the original ones) have replies etc. in the game data that
// is indexed by their profile ID.
#define NUM_ORIGINAL_AIM_MERCS		40
// The profile IDs of the A.I.M. mercs; the order is set by the sort page.
extern UINT8 AimMercArray[MAX_NUMBER_MERCS];
// Fill AimMercArray with all A.I.M. mercs in profile ID order.
void ResetAimMercArray(void);

#define NUM_AIM_SCREENS			6

#define IMAGE_OFFSET_X			LAPTOP_SCREEN_UL_X//111
#define IMAGE_OFFSET_Y			LAPTOP_SCREEN_WEB_UL_Y//24


#define AIM_LOGO_TEXT_X			(175 + STD_SCREEN_X)
#define AIM_LOGO_TEXT_Y			(77 + LAPTOP_SCREEN_WEB_DELTA_Y + 4 + STD_SCREEN_Y)
#define AIM_LOGO_TEXT_WIDTH		360

// Aim Symbol 203, 51
#define AIM_SYMBOL_SIZE_Y		51

//262, 28
#define AIM_SYMBOL_X			IMAGE_OFFSET_X + 149
#define AIM_SYMBOL_Y			IMAGE_OFFSET_Y + 3
#define AIM_SYMBOL_WIDTH		203
#define AIM_SYMBOL_HEIGHT		51

// small logo (AIMSYMBOL_SMALL.STI, 102x26) used on the sort page
#define AIM_SMALL_SYMBOL_X		IMAGE_OFFSET_X + 4
#define AIM_SMALL_SYMBOL_Y		IMAGE_OFFSET_Y + 4
#define AIM_SMALL_SYMBOL_WIDTH		102
#define AIM_SMALL_SYMBOL_HEIGHT		26

// RustBackGround
#define RUSTBACKGROUND_SIZE_X		125
#define RUSTBACKGROUND_SIZE_Y		100

#define RUSTBACKGROUND_1_X		IMAGE_OFFSET_X
#define RUSTBACKGROUND_1_Y		IMAGE_OFFSET_Y

//Bottom Buttons
#define NUM_AIM_BOTTOMBUTTONS		6
#define BOTTOM_BUTTON_START_WIDTH	75
#define BOTTOM_BUTTON_START_HEIGHT	18
#define BOTTOM_BUTTON_START_X		LAPTOP_SCREEN_UL_X + 25
#define BOTTOM_BUTTON_START_Y		LAPTOP_SCREEN_WEB_LR_Y - BOTTOM_BUTTON_START_HEIGHT - 3
#define BOTTOM_BUTTON_AMOUNT		NUM_AIM_SCREENS

#define AIM_LOGO_FONT			FONT10ARIAL
#define AIM_COPYRIGHT_FONT		FONT10ARIAL
#define AIM_WARNING_FONT		FONT12ARIAL
#define AIM_FONT12ARIAL			FONT12ARIAL
#define AIM_FONT_MCOLOR_WHITE		FONT_MCOLOR_WHITE
#define AIM_GREEN			157
#define AIM_MAINTITLE_COLOR		AIM_GREEN
#define AIM_MAINTITLE_FONT		FONT14ARIAL
#define AIM_BUTTON_ON_COLOR		FONT_MCOLOR_DKWHITE
#define AIM_BUTTON_OFF_COLOR		138
#define AIM_CONTENTBUTTON_WIDTH		205
#define AIM_CONTENTBUTTON_HEIGHT	19
#define AIM_FONT_GOLD			170


enum
{
	AIM_SLOGAN,
	AIM_WARNING_1,
	AIM_WARNING_2,
	AIM_COPYRIGHT_1,
	AIM_COPYRIGHT_2,
	AIM_COPYRIGHT_3
};

void GameInitAIM(void);
void EnterAIM(void);
void ExitAIM(void);
void HandleAIM(void);
void RenderAIM(void);

void ExitAimMenuBar();
void InitAimMenuBar();

void RemoveAimDefaults(void);
void InitAimDefaults(void);
// Use the small AIM logo (top-left corner) instead of the big one; call before
// InitAimDefaults() and reset after RemoveAimDefaults().
void SetAimSmallLogo(bool small);
void DrawAimDefaults(void);

void DisplayAimSlogan(void);
void DisplayAimCopyright(void);

void DisableAimButton(void);

#endif
