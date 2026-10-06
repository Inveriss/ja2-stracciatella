#include "Interface_Utils.h"
#include "ContentManager.h"
#include "Directories.h"
#include "Faces.h"
#include "GameInstance.h"
#include "HImage.h"
#include "JAScreens.h"
#include "Line.h"
#include "MagazineModel.h"
#include "Object_Cache.h"
#include "Overhead.h"
#include "Render_Dirty.h"
#include "Soldier_Macros.h"
#include "SysUtil.h"
#include "UILayout.h"
#include "Vehicles.h"
#include "Video.h"
#include "VObject.h"
#include "VSurface.h"
#include <algorithm>
#include <stdexcept>
#include <string_theory/format>
#include <string_theory/string>


#define LIFE_BAR_SHADOW		FROMRGB(108, 12, 12)
#define LIFE_BAR			FROMRGB(200, 0, 0)
#define BANDAGE_BAR_SHADOW		FROMRGB(156, 60, 60)
#define BANDAGE_BAR			FROMRGB(222, 132, 132)
#define BLEEDING_BAR_SHADOW		FROMRGB(128, 128, 60)
#define BLEEDING_BAR			FROMRGB(240,  240, 20)
#define CURR_BREATH_BAR_SHADOW		FROMRGB(8, 12, 118) // the MAX max breatth, always at 100%
#define CURR_BREATH_BAR		FROMRGB(8, 12, 160)
#define CURR_MAX_BREATH		FROMRGB(0, 0, 0) // the current max breath, black
#define CURR_MAX_BREATH_SHADOW		FROMRGB(0, 0, 0)
#define MORALE_BAR_SHADOW		FROMRGB(8, 112, 12)
#define MORALE_BAR			FROMRGB(8, 180, 12)
#define BREATH_BAR_SHADOW		FROMRGB(60, 108, 108) // the lt blue current breath
#define BREATH_BAR			FROMRGB(113, 178, 218)
#define BREATH_BAR_SHAD_BACK		FROMRGB(1, 1, 1)
#define FACE_WIDTH			48
#define FACE_HEIGHT			43

namespace {
// the ids for the car portraits
cache_key_t const giCarPortraits[]
{
	INTERFACEDIR "/eldorado.sti",
	INTERFACEDIR "/hummer.sti",
	INTERFACEDIR "/ice cream truck.sti",
	INTERFACEDIR "/jeep.sti"
};

// backgrounds for breath max background
cache_key_t const guiBrownBackgroundForTeamPanel{ INTERFACEDIR "/bars.sti" };
}


// get rid of the images we loaded for the mapscreen car portraits
void UnLoadCarPortraits(void)
{
	for (auto * const portraitName : giCarPortraits)
	{
		RemoveVObject(portraitName);
	}
}


static void DrawBar(UINT32 const XPos, UINT32 const YPos, UINT32 const Height, UINT16 const Color, UINT16 const ShadowColor, UINT16* const DestBuf)
{
	LineDraw(TRUE, XPos + 0, YPos, XPos + 0, YPos - Height, ShadowColor, DestBuf);
	LineDraw(TRUE, XPos + 1, YPos, XPos + 1, YPos - Height, Color,       DestBuf);
	LineDraw(TRUE, XPos + 2, YPos, XPos + 2, YPos - Height, ShadowColor, DestBuf);
}


static void DrawLifeUIBar(SOLDIERTYPE const& s, UINT32 const XPos, UINT32 YPos, UINT32 const MaxHeight, UINT16* const pDestBuf)
{
	UINT32 Height;

	// FIRST DO MAX LIFE
	Height = MaxHeight * s.bLife / 100;
	DrawBar(XPos, YPos, Height, Get16BPPColor(LIFE_BAR), Get16BPPColor(LIFE_BAR_SHADOW), pDestBuf);

	// NOW DO BANDAGE
	// Calculate bandage
	UINT32 Bandage = s.bLifeMax - s.bLife - s.bBleeding;
	if (Bandage != 0)
	{
		YPos   -= Height;
		Height  = MaxHeight * Bandage / 100;
		DrawBar(XPos, YPos, Height, Get16BPPColor(BANDAGE_BAR), Get16BPPColor(BANDAGE_BAR_SHADOW), pDestBuf);
	}

	// NOW DO BLEEDING
	if (s.bBleeding != 0)
	{
		YPos   -= Height;
		Height  = MaxHeight * s.bBleeding / 100;
		DrawBar(XPos, YPos, Height, Get16BPPColor(BLEEDING_BAR), Get16BPPColor(BLEEDING_BAR_SHADOW), pDestBuf);
	}
}


