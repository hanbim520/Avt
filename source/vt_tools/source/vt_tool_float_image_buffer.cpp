
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_float_image_buffer.cpp
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

#include "vt_tool_float_image_buffer.hpp"
#include <memory>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cmath>

/*
 * Most of the code in this file was based on or copied from the
 * NVidia Texture Tools library: http://code.google.com/p/nvidia-texture-tools/
 */

namespace vt
{
namespace tool
{

// ======================================================
// Local helpers:
// ======================================================

namespace {

template<typename T>
T clampTo(T x, const T minimum, const T maximum)
{
	return (x < minimum) ? minimum : (x > maximum) ? maximum : x;
}

// ======================================================

inline void copyChannel(const FloatImageBuffer & sourceImg, FloatImageBuffer & destImg, const uint32_t channel, const int xOffset, const int yOffset,
                        const int destStartX, const int destStartY, const int rectWidth, const int rectHeight, const FloatImageBuffer::WrapMode wm)
{
	// Source and dest should be different images!
	const float * __restrict src = sourceImg.getChannel(channel);
	float * __restrict dest = destImg.getChannel(channel);

	for (int dy = destStartY, y = 0; y < rectHeight; ++y, ++dy)
	{
		for (int dx = destStartX, x = 0; x < rectWidth; ++x, ++dx)
		{
			const uint32_t idxSrc = sourceImg.getIndex(x + xOffset, y + yOffset, wm);
			const uint32_t idxDst = destImg.getIndex(dx, dy, wm);
			dest[idxDst] = src[idxSrc];
		}
	}
}

// ======================================================

inline void copyChannelVFlip(const FloatImageBuffer & sourceImg, FloatImageBuffer & destImg, const uint32_t channel, const int xOffset, const int yOffset,
                             const int destStartX, const int destStartY, const int rectWidth, const int rectHeight, const FloatImageBuffer::WrapMode wm)
{
	// Source and dest should be different images!
	const float * __restrict src = sourceImg.getChannel(channel);
	float * __restrict dest = destImg.getChannel(channel);

	int maxY = (sourceImg.getHeight() - 1);
	for (int dy = destStartY, y = 0; y < rectHeight; ++y, ++dy)
	{
		for (int dx = destStartX, x = 0; x < rectWidth; ++x, ++dx)
		{
			const uint32_t idxSrc = sourceImg.getIndex(x + xOffset, maxY - yOffset, wm);
			const uint32_t idxDst = destImg.getIndex(dx, dy, wm);
			dest[idxDst] = src[idxSrc];
		}
		--maxY;
	}
}

// ======================================================

inline void swapPixelsForChannel(const uint32_t idx0, const uint32_t idx1, float * channel)
{
	const float p0 = channel[idx0];
	const float p1 = channel[idx1];
	channel[idx0] = p1;
	channel[idx1] = p0;
}

} // namespace {}

// ======================================================
// FloatImageBuffer:
// ======================================================

FloatImageBuffer::FloatImageBuffer()
{
	initEmpty();
}

FloatImageBuffer::FloatImageBuffer(const Image & img)
{
	initFromImage(img);
}

FloatImageBuffer::FloatImageBuffer(const FloatImageBuffer & other)
{
	initFromCopy(other);
}

FloatImageBuffer::FloatImageBuffer(FloatImageBuffer && other) noexcept
{
	width               = other.width;
	height              = other.height;
	numComponents       = other.numComponents;
	mem                 = other.mem;

	other.width         = 0;
	other.height        = 0;
	other.numComponents = 0;
	other.mem           = nullptr;
}

FloatImageBuffer::~FloatImageBuffer()
{
	freeImageStorage();
}

FloatImageBuffer & FloatImageBuffer::operator = (const Image & img)
{
	freeImageStorage();
	initFromImage(img);
	return *this;
}

FloatImageBuffer & FloatImageBuffer::operator = (const FloatImageBuffer & other)
{
	freeImageStorage();
	initFromCopy(other);
	return *this;
}

FloatImageBuffer & FloatImageBuffer::operator = (FloatImageBuffer && other) noexcept
{
	freeImageStorage();

	width               = other.width;
	height              = other.height;
	numComponents       = other.numComponents;
	mem                 = other.mem;

	other.width         = 0;
	other.height        = 0;
	other.numComponents = 0;
	other.mem           = nullptr;

	return *this;
}

void FloatImageBuffer::initEmpty()
{
	width         = 0;
	height        = 0;
	numComponents = 0;
	mem           = nullptr;
}

void FloatImageBuffer::initFromImage(const Image & img)
{
	initEmpty();
	allocImageStorage(img.getNumComponents(), img.getWidth(), img.getHeight());

	// Currently, we only offer support for RGB and RGBA float32 and u8 images.
	switch (img.getFormat())
	{
	case PixelFormat::RgbU8   : importRgbU8(img);   break;
	case PixelFormat::RgbaU8  : importRgbaU8(img);  break;
	case PixelFormat::RgbF32  : importRgbF32(img);  break;
	case PixelFormat::RgbaF32 : importRgbaF32(img); break;
	default : assert(false && "Image pixel format currently unsupported by FloatImageBuffer!");
	} // switch (pf)
}

void FloatImageBuffer::initFromCopy(const FloatImageBuffer & fImgBuf)
{
	initEmpty();
	allocImageStorage(fImgBuf.getNumComponents(), fImgBuf.getWidth(), fImgBuf.getHeight());
	assert(getDataSizeBytes() == fImgBuf.getDataSizeBytes());
	std::memcpy(mem, fImgBuf.mem, getDataSizeBytes());
}

bool FloatImageBuffer::isImageFormatCompatible(const PixelFormat::Enum pf)
{
	switch (pf)
	{
	case PixelFormat::RgbU8   : return true;
	case PixelFormat::RgbaU8  : return true;
	case PixelFormat::RgbF32  : return true;
	case PixelFormat::RgbaF32 : return true;
	default : return false;
	} // switch (pf)
}

void FloatImageBuffer::importRgbU8(const Image & img)
{
	constexpr float oneOver255 = (1.0f / 255.0f);

	float * redChannel   = getChannel(ChRed);
	float * greenChannel = getChannel(ChGreen);
	float * blueChannel  = getChannel(ChBlue);
	float * alphaChannel = (numComponents == 4) ? getChannel(ChAlpha) : nullptr;

	const TPixel3<uint8_t> * srcPixels = img.getDataPtr< TPixel3<uint8_t> >();
	const uint32_t pixelCount = (width * height);

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		TPixel3<uint8_t> pixel = srcPixels[i];
		redChannel[i]   = static_cast<float>(pixel.r) * oneOver255;
		greenChannel[i] = static_cast<float>(pixel.g) * oneOver255;
		blueChannel[i]  = static_cast<float>(pixel.b) * oneOver255;
		if (alphaChannel)
		{
			alphaChannel[i] = 1.0f; // Fixed alpha
		}
	}
}

void FloatImageBuffer::importRgbaU8(const Image & img)
{
	constexpr float oneOver255 = (1.0f / 255.0f);

	float * redChannel   = getChannel(ChRed);
	float * greenChannel = getChannel(ChGreen);
	float * blueChannel  = getChannel(ChBlue);
	float * alphaChannel = (numComponents == 4) ? getChannel(ChAlpha) : nullptr;

	const TPixel4<uint8_t> * srcPixels = img.getDataPtr< TPixel4<uint8_t> >();
	const uint32_t pixelCount = (width * height);

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		TPixel4<uint8_t> pixel = srcPixels[i];
		redChannel[i]   = static_cast<float>(pixel.r) * oneOver255;
		greenChannel[i] = static_cast<float>(pixel.g) * oneOver255;
		blueChannel[i]  = static_cast<float>(pixel.b) * oneOver255;
		if (alphaChannel)
		{
			alphaChannel[i] = static_cast<float>(pixel.a) * oneOver255;
		}
	}
}

