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
#include "TileDef.h"
#include "Tile_Animation.h"
#include "Tile_Cache.h"
#include "Tile_Surface.h"
#include "Interactive_Tiles.h"
#include "VSurface.h"
#include "Animation_Data.h"
#include "Lighting.h"
#include "RenderWorld.h"

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


// ---------------------------------------------------------------------------
// Animations: frame durations

TEST(PNG, frameDurationsInFrames)
{
	PNGMetadata const meta = ParsePNGMetadata(R"({ "frameDuration": 70, "frames": [
		{ "x": 0, "y": 0, "w": 2, "h": 2, "duration": 50 },
		{ "x": 2, "y": 0, "w": 2, "h": 2 },
		{ "x": 4, "y": 0, "w": 2, "h": 2, "duration": 0 },
		{ "x": 6, "y": 0, "w": 2, "h": 2, "duration": 65535 } ] })", 8, 2);
	ASSERT_EQ(meta.frames.size(), 4u);
	EXPECT_EQ(meta.frames[0].duration, 50);
	EXPECT_EQ(meta.frames[1].duration, 70); // "frameDuration"
	EXPECT_EQ(meta.frames[2].duration, 70); // 0 = not given
	EXPECT_EQ(meta.frames[3].duration, 65535);
}


TEST(PNG, frameDurationsInGrid)
{
	PNGMetadata const meta = ParsePNGMetadata(
		R"({ "grid": { "w": 2, "h": 2, "count": 3 }, "durations": [ 10, 0, 30 ], "frameDuration": 99 })", 6, 2);
	ASSERT_EQ(meta.frames.size(), 3u);
	EXPECT_EQ(meta.frames[0].duration, 10);
	EXPECT_EQ(meta.frames[1].duration, 99);
	EXPECT_EQ(meta.frames[2].duration, 30);
}


TEST(PNG, frameDurationsDefaults)
{
	// no durations at all
	PNGMetadata const none = ParsePNGMetadata(R"({ "grid": { "w": 2, "h": 2 } })", 4, 2);
	for (PNGFrame const& f : none.frames) EXPECT_EQ(f.duration, 0);

	// only "frameDuration", also for the whole image as one frame
	PNGMetadata const whole = ParsePNGMetadata(R"({ "frameDuration": 120 })", 4, 2);
	ASSERT_EQ(whole.frames.size(), 1u);
	EXPECT_EQ(whole.frames[0].duration, 120);
}


TEST(PNG, invalidFrameDurationsFail)
{
	char const* const invalid[] = {
		R"({ "frameDuration": -1 })",
		R"({ "frameDuration": 65536 })",
		R"({ "frameDuration": "fast" })",
		R"({ "frameDuration": 1.5 })",
		R"({ "frames": [ { "x": 0, "y": 0, "w": 1, "h": 1, "duration": -5 } ] })",
		R"({ "frames": [ { "x": 0, "y": 0, "w": 1, "h": 1, "duration": true } ] })",
		R"({ "grid": { "w": 5, "h": 5, "count": 2 }, "durations": [ 10 ] })",
		R"({ "grid": { "w": 5, "h": 5, "count": 2 }, "durations": 10 })",
		R"({ "grid": { "w": 5, "h": 5, "count": 2 }, "durations": [ 10, "x" ] })",
		R"({ "durations": [ 10 ] })",                                           // no grid
		R"({ "frames": [ { "x": 0, "y": 0, "w": 1, "h": 1 } ], "durations": [ 10 ] })",
	};
	for (char const* const json : invalid)
	{
		SCOPED_TRACE(json);
		EXPECT_THROW(ParsePNGMetadata(json, 10, 10), std::runtime_error);
	}
}


TEST(PNG, frameDurationsInImageAndVideoObject)
{
	std::vector<PNGFrame> frames{ { 0, 0, 2, 1, 0, 0 }, { 2, 0, 2, 1, 0, 0 } };
	frames[0].duration = 40;
	frames[1].duration = 80;

	// palettised
	AutoSGPImage indexed(ConvertIndexedPNGToImage(SmallIndexedPNG(), frames, IMAGE_ALLIMAGEDATA, "test"));
	EXPECT_EQ(indexed->frameDurations, (std::vector<UINT16>{ 40, 80 }));
	std::unique_ptr<SGPVObject> const vo(AddVideoObjectFromHImage(indexed.get()));
	EXPECT_TRUE(vo->HasFrameDurations());
	EXPECT_EQ(vo->FrameDuration(0), 40);
	EXPECT_EQ(vo->FrameDuration(1), 80);
	EXPECT_EQ(vo->FrameDuration(2), 0); // out of range

	// RGBA
	frames = { { 0, 0, 3, 3, 0, 0 } };
	frames[0].duration = 25;
	AutoSGPImage rgba(ConvertRGBAPNGToImage(SmallRGBAPNG(), frames, IMAGE_ALLIMAGEDATA, "test"));
	EXPECT_EQ(rgba->frameDurations, (std::vector<UINT16>{ 25 }));

	// no durations: empty, as for STI files
	AutoSGPImage plain(ConvertIndexedPNGToImage(SmallIndexedPNG(), { PNGFrame{ 0, 0, 4, 2, 0, 0 } }, IMAGE_ALLIMAGEDATA, "test"));
	EXPECT_TRUE(plain->frameDurations.empty());
	std::unique_ptr<SGPVObject> const plainVO(AddVideoObjectFromHImage(plain.get()));
	EXPECT_FALSE(plainVO->HasFrameDurations());
	EXPECT_EQ(plainVO->FrameDuration(0), 0);
}


TEST(PNG, frameDurationOrOwnDelay)
{
	// what an animation waits for: the PNG duration, else its own delay
	std::vector<PNGFrame> frames{ { 0, 0, 1, 1, 0, 0 }, { 1, 0, 1, 1, 0, 0 }, { 2, 0, 1, 1, 0, 0 } };
	frames[0].duration = 400;
	frames[2].duration = 50;
	AutoSGPImage img(ConvertIndexedPNGToImage(SmallIndexedPNG(), frames, IMAGE_ALLIMAGEDATA, "test"));
	std::unique_ptr<SGPVObject> const vo(AddVideoObjectFromHImage(img.get()));
	EXPECT_EQ(vo->FrameDurationOr(0, 150), 400u);
	EXPECT_EQ(vo->FrameDurationOr(1, 150), 150u); // no duration of its own
	EXPECT_EQ(vo->FrameDurationOr(2, 150), 50u);
	EXPECT_EQ(vo->FrameDurationOr(3, 150), 150u); // out of range

	// no durations at all (as for STI files): always the own delay
	AutoSGPImage plain(ConvertIndexedPNGToImage(SmallIndexedPNG(), { PNGFrame{ 0, 0, 4, 2, 0, 0 } }, IMAGE_ALLIMAGEDATA, "test"));
	std::unique_ptr<SGPVObject> const plainVO(AddVideoObjectFromHImage(plain.get()));
	EXPECT_EQ(plainVO->FrameDurationOr(0, 150), 150u);
}


