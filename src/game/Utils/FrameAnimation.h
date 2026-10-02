#pragma once

#include "Types.h"
#include "VSurface.h"

#include <memory>
#include <vector>


// Plays subimages of a video object one after the other, each for its
// duration from the PNG metadata (SGPVObject::FrameDuration()), or for the
// animation's own delay where the PNG gives none. See docs/png-images.md.
class FrameAnimation
{
public:
	struct State
	{
		UINT16 frame;    // the subimage to show
		bool   changed;  // a different subimage than at the last Update()
		bool   finished; // the last subimage has been shown for its duration
	};

	// sequence: the subimages in playing order (an index may repeat).
	void Start(SGPVObject const& vo, std::vector<UINT16> sequence, UINT32 ownDelay, UINT32 now);

	// The state at time now (milliseconds, the clock given to Start()). The
	// first call after Start() always reports a change.
	State Update(UINT32 now);

	bool Started() const { return !sequence_.empty(); }

private:
	std::vector<UINT16> sequence_;
	std::vector<UINT32> ends_;  // end of each step, relative to start_
	UINT32              start_ = 0;
	size_t              shown_ = SIZE_MAX;
};


// A copy of an area of a 16 bit video surface, to draw each animation frame on
// the same background instead of on the previous frame (frames with
// transparent or half transparent pixels). It keeps plain pixels, not a video
// surface, so it can live in a static variable (all video surfaces are freed
// at shutdown).
class ScreenAreaBackup
{
public:
	void Save(SGPVSurface* src, SGPBox const& area);
	void Restore(SGPVSurface* dst) const;
	bool Saved() const { return !pixels_.empty(); }

private:
	std::vector<UINT16> pixels_;
	SGPBox              area_{};
};
