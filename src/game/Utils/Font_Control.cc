#include "Directories.h"
#include "Font.h"
#include "HImage.h"
#include "VObject.h"
#include "VSurface.h"
#include "Font_Control.h"
#include "GameRes.h"
#include "GameMode.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "Logger.h"
#include "UILayout.h"
#include "JAScreens.h"
#include "ScreenIDs.h"


SGPFont gp10PointArial;
SGPFont gp10PointArialBold;
SGPFont gp12PointArial;
SGPFont gp12PointArialFixedFont;
SGPFont gp12PointFont1;
SGPFont gp14PointArial;
SGPFont gp14PointHumanist;
SGPFont gp16PointArial;
SGPFont gpBlockFontNarrow;
SGPFont gpBlockyFont;
SGPFont gpBlockyFont2;
SGPFont gpMapFont;
SGPFont gpGridFont;
SGPFont gpSectorInvFont;
SGPFont gpMapInvBigFont;
SGPFont gpMapInvBigCountFont;
SGPFont gpSecInvBigCountFont;
SGPFont gpCharInfoFont;
SGPFont gpStrategicGeneralFont;
SGPFont gpStrategicTooltipFont;
SGPFont gpCompFont;
SGPFont gpLargeFontType1;
SGPFont gpSmallCompFont;
SGPFont gpTextInventoryFont;
SGPFont gpValueInventoryFont;
SGPFont gpSmallFontType1;
SGPFont gpTinyFontType1;

SGPFont gpHugeFont;


static void CreateFontPaletteTables(SGPFont);


// An optional font drawn in the colours of `colours` (see InitializeFonts()):
// nullptr, logged, when the file (or the PNG next to it) is missing or
// broken. `colours` must be loaded already.
static SGPFont LoadOptionalFontInColoursOf(char const* const file, SGPFont const colours)
{
	if (GCM->getPNGReplacement(file).empty() && !GCM->doesGameResExists(file)) return nullptr;
	try
	{
		SGPFont const font = LoadFontFile(file);
		font->ReplacePalette(colours->Palette());
		CreateFontPaletteTables(font);
		return font;
	}
	catch (std::exception const& e)
	{
		SLOGE("Cannot use {}, keeping the original font: {}", file, e.what());
		return nullptr;
	}
}


SGPFont StrategicGeneralFont()
{
	return g_ui.isExtraWideStrategicScreen() && gpStrategicGeneralFont ? gpStrategicGeneralFont : gpBlockyFont2;
}


TooltipFonts GetTooltipFonts()
{
	bool const strategic = guiCurrentScreen == MAP_SCREEN || guiCurrentScreen == AUTORESOLVE_SCREEN;
	if (g_ui.isExtraWideStrategicScreen() && strategic && gpStrategicTooltipFont)
	{
		return { gpStrategicTooltipFont, gp14PointArial, true };
	}
	return { gp10PointArial, gp10PointArialBold, false };
}