void FloatImageBuffer::importRgbF32(const Image & img)
{
	float * redChannel   = getChannel(ChRed);
	float * greenChannel = getChannel(ChGreen);
	float * blueChannel  = getChannel(ChBlue);
	float * alphaChannel = (numComponents == 4) ? getChannel(ChAlpha) : nullptr;

	const TPixel3<float> * srcPixels = img.getDataPtr< TPixel3<float> >();
	const uint32_t pixelCount = (width * height);

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		TPixel3<float> pixel = srcPixels[i];
		redChannel[i]   = pixel.r;
		greenChannel[i] = pixel.g;
		blueChannel[i]  = pixel.b;
		if (alphaChannel)
		{
			alphaChannel[i] = 1.0f; // Fixed alpha
		}
	}
}

void FloatImageBuffer::importRgbaF32(const Image & img)
{
	float * redChannel   = getChannel(ChRed);
	float * greenChannel = getChannel(ChGreen);
	float * blueChannel  = getChannel(ChBlue);
	float * alphaChannel = (numComponents == 4) ? getChannel(ChAlpha) : nullptr;

	const TPixel4<float> * srcPixels = img.getDataPtr< TPixel4<float> >();
	const uint32_t pixelCount = (width * height);

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		TPixel4<float> pixel = srcPixels[i];
		redChannel[i]   = pixel.r;
		greenChannel[i] = pixel.g;
		blueChannel[i]  = pixel.b;
		if (alphaChannel)
		{
			alphaChannel[i] = pixel.a;
		}
	}
}

