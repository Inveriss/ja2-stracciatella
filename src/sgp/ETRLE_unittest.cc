#include "gtest/gtest.h"

#include "ETRLE.h"

#include <vector>

namespace
{

ETRLETransparency IndexZeroTransparent()
{
	ETRLETransparency t{};
	t[0] = true;
	return t;
}

std::vector<UINT8> Encode(std::vector<UINT8> const& pixels, UINT16 const width, UINT16 const height,
	ETRLETransparency const& transparent = IndexZeroTransparent())
{
	std::vector<UINT8> out;
	EncodeETRLE(pixels.data(), width, width, height, transparent, out);
	return out;
}

// Reference decoder; transparent pixels come out as -1.
std::vector<int> Decode(std::vector<UINT8> const& data, UINT16 const width, UINT16 const height)
{
	std::vector<int> pixels;
	size_t pos = 0;
	for (UINT16 y = 0; y != height; ++y)
	{
		size_t const rowStart = pixels.size();
		for (;;)
		{
			EXPECT_LT(pos, data.size());
			if (pos >= data.size()) return pixels;
			UINT8 const b = data[pos++];
			if (b == 0) break;
			EXPECT_LE(b & 0x7F, 127);
			if (b & 0x80)
			{
				pixels.insert(pixels.end(), b & 0x7F, -1);
			}
			else
			{
				for (UINT8 i = 0; i != b; ++i) pixels.push_back(data[pos++]);
			}
		}
		EXPECT_EQ(pixels.size() - rowStart, width) << "row " << y;
	}
	EXPECT_EQ(pos, data.size());
	return pixels;
}

}


TEST(ETRLE, singlePixels)
{
	EXPECT_EQ(Encode({ 5 }, 1, 1), (std::vector<UINT8>{ 1, 5, 0 }));
	EXPECT_EQ(Encode({ 0 }, 1, 1), (std::vector<UINT8>{ 0x81, 0 }));
}


TEST(ETRLE, mixedRunsAndRows)
{
	// row 0: 2 transparent, 3 opaque (incl. 254 and 255), 1 transparent
	// row 1: all transparent
	std::vector<UINT8> const pixels{ 0, 0, 7, 254, 255, 0,
	                                 0, 0, 0, 0,   0,   0 };
	EXPECT_EQ(Encode(pixels, 6, 2),
		(std::vector<UINT8>{ 0x82, 3, 7, 254, 255, 0x81, 0,
		                     0x86, 0 }));
}


TEST(ETRLE, runsAreSplitAt127Pixels)
{
	for (UINT16 const width : { UINT16{127}, UINT16{128}, UINT16{300} })
	{
		SCOPED_TRACE(width);
		std::vector<UINT8> opaque(width, 9);
		std::vector<UINT8> const encodedOpaque = Encode(opaque, width, 1);
		std::vector<int> const decodedOpaque = Decode(encodedOpaque, width, 1);
		EXPECT_EQ(decodedOpaque, std::vector<int>(width, 9));
		EXPECT_EQ(encodedOpaque[0], 127);

		std::vector<UINT8> clear(width, 0);
		std::vector<UINT8> const encodedClear = Encode(clear, width, 1);
		EXPECT_EQ(Decode(encodedClear, width, 1), std::vector<int>(width, -1));
		EXPECT_EQ(encodedClear[0], 0x80 | 127);
		EXPECT_EQ(encodedClear.size(), static_cast<size_t>((width + 126) / 127 + 1));
	}
}


TEST(ETRLE, transparencyTableAndRoundTrip)
{
	// Index 0 opaque, 3 transparent: only the table decides.
	ETRLETransparency t{};
	t[3] = true;
	UINT16 const w = 13;
	UINT16 const h = 7;
	std::vector<UINT8> pixels(w * h);
	for (size_t i = 0; i != pixels.size(); ++i) pixels[i] = static_cast<UINT8>((i * 7) % 5);

	std::vector<int> const decoded = Decode(Encode(pixels, w, h, t), w, h);
	ASSERT_EQ(decoded.size(), pixels.size());
	for (size_t i = 0; i != pixels.size(); ++i)
	{
		EXPECT_EQ(decoded[i], pixels[i] == 3 ? -1 : pixels[i]) << "pixel " << i;
	}
}


TEST(ETRLE, subAreaUsesPitch)
{
	// 2x2 area at (1,1) of a 4x3 image
	std::vector<UINT8> const image{ 1, 1, 1, 1,
	                                1, 5, 0, 1,
	                                1, 6, 7, 1 };
	std::vector<UINT8> out;
	EncodeETRLE(image.data() + 4 + 1, 4, 2, 2, IndexZeroTransparent(), out);
	EXPECT_EQ(out, (std::vector<UINT8>{ 1, 5, 0x81, 0, 2, 6, 7, 0 }));
}
