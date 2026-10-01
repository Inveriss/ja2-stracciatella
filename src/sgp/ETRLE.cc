#include "ETRLE.h"

#include "ImgFmt.h"


void EncodeETRLE(UINT8 const* pixels, size_t const pitch, UINT16 const width, UINT16 const height,
	ETRLETransparency const& transparent, std::vector<UINT8>& out)
{
	for (UINT16 y = 0; y != height; ++y, pixels += pitch)
	{
		UINT16 x = 0;
		while (x != width)
		{
			bool   const clear = transparent[pixels[x]];
			UINT16 const start = x;
			do
			{
				++x;
			}
			while (x != width && transparent[pixels[x]] == clear && x - start < COMPRESS_RUN_LIMIT);

			UINT8 const count = static_cast<UINT8>(x - start);
			if (clear)
			{
				out.push_back(COMPRESS_TRANSPARENT | count);
			}
			else
			{
				out.push_back(count);
				out.insert(out.end(), pixels + start, pixels + x);
			}
		}
		out.push_back(0);
	}
}