void FloatImageBuffer::exportRgb8(Image & destImage, const uint32_t baseComponent, const uint32_t numColorComps) const
{
	assert(mem != nullptr);
	assert(numColorComps > 0 && numColorComps <= 4);
	assert((baseComponent + numColorComps) <= numComponents);

	destImage.freeImageStorage();
	destImage.allocImageStorage((width * height * numColorComps),
		width, height, (numColorComps == 4) ? PixelFormat::RgbaU8 : PixelFormat::RgbU8);

	uint8_t *      destPixels       = destImage.getDataPtr<uint8_t>();
	const uint32_t pixelCount       = (width * height);
	const uint32_t actualPixelCount = getActualPixelCount();

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		uint32_t c;
		uint8_t rgba[4] = { 0x0, 0x0, 0x0, 0xFF };

		for (c = 0; c < numColorComps; ++c)
		{
			float f = mem[actualPixelCount * (baseComponent + c) + i];
			rgba[c] = static_cast<uint8_t>(clampTo(static_cast<int32_t>(255.0f * f), 0, 255));
		}

		for (c = 0; c < numColorComps; ++c)
		{
			destPixels[c] = rgba[c];
		}

		destPixels += numColorComps;
	}
}

void FloatImageBuffer::toImageRgbU8(Image & destImage) const
{
	assert(numComponents >= 3);
	exportRgb8(destImage, 0, 3);
}

void FloatImageBuffer::toImageRgbaU8(Image & destImage) const
{
	assert(numComponents >= 3);
	exportRgb8(destImage, 0, (numComponents == 4) ? 4 : 3);
}

void FloatImageBuffer::toImageRgbaF32(Image & destImage) const
{
	assert(numComponents >= 3);

	destImage.freeImageStorage();
	const size_t dataSize = (width * height * 4 * sizeof(float));
	destImage.allocImageStorage(dataSize, width, height, PixelFormat::RgbaF32);

	const float * redChannel   = getChannel(ChRed);
	const float * greenChannel = getChannel(ChGreen);
	const float * blueChannel  = getChannel(ChBlue);
	const float * alphaChannel = (numComponents == 4) ? getChannel(ChAlpha) : nullptr;

	TPixel4<float> * destPixels = destImage.getDataPtr< TPixel4<float> >();
	const uint32_t pixelCount = (width * height);

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		destPixels[i].r = redChannel[i];
		destPixels[i].g = greenChannel[i];
		destPixels[i].b = blueChannel[i];
		if (alphaChannel)
		{
			destPixels[i].a = alphaChannel[i];
		}
		else
		{
			destPixels[i].a = 1.0f;
		}
	}
}

size_t FloatImageBuffer::getDataSizeBytes() const
{
	return getActualPixelCount() * numComponents * sizeof(float);
}

float * FloatImageBuffer::allocImageStorage(const uint32_t c, const uint32_t w, const uint32_t h)
{
	assert(mem == nullptr && "freeImageStorage() before allocating new one!");
	assert(w > 0 && h > 0);
	assert(c != 0 && c <= 4);

	width  = w;
	height = h;
	numComponents = c;
	mem = new float[getActualPixelCount() * numComponents];

	// Set the extra black pixel used by ClampToBlack:
	const uint32_t pixelCount = (width * height);
	for (uint32_t i = 0; i < numComponents; ++i)
	{
		getChannel(i)[pixelCount] = 0.0f;
	}
	if (numComponents == 4)
	{
		getChannel(ChAlpha)[pixelCount] = 1.0f;
	}

	// Return base address:
	return mem;
}

void FloatImageBuffer::freeImageStorage()
{
	if (mem != nullptr)
	{
		delete[] mem;
		initEmpty();
	}
}

void FloatImageBuffer::downsample(FloatImageBuffer & destImage, const Filter & filter, const WrapMode wm) const
{
	const uint32_t w = std::max(uint32_t(1), (width  / 2));
	const uint32_t h = std::max(uint32_t(1), (height / 2));
	return resize(destImage, filter, w, h, wm);
}

