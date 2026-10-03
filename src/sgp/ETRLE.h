#pragma once

#include "Types.h"

#include <array>
#include <cstddef>
#include <vector>


// ETRLE is the run-length encoding of the subimages of indexed STCI images,
// and the pixel data format of 8 bit SGPVObjects. Each scanline is a series
// of runs ended by a 0 byte: a byte with bit 7 set skips (byte & 0x7F)
// transparent pixels, any other byte is followed by that many palette
// indices. A run covers at most 127 pixels and every scanline covers the
// whole width, trailing transparent pixels included.

// Which palette indices are transparent.
using ETRLETransparency = std::array<bool, 256>;

// Appends the ETRLE encoding of a width x height area of 8 bit pixels to out.
// pixels points to the top left pixel of the area, pitch is the distance in
// bytes between the starts of two rows. The output is identical to the one
// of Sir-Tech's ETRLECompress() and of tools/sti_editor.
void EncodeETRLE(UINT8 const* pixels, size_t pitch, UINT16 width, UINT16 height,
	ETRLETransparency const& transparent, std::vector<UINT8>& out);
