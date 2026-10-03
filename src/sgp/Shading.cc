#include "HImage.h"
#include "Shading.h"
#include "VObject.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <vector>

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


static UINT32 Brightness(UINT32 const r, UINT32 const g, UINT32 const b)
{
	// as Create16BPPPaletteShaded()
	return (r * 299 + g * 587 + b * 114) / 1000;
}


void BuildRGBARecolour(RGBARecolour& rc, SGPPaletteEntry const original[256], SGPPaletteEntry const changed[256], PaletteRange const* const ranges, size_t const rangeCount)
{
	std::copy_n(changed, 256, rc.palette);
	std::fill(std::begin(rc.range), std::end(rc.range), RGBARecolour::NO_RANGE);

	for (size_t i = 0; i != rangeCount && i != RGBARecolour::MAX_RANGES; ++i)
	{
		PaletteRange const& pr = ranges[i];
		if (pr.end < pr.start) continue;
		for (UINT32 idx = pr.start; idx <= pr.end; ++idx) rc.range[idx] = static_cast<UINT8>(i);

		// the brightness of the original colours of the range
		size_t const n = pr.end - pr.start + 1U;
		std::vector<INT32> lum(n);
		for (size_t k = 0; k != n; ++k)
		{
			SGPPaletteEntry const& c = original[pr.start + k];
			lum[k] = static_cast<INT32>(Brightness(c.r, c.g, c.b));
		}

		for (INT32 v = 0; v != 256; ++v)
		{
			UINT8* const out = rc.ramp[i][v];
			// between two neighbouring colours of the range: interpolated
			bool found = false;
			for (size_t k = 0; k + 1 < n && !found; ++k)
			{
				INT32 const a = lum[k];
				INT32 const b = lum[k + 1];
				if (v < std::min(a, b) || v > std::max(a, b)) continue;
				SGPPaletteEntry const& c0 = changed[pr.start + k];
				SGPPaletteEntry const& c1 = changed[pr.start + k + 1];
				// v is num/den of the way from colour k to colour k + 1
				INT32 const den = std::abs(b - a);
				INT32 const num = std::abs(v - a);
				auto const lerp = [&](INT32 const x0, INT32 const x1)
				{
					if (den == 0) return static_cast<UINT8>(x0);
					INT32 const d = (x1 - x0) * num;
					INT32 const step = (std::abs(d) * 2 + den) / (den * 2);
					return static_cast<UINT8>(d < 0 ? x0 - step : x0 + step);
				};
				out[0] = lerp(c0.r, c1.r);
				out[1] = lerp(c0.g, c1.g);
				out[2] = lerp(c0.b, c1.b);
				found = true;
			}
			if (found) continue;

			// outside the range: the nearest colour
			size_t best = 0;
			for (size_t k = 1; k != n; ++k)
			{
				if (std::abs(lum[k] - v) < std::abs(lum[best] - v)) best = k;
			}
			SGPPaletteEntry const& c = changed[pr.start + best];
			out[0] = c.r;
			out[1] = c.g;
			out[2] = c.b;
		}
	}
}


void ApplyRGBARecolour(RGBARecolour const& rc, UINT8 const maskIndex, UINT8& r, UINT8& g, UINT8& b)
{
	UINT8 const range = rc.range[maskIndex];
	if (range != RGBARecolour::NO_RANGE)
	{
		UINT8 const* const c = rc.ramp[range][Brightness(r, g, b)];
		r = c[0];
		g = c[1];
		b = c[2];
	}
	else
	{
		SGPPaletteEntry const& c = rc.palette[maskIndex];
		r = c.r;
		g = c.g;
		b = c.b;
	}
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
