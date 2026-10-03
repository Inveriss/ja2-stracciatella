#include "PNG.h"

#include "ETRLE.h"
#include "HImage.h"
#include "Json.h"
#include "Logger.h"
#include "SGPFile.h"
#include "VObject.h"

#include "ContentManager.h"
#include "GameInstance.h"

#include <string_theory/format>

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>

// stb_image is only used in this file: its implementation is compiled here
// with internal linkage, and everything but the PNG decoder is disabled.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <stb_image.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif


namespace
{

UINT8 const PNG_SIGNATURE[8] = { 137, 'P', 'N', 'G', '\r', '\n', 26, '\n' };

UINT8 const COLOUR_TYPE_INDEXED = 3;

// Upper limit for width * height, to reject absurd allocations up front.
// 2^26 pixels are 256 MiB as RGBA, far beyond anything the game shows.
size_t const MAX_PIXELS = size_t{1} << 26;


[[noreturn]] void Fail(ST::string const& message)
{
	throw std::runtime_error(ST::format("PNG: {}", message).to_std_string());
}


UINT32 ReadBE32(UINT8 const* const p)
{
	return UINT32{p[0]} << 24 | UINT32{p[1]} << 16 | UINT32{p[2]} << 8 | UINT32{p[3]};
}


struct PNGHeader
{
	UINT32 width;
	UINT32 height;
	UINT8  bitDepth;
	UINT8  colourType;
	UINT8  interlace;
};


PNGHeader ReadHeader(UINT8 const* const data, size_t const size)
{
	if (size < sizeof(PNG_SIGNATURE) || memcmp(data, PNG_SIGNATURE, sizeof(PNG_SIGNATURE)) != 0)
	{
		Fail("not a PNG file");
	}

	// IHDR must be the first chunk: length (4), type (4), 13 bytes of data, CRC (4)
	size_t const pos = sizeof(PNG_SIGNATURE);
	if (size - pos < 8 + 13 + 4 || ReadBE32(data + pos) != 13 || memcmp(data + pos + 4, "IHDR", 4) != 0)
	{
		Fail("missing or invalid IHDR chunk");
	}

	UINT8 const* const ihdr = data + pos + 8;
	PNGHeader h;
	h.width      = ReadBE32(ihdr);
	h.height     = ReadBE32(ihdr + 4);
	h.bitDepth   = ihdr[8];
	h.colourType = ihdr[9];
	h.interlace  = ihdr[12];

	if (h.width == 0 || h.height == 0)
	{
		Fail("image has no pixels");
	}
	if (h.width > UINT16_MAX || h.height > UINT16_MAX ||
		size_t{h.width} * h.height > MAX_PIXELS)
	{
		Fail(ST::format("image too large ({}x{})", h.width, h.height));
	}
	if (ihdr[10] != 0 || ihdr[11] != 0 || h.interlace > 1)
	{
		Fail("unknown compression, filter or interlace method");
	}
	return h;
}


DecodedPNG DecodeWithStb(UINT8 const* const data, size_t const size, PNGHeader const& h)
{
	if (size > INT_MAX) Fail("file too large");

	int w;
	int ht;
	int channels;
	std::unique_ptr<stbi_uc, void (*)(void*)> const px{
		stbi_load_from_memory(data, static_cast<int>(size), &w, &ht, &channels, 4),
		stbi_image_free };
	if (!px)
	{
		Fail(ST::format("failed to decode image: {}", stbi_failure_reason()));
	}

	DecodedPNG png;
	png.kind             = DecodedPNG::Kind::RGBA;
	png.width            = static_cast<UINT16>(w);
	png.height           = static_cast<UINT16>(ht);
	png.sourceColourType = h.colourType;
	png.sourceBitDepth   = h.bitDepth;
	png.pixels.assign(px.get(), px.get() + size_t{png.width} * png.height * 4);
	return png;
}


UINT8 Paeth(UINT8 const a, UINT8 const b, UINT8 const c)
{
	int const p  = int{a} + b - c;
	int const pa = std::abs(p - a);
	int const pb = std::abs(p - b);
	int const pc = std::abs(p - c);
	if (pa <= pb && pa <= pc) return a;
	if (pb <= pc)             return b;
	return c;
}


// Reverses the PNG row filters of one (sub)image. src holds `rows` rows of a
// filter type byte followed by rowBytes bytes, dst receives rows * rowBytes
// bytes. Palettised images have at most 8 bits per pixel, so the filters
// always work on neighbouring bytes.
void Unfilter(UINT8 const* src, UINT8* const dst, size_t const rowBytes, size_t const rows)
{
	std::vector<UINT8> const zeroRow(rowBytes, 0);
	for (size_t r = 0; r != rows; ++r)
	{
		UINT8 const  filter = *src++;
		UINT8*       cur    = dst + r * rowBytes;
		UINT8 const* prev   = r == 0 ? zeroRow.data() : cur - rowBytes;
		for (size_t i = 0; i != rowBytes; ++i)
		{
			UINT8 const a = i != 0 ? cur[i - 1]  : 0;
			UINT8 const b = prev[i];
			UINT8 const c = i != 0 ? prev[i - 1] : 0;
			UINT8 const x = src[i];
			switch (filter)
			{
				case 0: cur[i] = x;                                         break;
				case 1: cur[i] = static_cast<UINT8>(x + a);                 break;
				case 2: cur[i] = static_cast<UINT8>(x + b);                 break;
				case 3: cur[i] = static_cast<UINT8>(x + ((a + b) >> 1));    break;
				case 4: cur[i] = static_cast<UINT8>(x + Paeth(a, b, c));    break;
				default: Fail(ST::format("invalid row filter type {}", filter));
			}
		}
		src += rowBytes;
	}
}


// Expands one row of packed 1, 2, 4 or 8 bit palette indices.
void UnpackRow(UINT8 const* const row, UINT8 const bitDepth, size_t const count,
	UINT8* dst, size_t const dstStep)
{
	if (bitDepth == 8)
	{
		for (size_t i = 0; i != count; ++i, dst += dstStep) *dst = row[i];
		return;
	}

	UINT8 const mask = static_cast<UINT8>((1U << bitDepth) - 1);
	for (size_t i = 0; i != count; ++i, dst += dstStep)
	{
		size_t   const bit   = i * bitDepth;
		unsigned const shift = 8 - bitDepth - static_cast<unsigned>(bit & 7);
		*dst = static_cast<UINT8>((row[bit >> 3] >> shift) & mask);
	}
}


struct Pass
{
	size_t x0, y0, dx, dy;
};

// Adam7 interlacing; a non-interlaced image is one pass over all pixels.
Pass const ADAM7_PASSES[] =
{
	{ 0, 0, 8, 8 }, { 4, 0, 8, 8 }, { 0, 4, 4, 8 }, { 2, 0, 4, 4 },
	{ 0, 2, 2, 4 }, { 1, 0, 2, 2 }, { 0, 1, 1, 2 }
};
Pass const SINGLE_PASS[] = { { 0, 0, 1, 1 } };


DecodedPNG DecodeIndexed(UINT8 const* const data, size_t const size, PNGHeader const& h)
{
	switch (h.bitDepth)
	{
		case 1: case 2: case 4: case 8: break;
		default: Fail(ST::format("invalid bit depth {} for a palettised image", h.bitDepth));
	}

	DecodedPNG png;
	png.kind             = DecodedPNG::Kind::Indexed;
	png.width            = static_cast<UINT16>(h.width);
	png.height           = static_cast<UINT16>(h.height);
	png.sourceColourType = h.colourType;
	png.sourceBitDepth   = h.bitDepth;

	// Collect PLTE, tRNS and IDAT. The signature and IHDR were checked already.
	std::vector<UINT8> idat;
	bool hasPalette = false;
	bool hasEnd     = false;
	for (size_t pos = sizeof(PNG_SIGNATURE); pos != size;)
	{
		if (size - pos < 12) Fail("file is truncated");
		UINT32       const len   = ReadBE32(data + pos);
		UINT8  const*const type  = data + pos + 4;
		UINT8  const*const chunk = data + pos + 8;
		if (len > size - pos - 12) Fail("file is truncated");

		if (memcmp(type, "IHDR", 4) == 0)
		{
			if (pos != sizeof(PNG_SIGNATURE)) Fail("duplicate IHDR chunk");
		}
		else if (memcmp(type, "PLTE", 4) == 0)
		{
			if (hasPalette || !idat.empty()) Fail("misplaced PLTE chunk");
			if (len == 0 || len % 3 != 0 || len > 256 * 3) Fail("invalid PLTE chunk");
			png.palette.resize(len / 3);
			for (size_t i = 0; i != png.palette.size(); ++i)
			{
				SGPPaletteEntry& e = png.palette[i];
				e.r = chunk[i * 3];
				e.g = chunk[i * 3 + 1];
				e.b = chunk[i * 3 + 2];
				e.a = 255;
			}
			hasPalette = true;
		}
		else if (memcmp(type, "tRNS", 4) == 0)
		{
			if (!hasPalette || !idat.empty()) Fail("misplaced tRNS chunk");
			if (len > png.palette.size()) Fail("tRNS chunk has more entries than the palette");
			for (size_t i = 0; i != len; ++i) png.palette[i].a = chunk[i];
		}
		else if (memcmp(type, "IDAT", 4) == 0)
		{
			if (!hasPalette) Fail("missing PLTE chunk");
			idat.insert(idat.end(), chunk, chunk + len);
		}
		else if (memcmp(type, "IEND", 4) == 0)
		{
			hasEnd = true;
			break;
		}
		else if (!(type[0] & 0x20))
		{
			Fail(ST::format("unknown critical chunk {}{}{}{}",
				char(type[0]), char(type[1]), char(type[2]), char(type[3])));
		}
		// Ancillary chunks we do not know are skipped.

		pos += 12 + size_t{len};
	}
	if (!hasEnd)      Fail("file is truncated (missing IEND chunk)");
	if (idat.empty()) Fail("missing IDAT chunk");

	// Work out the passes and the amount of filtered data they take.
	Pass const* const passBegin = h.interlace ? std::begin(ADAM7_PASSES) : std::begin(SINGLE_PASS);
	Pass const* const passEnd   = h.interlace ? std::end(ADAM7_PASSES)   : std::end(SINGLE_PASS);
	auto const passWidth  = [&](Pass const& p) { return h.width  > p.x0 ? (h.width  - p.x0 + p.dx - 1) / p.dx : 0; };
	auto const passHeight = [&](Pass const& p) { return h.height > p.y0 ? (h.height - p.y0 + p.dy - 1) / p.dy : 0; };
	auto const rowBytesOf = [&](size_t const w) { return (w * h.bitDepth + 7) / 8; };

	size_t expected = 0;
	for (Pass const* p = passBegin; p != passEnd; ++p)
	{
		size_t const w = passWidth(*p);
		size_t const ht = passHeight(*p);
		if (w != 0 && ht != 0) expected += ht * (1 + rowBytesOf(w));
	}

	if (idat.size() > INT_MAX) Fail("image data too large");
	int rawLen = 0;
	std::unique_ptr<char, void (*)(void*)> const raw{
		stbi_zlib_decode_malloc_guesssize_headerflag(reinterpret_cast<char const*>(idat.data()),
			static_cast<int>(idat.size()), static_cast<int>(expected), &rawLen, 1),
		std::free };
	if (!raw) Fail("image data is corrupt");
	if (static_cast<size_t>(rawLen) < expected) Fail("image data is too short");

	png.pixels.resize(size_t{h.width} * h.height);
	UINT8 const*       src = reinterpret_cast<UINT8 const*>(raw.get());
	std::vector<UINT8> rows;
	for (Pass const* p = passBegin; p != passEnd; ++p)
	{
		size_t const w  = passWidth(*p);
		size_t const ht = passHeight(*p);
		if (w == 0 || ht == 0) continue;

		size_t const rowBytes = rowBytesOf(w);
		rows.resize(rowBytes * ht);
		Unfilter(src, rows.data(), rowBytes, ht);
		src += ht * (1 + rowBytes);

		for (size_t y = 0; y != ht; ++y)
		{
			UINT8* const dst = png.pixels.data() + (p->y0 + y * p->dy) * h.width + p->x0;
			UnpackRow(rows.data() + y * rowBytes, h.bitDepth, w, dst, p->dx);
		}
	}

	for (UINT8 const index : png.pixels)
	{
		if (index >= png.palette.size())
		{
			Fail(ST::format("palette index {} out of range, the palette has {} entries",
				index, png.palette.size()));
		}
	}
	return png;
}

}


