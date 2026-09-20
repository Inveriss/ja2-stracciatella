#include "Cursors.h"
#include "Directories.h"
#include "HImage.h"
#include "Laptop.h"
#include "AIMFacialIndex.h"
#include "MercPortrait.h"
#include "Merc_Hiring.h"
#include "Soldier_Control.h"
#include "VObject.h"
#include "WordWrap.h"
#include "AIM.h"
#include "Soldier_Profile.h"
#include "Text.h"
#include "AIMSort.h"
#include "Assignments.h"
#include "Button_System.h"
#include "Object_Cache.h"
#include "Video.h"
#include "VSurface.h"
#include "Font_Control.h"
#include "Font.h"

#include <string_theory/string>


extern UINT8			gbCurrentIndex;


static cache_key_t const guiMugShotBorder{ LAPTOPDIR "/mugshotborder3.sti" };
static SGPVObject* guiAimFiFace[MAX_NUMBER_MERCS];

// With more mercs than fit on the screen the index has several pages.
static UINT8 gubAimFiPage = 0;
static BUTTON_PICS* guiAimFiButtonImage;
static GUIButtonRef guiAimFiPreviousButton;
static GUIButtonRef guiAimFiNextButton;
#define AIM_FI_NUM_FILTER_BUTTONS 6
static GUIButtonRef guiAimFiFilterButtons[AIM_FI_NUM_FILTER_BUTTONS];



#define AIM_FI_NUM_MUHSHOTS_X		8
#define AIM_FI_NUM_MUHSHOTS_Y		5
#define AIM_FI_MUGSHOTS_PER_PAGE	(AIM_FI_NUM_MUHSHOTS_X * AIM_FI_NUM_MUHSHOTS_Y)

// The small logo, the Previous and Next buttons and the row of the filter buttons above the faces.
// The positions are relative to the upper left corner of the laptop web screen.
#define AIM_FI_LOGO_X			(IMAGE_OFFSET_X + 91)
#define AIM_FI_LOGO2_X			(IMAGE_OFFSET_X + 309)
#define AIM_FI_LOGO_Y			(IMAGE_OFFSET_Y + 8)
#define AIM_FI_PAGE_BUTTON_Y		(IMAGE_OFFSET_Y + 8)
#define AIM_FI_PREVIOUS_BUTTON_X	(IMAGE_OFFSET_X + 5)
#define AIM_FI_NEXT_BUTTON_X		(IMAGE_OFFSET_X + 420)

#define AIM_FI_PORTRAIT_WIDTH		52
#define AIM_FI_PORTRAIT_HEIGHT		48

#define AIM_FI_FIRST_MUGSHOT_X		IMAGE_OFFSET_X + 6
#define AIM_FI_FIRST_MUGSHOT_Y		IMAGE_OFFSET_Y + 69//67//70 //68 //65
#define AIM_FI_MUGSHOT_GAP_X		10
#define AIM_FI_MUGSHOT_GAP_Y		13
#define AIM_FI_FACE_OFFSET		2

#define AIM_FI_NNAME_OFFSET_X		2
#define AIM_FI_NNAME_OFFSET_Y		AIM_FI_PORTRAIT_HEIGHT+1
#define AIM_FI_NNAME_WIDTH		AIM_FI_PORTRAIT_WIDTH+4

#define AIM_FI_AWAY_TEXT_OFFSET_X	3
#define AIM_FI_AWAY_TEXT_OFFSET_Y	23//3//36
#define AIM_FI_AWAY_TEXT_OFFSET_WIDTH	48


//Mouse Regions

//Face regions
static MOUSE_REGION gMercFaceMouseRegions[MAX_NUMBER_MERCS];

//Screen region, used to right click to go back to previous page
static MOUSE_REGION gScreenMouseRegions;


static UINT8 NumAimFiPages()
{
	return (gubNumAimMercs + AIM_FI_MUGSHOTS_PER_PAGE - 1) / AIM_FI_MUGSHOTS_PER_PAGE;
}


