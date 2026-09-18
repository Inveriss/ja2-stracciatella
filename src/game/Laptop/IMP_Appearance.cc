#include "Button_System.h"
#include "CharProfile.h"
#include "Cursors.h"
#include "Directories.h"
#include "Font.h"
#include "Font_Control.h"
#include "IMP_Appearance.h"
#include "IMPVideoObjects.h"
#include "Laptop.h"
#include "MouseSystem.h"
#include "Soldier_Profile_Type.h"
#include "Text.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"
#include "WordWrap.h"

#include <string_theory/string>


// Indices into pImpButtonText
enum
{
	IMP_APP_TXT_TITLE       = 29,
	IMP_APP_TXT_DESCRIPTION = 30,
	IMP_APP_TXT_HAIR        = 31,
	IMP_APP_TXT_SKIN        = 32,
	IMP_APP_TXT_SHIRT       = 33,
	IMP_APP_TXT_PANTS       = 34,
	IMP_APP_TXT_NORMAL_BODY = 35,
	IMP_APP_TXT_BIG_BODY    = 36,
	IMP_APP_TXT_OK          = 37,
	IMP_APP_TXT_RIFLE_TIP   = 38,
	IMP_APP_TXT_CANCEL      = 19
};

// Selectable rows, in the order they appear on the page.
enum { ROW_HAIR, ROW_SKIN, ROW_SHIRT, ROW_PANTS, NUM_ROWS };

// Palette names, in the order of the matching sub-images in Body_*.sti:
// skin 1-4, hair 5-9, shirts 10-20, pants 21-26 (sub-image 0 is the whole merc).
static char const* const gSkins[]  = { "PINKSKIN", "TANSKIN", "DARKSKIN", "BLACKSKIN" };
static char const* const gHairs[]  = { "BROWNHEAD", "BLACKHEAD", "WHITEHEAD", "REDHEAD", "BLONDHEAD" };
static char const* const gShirts[] =
{
	"WHITEVEST", "YELLOWVEST", "GYELLOWSHIRT", "greyVEST", "BROWNVEST", "PURPLESHIRT",
	"BLUEVEST", "JEANVEST", "GREENVEST", "REDVEST", "BLACKSHIRT"
};
static char const* const gPants[]  = { "BLUEPANTS", "BLACKPANTS", "JEANPANTS", "TANPANTS", "BEIGEPANTS", "GREENPANTS" };

static INT32 const gRowCount[NUM_ROWS]   = { 5, 4, 11, 6 };
static INT32 const gRowFirstFrame[NUM_ROWS] = { 5, 1, 10, 21 };

// Layout, relative to the upper left corner of the laptop web screen
static INT32 const APP_DESC_X        = 108;
static INT32 const APP_DESC_Y        =  42;
static INT32 const APP_FRAME_X       = 111;
static INT32 const APP_FRAME_Y       = 101;
static INT32 const APP_BODY_X        = 225;
static INT32 const APP_BODY_Y        = 114;
static INT32 const APP_ARROW_LEFT_X  = 204;
static INT32 const APP_ARROW_RIGHT_X = 331;
static INT32 const APP_ARROW_FIRST_Y = 119;
static INT32 const APP_ARROW_STEP_Y  =  27;
static INT32 const APP_ARROW_SIZE    =  22;
static INT32 const APP_LABEL_X       = 112;
static INT32 const APP_LABEL_W       =  80;
static INT32 const APP_BODY_BTN_X    = 123;
static INT32 const APP_BODY_BTN_Y    = 249;
static INT32 const APP_BODY_BTN_W    =  52;
static INT32 const APP_BODY_BTN_H    =  35;
static INT32 const APP_BODY_BAR_X    = 188;
static INT32 const APP_BODY_BAR_DY   =   5;
static INT32 const APP_BODY_BAR_W    = 161;
static INT32 const APP_BODY_BAR_H    =  24;
static INT32 const APP_BODY_STEP_Y   =  38;
static INT32 const APP_CHECK_X       = 353;
static INT32 const APP_CHECK_Y       = 298;
static INT32 const APP_CHECK_SIZE    =  15;
static INT32 const APP_OK_X          = 350;
static INT32 const APP_OK_Y          = 340;
static INT32 const APP_CANCEL_X      =  15;
static INT32 const APP_CANCEL_Y      = 360;

