#include "Mercs_Files.h"
#include "AIMMembers.h"
#include "Assignments.h"
#include "Button_System.h"
#include "Cheats.h"
#include "ContentManager.h"
#include "Cursors.h"
#include "Directories.h"
#include "EDT.h"
#include "English.h"
#include "Facts.h"
#include "Font.h"
#include "Font_Control.h"
#include "GameInstance.h"
#include "GameRes.h"
#include "HImage.h"
#include "Input.h"
#include "JA2Types.h"
#include "Laptop.h"
#include "LaptopSave.h"
#include "MERCListingModel.h"
#include "MercProfile.h"
#include "MercProfileInfo.h"
#include "MercPortrait.h"
#include "Merc_Hiring.h"
#include "MercPortrait.h"
#include "Mercs.h"
#include "Object_Cache.h"
#include "Quests.h"
#include "ScreenIDs.h"
#include "Soldier_Control.h"
#include "Soldier_Profile.h"
#include "Soldier_Profile_Type.h"
#include "Speck_Quotes.h"
#include "Text.h"
#include "Types.h"
#include "UILayout.h"
#include "Video.h"
#include "VObject.h"
#include "VSurface.h"
#include "WordWrap.h"
#include <string_theory/format>
#include <string_theory/string>


#define MERC_BIO_FONT			FONT14ARIAL//FONT12ARIAL
#define MERC_BIO_COLOR			FONT_MCOLOR_WHITE

#define MERC_TITLE_FONT			FONT14ARIAL
#define MERC_TITLE_COLOR		146

#define MERC_NAME_FONT			FONT14ARIAL
#define MERC_NAME_COLOR			FONT_MCOLOR_WHITE

#define MERC_STATS_FONT			FONT12ARIAL
#define MERC_STATIC_STATS_COLOR		146
#define MERC_DYNAMIC_STATS_COLOR	FONT_MCOLOR_WHITE


#define MERC_FILES_PORTRAIT_BOX_X	LAPTOP_SCREEN_UL_X + 16
#define MERC_FILES_PORTRAIT_BOX_Y	LAPTOP_SCREEN_WEB_UL_Y + 17

#define MERC_FACE_X			MERC_FILES_PORTRAIT_BOX_X + 2
#define MERC_FACE_Y			MERC_FILES_PORTRAIT_BOX_Y + 2
#define MERC_FACE_WIDTH			106
#define MERC_FACE_HEIGHT		122

#define MERC_FILES_STATS_BOX_X		LAPTOP_SCREEN_UL_X + 164
#define MERC_FILES_STATS_BOX_Y		MERC_FILES_PORTRAIT_BOX_Y


#define MERC_FILES_BIO_BOX_X		MERC_FILES_PORTRAIT_BOX_X
#define MERC_FILES_BIO_BOX_Y		LAPTOP_SCREEN_WEB_UL_Y + 155

#define MERC_FILES_PREV_BUTTON_X	(STD_SCREEN_X + 128)
#define MERC_FILES_NEXT_BUTTON_X	(STD_SCREEN_X + 490)
#define MERC_FILES_HIRE_BUTTON_X	(STD_SCREEN_X + 260)
#define MERC_FILES_BACK_BUTTON_X	(STD_SCREEN_X + 380)
#define MERC_FILES_BUTTON_Y		(STD_SCREEN_Y + 380 + 18)

#define MERC_NAME_X			MERC_FILES_STATS_BOX_X + 50
#define MERC_NAME_Y			MERC_FILES_STATS_BOX_Y + 10

#define MERC_BIO_TEXT_X			MERC_FILES_BIO_BOX_X + 5
#define MERC_BIO_TEXT_Y			MERC_FILES_BIO_BOX_Y + 10 - 10

#define MERC_ADD_BIO_TITLE_X		MERC_BIO_TEXT_X
#define MERC_ADD_BIO_TITLE_Y		MERC_BIO_TEXT_Y + 100 - 9

#define MERC_ADD_BIO_TEXT_X		MERC_BIO_TEXT_X
#define MERC_ADD_BIO_TEXT_Y		MERC_ADD_BIO_TITLE_Y + 20

#define MERC_BIO_WIDTH			460 - 10

#define MERC_STATS_FIRST_COL_X		MERC_NAME_X
#define MERC_STATS_FIRST_NUM_COL_X	MERC_STATS_FIRST_COL_X + 90
#define MERC_STATS_SECOND_COL_X		MERC_FILES_STATS_BOX_X + 170
#define MERC_STATS_SECOND_NUM_COL_X	MERC_STATS_SECOND_COL_X + 115
#define MERC_SPACE_BN_LINES		15

#define MERC_HEALTH_Y			MERC_FILES_STATS_BOX_Y + 30

#define MERC_PORTRAIT_TEXT_OFFSET_Y	110

// "Unavailable" is moved to the top of the portrait, and a "Click hire / to leave a message"
// hint is added at the bottom, in the same font/size/centering (only for the "merc is simply
// away" case -- dead/POW/already-hired mercs keep the old single centered line).
#define MERC_UNAVAILABLE_TOP_TEXT_Y_OFFSET	4
#define MERC_UNAVAILABLE_BOTTOM_TEXT_Y_OFFSET	91
#define MERC_UNAVAILABLE_TEXT_LINE_HEIGHT	16