// The index (in AimMercArray) of the merc shown at the given place of the current page,
// or gubNumAimMercs if the place is empty.
static UINT8 MercAtPlace(UINT8 const place)
{
	unsigned const index = gubAimFiPage * AIM_FI_MUGSHOTS_PER_PAGE + place;
	return index < gubNumAimMercs ? index : gubNumAimMercs;
}


static void SelectMercFaceMoveRegionCallBack(MOUSE_REGION* pRegion, UINT32 reason);
static void SelectMercFaceRegionCallBackPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectMercFaceRegionCallBackSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
// There is no SelectScreenRegionCallBackPrimary
static void SelectScreenRegionCallBackSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
static void MouseWheelRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);


// The filter buttons in the order of AimFilter: ALL is at the top between the logos, the others are
// in a row above the faces (OTHERS under Next). The positions are relative to the upper left corner
// of the laptop web screen; the offset of the screen (IMAGE_OFFSET_X/Y, it depends on the resolution)
// is added when the buttons are made, not when the program starts.
static INT16 const AIM_FI_FILTER_BUTTON_REL_X[AIM_FI_NUM_FILTER_BUTTONS] = { 213, 6, 110, 213, 315, 417 };
static INT16 const AIM_FI_FILTER_BUTTON_REL_Y[AIM_FI_NUM_FILTER_BUTTONS] = { 13, 46, 46, 46, 46, 46 };
static char const* const gAimFiFilterNames[AIM_FI_NUM_FILTER_BUTTONS] = { "ALL", "JA2", "UB", "WILDFIRE", "JA1", "OTHERS" };


// The faces of all mercs of AimMercArray, whatever page is shown; they are indexed like the array
static void UnloadAimFiFaces()
{
	FOR_EACH(SGPVObject*, i, guiAimFiFace)
	{
		if (*i != nullptr) DeleteVideoObject(*i);
		*i = nullptr;
	}
}


static void LoadAimFiFaces()
{
	for (UINT8 i = 0; i < gubNumAimMercs; ++i)
	{
		guiAimFiFace[i] = LoadSmallPortrait(GetProfile(AimMercArray[i]));
	}
}


// Changes the page by the given number of pages; the arrow buttons wrap around at the ends, the mouse wheel does not
static void ChangeAimFiPage(int const delta, bool const wrap)
{
	int const pages = NumAimFiPages();
	if (pages == 0) return;
	int page = gubAimFiPage + delta;
	if (wrap) page = (page + pages) % pages;
	if (page < 0 || page >= pages || page == gubAimFiPage) return;
	gubAimFiPage = page;
	RenderAimFacialIndex();
}


static GUIButtonRef MakeAimFiButton(ST::string const& text, INT16 const x, INT16 const y, GUI_CALLBACK click)
{
	GUIButtonRef const btn = CreateIconAndTextButton(
		guiAimFiButtonImage, text, FONT14ARIAL,
		FONT_MCOLOR_DKWHITE, DEFAULT_SHADOW,
		138,                 DEFAULT_SHADOW,
		x, y, MSYS_PRIORITY_HIGH, click);
	// the text sits 1 pixel to the right and 2 pixels lower than centered
	btn->SpecifyTextSubOffsets(1, 1, TRUE);
	btn->SetCursor(CURSOR_WWW);
	return btn;
}


// The button of the current filter is pressed, none if the list is empty
static void SyncFilterButtons()
{
	for (int i = 0; i < AIM_FI_NUM_FILTER_BUTTONS; ++i)
	{
		if (!guiAimFiFilterButtons[i]) continue;
		if (i == GetAimFilter()) guiAimFiFilterButtons[i]->uiFlags |= BUTTON_CLICKED_ON;
		else                     guiAimFiFilterButtons[i]->uiFlags &= ~BUTTON_CLICKED_ON;
	}
}