DecodedPNG DecodePNG(UINT8 const* const data, size_t const size)
{
	PNGHeader const h = ReadHeader(data, size);
	return h.colourType == COLOUR_TYPE_INDEXED
		? DecodeIndexed(data, size, h)
		: DecodeWithStb(data, size, h);
}


DecodedPNG DecodePNGFile(ST::string const& filename)
{
	AutoSGPFile f(GCM->openGameResForReading(filename));
	std::vector<uint8_t> const data = f->readToEnd();
	try
	{
		return DecodePNG(data.data(), data.size());
	}
	catch (std::runtime_error const& e)
	{
		throw std::runtime_error(ST::format("{}: {}", filename, e.what()).to_std_string());
	}
}


namespace
{

PNGFrame MakeFrame(int const x, int const y, int const w, int const h, int const offsetX, int const offsetY,
	UINT16 const imageWidth, UINT16 const imageHeight, size_t const index)
{
	if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > imageWidth || y + h > imageHeight)
	{
		Fail(ST::format("frame {} ({},{} {}x{}) is not inside the {}x{} image",
			index, x, y, w, h, imageWidth, imageHeight));
	}
	if (offsetX < INT16_MIN || offsetX > INT16_MAX || offsetY < INT16_MIN || offsetY > INT16_MAX)
	{
		Fail(ST::format("frame {} has an offset out of range", index));
	}
	return PNGFrame{ static_cast<UINT16>(x), static_cast<UINT16>(y),
		static_cast<UINT16>(w), static_cast<UINT16>(h),
		static_cast<INT16>(offsetX), static_cast<INT16>(offsetY) };
}