TEST_F(PNGLoadTest, animationsThroughVFS)
{
	// palettised, frames of different sizes and offsets (anim_frames.png.json)
	std::unique_ptr<SGPVObject> const pal(AddVideoObjectFromFile("pngtest/anim_frames.png"));
	ASSERT_EQ(pal->SubregionCount(), 3);
	EXPECT_FALSE(pal->IsRGBA());
	ETRLEObject const& f0 = pal->SubregionProperties(0);
	ETRLEObject const& f1 = pal->SubregionProperties(1);
	ETRLEObject const& f2 = pal->SubregionProperties(2);
	EXPECT_EQ(f0.usWidth, 6);
	EXPECT_EQ(f0.usHeight, 4);
	EXPECT_EQ(f0.sOffsetX, -1);
	EXPECT_EQ(f0.sOffsetY, 2);
	EXPECT_EQ(f1.usWidth, 3);
	EXPECT_EQ(f1.usHeight, 5);
	EXPECT_EQ(f1.sOffsetX, 4);
	EXPECT_EQ(f1.sOffsetY, -3);
	EXPECT_EQ(f2.usWidth, 8);
	EXPECT_EQ(f2.usHeight, 2);
	EXPECT_EQ(pal->FrameDuration(0), 50);
	EXPECT_EQ(pal->FrameDuration(1), 100);
	EXPECT_EQ(pal->FrameDuration(2), 70);

	// RGBA grid with "durations"
	std::unique_ptr<SGPVObject> const rgba(AddVideoObjectFromFile("pngtest/anim_grid_rgba.png"));
	ASSERT_EQ(rgba->SubregionCount(), 4);
	EXPECT_TRUE(rgba->IsRGBA());
	for (UINT16 i = 0; i != 4; ++i) EXPECT_EQ(rgba->FrameDuration(i), (i + 1) * 10);

	// a multi-frame PNG without durations, and an STI
	std::unique_ptr<SGPVObject> const noDurations(AddVideoObjectFromFile("pngtest/frames.png"));
	EXPECT_EQ(noDurations->SubregionCount(), 3);
	EXPECT_FALSE(noDurations->HasFrameDurations());
	std::unique_ptr<SGPVObject> const sti(AddVideoObjectFromFile("pngtest/etrle_only.sti"));
	EXPECT_FALSE(sti->HasFrameDurations());
}


TEST_F(PNGLoadTest, animationFramesRender)
{
	// first, middle and last frame of the RGBA grid at an offset position;
	// each cell is one colour: (n * 60, 255 - n * 60, 0)
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo(AddVideoObjectFromFile("pngtest/anim_grid_rgba.png"));
	ScopedClip const clip(SGPRect{ 0, 0, 8, 8 });
	for (UINT16 const frame : { UINT16{0}, UINT16{2}, UINT16{3} })
	{
		SCOPED_TRACE(frame);
		SGPVSurface dst(8, 8, 16);
		dst.Fill(0);
		BltVideoObject(&dst, vo.get(), frame, 2, 3);
		UINT16 const want = static_cast<UINT16>((frame * 60 >> 3) << 11 | ((255 - frame * 60) >> 2) << 5);
		EXPECT_EQ(Pixel(dst, 2, 3), want);
		EXPECT_EQ(Pixel(dst, 5, 6), want);
		EXPECT_EQ(Pixel(dst, 1, 3), 0);
		EXPECT_EQ(Pixel(dst, 6, 3), 0);
	}
}


// ---------------------------------------------------------------------------
// Animations with application data (tile cache animations, cursors)

TEST(PNG, animationMetadata)
{
	PNGMetadata const meta = ParsePNGMetadata(R"({ "animation": { "framesPerDirection": 3 }, "grid": { "w": 1, "h": 1 } })", 6, 1);
	EXPECT_EQ(meta.framesPerDirection, 3);
	EXPECT_EQ(ParsePNGMetadata("{}", 6, 1).framesPerDirection, 0);

	char const* const invalid[] = {
		R"({ "animation": 3 })",
		R"({ "animation": {} })",
		R"({ "animation": { "framesPerDirection": 0 } })",
		R"({ "animation": { "framesPerDirection": 256 } })",
		R"({ "animation": { "framesPerDirection": "3" } })",
		R"({ "animation": { "framesPerDirection": 2 } })", // more than the 1 frame
	};
	for (char const* const json : invalid)
	{
		SCOPED_TRACE(json);
		EXPECT_THROW(ParsePNGMetadata(json, 6, 1), std::runtime_error);
	}
}


TEST(PNG, convertIndexedWithAnimationAppData)
{
	DecodedPNG const png = SmallIndexedPNG(); // 4x2
	std::vector<PNGFrame> const frames{ { 0, 0, 1, 2, 0, 0 }, { 1, 0, 1, 2, 0, 0 }, { 2, 0, 1, 2, 0, 0 }, { 3, 0, 1, 2, 0, 0 } };

	AutoSGPImage const img(ConvertIndexedPNGToImage(png, frames, IMAGE_ALLDATA, "test", 2));
	EXPECT_EQ(img->fFlags & IMAGE_APPDATA, IMAGE_APPDATA);
	ASSERT_EQ(img->uiAppDataSize, 4 * sizeof(AuxObjectData));
	AuxObjectData const* const aux = reinterpret_cast<AuxObjectData const*>(static_cast<UINT8 const*>(img->pAppData));
	for (size_t i = 0; i != 4; ++i)
	{
		SCOPED_TRACE(i);
		bool const first = i % 2 == 0; // first frame of a direction
		EXPECT_EQ(aux[i].ubNumberOfFrames, first ? 2 : 0);
		EXPECT_EQ(aux[i].fFlags, first ? AUX_ANIMATED_TILE : 0);
		EXPECT_EQ(aux[i].ubCurrentFrame, 0);
		EXPECT_EQ(aux[i].usTileLocIndex, 0);
		EXPECT_EQ(aux[i].ubNumberOfTiles, 0);
		EXPECT_EQ(aux[i].ubWallOrientation, 0);
	}

	// no application data without IMAGE_APPDATA or without the animation section
	AutoSGPImage const noAppData(ConvertIndexedPNGToImage(png, frames, IMAGE_ALLIMAGEDATA, "test", 2));
	EXPECT_EQ(noAppData->uiAppDataSize, 0u);
	AutoSGPImage const noAnimation(ConvertIndexedPNGToImage(png, frames, IMAGE_ALLDATA, "test", 0));
	EXPECT_EQ(noAnimation->uiAppDataSize, 0u);
}