static void BtnPreviousPageCallback(GUI_BUTTON*, UINT32 reason);
static void BtnNextPageCallback(GUI_BUTTON*, UINT32 reason);
static void BtnFilterCallback(GUI_BUTTON* const btn, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		// pressing the pressed button releases it, the list is empty then
		AimFilter filter = static_cast<AimFilter>(btn->GetUserData());
		if (filter == GetAimFilter()) filter = AIM_FILTER_NONE;
		if (filter != GetAimFilter())
		{
			SetAimFilter(filter);
			ResetAimMercArray();
			SortAimMercArray();
			UnloadAimFiFaces();
			LoadAimFiFaces();
			gubAimFiPage = 0;
			RenderAimFacialIndex();
		}
	}
	SyncFilterButtons();
}


void EnterAimFacialIndex()
{
	UINT8	i;
	UINT16		usPosX, usPosY, x,y;

	if (gubAimFiPage >= NumAimFiPages()) gubAimFiPage = 0;

	usPosX = AIM_FI_FIRST_MUGSHOT_X;
	usPosY = AIM_FI_FIRST_MUGSHOT_Y;
	i=0;
	for(y=0; y<AIM_FI_NUM_MUHSHOTS_Y; y++)
	{
		for(x=0; x<AIM_FI_NUM_MUHSHOTS_X; x++)
		{

			MSYS_DefineRegion(&gMercFaceMouseRegions[ i ], usPosX, usPosY,
						(INT16)(usPosX + AIM_FI_PORTRAIT_WIDTH),
						(INT16)(usPosY + AIM_FI_PORTRAIT_HEIGHT),
						MSYS_PRIORITY_HIGH,
						CURSOR_WWW, SelectMercFaceMoveRegionCallBack,
						MouseCallbackPrimarySecondary(SelectMercFaceRegionCallBackPrimary, SelectMercFaceRegionCallBackSecondary, MouseWheelRegionCallBack));
			MSYS_SetRegionUserData( &gMercFaceMouseRegions[ i ], 0, i);

			usPosX += AIM_FI_PORTRAIT_WIDTH + AIM_FI_MUGSHOT_GAP_X;
			i++;
		}
		usPosX = AIM_FI_FIRST_MUGSHOT_X;
		usPosY += AIM_FI_PORTRAIT_HEIGHT + AIM_FI_MUGSHOT_GAP_Y;
	}

	LoadAimFiFaces();

	MSYS_DefineRegion(&gScreenMouseRegions, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
				LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y, MSYS_PRIORITY_HIGH-1,
				CURSOR_LAPTOP_SCREEN, MSYS_NO_CALLBACK, MouseCallbackPrimarySecondary(MSYS_NO_CALLBACK, SelectScreenRegionCallBackSecondary, MouseWheelRegionCallBack));

	InitAimMenuBar();
	SetAimSmallLogo(true, AIM_FI_LOGO_X, AIM_FI_LOGO_Y);
	SetAimSecondSmallLogo(AIM_FI_LOGO2_X, AIM_FI_LOGO_Y);
	InitAimDefaults();

	guiAimFiButtonImage = LoadButtonImage(LAPTOPDIR "/bottombuttons2.sti", 0, 1);
	guiAimFiPreviousButton = MakeAimFiButton(CharacterInfo[AIM_MEMBER_PREVIOUS], AIM_FI_PREVIOUS_BUTTON_X, AIM_FI_PAGE_BUTTON_Y, BtnPreviousPageCallback);
	guiAimFiNextButton     = MakeAimFiButton(CharacterInfo[AIM_MEMBER_NEXT],     AIM_FI_NEXT_BUTTON_X,     AIM_FI_PAGE_BUTTON_Y, BtnNextPageCallback);
	for (int i = 0; i < AIM_FI_NUM_FILTER_BUTTONS; ++i)
	{
		guiAimFiFilterButtons[i] = MakeAimFiButton(gAimFiFilterNames[i], IMAGE_OFFSET_X + AIM_FI_FILTER_BUTTON_REL_X[i], IMAGE_OFFSET_Y + AIM_FI_FILTER_BUTTON_REL_Y[i], BtnFilterCallback);
		guiAimFiFilterButtons[i]->SetUserData(i);
	}

	RenderAimFacialIndex();
}