// The "unavailable merc" video-conferencing box: opened from the Hire button when the merc is
// simply away on assignment (not dead, not already hired). Lets the player leave a message and
// get an e-mail once the merc becomes available -- like A.I.M.'s answering machine, but without
// any animation or talking, just the merc's portrait and name.
#define MERC_UNAVAIL_BOX_X			(STD_SCREEN_X + 125)
#define MERC_UNAVAIL_BOX_Y			(STD_SCREEN_Y + 97 + LAPTOP_SCREEN_WEB_DELTA_Y)
#define MERC_UNAVAIL_BOX_TITLE_HEIGHT		20

#define MERC_UNAVAIL_BOX_NAME_X			(MERC_UNAVAIL_BOX_X + 7)
#define MERC_UNAVAIL_BOX_NAME_Y			(MERC_UNAVAIL_BOX_Y + 5 + 1)

#define MERC_UNAVAIL_BOX_FACE_X			(MERC_UNAVAIL_BOX_X + 8 - 4)
#define MERC_UNAVAIL_BOX_FACE_Y			(MERC_UNAVAIL_BOX_Y + MERC_UNAVAIL_BOX_TITLE_HEIGHT + 4 - 1)

#define MERC_UNAVAIL_BOX_BUTTON_X		(MERC_UNAVAIL_BOX_X + 134)
#define MERC_UNAVAIL_BOX_BUTTON_Y1		(MERC_UNAVAIL_BOX_Y + MERC_UNAVAIL_BOX_TITLE_HEIGHT + 40)
#define MERC_UNAVAIL_BOX_BUTTON_Y2		(MERC_UNAVAIL_BOX_BUTTON_Y1 + 30)

#define MERC_UNAVAIL_BOX_LEAVE_MSG_X		(MERC_UNAVAIL_BOX_BUTTON_X - 18)
#define MERC_UNAVAIL_BOX_LEAVE_MSG_Y		(MERC_UNAVAIL_BOX_BUTTON_Y1 + 7)

#define MERC_UNAVAIL_BOX_XCLOSE_X		(MERC_UNAVAIL_BOX_X + 348)
#define MERC_UNAVAIL_BOX_XCLOSE_Y		(MERC_UNAVAIL_BOX_Y + 3)

#define MERC_UNAVAIL_BOX_HANG_UP_X		(MERC_UNAVAIL_BOX_BUTTON_X + 108)
#define MERC_UNAVAIL_BOX_HANG_UP_Y		(MERC_UNAVAIL_BOX_BUTTON_Y2 - 24 + 1)

// skill / attitude / disability boxes (2x2 grid, MERC_BOX_SKILLS.STI, 132x22) -- same rules as
// the AIM Members CONTENTBUTTON_SKILLS.STI buttons: top-left = first skill (or "X (expert)"),
// bottom-left = second distinct skill, both invisible if the merc has no skill(s); top-right =
// Attitude; bottom-right = Disabilities.
#define MERC_SKILL_BOX_WIDTH		132
#define MERC_SKILL_BOX_HEIGHT		22
#define MERC_SKILL_BOX_TEXT_Y_OFFSET	6

#define MERC_SKILL_BOX_LEFT_X		(STD_SCREEN_X + 128)
#define MERC_SKILL_BOX_RIGHT_X		(STD_SCREEN_X + 380)
#define MERC_SKILL_BOX_TOP_Y		(STD_SCREEN_Y + 350)
#define MERC_SKILL_BOX_BOTTOM_Y		(STD_SCREEN_Y + 374)

#define MERC_SKILL_BOX_FONT		FONT12ARIAL
#define MERC_SKILL_BOX_COLOR		FONT_MCOLOR_WHITE

// pImpButtonText[] offsets for the attitude/disability display strings -- must match
// IMP_ATT_TXT_FIRST (IMP_Attitude.cc) and IMP_DIS_TXT_FIRST (IMP_Disability.cc).
#define MERC_ATT_TXT_FIRST		40
#define MERC_DIS_TXT_FIRST		51

#define MERC_ATT_DIS_TEXT_X_OFFSET	7


namespace {
constexpr MultiLanguageGraphic guiStatsBox{ MLG_STATSBOX };
cache_key_t const guiBioBox{ LAPTOPDIR "/biobox.sti" };
cache_key_t const guiPortraitBox{ LAPTOPDIR "/portraitbox.sti" };
cache_key_t const guiMercUnavailableBox{ LAPTOPDIR "/videoconfterminal.sti" };
cache_key_t const guiMercSkillsBox{ LAPTOPDIR "/MERC_BOX_SKILLS.STI" };
}

//
// Buttons
//

// The Prev button
static void BtnMercPrevButtonCallback(GUI_BUTTON *btn, UINT32 reason);
static BUTTON_PICS* guiButtonImage;
GUIButtonRef guiPrevButton;

