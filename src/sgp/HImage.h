#ifndef __IMAGE_H
#define __IMAGE_H

#include "Buffer.h"
#include "Types.h"
#include <memory>

// The HIMAGE module provides a common interface for managing image data. This module
// includes:
// - A set of data structures representing image data. Data can be 8 or 16 bpp and/or
//   compressed
// - A set of file loaders which load specific file formats into the internal data format
// - A set of blitters which blt the data to memory
// - A comprehensive automatic blitter which blits the appropriate type based on the
//   image header.


// Defines for buffer bit depth
#define BUFFER_8BPP							0x1
#define BUFFER_16BPP						0x2

// Defines for image charactoristics
#define IMAGE_TRLECOMPRESSED		0x0002
#define IMAGE_PALETTE						0x0004
#define IMAGE_BITMAPDATA				0x0008
#define IMAGE_APPDATA						0x0010
#define IMAGE_ALLIMAGEDATA			0x000C
#define IMAGE_ALLDATA						0x001C

// Additional content flag for CreateImage(): the image becomes a video
// surface, so it needs plain (not ETRLE compressed) pixels. Only loaders that
// can produce either form (PNG) look at it.
#define IMAGE_FOR_SURFACE				0x0020

// Additional content flag for CreateImage(): the caller uses the palette of
// the image (palette shading, palette colours, pixel values), so it must be an
// 8 bit palettised image; a full colour PNG is not loaded (and a PNG
// replacement of that kind is skipped).
#define IMAGE_NEEDS_PALETTE			0x0040

// SGPImage flag: the bitmap data holds the frames as 32 bit RGBA (8 bits per
// channel, in this byte order), one after the other; ubBitDepth is 32 and
// the ETRLEObjects give the byte offset, size and drawing offset of each frame.
// (Kept clear of the content flags above, which the PCX loader copies into
// fFlags.)
#define IMAGE_RGBA							0x0100

// SGPImage flag for IMAGE_RGBA images: no outline is drawn around the image
// (see SGPVObject::OutlineMask()).
#define IMAGE_NO_OUTLINE				0x0200

// This is the color substituted to keep a 24bpp -> 16bpp color
// from going transparent (0x0000) -- DB
#define BLACK_SUBSTITUTE	0x0001

#define AUX_FULL_TILE					0x01
#define AUX_ANIMATED_TILE			0x02
#define AUX_DYNAMIC_TILE			0x04
#define AUX_INTERACTIVE_TILE	0x08
#define AUX_IGNORES_HEIGHT		0x10
#define AUX_USES_LAND_Z				0x20

struct AuxObjectData
{
	UINT8		ubWallOrientation;
	UINT8		ubNumberOfTiles;
	UINT16	usTileLocIndex;
	UINT8		ubUnused1[3]; // XXX HACK000B
	UINT8		ubCurrentFrame;
	UINT8		ubNumberOfFrames;
	UINT8		fFlags;
	UINT8		ubUnused[6]; // XXX HACK000B
};

struct RelTileLoc
{
	INT8		bTileOffsetX;
	INT8		bTileOffsetY;
}; // relative tile location

// TRLE subimage structure, mirroring that of ST(C)I
struct ETRLEObject
{
	UINT32			uiDataOffset;
	UINT32			uiDataLength;
	INT16				sOffsetX;
	INT16				sOffsetY;
	UINT16			usHeight;
	UINT16			usWidth;
};


// Image header structure
struct SGPImage
{
	SGPImage(UINT16 const w, UINT16 const h, UINT8 const bpp) :
		usWidth(w),
		usHeight(h),
		ubBitDepth(bpp),
		fFlags(),
		uiAppDataSize(),
		uiSizePixData(),
		usNumberOfObjects()
	{}

	UINT16                       usWidth;
	UINT16                       usHeight;
	UINT8                        ubBitDepth;
	UINT16                       fFlags;
	SGP::Buffer<SGPPaletteEntry> pPalette;
	SGP::Buffer<UINT16>          pui16BPPPalette;
	SGP::Buffer<UINT8>           pAppData;
	UINT32                       uiAppDataSize;
	SGP::Buffer<UINT8>           pImageData;
	UINT32                       uiSizePixData;
	SGP::Buffer<ETRLEObject>     pETRLEObject;
	UINT16                       usNumberOfObjects;
};


#define SGPGetRValue(rgb)   ((BYTE) (rgb))
#define SGPGetBValue(rgb)   ((BYTE) ((rgb) >> 16))
#define SGPGetGValue(rgb)   ((BYTE) (((UINT16) (rgb)) >> 8))


SGPImage* CreateImage(const ST::string& ImageFile, UINT16 fContents);

// UTILITY FUNCTIONS

// Used to create a 16BPP Palette from an 8 bit palette, found in himage.c
UINT16* Create16BPPPaletteShaded(const SGPPaletteEntry* pPalette, UINT32 rscale, UINT32 gscale, UINT32 bscale, BOOLEAN mono);
UINT16* Create16BPPPalette(const SGPPaletteEntry* pPalette);
UINT16 Get16BPPColor( UINT32 RGBValue );
UINT32 GetRGBColor( UINT16 Value16BPP );

extern UINT16 gusRedMask;
extern UINT16 gusGreenMask;
extern UINT16 gusBlueMask;
extern INT16  gusRedShift;
extern INT16  gusBlueShift;
extern INT16  gusGreenShift;

// used to convert 565 RGB data into different bit-formats
void ConvertRGBDistribution565To555( UINT16 * p16BPPData, UINT32 uiNumberOfPixels );
void ConvertRGBDistribution565To655( UINT16 * p16BPPData, UINT32 uiNumberOfPixels );
void ConvertRGBDistribution565To556( UINT16 * p16BPPData, UINT32 uiNumberOfPixels );
void ConvertRGBDistribution565ToAny( UINT16 * p16BPPData, UINT32 uiNumberOfPixels );

typedef std::unique_ptr<SGPImage> AutoSGPImage;

#endif