void ExitAimFacialIndex()
{
	RemoveAimDefaults();

	RemoveVObject(guiMugShotBorder);

	UnloadAimFiFaces();
	FOR_EACH(MOUSE_REGION, i, gMercFaceMouseRegions) MSYS_RemoveRegion(&*i);
	ExitAimMenuBar();

	MSYS_RemoveRegion(&gScreenMouseRegions);

	RemoveButton(guiAimFiPreviousButton);
	RemoveButton(guiAimFiNextButton);
	FOR_EACH(GUIButtonRef, i, guiAimFiFilterButtons) { RemoveButton(*i); *i = GUIButtonRef(); }
	UnloadButtonImage(guiAimFiButtonImage);

	SetAimSmallLogo(false);
}


static void DrawMercsFaceToScreen(UINT8 ubMercID, UINT16 usPosX, UINT16 usPosY, UINT8 ubImage);


void RenderAimFacialIndex()
{
	UINT16		usPosX, usPosY, x,y;
	UINT8			i;

	DrawAimDefaults();

	//Draw the mug shot border and face
	usPosX = AIM_FI_FIRST_MUGSHOT_X;
	usPosY = AIM_FI_FIRST_MUGSHOT_Y;

	i=0;
	for(y=0; y<AIM_FI_NUM_MUHSHOTS_Y; y++)
	{
		for(x=0; x<AIM_FI_NUM_MUHSHOTS_X; x++)
		{
			UINT8 const merc = MercAtPlace(i);
			if (merc < gubNumAimMercs)
			{
				DrawMercsFaceToScreen(merc, usPosX, usPosY, 1);
				DrawTextToScreen(gMercProfiles[AimMercArray[merc]].zNickname, usPosX - AIM_FI_NNAME_OFFSET_X, usPosY + AIM_FI_NNAME_OFFSET_Y, AIM_FI_NNAME_WIDTH, AIM_FONT12ARIAL, AIM_FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
			}

			usPosX += AIM_FI_PORTRAIT_WIDTH + AIM_FI_MUGSHOT_GAP_X;
			i++;
		}
		usPosX = AIM_FI_FIRST_MUGSHOT_X;
		usPosY += AIM_FI_PORTRAIT_HEIGHT + AIM_FI_MUGSHOT_GAP_Y;
	}

	DisableAimButton();
	SyncFilterButtons();

	MarkButtonsDirty( );

	RenderWWWProgramTitleBar( );

	InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
}


static void BtnPreviousPageCallback(GUI_BUTTON*, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ChangeAimFiPage(-1, true);
}


static void BtnNextPageCallback(GUI_BUTTON*, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP) ChangeAimFiPage(1, true);
}



// The mouse wheel turns the pages, without wrapping around at the first and the last page
static void MouseWheelRegionCallBack(MOUSE_REGION*, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)        ChangeAimFiPage(-1, false);
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN) ChangeAimFiPage(1, false);
}


static void SelectMercFaceRegionCallBackPrimary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	UINT8 const merc = MercAtPlace(MSYS_GetRegionUserData(pRegion, 0));
	if (merc >= gubNumAimMercs) return; // empty place on the last page

	guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS;
	gbCurrentIndex = merc;
}

static void SelectMercFaceRegionCallBackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS_SORTED_FILES;
}


static void SelectScreenRegionCallBackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS_SORTED_FILES;
}


