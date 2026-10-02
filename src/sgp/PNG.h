#pragma once

#include "Types.h"

#include <string_theory/string>

#include <cstddef>
#include <cstdint>
#include <vector>


// PNG decoding, independent of how the engine uses the image afterwards.
//
// Palettised PNGs (colour type 3) keep their palette indices unchanged, so
// palette index semantics used by the engine (0 = transparent, 254 = shadow
// or outline) survive the round trip. Their palette and tRNS chunks are read
// by our own code, only the zlib stream is inflated by stb_image.
//
// All other PNGs (grey, grey + alpha, RGB, RGBA; 1 to 16 bits per channel)
// are decoded by stb_image to 8 bit RGBA.
struct DecodedPNG
{
	enum class Kind
	{
		Indexed, // pixels holds one palette index per pixel
		RGBA     // pixels holds 4 bytes (R, G, B, A) per pixel
	};

	Kind   kind{};
	UINT16 width{};
	UINT16 height{};

	// Colour type and bit depth as stored in the file, for diagnostics.
	UINT8  sourceColourType{};
	UINT8  sourceBitDepth{};

	// Indexed only: the palette as stored in the file (1 to 256 entries).
	// The alpha value of each entry comes from the tRNS chunk, 255 if the
	// entry has none.
	std::vector<SGPPaletteEntry> palette;

	// Row-major, top to bottom, without padding.
	std::vector<UINT8> pixels;
};


// Decodes a PNG from memory. Throws std::runtime_error on invalid or
// unsupported data.
DecodedPNG DecodePNG(UINT8 const* data, size_t size);

// Reads a PNG game resource through the VFS and decodes it.
DecodedPNG DecodePNGFile(ST::string const& filename);


// A frame (subimage) of a PNG: a rectangle of the image, and the offset at
// which it is drawn relative to the blit position (like the subimages of an
// STCI file).
struct PNGFrame
{
	UINT16 x;
	UINT16 y;
	UINT16 width;
	UINT16 height;
	INT16  offsetX;
	INT16  offsetY;
	// How long the frame is shown in milliseconds, 0 if the metadata does not
	// say. Only animations that support it use it (see docs/png-images.md);
	// the others keep their own timing.
	UINT16 duration = 0;
};

// The metadata of a PNG (the contents of <image>.png.json).
struct PNGMetadata
{
	// frame durations are already resolved: "duration" of the frame, else
	// "frameDuration" of the image, else 0
	std::vector<PNGFrame> frames;
	// Full colour images only: draw an outline around the image where the
	// game asks for one (e.g. compatible items). Palettised images have their
	// outline in the image itself (index 254).
	bool outline = true;
	// "animation": { "framesPerDirection": N } – the frames are animations
	// of N frames each (one per direction); 0 if there is no such section.
	UINT8 framesPerDirection = 0;
};

// Parses the metadata of a PNG for an image of the given size. The frames
// are given in one of two forms:
//
//   { "frames": [ { "x": 0, "y": 0, "w": 32, "h": 24, "offsetX": -3, "offsetY": 0 }, ... ] }
//
//   { "grid": { "w": 32, "h": 24, "count": 10 }, "offsets": [ [ -3, 0 ], ... ] }
//
// or not at all, then the whole image is one frame. "offsetX"/"offsetY"
// default to 0. A grid is read left to right, top to bottom; "count" defaults
// to all cells and "offsets", if given, must have one entry per frame.
// "outline": false turns the outline off.
//
// "animation": { "framesPerDirection": N } (1 to 255) describes animated
// images loaded with application data (explosions and other tile cache
// animations, animated cursors), see ConvertIndexedPNGToImage().
//
// Frame durations in milliseconds (0 to 65535): "duration" in a frame of
// "frames", "durations": [ ... ] with one entry per frame next to "grid", and
// "frameDuration" for the frames without one of their own.
//
// Throws std::runtime_error on invalid metadata.
PNGMetadata ParsePNGMetadata(ST::string const& json, UINT16 imageWidth, UINT16 imageHeight);

// Converts a palettised PNG to an 8 bit SGPImage with one ETRLE compressed
// subimage per frame, like an indexed STCI image. fContents selects what is
// filled in (IMAGE_PALETTE, IMAGE_BITMAPDATA), as for the STCI loader.
// Transparent are palette index 0 (as in STCI files) and every index whose
// tRNS alpha is below 128; name is only used in messages.
//
// With IMAGE_APPDATA in fContents and framesPerDirection != 0, the image gets
// application data like an animated STCI image: one AuxObjectData per frame,
// the first frame of each animation (every framesPerDirection-th) with
// ubNumberOfFrames = framesPerDirection and AUX_ANIMATED_TILE, the others 0.
SGPImage* ConvertIndexedPNGToImage(DecodedPNG const& png, std::vector<PNGFrame> const& frames,
	UINT16 fContents, ST::string const& name, UINT8 framesPerDirection = 0);

// Converts a full colour PNG to a 32 bit SGPImage (IMAGE_RGBA): the frames
// are stored one after the other as RGBA rows, described by the ETRLEObjects.
// Without outline, the image gets IMAGE_NO_OUTLINE.
SGPImage* ConvertRGBAPNGToImage(DecodedPNG const& png, std::vector<PNGFrame> const& frames,
	UINT16 fContents, ST::string const& name, bool outline = true);

// Converts a PNG to an SGPImage with plain pixels for a video surface:
// - palettised: 8 bpp palette indices (unchanged, tRNS is ignored like the
//   transparency of PCX images) and the palette padded to 256 entries;
// - all others: 16 bpp in the current screen pixel format (gusRedMask etc.).
//   Pixels with alpha below 128 become 0x0000, the colour key of video
//   surfaces; opaque black becomes BLACK_SUBSTITUTE so it stays visible.
// fContents selects what is filled in (IMAGE_PALETTE, IMAGE_BITMAPDATA).
SGPImage* ConvertPNGToSurfaceImage(DecodedPNG const& png, UINT16 fContents);

// Loads a PNG game resource as an SGPImage. With IMAGE_FOR_SURFACE in
// fContents, see ConvertPNGToSurfaceImage(). Otherwise the metadata is read
// from <filename>.json if that exists (else the whole image is one frame) and
// the image is converted by ConvertIndexedPNGToImage() or, for full colour
// PNGs, ConvertRGBAPNGToImage() (not with IMAGE_NEEDS_PALETTE).
SGPImage* LoadPNGFileToImage(ST::string const& filename, UINT16 fContents);