// The Next button
static void BtnMercNextButtonCallback(GUI_BUTTON *btn, UINT32 reason);
GUIButtonRef guiNextButton;

// The Hire button
static void BtnMercHireButtonCallback(GUI_BUTTON *btn, UINT32 reason);
GUIButtonRef guiHireButton;

// The Back button
static void BtnMercFilesBackButtonCallback(GUI_BUTTON *btn, UINT32 reason);
GUIButtonRef guiMercBackButton;

// The "unavailable merc" box's Leave Message / Hang Up / X-to-close buttons -- created only
// while the box is open (see OpenMercUnavailableBox()/CloseMercUnavailableBox())
static void BtnMercUnavailableLeaveMessageCallback(GUI_BUTTON *btn, UINT32 reason);
static void BtnMercUnavailableHangUpCallback(GUI_BUTTON *btn, UINT32 reason);
static BUTTON_PICS* guiMercUnavailableBoxButtonImage;
static GUIButtonRef guiLeaveMessageButton;
static GUIButtonRef guiHangUpBoxButton;

static BUTTON_PICS* guiMercUnavailableBoxCloseButtonImage;
static GUIButtonRef guiMercUnavailableBoxCloseButton;

static BOOLEAN gfMercUnavailableBoxActive;
static ProfileID gubMercUnavailableBoxMercID;


static GUIButtonRef MakeButton(const ST::string& text, INT16 x, GUI_CALLBACK click)
{
	const INT16 shadow_col = DEFAULT_SHADOW;
	GUIButtonRef const btn = CreateIconAndTextButton(guiButtonImage, text, FONT12ARIAL, MERC_BUTTON_UP_COLOR, shadow_col, MERC_BUTTON_DOWN_COLOR, shadow_col, x, MERC_FILES_BUTTON_Y, MSYS_PRIORITY_HIGH, click);
	btn->SetCursor(CURSOR_LAPTOP_SCREEN);
	btn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_SHADED);
	return btn;
}


static GUIButtonRef MakeUnavailableBoxButton(const ST::string& text, INT16 x, INT16 y, GUI_CALLBACK click)
{
	const INT16 shadow_col = DEFAULT_SHADOW;
	GUIButtonRef const btn = CreateIconAndTextButton(guiMercUnavailableBoxButtonImage, text, FONT12ARIAL, MERC_BUTTON_UP_COLOR, shadow_col, MERC_BUTTON_DOWN_COLOR, shadow_col, x, y, MSYS_PRIORITY_HIGH, click);
	btn->SetCursor(CURSOR_LAPTOP_SCREEN);
	btn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_SHADED);
	return btn;
}


void EnterMercsFiles()
{
	InitMercBackGround();

	guiButtonImage    = LoadButtonImage(LAPTOPDIR "/bigbuttons.sti", 0, 1);
	guiPrevButton     = MakeButton(MercInfo[MERC_FILES_PREVIOUS], MERC_FILES_PREV_BUTTON_X, BtnMercPrevButtonCallback);
	guiNextButton     = MakeButton(MercInfo[MERC_FILES_NEXT],     MERC_FILES_NEXT_BUTTON_X, BtnMercNextButtonCallback);
	guiHireButton     = MakeButton(MercInfo[MERC_FILES_HIRE],     MERC_FILES_HIRE_BUTTON_X, BtnMercHireButtonCallback);
	guiMercBackButton = MakeButton(MercInfo[MERC_FILES_HOME],     MERC_FILES_BACK_BUTTON_X, BtnMercFilesBackButtonCallback);

	// same button graphic A.I.M.'s answering machine uses (frames 2/3 = up/down)
	guiMercUnavailableBoxButtonImage      = LoadButtonImage(LAPTOPDIR "/videoconfbuttons.sti", 2, 3);
	guiMercUnavailableBoxCloseButtonImage = LoadButtonImage(LAPTOPDIR "/x_button.sti", 0, 1);
	gfMercUnavailableBoxActive            = FALSE;

	//RenderMercsFiles();
}


void ExitMercsFiles()
{
	RemoveVObject(guiPortraitBox);
	RemoveVObject(guiStatsBox);
	RemoveVObject(guiBioBox);
	RemoveVObject(guiMercUnavailableBox);
	RemoveVObject(guiMercSkillsBox);

	UnloadButtonImage( guiButtonImage );
	RemoveButton( guiPrevButton );
	RemoveButton( guiNextButton );
	RemoveButton( guiHireButton );
	RemoveButton( guiMercBackButton );

	UnloadButtonImage( guiMercUnavailableBoxButtonImage );
	UnloadButtonImage( guiMercUnavailableBoxCloseButtonImage );
	if (gfMercUnavailableBoxActive)
	{
		RemoveButton( guiLeaveMessageButton );
		RemoveButton( guiHangUpBoxButton );
		RemoveButton( guiMercUnavailableBoxCloseButton );
		gfMercUnavailableBoxActive = FALSE;
	}

	RemoveMercBackGround();
}


