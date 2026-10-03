#ifndef SHADING_H
#define SHADING_H

#include "Types.h"

void BuildShadeTable(void);
void BuildIntensityTable(void);
void SetShadeTablePercent(float uiShadePercent);

extern UINT16 IntensityTable[65536];
extern UINT16 ShadeTable[65536];
extern UINT16 White16BPPPalette[256];

#define DEFAULT_SHADE_LEVEL 4


// The colour change of a 16 bit shade table made from a palette (such as
// Create16BPPPaletteShaded()), for full colour images, which have no palette
// (docs/png-images.md): add the bias (saturating), take the brightness if
// mono, scale by scale/256 and raise to at least min.
struct RGBAShade
{
	UINT8  biasR,  biasG,  biasB;
	UINT16 scaleR, scaleG, scaleB;
	UINT8  minR,   minG,   minB;
	bool   mono;
};

// The colours unchanged.
RGBAShade constexpr RGBA_SHADE_NONE{ 0, 0, 0, 256, 256, 256, 0, 0, 0, false };
// White16BPPPalette
RGBAShade constexpr RGBA_SHADE_WHITE{ 0, 0, 0, 256, 256, 256, 255, 255, 255, false };

// As Create16BPPPaletteShaded()
RGBAShade MakeRGBAShade(UINT32 rscale, UINT32 gscale, UINT32 bscale, bool mono);

// Changes the colour in place.
void ApplyRGBAShade(RGBAShade const&, UINT8& r, UINT8& g, UINT8& b);

#endif