namespace
{

void ExpectSameAnimatedImage(SGPImage const& sti, SGPImage const& png)
{
	ASSERT_EQ(png.ubBitDepth, 8);
	ASSERT_EQ(png.usNumberOfObjects, sti.usNumberOfObjects);
	ETRLEObject const* const a = sti.pETRLEObject;
	ETRLEObject const* const b = png.pETRLEObject;
	for (size_t i = 0; i != sti.usNumberOfObjects; ++i)
	{
		EXPECT_EQ(b[i].uiDataOffset, a[i].uiDataOffset) << "frame " << i;
		EXPECT_EQ(b[i].sOffsetX,     a[i].sOffsetX)     << "frame " << i;
		EXPECT_EQ(b[i].sOffsetY,     a[i].sOffsetY)     << "frame " << i;
		EXPECT_EQ(b[i].usWidth,      a[i].usWidth)      << "frame " << i;
	}
	ASSERT_EQ(png.uiSizePixData, sti.uiSizePixData);
	UINT8 const* const stiData = sti.pImageData;
	UINT8 const* const pngData = png.pImageData;
	EXPECT_EQ(std::memcmp(pngData, stiData, sti.uiSizePixData), 0);

	// the application data is byte for byte the one of the STI
	ASSERT_EQ(png.uiAppDataSize, sti.uiAppDataSize);
	UINT8 const* const stiApp = sti.pAppData;
	UINT8 const* const pngApp = png.pAppData;
	EXPECT_EQ(std::memcmp(pngApp, stiApp, sti.uiAppDataSize), 0);
}

}


TEST_F(PNGLoadTest, worldAnimationReplacesSTI)
{
	AutoSGPImage const sti(LoadSTCIFileToImage("pngtest/anim_tile.sti", IMAGE_ALLDATA));
	AutoSGPImage const png(CreateImage("pngtest/anim_tile.sti", IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA));
	ExpectSameAnimatedImage(*sti, *png);
	EXPECT_EQ(png->frameDurations, (std::vector<UINT16>{ 20, 40, 60, 80, 100, 120 }));
}


TEST_F(PNGLoadTest, worldAnimationKeepsSTI)
{
	// without IMAGE_ANIMATION_METADATA (tilesets): no PNG for application data
	AutoSGPImage const tileset(CreateImage("pngtest/anim_tile.sti", IMAGE_ALLDATA));
	EXPECT_TRUE(tileset->frameDurations.empty());

	// a PNG without "animation": the STI is loaded
	AutoSGPImage const noAnimation(CreateImage("pngtest/anim_tile_noanim.sti", IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA));
	EXPECT_TRUE(noAnimation->frameDurations.empty());
	EXPECT_EQ(noAnimation->uiAppDataSize, 6 * sizeof(AuxObjectData));

	// an RGBA PNG where the palette is needed (characters, corpses, cursors): the STI
	AutoSGPImage const rgba(CreateImage("pngtest/anim_tile_rgba.sti", IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA | IMAGE_NEEDS_PALETTE));
	EXPECT_EQ(rgba->ubBitDepth, 8);
	EXPECT_EQ(rgba->usNumberOfObjects, 6);

	// such PNGs named directly fail
	EXPECT_THROW(CreateImage("pngtest/anim_tile_noanim.png", IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA), std::runtime_error);
	EXPECT_THROW(CreateImage("pngtest/anim_tile_rgba.png", IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA | IMAGE_NEEDS_PALETTE), std::runtime_error);
}


TEST_F(PNGLoadTest, worldAnimationRGBAReplacesSTI)
{
	// tile cache animations may be full colour: the same frames and
	// application data as the STI
	AutoSGPImage const sti(LoadSTCIFileToImage("pngtest/anim_tile_rgba.sti", IMAGE_ALLDATA));
	AutoSGPImage const png(CreateImage("pngtest/anim_tile_rgba.sti", IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA));
	ASSERT_EQ(png->ubBitDepth, 32);
	EXPECT_EQ(png->fFlags & IMAGE_RGBA, IMAGE_RGBA);
	ASSERT_EQ(png->usNumberOfObjects, sti->usNumberOfObjects);
	ETRLEObject const* const a = sti->pETRLEObject;
	ETRLEObject const* const b = png->pETRLEObject;
	for (size_t i = 0; i != sti->usNumberOfObjects; ++i)
	{
		EXPECT_EQ(b[i].sOffsetX, a[i].sOffsetX) << "frame " << i;
		EXPECT_EQ(b[i].sOffsetY, a[i].sOffsetY) << "frame " << i;
		EXPECT_EQ(b[i].usWidth,  a[i].usWidth)  << "frame " << i;
		EXPECT_EQ(b[i].usHeight, a[i].usHeight) << "frame " << i;
	}
	ASSERT_EQ(png->uiAppDataSize, sti->uiAppDataSize);
	UINT8 const* const stiApp = sti->pAppData;
	UINT8 const* const pngApp = png->pAppData;
	EXPECT_EQ(std::memcmp(pngApp, stiApp, sti->uiAppDataSize), 0);

	// through the tile cache loader, and not for corpses (needsPalette)
	TILE_IMAGERY* const effect = LoadTileSurface("pngtest/anim_tile_rgba.sti", true);
	TILE_IMAGERY* const corpse = LoadTileSurface("pngtest/anim_tile_rgba.sti", true, true);
	EXPECT_TRUE(effect->vo->IsRGBA());
	ASSERT_TRUE(effect->pAuxData != NULL);
	EXPECT_EQ(effect->pAuxData[0].ubNumberOfFrames, 3);
	EXPECT_EQ(effect->pAuxData[3].ubNumberOfFrames, 3);
	EXPECT_FALSE(corpse->vo->IsRGBA());
	DeleteTileSurface(effect);
	DeleteTileSurface(corpse);
}


TEST_F(PNGLoadTest, tileCacheAnimationTiming)
{
	// the tile cache loads with pngAnimation; tilesets do not
	TILE_IMAGERY* const pngTile = LoadTileSurface("pngtest/anim_tile.sti", true);
	TILE_IMAGERY* const stiTile = LoadTileSurface("pngtest/anim_tile.sti");
	ASSERT_TRUE(pngTile->pAuxData != NULL);
	EXPECT_EQ(pngTile->pAuxData[0].ubNumberOfFrames, 3);
	EXPECT_TRUE(pngTile->vo->HasFrameDurations());
	EXPECT_FALSE(stiTile->vo->HasFrameDurations());

	// GetAniTileFrameDelay() looks the image up in the tile cache
	TILE_CACHE_ELEMENT cache[2];
	cache[0].pImagery = pngTile;
	cache[1].pImagery = stiTile;
	TILE_CACHE_ELEMENT* const oldCache = gpTileCache;
	gpTileCache = cache;

	ANITILE a{};
	a.sDelay        = 80;
	a.sCachedTileID = 0;
	a.sCurrentFrame = 2;
	EXPECT_EQ(GetAniTileFrameDelay(a), 60u);   // the PNG duration of frame 2
	a.sCurrentFrame = 5;
	EXPECT_EQ(GetAniTileFrameDelay(a), 120u);
	a.sCachedTileID = 1;
	EXPECT_EQ(GetAniTileFrameDelay(a), 80u);   // STI: sDelay
	a.sCachedTileID = -1;
	EXPECT_EQ(GetAniTileFrameDelay(a), 80u);   // not a cached tile

	gpTileCache = oldCache;
	DeleteTileSurface(pngTile);
	DeleteTileSurface(stiTile);
}


// ---------------------------------------------------------------------------
// Full colour objects in the game world: Z-buffer blitter and hit test

namespace
{

// A 16 bit surface and a Z-buffer of the same size, Z-buffer filled with z.
struct ZTarget
{
	SGPVSurface         surface;
	std::vector<UINT16> zbuf;

