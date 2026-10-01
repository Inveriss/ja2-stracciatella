#include "gtest/gtest.h"

#include "DefaultContentManagerUT.h"
#include "HImage.h"
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


// ---------------------------------------------------------------------------
// Frame metadata

TEST(PNG, framesMetadata)
{
	std::vector<PNGFrame> const frames = ParsePNGFrames(R"({ "frames": [
		{ "x": 0, "y": 0, "w": 10, "h": 6, "offsetX": -3, "offsetY": 2 },
		{ "x": 12, "y": 1, "w": 4, "h": 9 } ] })", 20, 12);
	ASSERT_EQ(frames.size(), 2u);
	EXPECT_EQ(frames[0].x, 0);
	EXPECT_EQ(frames[0].width, 10);
	EXPECT_EQ(frames[0].height, 6);
	EXPECT_EQ(frames[0].offsetX, -3);
	EXPECT_EQ(frames[0].offsetY, 2);
	EXPECT_EQ(frames[1].x, 12);
	EXPECT_EQ(frames[1].y, 1);
	EXPECT_EQ(frames[1].offsetX, 0);
	EXPECT_EQ(frames[1].offsetY, 0);
}


TEST(PNG, gridMetadata)
{
	// 3x2 cells of 5x4, the remaining 2 pixels of width are ignored
	std::vector<PNGFrame> const all = ParsePNGFrames(R"({ "grid": { "w": 5, "h": 4 } })", 17, 8);
	ASSERT_EQ(all.size(), 6u);
	EXPECT_EQ(all[2].x, 10);
	EXPECT_EQ(all[2].y, 0);
	EXPECT_EQ(all[3].x, 0);
	EXPECT_EQ(all[3].y, 4);
	EXPECT_EQ(all[5].offsetX, 0);

	std::vector<PNGFrame> const some = ParsePNGFrames(
		R"({ "grid": { "w": 5, "h": 4, "count": 4 }, "offsets": [ [0, 0], [-1, 1], [2, -2], [3, 3] ] })", 15, 8);
	ASSERT_EQ(some.size(), 4u);
	EXPECT_EQ(some[3].x, 0);
	EXPECT_EQ(some[3].y, 4);
	EXPECT_EQ(some[2].offsetX, 2);
	EXPECT_EQ(some[2].offsetY, -2);
}


TEST(PNG, invalidMetadataFails)
{
	char const* const invalid[] = {
		"not json",
		"[]",
		"{}",
		R"({ "frames": [], "grid": { "w": 1, "h": 1 } })",
		R"({ "frames": [] })",
		R"({ "frames": [ { "x": 0, "y": 0, "w": 11, "h": 1 } ] })",  // wider than the image
		R"({ "frames": [ { "x": -1, "y": 0, "w": 1, "h": 1 } ] })",
		R"({ "frames": [ { "x": 0, "y": 0, "w": 0, "h": 1 } ] })",
		R"({ "frames": [ { "x": 0, "y": 0, "w": 1 } ] })",          // missing "h"
		R"({ "frames": [ { "x": 0, "y": 0, "w": 1, "h": 1, "offsetX": 40000 } ] })",
		R"({ "grid": { "w": 3, "h": 3, "count": 10 } })",           // only 9 cells
		R"({ "grid": { "w": 11, "h": 1 } })",
		R"({ "grid": { "w": 5, "h": 5, "count": 2 }, "offsets": [ [0, 0] ] })",
		R"({ "grid": { "w": 5, "h": 5, "count": 1 }, "offsets": [ [0] ] })",
	};
	for (char const* const json : invalid)
	{
		SCOPED_TRACE(json);
		EXPECT_THROW(ParsePNGFrames(json, 10, 10), std::runtime_error);
	}
}


// ---------------------------------------------------------------------------
// Conversion to SGPImage

namespace
{

// 4x2 image with a 4 entry palette; index 2 has alpha 0, index 3 alpha 200
DecodedPNG SmallIndexedPNG()
{
	DecodedPNG png;
	png.kind             = DecodedPNG::Kind::Indexed;
	png.width            = 4;
	png.height           = 2;
	png.sourceColourType = 3;
	png.sourceBitDepth   = 8;
	png.palette = { { 1, 2, 3, 255 }, { 4, 5, 6, 255 }, { 7, 8, 9, 0 }, { 10, 11, 12, 200 } };
	png.pixels  = { 0, 1, 2, 3,
	                3, 3, 1, 0 };
	return png;
}

std::vector<PNGFrame> WholeImage(DecodedPNG const& png)
{
	return { PNGFrame{ 0, 0, png.width, png.height, 0, 0 } };
}

}


