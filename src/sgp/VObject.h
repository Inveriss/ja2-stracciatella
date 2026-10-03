#ifndef __VOBJECT_H
#define __VOBJECT_H

#include "Types.h"
#include <memory>
#include <vector>


// Defines for HVOBJECT limits
#define HVOBJECT_SHADE_TABLES										48


// Z-buffer info structure for properly assigning Z values
struct ZStripInfo
{
	INT8  pbZChange[16];      // change to the Z value in each strip (after the first)
	INT8  bInitialZChange;    // difference in Z value between the leftmost and base strips
	UINT8 ubFirstZStripWidth; // # of pixels in the leftmost strip
	UINT8 ubNumberOfZChanges; // number of strips (after the first)
};

// This definition mimics what is found in WINDOWS.H ( for Direct Draw compatiblity )
// From RGB to COLORVAL
#define FROMRGB(r, g ,b)  ((UINT32) (((UINT8) (r) | ((UINT16) (g) << 8)) | (((UINT32) (UINT8) (b)) << 16)))

// This structure is a video object.
// The video object contains different data based on it's type, compressed or not
class SGPVObject
{
	public:
		// This modifies the SGPImage: relevant data is moved away from it
		SGPVObject(SGPImage *);
		~SGPVObject();

		UINT8 BPP() const { return bit_depth_; }

		// A full colour (32 bit RGBA) video object, loaded from a PNG. It has no
		// palette and no shade tables: Palette(), PixData() and
		// GetETRLEPixelValue() throw, CurrentShade(idx) does nothing and
		// CurrentShade() is null. It is drawn by BltVideoObject(),
		// BltVideoObjectOutline(), BltVideoObjectOutlineShadow() and
		// Blt8BPPDataTo16BPPBufferTransparent[Clip]().
		bool IsRGBA() const { return bit_depth_ == 32; }

		SGPPaletteEntry const* Palette() const;

		UINT16 const* Palette16() const { return palette16_; }

		UINT16 const* CurrentShade() const { return current_shade_; }

		// Set the current object shade table
		void CurrentShade(size_t idx);

		UINT16 SubregionCount() const { return subregion_count_; }

		// How long a subimage is shown in milliseconds, from the metadata of a
		// PNG ("duration"); 0 if not given, always 0 for STI files. Animations
		// that support it use it instead of their own delay.
		UINT16 FrameDuration(size_t idx) const
		{
			return idx < frame_durations_.size() ? frame_durations_[idx] : 0;
		}
		bool HasFrameDurations() const { return !frame_durations_.empty(); }

		// FrameDuration(idx), or the animation's own delay if it is not given.
		UINT32 FrameDurationOr(size_t const idx, UINT32 const ownDelay) const
		{
			UINT16 const ms = FrameDuration(idx);
			return ms != 0 ? ms : ownDelay;
		}

		ETRLEObject const& SubregionProperties(size_t idx) const;

		// ETRLE data of a subimage (8 bit objects only)
		UINT8 const* PixData(ETRLEObject const&) const;

		// RGBA rows of a subimage (32 bit objects only)
		UINT8 const* RGBAData(ETRLEObject const&) const;

		// Outline of a subimage of a 32 bit object, one byte per pixel: non-zero
		// for the transparent pixels next to an opaque one (left, right, above
		// or below). It replaces the outline colour pixels (index 254) of
		// palettised images. Null if the image has no outline ("outline": false
		// in its .png.json).
		UINT8 const* OutlineMask(ETRLEObject const&) const;

		// Colour mask of a subimage of a 32 bit object, one palette index per
		// pixel: 0 for the colour of the pixel itself, otherwise the palette
		// index whose colour the character gets there (clothing, hair, skin;
		// docs/png-images.md). Null if the image has no colour mask.
		UINT8 const* ColourMask(ETRLEObject const&) const;

		/* Given a ETRLE image index, retrieves the value of the pixel located at
		 * the given image coordinates. The value returned is an 8-bit palette index
		 */
		UINT8 GetETRLEPixelValue(UINT16 usETLREIndex, UINT16 usX, UINT16 usY) const;

		// Deletes the 16-bit palette tables
		void DestroyPalettes();

		void ShareShadetables(SGPVObject*);

		enum Flags
		{
			NONE              = 0,
			SHADETABLE_SHARED = 1U << 0
		};

	private:
		void BuildOutlineMask();

		Flags                        flags_;                         // Special flags
		std::unique_ptr<SGPPaletteEntry const []> palette_;          // 8BPP Palette
		UINT16*                      palette16_;                     // A 16BPP palette used for 8->16 blits

		std::unique_ptr<UINT8 const []> pix_data_;                   // ETRLE pixel data, or RGBA rows
		std::unique_ptr<ETRLEObject const []> etrle_object_;         // Object offset data etc
		std::unique_ptr<UINT8 []>    outline_mask_;                  // 32 bit objects: see OutlineMask()
		std::vector<UINT8>           colour_mask_;                   // 32 bit objects: see ColourMask()
	public:
		UINT16*                      pShades[HVOBJECT_SHADE_TABLES]; // Shading tables
	private:
		UINT16 const*                current_shade_;
	public:
		// Smart pointer to an array of smart pointers to ZStripInfo structs.
		std::unique_ptr<std::unique_ptr<ZStripInfo> []> ppZStripInfo;// Z-value strip info arrays

	private:
		UINT16                       subregion_count_;               // Total number of objects
		std::vector<UINT16>          frame_durations_;               // see FrameDuration()
		UINT8                        bit_depth_;                     // BPP

	public:
		SGPVObject*                  next_;
};
ENUM_BITSET(SGPVObject::Flags)


// Creates a list to contain video objects
void InitializeVideoObjectManager(void);

// Deletes any video object placed into list
void ShutdownVideoObjectManager(void);

// Creates and adds a video object to list
SGPVObject* AddVideoObjectFromHImage(SGPImage*);
// needsPalette: the caller uses the palette of the object (shades it with
// Create16BPPPaletteShaded(), reads palette colours or pixel values), so a full
// colour image is not loaded (see IMAGE_NEEDS_PALETTE).
SGPVObject* AddVideoObjectFromFile(const ST::string& ImageFile, bool needsPalette = false);

// Removes a video object
static inline void DeleteVideoObject(SGPVObject* const vo)
{
	delete vo;
}

// Blits a video object to another video object
void BltVideoObject(SGPVSurface* dst, SGPVObject const* src, UINT16 usRegionIndex, INT32 iDestX, INT32 iDestY);


void BltVideoObjectOutline(SGPVSurface* dst, SGPVObject const* src, UINT16 usIndex, INT32 iDestX, INT32 iDestY, INT16 s16BPPColor);
void BltVideoObjectOutlineShadow(SGPVSurface* dst, SGPVObject const* src, UINT16 usIndex, INT32 iDestX, INT32 iDestY);

/* Loads a video object, blits it once and frees it */
void BltVideoObjectOnce(SGPVSurface* dst, char const* filename, UINT16 region, INT32 x, INT32 y);

typedef std::unique_ptr<SGPVObject> AutoSGPVObject;

#endif