	ZTarget(UINT16 const w, UINT16 const h, UINT16 const colour, UINT16 const z) :
		surface(w, h, 16), zbuf()
	{
		surface.Fill(colour);
		zbuf.assign(static_cast<size_t>(surface.GetSDLSurface().pitch / 2) * h, z);
	}

	UINT16& Z(int const x, int const y) { return zbuf[y * (surface.GetSDLSurface().pitch / 2) + x]; }

	void Blit(SGPVObject const* const vo, INT32 const x, INT32 const y, UINT16 const z, bool const writeZ, bool const translucent, SGPRect const& clip)
	{
		SGPVSurface::Lock l(&surface);
		Blt32BPPDataTo16BPPBufferAlphaZ(l.Buffer<UINT16>(), l.Pitch(), zbuf.data(), z, vo, x, y, 0, &clip, writeZ, translucent);
	}
};

}


TEST(PNG, rgbaZBlitterTestsZ)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	SGPRect const clip{ 0, 0, 3, 3 };

	// Z-buffer value <= object Z: drawn
	ZTarget equal(3, 3, BLUE_565, 10);
	equal.Blit(vo.get(), 0, 0, 10, false, false, clip);
	EXPECT_EQ(Pixel(equal.surface, 1, 1), 0xF800);
	EXPECT_EQ(Pixel(equal.surface, 2, 1), BLACK_SUBSTITUTE);
	EXPECT_EQ(Pixel(equal.surface, 1, 2), 0x841F);  // white at alpha 128 over blue

	// Z-buffer value > object Z: hidden
	ZTarget behind(3, 3, BLUE_565, 11);
	behind.Blit(vo.get(), 0, 0, 10, false, false, clip);
	for (int y = 0; y != 3; ++y)
	{
		for (int x = 0; x != 3; ++x) EXPECT_EQ(Pixel(behind.surface, x, y), BLUE_565) << x << "," << y;
	}

	// only one pixel hidden
	ZTarget one(3, 3, BLUE_565, 5);
	one.Z(1, 1) = 20;
	one.Blit(vo.get(), 0, 0, 10, false, false, clip);
	EXPECT_EQ(Pixel(one.surface, 1, 1), BLUE_565);
	EXPECT_EQ(Pixel(one.surface, 2, 1), BLACK_SUBSTITUTE);
}


TEST(PNG, rgbaZBlitterWritesZForOpaquePixels)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject();
	SGPRect const clip{ 0, 0, 3, 3 };

	ZTarget t(3, 3, BLUE_565, 5);
	t.Blit(vo.get(), 0, 0, 10, true, false, clip);
	EXPECT_EQ(t.Z(1, 1), 10); // alpha 255
	EXPECT_EQ(t.Z(2, 1), 10);
	EXPECT_EQ(t.Z(1, 2), 10); // alpha 128
	EXPECT_EQ(t.Z(0, 0), 5);  // alpha 100: drawn, but no Z
	EXPECT_EQ(t.Z(2, 2), 5);  // alpha 0
	EXPECT_NE(Pixel(t.surface, 0, 0), BLUE_565);

	// without writeZ the Z-buffer stays
	ZTarget n(3, 3, BLUE_565, 5);
	n.Blit(vo.get(), 0, 0, 10, false, false, clip);
	EXPECT_EQ(n.Z(1, 1), 5);
}


TEST(PNG, rgbaZBlitterTranslucentAndClipped)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = SmallRGBAObject({ PNGFrame{ 0, 0, 3, 3, 1, 1 } }); // offset 1,1

	// translucent: alpha halved, opaque red over blue becomes half and half
	ZTarget t(5, 5, BLUE_565, 0);
	t.Blit(vo.get(), 0, 0, 1, true, true, SGPRect{ 0, 0, 5, 5 });
	EXPECT_EQ(Pixel(t.surface, 2, 2), 0x800F);
	EXPECT_EQ(t.Z(2, 2), 1); // Z from the pixel's own alpha

	// clipped to the column x = 2 (frame drawn at 1,1)
	ZTarget c(5, 5, BLUE_565, 0);
	c.Blit(vo.get(), 0, 0, 1, true, false, SGPRect{ 2, 0, 3, 5 });
	EXPECT_EQ(Pixel(c.surface, 2, 2), 0xF800);  // frame pixel (1,1)
	EXPECT_EQ(Pixel(c.surface, 3, 2), BLUE_565); // frame pixel (2,1), clipped
	EXPECT_EQ(c.Z(3, 2), 0);
}


TEST(PNG, rgbaHitTestMatchesPalettisedHitTest)
{
	// the same shape as a palettised and as an RGBA object: opaque where
	// the RGBA alpha is >= 128
	std::unique_ptr<SGPVObject> const rgba = SmallRGBAObject();

	DecodedPNG pal;
	pal.kind   = DecodedPNG::Kind::Indexed;
	pal.width  = 3;
	pal.height = 3;
	pal.palette.assign(2, SGPPaletteEntry{ 0, 0, 0, 255 });
	DecodedPNG const src = SmallRGBAPNG();
	for (size_t i = 0; i != 9; ++i) pal.pixels.push_back(src.pixels[i * 4 + 3] >= 128 ? 1 : 0);
	AutoSGPImage palImage(ConvertIndexedPNGToImage(pal, { PNGFrame{ 0, 0, 3, 3, 0, 0 } }, IMAGE_ALLIMAGEDATA, "test"));
	std::unique_ptr<SGPVObject> const indexed(AddVideoObjectFromHImage(palImage.get()));

	int hits = 0;
	for (INT32 y = -1; y <= 5; ++y)
	{
		for (INT32 x = -1; x <= 5; ++x)
		{
			BOOLEAN const a = CheckVideoObjectScreenCoordinateInData(rgba.get(), 0, x, y);
			BOOLEAN const b = CheckVideoObjectScreenCoordinateInData(indexed.get(), 0, x, y);
			EXPECT_EQ(a, b) << "test point " << x << "," << y;
			hits += a;
		}
	}
	EXPECT_GT(hits, 0);

	// pixel (1,1): testX = 1 + 1, testY = height - 1
	EXPECT_TRUE(CheckVideoObjectScreenCoordinateInData(rgba.get(), 0, 2, 2));
	// pixel (0,0) has alpha 100
	EXPECT_FALSE(CheckVideoObjectScreenCoordinateInData(rgba.get(), 0, 1, 3));
}



// ---------------------------------------------------------------------------
// Full colour characters: shades instead of palette shade tables

namespace
{

// 256 colours spread over the colour cube
std::vector<SGPPaletteEntry> TestPalette()
{
	std::vector<SGPPaletteEntry> pal;
	for (UINT32 i = 0; i != 256; ++i)
	{
		pal.push_back(SGPPaletteEntry{ static_cast<UINT8>(i), static_cast<UINT8>(i * 7 + 13), static_cast<UINT8>(255 - i * 3), 255 });
	}
	return pal;
}

// The 16 bit colour of a palette entry changed by the shade, as a shade
// table entry
UINT16 Shaded16(RGBAShade const& shade, SGPPaletteEntry const& c)
{
	UINT8 r = c.r;
	UINT8 g = c.g;
	UINT8 b = c.b;
	ApplyRGBAShade(shade, r, g, b);
	return Get16BPPColor(FROMRGB(r, g, b));
}

}