// Selection state. Kept outside the merc profile: the voice (and so the
// profile slot) isn't known yet when this page is shown.
static INT32   giSelection[NUM_ROWS];
static BOOLEAN gfBigBody;
static BOOLEAN gfAltRifleHold;
static INT8    gbSelectionForMale = -1; // gender the selection was made for, -1 = none yet

// Page state
static BOOLEAN gfReDraw;
static INT32   giPressedArrow = -1; // row * 2 + (0 = left, 1 = right), or -1

static SGPVObject* gvoDescription;
static SGPVObject* gvoFrame;
static SGPVObject* gvoBodyNormal;
static SGPVObject* gvoBodyBig;
static SGPVObject* gvoBodyFemale;
static SGPVObject* gvoArrows;
static SGPVObject* gvoBodyButtons;
static SGPVObject* gvoBodyBars;
static SGPVObject* gvoCheckBox;

static BUTTON_PICS* gpOkImage;
static BUTTON_PICS* gpCancelImage;
static GUIButtonRef gOkButton;
static GUIButtonRef gCancelButton;

static MOUSE_REGION gArrowRegions[NUM_ROWS * 2];
static MOUSE_REGION gBodyRegions[2];
static MOUSE_REGION gCheckRegion;
static BOOLEAN      gfBodyRegionsCreated;


static INT32 ScreenX(void) { return LAPTOP_SCREEN_UL_X; }
static INT32 ScreenY(void) { INT32 const y = LAPTOP_SCREEN_WEB_UL_Y; return y; }


void ResetImpAppearance(void)
{
	for (INT32 i = 0; i < NUM_ROWS; ++i) giSelection[i] = 0;
	gfBigBody          = FALSE;
	gfAltRifleHold     = FALSE;
	gbSelectionForMale = -1;
}


void ApplyImpAppearanceToProfile(MERCPROFILESTRUCT& p)
{
	p.SKIN  = gSkins[giSelection[ROW_SKIN]];
	p.HAIR  = gHairs[giSelection[ROW_HAIR]];
	p.VEST  = gShirts[giSelection[ROW_SHIRT]];
	p.PANTS = gPants[giSelection[ROW_PANTS]];
}


BOOLEAN ImpAppearanceIsBigBody(void)
{
	return gfBigBody;
}


BOOLEAN ImpAppearanceUsesAltRifleHold(void)
{
	return gfBigBody && gfAltRifleHold;
}


static BOOLEAN IsMale(void)
{
	return fCharacterIsMale;
}


static BOOLEAN CheckBoxEnabled(void)
{
	return IsMale() && gfBigBody;
}


static void Redraw(void)
{
	gfReDraw = TRUE;
}


