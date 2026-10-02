#include "FrameAnimation.h"

#include "VObject.h"

#include <algorithm>
#include <cstdint>
#include <utility>


void FrameAnimation::Start(SGPVObject const& vo, std::vector<UINT16> sequence, UINT32 const ownDelay, UINT32 const now)
{
	sequence_ = std::move(sequence);
	ends_.clear();
	UINT32 end = 0;
	for (UINT16 const frame : sequence_)
	{
		end += vo.FrameDurationOr(frame, ownDelay);
		ends_.push_back(end);
	}
	start_ = now;
	shown_ = SIZE_MAX;
}


FrameAnimation::State FrameAnimation::Update(UINT32 const now)
{
	if (sequence_.empty()) return State{ 0, false, true };

	UINT32 const elapsed = now - start_;
	size_t step = 0;
	while (step != ends_.size() && elapsed >= ends_[step]) ++step;

	bool const finished = step == ends_.size();
	if (finished) step = ends_.size() - 1;

	bool const changed = step != shown_;
	shown_ = step;
	return State{ sequence_[step], changed, finished };
}


void ScreenAreaBackup::Save(SGPVSurface* const src, SGPBox const& area)
{
	pixels_.clear();
	if (src->BPP() != 16) return;

	// only the part inside the surface
	area_.x = std::min<UINT16>(area.x, src->Width());
	area_.y = std::min<UINT16>(area.y, src->Height());
	area_.w = std::min<UINT16>(area.w, static_cast<UINT16>(src->Width()  - area_.x));
	area_.h = std::min<UINT16>(area.h, static_cast<UINT16>(src->Height() - area_.y));
	if (area_.w == 0 || area_.h == 0) return;

	SGPVSurface::Lock l(src);
	UINT8 const* const base  = l.Buffer<UINT8>();
	UINT32       const pitch = l.Pitch();
	pixels_.resize(size_t{area_.w} * area_.h);
	for (UINT16 y = 0; y != area_.h; ++y)
	{
		UINT16 const* const row = reinterpret_cast<UINT16 const*>(base + (area_.y + y) * pitch) + area_.x;
		std::copy(row, row + area_.w, pixels_.begin() + size_t{y} * area_.w);
	}
}


void ScreenAreaBackup::Restore(SGPVSurface* const dst) const
{
	if (pixels_.empty() || dst->BPP() != 16) return;
	if (area_.x + area_.w > dst->Width() || area_.y + area_.h > dst->Height()) return;

	SGPVSurface::Lock l(dst);
	UINT8* const base  = l.Buffer<UINT8>();
	UINT32 const pitch = l.Pitch();
	for (UINT16 y = 0; y != area_.h; ++y)
	{
		UINT16* const row = reinterpret_cast<UINT16*>(base + (area_.y + y) * pitch) + area_.x;
		auto const first = pixels_.begin() + size_t{y} * area_.w;
		std::copy(first, first + area_.w, row);
	}
}


#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"

#include "HImage.h"
#include "PNG.h"

namespace
{

// A palettised video object with one 1x1 subimage per duration.
std::unique_ptr<SGPVObject> ObjectWithDurations(std::vector<UINT16> const& durations)
{
	DecodedPNG png;
	png.kind   = DecodedPNG::Kind::Indexed;
	png.width  = static_cast<UINT16>(durations.size());
	png.height = 1;
	png.palette.assign(2, SGPPaletteEntry{ 0, 0, 0, 255 });
	png.pixels.assign(durations.size(), 1);

	std::vector<PNGFrame> frames;
	for (size_t i = 0; i != durations.size(); ++i)
	{
		frames.push_back(PNGFrame{ static_cast<UINT16>(i), 0, 1, 1, 0, 0 });
		frames.back().duration = durations[i];
	}
	AutoSGPImage img(ConvertIndexedPNGToImage(png, frames, IMAGE_ALLIMAGEDATA, "test"));
	return std::unique_ptr<SGPVObject>(AddVideoObjectFromHImage(img.get()));
}

void ExpectState(FrameAnimation::State const& s, UINT16 const frame, bool const changed, bool const finished)
{
	EXPECT_EQ(s.frame, frame);
	EXPECT_EQ(s.changed, changed);
	EXPECT_EQ(s.finished, finished);
}

}


