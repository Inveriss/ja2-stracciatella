#include "gtest/gtest.h"

#include "PNG.h"
#include "TestUtils.h"

#include <string_theory/format>

#include <cstring>
#include <stdexcept>
#include <vector>

// The test files are made by tools/generate_png_unittest_data.py; the
// formulas below repeat the ones used there.

namespace
{

std::vector<uint8_t> ReadTestPNG(char const* const name)
{
	ST::string const path = ST::format("unittests/png/{}", name);
	AutoSGPFile f(OpenTestResourceForReading(path.c_str()));
	return f->readToEnd();
}

DecodedPNG DecodeTestPNG(char const* const name)
{
	std::vector<uint8_t> const data = ReadTestPNG(name);
	return DecodePNG(data.data(), data.size());
}

SGPPaletteEntry PaletteEntry(unsigned const i)
{
	unsigned const j = i & ~1U;
	return SGPPaletteEntry{ UINT8(j), UINT8(255 - j), UINT8((j * 7) & 0xFF), 255 };
}

unsigned Idx8Value(unsigned const x, unsigned const y)
{
	if (x == 1 && y == 0) return 254;
	if (x == 2 && y == 0) return 255;
	return (x * 37 + y * 101) % 256;
}

unsigned InterlacedValue(unsigned const x, unsigned const y)
{
	return (x * 37 + y * 101) % 256;
}

void ExpectPaletteEntry(SGPPaletteEntry const& got, SGPPaletteEntry const& want, size_t const i)
{
	EXPECT_EQ(got.r, want.r) << "palette entry " << i;
	EXPECT_EQ(got.g, want.g) << "palette entry " << i;
	EXPECT_EQ(got.b, want.b) << "palette entry " << i;
	EXPECT_EQ(got.a, want.a) << "palette entry " << i;
}

void ExpectRGBA(DecodedPNG const& png, unsigned const x, unsigned const y,
	unsigned const r, unsigned const g, unsigned const b, unsigned const a)
{
	UINT8 const* const p = &png.pixels[(y * png.width + x) * 4];
	EXPECT_EQ(p[0], r) << "pixel " << x << "," << y;
	EXPECT_EQ(p[1], g) << "pixel " << x << "," << y;
	EXPECT_EQ(p[2], b) << "pixel " << x << "," << y;
	EXPECT_EQ(p[3], a) << "pixel " << x << "," << y;
}

void ExpectDecodeFails(std::vector<uint8_t> const& data)
{
	EXPECT_THROW(DecodePNG(data.data(), data.size()), std::runtime_error);
}

}


TEST(PNG, indexed8KeepsIndicesAndPalette)
{
	// 7x5, all five row filter types, palette with duplicate colours
	DecodedPNG const png = DecodeTestPNG("indexed8.png");
	EXPECT_EQ(png.kind, DecodedPNG::Kind::Indexed);
	EXPECT_EQ(png.width, 7);
	EXPECT_EQ(png.height, 5);
	EXPECT_EQ(png.sourceColourType, 3);
	EXPECT_EQ(png.sourceBitDepth, 8);

	ASSERT_EQ(png.palette.size(), 256u);
	for (size_t i = 0; i != png.palette.size(); ++i)
	{
		ExpectPaletteEntry(png.palette[i], PaletteEntry(unsigned(i)), i);
	}

	ASSERT_EQ(png.pixels.size(), 7u * 5u);
	for (unsigned y = 0; y != 5; ++y)
	{
		for (unsigned x = 0; x != 7; ++x)
		{
			EXPECT_EQ(png.pixels[y * 7 + x], Idx8Value(x, y)) << "pixel " << x << "," << y;
		}
	}
	EXPECT_EQ(png.pixels[0], 0);
	EXPECT_EQ(png.pixels[1], 254);
	EXPECT_EQ(png.pixels[2], 255);
}


TEST(PNG, indexedLowBitDepths)
{
	for (unsigned const depth : { 1U, 2U, 4U })
	{
		ST::string const name = ST::format("indexed{}.png", depth);
		SCOPED_TRACE(name.c_str());
		DecodedPNG const png = DecodeTestPNG(name.c_str());
		EXPECT_EQ(png.kind, DecodedPNG::Kind::Indexed);
		EXPECT_EQ(png.sourceBitDepth, depth);
		ASSERT_EQ(png.width, 5);
		ASSERT_EQ(png.height, 3);
		ASSERT_EQ(png.palette.size(), size_t{1} << depth);

		for (unsigned y = 0; y != 3; ++y)
		{
			for (unsigned x = 0; x != 5; ++x)
			{
				EXPECT_EQ(png.pixels[y * 5 + x], (x + 2 * y) % (1U << depth)) << "pixel " << x << "," << y;
			}
		}
	}
}