// Puts the frame durations into the image, if any frame has one.
void SetFrameDurations(SGPImage& img, std::vector<PNGFrame> const& frames)
{
	img.frameDurations.clear();
	for (PNGFrame const& f : frames)
	{
		if (f.duration == 0) continue;
		for (PNGFrame const& g : frames) img.frameDurations.push_back(g.duration);
		return;
	}
}

// With IMAGE_APPDATA and framesPerDirection != 0: the application data of an
// animated STCI image; the first frame of each animation tells the number of
// frames, all other entries are 0. Otherwise no application data.
void SetAnimationAppData(SGPImage& img, size_t const frameCount, UINT8 const framesPerDirection, UINT16 const fContents)
{
	img.uiAppDataSize = 0;
	if (!(fContents & IMAGE_APPDATA) || framesPerDirection == 0 || frameCount == 0) return;

	size_t const bytes = frameCount * sizeof(AuxObjectData);
	UINT8* const appData = img.pAppData.Allocate(bytes);
	std::fill(appData, appData + bytes, 0);
	AuxObjectData* const aux = reinterpret_cast<AuxObjectData*>(appData);
	for (size_t i = 0; i < frameCount; i += framesPerDirection)
	{
		aux[i].ubNumberOfFrames = framesPerDirection;
		aux[i].fFlags           = AUX_ANIMATED_TILE;
	}
	img.uiAppDataSize  = static_cast<UINT32>(bytes);
	img.fFlags        |= IMAGE_APPDATA;
}

