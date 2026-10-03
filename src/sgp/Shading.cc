#include "HImage.h"
#include "Shading.h"
#include "VObject.h"

#include <algorithm>
#include <iterator>

UINT16 IntensityTable[65536];
UINT16 ShadeTable[65536];
UINT16 White16BPPPalette[256];
static float guiShadePercent = 0.48f;


/* Builds a 16-bit color shading table. This function should be called only
 * after the current video adapter's pixel format is known (IE:
 * GetRgbDistribution() has been called, and the globals for masks and shifts
 * have been initialized by that function), and before any blitting is done.
 * Using the table is a straight lookup. The pixel to be shaded down is used as
 * the index into the table and the entry at that point will be a pixel that is
 * 25% darker.
 */
void BuildShadeTable(void)
{
	for (UINT16 red = 0; red < 256; red += 4)
	{
		for (UINT16 green = 0; green < 256; green += 4)
		{
			for (UINT16 blue = 0; blue < 256; blue += 4)
			{
				UINT16 index = Get16BPPColor(FROMRGB(red, green, blue));
				ShadeTable[index] = Get16BPPColor(FROMRGB(red * guiShadePercent, green * guiShadePercent, blue * guiShadePercent));
			}
		}
	}

	std::fill(std::begin(White16BPPPalette), std::end(White16BPPPalette), UINT16_MAX);
}


RGBAShade MakeRGBAShade(UINT32 const rscale, UINT32 const gscale, UINT32 const bscale, bool const mono)
{
	return RGBAShade{ 0, 0, 0,
		static_cast<UINT16>(rscale), static_cast<UINT16>(gscale), static_cast<UINT16>(bscale),
		0, 0, 0, mono };
}


void ApplyRGBAShade(RGBAShade const& s, UINT8& r, UINT8& g, UINT8& b)
{
	// The same steps as Create16BPPPaletteShaded() and AddSaturatePalette()
	UINT32 vr = std::min(r + s.biasR, 255);
	UINT32 vg = std::min(g + s.biasG, 255);
	UINT32 vb = std::min(b + s.biasB, 255);
	if (s.mono)
	{
		UINT32 const lumin = (vr * 299 + vg * 587 + vb * 114) / 1000;
		vr = vg = vb = lumin;
	}
	r = static_cast<UINT8>(std::min(std::max<UINT32>(s.scaleR * vr / 256, s.minR), 255U));
	g = static_cast<UINT8>(std::min(std::max<UINT32>(s.scaleG * vg / 256, s.minG), 255U));
	b = static_cast<UINT8>(std::min(std::max<UINT32>(s.scaleB * vb / 256, s.minB), 255U));
}


/* Builds a 16-bit color shading table. This function should be called only
 * after the current video adapter's pixel format is known (IE:
 * GetRgbDistribution() has been called, and the globals for masks and shifts
 * have been initialized by that function), and before any blitting is done.
 */
void BuildIntensityTable(void)
{
	const float dShadedPercent = 0.80f;

	for (UINT16 red = 0; red < 256; red += 4)
	{
		for (UINT16 green = 0; green < 256; green += 4)
		{
			for (UINT16 blue = 0; blue < 256; blue += 4)
			{
				UINT16 index = Get16BPPColor(FROMRGB(red, green, blue));
				IntensityTable[index] = Get16BPPColor(FROMRGB(red * dShadedPercent, green * dShadedPercent, blue * dShadedPercent));
			}
		}
	}
}


void SetShadeTablePercent(float uiShadePercent)
{
	guiShadePercent = uiShadePercent;
	BuildShadeTable();
}