static void SelectMercFaceMoveRegionCallBack(MOUSE_REGION* pRegion, UINT32 reason)
{
	UINT8	ubPlace;
	UINT16 usPosX, usPosY;

	ubPlace = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );
	UINT8 const ubMercNum = MercAtPlace(ubPlace);
	if (ubMercNum >= gubNumAimMercs) return; // empty place on the last page

	usPosY = ubPlace / AIM_FI_NUM_MUHSHOTS_X;
	usPosY = AIM_FI_FIRST_MUGSHOT_Y + (AIM_FI_PORTRAIT_HEIGHT + AIM_FI_MUGSHOT_GAP_Y) * usPosY;

	usPosX = ubPlace % AIM_FI_NUM_MUHSHOTS_X;
	usPosX = AIM_FI_FIRST_MUGSHOT_X + (AIM_FI_PORTRAIT_WIDTH + AIM_FI_MUGSHOT_GAP_X) * usPosX;

	//fReDrawNewMailFlag = TRUE;

	if( reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		DrawMercsFaceToScreen(ubMercNum, usPosX, usPosY, 1);
		InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX, pRegion->RegionBottomRightY);
	}
	else if( reason & MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		DrawMercsFaceToScreen(ubMercNum, usPosX, usPosY, 0);
		InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX, pRegion->RegionBottomRightY);
	}
}


static void DrawMercsFaceToScreen(const UINT8 ubMercID, const UINT16 usPosX, const UINT16 usPosY, const UINT8 ubImage)
{
	const ProfileID          id = AimMercArray[ubMercID];
	const SOLDIERTYPE* const s  = FindSoldierByProfileIDOnPlayerTeam(id);

	//Blt the portrait background
	BltVideoObject(FRAME_BUFFER, guiMugShotBorder, ubImage, usPosX, usPosY);

	SGPVObject* const face = guiAimFiFace[ubMercID];

	BOOLEAN                  shaded;
	ST::string text;
	MERCPROFILESTRUCT const& p = GetProfile(id);
	if (IsMercDead(p))
	{
		// the merc is dead, so shade the face red
		if (face->pShades[0] == nullptr)
		{
			face->pShades[0] = Create16BPPPaletteShaded(face->Palette(),
				DEAD_MERC_COLOR_RED, DEAD_MERC_COLOR_GREEN, DEAD_MERC_COLOR_BLUE, TRUE);
		}
		face->CurrentShade(0);
		shaded = FALSE;
		text   = AimFiText[AIM_FI_DEAD];
	}
	else if (p.bMercStatus == MERC_FIRED_AS_A_POW || (s && s->bAssignment == ASSIGNMENT_POW))
	{
		// the merc is currently a POW or, the merc was fired as a pow
		shaded = TRUE;
		text   = pPOWStrings[0];
	}
	else if (s != NULL)
	{
		// the merc is on our team
		shaded = TRUE;
		text   = MercInfo[MERC_FILES_ALREADY_HIRED];
	}
	else if (!IsMercHireable(p))
	{
		// the merc is away, shadow his/her face and blit 'away' over top
		shaded = TRUE;
		text   = AimFiText[AIM_FI_DEAD + 1];
	}
	else
	{
		shaded = FALSE;
		text.clear();
	}

	BltVideoObject(FRAME_BUFFER, face, 0, usPosX + AIM_FI_FACE_OFFSET, usPosY + AIM_FI_FACE_OFFSET);

	if (shaded)
	{
		FRAME_BUFFER->ShadowRect(usPosX + AIM_FI_FACE_OFFSET, usPosY + AIM_FI_FACE_OFFSET, usPosX + 48 + AIM_FI_FACE_OFFSET, usPosY + 43 + AIM_FI_FACE_OFFSET);
	}

	if (!text.empty())
	{
		DrawTextToScreen(text, usPosX + AIM_FI_AWAY_TEXT_OFFSET_X, usPosY + AIM_FI_AWAY_TEXT_OFFSET_Y, AIM_FI_AWAY_TEXT_OFFSET_WIDTH, FONT10ARIAL, 145, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
	}
}
