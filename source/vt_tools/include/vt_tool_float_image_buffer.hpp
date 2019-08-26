
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_float_image_buffer.hpp
// Author: Guilherme R. Lampert
// Created on: 23/10/14
// Brief: Buffer of floats used to store images. This format is used for offline image processing.
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

#ifndef VT_TOOL_FLOAT_IMAGE_BUFFER_HPP
#define VT_TOOL_FLOAT_IMAGE_BUFFER_HPP

#include "vt_tool_image.hpp"
#include "vt_tool_filters.hpp"

namespace vt
{
namespace tool
{

// ======================================================
// FloatImageBuffer:
// ======================================================

//
// Buffer of floats used to store images.
//
// Data in the float image is stored in a single sequential array of floats
// with each color channel following the previous. Conceptually, a float image
// would be something like a set of single color arrays:
//
//   float red[num_pixels], blue[num_pixels], green[num_pixels], alpha[num_pixels];
//
// This format is normally used for offline image processing.
// The MipMapper class, for instance, can only work with FloatImageBuffers.
//
// This class is mostly based on the FloatImage class from the
// NVidia Texture Tools library:
//   http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvimage/FloatImage.h
//
class FloatImageBuffer
{
public:

	// Addressing modes for the getIndex*() methods:
	enum WrapMode
	{
		Clamp,
		Repeat,
		Mirror,
		RepeatFirstPixel, // Use pixel at index [0,0]
		ClampToBlack      // Use a black (0,0,0,1) pixel
	};

	// Common color channel indexes, for use with the getChannel() accessors:
	enum ColorChannelIndex
	{
		ChRed,
		ChGreen,
		ChBlue,
		ChAlpha
	};

	// Constructors:
	FloatImageBuffer();
	FloatImageBuffer(const Image & img);
	FloatImageBuffer(const FloatImageBuffer & other);
	FloatImageBuffer(FloatImageBuffer && other) noexcept;

	// Destructor free the internal memory.
	~FloatImageBuffer();

	// Assignment / move:
	FloatImageBuffer & operator = (const Image & img);
	FloatImageBuffer & operator = (const FloatImageBuffer & other);
	FloatImageBuffer & operator = (FloatImageBuffer && other) noexcept;

	// Convert back to Image:
	void toImageRgbU8(Image & destImage) const;   // Discards alpha if present.
	void toImageRgbaU8(Image & destImage) const;  // Adds 255 alpha if not present.
	void toImageRgbaF32(Image & destImage) const; // Adds 1.0 alpha if not present.

	// Import data from Image. You can also use the assignment (=) operator.
	void importRgbU8(const Image & img);
	void importRgbaU8(const Image & img);
	void importRgbF32(const Image & img);
	void importRgbaF32(const Image & img);

	// Allocate/deallocate storage:
	size_t getDataSizeBytes() const;
	float * allocImageStorage(uint32_t c, uint32_t w, uint32_t h);
	void freeImageStorage();

	// Resizing/resampling:
	void downsample(FloatImageBuffer & destImage, const Filter & filter, WrapMode wm) const;
	void resize(FloatImageBuffer & destImage, const Filter & filter, uint32_t w, uint32_t h, WrapMode wm) const;

	// Filtering:
	void applyKernelHorizontal(const PolyphaseKernel & k, int32_t y, uint32_t c, WrapMode wm, float * output) const;
	void applyKernelVertical(const PolyphaseKernel & k, int32_t x, uint32_t c, WrapMode wm, float * output) const;

	// Miscellaneous:
	void colorFill(const TPixel4<float> & color);
	void copyRect(FloatImageBuffer & destImage, int32_t xOffset, int32_t yOffset, int32_t destStartX,
	              int32_t destStartY, uint32_t rectWidth, uint32_t rectHeight, bool topLeft, WrapMode wm) const;

	// Flip this image in-place.
	void flipVInPlace();
	void flipHInPlace();

	// Color channel block access:
	const float * getChannel(uint32_t c) const;
	float * getChannel(uint32_t c);

	// Indexing:
	uint32_t getIndex(uint32_t x, uint32_t y) const;
	uint32_t getIndexClamp(int32_t x, int32_t y) const;
	uint32_t getIndexRepeat(int32_t x, int32_t y) const;
	uint32_t getIndexMirror(int32_t x, int32_t y) const;
	uint32_t getIndexRepeatFirstPixel(int32_t x, int32_t y) const;
	uint32_t getIndexClampToBlack(int32_t x, int32_t y) const;
	uint32_t getIndex(int32_t x, int32_t y, WrapMode wm) const;

	// Inline Accessors:
	uint32_t getWidth()         const { return width;          }
	uint32_t getHeight()        const { return height;         }
	uint32_t getPixelCount()    const { return width * height; }
	uint32_t getNumComponents() const { return numComponents;  }
	float *  getDataPtr()       const { return mem;            }

	// Test if a given Image pixel format can be converted to a float buffer.
	static bool isImageFormatCompatible(PixelFormat::Enum pf);

private:

	// Set everything to zero/null.
	void initEmpty();

	// Create new from an Image object.
	void initFromImage(const Image & img);

	// Create new from copy.
	void initFromCopy(const FloatImageBuffer & fImgBuf);

	// Back to conventional RGB[A] Image:
	void exportRgb8(Image & destImage, uint32_t baseComponent = 0, uint32_t numColorComps = 4) const;

	// Real pixel count (pixelCount + 1).
	// We have this extra padding pixel, which is always black,
	// so that getIndexClampToBlack() can use it.
	uint32_t getActualPixelCount() const { return (width * height) + 1; }

private:

	uint32_t width;
	uint32_t height;
	uint32_t numComponents;
	float *  mem; // NOTE: There is an extra pixel at the end for getIndexClampToBlack().
	              // So the actual pixel count is (width * height) + 1.
};

} // namespace tool {}
} // namespace vt {}

#endif // VT_TOOL_FLOAT_IMAGE_BUFFER_HPP