TEST(PNG, rgbaShadeMatchesShadedPalette)
{
	RGB565Format const format;
	std::vector<SGPPaletteEntry> const pal = TestPalette();
	struct { UINT32 r, g, b; bool mono; } const cases[] = {
		{ 255, 255, 255, false }, { 100, 100, 100, true }, { 500, 500, 500, true },
		{ 115, 115, 160, false }, { 48, 222, 48, false }, { 0, 0, 0, false } };
	for (auto const& c : cases)
	{
		std::unique_ptr<UINT16[]> const table(Create16BPPPaletteShaded(pal.data(), c.r, c.g, c.b, c.mono));
		RGBAShade const shade = MakeRGBAShade(c.r, c.g, c.b, c.mono);
		for (size_t i = 0; i != 256; ++i)
		{
			EXPECT_EQ(Shaded16(shade, pal[i]), table[i]) << c.r << "," << c.g << "," << c.b << " colour " << i;
		}
	}
}


TEST(PNG, rgbaShadesMatchBiasedShadedPalettes)
{
	RGB565Format const format;
	std::vector<SGPPaletteEntry> const pal = TestPalette();
	SGPPaletteEntry const oldLight = g_light_color;
	g_light_color = SGPPaletteEntry{ 30, 0, 90, 0 };

	UINT16*   tables[16];
	RGBAShade shades[16];
	CreateBiasedShadedPalettes(tables, pal.data());
	CreateBiasedRGBAShades(shades);
	for (size_t level = 0; level != 16; ++level)
	{
		for (size_t i = 0; i != 256; ++i)
		{
			EXPECT_EQ(Shaded16(shades[level], pal[i]), tables[level][i]) << "level " << level << " colour " << i;
		}
		delete[] tables[level];
	}
	g_light_color = oldLight;
}


TEST(PNG, rgbaShadeMinimumAndWhite)
{
	// the enemy glow: red raised to at least the glow value
	RGBAShade glow = RGBA_SHADE_NONE;
	glow.minR = 200;
	UINT8 r = 10, g = 20, b = 30;
	ApplyRGBAShade(glow, r, g, b);
	EXPECT_EQ(r, 200);
	EXPECT_EQ(g, 20);
	EXPECT_EQ(b, 30);

	r = 0; g = 0; b = 0;
	ApplyRGBAShade(RGBA_SHADE_WHITE, r, g, b);
	EXPECT_EQ(r, 255);
	EXPECT_EQ(g, 255);
	EXPECT_EQ(b, 255);
}


namespace
{

// 4x1: red, shadow (black at alpha 128), opaque black, transparent
std::unique_ptr<SGPVObject> CharacterRGBAObject()
{
	DecodedPNG png;
	png.kind             = DecodedPNG::Kind::RGBA;
	png.width            = 4;
	png.height           = 1;
	png.sourceColourType = 6;
	png.sourceBitDepth   = 8;
	png.pixels = {
		255, 0, 0, 255,   0, 0, 0, 128,   0, 0, 0, 255,   0, 0, 0, 0 };
	AutoSGPImage img(ConvertRGBAPNGToImage(png, { PNGFrame{ 0, 0, 4, 1, 0, 0 } }, IMAGE_ALLIMAGEDATA, "test"));
	return std::unique_ptr<SGPVObject>(AddVideoObjectFromHImage(img.get()));
}

void BlitShaded(ZTarget& t, SGPVObject const* const vo, INT32 const x, INT32 const y, UINT16 const z, RGBAShade const& shade, bool const useZ, bool const writeZ, bool const obscured, bool const translucent)
{
	SGPRect const clip{ 0, 0, t.surface.Width(), t.surface.Height() };
	SGPVSurface::Lock l(&t.surface);
	Blt32BPPDataTo16BPPBufferShadeZ(l.Buffer<UINT16>(), l.Pitch(), useZ ? t.zbuf.data() : nullptr, z, vo, x, y, 0, &clip, shade, nullptr, writeZ, obscured, translucent);
}

}


TEST(PNG, rgbaShadeBlitterShadesButNotShadow)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = CharacterRGBAObject();

	// half brightness: red becomes 127; the shadow (alpha 128) darkens blue to 127;
	// opaque black stays black
	ZTarget t(4, 1, BLUE_565, 0);
	BlitShaded(t, vo.get(), 0, 0, 5, MakeRGBAShade(128, 128, 128, false), true, true, false, false);
	EXPECT_EQ(Pixel(t.surface, 0, 0), 0x7800);
	EXPECT_EQ(Pixel(t.surface, 1, 0), 0x000F);
	EXPECT_EQ(Pixel(t.surface, 2, 0), BLACK_SUBSTITUTE);
	EXPECT_EQ(Pixel(t.surface, 3, 0), BLUE_565);
	EXPECT_EQ(t.Z(0, 0), 5);
	EXPECT_EQ(t.Z(1, 0), 5); // alpha 128
	EXPECT_EQ(t.Z(3, 0), 0);

	// the white flash whitens everything but the shadow
	ZTarget w(4, 1, BLUE_565, 0);
	BlitShaded(w, vo.get(), 0, 0, 5, RGBA_SHADE_WHITE, true, false, false, false);
	EXPECT_EQ(Pixel(w.surface, 0, 0), 0xFFFF);
	EXPECT_EQ(Pixel(w.surface, 1, 0), 0x000F);
	EXPECT_EQ(Pixel(w.surface, 2, 0), 0xFFFF);
	EXPECT_EQ(w.Z(0, 0), 0); // no writeZ
}


TEST(PNG, rgbaShadeBlitterZObscuredAndNoZ)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = CharacterRGBAObject();

	// behind something: hidden
	ZTarget hidden(6, 2, BLUE_565, 9);
	BlitShaded(hidden, vo.get(), 1, 0, 5, RGBA_SHADE_NONE, true, false, false, false);
	EXPECT_EQ(Pixel(hidden.surface, 1, 0), BLUE_565);

	// obscured: every other pixel, (x & 1) == (y & 1) as the 8 bit blitters
	ZTarget even(6, 2, BLUE_565, 9);
	BlitShaded(even, vo.get(), 1, 0, 5, RGBA_SHADE_NONE, true, false, true, false);
	EXPECT_EQ(Pixel(even.surface, 1, 0), BLUE_565); // red: odd column on an even row
	EXPECT_EQ(Pixel(even.surface, 2, 0), 0x000F);   // shadow: even column
	EXPECT_EQ(Pixel(even.surface, 3, 0), BLUE_565); // opaque black: odd column
	ZTarget odd(6, 2, BLUE_565, 9);
	BlitShaded(odd, vo.get(), 1, 1, 5, RGBA_SHADE_NONE, true, false, true, false);
	EXPECT_EQ(Pixel(odd.surface, 1, 1), 0xF800);
	EXPECT_EQ(Pixel(odd.surface, 2, 1), BLUE_565);
	EXPECT_EQ(Pixel(odd.surface, 3, 1), BLACK_SUBSTITUTE);
	EXPECT_EQ(odd.Z(1, 1), 9); // a hidden pixel never writes Z

	// no Z-buffer: everything drawn; translucent halves alpha
	ZTarget noZ(4, 1, BLUE_565, 9);
	BlitShaded(noZ, vo.get(), 0, 0, 5, RGBA_SHADE_NONE, false, true, false, true);
	EXPECT_EQ(Pixel(noZ.surface, 0, 0), 0x800F);
	EXPECT_EQ(noZ.Z(0, 0), 9);
}


