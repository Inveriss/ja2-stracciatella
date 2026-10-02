#include "gtest/gtest.h"

#include "DefaultContentManagerUT.h"
#include "HImage.h"
#include "PCX.h"
#include "PNG.h"
#include "STCI.h"
#include "TestUtils.h"
#include "VObject.h"
#include "VObject_Blitters.h"
#include "Shading.h"
#include "VSurface.h"

#include <string_theory/format>

#include <chrono>
#include <cstdio>
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
	std::vector<PNGFrame> const frames = ParsePNGMetadata(R"({ "frames": [
		{ "x": 0, "y": 0, "w": 10, "h": 6, "offsetX": -3, "offsetY": 2 },
		{ "x": 12, "y": 1, "w": 4, "h": 9 } ] })", 20, 12).frames;
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
	std::vector<PNGFrame> const all = ParsePNGMetadata(R"({ "grid": { "w": 5, "h": 4 } })", 17, 8).frames;
	ASSERT_EQ(all.size(), 6u);
	EXPECT_EQ(all[2].x, 10);
	EXPECT_EQ(all[2].y, 0);
	EXPECT_EQ(all[3].x, 0);
	EXPECT_EQ(all[3].y, 4);
	EXPECT_EQ(all[5].offsetX, 0);

	std::vector<PNGFrame> const some = ParsePNGMetadata(
		R"({ "grid": { "w": 5, "h": 4, "count": 4 }, "offsets": [ [0, 0], [-1, 1], [2, -2], [3, 3] ] })", 15, 8).frames;
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
		R"({ "offsets": [ [0, 0] ] })",                              // offsets without a grid
		R"({ "outline": "no" })",
	};
	for (char const* const json : invalid)
	{
		SCOPED_TRACE(json);
		EXPECT_THROW(ParsePNGMetadata(json, 10, 10), std::runtime_error);
	}
}