static void DrawBreathUIBar(SOLDIERTYPE const& s, UINT32 const XPos, UINT32 const sYPos, UINT32 const MaxHeight, UINT16* const pDestBuf)
{
	UINT32 Height;

	if (s.bBreathMax <= 97)
	{
		Height = MaxHeight * (s.bBreathMax + 3) / 100;
		// the old background colors for breath max diff
		DrawBar(XPos, sYPos, Height, Get16BPPColor(BREATH_BAR_SHAD_BACK), Get16BPPColor(BREATH_BAR_SHAD_BACK), pDestBuf);
	}

	Height = MaxHeight * s.bBreathMax / 100;
	DrawBar(XPos, sYPos, Height, Get16BPPColor(CURR_MAX_BREATH), Get16BPPColor(CURR_MAX_BREATH_SHADOW), pDestBuf);

	// NOW DO BREATH
	Height = MaxHeight * s.bBreath / 100;
	DrawBar(XPos, sYPos, Height, Get16BPPColor(CURR_BREATH_BAR), Get16BPPColor(CURR_BREATH_BAR_SHADOW), pDestBuf);
}


static void DrawMoraleUIBar(SOLDIERTYPE const& s, UINT32 const XPos, UINT32 const YPos, UINT32 const MaxHeight, UINT16* const pDestBuf)
{
	UINT32 const Height = MaxHeight * s.bMorale / 100;
	DrawBar(XPos, YPos, Height, Get16BPPColor(MORALE_BAR), Get16BPPColor(MORALE_BAR_SHADOW), pDestBuf);
}


void DrawSoldierUIBars(SOLDIERTYPE const& s, INT16 const sXPos, INT16 const sYPos, BOOLEAN const fErase, SGPVSurface* const uiBuffer)
{
	const UINT32 BarWidth  =  3;
	const UINT32 BarHeight = 42;
	const UINT32 BreathOff =  6;
	const UINT32 MoraleOff = 12;

	// Erase what was there
	if (fErase)
	{
		RestoreExternBackgroundRect(sXPos, sYPos - BarHeight, MoraleOff + BarWidth, BarHeight + 1);
	}

	if (s.bLife == 0) return;

	if (!(s.uiStatusFlags & SOLDIER_ROBOT))
	{
		// DO MAX BREATH
		// brown guy
		UINT16 Region;
		if (guiCurrentScreen != MAP_SCREEN &&
			GetSelectedMan() == &s &&
			gTacticalStatus.ubCurrentTeam == OUR_TEAM &&
			OK_INTERRUPT_MERC(&s))
		{
			Region = 1; // gold, the second entry in the .sti
		}
		else
		{
			Region = 0; // brown, first entry
		}
		BltVideoObject(uiBuffer, guiBrownBackgroundForTeamPanel, Region, sXPos + BreathOff, sYPos - BarHeight);
	}

	SGPVSurface::Lock l(uiBuffer);
	SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	UINT16* const pDestBuf = l.Buffer<UINT16>();

	DrawLifeUIBar(s, sXPos, sYPos, BarHeight, pDestBuf);
	if (!(s.uiStatusFlags & SOLDIER_ROBOT))
	{
		DrawBreathUIBar(s, sXPos + BreathOff, sYPos, BarHeight, pDestBuf);
		if (!(s.uiStatusFlags & SOLDIER_VEHICLE))
		{
			DrawMoraleUIBar(s, sXPos + MoraleOff, sYPos, BarHeight, pDestBuf);
		}
	}
}