// A frame duration in milliseconds: a whole number from 0 to 65535.
UINT16 ReadDuration(JsonValue const& v, ST::string const& what)
{
	if (!v.isInt()) Fail(ST::format("{} must be a whole number of milliseconds", what));
	int const ms = v.toInt();
	if (ms < 0 || ms > UINT16_MAX) Fail(ST::format("{} must be from 0 to {} milliseconds", what, UINT16_MAX));
	return static_cast<UINT16>(ms);
}

}


PNGMetadata ParsePNGMetadata(ST::string const& json, UINT16 const imageWidth, UINT16 const imageHeight)
{
	JsonValue const root = JsonValue::deserialize(json);
	if (!root.isObject()) Fail("metadata must be a JSON object");
	JsonObject const meta = root.toObject();

	PNGMetadata result;
	if (meta.has("outline"))
	{
		JsonValue const outline = meta.GetValue("outline");
		if (!outline.isBool()) Fail("\"outline\" must be true or false");
		result.outline = outline.toBool();
	}

	if (meta.has("animation"))
	{
		JsonValue const animation = meta.GetValue("animation");
		if (!animation.isObject()) Fail("\"animation\" must be a JSON object");
		JsonObject const a = animation.toObject();
		if (!a.has("framesPerDirection")) Fail("\"animation\" needs \"framesPerDirection\"");
		JsonValue const n = a.GetValue("framesPerDirection");
		if (!n.isInt() || n.toInt() < 1 || n.toInt() > UINT8_MAX)
		{
			Fail(ST::format("\"framesPerDirection\" must be a whole number from 1 to {}", UINT8_MAX));
		}
		result.framesPerDirection = static_cast<UINT8>(n.toInt());
	}

	UINT16 const frameDuration = meta.has("frameDuration")
		? ReadDuration(meta.GetValue("frameDuration"), "\"frameDuration\"") : 0;

	bool const hasFrames = meta.has("frames");
	bool const hasGrid   = meta.has("grid");
	if (hasFrames && hasGrid) Fail("metadata can have either \"frames\" or \"grid\", not both");
	if (meta.has("durations") && !hasGrid) Fail("\"durations\" needs a \"grid\", use \"duration\" in \"frames\"");

	std::vector<PNGFrame>& frames = result.frames;
	if (!hasFrames && !hasGrid)
	{
		if (meta.has("offsets")) Fail("\"offsets\" needs a \"grid\"");
		frames.push_back(PNGFrame{ 0, 0, imageWidth, imageHeight, 0, 0 });
	}
	else if (hasFrames)
	{
		JsonValue const list = meta.GetValue("frames");
		if (!list.isVec()) Fail("\"frames\" must be an array");
		for (JsonValue const& v : list.toVec())
		{
			if (!v.isObject()) Fail("each frame must be a JSON object");
			JsonObject const f = v.toObject();
			frames.push_back(MakeFrame(f.GetInt("x"), f.GetInt("y"), f.GetInt("w"), f.GetInt("h"),
				f.getOptionalInt("offsetX"), f.getOptionalInt("offsetY"),
				imageWidth, imageHeight, frames.size()));
			if (f.has("duration"))
			{
				frames.back().duration = ReadDuration(f.GetValue("duration"),
					ST::format("\"duration\" of frame {}", frames.size() - 1));
			}
		}
	}
	else
	{
		JsonValue const gridValue = meta.GetValue("grid");
		if (!gridValue.isObject()) Fail("\"grid\" must be a JSON object");
		JsonObject const grid = gridValue.toObject();
		int const w = grid.GetInt("w");
		int const h = grid.GetInt("h");
		if (w <= 0 || h <= 0 || w > imageWidth || h > imageHeight)
		{
			Fail(ST::format("grid cell {}x{} does not fit the {}x{} image", w, h, imageWidth, imageHeight));
		}
		int const columns = imageWidth / w;
		int const cells   = columns * (imageHeight / h);
		int const count   = grid.getOptionalInt("count", cells);
		if (count <= 0 || count > cells)
		{
			Fail(ST::format("grid count {} is not between 1 and the {} cells of the image", count, cells));
		}

		std::vector<JsonValue> offsets;
		if (meta.has("offsets"))
		{
			JsonValue const list = meta.GetValue("offsets");
			if (!list.isVec()) Fail("\"offsets\" must be an array");
			offsets = list.toVec();
			if (offsets.size() != static_cast<size_t>(count))
			{
				Fail(ST::format("\"offsets\" has {} entries for {} frames", offsets.size(), count));
			}
		}

		std::vector<JsonValue> durations;
		if (meta.has("durations"))
		{
			JsonValue const list = meta.GetValue("durations");
			if (!list.isVec()) Fail("\"durations\" must be an array");
			durations = list.toVec();
			if (durations.size() != static_cast<size_t>(count))
			{
				Fail(ST::format("\"durations\" has {} entries for {} frames", durations.size(), count));
			}
		}

		for (int i = 0; i != count; ++i)
		{
			int offsetX = 0;
			int offsetY = 0;
			if (!offsets.empty())
			{
				std::vector<JsonValue> const xy = offsets[i].isVec() ? offsets[i].toVec() : std::vector<JsonValue>{};
				if (xy.size() != 2) Fail(ST::format("offset {} must be an array [x, y]", i));
				offsetX = xy[0].toInt();
				offsetY = xy[1].toInt();
			}
			frames.push_back(MakeFrame(i % columns * w, i / columns * h, w, h, offsetX, offsetY,
				imageWidth, imageHeight, frames.size()));
			if (!durations.empty())
			{
				frames.back().duration = ReadDuration(durations[i], ST::format("duration {}", i));
			}
		}
	}

	if (frames.empty())           Fail("metadata has no frames");
	if (frames.size() > UINT16_MAX) Fail("metadata has too many frames");
	if (result.framesPerDirection > frames.size())
	{
		Fail(ST::format("\"framesPerDirection\" is {}, but there are only {} frames", result.framesPerDirection, frames.size()));
	}

	for (PNGFrame& f : frames)
	{
		if (f.duration == 0) f.duration = frameDuration;
	}
	return result;
}


