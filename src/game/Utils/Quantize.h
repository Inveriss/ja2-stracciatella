#ifndef QUANTIZE_H
#define QUANTIZE_H

#include "Types.h"


// sMaxColors: upper bound on distinct colors written to the palette/index
// buffer (indices 0..sMaxColors-1 may be used; default 255, the format's
// own limit of 256 minus nothing reserved). Every index at or above
// sMaxColors is then guaranteed unused by both the palette and every pixel,
// regardless of how many distinct colors the source image actually has --
// pass a lower value here to reserve one or more high indices for the
// caller's own use (e.g. MapUtility.cc reserves one to relocate whatever
// real color the quantizer put at index 0 to, working around index 0's
// unrelated meaning as the .sti format's own transparency marker).
void QuantizeImage(UINT8* pDest, const SGPPaletteEntry* pSrc, INT16 sWidth, INT16 sHeight, SGPPaletteEntry* pPalette, INT16 sMaxColors = 255);

#endif
