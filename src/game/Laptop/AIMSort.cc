#include "Cursors.h"
#include "Directories.h"
#include "Font.h"
#include "Laptop.h"
#include "AIMSort.h"
#include "AIM.h"
#include "VObject.h"
#include "WordWrap.h"
#include "Soldier_Profile.h"
#include "Text.h"
#include "GameRes.h"
#include "Button_System.h"
#include "Video.h"
#include "VSurface.h"
#include "Font_Control.h"
#include "Logger.h"

#include <algorithm>
#include <string_theory/string>


//#define

#define AIM_SORT_FONT_TITLE		FONT14ARIAL
#define AIM_SORT_FONT_SORT_TEXT		FONT10ARIAL

#define AIM_SORT_COLOR_SORT_TEXT	AIM_FONT_MCOLOR_WHITE
#define AIM_SORT_SORT_BY_COLOR		146
#define AIM_SORT_LINK_TEXT_COLOR	146

#define AIM_SORT_GAP_BN_ICONS		60
#define AIM_SORT_CHECKBOX_SIZE		10
#define AIM_SORT_ON			0
#define AIM_SORT_OFF			1

// SORTBY_LONG.STI is 432 wide: centered in the 500 pixel wide page
#define AIM_SORT_SORT_BY_X		(IMAGE_OFFSET_X + 34)
#define AIM_SORT_SORT_BY_Y		IMAGE_OFFSET_Y + 96

#define AIM_SORT_TO_MUGSHOTS_X		IMAGE_OFFSET_X + 89
#define AIM_SORT_TO_MUGSHOTS_Y		IMAGE_OFFSET_Y + 184
#define AIM_SORT_TO_MUGSHOTS_SIZE	54

#define AIM_SORT_TO_STATS_X		AIM_SORT_TO_MUGSHOTS_X
#define AIM_SORT_TO_STATS_Y		AIM_SORT_TO_MUGSHOTS_Y + AIM_SORT_GAP_BN_ICONS
#define AIM_SORT_TO_STATS_SIZE		AIM_SORT_TO_MUGSHOTS_SIZE

#define AIM_SORT_TO_ALUMNI_X		AIM_SORT_TO_MUGSHOTS_X
#define AIM_SORT_TO_ALUMNI_Y		AIM_SORT_TO_STATS_Y + AIM_SORT_GAP_BN_ICONS
#define AIM_SORT_TO_ALUMNI_SIZE		AIM_SORT_TO_MUGSHOTS_SIZE

#define AIM_SORT_AIM_MEMBER_X		(IMAGE_OFFSET_X + 155)
#define AIM_SORT_AIM_MEMBER_Y		(STD_SCREEN_Y + 105 + LAPTOP_SCREEN_WEB_DELTA_Y)
#define AIM_SORT_AIM_MEMBER_WIDTH	190

#define AIM_SORT_SORT_BY_TEXT_X		AIM_SORT_SORT_BY_X + 9
#define AIM_SORT_SORT_BY_TEXT_Y		AIM_SORT_SORT_BY_Y + 8

#define AIM_SORT_ASC_DESC_WIDTH		100


#define AIM_SORT_MUGSHOT_TEXT_X		(STD_SCREEN_X + 266)
#define AIM_SORT_MUGSHOT_TEXT_Y		(STD_SCREEN_Y + 230 + LAPTOP_SCREEN_WEB_DELTA_Y)

#define AIM_SORT_MERC_STATS_TEXT_X	AIM_SORT_MUGSHOT_TEXT_X
#define AIM_SORT_MERC_STATS_TEXT_Y	(STD_SCREEN_Y + 293 + LAPTOP_SCREEN_WEB_DELTA_Y)

#define AIM_SORT_ALUMNI_TEXT_X		AIM_SORT_MUGSHOT_TEXT_X
#define AIM_SORT_ALUMNI_TEXT_Y		(STD_SCREEN_Y + 351 + LAPTOP_SCREEN_WEB_DELTA_Y)

// "Filter by skill" box (SORTBY_LONG_SKILLS.STI, 116x192): up to 2 of these 14 checkboxes can
// be selected; AimMercArray is then reduced to mercs having (both of) the selected skill(s).
#define AIM_SORT_SKILLS_X			(STD_SCREEN_X + 460)
#define AIM_SORT_SKILLS_Y			(STD_SCREEN_Y + 230)
#define AIM_SORT_SKILLS_WIDTH			116
#define AIM_SORT_SKILLS_HEIGHT			192