TEST(PNG, fullColourCharacterAnimations)
{
	using C = AnimationColours;
	// no palette colour changes: full colour
	EXPECT_EQ(GetAnimationSurfaceColours(CROWWALKING),          C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(ROBOTNWBREATH),        C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(COWSTANDING),          C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(CATBREATH),            C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(QUEENMONSTERSTANDING), C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(HUMVEE_BASIC),         C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(TANKNE_DIE),           C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(LVBREATH),             C::FullColour);
	EXPECT_EQ(GetAnimationSurfaceColours(IATTACK),              C::FullColour);
	// people: with a colour mask
	EXPECT_EQ(GetAnimationSurfaceColours(RGMSTANDING),          C::ColourMask);
	EXPECT_EQ(GetAnimationSurfaceColours(KIDCIVSTANDING),       C::ColourMask);
	EXPECT_EQ(GetAnimationSurfaceColours(BODYEXPLODE),          C::ColourMask);
	// adult creatures (.COL palettes): palettised only
	EXPECT_EQ(GetAnimationSurfaceColours(AFMONSTERSTANDING),    C::Palette);
	EXPECT_EQ(GetAnimationSurfaceColours(AFMMELT),              C::Palette);
}


namespace
{

// 45x1, opaque red, with Z strips: the first 5 columns wide, then 20 wide;
// the Z value goes up after the first strip and down after the second.
std::unique_ptr<SGPVObject> StripRGBAObject(size_t const transparentColumn = SIZE_MAX)
{
	DecodedPNG png;
	png.kind             = DecodedPNG::Kind::RGBA;
	png.width            = 45;
	png.height           = 1;
	png.sourceColourType = 6;
	png.sourceBitDepth   = 8;
	for (size_t x = 0; x != 45; ++x)
	{
		UINT8 const a = x == transparentColumn ? 0 : 255;
		png.pixels.insert(png.pixels.end(), { 255, 0, 0, a });
	}
	AutoSGPImage img(ConvertRGBAPNGToImage(png, { PNGFrame{ 0, 0, 45, 1, 0, 0 } }, IMAGE_ALLIMAGEDATA, "test"));
	std::unique_ptr<SGPVObject> vo(AddVideoObjectFromHImage(img.get()));

	auto z = std::make_unique<ZStripInfo>();
	z->pbZChange[0]       = 1;
	z->pbZChange[1]       = -1;
	z->bInitialZChange    = 0;
	z->ubFirstZStripWidth = 5;
	z->ubNumberOfZChanges = 2;
	vo->ppZStripInfo = std::make_unique<std::unique_ptr<ZStripInfo>[]>(1);
	vo->ppZStripInfo[0] = std::move(z);
	return vo;
}

void BlitStrips(ZTarget& t, SGPVObject const* const vo, SGPRect const& clip, bool const obscured)
{
	SGPVSurface::Lock l(&t.surface);
	Blt32BPPDataTo16BPPBufferShadeZStrips(l.Buffer<UINT16>(), l.Pitch(), t.zbuf.data(), 100, vo, 0, 0, 0, &clip, 0, Z_SUBLAYERS, RGBA_SHADE_NONE, nullptr, obscured);
}

}


TEST(PNG, rgbaZStripsStepAsThe8BitBlitter)
{
	RGB565Format const format;
	SGPRect const all{ 0, 0, 45, 1 };

	// crossing into the next strip on an opaque pixel: Z_SUBLAYERS
	std::unique_ptr<SGPVObject> const vo = StripRGBAObject();
	ZTarget t(45, 1, BLUE_565, 0);
	BlitStrips(t, vo.get(), all, false);
	EXPECT_EQ(t.Z(0, 0),  100);
	EXPECT_EQ(t.Z(4, 0),  100);
	EXPECT_EQ(t.Z(5, 0),  100 + Z_SUBLAYERS);
	EXPECT_EQ(t.Z(24, 0), 100 + Z_SUBLAYERS);
	EXPECT_EQ(t.Z(25, 0), 100);
	EXPECT_EQ(Pixel(t.surface, 30, 0), 0xF800);

	// on a transparent pixel: ten times that
	std::unique_ptr<SGPVObject> const gap = StripRGBAObject(4);
	ZTarget g(45, 1, BLUE_565, 0);
	BlitStrips(g, gap.get(), all, false);
	EXPECT_EQ(g.Z(4, 0),  0);
	EXPECT_EQ(Pixel(g.surface, 4, 0), BLUE_565);
	EXPECT_EQ(g.Z(5, 0),  100 + 10 * Z_SUBLAYERS);
	EXPECT_EQ(g.Z(25, 0), 100 + 9 * Z_SUBLAYERS);

	// hidden where the Z-buffer is in front
	ZTarget h(45, 1, BLUE_565, 0);
	h.Z(10, 0) = 100 + Z_SUBLAYERS + 1;
	BlitStrips(h, vo.get(), all, false);
	EXPECT_EQ(Pixel(h.surface, 10, 0), BLUE_565);
	EXPECT_EQ(Pixel(h.surface, 11, 0), 0xF800);

	// left clipped into the third strip: the same Z value
	ZTarget c(45, 1, BLUE_565, 0);
	BlitStrips(c, vo.get(), SGPRect{ 30, 0, 45, 1 }, false);
	EXPECT_EQ(c.Z(29, 0), 0);
	EXPECT_EQ(c.Z(30, 0), 100);
}


TEST(PNG, rgbaZStripsObscured)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = StripRGBAObject();

	// all behind: only the checkerboard, steps of ten times Z_SUBLAYERS
	ZTarget t(45, 2, BLUE_565, 1000);
	BlitStrips(t, vo.get(), SGPRect{ 0, 0, 45, 1 }, true);
	EXPECT_EQ(Pixel(t.surface, 0, 0), 0xF800);
	EXPECT_EQ(Pixel(t.surface, 1, 0), BLUE_565);
	EXPECT_EQ(t.Z(0, 0), 100);
	EXPECT_EQ(t.Z(1, 0), 1000);
	EXPECT_EQ(t.Z(6, 0), 100 + 10 * Z_SUBLAYERS);

	// in front: drawn where the Z-buffer is below
	ZTarget f(45, 1, BLUE_565, 0);
	BlitStrips(f, vo.get(), SGPRect{ 0, 0, 45, 1 }, true);
	EXPECT_EQ(Pixel(f.surface, 1, 0), 0xF800);
	EXPECT_EQ(f.Z(1, 0), 100);
}