TEST(PNG, indexedInterlaced)
{
	DecodedPNG const png = DecodeTestPNG("indexed8_interlaced.png");
	ASSERT_EQ(png.width, 9);
	ASSERT_EQ(png.height, 9);
	for (unsigned y = 0; y != 9; ++y)
	{
		for (unsigned x = 0; x != 9; ++x)
		{
			EXPECT_EQ(png.pixels[y * 9 + x], InterlacedValue(x, y)) << "pixel " << x << "," << y;
		}
	}

	// Most Adam7 passes are empty for a single pixel.
	DecodedPNG const tiny = DecodeTestPNG("indexed8_interlaced_1x1.png");
	ASSERT_EQ(tiny.width, 1);
	ASSERT_EQ(tiny.height, 1);
	EXPECT_EQ(tiny.pixels[0], 0);
}


TEST(PNG, indexedTransparency)
{
	// palette of 4 entries, tRNS gives alpha 0 and 128 to the first two
	DecodedPNG const png = DecodeTestPNG("indexed8_trns.png");
	ASSERT_EQ(png.palette.size(), 4u);
	UINT8 const alpha[] = { 0, 128, 255, 255 };
	for (unsigned i = 0; i != 4; ++i)
	{
		SGPPaletteEntry want = PaletteEntry(i);
		want.a = alpha[i];
		ExpectPaletteEntry(png.palette[i], want, i);
	}

	for (unsigned y = 0; y != 2; ++y)
	{
		for (unsigned x = 0; x != 3; ++x)
		{
			EXPECT_EQ(png.pixels[y * 3 + x], (x + y) % 4);
		}
	}
}


TEST(PNG, indexedOutOfRangeIndexFails)
{
	ExpectDecodeFails(ReadTestPNG("indexed8_out_of_range.png"));
}


TEST(PNG, rgb8)
{
	DecodedPNG const png = DecodeTestPNG("rgb8.png");
	EXPECT_EQ(png.kind, DecodedPNG::Kind::RGBA);
	EXPECT_EQ(png.sourceColourType, 2);
	ASSERT_EQ(png.width, 4);
	ASSERT_EQ(png.height, 3);
	ASSERT_EQ(png.pixels.size(), 4u * 3u * 4u);
	EXPECT_TRUE(png.palette.empty());
	for (unsigned y = 0; y != 3; ++y)
	{
		for (unsigned x = 0; x != 4; ++x)
		{
			ExpectRGBA(png, x, y, x * 60, y * 100, (x + y) * 30, 255);
		}
	}
}


TEST(PNG, rgba8)
{
	DecodedPNG const png = DecodeTestPNG("rgba8.png");
	EXPECT_EQ(png.kind, DecodedPNG::Kind::RGBA);
	EXPECT_EQ(png.sourceColourType, 6);
	ASSERT_EQ(png.width, 4);
	ASSERT_EQ(png.height, 3);
	for (unsigned y = 0; y != 3; ++y)
	{
		for (unsigned x = 0; x != 4; ++x)
		{
			ExpectRGBA(png, x, y, x * 60, y * 100, (x + y) * 30, (x * 80 + y * 20) % 256);
		}
	}
}


TEST(PNG, grey8)
{
	DecodedPNG const png = DecodeTestPNG("grey8.png");
	EXPECT_EQ(png.kind, DecodedPNG::Kind::RGBA);
	EXPECT_EQ(png.sourceColourType, 0);
	ASSERT_EQ(png.width, 4);
	ASSERT_EQ(png.height, 3);
	for (unsigned y = 0; y != 3; ++y)
	{
		for (unsigned x = 0; x != 4; ++x)
		{
			unsigned const v = x * 60 + y * 20;
			ExpectRGBA(png, x, y, v, v, v, 255);
		}
	}
}


TEST(PNG, rgb16IsReducedTo8Bits)
{
	DecodedPNG const png = DecodeTestPNG("rgb16.png");
	EXPECT_EQ(png.kind, DecodedPNG::Kind::RGBA);
	EXPECT_EQ(png.sourceBitDepth, 16);
	ASSERT_EQ(png.width, 2);
	ASSERT_EQ(png.height, 2);
	for (unsigned y = 0; y != 2; ++y)
	{
		for (unsigned x = 0; x != 2; ++x)
		{
			ExpectRGBA(png, x, y, x * 100, y * 100, 50, 255);
		}
	}
}


TEST(PNG, invalidDataFails)
{
	std::vector<uint8_t> const good = ReadTestPNG("indexed8.png");
	ASSERT_NO_THROW(DecodePNG(good.data(), good.size()));

	// empty input and a wrong signature
	ExpectDecodeFails({});
	{
		std::vector<uint8_t> bad = good;
		bad[1] = 'X';
		ExpectDecodeFails(bad);
	}

	// truncated: within the header, within the image data, and without IEND
	for (size_t const len : { size_t{8}, size_t{20}, good.size() / 2, good.size() - 12 })
	{
		SCOPED_TRACE(len);
		ExpectDecodeFails(std::vector<uint8_t>(good.begin(), good.begin() + len));
	}

	// zero width (the IHDR width is at offset 16)
	{
		std::vector<uint8_t> bad = good;
		std::memset(&bad[16], 0, 4);
		ExpectDecodeFails(bad);
	}

	// truncated RGB image, decoded by stb_image
	std::vector<uint8_t> const rgb = ReadTestPNG("rgb8.png");
	ExpectDecodeFails(std::vector<uint8_t>(rgb.begin(), rgb.begin() + rgb.size() / 2));
}
