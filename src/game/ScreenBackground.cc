#include "ScreenBackground.h"

#include "ContentManager.h"
#include "GameInstance.h"
#include "Logger.h"
#include "UILayout.h"

#include <string_theory/format>
#include <string_theory/string>

#include <exception>


std::unique_ptr<SGPVSurface> LoadResolutionBackground(char const* const originalPath)
{
	ST::string const original{ originalPath };
	ST::string const base = ST::format("{}_{}x{}", original.before_last('.'), SCREEN_WIDTH, SCREEN_HEIGHT);

	for (char const* const ext : { ".png", ".sti", ".pcx" })
	{
		ST::string const file = base + ext;
		if (!GCM->doesGameResExists(file)) continue;

		try
		{
			std::unique_ptr<SGPVSurface> const image(AddVideoSurfaceFromFile(file.c_str()));
			auto background = std::make_unique<SGPVSurface>(SCREEN_WIDTH, SCREEN_HEIGHT, 16);
			if (image->Width() == SCREEN_WIDTH && image->Height() == SCREEN_HEIGHT)
			{
				BltVideoSurface(background.get(), image.get(), 0, 0, nullptr);
			}
			else
			{
				SLOGW("{} is {}x{}, stretched to the {}x{} screen",
					file, image->Width(), image->Height(), SCREEN_WIDTH, SCREEN_HEIGHT);
				FillVideoSurfaceWithStretch(background.get(), image.get());
			}
			return background;
		}
		catch (std::exception const& e)
		{
			SLOGE("Cannot use {} as background, using {}: {}", file, original, e.what());
			return nullptr;
		}
	}
	return nullptr;
}


void DrawResolutionBackground(SGPVSurface* const dst, SGPVSurface* const background)
{
	BltVideoSurface(dst, background, 0, 0, nullptr);
}
