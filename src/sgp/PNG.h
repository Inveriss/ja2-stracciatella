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