TEST(PNG, convertIndexed)
{
	DecodedPNG const png = SmallIndexedPNG();
	AutoSGPImage const img(ConvertIndexedPNGToImage(png, WholeImage(png), IMAGE_ALLIMAGEDATA, "test"));

	EXPECT_EQ(img->ubBitDepth, 8);
	EXPECT_EQ(img->usWidth, 4);
	EXPECT_EQ(img->usHeight, 2);
	EXPECT_EQ(img->fFlags & (IMAGE_PALETTE | IMAGE_BITMAPDATA | IMAGE_TRLECOMPRESSED),
		IMAGE_PALETTE | IMAGE_BITMAPDATA | IMAGE_TRLECOMPRESSED);
	EXPECT_EQ(img->uiAppDataSize, 0u);

	// palette padded to 256 entries, alpha unused like in STCI images
	SGPPaletteEntry const* const pal = img->pPalette;
	EXPECT_EQ(pal[3].r, 10);
	EXPECT_EQ(pal[3].g, 11);
	EXPECT_EQ(pal[3].b, 12);
	EXPECT_EQ(pal[3].a, 0);
	EXPECT_EQ(pal[4].r, 0);
	EXPECT_EQ(pal[255].b, 0);

	// index 0 and index 2 (alpha 0) are transparent, index 3 (alpha 200) is not
	ASSERT_EQ(img->usNumberOfObjects, 1);
	ETRLEObject const& o = img->pETRLEObject[0];
	EXPECT_EQ(o.uiDataOffset, 0u);
	EXPECT_EQ(o.usWidth, 4);
	EXPECT_EQ(o.usHeight, 2);
	EXPECT_EQ(o.sOffsetX, 0);
	EXPECT_EQ(o.sOffsetY, 0);
	std::vector<UINT8> const expected{ 0x81, 1, 1, 0x81, 1, 3, 0,
	                                   3, 3, 3, 1, 0x81, 0 };
	ASSERT_EQ(o.uiDataLength, expected.size());
	ASSERT_EQ(img->uiSizePixData, expected.size());
	UINT8 const* const data = img->pImageData;
	EXPECT_EQ(std::vector<UINT8>(data, data + expected.size()), expected);
}


TEST(PNG, convertIndexedFrames)
{
	DecodedPNG const png = SmallIndexedPNG();
	std::vector<PNGFrame> const frames{ { 1, 0, 2, 1, -5, 7 }, { 0, 1, 4, 1, 0, 0 } };
	AutoSGPImage const img(ConvertIndexedPNGToImage(png, frames, IMAGE_ALLIMAGEDATA, "test"));

	ASSERT_EQ(img->usNumberOfObjects, 2);
	ETRLEObject const& a = img->pETRLEObject[0];
	ETRLEObject const& b = img->pETRLEObject[1];
	EXPECT_EQ(a.sOffsetX, -5);
	EXPECT_EQ(a.sOffsetY, 7);
	EXPECT_EQ(a.usWidth, 2);
	EXPECT_EQ(a.usHeight, 1);
	EXPECT_EQ(a.uiDataOffset, 0u);
	EXPECT_EQ(a.uiDataLength, 4u); // 1, 1, 0x81, 0
	EXPECT_EQ(b.uiDataOffset, 4u);
	EXPECT_EQ(b.uiDataLength, 6u); // 3, 3, 3, 1, 0x81, 0
	EXPECT_EQ(img->uiSizePixData, 10u);
}


TEST(PNG, convertIndexedContents)
{
	DecodedPNG const png = SmallIndexedPNG();

	AutoSGPImage const palOnly(ConvertIndexedPNGToImage(png, WholeImage(png), IMAGE_PALETTE, "test"));
	EXPECT_EQ(palOnly->fFlags, IMAGE_PALETTE);
	EXPECT_TRUE(static_cast<SGPPaletteEntry const*>(palOnly->pPalette) != nullptr);
	EXPECT_TRUE(static_cast<UINT8 const*>(palOnly->pImageData) == nullptr);
	EXPECT_EQ(palOnly->usNumberOfObjects, 0);

	AutoSGPImage const dataOnly(ConvertIndexedPNGToImage(png, WholeImage(png), IMAGE_BITMAPDATA, "test"));
	EXPECT_EQ(dataOnly->fFlags, IMAGE_BITMAPDATA | IMAGE_TRLECOMPRESSED);
	EXPECT_TRUE(static_cast<SGPPaletteEntry const*>(dataOnly->pPalette) == nullptr);
	EXPECT_EQ(dataOnly->usNumberOfObjects, 1);
}