void FloatImageBuffer::resize(FloatImageBuffer & destImage, const Filter & filter, const uint32_t w, const uint32_t h, const WrapMode wm) const
{
	FloatImageBuffer tempImage;
	destImage.freeImageStorage(); // Ensure cleared

	// Filter kernels:
	PolyphaseKernel xKernel(filter, width,  w, 32);
	PolyphaseKernel yKernel(filter, height, h, 32);

	// Allocate new images:
	tempImage.allocImageStorage(numComponents, w, height);
	destImage.allocImageStorage(numComponents, w, h);
	std::unique_ptr<float[]> tmpColumn(new float[h]);

	for (uint32_t c = 0; c < numComponents; ++c)
	{
		float * tmpChannel = tempImage.getChannel(c);

		for (uint32_t y = 0; y < height; ++y)
		{
			applyKernelHorizontal(xKernel, y, c, wm, (tmpChannel + y * w));
		}

		float * dstChannel = destImage.getChannel(c);

		for (uint32_t x = 0; x < w; ++x)
		{
			tempImage.applyKernelVertical(yKernel, x, c, wm, &tmpColumn[0]);

			for (uint32_t y = 0; y < h; ++y)
			{
				dstChannel[y * w + x] = tmpColumn[y];
			}
		}
	}
}

void FloatImageBuffer::applyKernelHorizontal(const PolyphaseKernel & k, const int32_t y, const uint32_t c, const WrapMode wm, float * __restrict output) const
{
	// Apply 1D horizontal kernel at the given coordinates and return result.
	const uint32_t length     = k.getLength();
	const float    scale      = static_cast<float>(length) / static_cast<float>(width);
	const float    iscale     = (1.0f / scale);
	const float    w          = k.getWidth();
	const int32_t  windowSize = k.getWindowSize();
	const float *  channel    = getChannel(c);

	for (uint32_t i = 0; i < length; ++i)
	{
		const float center  = (0.5f + i) * iscale;
		const int32_t left  = static_cast<int32_t>(std::floor(center - w));
		const int32_t right = static_cast<int32_t>(std::ceil(center + w));
		assert((right - left) <= windowSize);

		float sum = 0;
		for (int32_t j = 0; j < windowSize; ++j)
		{
			const int32_t idx = getIndex(left + j, y, wm);
			sum += k.getValueAt(i, j) * channel[idx];
		}

		output[i] = sum;
	}
}

void FloatImageBuffer::applyKernelVertical(const PolyphaseKernel & k, const int32_t x, const uint32_t c, const WrapMode wm, float * __restrict output) const
{
	// Apply 1D vertical kernel at the given coordinates and return result.
	const uint32_t length     = k.getLength();
	const float    scale      = static_cast<float>(length) / static_cast<float>(height);
	const float    iscale     = (1.0f / scale);
	const float    w          = k.getWidth();
	const int32_t  windowSize = k.getWindowSize();
	const float *  channel    = getChannel(c);

	for (uint32_t i = 0; i < length; ++i)
	{
		const float center  = (0.5f + i) * iscale;
		const int32_t left  = static_cast<int32_t>(std::floor(center - w));
		const int32_t right = static_cast<int32_t>(std::ceil(center + w));
		assert((right - left) <= windowSize);

		float sum = 0;
		for (int32_t j = 0; j < windowSize; ++j)
		{
			const int32_t idx = getIndex(x, j+left, wm);
			sum += k.getValueAt(i, j) * channel[idx];
		}

		output[i] = sum;
	}
}

void FloatImageBuffer::colorFill(const TPixel4<float> & color)
{
	assert(numComponents >= 3); // At least RGB
	const uint32_t pixelCount = (width * height);
	uint32_t i;

	float * redChannel   = getChannel(ChRed);
	float * greenChannel = getChannel(ChGreen);
	float * blueChannel  = getChannel(ChBlue);
	float * alphaChannel = (numComponents == 4) ? getChannel(ChAlpha) : nullptr;

	for (i = 0; i < pixelCount; ++i)
	{
		redChannel[i] = color.r;
	}
	for (i = 0; i < pixelCount; ++i)
	{
		greenChannel[i] = color.g;
	}
	for (i = 0; i < pixelCount; ++i)
	{
		blueChannel[i] = color.b;
	}
	if (alphaChannel)
	{
		for (i = 0; i < pixelCount; ++i)
		{
			alphaChannel[i] = color.a;
		}
	}
}