void RenderIMPAppearance(void)
{
	INT32 const dx = ScreenX();
	INT32 const dy = ScreenY();
	INT16 const sWidth = LAPTOP_SCREEN_LR_X - LAPTOP_SCREEN_UL_X + 1;

	RenderProfileBackGround();

	// title
	DisplayWrappedString(LAPTOP_SCREEN_UL_X, dy + 9, sWidth, 2, FONT14ARIAL, FONT_WHITE, pImpButtonText[IMP_APP_TXT_TITLE], FONT_BLACK, CENTER_JUSTIFIED);

	// description
	BltVideoObject(FRAME_BUFFER, gvoDescription, 0, dx + APP_DESC_X, dy + APP_DESC_Y);
	DisplayWrappedString(dx + APP_DESC_X + 10, dy + APP_DESC_Y + 4, 230, 2, FONT10ARIAL, 142, pImpButtonText[IMP_APP_TXT_DESCRIPTION], FONT_BLACK, CENTER_JUSTIFIED);

	// the merc: the whole picture first, then the selected skin, hair, shirt and
	// pants cut-outs on top of it. The frame covers the edges of the picture.
	SGPVObject* const body = !IsMale() ? gvoBodyFemale : gfBigBody ? gvoBodyBig : gvoBodyNormal;
	BltVideoObject(FRAME_BUFFER, body, 0, dx + APP_BODY_X, dy + APP_BODY_Y);
	for (INT32 row = 0; row < NUM_ROWS; ++row)
	{
		BltVideoObject(FRAME_BUFFER, body, gRowFirstFrame[row] + giSelection[row], dx + APP_BODY_X, dy + APP_BODY_Y);
	}
	BltVideoObject(FRAME_BUFFER, gvoFrame, 0, dx + APP_FRAME_X, dy + APP_FRAME_Y);

	// labels and selection arrows
	UINT16 const labelHeight = GetFontHeight(FONT12ARIAL);
	for (INT32 row = 0; row < NUM_ROWS; ++row)
	{
		INT32 const y = dy + APP_ARROW_FIRST_Y + APP_ARROW_STEP_Y * row;

		DrawTextToScreen(pImpButtonText[IMP_APP_TXT_HAIR + row], dx + APP_LABEL_X, y + (APP_ARROW_SIZE - labelHeight) / 2, APP_LABEL_W, FONT12ARIAL, FONT_WHITE, FONT_BLACK, RIGHT_JUSTIFIED);

		bool const canGoLeft  = giSelection[row] > 0;
		bool const canGoRight = giSelection[row] < gRowCount[row] - 1;
		INT32 const left  = !canGoLeft  ? 2 : giPressedArrow == row * 2     ? 1 : 0;
		INT32 const right = !canGoRight ? 5 : giPressedArrow == row * 2 + 1 ? 4 : 3;
		BltVideoObject(FRAME_BUFFER, gvoArrows, left,  dx + APP_ARROW_LEFT_X,  y);
		BltVideoObject(FRAME_BUFFER, gvoArrows, right, dx + APP_ARROW_RIGHT_X, y);
	}

	// body type (a woman only has the one body)
	if (IsMale())
	{
		for (INT32 i = 0; i < 2; ++i)
		{
			bool const selected = gfBigBody == (i == 1);
			INT32 const btnY = dy + APP_BODY_BTN_Y + APP_BODY_STEP_Y * i;
			INT32 const barY = btnY + APP_BODY_BAR_DY;
			BltVideoObject(FRAME_BUFFER, gvoBodyButtons, selected ? 1 : 0, dx + APP_BODY_BTN_X, btnY);
			BltVideoObject(FRAME_BUFFER, gvoBodyBars,    selected ? 1 : 0, dx + APP_BODY_BAR_X, barY);
			DrawTextToScreen(pImpButtonText[IMP_APP_TXT_NORMAL_BODY + i], dx + APP_BODY_BAR_X + 7, barY + (APP_BODY_BAR_H - labelHeight) / 2, APP_BODY_BAR_W - 14, FONT12ARIAL, FONT_WHITE, FONT_BLACK, LEFT_JUSTIFIED);
		}
	}

	// alternative rifle holding: 0/2 = enabled (off/on), 1/3 = disabled (off/on)
	INT32 const checkFrame = (gfAltRifleHold ? 2 : 0) + (CheckBoxEnabled() ? 0 : 1);
	BltVideoObject(FRAME_BUFFER, gvoCheckBox, checkFrame, dx + APP_CHECK_X, dy + APP_CHECK_Y);

	MarkButtonsDirty();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, dy, LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_LR_Y);
}


void HandleIMPAppearance(void)
{
	if (gfReDraw)
	{
		RenderIMPAppearance();
		gfReDraw = FALSE;
	}
}


