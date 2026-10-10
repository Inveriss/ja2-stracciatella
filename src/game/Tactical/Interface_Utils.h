#ifndef __INTERFACE_UTILS_H
#define __INTERFACE_UTILS_H

#include "JA2Types.h"


#define DRAW_ITEM_STATUS_ATTACHMENT1	200
#define DRAW_ITEM_STATUS_ATTACHMENT2	201
#define DRAW_ITEM_STATUS_ATTACHMENT3	202
#define DRAW_ITEM_STATUS_ATTACHMENT4	203

void DrawSoldierUIBars(SOLDIERTYPE const&, INT16 sXPos, INT16 sYPos, BOOLEAN fErase, SGPVSurface* buffer);

// The 1366x768 character info panel's tall bars: life, breath and morale,
// `width` (1-6) px wide and `height` px tall each, in their own windows
// (left columns life_x/breath_x/morale_x, top row top_y), which are black in
// the saved background -- erased from it, then drawn into `buffer`.
void DrawSoldierUIBarsTall(SOLDIERTYPE const&, INT16 life_x, INT16 breath_x, INT16 morale_x, INT16 top_y, INT16 width, INT16 height, SGPVSurface* buffer);
// Only the tall life bar (life, bandaged, bleeding) of the above, in the same
// colours; draws over what is there, nothing is restored first.
void DrawSoldierLifeBarTall(SOLDIERTYPE const&, INT16 x, INT16 top_y, INT16 width, INT16 height, SGPVSurface* buffer);

// sWidth: bar width in pixels; the last column uses sColor2 (shadow), the
// rest sColor1. The default 2 is the original one-line-plus-shadow bar.
void DrawItemUIBarEx(OBJECTTYPE const&, UINT8 ubStatus, INT16 sXPos, INT16 sYPos, INT16 sHeight, INT16 sColor1, INT16 sColor2, SGPVSurface* buffer, INT16 sWidth = 2);

void RenderSoldierFace(SOLDIERTYPE const&, INT16 sFaceX, INT16 sFaceY);

// get rid of the loaded portraits for cars
void UnLoadCarPortraits( void );

void DeleteInterfaceUtilsGraphics();

#endif