void FloatImageBuffer::copyRect(FloatImageBuffer & destImage, const int32_t xOffset, const int32_t yOffset, const int32_t destStartX, const int32_t destStartY,
                                const uint32_t rectWidth, const uint32_t rectHeight, const bool topLeft, const WrapMode wm) const
{
	// A zero dimension wouldn't make sense
	assert(rectWidth  > 0);
	assert(rectHeight > 0);

	// Validate images:
	assert(mem != nullptr);
	assert(destImage.mem != nullptr);
	assert(getNumComponents() == destImage.getNumComponents());

	if (topLeft)
	{
		// Use the top-left corner of the image as the (0,0) origin.
		// Invert Y:
		for (uint32_t c = 0; c < numComponents; ++c)
		{
			copyChannelVFlip(*this, destImage, c, xOffset, yOffset, destStartX, destStartY, rectWidth, rectHeight, wm);
		}
	}
	else
	{
		// Use the bottom-left corner of the image as the (0,0) origin.
		// Default Y:
		for (uint32_t c = 0; c < numComponents; ++c)
		{
			copyChannel(*this, destImage, c, xOffset, yOffset, destStartX, destStartY, rectWidth, rectHeight, wm);
		}
	}
}

void FloatImageBuffer::flipVInPlace()
{
	assert(mem != nullptr);

	for (uint32_t c = 0; c < numComponents; ++c)
	{
		float * channel = getChannel(c);

		for (uint32_t y = 0; y < (height / 2); ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				swapPixelsForChannel(getIndex(x, y), getIndex(x, (height - 1) - y), channel);
			}
		}
	}
}

void FloatImageBuffer::flipHInPlace()
{
	assert(mem != nullptr);

	for (uint32_t c = 0; c < numComponents; ++c)
	{
		float * channel = getChannel(c);

		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < (width / 2); ++x)
			{
				swapPixelsForChannel(getIndex(x, y), getIndex((width - 1) - x, y), channel);
			}
		}
	}
}

const float * FloatImageBuffer::getChannel(const uint32_t c) const
{
	assert(mem != nullptr);
	assert(c < numComponents);
	return (mem + c * getActualPixelCount());
}

float * FloatImageBuffer::getChannel(const uint32_t c)
{
	assert(mem != nullptr);
	assert(c < numComponents);
	return (mem + c * getActualPixelCount());
}

uint32_t FloatImageBuffer::getIndex(const uint32_t x, const uint32_t y) const
{
	assert(x < width);
	assert(y < height);
	return (y * width + x);
}

uint32_t FloatImageBuffer::getIndexClamp(const int32_t x, const int32_t y) const
{
	return clampTo(y, int32_t(0), int32_t(height - 1)) * width + clampTo(x, int32_t(0), int32_t(width - 1));
}

uint32_t FloatImageBuffer::getIndexRepeat(const int32_t x, const int32_t y) const
{
	struct Repeat {
		static int32_t remainder(int32_t a, int32_t b)
		{
			if (a >= 0)
			{
				return (a % b);
			}
			else
			{
				return ((a + 1) % b + b - 1);
			}
		}
	};
	return Repeat::remainder(y, height) * width + Repeat::remainder(x, width);
}

uint32_t FloatImageBuffer::getIndexMirror(int32_t x, int32_t y) const
{
	if (width == 1)
	{
		x = 0;
	}

	x = abs(static_cast<int>(x));
	while (x >= static_cast<int32_t>(width))
	{
		x = abs(static_cast<int>(width + width - x - 2));
	}

	if (height == 1)
	{
		y = 0;
	}

	y = abs(static_cast<int>(y));
	while (y >= static_cast<int32_t>(height))
	{
		y = abs(static_cast<int>(height + height - y - 2));
	}

	return getIndex(x, y);
}

uint32_t FloatImageBuffer::getIndexRepeatFirstPixel(int32_t x, int32_t y) const
{
	if ((static_cast<uint32_t>(x) >= width) || (static_cast<uint32_t>(y) >= height))
	{
		x = y = 0;
	}
	return getIndex(x, y);
}

uint32_t FloatImageBuffer::getIndexClampToBlack(const int32_t x, const int32_t y) const
{
	if ((static_cast<uint32_t>(x) >= width) || (static_cast<uint32_t>(y) >= height))
	{
		return (width * height); // Return index of last "hidden" black pixel
	}
	return getIndex(x, y);
}

uint32_t FloatImageBuffer::getIndex(const int32_t x, const int32_t y, const WrapMode wm) const
{
	switch (wm)
	{
	case Clamp :
		return getIndexClamp(x, y);
	case Repeat :
		return getIndexRepeat(x, y);
	case Mirror :
		return getIndexMirror(x, y);
	case RepeatFirstPixel :
		return getIndexRepeatFirstPixel(x, y);
	case ClampToBlack :
		return getIndexClampToBlack(x, y);
	default :
		assert(false && "Invalid FloatImageBuffer::WrapMode!");
	} // switch (wm)
}

} // namespace tool {}
} // namespace vt {}
