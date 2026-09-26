#include "IMP_Disability.h"

#include "Button_Sound_Control.h"
#include "Button_System.h"
#include "CharProfile.h"
#include "Cursors.h"
#include "Directories.h"
#include "Font.h"
#include "Font_Control.h"
#include "IMPVideoObjects.h"
#include "IMP_MainPage.h"
#include "Laptop.h"
#include "Soldier_Profile_Type.h"
#include "Text.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"
#include "WordWrap.h"

// pImpButtonText: title, then one label per personality trait in enum order
// the personality traits the game has code for: NO_PERSONALITYTRAIT .. PSYCHO
#define NUM_DISABILITIES   (PSYCHO + 1)

#define IMP_DIS_TXT_TITLE  50
#define IMP_DIS_TXT_FIRST  51

#define IMP_DIS_FONT       FONT12ARIAL
#define IMP_DIS_TITLE_FONT FONT14ARIAL
#define IMP_DIS_COLOR      FONT_MCOLOR_WHITE

// same layout as the skill traits page
#define IMP_DIS_LEFT_COLUMN_X   (LAPTOP_SCREEN_UL_X + 15)
#define IMP_DIS_RIGHT_COLUMN_X  (IMP_DIS_LEFT_COLUMN_X + 241)
#define IMP_DIS_START_Y         (LAPTOP_SCREEN_WEB_UL_Y + 40)
#define IMP_DIS_ROW_SPACING     38
#define IMP_DIS_ROWS_PER_COLUMN 7

#define IMP_DIS_TEXT_OFFSET_X   65
#define IMP_DIS_TEXT_OFFSET_Y   12
#define IMP_DIS_BOX_OFFSET_X    5
#define IMP_DIS_BOX_OFFSET_Y    7

#define IMP_DIS_TITLE_X         LAPTOP_SCREEN_UL_X
#define IMP_DIS_TITLE_Y         (LAPTOP_SCREEN_WEB_UL_Y + 10)
#define IMP_DIS_TITLE_WIDTH     (LAPTOP_SCREEN_LR_X - LAPTOP_SCREEN_UL_X)


static BOOLEAN gfRedraw = FALSE;

static GUIButtonRef  gDisabilityButton[NUM_DISABILITIES];
static BUTTON_PICS*  gDisabilityButtonImage[NUM_DISABILITIES];

static GUIButtonRef  gNextButton;
static BUTTON_PICS*  gNextButtonImage;

static SGPVObject*   guiGreyGoldBox = nullptr;


static INT16 ColumnX(INT32 i)
{
	return i < IMP_DIS_ROWS_PER_COLUMN ? IMP_DIS_LEFT_COLUMN_X : IMP_DIS_RIGHT_COLUMN_X;
}


static INT16 RowY(INT32 i)
{
	return IMP_DIS_START_Y + (i % IMP_DIS_ROWS_PER_COLUMN) * IMP_DIS_ROW_SPACING;
}


static void UpdateButtonStates(void)
{
	for (INT32 i = 0; i < NUM_DISABILITIES; ++i)
	{
		if (i == iPersonality)
		{
			gDisabilityButton[i]->uiFlags |= BUTTON_CLICKED_ON;
		}
		else
		{
			gDisabilityButton[i]->uiFlags &= ~BUTTON_CLICKED_ON;
		}
	}
}


static void BtnDisabilityCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED)) return;

	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
	{
		INT32 const disability = btn->GetUserData();
		if (disability == iPersonality)
		{
			// keep it pressed, one disability is always selected
			UpdateButtonStates();
			return;
		}

		iPersonality = disability;
		PlayButtonSound(btn, BUTTON_SOUND_CLICKED_ON);
		UpdateButtonStates();
		gfRedraw = TRUE;
	}
}