TEST(PNG, convertFails)
{
	DecodedPNG const indexed = SmallIndexedPNG();
	std::vector<PNGFrame> const outside{ { 2, 0, 3, 1, 0, 0 } };
	EXPECT_THROW(ConvertIndexedPNGToImage(indexed, outside, IMAGE_ALLIMAGEDATA, "test"), std::runtime_error);
	EXPECT_THROW(ConvertIndexedPNGToImage(indexed, {}, IMAGE_ALLIMAGEDATA, "test"), std::runtime_error);

	// RGBA PNGs need the full colour support of a later stage
	DecodedPNG const rgba = DecodeTestPNG("rgba8.png");
	EXPECT_THROW(ConvertIndexedPNGToImage(rgba, WholeImage(rgba), IMAGE_ALLIMAGEDATA, "test"), std::runtime_error);
}


// ---------------------------------------------------------------------------
// Loading through the VFS, compared to the same image as STI.
// The files in unittests/data/pngtest/ are made by
// tools/generate_png_unittest_data.py as well.

using PNGLoadTest = DefaultContentManagerUT::BaseTest;

namespace
{

void ExpectSameImage(SGPImage const& sti, SGPImage const& png)
{
	UINT16 const flags = IMAGE_PALETTE | IMAGE_BITMAPDATA | IMAGE_TRLECOMPRESSED;
	ASSERT_EQ(sti.fFlags & flags, flags);
	ASSERT_EQ(png.fFlags & flags, flags);
	EXPECT_EQ(png.ubBitDepth, sti.ubBitDepth);
	EXPECT_EQ(png.uiAppDataSize, 0u);

	SGPPaletteEntry const* const stiPal = sti.pPalette;
	SGPPaletteEntry const* const pngPal = png.pPalette;
	for (size_t i = 0; i != 256; ++i)
	{
		SGPPaletteEntry const& a = stiPal[i];
		SGPPaletteEntry const& b = pngPal[i];
		EXPECT_TRUE(a.r == b.r && a.g == b.g && a.b == b.b) << "palette entry " << i;
	}

	ASSERT_EQ(png.usNumberOfObjects, sti.usNumberOfObjects);
	ETRLEObject const* const stiObjects = sti.pETRLEObject;
	ETRLEObject const* const pngObjects = png.pETRLEObject;
	for (size_t i = 0; i != sti.usNumberOfObjects; ++i)
	{
		ETRLEObject const& a = stiObjects[i];
		ETRLEObject const& b = pngObjects[i];
		EXPECT_EQ(b.uiDataOffset, a.uiDataOffset) << "frame " << i;
		EXPECT_EQ(b.uiDataLength, a.uiDataLength) << "frame " << i;
		EXPECT_EQ(b.sOffsetX,     a.sOffsetX)     << "frame " << i;
		EXPECT_EQ(b.sOffsetY,     a.sOffsetY)     << "frame " << i;
		EXPECT_EQ(b.usWidth,      a.usWidth)      << "frame " << i;
		EXPECT_EQ(b.usHeight,     a.usHeight)     << "frame " << i;
	}

	ASSERT_EQ(png.uiSizePixData, sti.uiSizePixData);
	UINT8 const* const stiData = sti.pImageData;
	UINT8 const* const pngData = png.pImageData;
	EXPECT_EQ(std::memcmp(pngData, stiData, sti.uiSizePixData), 0);
}

void ExpectSameAsSTI(char const* const stem)
{
	SCOPED_TRACE(stem);
	AutoSGPImage const sti(CreateImage(ST::format("pngtest/{}.sti", stem), IMAGE_ALLDATA));
	AutoSGPImage const png(CreateImage(ST::format("pngtest/{}.png", stem), IMAGE_ALLDATA));
	ExpectSameImage(*sti, *png);
}

}


TEST_F(PNGLoadTest, singleFrame)
{
	ExpectSameAsSTI("single");
}


TEST_F(PNGLoadTest, framesMetadata)
{
	ExpectSameAsSTI("frames");
}


TEST_F(PNGLoadTest, gridMetadata)
{
	ExpectSameAsSTI("grid");
}


TEST_F(PNGLoadTest, transparency)
{
	ExpectSameAsSTI("trns");
}


TEST_F(PNGLoadTest, extensionIsCaseInsensitive)
{
	AutoSGPImage const img(CreateImage("pngtest/single.PNG", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(img->usNumberOfObjects, 1);
}


TEST_F(PNGLoadTest, missingFileFails)
{
	EXPECT_THROW(CreateImage("pngtest/does_not_exist.png", IMAGE_ALLIMAGEDATA), std::runtime_error);
}