static void DisplayMercFace(ProfileID);
static void DisplayMercsStats(MERCPROFILESTRUCT const&);
static void EnableDisableMercFilesNextPreviousButton(void);
static void LoadAndDisplayMercBio(MERCListingModel const& listing);
static void DisplayMercUnavailableBox(void);
static void DisplayMercSkillAttitudeDisabilityBoxes(MERCPROFILESTRUCT const&);


// gzIMPSkillTraitsText[]'s entry order follows the IMP skill-selection screen's own layout, not
// the SkillTrait enum (see IMP_SkillTraits.cc's skillTraitsMapping) -- this maps a merc's actual
// SkillTrait to the matching gzIMPSkillTraitsText index. THIEF has no equivalent there (never
// offered during IMP creation, and no merc profile currently uses it), so it falls back to "None".
static const INT8 gbMercSkillTraitToImpSkillText[NUM_SKILLTRAITS] =
{
	/* NO_SKILLTRAIT */ 14,
	/* LOCKPICKING   */ 0,
	/* HANDTOHAND    */ 1,
	/* ELECTRONICS   */ 2,
	/* NIGHTOPS      */ 3,
	/* THROWING      */ 4,
	/* TEACHING      */ 5,
	/* HEAVY_WEAPS   */ 6,
	/* AUTO_WEAPS    */ 7,
	/* STEALTHY      */ 8,
	/* AMBIDEXT      */ 9,
	/* THIEF         */ 14,
	/* MARTIALARTS   */ 13,
	/* KNIFING       */ 10,
	/* ONROOF        */ 11,
	/* CAMOUFLAGED   */ 12,
};