static void BtnNextCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED)) return;

	// act on release so the pressed state is drawn first
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		// the whole personality/skills/attitude/disability section is done
		iCurrentImpPage = IMP_MAIN_PAGE;
		iCurrentProfileMode = 2;
	}
}


void EnterIMPDisability(void)
{
	for (INT32 i = 0; i < NUM_DISABILITIES; ++i)
	{
		if (i == 0)
		{
			gDisabilityButtonImage[i] = LoadButtonImage(LAPTOPDIR "/button_6.sti", -1, 0, -1, 1, -1);
		}
		else
		{
			gDisabilityButtonImage[i] = UseLoadedButtonImage(gDisabilityButtonImage[0], -1, 0, -1, 1, -1);
		}

		gDisabilityButton[i] = QuickCreateButtonToggle(gDisabilityButtonImage[i], ColumnX(i), RowY(i),
					MSYS_PRIORITY_HIGHEST - 3, BtnDisabilityCallback);
		gDisabilityButton[i]->SetUserData(i);
		gDisabilityButton[i]->SetCursor(CURSOR_WWW);

		// the click sound is played by the callback
		gDisabilityButton[i]->ubSoundSchemeID = 0;
	}

	guiGreyGoldBox = AddVideoObjectFromFile("sti/laptop/SkillTraitSmallGreyIdent.sti");

	gNextButtonImage = LoadButtonImage(LAPTOPDIR "/button_5.sti", -1, 0, -1, 1, -1);
	gNextButton = CreateIconAndTextButton(gNextButtonImage, pImpButtonText[11], FONT12ARIAL,
		FONT_WHITE, DEFAULT_SHADOW,
		FONT_WHITE, DEFAULT_SHADOW,
		LAPTOP_SCREEN_UL_X + 350, LAPTOP_SCREEN_WEB_UL_Y + 340, MSYS_PRIORITY_HIGH,
		BtnNextCallback);
	gNextButton->SetCursor(CURSOR_WWW);

	if (iPersonality < 0 || iPersonality >= NUM_DISABILITIES) iPersonality = NO_PERSONALITYTRAIT;

	UpdateButtonStates();
}


void RenderIMPDisability(void)
{
	RenderProfileBackGround();

	// can be drawn once before Enter has run while the page change is pending
	if (!guiGreyGoldBox) return;

	DrawTextToScreen(pImpButtonText[IMP_DIS_TXT_TITLE],
		IMP_DIS_TITLE_X, IMP_DIS_TITLE_Y, IMP_DIS_TITLE_WIDTH, IMP_DIS_TITLE_FONT,
		IMP_DIS_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

	for (INT32 i = 0; i < NUM_DISABILITIES; ++i)
	{
		INT16 const textX = ColumnX(i) + IMP_DIS_TEXT_OFFSET_X;
		INT16 const textY = RowY(i) + IMP_DIS_TEXT_OFFSET_Y;

		// gold box for the selected disability, grey for the others
		BltVideoObject(FRAME_BUFFER, guiGreyGoldBox, i == iPersonality ? 1 : 0,
			textX - IMP_DIS_BOX_OFFSET_X, textY - IMP_DIS_BOX_OFFSET_Y);

		DrawTextToScreen(pImpButtonText[IMP_DIS_TXT_FIRST + i], textX, textY, 0,
			IMP_DIS_FONT, IMP_DIS_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}
}


void ExitIMPDisability(void)
{
	DeleteVideoObject(guiGreyGoldBox);
	guiGreyGoldBox = nullptr;

	for (INT32 i = 0; i < NUM_DISABILITIES; ++i)
	{
		RemoveButton(gDisabilityButton[i]);
		UnloadButtonImage(gDisabilityButtonImage[i]);
	}

	RemoveButton(gNextButton);
	UnloadButtonImage(gNextButtonImage);
}


void HandleIMPDisability(void)
{
	if (gfRedraw)
	{
		RenderIMPDisability();
		gfRedraw = FALSE;
	}

	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}