// Tall 5 px bars (1366x768 character info panel): one colour per column,
// left to right; the life, breath and morale ones are the user's design
// (bars image, columns dark-mid-light-mid-dark), the others are made the
// same way from the 3 px bars' shadow and colour above.
static UINT32 const TALL_LIFE_BAR[5]     = { FROMRGB(175,   0,   0), FROMRGB(186,   0,   0), FROMRGB(205,   0,   0), FROMRGB(186,   0,   0), FROMRGB(175,   0,   0) };
static UINT32 const TALL_BANDAGE_BAR[5]  = { BANDAGE_BAR_SHADOW, FROMRGB(189,  96,  96), BANDAGE_BAR, FROMRGB(189,  96,  96), BANDAGE_BAR_SHADOW };
static UINT32 const TALL_BLEEDING_BAR[5] = { BLEEDING_BAR_SHADOW, FROMRGB(184, 184,  40), BLEEDING_BAR, FROMRGB(184, 184,  40), BLEEDING_BAR_SHADOW };
static UINT32 const TALL_BREATH_BAR[5]   = { FROMRGB(  8,  12, 110), FROMRGB(  8,  12, 128), FROMRGB(  8,  12, 159), FROMRGB(  8,  12, 128), FROMRGB(  8,  12, 110) };
static UINT32 const TALL_MAX_BREATH[5]   = { CURR_MAX_BREATH, CURR_MAX_BREATH, CURR_MAX_BREATH, CURR_MAX_BREATH, CURR_MAX_BREATH };
static UINT32 const TALL_BREATH_BACK[5]  = { BREATH_BAR_SHAD_BACK, BREATH_BAR_SHAD_BACK, BREATH_BAR_SHAD_BACK, BREATH_BAR_SHAD_BACK, BREATH_BAR_SHAD_BACK };
static UINT32 const TALL_MORALE_BAR[5]   = { FROMRGB(  8, 134,   8), FROMRGB(  8, 154,   8), FROMRGB(  8, 174,   8), FROMRGB(  8, 154,   8), FROMRGB(  8, 134,   8) };


// One segment of a tall bar: `height` rows ending at row `bottom`
// (inclusive), `width` columns from `x`, coloured with the middle `width`
// of the 5 column colours (all 5 for a 5 px bar; a 3 px one drops the two
// outer ones). Returns the row above it, where the next segment starts.
static INT16 DrawTallBarSegment(SGPVSurface* const buffer, INT16 const x, INT16 const bottom, INT16 const width, INT16 const height, UINT32 const (&colours)[5])
{
	if (height <= 0) return bottom;
	INT16 const top   = bottom - height + 1;
	INT16 const first = (5 - width) / 2;
	for (INT16 i = 0; i != width; ++i)
	{
		ColorFillVideoSurfaceArea(buffer, x + i, top, x + i + 1, bottom + 1, Get16BPPColor(colours[first + i]));
	}
	return top - 1;
}


void DrawSoldierUIBarsTall(SOLDIERTYPE const& s, INT16 const life_x, INT16 const breath_x, INT16 const morale_x, INT16 const top_y, INT16 const width_in, INT16 const height, SGPVSurface* const buffer)
{
	INT16 const width = std::clamp<INT16>(width_in, 1, 5);

	// Erase what was there: the windows are black in the saved background.
	for (INT16 const x : { life_x, breath_x, morale_x })
	{
		RestoreExternBackgroundRect(x, top_y, width, height);
	}

	if (s.bLife == 0) return;

	INT16 const bottom = top_y + height - 1;
	auto const rows = [height](INT32 const percent) { return static_cast<INT16>(height * percent / 100); };

	// life, then bandaged and bleeding above it -- as DrawLifeUIBar()
	INT16 y = DrawTallBarSegment(buffer, life_x, bottom, width, rows(s.bLife), TALL_LIFE_BAR);
	INT32 const bandage = s.bLifeMax - s.bLife - s.bBleeding;
	y = DrawTallBarSegment(buffer, life_x, y, width, rows(bandage), TALL_BANDAGE_BAR);
	DrawTallBarSegment(buffer, life_x, y, width, rows(s.bBleeding), TALL_BLEEDING_BAR);

	if (s.uiStatusFlags & SOLDIER_ROBOT) return;

	// breath -- as DrawBreathUIBar(): the old max, the current max, then breath
	if (s.bBreathMax <= 97)
	{
		DrawTallBarSegment(buffer, breath_x, bottom, width, rows(s.bBreathMax + 3), TALL_BREATH_BACK);
	}
	DrawTallBarSegment(buffer, breath_x, bottom, width, rows(s.bBreathMax), TALL_MAX_BREATH);
	DrawTallBarSegment(buffer, breath_x, bottom, width, rows(s.bBreath), TALL_BREATH_BAR);

	if (s.uiStatusFlags & SOLDIER_VEHICLE) return;

	DrawTallBarSegment(buffer, morale_x, bottom, width, rows(s.bMorale), TALL_MORALE_BAR);
}