TEST(PNG, metadataWithoutFrames)
{
	// no frames given: the whole image is one frame
	PNGMetadata const empty = ParsePNGMetadata("{}", 7, 5);
	ASSERT_EQ(empty.frames.size(), 1u);
	EXPECT_EQ(empty.frames[0].width, 7);
	EXPECT_EQ(empty.frames[0].height, 5);
	EXPECT_TRUE(empty.outline);

	PNGMetadata const noOutline = ParsePNGMetadata(R"({ "outline": false })", 7, 5);
	EXPECT_FALSE(noOutline.outline);
	EXPECT_EQ(noOutline.frames.size(), 1u);

	PNGMetadata const withGrid = ParsePNGMetadata(R"({ "grid": { "w": 7, "h": 1 }, "outline": false })", 7, 5);
	EXPECT_FALSE(withGrid.outline);
	EXPECT_EQ(withGrid.frames.size(), 5u);
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
	// Not CreateImage(): the PNG next to the STI would replace it.
	AutoSGPImage const sti(LoadSTCIFileToImage(ST::format("pngtest/{}.sti", stem), IMAGE_ALLDATA));
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


// ---------------------------------------------------------------------------
// Video surfaces (IMAGE_FOR_SURFACE)

namespace
{

// The 16 bpp conversion uses the screen's pixel format, which Video.cc sets
// up at run time; the tests use RGB565 as the game does.
class RGB565Format
{
public:
	RGB565Format() :
		red_{ gusRedMask }, green_{ gusGreenMask }, blue_{ gusBlueMask },
		redShift_{ gusRedShift }, greenShift_{ gusGreenShift }, blueShift_{ gusBlueShift }
	{
		gusRedMask    = 0xF800;
		gusGreenMask  = 0x07E0;
		gusBlueMask   = 0x001F;
		gusRedShift   = 8;
		gusGreenShift = 3;
		gusBlueShift  = -3;
	}

	~RGB565Format()
	{
		gusRedMask    = red_;
		gusGreenMask  = green_;
		gusBlueMask   = blue_;
		gusRedShift   = redShift_;
		gusGreenShift = greenShift_;
		gusBlueShift  = blueShift_;
	}

private:
	UINT16 red_, green_, blue_;
	INT16  redShift_, greenShift_, blueShift_;
};

// Expected 16 bpp values of unittests/data/pngtest/rgba_surface.png
UINT16 const RGBA_SURFACE_565[] = {
	0xF800, 0x07E0, 0x001F, BLACK_SUBSTITUTE,   // red, green, blue, black stays visible
	0x0000, 0x0000, 0x08A3, BLACK_SUBSTITUTE    // alpha 0 and 127 transparent, alpha 128 opaque, too dark red
};

}


TEST(PNG, surfaceIndexedKeepsIndices)
{
	DecodedPNG const png = SmallIndexedPNG();
	AutoSGPImage const img(ConvertPNGToSurfaceImage(png, IMAGE_ALLIMAGEDATA | IMAGE_FOR_SURFACE));

	EXPECT_EQ(img->ubBitDepth, 8);
	EXPECT_EQ(img->usWidth, 4);
	EXPECT_EQ(img->usHeight, 2);
	EXPECT_EQ(img->fFlags, IMAGE_PALETTE | IMAGE_BITMAPDATA);
	EXPECT_EQ(img->usNumberOfObjects, 0);
	ASSERT_EQ(img->uiSizePixData, 8u);

	// plain indices; tRNS does not matter (index 2 has alpha 0)
	UINT8 const* const data = img->pImageData;
	EXPECT_EQ(std::vector<UINT8>(data, data + 8), png.pixels);

	SGPPaletteEntry const* const pal = img->pPalette;
	EXPECT_EQ(pal[2].r, 7);
	EXPECT_EQ(pal[2].g, 8);
	EXPECT_EQ(pal[2].b, 9);
	EXPECT_EQ(pal[200].r, 0);

	AutoSGPImage const palOnly(ConvertPNGToSurfaceImage(png, IMAGE_PALETTE));
	EXPECT_EQ(palOnly->fFlags, IMAGE_PALETTE);
	EXPECT_TRUE(static_cast<UINT8 const*>(palOnly->pImageData) == nullptr);
}


TEST(PNG, surfaceRGBATo565)
{
	RGB565Format const format;
	DecodedPNG const rgb = DecodeTestPNG("rgb8.png");
	AutoSGPImage const img(ConvertPNGToSurfaceImage(rgb, IMAGE_ALLIMAGEDATA | IMAGE_FOR_SURFACE));

	EXPECT_EQ(img->ubBitDepth, 16);
	EXPECT_EQ(img->fFlags, IMAGE_BITMAPDATA);
	ASSERT_EQ(img->uiSizePixData, 4u * 3u * 2u);
	UINT16 const* const data = reinterpret_cast<UINT16 const*>(static_cast<UINT8 const*>(img->pImageData));
	for (unsigned y = 0; y != 3; ++y)
	{
		for (unsigned x = 0; x != 4; ++x)
		{
			unsigned const r = x * 60, g = y * 100, b = (x + y) * 30;
			UINT16 want = static_cast<UINT16>((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
			if (want == 0) want = BLACK_SUBSTITUTE;
			EXPECT_EQ(data[y * 4 + x], want) << "pixel " << x << "," << y;
		}
	}
}


TEST_F(PNGLoadTest, surfaceImageThroughVFS)
{
	RGB565Format const format;
	AutoSGPImage const img(CreateImage("pngtest/rgba_surface.png", IMAGE_ALLIMAGEDATA | IMAGE_FOR_SURFACE));
	ASSERT_EQ(img->ubBitDepth, 16);
	UINT16 const* const data = reinterpret_cast<UINT16 const*>(static_cast<UINT8 const*>(img->pImageData));
	for (size_t i = 0; i != 8; ++i)
	{
		EXPECT_EQ(data[i], RGBA_SURFACE_565[i]) << "pixel " << i;
	}

	// The frame metadata of a sheet is not used for a surface.
	AutoSGPImage const sheet(CreateImage("pngtest/frames.png", IMAGE_ALLIMAGEDATA | IMAGE_FOR_SURFACE));
	EXPECT_EQ(sheet->ubBitDepth, 8);
	EXPECT_EQ(sheet->usWidth, 20);
	EXPECT_EQ(sheet->usHeight, 12);
	EXPECT_EQ(sheet->fFlags & IMAGE_TRLECOMPRESSED, 0);
}


TEST_F(PNGLoadTest, videoSurfaceFromPNG)
{
	RGB565Format const format;

	std::unique_ptr<SGPVSurface> const rgba(AddVideoSurfaceFromFile("pngtest/rgba_surface.png"));
	ASSERT_EQ(rgba->BPP(), 16);
	ASSERT_EQ(rgba->Width(), 4);
	ASSERT_EQ(rgba->Height(), 2);
	{
		SDL_Surface const& s = rgba->GetSDLSurface();
		for (int y = 0; y != 2; ++y)
		{
			UINT16 const* const row = reinterpret_cast<UINT16 const*>(static_cast<UINT8 const*>(s.pixels) + y * s.pitch);
			for (int x = 0; x != 4; ++x)
			{
				EXPECT_EQ(row[x], RGBA_SURFACE_565[y * 4 + x]) << "pixel " << x << "," << y;
			}
		}
	}

	std::unique_ptr<SGPVSurface> const indexed(AddVideoSurfaceFromFile("pngtest/single.png"));
	ASSERT_EQ(indexed->BPP(), 8);
	ASSERT_EQ(indexed->Width(), 150);
	ASSERT_EQ(indexed->Height(), 4);
	{
		std::vector<uint8_t> const file = [] {
			AutoSGPFile f(GCM->openGameResForReading("pngtest/single.png"));
			return f->readToEnd();
		}();
		DecodedPNG const png = DecodePNG(file.data(), file.size());
		SDL_Surface const& s = indexed->GetSDLSurface();
		for (int y = 0; y != 4; ++y)
		{
			UINT8 const* const row = static_cast<UINT8 const*>(s.pixels) + y * s.pitch;
			EXPECT_EQ(std::memcmp(row, &png.pixels[y * 150], 150), 0) << "row " << y;
		}
		SGPPaletteEntry const* const pal = indexed->GetPalette();
		ASSERT_TRUE(pal != nullptr);
		EXPECT_EQ(pal[7].r, png.palette[7].r);
		EXPECT_EQ(pal[7].g, png.palette[7].g);
		EXPECT_EQ(pal[7].b, png.palette[7].b);
	}
}


TEST_F(PNGLoadTest, videoSurfaceRejectsETRLEImages)
{
	// An indexed ETRLE STI can only become a video object.
	EXPECT_THROW(AddVideoSurfaceFromFile("pngtest/etrle_only.sti"), std::runtime_error);
}


TEST_F(PNGLoadTest, stretchPalettisedSurface)
{
	RGB565Format const format;
	std::unique_ptr<SGPVSurface> const src(AddVideoSurfaceFromFile("pngtest/single.png"));
	ASSERT_EQ(src->BPP(), 8);
	SGPVSurface dst(150, 4, 16);
	FillVideoSurfaceWithStretch(&dst, src.get());

	SDL_Surface const& s = src->GetSDLSurface();
	SDL_Surface const& d = dst.GetSDLSurface();
	SGPPaletteEntry const* const pal = src->GetPalette();
	for (int y = 0; y != 4; ++y)
	{
		for (int x = 0; x != 150; ++x)
		{
			UINT8  const index = static_cast<UINT8 const*>(s.pixels)[y * s.pitch + x];
			UINT16 const got   = reinterpret_cast<UINT16 const*>(static_cast<UINT8 const*>(d.pixels) + y * d.pitch)[x];
			EXPECT_EQ(got, Get16BPPColor(FROMRGB(pal[index].r, pal[index].g, pal[index].b))) << "pixel " << x << "," << y;
		}
	}
}


// ---------------------------------------------------------------------------
// A PNG next to an image of another format replaces it (CreateImage()).
// The STI files used here have 2 frames, the PNGs 1.

TEST_F(PNGLoadTest, replacementInTheSameLayer)
{
	EXPECT_EQ(GCM->getPNGReplacement("pngtest/replaced.sti"), "pngtest/replaced.png");
	EXPECT_FALSE(GCM->getPNGReplacement("PNGTEST/Replaced.STI").empty());
	EXPECT_TRUE(GCM->getPNGReplacement("pngtest/replaced.png").empty());

	AutoSGPImage const img(CreateImage("pngtest/replaced.sti", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(img->usNumberOfObjects, 1);
}


TEST_F(PNGLoadTest, noReplacement)
{
	EXPECT_TRUE(GCM->getPNGReplacement("pngtest/etrle_only.sti").empty());
	AutoSGPImage const img(CreateImage("pngtest/etrle_only.sti", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(img->usNumberOfObjects, 2);
}


TEST_F(PNGLoadTest, noReplacementWhenAppDataIsNeeded)
{
	// tiles, animations and cursors: PNG files have no application data
	AutoSGPImage const img(CreateImage("pngtest/replaced.sti", IMAGE_ALLDATA));
	EXPECT_EQ(img->usNumberOfObjects, 2);
}


TEST_F(PNGLoadTest, replacementFollowsLayerPriority)
{
	// loose_sti.sti is a loose file in data/, loose_sti.png is in data/pngtest.slf,
	// a lower priority layer: the STI stays.
	EXPECT_TRUE(GCM->getPNGReplacement("pngtest/loose_sti.sti").empty());
	AutoSGPImage const sti(CreateImage("pngtest/loose_sti.sti", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(sti->usNumberOfObjects, 2);

	// loose_png.sti is in the SLF, loose_png.png a loose file: the PNG wins.
	EXPECT_EQ(GCM->getPNGReplacement("pngtest/loose_png.sti"), "pngtest/loose_png.png");
	AutoSGPImage const png(CreateImage("pngtest/loose_png.sti", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(png->usNumberOfObjects, 1);
}


TEST_F(PNGLoadTest, unusableReplacementFallsBackToOriginal)
{
	// not a PNG at all
	EXPECT_FALSE(GCM->getPNGReplacement("pngtest/broken.sti").empty());
	AutoSGPImage const broken(CreateImage("pngtest/broken.sti", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(broken->usNumberOfObjects, 2);

	// an RGBA PNG where the palette is needed
	AutoSGPImage const object(CreateImage("pngtest/rgba_next_to_sti.sti", IMAGE_ALLIMAGEDATA | IMAGE_NEEDS_PALETTE));
	EXPECT_EQ(object->ubBitDepth, 8);
	EXPECT_EQ(object->usNumberOfObjects, 2);

	// the same PNG as a video surface
	RGB565Format const format;
	std::unique_ptr<SGPVSurface> const surface(AddVideoSurfaceFromFile("pngtest/rgba_next_to_sti.sti"));
	EXPECT_EQ(surface->BPP(), 16);
	EXPECT_EQ(surface->Width(), 4);
	EXPECT_EQ(surface->Height(), 3);
}


// ---------------------------------------------------------------------------
// Full colour (32 bit RGBA) video objects

namespace
{

// 3x3 RGBA image: transparent except an opaque red centre, a black pixel
// right of it and a half transparent white one below it.
DecodedPNG SmallRGBAPNG()
{
	DecodedPNG png;
	png.kind             = DecodedPNG::Kind::RGBA;
	png.width            = 3;
	png.height           = 3;
	png.sourceColourType = 6;
	png.sourceBitDepth   = 8;
	png.pixels.assign(3 * 3 * 4, 0);
	auto const set = [&](size_t x, size_t y, UINT8 r, UINT8 g, UINT8 b, UINT8 a)
	{
		UINT8* const p = &png.pixels[(y * 3 + x) * 4];
		p[0] = r; p[1] = g; p[2] = b; p[3] = a;
	};
	set(1, 1, 255,   0,   0, 255);
	set(2, 1,   0,   0,   0, 255);
	set(1, 2, 255, 255, 255, 128);
	set(0, 0,  50,  60,  70, 100); // below the opacity threshold
	return png;
}

std::unique_ptr<SGPVObject> SmallRGBAObject(std::vector<PNGFrame> const& frames)
{
	AutoSGPImage img(ConvertRGBAPNGToImage(SmallRGBAPNG(), frames, IMAGE_ALLIMAGEDATA, "test"));
	return std::unique_ptr<SGPVObject>(AddVideoObjectFromHImage(img.get()));
}

std::unique_ptr<SGPVObject> SmallRGBAObject()
{
	return SmallRGBAObject({ PNGFrame{ 0, 0, 3, 3, 0, 0 } });
}

UINT16 Pixel(SGPVSurface const& s, int const x, int const y)
{
	SDL_Surface const& sdl = s.GetSDLSurface();
	return reinterpret_cast<UINT16 const*>(static_cast<UINT8 const*>(sdl.pixels) + y * sdl.pitch)[x];
}

// Sets the clip rect for the lifetime of the object.
class ScopedClip
{
public:
	explicit ScopedClip(SGPRect const r) : old_{ SetClippingRect(r) } {}
	~ScopedClip() { SetClippingRect(old_); }
private:
	SGPRect old_;
};

UINT16 const BLUE_565 = 0x001F;

}


TEST(PNG, convertRGBA)
{
	DecodedPNG const png = SmallRGBAPNG();
	std::vector<PNGFrame> const frames{ { 1, 1, 2, 2, -3, 4 }, { 0, 0, 3, 1, 0, 0 } };
	AutoSGPImage const img(ConvertRGBAPNGToImage(png, frames, IMAGE_ALLIMAGEDATA, "test"));

	EXPECT_EQ(img->ubBitDepth, 32);
	EXPECT_EQ(img->fFlags, IMAGE_RGBA | IMAGE_BITMAPDATA);
	ASSERT_EQ(img->usNumberOfObjects, 2);
	ASSERT_EQ(img->uiSizePixData, (2u * 2u + 3u) * 4u);

	ETRLEObject const* const o = img->pETRLEObject;
	EXPECT_EQ(o[0].uiDataOffset, 0u);
	EXPECT_EQ(o[0].uiDataLength, 16u);
	EXPECT_EQ(o[0].sOffsetX, -3);
	EXPECT_EQ(o[0].sOffsetY, 4);
	EXPECT_EQ(o[0].usWidth, 2);
	EXPECT_EQ(o[0].usHeight, 2);
	EXPECT_EQ(o[1].uiDataOffset, 16u);
	EXPECT_EQ(o[1].uiDataLength, 12u);

	// frame 0 is the 2x2 area at 1,1: red, black / white 128, transparent
	UINT8 const* const data = img->pImageData;
	EXPECT_EQ(std::vector<UINT8>(data, data + 16), (std::vector<UINT8>{
		255, 0, 0, 255,      0, 0, 0, 255,
		255, 255, 255, 128,  0, 0, 0, 0 }));
	// frame 1 is the first row
	EXPECT_EQ(std::vector<UINT8>(data + 16, data + 20), (std::vector<UINT8>{ 50, 60, 70, 100 }));

	EXPECT_THROW(ConvertRGBAPNGToImage(SmallIndexedPNG(), frames, IMAGE_ALLIMAGEDATA, "test"), std::runtime_error);
	std::vector<PNGFrame> const outside{ { 2, 2, 2, 2, 0, 0 } };
	EXPECT_THROW(ConvertRGBAPNGToImage(png, outside, IMAGE_ALLIMAGEDATA, "test"), std::runtime_error);
}


TEST(PNG, rgbaVideoObjectHasNoPalette)
{
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	EXPECT_TRUE(vo->IsRGBA());
	EXPECT_EQ(vo->BPP(), 32);
	EXPECT_EQ(vo->SubregionCount(), 1);
	EXPECT_THROW(vo->Palette(), std::logic_error);
	EXPECT_THROW(vo->PixData(vo->SubregionProperties(0)), std::logic_error);
	EXPECT_THROW(vo->GetETRLEPixelValue(0, 1, 1), std::logic_error);
	EXPECT_TRUE(vo->CurrentShade() == nullptr);
	EXPECT_NO_THROW(vo->CurrentShade(3)); // ignored
	EXPECT_NO_THROW(vo->RGBAData(vo->SubregionProperties(0)));
}


TEST(PNG, rgbaOutlineMask)
{
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	UINT8 const* const mask = vo->OutlineMask(vo->SubregionProperties(0));
	// opaque: (1,1) and (2,1); (1,2) has alpha 128, so it is opaque too;
	// (0,0) has alpha 100 and is no neighbour of an opaque pixel's side
	UINT8 const expected[] = {
		0, 1, 1,
		1, 0, 0,
		1, 0, 1 };
	for (size_t i = 0; i != 9; ++i)
	{
		EXPECT_EQ(mask[i] != 0, expected[i] != 0) << "pixel " << i % 3 << "," << i / 3;
	}

	// per frame: the mask of frame 1 (the top row only) has no opaque pixels next to it
	std::unique_ptr<SGPVObject> const frames = SmallRGBAObject({ { 0, 1, 3, 2, 0, 0 }, { 0, 0, 3, 1, 0, 0 } });
	UINT8 const* const top = frames->OutlineMask(frames->SubregionProperties(1));
	EXPECT_EQ(top[0] | top[1] | top[2], 0);
}


TEST(PNG, rgbaBlitAlphaBlends)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject({ PNGFrame{ 0, 0, 3, 3, 1, 2 } });
	SGPVSurface dst(6, 6, 16);
	dst.Fill(BLUE_565);
	ScopedClip const clip(SGPRect{ 0, 0, 6, 6 });

	BltVideoObject(&dst, vo.get(), 0, 1, 0); // drawn at 2,2 (offset 1,2)

	EXPECT_EQ(Pixel(dst, 3, 3), 0xF800);           // opaque red
	EXPECT_EQ(Pixel(dst, 4, 3), BLACK_SUBSTITUTE); // opaque black stays visible
	EXPECT_EQ(Pixel(dst, 3, 4), 0x841F);           // white at alpha 128 over blue
	EXPECT_EQ(Pixel(dst, 2, 2), 0x10D6);           // (50, 60, 70) at alpha 100 over blue: 20, 24, 182
	EXPECT_EQ(Pixel(dst, 4, 4), BLUE_565);         // alpha 0 untouched
}


TEST(PNG, rgbaBlitAlphaPartial)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	SGPVSurface dst(3, 3, 16);
	dst.Fill(0);
	ScopedClip const clip(SGPRect{ 0, 0, 3, 3 });
	BltVideoObject(&dst, vo.get(), 0, 0, 0);
	// (50, 60, 70) at alpha 100 over black: 20, 24, 27 -> 565
	UINT16 const want = (20 >> 3) << 11 | (24 >> 2) << 5 | (27 >> 3);
	EXPECT_EQ(Pixel(dst, 0, 0), want);
}


TEST(PNG, rgbaBlitClips)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	SGPVSurface dst(4, 4, 16);
	dst.Fill(BLUE_565);
	{
		// only column 2 and below row 1 of the surface are inside
		ScopedClip const clip(SGPRect{ 2, 1, 3, 4 });
		BltVideoObject(&dst, vo.get(), 0, 1, 0);  // red at 2,1, black at 3,1
	}
	EXPECT_EQ(Pixel(dst, 2, 1), 0xF800);
	EXPECT_EQ(Pixel(dst, 3, 1), BLUE_565); // clipped
	EXPECT_EQ(Pixel(dst, 2, 2), 0x841F);

	// completely outside, or partly left of / above the surface: no crash
	dst.Fill(BLUE_565);
	ScopedClip const clip(SGPRect{ 0, 0, 4, 4 });
	BltVideoObject(&dst, vo.get(), 0, 10, 10);
	BltVideoObject(&dst, vo.get(), 0, -1, -1); // red at 0,0
	EXPECT_EQ(Pixel(dst, 0, 0), 0xF800);
	EXPECT_EQ(Pixel(dst, 1, 1), BLUE_565);
}


TEST(PNG, rgbaOutlineAndShadow)
{
	RGB565Format const format;
	BuildShadeTable();
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	ScopedClip const clip(SGPRect{ 0, 0, 3, 3 });

	SGPVSurface dst(3, 3, 16);
	dst.Fill(BLUE_565);
	BltVideoObjectOutline(&dst, vo.get(), 0, 0, 0, 0x07E0);
	EXPECT_EQ(Pixel(dst, 1, 0), 0x07E0); // outline
	EXPECT_EQ(Pixel(dst, 0, 1), 0x07E0);
	EXPECT_EQ(Pixel(dst, 2, 2), 0x07E0);
	EXPECT_EQ(Pixel(dst, 1, 1), 0xF800); // image
	EXPECT_EQ(Pixel(dst, 2, 1), BLACK_SUBSTITUTE);

	dst.Fill(BLUE_565);
	BltVideoObjectOutline(&dst, vo.get(), 0, 0, 0, SGP_TRANSPARENT);
	EXPECT_EQ(Pixel(dst, 1, 0), BLUE_565); // no outline
	EXPECT_EQ(Pixel(dst, 1, 1), 0xF800);

	dst.Fill(BLUE_565);
	BltVideoObjectOutlineShadow(&dst, vo.get(), 0, 0, 0);
	EXPECT_EQ(Pixel(dst, 1, 1), ShadeTable[BLUE_565]); // opaque: shaded
	EXPECT_EQ(Pixel(dst, 1, 2), ShadeTable[BLUE_565]); // alpha 128
	EXPECT_EQ(Pixel(dst, 0, 0), BLUE_565);             // alpha 100
}


TEST(PNG, rgbaThroughTransparentBlitter)
{
	// direct callers of the 8 bit transparent blitters (buttons, map markers)
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	SGPVSurface dst(3, 3, 16);
	dst.Fill(BLUE_565);
	SGPRect const clip{ 0, 0, 3, 3 };
	{
		SGPVSurface::Lock l(&dst);
		Blt8BPPDataTo16BPPBufferTransparentClip(l.Buffer<UINT16>(), l.Pitch(), vo.get(), 0, 0, 0, &clip);
	}
	EXPECT_EQ(Pixel(dst, 1, 1), 0xF800);
	EXPECT_EQ(Pixel(dst, 1, 2), 0x841F);

	dst.Fill(BLUE_565);
	{
		SGPVSurface::Lock l(&dst);
		Blt8BPPDataTo16BPPBufferTransparent(l.Buffer<UINT16>(), l.Pitch(), vo.get(), 0, 0, 0);
	}
	EXPECT_EQ(Pixel(dst, 2, 1), BLACK_SUBSTITUTE);
}


TEST_F(PNGLoadTest, rgbaVideoObjectFromFile)
{
	// the RGBA PNG next to the STI
	std::unique_ptr<SGPVObject> const rgba(AddVideoObjectFromFile("pngtest/rgba_next_to_sti.sti"));
	EXPECT_TRUE(rgba->IsRGBA());
	EXPECT_EQ(rgba->SubregionCount(), 1);
	EXPECT_EQ(rgba->SubregionProperties(0).usWidth, 4);
	EXPECT_EQ(rgba->SubregionProperties(0).usHeight, 3);

	// where the palette is used, the STI
	std::unique_ptr<SGPVObject> const pal(AddVideoObjectFromFile("pngtest/rgba_next_to_sti.sti", true));
	EXPECT_FALSE(pal->IsRGBA());
	EXPECT_EQ(pal->SubregionCount(), 2);

	// an RGBA PNG loaded directly where the palette is used
	EXPECT_THROW(AddVideoObjectFromFile("pngtest/rgba_surface.png", true), std::runtime_error);
}


TEST(PNG, rgbaWithoutOutline)
{
	RGB565Format const format;
	AutoSGPImage img(ConvertRGBAPNGToImage(SmallRGBAPNG(), { PNGFrame{ 0, 0, 3, 3, 0, 0 } },
		IMAGE_ALLIMAGEDATA, "test", false));
	EXPECT_EQ(img->fFlags & IMAGE_NO_OUTLINE, IMAGE_NO_OUTLINE);
	std::unique_ptr<SGPVObject> const vo(AddVideoObjectFromHImage(img.get()));
	EXPECT_TRUE(vo->OutlineMask(vo->SubregionProperties(0)) == nullptr);

	ScopedClip const clip(SGPRect{ 0, 0, 3, 3 });
	SGPVSurface dst(3, 3, 16);
	dst.Fill(BLUE_565);
	BltVideoObjectOutline(&dst, vo.get(), 0, 0, 0, 0x07E0);
	EXPECT_EQ(Pixel(dst, 1, 0), BLUE_565); // no outline
	EXPECT_EQ(Pixel(dst, 2, 2), BLUE_565);
	EXPECT_EQ(Pixel(dst, 1, 1), 0xF800);   // the image itself
}


TEST_F(PNGLoadTest, rgbaOutlineOffInMetadata)
{
	// rgba_no_outline.png.json: { "outline": false }
	std::unique_ptr<SGPVObject> const off(AddVideoObjectFromFile("pngtest/rgba_no_outline.png"));
	ASSERT_TRUE(off->IsRGBA());
	EXPECT_TRUE(off->OutlineMask(off->SubregionProperties(0)) == nullptr);

	std::unique_ptr<SGPVObject> const on(AddVideoObjectFromFile("pngtest/rgba_surface.png"));
	ASSERT_TRUE(on->IsRGBA());
	EXPECT_TRUE(on->OutlineMask(on->SubregionProperties(0)) != nullptr);
}


TEST_F(PNGLoadTest, replacementCanBeTurnedOff)
{
	cm->setImagePNGOverride(false);
	EXPECT_TRUE(GCM->getPNGReplacement("pngtest/replaced.sti").empty());
	AutoSGPImage const sti(CreateImage("pngtest/replaced.sti", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(sti->usNumberOfObjects, 2);

	// a PNG named directly is still loaded
	AutoSGPImage const png(CreateImage("pngtest/replaced.png", IMAGE_ALLIMAGEDATA));
	EXPECT_EQ(png->usNumberOfObjects, 1);
}


// ---------------------------------------------------------------------------
// Load time and memory of PNG images compared to the images they replace.
// Not run by default. Put pairs of <name>.png (+ <name>.png.json) and
// <name>.sti or <name>.pcx into <build dir>/unittests/data/pngbench/ and run
//   GTEST_ALSO_RUN_DISABLED_TESTS=1 GTEST_FILTER=PNGLoadTest.DISABLED_benchmark ja2 -unittests

namespace
{

template<typename F> double MillisecondsPerCall(F&& f)
{
	using Clock = std::chrono::steady_clock;
	f(); // warm up the file cache
	int n = 0;
	Clock::time_point const start = Clock::now();
	Clock::time_point now;
	do
	{
		f();
		++n;
		now = Clock::now();
	}
	while (n < 5 || (now - start < std::chrono::milliseconds(300) && n < 500));
	return std::chrono::duration<double, std::milli>(now - start).count() / n;
}

}


TEST_F(PNGLoadTest, DISABLED_benchmark)
{
	RGB565Format const format;
	std::vector<ST::string> const pngs = GCM->getAllFiles("pngbench", "png");
	if (pngs.empty()) GTEST_SKIP() << "no files in unittests/data/pngbench";

	std::printf("%-28s %-8s %8s %8s %6s %10s %10s\n",
		"image", "used as", "orig ms", "png ms", "ratio", "orig KiB", "png KiB");
	for (ST::string const& png : pngs)
	{
		ST::string const stem = png.substr(0, png.size() - 4);
		ST::string original;
		for (char const* const ext : { ".sti", ".pcx" })
		{
			if (GCM->doesGameResExists(stem + ext)) original = stem + ext;
		}
		if (original.empty()) continue;
		bool const isPCX = original.ends_with(".pcx", ST::case_insensitive);

		// as the original is used: ETRLE images become video objects, all others surfaces
		auto const loadOriginal = [&](UINT16 const contents) {
			return isPCX ? LoadPCXFileToImage(original, contents) : LoadSTCIFileToImage(original, contents);
		};
		AutoSGPImage const probe(loadOriginal(IMAGE_ALLIMAGEDATA));
		bool   const object   = (probe->fFlags & IMAGE_TRLECOMPRESSED) != 0;
		UINT16 const contents = IMAGE_ALLIMAGEDATA | (object ? 0 : IMAGE_FOR_SURFACE);
		AutoSGPImage const converted(LoadPNGFileToImage(png, contents));

		auto const imageBytes = [](SGPImage const& img) {
			size_t bytes = img.uiSizePixData != 0 ? img.uiSizePixData : size_t{img.usWidth} * img.usHeight * img.ubBitDepth / 8;
			if (img.fFlags & IMAGE_RGBA) bytes += bytes / 4; // outline mask
			return bytes;
		};

		double const msOriginal = MillisecondsPerCall([&] {
			AutoSGPImage img(loadOriginal(contents));
			if (object) std::unique_ptr<SGPVObject>(AddVideoObjectFromHImage(img.get()));
		});
		double const msPNG = MillisecondsPerCall([&] {
			AutoSGPImage img(LoadPNGFileToImage(png, contents));
			if (object) std::unique_ptr<SGPVObject>(AddVideoObjectFromHImage(img.get()));
		});

		ST::string const kind = ST::format("{}{}", object ? "object" : "surface",
			converted->ubBitDepth == 32 ? " RGBA" : "");
		std::printf("%-28s %-8s %8.3f %8.3f %6.1f %10.1f %10.1f\n",
			png.c_str(), kind.c_str(), msOriginal, msPNG, msPNG / msOriginal,
			imageBytes(*probe) / 1024.0, imageBytes(*converted) / 1024.0);
	}

	// the replacement lookup for images without a PNG: first through the VFS, then cached
	int const lookups = 500;
	auto const timeLookups = [&] {
		auto const start = std::chrono::steady_clock::now();
		for (int i = 0; i != lookups; ++i) GCM->getPNGReplacement(ST::format("pngbench/missing_{}.sti", i));
		return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / lookups;
	};
	double const first  = timeLookups();
	double const cached = timeLookups();
	std::printf("replacement lookup: %.1f us first, %.2f us cached\n", first, cached);
}