void InitializeFonts(void)
{
#define M(var, file) (CreateFontPaletteTables((var) = LoadFontFile((file))))
	M(gp10PointArial,          FONTSDIR "/font10arial.sti");
	M(gp10PointArialBold,      FONTSDIR "/font10arialbold.sti");
	M(gp12PointArial,          FONTSDIR "/font12arial.sti");
	M(gp12PointArialFixedFont, FONTSDIR "/font12arialfixedwidth.sti");
	M(gp12PointFont1,          FONTSDIR "/font12point1.sti");
	M(gp14PointArial,          FONTSDIR "/font14arial.sti");
	M(gp14PointHumanist,       FONTSDIR "/font14humanist.sti");
	M(gp16PointArial,          FONTSDIR "/font16arial.sti");
	M(gpBlockFontNarrow,       FONTSDIR "/blockfontnarrow.sti");
	M(gpBlockyFont,            FONTSDIR "/blockfont.sti");
	M(gpBlockyFont2,           FONTSDIR "/blockfont2.sti");
	M(gpMapFont,               FONTSDIR "/font_map.sti");
	M(gpGridFont,              FONTSDIR "/font_grid.sti");
	M(gpSectorInvFont,         FONTSDIR "/font_sector_inv.sti");
	M(gpMapInvBigFont,         FONTSDIR "/font_mapinv_big.sti");
	M(gpMapInvBigCountFont,    FONTSDIR "/font_mapinv_big_count.sti");
	M(gpSecInvBigCountFont,    FONTSDIR "/font_sec_inv_big_count.sti");
	M(gpCompFont,              FONTSDIR "/compfont.sti");
	M(gpLargeFontType1,        FONTSDIR "/largefont1.sti");
	M(gpSmallCompFont,         FONTSDIR "/smallcompfont.sti");
	M(gpTextInventoryFont,     FONTSDIR "/FONT_TEXT_Inventory.STI");
	M(gpValueInventoryFont,    FONTSDIR "/FONT_VALUE_Inventory.STI");
	M(gpSmallFontType1,        FONTSDIR "/smallfont1.sti");
	M(gpTinyFontType1,         FONTSDIR "/tinyfont1.sti");

	// Optional: only the 1366x768 interface uses them, and it keeps the
	// original font (BLOCKFONT2, FONT10ARIAL for the tooltips) when the file
	// (or the PNG next to it) is missing or broken. Drawn in the original
	// font's colours: the text colours are indices into the font's own
	// palette, so its palette is replaced with the original's -- only the
	// glyph shapes come from the file.
	gpCharInfoFont         = LoadOptionalFontInColoursOf(FONTSDIR "/font_charinfo.sti",          gpBlockyFont2);
	gpStrategicGeneralFont = LoadOptionalFontInColoursOf(FONTSDIR "/font_strategic_general.sti", gpBlockyFont2);
	gpStrategicTooltipFont = LoadOptionalFontInColoursOf(FONTSDIR "/font_strategic_tooltip.sti", gp10PointArial);

	if(GameMode::getInstance()->isEditorMode() && isEnglishVersion())
	{
		M(gpHugeFont, FONTSDIR "/hugefont.sti");
	}
#undef M

	// Set default for font system
	SetFontDestBuffer(FRAME_BUFFER);
}


// Set shades for fonts
void SetFontShade(SGPFont const font, FontShade const shade)
{
	font->CurrentShade(shade);
}


static void CreateFontPaletteTables(SGPFont const f)
{
	const SGPPaletteEntry* const pal = f->Palette();
	f->pShades[FONT_SHADE_RED]     = Create16BPPPaletteShaded(pal, 255,   0,   0, TRUE);
	f->pShades[FONT_SHADE_BLUE]    = Create16BPPPaletteShaded(pal,   0,   0, 255, TRUE);
	f->pShades[FONT_SHADE_GREEN]   = Create16BPPPaletteShaded(pal,   0, 255,   0, TRUE);
	f->pShades[FONT_SHADE_YELLOW]  = Create16BPPPaletteShaded(pal, 255, 255,   0, TRUE);
	f->pShades[FONT_SHADE_NEUTRAL] = Create16BPPPaletteShaded(pal, 255, 255, 255, FALSE);
	f->pShades[FONT_SHADE_WHITE]   = Create16BPPPaletteShaded(pal, 255, 255, 255, TRUE);

	// the rest are darkening tables, right down to all-black.
	f->pShades[ 0] = Create16BPPPaletteShaded(pal, 165, 165, 165, FALSE);
	f->pShades[ 7] = Create16BPPPaletteShaded(pal, 135, 135, 135, FALSE);
	f->pShades[ 8] = Create16BPPPaletteShaded(pal, 105, 105, 105, FALSE);
	f->pShades[ 9] = Create16BPPPaletteShaded(pal,  75,  75,  75, FALSE);
	f->pShades[10] = Create16BPPPaletteShaded(pal,  45,  45,  45, FALSE);
	f->pShades[11] = Create16BPPPaletteShaded(pal,  36,  36,  36, FALSE);
	f->pShades[12] = Create16BPPPaletteShaded(pal,  27,  27,  27, FALSE);
	f->pShades[13] = Create16BPPPaletteShaded(pal,  18,  18,  18, FALSE);
	f->pShades[14] = Create16BPPPaletteShaded(pal,   9,   9,   9, FALSE);
	f->pShades[15] = Create16BPPPaletteShaded(pal,   0,   0,   0, FALSE);

	// Set current shade table to neutral color
	f->CurrentShade(FONT_SHADE_NEUTRAL);
}