TEST(FrameAnimation, durationsAndOwnDelay)
{
	// frame 1 has no duration of its own: the own delay (50)
	std::unique_ptr<SGPVObject> const vo = ObjectWithDurations({ 100, 0, 30 });
	FrameAnimation a;
	a.Start(*vo, { 0, 1, 2 }, 50, 1000);
	ExpectState(a.Update(1000), 0, true,  false);
	ExpectState(a.Update(1099), 0, false, false);
	ExpectState(a.Update(1100), 1, true,  false);
	ExpectState(a.Update(1149), 1, false, false);
	ExpectState(a.Update(1150), 2, true,  false);
	ExpectState(a.Update(1179), 2, false, false);
	ExpectState(a.Update(1180), 2, false, true);
	ExpectState(a.Update(5000), 2, false, true);
}


TEST(FrameAnimation, skipsFramesWhenLate)
{
	std::unique_ptr<SGPVObject> const vo = ObjectWithDurations({ 10, 10, 10, 10 });
	FrameAnimation a;
	a.Start(*vo, { 0, 1, 2, 3 }, 0, 0);
	ExpectState(a.Update(0),  0, true, false);
	ExpectState(a.Update(25), 2, true, false); // frame 1 was never due at an update
}


TEST(FrameAnimation, sequenceWithRepeatedFrames)
{
	std::unique_ptr<SGPVObject> const vo = ObjectWithDurations({ 20, 40 });
	FrameAnimation a;
	a.Start(*vo, { 0, 1, 0, 1 }, 0, 0);
	ExpectState(a.Update(0),   0, true,  false);
	ExpectState(a.Update(20),  1, true,  false);
	ExpectState(a.Update(60),  0, true,  false); // the same subimage again is a change
	ExpectState(a.Update(80),  1, true,  false);
	ExpectState(a.Update(120), 1, false, true);
}


TEST(FrameAnimation, noDurationsUsesOwnDelay)
{
	std::unique_ptr<SGPVObject> const vo = ObjectWithDurations({ 0, 0 });
	EXPECT_FALSE(vo->HasFrameDurations());
	FrameAnimation a;
	a.Start(*vo, { 0, 1 }, 150, 0);
	ExpectState(a.Update(149), 0, true,  false);
	ExpectState(a.Update(150), 1, true,  false);
	ExpectState(a.Update(300), 1, false, true);
}


TEST(FrameAnimation, emptySequence)
{
	FrameAnimation a;
	EXPECT_FALSE(a.Started());
	ExpectState(a.Update(0), 0, false, true);
}


TEST(FrameAnimation, screenAreaBackup)
{
	SGPVSurface screen(8, 6, 16);
	screen.Fill(0x1234);
	ScreenAreaBackup backup;
	EXPECT_FALSE(backup.Saved());
	backup.Save(&screen, SGPBox{ 2, 1, 4, 3 });
	EXPECT_TRUE(backup.Saved());

	screen.Fill(0x0F0F);
	backup.Restore(&screen);

	SDL_Surface const& s = screen.GetSDLSurface();
	auto const px = [&](int x, int y) {
		return reinterpret_cast<UINT16 const*>(static_cast<UINT8 const*>(s.pixels) + y * s.pitch)[x];
	};
	EXPECT_EQ(px(2, 1), 0x1234);
	EXPECT_EQ(px(5, 3), 0x1234);
	EXPECT_EQ(px(1, 1), 0x0F0F); // outside the area
	EXPECT_EQ(px(6, 1), 0x0F0F);
	EXPECT_EQ(px(2, 4), 0x0F0F);

	// an area partly outside the surface keeps only the part inside
	screen.Fill(0x1111);
	backup.Save(&screen, SGPBox{ 6, 4, 10, 10 });
	EXPECT_TRUE(backup.Saved());
	screen.Fill(0x2222);
	backup.Restore(&screen);
	EXPECT_EQ(px(7, 5), 0x1111);
	EXPECT_EQ(px(5, 5), 0x2222);
}

#endif