static void ChangeSelection(INT32 const row, INT32 const delta)
{
	INT32 const value = giSelection[row] + delta;
	if (value < 0 || value >= gRowCount[row]) return;
	giSelection[row] = value;
}


static void ArrowMoveCallback(MOUSE_REGION*, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		if (giPressedArrow != -1)
		{
			giPressedArrow = -1;
			Redraw();
		}
	}
}


static void ArrowClickCallback(MOUSE_REGION* const region, UINT32 const reason)
{
	INT32 const idx   = static_cast<INT32>(region - gArrowRegions);
	INT32 const row   = idx / 2;
	INT32 const delta = (idx % 2 == 0) ? -1 : 1;

	bool const enabled = delta < 0 ? giSelection[row] > 0 : giSelection[row] < gRowCount[row] - 1;
	if (!enabled) return;

	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
	{
		giPressedArrow = idx;
		Redraw();
	}
	else if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		if (giPressedArrow == idx) ChangeSelection(row, delta);
		giPressedArrow = -1;
		Redraw();
	}
}


static void BodyClickCallback(MOUSE_REGION* const region, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	BOOLEAN const big = (region == &gBodyRegions[1]);
	if (gfBigBody == big) return;

	gfBigBody = big;
	// the alternative rifle holding only exists for the big body
	if (!gfBigBody) gfAltRifleHold = FALSE;
	Redraw();
}


static void CheckClickCallback(MOUSE_REGION*, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (!CheckBoxEnabled()) return;

	gfAltRifleHold = !gfAltRifleHold;
	Redraw();
}


static void BtnAppearanceOkCallback(GUI_BUTTON*, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	iCurrentImpPage    = IMP_MAIN_PAGE;
	fButtonPendingFlag = TRUE;
}


static void BtnAppearanceCancelCallback(GUI_BUTTON*, UINT32 const reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	// cancelling means the default colors and body
	ResetImpAppearance();
	iCurrentImpPage    = IMP_MAIN_PAGE;
	fButtonPendingFlag = TRUE;
}


static SGPVObject* Load(char const* const file)
{
	return AddVideoObjectFromFile(file);
}


static void MakeRegion(MOUSE_REGION* const r, INT32 const x, INT32 const y, INT32 const w, INT32 const h, MOUSE_CALLBACK const move, MOUSE_CALLBACK const click)
{
	MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, CURSOR_WWW, move, click);
}


