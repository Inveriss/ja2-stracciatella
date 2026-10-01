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
};

// Parses the frame metadata of a PNG (the contents of <image>.png.json) for
// an image of the given size. Two forms are accepted:
//
//   { "frames": [ { "x": 0, "y": 0, "w": 32, "h": 24, "offsetX": -3, "offsetY": 0 }, ... ] }
//
//   { "grid": { "w": 32, "h": 24, "count": 10 }, "offsets": [ [ -3, 0 ], ... ] }
//
// "offsetX"/"offsetY" default to 0. A grid is read left to right, top to
// bottom; "count" defaults to all cells and "offsets", if given, must have
// one entry per frame. Throws std::runtime_error on invalid metadata.
std::vector<PNGFrame> ParsePNGFrames(ST::string const& json, UINT16 imageWidth, UINT16 imageHeight);

// Converts a palettised PNG to an 8 bit SGPImage with one ETRLE compressed
// subimage per frame, like an indexed STCI image. fContents selects what is
// filled in (IMAGE_PALETTE, IMAGE_BITMAPDATA), as for the STCI loader.
// Transparent are palette index 0 (as in STCI files) and every index whose
// tRNS alpha is below 128; name is only used in messages.
SGPImage* ConvertIndexedPNGToImage(DecodedPNG const& png, std::vector<PNGFrame> const& frames,
	UINT16 fContents, ST::string const& name);

// Converts a PNG to an SGPImage with plain pixels for a video surface:
// - palettised: 8 bpp palette indices (unchanged, tRNS is ignored like the
//   transparency of PCX images) and the palette padded to 256 entries;
// - all others: 16 bpp in the current screen pixel format (gusRedMask etc.).
//   Pixels with alpha below 128 become 0x0000, the colour key of video
//   surfaces; opaque black becomes BLACK_SUBSTITUTE so it stays visible.
// fContents selects what is filled in (IMAGE_PALETTE, IMAGE_BITMAPDATA).
SGPImage* ConvertPNGToSurfaceImage(DecodedPNG const& png, UINT16 fContents);

// Loads a PNG game resource as an SGPImage. With IMAGE_FOR_SURFACE in
// fContents, see ConvertPNGToSurfaceImage(). Otherwise frame metadata is read
// from <filename>.json if that exists (else the whole image is one frame) and
// the image is converted by ConvertIndexedPNGToImage(); only palettised PNGs
// are supported there so far.
SGPImage* LoadPNGFileToImage(ST::string const& filename, UINT16 fContents);
