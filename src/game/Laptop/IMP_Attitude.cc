#include "IMP_Attitude.h"

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

// pImpButtonText: title, then one label per attitude in enum order
#define IMP_ATT_TXT_TITLE  39
#define IMP_ATT_TXT_FIRST  40

#define IMP_ATT_FONT       FONT12ARIAL
#define IMP_ATT_TITLE_FONT FONT14ARIAL
#define IMP_ATT_COLOR      FONT_MCOLOR_WHITE

// same layout as the skill traits page
#define IMP_ATT_LEFT_COLUMN_X   (LAPTOP_SCREEN_UL_X + 15)
#define IMP_ATT_RIGHT_COLUMN_X  (IMP_ATT_LEFT_COLUMN_X + 241)
#define IMP_ATT_START_Y         (LAPTOP_SCREEN_WEB_UL_Y + 40)
#define IMP_ATT_ROW_SPACING     38
#define IMP_ATT_ROWS_PER_COLUMN 7

#define IMP_ATT_TEXT_OFFSET_X   65
#define IMP_ATT_TEXT_OFFSET_Y   12
#define IMP_ATT_BOX_OFFSET_X    5
#define IMP_ATT_BOX_OFFSET_Y    7

#define IMP_ATT_TITLE_X         LAPTOP_SCREEN_UL_X
#define IMP_ATT_TITLE_Y         (LAPTOP_SCREEN_WEB_UL_Y + 10)
#define IMP_ATT_TITLE_WIDTH     (LAPTOP_SCREEN_LR_X - LAPTOP_SCREEN_UL_X)


static BOOLEAN gfRedraw = FALSE;

static GUIButtonRef  gAttitudeButton[NUM_ATTITUDES];
static BUTTON_PICS*  gAttitudeButtonImage[NUM_ATTITUDES];

static GUIButtonRef  gNextButton;
static BUTTON_PICS*  gNextButtonImage;

static SGPVObject*   guiGreyGoldBox = nullptr;


static INT16 ColumnX(INT32 i)
{
	return i < IMP_ATT_ROWS_PER_COLUMN ? IMP_ATT_LEFT_COLUMN_X : IMP_ATT_RIGHT_COLUMN_X;
}


static INT16 RowY(INT32 i)
{
	return IMP_ATT_START_Y + (i % IMP_ATT_ROWS_PER_COLUMN) * IMP_ATT_ROW_SPACING;
}


static void UpdateButtonStates(void)
{
	for (INT32 i = 0; i < NUM_ATTITUDES; ++i)
	{
		if (i == iAttitude)
		{
			gAttitudeButton[i]->uiFlags |= BUTTON_CLICKED_ON;
		}
		else
		{
			gAttitudeButton[i]->uiFlags &= ~BUTTON_CLICKED_ON;
		}
	}
}


static void BtnAttitudeCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED)) return;

	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
	{
		INT32 const attitude = btn->GetUserData();
		if (attitude == iAttitude)
		{
			// keep it pressed, one attitude is always selected
			UpdateButtonStates();
			return;
		}

		iAttitude = attitude;
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
		// the disabilities page follows; it finishes this section
		fButtonPendingFlag = TRUE;
		iCurrentImpPage = IMP_DISABILITY;
	}
}


void EnterIMPAttitude(void)
{
	for (INT32 i = 0; i < NUM_ATTITUDES; ++i)
	{
		if (i == 0)
		{
			gAttitudeButtonImage[i] = LoadButtonImage(LAPTOPDIR "/button_6.sti", -1, 0, -1, 1, -1);
		}
		else
		{
			gAttitudeButtonImage[i] = UseLoadedButtonImage(gAttitudeButtonImage[0], -1, 0, -1, 1, -1);
		}

		gAttitudeButton[i] = QuickCreateButtonToggle(gAttitudeButtonImage[i], ColumnX(i), RowY(i),
					MSYS_PRIORITY_HIGHEST - 3, BtnAttitudeCallback);
		gAttitudeButton[i]->SetUserData(i);
		gAttitudeButton[i]->SetCursor(CURSOR_WWW);

		// the click sound is played by the callback
		gAttitudeButton[i]->ubSoundSchemeID = 0;
	}

	guiGreyGoldBox = AddVideoObjectFromFile("sti/laptop/SkillTraitSmallGreyIdent.sti");

	gNextButtonImage = LoadButtonImage(LAPTOPDIR "/button_5.sti", -1, 0, -1, 1, -1);
	gNextButton = CreateIconAndTextButton(gNextButtonImage, pImpButtonText[11], FONT12ARIAL,
		FONT_WHITE, DEFAULT_SHADOW,
		FONT_WHITE, DEFAULT_SHADOW,
		LAPTOP_SCREEN_UL_X + 350, LAPTOP_SCREEN_WEB_UL_Y + 340, MSYS_PRIORITY_HIGH,
		BtnNextCallback);
	gNextButton->SetCursor(CURSOR_WWW);

	if (iAttitude < 0 || iAttitude >= NUM_ATTITUDES) iAttitude = ATT_NORMAL;

	UpdateButtonStates();
}


void RenderIMPAttitude(void)
{
	RenderProfileBackGround();

	// can be drawn once before Enter has run while the page change is pending
	if (!guiGreyGoldBox) return;

	DrawTextToScreen(pImpButtonText[IMP_ATT_TXT_TITLE],
		IMP_ATT_TITLE_X, IMP_ATT_TITLE_Y, IMP_ATT_TITLE_WIDTH, IMP_ATT_TITLE_FONT,
		IMP_ATT_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

	for (INT32 i = 0; i < NUM_ATTITUDES; ++i)
	{
		INT16 const textX = ColumnX(i) + IMP_ATT_TEXT_OFFSET_X;
		INT16 const textY = RowY(i) + IMP_ATT_TEXT_OFFSET_Y;

		// gold box for the selected attitude, grey for the others
		BltVideoObject(FRAME_BUFFER, guiGreyGoldBox, i == iAttitude ? 1 : 0,
			textX - IMP_ATT_BOX_OFFSET_X, textY - IMP_ATT_BOX_OFFSET_Y);

		DrawTextToScreen(pImpButtonText[IMP_ATT_TXT_FIRST + i], textX, textY, 0,
			IMP_ATT_FONT, IMP_ATT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}
}


void ExitIMPAttitude(void)
{
	DeleteVideoObject(guiGreyGoldBox);
	guiGreyGoldBox = nullptr;

	for (INT32 i = 0; i < NUM_ATTITUDES; ++i)
	{
		RemoveButton(gAttitudeButton[i]);
		UnloadButtonImage(gAttitudeButtonImage[i]);
	}

	RemoveButton(gNextButton);
	UnloadButtonImage(gNextButtonImage);
}


void HandleIMPAttitude(void)
{
	if (gfRedraw)
	{
		RenderIMPAttitude();
		gfRedraw = FALSE;
	}

	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}
