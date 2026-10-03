#ifndef _TILE_SURFACE_H
#define _TILE_SURFACE_H

#include "TileDat.h"
#include <string_theory/string>
struct TILE_IMAGERY;

extern TILE_IMAGERY* gTileSurfaceArray[NUMBEROFTILETYPES];


// pngAnimation: a palettised PNG with an "animation" section may replace the
// image (see IMAGE_ANIMATION_METADATA); only for tile cache animations, not
// for tilesets.
// needsPalette: see IMAGE_NEEDS_PALETTE; colourMask: see IMAGE_COLOUR_MASK.
TILE_IMAGERY* LoadTileSurface(ST::string const& cFilename, bool pngAnimation = false, bool needsPalette = false, bool colourMask = false);

void DeleteTileSurface(TILE_IMAGERY* pTileSurf);

void SetRaisedObjectFlag(ST::string const& filename, TILE_IMAGERY*);

#endif
