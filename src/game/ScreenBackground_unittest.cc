#include "gtest/gtest.h"

#include "DefaultContentManagerUT.h"
#include "HImage.h"
#include "ScreenBackground.h"
#include "UILayout.h"

// The test files are made by tools/generate_png_unittest_data.py.

namespace
{

class ScreenBackgroundTest : public DefaultContentManagerUT::BaseTest
{
protected:
	ScreenBackgroundTest() :
		width_{ SCREEN_WIDTH }, height_{ SCREEN_HEIGHT },
		red_{ gusRedMask }, green_{ gusGreenMask }, blue_{ gusBlueMask },
		redShift_{ gusRedShift }, greenShift_{ gusGreenShift }, blueShift_{ gusBlueShift }
	{
		g_ui.setScreenSize(1024, 720);
		gusRedMask    = 0xF800;
		gusGreenMask  = 0x07E0;
		gusBlueMask   = 0x001F;
		gusRedShift   = 8;
		gusGreenShift = 3;
		gusBlueShift  = -3;
	}

	~ScreenBackgroundTest()
	{
		g_ui.setScreenSize(width_, height_);
		gusRedMask    = red_;
		gusGreenMask  = green_;
		gusBlueMask   = blue_;
		gusRedShift   = redShift_;
		gusGreenShift = greenShift_;
		gusBlueShift  = blueShift_;
	}

	static UINT16 Pixel(SGPVSurface const& s, int const x, int const y)
	{
		SDL_Surface const& sdl = s.GetSDLSurface();
		return reinterpret_cast<UINT16 const*>(static_cast<UINT8 const*>(sdl.pixels) + y * sdl.pitch)[x];
	}

private:
	UINT16 width_, height_;
	UINT16 red_, green_, blue_;
	INT16  redShift_, greenShift_, blueShift_;
};

}


TEST_F(ScreenBackgroundTest, exactSize)
{
	std::unique_ptr<SGPVSurface> const bg = LoadResolutionBackground("pngtest/bg.sti");
	ASSERT_TRUE(bg != nullptr);
	EXPECT_EQ(bg->Width(), 1024);
	EXPECT_EQ(bg->Height(), 720);
	EXPECT_EQ(bg->BPP(), 16);
	EXPECT_EQ(Pixel(*bg, 0, 0), 0xF800);
	EXPECT_EQ(Pixel(*bg, 511, 719), 0xF800);
	EXPECT_EQ(Pixel(*bg, 512, 0), 0x001F);
	EXPECT_EQ(Pixel(*bg, 1023, 719), 0x001F);
}


TEST_F(ScreenBackgroundTest, otherSizeIsStretched)
{
	// bgsmall_1024x720.png is 4x3: the left quarter green, the rest white
	std::unique_ptr<SGPVSurface> const bg = LoadResolutionBackground("pngtest/bgsmall.sti");
	ASSERT_TRUE(bg != nullptr);
	EXPECT_EQ(bg->Width(), 1024);
	EXPECT_EQ(bg->Height(), 720);
	EXPECT_EQ(Pixel(*bg, 0, 0), 0x07E0);
	EXPECT_EQ(Pixel(*bg, 255, 719), 0x07E0);
	EXPECT_EQ(Pixel(*bg, 256, 0), 0xFFFF);
	EXPECT_EQ(Pixel(*bg, 1023, 719), 0xFFFF);
}


TEST_F(ScreenBackgroundTest, missingOrOtherResolution)
{
	EXPECT_TRUE(LoadResolutionBackground("pngtest/nothing.sti") == nullptr);

	// bg_1024x720.png exists, but not for 1280x720
	g_ui.setScreenSize(1280, 720);
	EXPECT_TRUE(LoadResolutionBackground("pngtest/bg.sti") == nullptr);
}
