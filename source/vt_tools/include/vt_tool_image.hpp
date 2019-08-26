
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_image.hpp
// Author: Guilherme R. Lampert
// Created on: 23/10/14
// Brief: Image loading and manipulation helpers.
//
// License:
//  This source code is released under the MIT License.
//  Copyright (c) 2014 Guilherme R. Lampert.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//
// ================================================================================================

#ifndef VT_TOOL_IMAGE_HPP
#define VT_TOOL_IMAGE_HPP

#include <string>
#include <cstdint>

namespace vt
{
namespace tool
{

// ======================================================
// PixelFormat:
// ======================================================

//
// Supported internal image and texture formats.
//
struct PixelFormat
{
	enum Enum
	{
		// RGB images are composed of 3 color components.
		// Each component can be a byte [0,255] or a normalized float [0,1].
		RgbU8,
		RgbF32,

		// RGBA images are composed of 3 color components and
		// 1 component for the transparency level (alpha).
		// Each component can be a byte [0,255] or a normalized float [0,1].
		RgbaU8,
		RgbaF32,

		// Sentinel value. Not a valid pixel format. Internal use.
		Invalid = -1
	};

	// Converts a pixel format enum value to a printable string, for debugging purposes.
	static const char * toString(PixelFormat::Enum pixelFormat);

	// Returns the size in bytes of a single pixel of a given pixel format.
	static size_t sizeBytes(PixelFormat::Enum pixelFormat);

	// Returns the number of color components of a pixel format.
	static uint32_t componentCount(PixelFormat::Enum pixelFormat);
};

// ======================================================
// TPixel templates:
// ======================================================

// RGB pixel:
template<class T>
struct TPixel3
{
	T r;
	T g;
	T b;
};

// RGBA pixel:
template<class T>
struct TPixel4
{
	T r;
	T g;
	T b;
	T a;
};

// ======================================================
// Image:
// ======================================================

//
// Represents and array of pixels.
// Images are most commonly loaded from image files.
//
class Image
{
public:

	// Default constructor; no image loaded.
	Image();

	// Construct from a deep copy.
	Image(const Image & other);

	// Assignment operator creates a deep copy of the source image.
	Image & operator = (const Image & other);

	// Tries to load an image from a file.
	// If the image fails to load and 'errorMessage' is not null, a small error description is returned in it.
	bool loadFromFile(const std::string & filename, std::string * errorMessage = nullptr, bool forceRGBA = false);

	// Creates an uncompressed image of any size, filled with the given color.
	// Provided data pointer must match pixel format size and type.
	void makeColorFilledImage(uint32_t w, uint32_t h, PixelFormat::Enum pf, const uint8_t * color);

	// Creates a RgbaU8 64x64 checkerboard pattern image.
	// The number of checker squares may range from 2 to 64, as long as it is always a power-of-two.
	void makeCheckerPatternImage(uint32_t numSquares);

	// Allocate image data storage:
	uint8_t * allocImageStorage(size_t dataSize, uint32_t w, uint32_t h, PixelFormat::Enum pf);

	// Free previously allocated image data.
	void freeImageStorage();

	// Get the width of the image in pixels.
	uint32_t getWidth() const { return width; }

	// Get the height of the image in pixels.
	uint32_t getHeight() const { return height; }

	// Get the internal pixel format of the image.
	PixelFormat::Enum getFormat() const { return pixelFormat; }

	// Number of color components for this image.
	uint32_t getNumComponents() const { return PixelFormat::componentCount(pixelFormat); }

	// Get the number of bytes allocated for image data.
	size_t getDataSizeBytes() const { return dataSizeBytes; }

	// Grab a pointer to the system memory image data.
	template<class PixelType> PixelType * getDataPtr() const { return reinterpret_cast<PixelType *>(data); }

	// Check if the image width and height are both powers of two.
	bool isPowerOfTwo() const;

	// Validate image dimensions/size/format and data pointers.
	bool isValid() const;

	// Get a pixel at a given coord. 'pixel' must be big enough to hold data of one pixel.
	void getPixelAt(uint32_t x, uint32_t y, uint8_t * pixel) const;

	// Set a pixel at a given coord.
	void setPixelAt(uint32_t x, uint32_t y, const uint8_t * pixel);