// ---------------------------------------------------------------------------
// Full colour people: colour masks

namespace
{

// Palette with a grey range 10..13 going from light to dark (brightness 200,
// 150, 100, 50) and other colours elsewhere.
std::vector<SGPPaletteEntry> RecolourOriginal()
{
	std::vector<SGPPaletteEntry> pal(256, SGPPaletteEntry{ 1, 2, 3, 0 });
	UINT8 const greys[] = { 200, 150, 100, 50 };
	for (size_t k = 0; k != 4; ++k) pal[10 + k] = SGPPaletteEntry{ greys[k], greys[k], greys[k], 0 };
	pal[40] = SGPPaletteEntry{ 9, 8, 7, 0 };
	return pal;
}

// The same with the range in reds and index 40 changed
std::vector<SGPPaletteEntry> RecolourChanged()
{
	std::vector<SGPPaletteEntry> pal = RecolourOriginal();
	UINT8 const reds[] = { 240, 180, 120, 60 };
	for (size_t k = 0; k != 4; ++k) pal[10 + k] = SGPPaletteEntry{ reds[k], 0, 0, 0 };
	pal[40] = SGPPaletteEntry{ 70, 80, 90, 0 };
	return pal;
}

std::unique_ptr<RGBARecolour> MakeTestRecolour()
{
	auto rc = std::make_unique<RGBARecolour>();
	PaletteRange const range{ 10, 13 };
	std::vector<SGPPaletteEntry> const original = RecolourOriginal();
	std::vector<SGPPaletteEntry> const changed  = RecolourChanged();
	BuildRGBARecolour(*rc, original.data(), changed.data(), &range, 1);
	return rc;
}

void Recolour(RGBARecolour const& rc, UINT8 const index, UINT8 const r, UINT8 const g, UINT8 const b, UINT8 const er, UINT8 const eg, UINT8 const eb)
{
	UINT8 rr = r, gg = g, bb = b;
	ApplyRGBARecolour(rc, index, rr, gg, bb);
	EXPECT_EQ(rr, er) << "index " << int(index) << " from " << int(r) << "," << int(g) << "," << int(b);
	EXPECT_EQ(gg, eg);
	EXPECT_EQ(bb, eb);
}

}


TEST(PNG, recolourFollowsBrightnessOnTheRange)
{
	std::unique_ptr<RGBARecolour> const rc = MakeTestRecolour();

	// the original colours of the range become the changed ones, whichever
	// index of the range the mask names
	Recolour(*rc, 10, 200, 200, 200, 240, 0, 0);
	Recolour(*rc, 12, 150, 150, 150, 180, 0, 0);
	Recolour(*rc, 13, 50, 50, 50, 60, 0, 0);
	// in between: interpolated
	Recolour(*rc, 11, 125, 125, 125, 150, 0, 0);
	Recolour(*rc, 11, 175, 175, 175, 210, 0, 0);
	// a coloured pixel: by its brightness (0.299 * 255 = 76)
	Recolour(*rc, 11, 255, 0, 0, 91, 0, 0);
	// lighter or darker than the range: its lightest or darkest colour
	Recolour(*rc, 10, 255, 255, 255, 240, 0, 0);
	Recolour(*rc, 10, 0, 0, 0, 60, 0, 0);
	// outside the ranges: the colour of the index in the changed palette
	Recolour(*rc, 40, 1, 2, 3, 70, 80, 90);
	Recolour(*rc, 41, 1, 2, 3, 1, 2, 3);
}


TEST(PNG, recolourUnchangedPaletteKeepsTheRange)
{
	auto rc = std::make_unique<RGBARecolour>();
	PaletteRange const range{ 10, 13 };
	std::vector<SGPPaletteEntry> const original = RecolourOriginal();
	BuildRGBARecolour(*rc, original.data(), original.data(), &range, 1);
	Recolour(*rc, 10, 150, 150, 150, 150, 150, 150);
	Recolour(*rc, 12, 120, 120, 120, 120, 120, 120);
	EXPECT_EQ(rc->range[9], RGBARecolour::NO_RANGE);
	EXPECT_EQ(rc->range[13], 0);
}


TEST(PNG, colourMaskFileName)
{
	EXPECT_EQ(ColourMaskFileName("anims/s_merc/s_r_std.png"), "anims/s_merc/s_r_std.mask.png");
	EXPECT_EQ(ColourMaskFileName("A/B.PNG"), "A/B.mask.png");
}


namespace
{

// A 4x1 character: grey 150, shadow (black at alpha 128), opaque black,
// transparent; its colour mask names range 10..13 for the first two pixels,
// none for the black one and index 40 for the transparent one.
std::unique_ptr<SGPVObject> MaskedCharacterObject()
{
	DecodedPNG png;
	png.kind             = DecodedPNG::Kind::RGBA;
	png.width            = 4;
	png.height           = 1;
	png.sourceColourType = 6;
	png.sourceBitDepth   = 8;
	png.pixels = { 150, 150, 150, 255,   0, 0, 0, 128,   0, 0, 0, 255,   0, 0, 0, 0 };
	std::vector<PNGFrame> const frames{ PNGFrame{ 0, 0, 4, 1, 0, 0 } };
	AutoSGPImage img(ConvertRGBAPNGToImage(png, frames, IMAGE_ALLIMAGEDATA, "test"));

	DecodedPNG mask;
	mask.kind   = DecodedPNG::Kind::Indexed;
	mask.width  = 4;
	mask.height = 1;
	mask.palette.assign(256, SGPPaletteEntry{});
	mask.pixels = { 12, 12, 0, 40 };
	AddColourMask(*img, mask, frames, "test mask");
	return std::unique_ptr<SGPVObject>(AddVideoObjectFromHImage(img.get()));
}

}


TEST(PNG, colourMaskOfFrames)
{
	DecodedPNG const png = SmallRGBAPNG();
	std::vector<PNGFrame> const frames{ { 1, 1, 2, 2, 0, 0 }, { 0, 0, 3, 1, 0, 0 } };
	AutoSGPImage img(ConvertRGBAPNGToImage(png, frames, IMAGE_ALLIMAGEDATA, "test"));

	DecodedPNG mask;
	mask.kind   = DecodedPNG::Kind::Indexed;
	mask.width  = 3;
	mask.height = 3;
	mask.palette.assign(256, SGPPaletteEntry{});
	mask.pixels = { 1, 2, 3,  4, 5, 6,  7, 8, 9 };
	AddColourMask(*img, mask, frames, "test");
	EXPECT_EQ(img->colourMask, (std::vector<UINT8>{ 5, 6, 8, 9,  1, 2, 3 }));

	// the colour mask must be palettised and of the same size
	DecodedPNG wrongSize = mask;
	wrongSize.width = 2;
	EXPECT_THROW(AddColourMask(*img, wrongSize, frames, "test"), std::runtime_error);
	EXPECT_THROW(AddColourMask(*img, png, frames, "test"), std::runtime_error);

	std::unique_ptr<SGPVObject> const vo(AddVideoObjectFromHImage(img.get()));
	UINT8 const* const m1 = vo->ColourMask(vo->SubregionProperties(1));
	ASSERT_TRUE(m1 != nullptr);
	EXPECT_EQ(m1[0], 1);
	std::unique_ptr<SGPVObject> const plain = SmallRGBAObject();
	EXPECT_TRUE(plain->ColourMask(plain->SubregionProperties(0)) == nullptr);
}