static void DisplayMercSkillAttitudeDisabilityBoxes(MERCPROFILESTRUCT const& p)
{
	INT8 bSkill1 = p.bSkillTrait;
	INT8 bSkill2 = p.bSkillTrait2;
	if (bSkill1 == NO_SKILLTRAIT) { bSkill1 = bSkill2; bSkill2 = NO_SKILLTRAIT; }

	if (bSkill1 != NO_SKILLTRAIT)
	{
		BltVideoObject(FRAME_BUFFER, GetVObject(guiMercSkillsBox), 0, MERC_SKILL_BOX_LEFT_X, MERC_SKILL_BOX_TOP_Y);
		DrawTextToScreen(gzIMPSkillTraitsText[gbMercSkillTraitToImpSkillText[bSkill1]], MERC_SKILL_BOX_LEFT_X, MERC_SKILL_BOX_TOP_Y + MERC_SKILL_BOX_TEXT_Y_OFFSET,
			MERC_SKILL_BOX_WIDTH, MERC_SKILL_BOX_FONT, MERC_SKILL_BOX_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

		if (bSkill1 == bSkill2)
		{
			BltVideoObject(FRAME_BUFFER, GetVObject(guiMercSkillsBox), 0, MERC_SKILL_BOX_LEFT_X, MERC_SKILL_BOX_BOTTOM_Y);
			DrawTextToScreen(gzMercSkillText[NUM_SKILLTRAITS], MERC_SKILL_BOX_LEFT_X, MERC_SKILL_BOX_BOTTOM_Y + MERC_SKILL_BOX_TEXT_Y_OFFSET,
				MERC_SKILL_BOX_WIDTH, MERC_SKILL_BOX_FONT, MERC_SKILL_BOX_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
		}
		else if (bSkill2 != NO_SKILLTRAIT)
		{
			BltVideoObject(FRAME_BUFFER, GetVObject(guiMercSkillsBox), 0, MERC_SKILL_BOX_LEFT_X, MERC_SKILL_BOX_BOTTOM_Y);
			DrawTextToScreen(gzIMPSkillTraitsText[gbMercSkillTraitToImpSkillText[bSkill2]], MERC_SKILL_BOX_LEFT_X, MERC_SKILL_BOX_BOTTOM_Y + MERC_SKILL_BOX_TEXT_Y_OFFSET,
				MERC_SKILL_BOX_WIDTH, MERC_SKILL_BOX_FONT, MERC_SKILL_BOX_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
		}
	}

	BltVideoObject(FRAME_BUFFER, GetVObject(guiMercSkillsBox), 0, MERC_SKILL_BOX_RIGHT_X, MERC_SKILL_BOX_TOP_Y);
	DrawTextToScreen(ST::format("Att: {}", pImpButtonText[MERC_ATT_TXT_FIRST + p.bAttitude]),
		MERC_SKILL_BOX_RIGHT_X + MERC_ATT_DIS_TEXT_X_OFFSET, MERC_SKILL_BOX_TOP_Y + MERC_SKILL_BOX_TEXT_Y_OFFSET,
		0, MERC_SKILL_BOX_FONT, MERC_SKILL_BOX_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	BltVideoObject(FRAME_BUFFER, GetVObject(guiMercSkillsBox), 0, MERC_SKILL_BOX_RIGHT_X, MERC_SKILL_BOX_BOTTOM_Y);
	DrawTextToScreen(ST::format("Dis: {}", pImpButtonText[MERC_DIS_TXT_FIRST + p.bPersonalityTrait]),
		MERC_SKILL_BOX_RIGHT_X + MERC_ATT_DIS_TEXT_X_OFFSET, MERC_SKILL_BOX_BOTTOM_Y + MERC_SKILL_BOX_TEXT_Y_OFFSET,
		0, MERC_SKILL_BOX_FONT, MERC_SKILL_BOX_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
}


void RenderMercsFiles()
{
	DrawMecBackGround();

	BltVideoObject(FRAME_BUFFER, guiPortraitBox, 0, MERC_FILES_PORTRAIT_BOX_X, MERC_FILES_PORTRAIT_BOX_Y);
	BltVideoObject(FRAME_BUFFER, guiStatsBox,    0, MERC_FILES_STATS_BOX_X,    MERC_FILES_STATS_BOX_Y);
	BltVideoObject(FRAME_BUFFER, guiBioBox,      0, MERC_FILES_BIO_BOX_X + 1,  MERC_FILES_BIO_BOX_Y);

	const MERCListingModel*  l   = GCM->getMERCListings().at(gubCurMercIndex);
	ProfileID         const  pid = GetProfileIDFromMERCListing(l);
	MERCPROFILESTRUCT const& p   = GetProfile(pid);

	//Display the mercs face
	DisplayMercFace(pid);

	//Display Mercs Name
	DrawTextToScreen(p.zName, MERC_NAME_X, MERC_NAME_Y, 0, MERC_NAME_FONT, MERC_NAME_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	//Load and display the mercs bio
	LoadAndDisplayMercBio(*l);

	//Display the mercs statistic
	DisplayMercsStats(p);

	DisplayMercSkillAttitudeDisabilityBoxes(p);

	if (gfMercUnavailableBoxActive)
	{
		DisplayMercUnavailableBox();
	}
	else
	{
		bool const enable =
			!IsMercDead(p) &&
			(
				LaptopSaveInfo.gubPlayersMercAccountStatus == MERC_ACCOUNT_VALID     ||
				LaptopSaveInfo.gubPlayersMercAccountStatus == MERC_ACCOUNT_SUSPENDED ||
				LaptopSaveInfo.gubPlayersMercAccountStatus == MERC_ACCOUNT_VALID_FIRST_WARNING
			);
		EnableButton(guiHireButton, enable);

		//Enable or disable the buttons
		EnableDisableMercFilesNextPreviousButton();
	}

	MarkButtonsDirty();
	RenderWWWProgramTitleBar();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}


static void BtnMercPrevButtonCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		if (gubCurMercIndex > 0) gubCurMercIndex--;
		fReDrawScreenFlag = TRUE;
		EnableDisableMercFilesNextPreviousButton();
	}
}


static void BtnMercNextButtonCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		if (gubCurMercIndex <= LaptopSaveInfo.gubLastMercIndex - 1) gubCurMercIndex++;
		fReDrawScreenFlag = TRUE;
		EnableDisableMercFilesNextPreviousButton( );
	}
}


static BOOLEAN MercFilesHireMerc(UINT8 ubMercID);
static void OpenMercUnavailableBox(ProfileID pid);


// Is this merc simply away on assignment (not dead, not a POW, not already hired)? That's the
// only case where clicking Hire should open the "leave a message" box instead of the usual
// hire-attempt/Speck-quote handling -- mirrors the branch order DisplayMercFace() uses to pick
// which status text to show.
static bool IsMercUnavailableForBox(MERCPROFILESTRUCT const& p, SOLDIERTYPE const* const s)
{
	if (IsMercDead(p)) return false;
	if (p.bMercStatus == MERC_FIRED_AS_A_POW || (s && s->bAssignment == ASSIGNMENT_POW)) return false;
	if (p.bMercStatus == MERC_HIRED_BUT_NOT_ARRIVED_YET || p.bMercStatus > 0) return false;
	return !IsMercHireable(p);
}


static void BtnMercHireButtonCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		//if the players accont is suspended, go back to the main screen and have Speck inform the players
		if (LaptopSaveInfo.gubPlayersMercAccountStatus == MERC_ACCOUNT_SUSPENDED)
		{
			guiCurrentLaptopMode = LAPTOP_MODE_MERC;
			gusMercVideoSpeckSpeech = SPECK_QUOTE_ALTERNATE_OPENING_5_PLAYER_OWES_SPECK_ACCOUNT_SUSPENDED;
			gubArrivedFromMercSubSite = MERC_CAME_FROM_HIRE_PAGE;
			return;
		}

		ProfileID          const pid = GetProfileIDFromMERCListingIndex(gubCurMercIndex);
		MERCPROFILESTRUCT const& p   = GetProfile(pid);
		SOLDIERTYPE  const* const s  = FindSoldierByProfileIDOnPlayerTeam(pid);

		if (IsMercUnavailableForBox(p, s))
		{
			OpenMercUnavailableBox(pid);
		}
		else if (MercFilesHireMerc(pid))
		{
			// else try to hire the merc
			guiCurrentLaptopMode = LAPTOP_MODE_MERC;
			gubArrivedFromMercSubSite = MERC_CAME_FROM_HIRE_PAGE;

			gfJustHiredAMercMerc = TRUE;
			DisplayPopUpBoxExplainingMercArrivalLocationAndTime();
		}
	}
}