SGPImage* ConvertIndexedPNGToImage(DecodedPNG const& png, std::vector<PNGFrame> const& frames,
	UINT16 const fContents, ST::string const& name, UINT8 const framesPerDirection)
{
	if (png.kind != DecodedPNG::Kind::Indexed)
	{
		Fail(ST::format("{}: not a palettised PNG (colour type {})",
			name, png.sourceColourType));
	}

	AutoSGPImage img(new SGPImage(png.width, png.height, 8));

	if (fContents & IMAGE_PALETTE)
	{
		// Like the STCI loader: always 256 entries, alpha unused (0).
		SGPPaletteEntry* const palette = img->pPalette.Allocate(256);
		for (size_t i = 0; i != 256; ++i)
		{
			SGPPaletteEntry& e = palette[i];
			if (i < png.palette.size())
			{
				e.r = png.palette[i].r;
				e.g = png.palette[i].g;
				e.b = png.palette[i].b;
			}
			else
			{
				e.r = e.g = e.b = 0;
			}
			e.a = 0;
		}
		img->fFlags |= IMAGE_PALETTE;
	}

	if (fContents & IMAGE_BITMAPDATA)
	{
		if (frames.empty() || frames.size() > UINT16_MAX)
		{
			Fail(ST::format("{}: invalid number of frames ({})", name, frames.size()));
		}

		ETRLETransparency transparent{};
		transparent[0] = true;
		bool partialAlpha = false;
		for (size_t i = 0; i != png.palette.size(); ++i)
		{
			UINT8 const a = png.palette[i].a;
			if (a < 128) transparent[i] = true;
			if (a != 0 && a != 255) partialAlpha = true;
		}
		if (partialAlpha)
		{
			SLOGW("{}: palette entries with partial alpha are drawn either opaque (alpha >= 128) or transparent", name);
		}

		std::vector<UINT8> data;
		ETRLEObject* const objects = img->pETRLEObject.Allocate(frames.size());
		for (size_t i = 0; i != frames.size(); ++i)
		{
			PNGFrame const& f = frames[i];
			if (f.width == 0 || f.height == 0 || f.x + f.width > png.width || f.y + f.height > png.height)
			{
				Fail(ST::format("{}: frame {} is not inside the image", name, i));
			}

			size_t const start = data.size();
			EncodeETRLE(png.pixels.data() + size_t{f.y} * png.width + f.x, png.width,
				f.width, f.height, transparent, data);

			ETRLEObject& o = objects[i];
			o.uiDataOffset = static_cast<UINT32>(start);
			o.uiDataLength = static_cast<UINT32>(data.size() - start);
			o.sOffsetX     = f.offsetX;
			o.sOffsetY     = f.offsetY;
			o.usHeight     = f.height;
			o.usWidth      = f.width;
		}

		std::copy(data.begin(), data.end(), static_cast<UINT8*>(img->pImageData.Allocate(data.size())));
		img->usNumberOfObjects = static_cast<UINT16>(frames.size());
		img->uiSizePixData     = static_cast<UINT32>(data.size());
		img->fFlags           |= IMAGE_TRLECOMPRESSED | IMAGE_BITMAPDATA;
		SetFrameDurations(*img, frames);
	}

	SetAnimationAppData(*img, frames.size(), framesPerDirection, fContents);
	return img.release();
}