void DrawItemUIBarEx(OBJECTTYPE const& o, const UINT8 ubStatus, const INT16 x, const INT16 y, INT16 max_h, const INT16 sColor1, const INT16 sColor2, SGPVSurface* const uiBuffer, INT16 width)
{
	if (width < 1) width = 1;
	INT16 value;
	// Adjust for ammo, other things
	const ItemModel * item = GCM->getItem(o.usItem);
	if (ubStatus >= DRAW_ITEM_STATUS_ATTACHMENT1)
	{
		value = o.bAttachStatus[ubStatus - DRAW_ITEM_STATUS_ATTACHMENT1];
	}
	else if (item->isKey())
	{
		value = 100;
	}
	else
	{
		if (ubStatus >= MAX_OBJECTS_PER_SLOT) 
			throw std::runtime_error(ST::format("invalid ubStatus value: {}", ubStatus).to_std_string());
		
		if (item->isAmmo())
		{
			value = 100 * o.ubShotsLeft[ubStatus] / (item->asAmmo()->capacity ? item->asAmmo()->capacity : 1);
			if (value > 100) value = 100;
		}
		else
		{
			value = o.bStatus[ubStatus];
		}
	}

	{ SGPVSurface::Lock l(uiBuffer);
		SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		UINT16* const pDestBuf = l.Buffer<UINT16>();

		--max_h; // LineDraw() includes the end point
		const INT h = max_h * value / 100;
		// All columns but the last in the main colour, the last one in the
		// shadow colour (width 2 == the original two LineDraw() calls); a
		// 1px bar has no shadow column.
		for (INT16 col = 0; col < width; ++col)
		{
			const INT16 colour = (col == width - 1 && width > 1) ? sColor2 : sColor1;
			LineDraw(TRUE, x + col, y, x + col, y - h, colour, pDestBuf);
		}
	}

	if (uiBuffer == guiSAVEBUFFER)
	{
		RestoreExternBackgroundRect(x, y - max_h, width, max_h + 1);
	}
	else
	{
		InvalidateRegion(x, y - max_h, x + width, y + 1);
	}
}


void RenderSoldierFace(SOLDIERTYPE const& s, INT16 const sFaceX, INT16 const sFaceY)
{
	if (s.uiStatusFlags & SOLDIER_VEHICLE)
	{
		// just draw the vehicle
		const UINT8 vehicle_type = pVehicleList[s.bVehicleID].ubVehicleType;
		BltVideoObject(guiSAVEBUFFER, giCarPortraits[vehicle_type], 0, sFaceX, sFaceY);
		RestoreExternBackgroundRect(sFaceX, sFaceY, FACE_WIDTH, FACE_HEIGHT);
	}
	else if (s.face->uiFlags & FACE_INACTIVE_HANDLED_ELSEWHERE) // OK, check if this face actually went active
	{
		ExternRenderFace(guiSAVEBUFFER, *s.face, sFaceX, sFaceY);
	}
	else
	{
		SetAutoFaceActive(FRAME_BUFFER, guiSAVEBUFFER, *s.face, sFaceX, sFaceY);
		RenderAutoFace(*s.face);
	}
}


void DeleteInterfaceUtilsGraphics()
{
	RemoveVObject(guiBrownBackgroundForTeamPanel);
}