	// Swaps the pixel at (x0,y0) with the pixel at (x1,y1).
	void swapPixels(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);

	// Applies the user supplied function to every pixel of this uncompressed image.
	void doForEveryPixel(void (* func)(uint8_t *, PixelFormat::Enum));

	// Flips this image vertically in-place without a copy.
	void flipVInPlace();

	// Flips this image horizontally in-place without a copy.
	void flipHInPlace();

	// Creates a vertically flipped copy of this image.
	void flipV(Image & destImage) const;

	// Creates a horizontally flipped copy of this image.
	void flipH(Image & destImage) const;

	// Resizes this image, in place.
	void resizeInPlace(uint32_t targetWidth, uint32_t targetHeight);

	// Creates a resized copy of this image.
	void resize(Image & destImage, uint32_t targetWidth, uint32_t targetHeight) const;

	// Rounds this image down to its nearest power for 2.
	void roundDownToPowerOfTwo();

	// Swizzle the fist and third color channels of the image,
	// effectively changing it from RGB[A] to BGR[A] and vice versa.
	void swizzleRGB();

	// Discard the alpha component of an RGBA/BGRA image.
	void discardAlphaComponent();

	// Set every byte of the image to the given value.
	// Equivalent to C's memset() function.
	void memSet(int val);

	// Copy a portion of this image to the destination image.
	// destImage  : Pointer to the image that will receive the rect copied from the source image.
	// xOffset    : Pixel offset in the X direction where to start copying the rect.
	// yOffset    : Pixel offset in the Y direction where to start copying the rect.
	// rectWidth  : Width of the rect to be copied. (Number of horizontal pixels to copy).
	// rectHeight : Height of the rect to be copied. (Number of vertical pixels to copy).
	// topLeft    : If true, the (0,0) origin of the image is considered to be the top-left corner.
	// Else, the origin point is the bottom-left corner of the image. This applies to both the source and destination images.
	void copyRect(Image & destImage, uint32_t xOffset, uint32_t yOffset, uint32_t rectWidth, uint32_t rectHeight, bool topLeft) const;

	// Set a portion of this image from a source image.
	// srcImage   : Pointer the source image.
	// xOffset    : Pixel offset in the X direction where to start filling the this image's rect.
	// yOffset    : Pixel offset in the Y direction where to start filling the this image's rect.
	// rectWidth  : Width of the rect to be copied. (Number of horizontal pixels to copy).
	// rectHeight : Height of the rect to be copied. (Number of vertical pixels to copy).
	// topLeft    : If true, the (0,0) origin of the image is considered to be the top-left corner.
	// Else, the origin point is the bottom-left corner of the image. This applies to both the source and destination images.
	void setRect(const Image & srcImage, uint32_t xOffset, uint32_t yOffset, uint32_t rectWidth, uint32_t rectHeight, bool topLeft);

	// setRect() overload that takes a raw pointer as data source.
	// Data is assumed to be of the same pixel format as this image.
	void setRect(const uint8_t * image, uint32_t xOffset, uint32_t yOffset, uint32_t rectWidth, uint32_t rectHeight, bool topLeft);

	// Image destructor releases any allocated memory.
	~Image();

private:

	// Construct an Image from a copy.
	void initFromCopy(const Image & img);

	// Move 'img' to this image. 'img' is invalidated after that.
	void moveCopy(Image & img);

private:

	// Pointer to in memory image data, compressed or uncompressed.
	uint8_t * data;

	// Size in bytes of the 'data' memory buffer.
	size_t dataSizeBytes;

	// Width and height of image, in pixels.
	uint32_t width, height;

	// Internal format of the image pixels.
	PixelFormat::Enum pixelFormat;
};

// ======================================================
// Miscellaneous helpers:
// ======================================================

// Minimal TGA image writer. Used mostly for debugging.
bool writeTgaImage(const std::string & filename, int width, int height, int colorComps,
                   const uint8_t * imageData, bool swizzle, std::string * resultMessage = nullptr);

// Little helper used to write debug info to page pixels.
void addDebugInfoToPageData(int x, int y, int lvl, uint8_t * pageData, int colorComps,
                            bool drawPageBorder, bool flipText, int pageSizePx, int pageBorderSizePx);

} // namespace tool {}
} // namespace vt {}

#endif // VT_TOOL_IMAGE_HPP