#define NUM_AIM_SKILL_FILTERS			14
#define AIM_SORT_SKILLS_FIRST_ROW_Y		8
#define AIM_SORT_SKILLS_ROW_HEIGHT		13
#define AIM_SORT_SKILLS_CHECKBOX_X		9
// the checkbox squares baked into SORTBY_LONG_SKILLS.STI sit 3px higher than the text row
#define AIM_SORT_SKILLS_CHECKBOX_Y		(AIM_SORT_SKILLS_FIRST_ROW_Y - 3)
#define AIM_SORT_SKILLS_TEXT_X			(AIM_SORT_SKILLS_CHECKBOX_X + 14)
#define AIM_SORT_SKILLS_TEXT_WIDTH		(AIM_SORT_SKILLS_WIDTH - AIM_SORT_SKILLS_TEXT_X - 6)


struct AIMSortInfo
{
	UINT16         const x;
	UINT16         const y;
	UINT32         const align;
	UINT16         const index;
	MOUSE_CALLBACK const click;
	MOUSE_REGION         region;
};


static void SelectSortCriterionRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectAscendBoxRegionCallBack(    MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectDescendBoxRegionCallBack(   MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectSkillFilterRegionCallBack(  MOUSE_REGION* pRegion, UINT32 iReason);


// gzIMPSkillTraitsText[]'s entry order follows the IMP skill-selection screen's own layout, not
// the SkillTrait enum (see IMP_SkillTraits.cc's skillTraitsMapping) -- this maps a SkillTrait to
// the matching gzIMPSkillTraitsText index. THIEF has no equivalent there (never offered during
// IMP creation) and NO_SKILLTRAIT isn't a real skill, so neither is offered as a filter.
static INT8 SkillToImpSkillTextIndex(SkillTrait const skill)
{
	switch (skill)
	{
		case LOCKPICKING: return 0;
		case HANDTOHAND:  return 1;
		case ELECTRONICS: return 2;
		case NIGHTOPS:    return 3;
		case THROWING:    return 4;
		case TEACHING:    return 5;
		case HEAVY_WEAPS: return 6;
		case AUTO_WEAPS:  return 7;
		case STEALTHY:    return 8;
		case AMBIDEXT:    return 9;
		case KNIFING:     return 10;
		case ONROOF:      return 11;
		case CAMOUFLAGED: return 12;
		case MARTIALARTS: return 13;
		default:          SLOGA("SkillToImpSkillTextIndex: not a filterable skill"); return 14;
	}
}


struct AimSkillFilterInfo
{
	SkillTrait   skill;
	ST::string   text; // gzIMPSkillTraitsText[], resolved once so the list can be sorted by it
	MOUSE_REGION region;
};

// The up to 2 skills currently selected as filters; NO_SKILLTRAIT means the slot is unset.
static SkillTrait gbAimSkillFilter[2] = { NO_SKILLTRAIT, NO_SKILLTRAIT };

static AimSkillFilterInfo g_aim_skill_filter[NUM_AIM_SKILL_FILTERS] =
{
	{ LOCKPICKING }, { HANDTOHAND }, { ELECTRONICS }, { NIGHTOPS }, { THROWING }, { TEACHING },
	{ HEAVY_WEAPS }, { AUTO_WEAPS }, { STEALTHY },     { AMBIDEXT }, { MARTIALARTS }, { KNIFING },
	{ ONROOF },      { CAMOUFLAGED },
};


// Resolves each filter's display text and sorts the list alphabetically by it (point 7): the
// display order depends on the current language, so this is (re)done on every entry to the page
// rather than hardcoded.
static void SortAimSkillFilterList()
{
	for (AimSkillFilterInfo& i : g_aim_skill_filter)
	{
		i.text = gzIMPSkillTraitsText[SkillToImpSkillTextIndex(i.skill)];
	}
	std::sort(std::begin(g_aim_skill_filter), std::end(g_aim_skill_filter),
		[](AimSkillFilterInfo const& a, AimSkillFilterInfo const& b) { return a.text.compare_i(b.text) < 0; });
}


BOOLEAN MercMatchesAimSkillFilter(ProfileID const id)
{
	if (gbAimSkillFilter[0] == NO_SKILLTRAIT) return TRUE;

	MERCPROFILESTRUCT const& p = GetProfile(id);
	auto const HasSkill = [&](SkillTrait const skill) { return p.bSkillTrait == skill || p.bSkillTrait2 == skill; };

	if (gbAimSkillFilter[1] == NO_SKILLTRAIT) return HasSkill(gbAimSkillFilter[0]);

	return HasSkill(gbAimSkillFilter[0]) && HasSkill(gbAimSkillFilter[1]);
}


// Indexed by sort mode (see AIMSort.h and str_aim_sort_list), so the order is
// fixed; x/y are the positions of the check boxes in SORTBY_LONG.STI, in four
// columns of up to four rows (Name has the extra first row in column 1).
#define AIM_SORT_ROW_0 21
#define AIM_SORT_ROW_1 34
#define AIM_SORT_ROW_2 47
#define AIM_SORT_ROW_3 60
#define AIM_SORT_COLUMN_0 9
#define AIM_SORT_COLUMN_1 114
#define AIM_SORT_COLUMN_2 219
#define AIM_SORT_COLUMN_3 324

static AIMSortInfo g_aim_sort_info[L10n::str_aim_sort_list_SIZE]
{
	// laid out column by column in the order the stats appear on a merc's file page:
	// Name, Price, Health, Agility, Dexterity, Strength, Leadership, Wisdom,
	// Experience, Marksmanship, Mechanical, Explosives, Medical
	{ AIM_SORT_COLUMN_0, AIM_SORT_ROW_1, LEFT_JUSTIFIED,   0, SelectSortCriterionRegionCallBack }, // Price
	{ AIM_SORT_COLUMN_2, AIM_SORT_ROW_2, LEFT_JUSTIFIED,   1, SelectSortCriterionRegionCallBack }, // Experience
	{ AIM_SORT_COLUMN_2, AIM_SORT_ROW_3, LEFT_JUSTIFIED,   2, SelectSortCriterionRegionCallBack }, // Marksmanship
	{ AIM_SORT_COLUMN_3, AIM_SORT_ROW_3, LEFT_JUSTIFIED,   3, SelectSortCriterionRegionCallBack }, // Medical
	{ AIM_SORT_COLUMN_3, AIM_SORT_ROW_2, LEFT_JUSTIFIED,   4, SelectSortCriterionRegionCallBack }, // Explosives
	{ AIM_SORT_COLUMN_3, AIM_SORT_ROW_1, LEFT_JUSTIFIED,   5, SelectSortCriterionRegionCallBack }, // Mechanical
	{ AIM_SORT_COLUMN_0, AIM_SORT_ROW_0, LEFT_JUSTIFIED,   6, SelectSortCriterionRegionCallBack }, // Name (nickname)
	{ AIM_SORT_COLUMN_0, AIM_SORT_ROW_2, LEFT_JUSTIFIED,   7, SelectSortCriterionRegionCallBack }, // Health
	{ AIM_SORT_COLUMN_0, AIM_SORT_ROW_3, LEFT_JUSTIFIED,   8, SelectSortCriterionRegionCallBack }, // Agility
	{ AIM_SORT_COLUMN_1, AIM_SORT_ROW_1, LEFT_JUSTIFIED,   9, SelectSortCriterionRegionCallBack }, // Dexterity
	{ AIM_SORT_COLUMN_1, AIM_SORT_ROW_2, LEFT_JUSTIFIED,  10, SelectSortCriterionRegionCallBack }, // Strength
	{ AIM_SORT_COLUMN_1, AIM_SORT_ROW_3, LEFT_JUSTIFIED,  11, SelectSortCriterionRegionCallBack }, // Leadership
	{ AIM_SORT_COLUMN_2, AIM_SORT_ROW_1, LEFT_JUSTIFIED,  12, SelectSortCriterionRegionCallBack }, // Wisdom
	{ 413,               5,              RIGHT_JUSTIFIED, 13, SelectAscendBoxRegionCallBack     },
	{ 413,               18,             RIGHT_JUSTIFIED, 14, SelectDescendBoxRegionCallBack    }
};

UINT8			gubCurrentSortMode;
UINT8			gubCurrentListMode;

// Mouse stuff
//Clicking on To Mugshot
static MOUSE_REGION gSelectedToMugShotRegion;

//Clicking on ToStats
static MOUSE_REGION gSelectedToStatsRegion;

//Clicking on ToStats
static MOUSE_REGION gSelectedToArchiveRegion;


static SGPVObject* guiSortByBox;
static SGPVObject* guiSortBySkillsBox;
static SGPVObject* guiToAlumni;
static SGPVObject* guiToMugShots;
static SGPVObject* guiToStats;
static SGPVObject* guiSelectLight;


void GameInitAimSort()
{
	ResetAimFilterForNewGame();
	gubCurrentSortMode=AIM_SORT_NAME;
	gubCurrentListMode=AIM_ASCEND;
	gbAimSkillFilter[0] = NO_SKILLTRAIT;
	gbAimSkillFilter[1] = NO_SKILLTRAIT;
}


static void SelectToArchiveRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectToMugShotRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectToStatsRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);


void EnterAimSort()
{
	//Everytime into Aim Sort, reset array.
	ResetAimMercArray();

	SetAimSmallLogo(true);
	InitAimDefaults();

	// load the SortBy box graphic and add it
	guiSortByBox = AddVideoObjectFromFile(LAPTOPDIR "/sortby_long.sti");

	// load the "filter by skill" box graphic and add it
	guiSortBySkillsBox = AddVideoObjectFromFile(LAPTOPDIR "/SORTBY_LONG_SKILLS.STI");

	// load the ToAlumni graphic and add it
	guiToAlumni = AddVideoObjectFromFile(MLG_TOALUMNI);

	// load the ToMugShots graphic and add it
	guiToMugShots = AddVideoObjectFromFile(MLG_TOMUGSHOTS);

	// load the ToStats graphic and add it
	guiToStats = AddVideoObjectFromFile(MLG_TOSTATS);

	// load the SelectLight graphic and add it
	guiSelectLight = AddVideoObjectFromFile(LAPTOPDIR "/selectlight.sti");


	//** Mouse Regions **

	//Mouse region for the ToMugShotRegion
	MSYS_DefineRegion(&gSelectedToMugShotRegion, AIM_SORT_TO_MUGSHOTS_X, AIM_SORT_TO_MUGSHOTS_Y,
				(AIM_SORT_TO_MUGSHOTS_X + AIM_SORT_TO_MUGSHOTS_SIZE),
				(AIM_SORT_TO_MUGSHOTS_Y + AIM_SORT_TO_MUGSHOTS_SIZE), MSYS_PRIORITY_HIGH,
				CURSOR_WWW, MSYS_NO_CALLBACK, SelectToMugShotRegionCallBack );

	//Mouse region for the ToStatsRegion
	MSYS_DefineRegion(&gSelectedToStatsRegion, AIM_SORT_TO_STATS_X, AIM_SORT_TO_STATS_Y,
				(AIM_SORT_TO_STATS_X + AIM_SORT_TO_STATS_SIZE),
				(AIM_SORT_TO_STATS_Y + AIM_SORT_TO_STATS_SIZE), MSYS_PRIORITY_HIGH,
				CURSOR_WWW, MSYS_NO_CALLBACK, SelectToStatsRegionCallBack );

	//Mouse region for the ToArhciveRegion
	MSYS_DefineRegion(&gSelectedToArchiveRegion, AIM_SORT_TO_ALUMNI_X, AIM_SORT_TO_ALUMNI_Y,
				(AIM_SORT_TO_ALUMNI_X + AIM_SORT_TO_ALUMNI_SIZE),
				(AIM_SORT_TO_ALUMNI_Y + AIM_SORT_TO_ALUMNI_SIZE), MSYS_PRIORITY_HIGH,
				CURSOR_WWW, MSYS_NO_CALLBACK, SelectToArchiveRegionCallBack );

	FOR_EACH(AIMSortInfo, i, g_aim_sort_info)
	{
		const UINT16 txt_w = StringPixLength(str_aim_sort_list[i->index], AIM_SORT_FONT_SORT_TEXT);
		const UINT16 x = AIM_SORT_SORT_BY_X + i->x - (i->align == LEFT_JUSTIFIED ? 0 : 4 + txt_w);
		const UINT16 w = AIM_SORT_CHECKBOX_SIZE + 4 + txt_w;
		const UINT16 y = AIM_SORT_SORT_BY_Y + i->y;
		const UINT16 h = AIM_SORT_CHECKBOX_SIZE;
		MSYS_DefineRegion(&i->region, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, i->click);
		MSYS_SetRegionUserData(&i->region, 0, i->index);
	}

	// the display order depends on the current language, so it's (re)computed every time
	SortAimSkillFilterList();
	UINT16 skillRow = 0;
	FOR_EACHX(AimSkillFilterInfo, i, g_aim_skill_filter, ++skillRow)
	{
		const UINT16 x = AIM_SORT_SKILLS_X + AIM_SORT_SKILLS_CHECKBOX_X;
		const UINT16 y = AIM_SORT_SKILLS_Y + AIM_SORT_SKILLS_FIRST_ROW_Y + skillRow * AIM_SORT_SKILLS_ROW_HEIGHT;
		const UINT16 w = AIM_SORT_SKILLS_TEXT_X - AIM_SORT_SKILLS_CHECKBOX_X + StringPixLength(i->text, AIM_SORT_FONT_SORT_TEXT);
		const UINT16 h = AIM_SORT_CHECKBOX_SIZE;
		MSYS_DefineRegion(&i->region, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, SelectSkillFilterRegionCallBack);
		MSYS_SetRegionUserData(&i->region, 0, skillRow);
	}

	InitAimMenuBar();
	RenderAimSort();
}


void ExitAimSort()
{
	// Sort the merc array
	SortAimMercArray();
	RemoveAimDefaults();
	SetAimSmallLogo(false);

	DeleteVideoObject(guiSortByBox);
	DeleteVideoObject(guiSortBySkillsBox);
	DeleteVideoObject(guiToAlumni);
	DeleteVideoObject(guiToMugShots);
	DeleteVideoObject(guiToStats);
	DeleteVideoObject(guiSelectLight);

	MSYS_RemoveRegion( &gSelectedToMugShotRegion);
	MSYS_RemoveRegion( &gSelectedToStatsRegion);
	MSYS_RemoveRegion( &gSelectedToArchiveRegion);

	FOR_EACH(AIMSortInfo, i, g_aim_sort_info)
	{
		MSYS_RemoveRegion(&i->region);
	}

	FOR_EACH(AimSkillFilterInfo, i, g_aim_skill_filter)
	{
		MSYS_RemoveRegion(&i->region);
	}

	ExitAimMenuBar();

}


static void DrawSelectLight(UINT8 ubMode, UINT8 ubImage);


void RenderAimSort()
{
	DrawAimDefaults();
	BltVideoObject(FRAME_BUFFER, guiSortByBox,       0, AIM_SORT_SORT_BY_X, AIM_SORT_SORT_BY_Y);
	BltVideoObject(FRAME_BUFFER, guiSortBySkillsBox, 0, AIM_SORT_SKILLS_X,  AIM_SORT_SKILLS_Y);
	BltVideoObject(FRAME_BUFFER, guiToMugShots, 0, AIM_SORT_TO_MUGSHOTS_X, AIM_SORT_TO_MUGSHOTS_Y);
	BltVideoObject(FRAME_BUFFER, guiToStats,    0, AIM_SORT_TO_STATS_X,    AIM_SORT_TO_STATS_Y);
	BltVideoObject(FRAME_BUFFER, guiToAlumni,   0, AIM_SORT_TO_ALUMNI_X,   AIM_SORT_TO_ALUMNI_Y);

	// Draw the aim slogan under the symbol
	DisplayAimSlogan();

	//Display AIM Member text
	DrawTextToScreen(AimSortText[AIM_AIMMEMBERS], AIM_SORT_AIM_MEMBER_X, AIM_SORT_AIM_MEMBER_Y, AIM_SORT_AIM_MEMBER_WIDTH, AIM_MAINTITLE_FONT, AIM_MAINTITLE_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

	//Display sort title
	DrawTextToScreen(AimSortText[SORT_BY], AIM_SORT_SORT_BY_TEXT_X, AIM_SORT_SORT_BY_TEXT_Y, 0, AIM_SORT_FONT_TITLE, AIM_SORT_SORT_BY_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	// Display all the sort by text
	FOR_EACH(AIMSortInfo const, i, g_aim_sort_info)
	{
		const UINT16 x = AIM_SORT_SORT_BY_X + i->x + (i->align == LEFT_JUSTIFIED ? 14 : -AIM_SORT_ASC_DESC_WIDTH - 4);
		DrawTextToScreen(str_aim_sort_list[i->index], x, AIM_SORT_SORT_BY_Y + i->y + 2, AIM_SORT_ASC_DESC_WIDTH, AIM_SORT_FONT_SORT_TEXT, AIM_SORT_COLOR_SORT_TEXT, FONT_MCOLOR_BLACK, i->align);
	}

	// Display text for the 3 icons
	DrawTextToScreen(AimSortText[MUGSHOT_INDEX],   AIM_SORT_MUGSHOT_TEXT_X,    AIM_SORT_MUGSHOT_TEXT_Y,    0, AIM_SORT_FONT_SORT_TEXT, AIM_SORT_LINK_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	DrawTextToScreen(AimSortText[MERCENARY_FILES], AIM_SORT_MERC_STATS_TEXT_X, AIM_SORT_MERC_STATS_TEXT_Y, 0, AIM_SORT_FONT_SORT_TEXT, AIM_SORT_LINK_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	DrawTextToScreen(AimSortText[ALUMNI_GALLERY],  AIM_SORT_ALUMNI_TEXT_X,     AIM_SORT_ALUMNI_TEXT_Y,     0, AIM_SORT_FONT_SORT_TEXT, AIM_SORT_LINK_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	DrawSelectLight(gubCurrentSortMode, AIM_SORT_ON);
	DrawSelectLight(gubCurrentListMode, AIM_SORT_ON);

	// Display the skill filter checkboxes and their (alphabetically sorted) text
	UINT16 skillRow = 0;
	FOR_EACHX(AimSkillFilterInfo const, i, g_aim_skill_filter, ++skillRow)
	{
		const UINT16 x = AIM_SORT_SKILLS_X + AIM_SORT_SKILLS_CHECKBOX_X;
		const UINT16 rowY = AIM_SORT_SKILLS_Y + AIM_SORT_SKILLS_CHECKBOX_Y + skillRow * AIM_SORT_SKILLS_ROW_HEIGHT;
		const BOOLEAN selected = (i->skill == gbAimSkillFilter[0] || i->skill == gbAimSkillFilter[1]);
		BltVideoObject(FRAME_BUFFER, guiSelectLight, selected ? AIM_SORT_ON : AIM_SORT_OFF, x, rowY);
		DrawTextToScreen(i->text, AIM_SORT_SKILLS_X + AIM_SORT_SKILLS_TEXT_X, rowY + 2, AIM_SORT_SKILLS_TEXT_WIDTH, AIM_SORT_FONT_SORT_TEXT, AIM_SORT_COLOR_SORT_TEXT, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}

	DisableAimButton();

	MarkButtonsDirty( );

	RenderWWWProgramTitleBar( );

	InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
}


static void SelectToMugShotRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS_FACIAL_INDEX;
	}
}


static void SelectToStatsRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS;
	}
}


static void SelectToArchiveRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS_ARCHIVES;
	}
}


static void SetSortCriterion(const UINT8 criterion)
{
	if (gubCurrentSortMode == criterion) return;
	DrawSelectLight(gubCurrentSortMode, AIM_SORT_OFF);
	gubCurrentSortMode = criterion;
	DrawSelectLight(criterion, AIM_SORT_ON);
}


// One callback for all the sort criteria; the region's user data is the criterion
static void SelectSortCriterionRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP) SetSortCriterion(MSYS_GetRegionUserData(pRegion, 0));
}