SGPImage* ConvertRGBAPNGToImage(DecodedPNG const& png, std::vector<PNGFrame> const& frames,
	UINT16 const fContents, ST::string const& name, bool const outline, UINT8 const framesPerDirection)
{
	if (png.kind != DecodedPNG::Kind::RGBA)
	{
		Fail(ST::format("{}: not a full colour PNG (colour type {})", name, png.sourceColourType));
	}

	AutoSGPImage img(new SGPImage(png.width, png.height, 32));

	if (fContents & IMAGE_BITMAPDATA)
	{
		if (frames.empty() || frames.size() > UINT16_MAX)
		{
			Fail(ST::format("{}: invalid number of frames ({})", name, frames.size()));
		}

		size_t total = 0;
		for (size_t i = 0; i != frames.size(); ++i)
		{
			PNGFrame const& f = frames[i];
			if (f.width == 0 || f.height == 0 || f.x + f.width > png.width || f.y + f.height > png.height)
			{
				Fail(ST::format("{}: frame {} is not inside the image", name, i));
			}
			total += size_t{f.width} * f.height * 4;
		}
		if (total > UINT32_MAX) Fail(ST::format("{}: image data too large", name));

		UINT8*       const data    = img->pImageData.Allocate(total);
		ETRLEObject* const objects = img->pETRLEObject.Allocate(frames.size());
		size_t offset = 0;
		for (size_t i = 0; i != frames.size(); ++i)
		{
			PNGFrame const& f = frames[i];
			size_t const rowBytes = size_t{f.width} * 4;
			for (UINT16 y = 0; y != f.height; ++y)
			{
				UINT8 const* const src = png.pixels.data() + ((size_t{f.y} + y) * png.width + f.x) * 4;
				std::copy(src, src + rowBytes, data + offset + y * rowBytes);
			}

			ETRLEObject& o = objects[i];
			o.uiDataOffset = static_cast<UINT32>(offset);
			o.uiDataLength = static_cast<UINT32>(rowBytes * f.height);
			o.sOffsetX     = f.offsetX;
			o.sOffsetY     = f.offsetY;
			o.usHeight     = f.height;
			o.usWidth      = f.width;
			offset += o.uiDataLength;
		}

		img->usNumberOfObjects = static_cast<UINT16>(frames.size());
		img->uiSizePixData     = static_cast<UINT32>(total);
		img->fFlags           |= IMAGE_RGBA | IMAGE_BITMAPDATA;
		if (!outline) img->fFlags |= IMAGE_NO_OUTLINE;
		SetFrameDurations(*img, frames);
	}

	SetAnimationAppData(*img, frames.size(), framesPerDirection, fContents);
	return img.release();
}


