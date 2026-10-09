#ifndef __OVERHEADMAP_H
#define __OVERHEADMAP_H

#include "JA2Types.h"
#include "Types.h"
#include "World_Tileset_Enums.h"


void InitNewOverheadDB(TileSetID);
void RenderOverheadMap( INT16 sStartPointX_M, INT16 sStartPointY_M, INT16 sStartPointX_S, INT16 sStartPointY_S, INT16 sEndXS, INT16 sEndYS, BOOLEAN fFromMapUtility );


void HandleOverheadMap(void);
BOOLEAN InOverheadMap(void);
void GoIntoOverheadMap(void);
void KillOverheadMap(void);

void CalculateRestrictedMapCoords( INT8 bDirection, INT16 *psX1, INT16 *psY1, INT16 *psX2, INT16 *psY2, INT16 sEndXS, INT16 sEndYS );

void TrashOverheadMap(void);

GridNo GetOverheadMouseGridNo(void);

// Tactical placement in the 1:1 view: the overhead map's own coordinates
// (from its top left corner, the placement rules are written in them) to
// the tactical viewport's screen coordinates and back.
void OverheadToViewportXY(INT16 ox, INT16 oy, INT16* vx, INT16* vy);
void ViewportToOverheadXY(INT16 vx, INT16 vy, INT16* ox, INT16* oy);

extern BOOLEAN gfOverheadMapDirty;

#endif