static void SetSortOrder(const UINT8 order)
{
	if (gubCurrentListMode == order) return;
	DrawSelectLight(gubCurrentListMode, AIM_SORT_OFF);
	gubCurrentListMode = order;
	DrawSelectLight(order, AIM_SORT_ON);
}


static void SelectAscendBoxRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP) SetSortOrder(AIM_ASCEND);
}


static void SelectDescendBoxRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP) SetSortOrder(AIM_DESCEND);
}


// Toggles a skill filter on/off. Up to 2 can be active; picking a 3rd while 2 are already set
// drops the oldest one, the same "last two win" rule IMP character creation uses for skills
// (see IMP_SkillTraits.cc's HandleLastSelectedTraits).
static void ToggleAimSkillFilter(SkillTrait const skill)
{
	if (gbAimSkillFilter[0] == skill)
	{
		gbAimSkillFilter[0] = gbAimSkillFilter[1];
		gbAimSkillFilter[1] = NO_SKILLTRAIT;
	}
	else if (gbAimSkillFilter[1] == skill)
	{
		gbAimSkillFilter[1] = NO_SKILLTRAIT;
	}
	else if (gbAimSkillFilter[0] == NO_SKILLTRAIT)
	{
		gbAimSkillFilter[0] = skill;
	}
	else if (gbAimSkillFilter[1] == NO_SKILLTRAIT)
	{
		gbAimSkillFilter[1] = skill;
	}
	else
	{
		gbAimSkillFilter[0] = gbAimSkillFilter[1];
		gbAimSkillFilter[1] = skill;
	}

	ResetAimMercArray();
	SortAimMercArray();
	RenderAimSort();
}