SGPImage* ConvertPNGToSurfaceImage(DecodedPNG const& png, UINT16 const fContents)
{
	size_t const pixelCount = size_t{png.width} * png.height;

	if (png.kind == DecodedPNG::Kind::Indexed)
	{
		AutoSGPImage img(new SGPImage(png.width, png.height, 8));
		if (fContents & IMAGE_PALETTE)
		{
			SGPPaletteEntry* const palette = img->pPalette.Allocate(256);
			for (size_t i = 0; i != 256; ++i)
			{
				SGPPaletteEntry& e = palette[i];
				if (i < png.palette.size())
				{
					e.r = png.palette[i].r;
					e.g = png.palette[i].g;
					e.b = png.palette[i].b;
				}
				else
				{
					e.r = e.g = e.b = 0;
				}
				e.a = 0;
			}
			img->fFlags |= IMAGE_PALETTE;
		}
		if (fContents & IMAGE_BITMAPDATA)
		{
			UINT8* const data = img->pImageData.Allocate(pixelCount);
			std::copy(png.pixels.begin(), png.pixels.end(), data);
			img->uiSizePixData = static_cast<UINT32>(pixelCount);
			img->fFlags       |= IMAGE_BITMAPDATA;
		}
		return img.release();
	}

	AutoSGPImage img(new SGPImage(png.width, png.height, 16));
	if (fContents & IMAGE_BITMAPDATA)
	{
		UINT16* const data = reinterpret_cast<UINT16*>(
			static_cast<UINT8*>(img->pImageData.Allocate(pixelCount * 2)));
		UINT8 const* src = png.pixels.data();
		for (size_t i = 0; i != pixelCount; ++i, src += 4)
		{
			UINT16 colour = 0;
			if (src[3] >= 128)
			{
				colour = Get16BPPColor(FROMRGB(src[0], src[1], src[2]));
				if (colour == 0) colour = BLACK_SUBSTITUTE;
			}
			data[i] = colour;
		}
		img->uiSizePixData = static_cast<UINT32>(pixelCount * 2);
		img->fFlags       |= IMAGE_BITMAPDATA;
	}
	return img.release();
}


