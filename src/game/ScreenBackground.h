#pragma once

#include "VSurface.h"

#include <memory>


// Full screen backgrounds for screens whose original background is a 640x480
// image drawn in the middle of the screen (main menu, initial game options).
//
// For the original <dir>/<name>.sti the background of the current resolution
// is <dir>/<name>_<width>x<height>.png (or .sti / .pcx), e.g.
// loadscreens/mainmenubackground_1366x768.png. It is loaded as a 16 bit
// video surface of the screen size; an image of another size is stretched to
// it. Returns null if there is no such image or it cannot be loaded (the error
// is logged), the caller then draws the original.
std::unique_ptr<SGPVSurface> LoadResolutionBackground(char const* originalPath);

// Draws a background from LoadResolutionBackground() over the whole screen.
void DrawResolutionBackground(SGPVSurface* dst, SGPVSurface* background);