static void SelectSkillFilterRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		ToggleAimSkillFilter(g_aim_skill_filter[MSYS_GetRegionUserData(pRegion, 0)].skill);
	}
}


static void DrawSelectLight(const UINT8 ubMode, const UINT8 ubImage)
{
	const AIMSortInfo* const asi = &g_aim_sort_info[ubMode];
	const INT32 x = AIM_SORT_SORT_BY_X + asi->x;
	const INT32 y = AIM_SORT_SORT_BY_Y + asi->y;
	BltVideoObject(FRAME_BUFFER, guiSelectLight, ubImage, x, y);
	InvalidateRegion(x, y, x + AIM_SORT_CHECKBOX_SIZE, y + AIM_SORT_CHECKBOX_SIZE);
}


static INT32 QsortCompare(const void* pNum1, const void* pNum2);


void SortAimMercArray(void)
{
	qsort(AimMercArray, gubNumAimMercs, sizeof(UINT8), QsortCompare);
}


static INT32 QsortCompare(const void* pNum1, const void* pNum2)
{
	MERCPROFILESTRUCT const& p1 = GetProfile(*(UINT8*)pNum1);
	MERCPROFILESTRUCT const& p2 = GetProfile(*(UINT8*)pNum2);

	INT32 ret;
	if (gubCurrentSortMode == AIM_SORT_NAME)
	{
		// by nickname, alphabetically, ignoring case
		ret = p1.zNickname.compare_i(p2.zNickname);
		ret = (ret > 0) - (ret < 0);
	}
	else
	{
		INT32 v1;
		INT32 v2;
		switch (gubCurrentSortMode)
		{
			/* Price        */ case 0:  v1 = p1.uiWeeklySalary; v2 = p2.uiWeeklySalary; break;
			/* Experience   */ case 1:  v1 = p1.bExpLevel;      v2 = p2.bExpLevel;      break;
			/* Marksmanship */ case 2:  v1 = p1.bMarksmanship;  v2 = p2.bMarksmanship;  break;
			/* Medical      */ case 3:  v1 = p1.bMedical;       v2 = p2.bMedical;       break;
			/* Explosives   */ case 4:  v1 = p1.bExplosive;     v2 = p2.bExplosive;     break;
			/* Mechanical   */ case 5:  v1 = p1.bMechanical;    v2 = p2.bMechanical;    break;
			/* Health       */ case 7:  v1 = p1.bLifeMax;       v2 = p2.bLifeMax;       break;
			/* Agility      */ case 8:  v1 = p1.bAgility;       v2 = p2.bAgility;       break;
			/* Dexterity    */ case 9:  v1 = p1.bDexterity;     v2 = p2.bDexterity;     break;
			/* Strength     */ case 10: v1 = p1.bStrength;      v2 = p2.bStrength;      break;
			/* Leadership   */ case 11: v1 = p1.bLeadership;    v2 = p2.bLeadership;    break;
			/* Wisdom       */ case 12: v1 = p1.bWisdom;        v2 = p2.bWisdom;        break;

			default: SLOGA("QsortCompare: invalid sort mode"); return 0;
		}
		ret = (v1 > v2) - (v1 < v2);
	}
	return gubCurrentListMode == AIM_ASCEND ? ret : -ret;
}