TEST(PNG, rgbaShadeBlitterRecolours)
{
	RGB565Format const format;
	std::unique_ptr<SGPVObject> const vo = MaskedCharacterObject();
	std::unique_ptr<RGBARecolour> const rc = MakeTestRecolour();
	SGPRect const clip{ 0, 0, 4, 1 };

	// grey 150 on the range becomes red 180, then halved by the shade; the
	// shadow stays a shadow; mask 0 keeps the pixel's colour
	ZTarget t(4, 1, BLUE_565, 0);
	{
		SGPVSurface::Lock l(&t.surface);
		Blt32BPPDataTo16BPPBufferShadeZ(l.Buffer<UINT16>(), l.Pitch(), t.zbuf.data(), 5, vo.get(), 0, 0, 0, &clip, MakeRGBAShade(128, 128, 128, false), rc.get(), false, false, false);
	}
	EXPECT_EQ(Pixel(t.surface, 0, 0), 0x5800); // 90 >> 3 = 11
	EXPECT_EQ(Pixel(t.surface, 1, 0), 0x000F);
	EXPECT_EQ(Pixel(t.surface, 2, 0), BLACK_SUBSTITUTE);
	EXPECT_EQ(Pixel(t.surface, 3, 0), BLUE_565);

	// without recolouring: the pixel's own colour
	ZTarget n(4, 1, BLUE_565, 0);
	{
		SGPVSurface::Lock l(&n.surface);
		Blt32BPPDataTo16BPPBufferShadeZ(l.Buffer<UINT16>(), l.Pitch(), n.zbuf.data(), 5, vo.get(), 0, 0, 0, &clip, RGBA_SHADE_NONE, nullptr, false, false, false);
	}
	EXPECT_EQ(Pixel(n.surface, 0, 0), 0x94B2); // 150, 150, 150
}


TEST_F(PNGLoadTest, characterNeedsColourMask)
{
	UINT16 const flags = IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA | IMAGE_COLOUR_MASK;

	// with its colour mask: full colour, the mask from <name>.mask.png
	AutoSGPImage const masked(CreateImage("pngtest/anim_tile_rgba.sti", flags));
	ASSERT_EQ(masked->ubBitDepth, 32);
	ASSERT_EQ(masked->colourMask.size(), masked->uiSizePixData / 4);
	// the mask of frame 2 is the part of the mask PNG at its place on the
	// sheet (x = 14, see the generator)
	DecodedPNG const maskPNG = DecodePNGFile("pngtest/anim_tile_rgba.mask.png");
	ETRLEObject const& e = masked->pETRLEObject[2];
	int nonZero = 0;
	for (UINT16 y = 0; y != e.usHeight; ++y)
	{
		for (UINT16 x = 0; x != e.usWidth; ++x)
		{
			UINT8 const m = masked->colourMask[e.uiDataOffset / 4 + y * e.usWidth + x];
			EXPECT_EQ(m, maskPNG.pixels[y * maskPNG.width + 14 + x]) << x << "," << y;
			nonZero += m != 0;
		}
	}
	EXPECT_GT(nonZero, 0);

	// without one: the STI
	AutoSGPImage const unmasked(CreateImage("pngtest/anim_tile_rgba_nomask.sti", flags));
	EXPECT_EQ(unmasked->ubBitDepth, 8);
	EXPECT_THROW(CreateImage("pngtest/anim_tile_rgba_nomask.png", flags), std::runtime_error);

	// tile cache animations do not use the mask
	AutoSGPImage const effect(CreateImage("pngtest/anim_tile_rgba.sti", IMAGE_ALLDATA | IMAGE_ANIMATION_METADATA));
	EXPECT_TRUE(effect->colourMask.empty());
}


// ---------------------------------------------------------------------------
// SGPVObject::GetETRLEPixelValue()

TEST(PNG, etrlePixelValueOfEveryPixel)
{
	// 300x4 indices with transparent and opaque runs longer than an ETRLE run
	// (127), short runs and single pixels; index 0 is transparent
	DecodedPNG png;
	png.kind   = DecodedPNG::Kind::Indexed;
	png.width  = 300;
	png.height = 4;
	png.palette.assign(256, SGPPaletteEntry{ 0, 0, 0, 255 });
	for (UINT16 y = 0; y != png.height; ++y)
	{
		for (UINT16 x = 0; x != png.width; ++x)
		{
			UINT8 v;
			switch (y)
			{
				case 0:  v = x < 200 ? 0 : 1 + x % 7;          break; // long transparent run
				case 1:  v = x < 150 ? 1 + x % 250 : 0;        break; // long opaque run, transparent end
				case 2:  v = (x / 3) % 2 ? 0 : 1 + (x + y) % 9; break; // short runs
				default: v = x % 2 ? 0 : 200;                  break; // single pixels
			}
			png.pixels.push_back(v);
		}
	}
	AutoSGPImage img(ConvertIndexedPNGToImage(png, { PNGFrame{ 0, 0, png.width, png.height, 0, 0 } }, IMAGE_ALLIMAGEDATA, "test"));
	std::unique_ptr<SGPVObject> const vo(AddVideoObjectFromHImage(img.get()));

	for (UINT16 y = 0; y != png.height; ++y)
	{
		for (UINT16 x = 0; x != png.width; ++x)
		{
			EXPECT_EQ(vo->GetETRLEPixelValue(0, x, y), png.pixels[y * png.width + x]) << x << "," << y;
		}
	}
	EXPECT_THROW(vo->GetETRLEPixelValue(0, png.width, 0), std::logic_error);
	EXPECT_THROW(vo->GetETRLEPixelValue(0, 0, png.height), std::logic_error);
}


TEST_F(PNGLoadTest, etrlePixelValueOfSTIFrames)
{
	// the frames of anim_tile_rgba.sti have the palette indices of its colour
	// mask (frame n at x = 7 * n on the sheet, see the generator)
	AutoSGPImage const sti(LoadSTCIFileToImage("pngtest/anim_tile_rgba.sti", IMAGE_ALLDATA));
	std::unique_ptr<SGPVObject> const vo(AddVideoObjectFromHImage(sti.get()));
	DecodedPNG const mask = DecodePNGFile("pngtest/anim_tile_rgba.mask.png");
	for (UINT16 n = 0; n != vo->SubregionCount(); ++n)
	{
		ETRLEObject const& e = vo->SubregionProperties(n);
		for (UINT16 y = 0; y != e.usHeight; ++y)
		{
			for (UINT16 x = 0; x != e.usWidth; ++x)
			{
				EXPECT_EQ(vo->GetETRLEPixelValue(n, x, y), mask.pixels[y * mask.width + 7 * n + x]) << "frame " << n << " " << x << "," << y;
			}
		}
	}
}