void EnterIMPAppearance(void)
{
	// a selection made for the other gender doesn't make sense any more
	INT8 const male = IsMale() ? 1 : 0;
	if (gbSelectionForMale != male)
	{
		ResetImpAppearance();
		gbSelectionForMale = male;
	}
	if (!IsMale())
	{
		gfBigBody      = FALSE;
		gfAltRifleHold = FALSE;
	}
	if (!gfBigBody) gfAltRifleHold = FALSE;

	giPressedArrow = -1;
	gfReDraw       = FALSE;

	gvoDescription = Load(LAPTOPDIR "/IMP/BackGround_Description.STI");
	gvoFrame       = Load(LAPTOPDIR "/IMP/BackGround_Main_Bodies.STI");
	gvoBodyNormal  = Load(LAPTOPDIR "/IMP/Body_Normal_Male.sti");
	gvoBodyBig     = Load(LAPTOPDIR "/IMP/Body_Big_Male.sti");
	gvoBodyFemale  = Load(LAPTOPDIR "/IMP/Body_Female.sti");
	gvoArrows      = Load(LAPTOPDIR "/IMP/Selection_Arrows.STI");
	gvoBodyButtons = Load(LAPTOPDIR "/IMP/Buttons_Body.STI");
	gvoBodyBars    = Load(LAPTOPDIR "/IMP/BackGround_Buttons_Bodies.STI");
	gvoCheckBox    = Load(LAPTOPDIR "/IMP/CheckBox_Rifle.STI");

	INT32 const dx = ScreenX();
	INT32 const dy = ScreenY();

	// OK and Cancel
	gpOkImage     = LoadButtonImage(LAPTOPDIR "/IMP/Button_Done.STI", 0, 1);
	gpCancelImage = LoadButtonImage(LAPTOPDIR "/IMP/Button_Cancel.STI", 0, 1);
	gOkButton = CreateIconAndTextButton(gpOkImage, pImpButtonText[IMP_APP_TXT_OK], FONT12ARIAL,
						FONT_WHITE, DEFAULT_SHADOW, FONT_WHITE, DEFAULT_SHADOW,
						dx + APP_OK_X, dy + APP_OK_Y, MSYS_PRIORITY_HIGH, BtnAppearanceOkCallback);
	gOkButton->SetCursor(CURSOR_WWW);
	gCancelButton = CreateIconAndTextButton(gpCancelImage, pImpButtonText[IMP_APP_TXT_CANCEL], FONT12ARIAL,
						FONT_WHITE, DEFAULT_SHADOW, FONT_WHITE, DEFAULT_SHADOW,
						dx + APP_CANCEL_X, dy + APP_CANCEL_Y, MSYS_PRIORITY_HIGH, BtnAppearanceCancelCallback);
	gCancelButton->SetCursor(CURSOR_WWW);

	// selection arrows
	for (INT32 row = 0; row < NUM_ROWS; ++row)
	{
		INT32 const y = dy + APP_ARROW_FIRST_Y + APP_ARROW_STEP_Y * row;
		MakeRegion(&gArrowRegions[row * 2],     dx + APP_ARROW_LEFT_X,  y, APP_ARROW_SIZE, APP_ARROW_SIZE, ArrowMoveCallback, ArrowClickCallback);
		MakeRegion(&gArrowRegions[row * 2 + 1], dx + APP_ARROW_RIGHT_X, y, APP_ARROW_SIZE, APP_ARROW_SIZE, ArrowMoveCallback, ArrowClickCallback);
	}

	// body type rows (the button and its bar), men only
	gfBodyRegionsCreated = IsMale();
	if (gfBodyRegionsCreated)
	{
		for (INT32 i = 0; i < 2; ++i)
		{
			INT32 const y = dy + APP_BODY_BTN_Y + APP_BODY_STEP_Y * i;
			MakeRegion(&gBodyRegions[i], dx + APP_BODY_BTN_X, y, APP_BODY_BAR_X + APP_BODY_BAR_W - APP_BODY_BTN_X, APP_BODY_BTN_H, MSYS_NO_CALLBACK, BodyClickCallback);
		}
	}

	// alternative rifle holding check box
	MakeRegion(&gCheckRegion, dx + APP_CHECK_X, dy + APP_CHECK_Y, APP_CHECK_SIZE, APP_CHECK_SIZE, MSYS_NO_CALLBACK, CheckClickCallback);
	gCheckRegion.SetFastHelpText(pImpButtonText[IMP_APP_TXT_RIFLE_TIP]);

	RenderIMPAppearance();
}


void ExitIMPAppearance(void)
{
	for (MOUSE_REGION& r : gArrowRegions) MSYS_RemoveRegion(&r);
	if (gfBodyRegionsCreated)
	{
		for (MOUSE_REGION& r : gBodyRegions) MSYS_RemoveRegion(&r);
		gfBodyRegionsCreated = FALSE;
	}
	MSYS_RemoveRegion(&gCheckRegion);

	RemoveButton(gOkButton);
	UnloadButtonImage(gpOkImage);
	RemoveButton(gCancelButton);
	UnloadButtonImage(gpCancelImage);

	DeleteVideoObject(gvoDescription);
	DeleteVideoObject(gvoFrame);
	DeleteVideoObject(gvoBodyNormal);
	DeleteVideoObject(gvoBodyBig);
	DeleteVideoObject(gvoBodyFemale);
	DeleteVideoObject(gvoArrows);
	DeleteVideoObject(gvoBodyButtons);
	DeleteVideoObject(gvoBodyBars);
	DeleteVideoObject(gvoCheckBox);
}