void AddColourMask(SGPImage& img, DecodedPNG const& mask, std::vector<PNGFrame> const& frames, ST::string const& name)
{
	if (mask.kind != DecodedPNG::Kind::Indexed)
	{
		Fail(ST::format("{}: the colour mask must be a palettised PNG", name));
	}
	if (!(img.fFlags & IMAGE_RGBA) || mask.width != img.usWidth || mask.height != img.usHeight ||
		frames.size() != img.usNumberOfObjects)
	{
		Fail(ST::format("{}: the colour mask must have the size of the image ({}x{})", name, img.usWidth, img.usHeight));
	}

	std::vector<UINT8> data(img.uiSizePixData / 4);
	for (size_t i = 0; i != frames.size(); ++i)
	{
		PNGFrame    const& f = frames[i];
		ETRLEObject const& o = img.pETRLEObject[i];
		for (UINT16 y = 0; y != f.height; ++y)
		{
			UINT8 const* const src = mask.pixels.data() + (size_t{f.y} + y) * mask.width + f.x;
			std::copy(src, src + f.width, data.begin() + o.uiDataOffset / 4 + size_t{y} * f.width);
		}
	}
	img.colourMask = std::move(data);
}


ST::string ColourMaskFileName(ST::string const& filename)
{
	ST::string const stem =
		filename.size() >= 4 && filename.substr(filename.size() - 4).compare_i(".png") == 0 ?
		filename.substr(0, filename.size() - 4) : filename;
	return stem + ".mask.png";
}


SGPImage* LoadPNGFileToImage(ST::string const& filename, UINT16 const fContents)
{
	DecodedPNG const png = DecodePNGFile(filename);
	if (fContents & IMAGE_FOR_SURFACE) return ConvertPNGToSurfaceImage(png, fContents);

	PNGMetadata meta;
	ST::string const metaName = filename + ".json";
	if (GCM->doesGameResExists(metaName))
	{
		AutoSGPFile f(GCM->openGameResForReading(metaName));
		try
		{
			meta = ParsePNGMetadata(f->readStringToEnd(), png.width, png.height);
		}
		catch (std::runtime_error const& e)
		{
			throw std::runtime_error(ST::format("{}: {}", metaName, e.what()).to_std_string());
		}
	}
	else
	{
		meta.frames.push_back(PNGFrame{ 0, 0, png.width, png.height, 0, 0 });
	}
	std::vector<PNGFrame> const& frames = meta.frames;

	// Animations in the game world (tile cache, characters, cursors) need the
	// number of frames. Full colour is only drawn for tile cache animations
	// without palette effects; the others (characters, corpses, cursors) come
	// with IMAGE_NEEDS_PALETTE, see below.
	if ((fContents & IMAGE_APPDATA) && (fContents & IMAGE_ANIMATION_METADATA))
	{
		if (meta.framesPerDirection == 0)
		{
			Fail(ST::format("{}: needs \"animation\": {{ \"framesPerDirection\": N }} in {}", filename, metaName));
		}
	}

	if (png.kind == DecodedPNG::Kind::Indexed)
	{
		return ConvertIndexedPNGToImage(png, frames, fContents, filename, meta.framesPerDirection);
	}
	if (fContents & IMAGE_NEEDS_PALETTE)
	{
		Fail(ST::format("{}: this image is used with its palette, so it must be a palettised PNG", filename));
	}
	if (!(fContents & IMAGE_COLOUR_MASK))
	{
		return ConvertRGBAPNGToImage(png, frames, fContents, filename, meta.outline, meta.framesPerDirection);
	}

	// Recoloured by the game: only with the colour mask
	ST::string const maskName = ColourMaskFileName(filename);
	if (!GCM->doesGameResExists(maskName))
	{
		Fail(ST::format("{}: the game changes the colours of this image, so a full colour PNG needs its colour mask {}", filename, maskName));
	}
	AutoSGPImage img(ConvertRGBAPNGToImage(png, frames, fContents, filename, meta.outline, meta.framesPerDirection));
	if (fContents & IMAGE_BITMAPDATA)
	{
		AddColourMask(*img, DecodePNGFile(maskName), frames, maskName);
	}
	return img.release();
}