static void DisplayMercFace(const ProfileID pid)
try
{
	BltVideoObject(FRAME_BUFFER, GetVObject(guiPortraitBox), 0, MERC_FILES_PORTRAIT_BOX_X, MERC_FILES_PORTRAIT_BOX_Y);

	MERCPROFILESTRUCT const&       p = GetProfile(pid);
	SOLDIERTYPE       const* const s = FindSoldierByProfileIDOnPlayerTeam(pid);

	// Load the face graphic
	AutoSGPVObject face(LoadBigPortrait(p));

	BOOLEAN        shaded;
	BOOLEAN        unavailable = FALSE;
	ST::string text;
	if (IsMercDead(p))
	{
		// The merc is dead, shade the face red and put text over top saying the merc is dead
		face->pShades[0] = Create16BPPPaletteShaded(face->Palette(), DEAD_MERC_COLOR_RED, DEAD_MERC_COLOR_GREEN, DEAD_MERC_COLOR_BLUE, TRUE);
		face->CurrentShade(0);
		shaded = FALSE;
		text   = MercInfo[MERC_FILES_MERC_IS_DEAD];
	}
	else if (pid == FLO && gubFact[FACT_PC_MARRYING_DARYL_IS_FLO])
	{
		shaded = TRUE;
		text   = pPersonnelDepartedStateStrings[2];
	}
	else if (p.bMercStatus == MERC_FIRED_AS_A_POW || (s && s->bAssignment == ASSIGNMENT_POW))
	{
		// The merc is currently a POW or the merc was fired as a pow
		shaded = TRUE;
		text   = pPOWStrings[0];
	}
	else if (p.bMercStatus == MERC_HIRED_BUT_NOT_ARRIVED_YET || p.bMercStatus > 0)
	{
		// The merc is hired already
		shaded = TRUE;
		text   = MercInfo[MERC_FILES_ALREADY_HIRED];
	}
	else if (IsMercUnavailableForBox(p, s))
	{
		// The merc is away on another assignemnt, say the merc is unavailable
		shaded      = TRUE;
		unavailable = TRUE;
		text        = MercInfo[MERC_FILES_MERC_UNAVAILABLE];
	}
	else
	{
		shaded = FALSE;
		text.clear();
	}

	BltVideoObject(FRAME_BUFFER, face.get(), 0, MERC_FACE_X, MERC_FACE_Y);

	if (shaded)
	{
		FRAME_BUFFER->ShadowRect(MERC_FACE_X, MERC_FACE_Y, MERC_FACE_X + MERC_FACE_WIDTH, MERC_FACE_Y + MERC_FACE_HEIGHT);
	}

	if (unavailable)
	{
		DrawTextToScreen(text, MERC_FACE_X, MERC_FACE_Y + MERC_UNAVAILABLE_TOP_TEXT_Y_OFFSET, MERC_FACE_WIDTH, FONT14ARIAL, 145, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

		const UINT16 bottomY = MERC_FACE_Y + MERC_UNAVAILABLE_BOTTOM_TEXT_Y_OFFSET;
		DrawTextToScreen(MercInfo[MERC_FILES_CLICK_HIRE_LINE1], MERC_FACE_X, bottomY,                                   MERC_FACE_WIDTH, FONT14ARIAL, 145, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
		DrawTextToScreen(MercInfo[MERC_FILES_CLICK_HIRE_LINE2], MERC_FACE_X, bottomY + MERC_UNAVAILABLE_TEXT_LINE_HEIGHT, MERC_FACE_WIDTH, FONT14ARIAL, 145, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
	}
	else if (!text.empty())
	{
		DisplayWrappedString(MERC_FACE_X, MERC_FACE_Y + MERC_PORTRAIT_TEXT_OFFSET_Y, MERC_FACE_WIDTH, 2, FONT14ARIAL, 145, text, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
	}
}
catch (...) { /* XXX ignore */ }


static void LoadAndDisplayMercBio(MERCListingModel const& listing)
{
	// The descriptions are in the names file of the language (mercs-profile-names-<language>.json),
	// without them in mercbios.edt (one row per merc, the row is the bioIndex of the listing).
	MercProfileInfo const& info = MercProfile(listing.profileID).getInfo();
	bool const fromJson = !info.biography.empty() || !info.additionalInfo.empty();
	EDTFile mercbios{ EDTFile::MERCBIOS };

	{
		//load and display the merc bio
		auto const sText{ fromJson ? info.biography : mercbios.at(listing.bioIndex, 0) };
		DisplayWrappedString(MERC_BIO_TEXT_X, MERC_BIO_TEXT_Y, MERC_BIO_WIDTH, 2, MERC_BIO_FONT, MERC_BIO_COLOR, sText, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}

	{
		//load and display the merc's additioanl info (if any)
		auto const sText{ fromJson ? info.additionalInfo : mercbios.at(listing.bioIndex, 1) };
		if (!sText.empty())
		{
			DrawTextToScreen(MercInfo[MERC_FILES_ADDITIONAL_INFO], MERC_ADD_BIO_TITLE_X, MERC_ADD_BIO_TITLE_Y, 0, MERC_TITLE_FONT, MERC_TITLE_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
			DisplayWrappedString(MERC_ADD_BIO_TEXT_X, MERC_ADD_BIO_TEXT_Y, MERC_BIO_WIDTH, 2, MERC_BIO_FONT, MERC_BIO_COLOR, sText, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		}
	}
}


static void DrawStat(UINT16 x, UINT16 y, const ST::string& stat, UINT16 x_val, INT32 val)
{
	DrawTextToScreen(stat, x, y, 0, MERC_STATS_FONT, MERC_STATIC_STATS_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	DrawNumeralsToScreen(val, 3, x_val, y, MERC_STATS_FONT, MERC_DYNAMIC_STATS_COLOR);
}


static void DisplayMercsStats(MERCPROFILESTRUCT const& p)
{
	const UINT16 x1     = MERC_STATS_FIRST_COL_X;
	const UINT16 x1_val = MERC_STATS_FIRST_NUM_COL_X;
	UINT16       y1     = MERC_HEALTH_Y;
	const UINT16 dy     = MERC_SPACE_BN_LINES;
	DrawStat(x1, y1,       str_stat_health,     x1_val, p.bLife);
	DrawStat(x1, y1 += dy, str_stat_agility,    x1_val, p.bAgility);
	DrawStat(x1, y1 += dy, str_stat_dexterity,  x1_val, p.bDexterity);
	DrawStat(x1, y1 += dy, str_stat_strength,   x1_val, p.bStrength);
	DrawStat(x1, y1 += dy, str_stat_leadership, x1_val, p.bLeadership);
	DrawStat(x1, y1 += dy, str_stat_wisdom,     x1_val, p.bWisdom);

	const UINT16 x2     = MERC_STATS_SECOND_COL_X;
	const UINT16 x2_val = MERC_STATS_SECOND_NUM_COL_X;
	UINT16       y2     = MERC_HEALTH_Y;
	DrawStat(x2, y2,       str_stat_exp_level,    x2_val, p.bExpLevel);
	DrawStat(x2, y2 += dy, str_stat_marksmanship, x2_val, p.bMarksmanship);
	DrawStat(x2, y2 += dy, str_stat_mechanical,   x2_val, p.bMechanical);
	DrawStat(x2, y2 += dy, str_stat_explosive,    x2_val, p.bExplosive);
	DrawStat(x2, y2 += dy, str_stat_medical,      x2_val, p.bMedical);

	//Daily Salary
	y2 += dy;
	ST::string salary = MercInfo[MERC_FILES_SALARY];
	DrawTextToScreen(salary, MERC_STATS_SECOND_COL_X, y2, 0, MERC_NAME_FONT, MERC_STATIC_STATS_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	const UINT16 x = MERC_STATS_SECOND_COL_X + StringPixLength(salary, MERC_NAME_FONT) + 1;
	ST::string sString = ST::format("{} {}", p.sSalary, MercInfo[MERC_FILES_PER_DAY]);
	DrawTextToScreen(sString, x, y2, 95, MERC_NAME_FONT, MERC_DYNAMIC_STATS_COLOR, FONT_MCOLOR_BLACK, RIGHT_JUSTIFIED);
}


static BOOLEAN MercFilesHireMerc(UINT8 ubMercID)
{
	MERC_HIRE_STRUCT HireMercStruct;
	INT8	bReturnCode;

	HireMercStruct = MERC_HIRE_STRUCT{};
	MERCPROFILESTRUCT& p = GetProfile(ubMercID);

	//if the ALT key is down
	if (_KeyDown(ALT) && CHEATER_CHEAT_LEVEL())
	{
		//set the merc to be hireable
		p.bMercStatus           = MERC_OK;
		p.uiDayBecomesAvailable = 0;
	}

	//if the merc is away, dont hire
	if (!IsMercHireable(p))
	{
		if (p.bMercStatus != MERC_IS_DEAD)
		{
			guiCurrentLaptopMode = LAPTOP_MODE_MERC;
			gusMercVideoSpeckSpeech = SPECK_QUOTE_PLAYER_TRIES_TO_HIRE_ALREADY_HIRED_MERC;
			gubArrivedFromMercSubSite = MERC_CAME_FROM_HIRE_PAGE;
		}

		return(FALSE);
	}

	HireMercStruct.ubProfileID = ubMercID;
	HireMercStruct.bWhatKindOfMerc = MERC_TYPE__MERC;


	//HireMercStruct.fCopyProfileItemsOver = gfBuyEquipment;

	HireMercStruct.fCopyProfileItemsOver = TRUE;

	HireMercStruct.iTotalContractLength = 1;

	//Specify where the merc is to appear
	HireMercStruct.sSector                   = g_merc_arrive_sector;
	HireMercStruct.fUseLandingZoneForArrival = TRUE;

	HireMercStruct.uiTimeTillMercArrives = GetMercArrivalTimeOfDay( );// + ubMercID


	//Set the time and ID of the last hired merc will arrive
	//LaptopSaveInfo.sLastHiredMerc.iIdOfMerc = HireMercStruct.ubProfileID;
	//LaptopSaveInfo.sLastHiredMerc.uiArrivalTime = HireMercStruct.uiTimeTillMercArrives;


	bReturnCode = HireMerc(HireMercStruct);
	//already have 20 mercs on the team
	if( bReturnCode == MERC_HIRE_OVER_20_MERCS_HIRED )
	{
		DoLapTopMessageBox( MSG_BOX_LAPTOP_DEFAULT, MercInfo[ MERC_FILES_HIRE_TO_MANY_PEOPLE_WARNING ], LAPTOP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return(FALSE);
	}
	else if( bReturnCode == MERC_HIRE_FAILED )
	{
		//function failed
		return(FALSE);
	}
	else
	{
		//if we succesfully hired the merc
		return(TRUE);
	}
}


static void OpenMercUnavailableBox(ProfileID const pid)
{
	gubMercUnavailableBoxMercID = pid;
	gfMercUnavailableBoxActive  = TRUE;

	guiLeaveMessageButton = MakeUnavailableBoxButton(MercInfo[MERC_FILES_LEAVE_MESSAGE], MERC_UNAVAIL_BOX_LEAVE_MSG_X, MERC_UNAVAIL_BOX_LEAVE_MSG_Y, BtnMercUnavailableLeaveMessageCallback);
	if (GetProfile(pid).ubMiscFlags2 & PROFILE_MISC_FLAG2_PLAYER_LEFT_MSG_FOR_MERC_AT_MERC)
	{
		DisableButton(guiLeaveMessageButton);
	}
	guiHangUpBoxButton = MakeUnavailableBoxButton(MercInfo[MERC_FILES_HANG_UP], MERC_UNAVAIL_BOX_HANG_UP_X, MERC_UNAVAIL_BOX_HANG_UP_Y, BtnMercUnavailableHangUpCallback);

	guiMercUnavailableBoxCloseButton = QuickCreateButton(guiMercUnavailableBoxCloseButtonImage, MERC_UNAVAIL_BOX_XCLOSE_X, MERC_UNAVAIL_BOX_XCLOSE_Y, MSYS_PRIORITY_HIGH, BtnMercUnavailableHangUpCallback);
	guiMercUnavailableBoxCloseButton->SetCursor(CURSOR_LAPTOP_SCREEN);
	guiMercUnavailableBoxCloseButton->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_NONE);

	EnableButton(guiPrevButton,     FALSE);
	EnableButton(guiNextButton,     FALSE);
	EnableButton(guiHireButton,     FALSE);
	EnableButton(guiMercBackButton, FALSE);

	fReDrawScreenFlag = TRUE;
}


static void CloseMercUnavailableBox()
{
	gfMercUnavailableBoxActive = FALSE;

	RemoveButton(guiMercUnavailableBoxCloseButton);

	RemoveButton(guiLeaveMessageButton);
	RemoveButton(guiHangUpBoxButton);

	EnableDisableMercFilesNextPreviousButton();
	EnableButton(guiMercBackButton, TRUE);

	fReDrawScreenFlag = TRUE;
}


static void BtnMercUnavailableLeaveMessageCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	GetProfile(gubMercUnavailableBoxMercID).ubMiscFlags2 |= PROFILE_MISC_FLAG2_PLAYER_LEFT_MSG_FOR_MERC_AT_MERC;
	CloseMercUnavailableBox();
	DoLapTopMessageBox(MSG_BOX_LAPTOP_DEFAULT, MercInfo[MERC_FILES_MESSAGE_RECORDED], LAPTOP_SCREEN, MSG_BOX_FLAG_OK, NULL);
}


static void BtnMercUnavailableHangUpCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) CloseMercUnavailableBox();
}


static void DisplayMercUnavailableBox()
{
	MERCPROFILESTRUCT const& p = GetProfile(gubMercUnavailableBoxMercID);

	BltVideoObject(FRAME_BUFFER, GetVObject(guiMercUnavailableBox), 0, MERC_UNAVAIL_BOX_X, MERC_UNAVAIL_BOX_Y);

	DrawTextToScreen(p.zName, MERC_UNAVAIL_BOX_NAME_X, MERC_UNAVAIL_BOX_NAME_Y, 0, FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

	AutoSGPVObject face(LoadBigPortrait(p));
	BltVideoObject(FRAME_BUFFER, face.get(), 0, MERC_UNAVAIL_BOX_FACE_X, MERC_UNAVAIL_BOX_FACE_Y);
}


static void BtnMercFilesBackButtonCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_MERC;
		gubArrivedFromMercSubSite = MERC_CAME_FROM_HIRE_PAGE;
	}
}


static void EnableDisableMercFilesNextPreviousButton(void)
{
	EnableButton(guiNextButton, gubCurMercIndex <= LaptopSaveInfo.gubLastMercIndex - 1);
	EnableButton(guiPrevButton, gubCurMercIndex > 0);
}
