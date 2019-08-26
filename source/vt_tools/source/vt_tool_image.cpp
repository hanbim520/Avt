
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_image.cpp
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

#include "vt_tool_image.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace vt
{
namespace tool
{

// ======================================================
// PixelFormat:
// ======================================================

const char * PixelFormat::toString(const PixelFormat::Enum pixelFormat)
{
	switch (pixelFormat)
	{
	case PixelFormat::Invalid : return "Invalid/Undefined";
	case PixelFormat::RgbU8   : return "RgbU8";
	case PixelFormat::RgbF32  : return "RgbF32";
	case PixelFormat::RgbaU8  : return "RgbaU8";
	case PixelFormat::RgbaF32 : return "RgbaF32";
	default : assert(false && "Invalid pixel format!");
	} // switch (pixelFormat)
}

size_t PixelFormat::sizeBytes(const PixelFormat::Enum pixelFormat)
{
	switch (pixelFormat)
	{
	case PixelFormat::Invalid : return 0; // Null value. Not used
	case PixelFormat::RgbU8   : return sizeof(uint8_t) * 3;
	case PixelFormat::RgbF32  : return sizeof(float)   * 3;
	case PixelFormat::RgbaU8  : return sizeof(uint8_t) * 4;
	case PixelFormat::RgbaF32 : return sizeof(float)   * 4;
	default : assert(false && "Invalid pixel format!");
	} // switch (pixelFormat)
}

uint32_t PixelFormat::componentCount(const PixelFormat::Enum pixelFormat)
{
	switch (pixelFormat)
	{
	case PixelFormat::Invalid : return 0;
	case PixelFormat::RgbU8   : return 3;
	case PixelFormat::RgbF32  : return 3;
	case PixelFormat::RgbaU8  : return 4;
	case PixelFormat::RgbaF32 : return 4;
	default : assert(false && "Invalid pixel format!");
	} // switch (pixelFormat)
}

// ======================================================
// Local helpers:
// ======================================================

namespace {

// STB Image has a few apparently harmless warnings.
// I'll silence them till proven that they are not!
#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wshadow"
	#pragma GCC diagnostic ignored "-Wunused-function"
#endif // __GNUC__

// STB Image library:
// (NO_HDR to disable the High Dynamic Range (hdr) image loader).
#define STBI_NO_HDR              1
#define STB_IMAGE_IMPLEMENTATION 1
#include "stb/stb_image.h"

// Restore warnings:
#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif // __GNUC__

// ======================================================

// A big value, enough to hold the largest pixel size imaginable and more.
// Used internally for allocation-less image resizing/transforming.
// The largest pixel format we deal with is RGBA float, which is just 16bytes wide.
constexpr uint32_t bigPixelSizeBytes = 256;

// Round an integer to a power-of-two that is not greater than 'x'.
template<class T>
T floorPowerOfTwo(const T x)
{
	T p2;
	for (p2 = 1; (p2 * 2) <= x; p2 <<= 1)
	{
		// Next...
	}
	return p2;
}

} // namespace {}

// ======================================================
// Image:
// ======================================================

Image::Image()
	: data(nullptr)
	, dataSizeBytes(0)
	, width(0)
	, height(0)
	, pixelFormat(PixelFormat::Invalid)
{
}

Image::Image(const Image & other)
	: data(nullptr)
	, dataSizeBytes(0)
	, width(0)
	, height(0)
	, pixelFormat(PixelFormat::Invalid)
{
	initFromCopy(other);
}

Image & Image::operator = (const Image & other)
{
	freeImageStorage();
	initFromCopy(other);
	return *this;
}

Image::~Image()
{
	freeImageStorage();
}

bool Image::loadFromFile(const std::string & filename, std::string * errorMessage, const bool forceRGBA)
{
	assert(!filename.empty());
	assert(data == nullptr && "Creating new image would cause a memory leak!");

	int w, h, comp;
	data = reinterpret_cast<uint8_t *>(stbi_load(filename.c_str(), &w, &h, &comp, (forceRGBA ? 4 : 0)));

	if (data == nullptr)
	{
		if (errorMessage != nullptr)
		{
			errorMessage->assign("\'stbi_load()\' failed with error: ");
			errorMessage->append(stbi_failure_reason());
		}
		return false;
	}

	if (forceRGBA)
	{
		pixelFormat = PixelFormat::RgbaU8;
		comp = 4;
	}
	else
	{
		switch (comp)
		{
		case 3 : // RGB
			pixelFormat = PixelFormat::RgbU8;
			break;

		case 4 : // RGBA
			pixelFormat = PixelFormat::RgbaU8;
			break;

		default : // Unsupported
			{
				std::free(data);
				data = nullptr;
				if (errorMessage != nullptr)
				{
					errorMessage->assign(filename);
					errorMessage->append(" image format not supported! comp = ");
					errorMessage->append(std::to_string(comp));
				}
				return false;
			}
		} // switch (comp)
	}

	// Non power-of-2 dimensions check:
	if (((w & (w - 1)) != 0) || ((h & (h - 1)) != 0))
	{
		// NOTE: This should be improved in the future.
		// We should define a configuration parameter that enables
		// downscaling a non-POT image.
		//
		if (errorMessage != nullptr)
		{
			errorMessage->assign(filename);
			errorMessage->append(" image does not have power-of-2 dimensions!");
		}
		// Allow it to continue...
	}

	// Set dimensions/size and we are done.
	width  = w;
	height = h;
	dataSizeBytes = (w * h * comp);

	return true;
}

void Image::makeColorFilledImage(const uint32_t w, const uint32_t h, const PixelFormat::Enum pf, const uint8_t * color)
{
	assert(color != nullptr);

	const size_t pixelSize = PixelFormat::sizeBytes(pf);
	if ((width != w) || (height != h) || (pixelFormat != pf))
	{
		freeImageStorage();
		allocImageStorage((w * h * pixelSize), w, h, pf);
	}

	for (uint32_t y = 0; y < h; ++y)
	{
		for (uint32_t x = 0; x < w; ++x)
		{
			std::memcpy((data + (x + y * w) * pixelSize), color, pixelSize);
		}
	}
}

void Image::makeCheckerPatternImage(const uint32_t numSquares)
{
	assert(numSquares >= 2 && numSquares <= 64);

	const uint32_t imgSize = 64; // Image dimensions (w & h) in pixels.
	const uint32_t checkerSize = (imgSize / numSquares); // Size of one checker square, in pixels.

	// One square black and one white:
	const uint8_t blackWhite[][4] = {
		{ 0,   0,   0,   255 },
		{ 255, 255, 255, 255 }
	};

	freeImageStorage();
	allocImageStorage((imgSize * imgSize * 4), imgSize, imgSize, PixelFormat::RgbaU8); // RGBA image

	uint32_t startY    = 0;
	uint32_t lastColor = 0;
	uint32_t color, rowX;

	while (startY < imgSize)
	{
		for (uint32_t y = startY; y < (startY + checkerSize); ++y)
		{
			color = lastColor, rowX = 0;

			for (uint32_t x = 0; x < imgSize; ++x)
			{
				if (rowX == checkerSize)
				{
					// Invert color every time we complete a checker box:
					color = !color;
					rowX = 0;
				}

				reinterpret_cast<uint32_t *>(data)[x + y * width] =
						(*reinterpret_cast<const uint32_t *>(blackWhite[color]));

				++rowX;
			}
		}

		startY += checkerSize;
		lastColor = !lastColor;
	}
}

bool Image::isPowerOfTwo() const
{
	return (((width & (width  - 1)) == 0)
		&& ((height & (height - 1)) == 0));
}

bool Image::isValid() const
{
	return (width != 0) && (height != 0) &&
		(pixelFormat != PixelFormat::Invalid) &&
		(data != nullptr) && (dataSizeBytes != 0);
}

uint8_t * Image::allocImageStorage(const size_t dataSize, const uint32_t w, const uint32_t h, const PixelFormat::Enum pf)
{
	assert(data == nullptr && "freeImageStorage() before allocating new one!");

	// Validate parameters:
	assert(w > 0 && h > 0);
	assert(dataSize != 0);
	assert(pf != PixelFormat::Invalid);

	// Save parameters:
	width         = w;
	height        = h;
	pixelFormat   = pf;
	dataSizeBytes = dataSize;

	// Allocate system memory for image data:
	data = reinterpret_cast<uint8_t *>(std::malloc(dataSizeBytes));
	return data;
}

void Image::freeImageStorage()
{
	if (data != nullptr) // Don't waste time nor print message if no data is present:
	{
		std::free(data);

		data          = nullptr;
		dataSizeBytes = 0;
		width         = 0;
		height        = 0;
		pixelFormat   = PixelFormat::Invalid;
	}
}

void Image::initFromCopy(const Image & img)
{
	// Allocate storage:
	allocImageStorage(img.dataSizeBytes, img.width, img.height, img.pixelFormat);

	// Copy 'img' data to this image:
	std::memcpy(data, img.data, img.dataSizeBytes);
}

void Image::moveCopy(Image & img)
{
	// Move:
	data              = img.data;
	dataSizeBytes     = img.dataSizeBytes;
	width             = img.width;
	height            = img.height;
	pixelFormat       = img.pixelFormat;

	// Invalidate the other:
	img.data          = nullptr;
	img.dataSizeBytes = 0;
	img.width         = 0;
	img.height        = 0;
	img.pixelFormat   = PixelFormat::Invalid;
}

void Image::getPixelAt(const uint32_t x, const uint32_t y, uint8_t * pixel) const
{
	assert(isValid());
	assert(x < width);
	assert(y < height);
	assert(pixel != nullptr);

	switch (pixelFormat)
	{
	case PixelFormat::RgbU8 :
		{
			const TPixel3<uint8_t> * rgb8 = &reinterpret_cast<const TPixel3<uint8_t> *>(data)[x + y * width];
			(*reinterpret_cast<TPixel3<uint8_t> *>(pixel)).r = rgb8->r;
			(*reinterpret_cast<TPixel3<uint8_t> *>(pixel)).g = rgb8->g;
			(*reinterpret_cast<TPixel3<uint8_t> *>(pixel)).b = rgb8->b;
			break;
		}
	case PixelFormat::RgbF32 :
		{
			const TPixel3<float> * rgb32 = &reinterpret_cast<const TPixel3<float> *>(data)[x + y * width];
			(*reinterpret_cast<TPixel3<float> *>(pixel)).r = rgb32->r;
			(*reinterpret_cast<TPixel3<float> *>(pixel)).g = rgb32->g;
			(*reinterpret_cast<TPixel3<float> *>(pixel)).b = rgb32->b;
			break;
		}
	case PixelFormat::RgbaU8 :
		{
			// 4-bytes long, use an unsigned integer instead:
			(*reinterpret_cast<uint32_t *>(pixel)) =
				reinterpret_cast<const uint32_t *>(data)[x + y * width];
			break;
		}
	case PixelFormat::RgbaF32 :
		{
			const TPixel4<float> * rgba32 = &reinterpret_cast<const TPixel4<float> *>(data)[x + y * width];
			(*reinterpret_cast<TPixel4<float> *>(pixel)).r = rgba32->r;
			(*reinterpret_cast<TPixel4<float> *>(pixel)).g = rgba32->g;
			(*reinterpret_cast<TPixel4<float> *>(pixel)).b = rgba32->b;
			(*reinterpret_cast<TPixel4<float> *>(pixel)).a = rgba32->a;
			break;
		}
	default:
		assert(false && "Bad image pixel format!");
	} // switch (pixelFormat)
}

void Image::setPixelAt(const uint32_t x, const uint32_t y, const uint8_t * pixel)
{
	assert(isValid());
	assert(x < width);
	assert(y < height);
	assert(pixel != nullptr);

	switch (pixelFormat)
	{
	case PixelFormat::RgbU8 :
		{
			TPixel3<uint8_t> * rgb8 = &reinterpret_cast<TPixel3<uint8_t> *>(data)[x + y * width];
			rgb8->r = (*reinterpret_cast<const TPixel3<uint8_t> *>(pixel)).r;
			rgb8->g = (*reinterpret_cast<const TPixel3<uint8_t> *>(pixel)).g;
			rgb8->b = (*reinterpret_cast<const TPixel3<uint8_t> *>(pixel)).b;
			break;
		}
	case PixelFormat::RgbF32 :
		{
			TPixel3<float> * rgb32 = &reinterpret_cast<TPixel3<float> *>(data)[x + y * width];
			rgb32->r = (*reinterpret_cast<const TPixel3<float> *>(pixel)).r;
			rgb32->g = (*reinterpret_cast<const TPixel3<float> *>(pixel)).g;
			rgb32->b = (*reinterpret_cast<const TPixel3<float> *>(pixel)).b;
			break;
		}
	case PixelFormat::RgbaU8 :
		{
			// 4-bytes long, use an unsigned integer instead:
			reinterpret_cast<uint32_t *>(data)[x + y * width] =
				(*reinterpret_cast<const uint32_t *>(pixel));
			break;
		}
	case PixelFormat::RgbaF32 :
		{
			TPixel4<float> * rgba32 = &reinterpret_cast<TPixel4<float> *>(data)[x + y * width];
			rgba32->r = (*reinterpret_cast<const TPixel4<float> *>(pixel)).r;
			rgba32->g = (*reinterpret_cast<const TPixel4<float> *>(pixel)).g;
			rgba32->b = (*reinterpret_cast<const TPixel4<float> *>(pixel)).b;
			rgba32->a = (*reinterpret_cast<const TPixel4<float> *>(pixel)).a;
			break;
		}
	default:
		assert(false && "Bad image pixel format!");
	} // switch (pixelFormat)
}

void Image::swapPixels(const uint32_t x0, const uint32_t y0, const uint32_t x1, const uint32_t y1)
{
	uint8_t tmpPixel0[bigPixelSizeBytes];
	uint8_t tmpPixel1[bigPixelSizeBytes];

	getPixelAt(x0, y0, tmpPixel0);
	getPixelAt(x1, y1, tmpPixel1);

	setPixelAt(x0, y0, tmpPixel1);
	setPixelAt(x1, y1, tmpPixel0);
}

void Image::doForEveryPixel(void (* func)(uint8_t *, PixelFormat::Enum))
{
	assert(isValid());
	assert(func != nullptr);

	const size_t pixelCount     = (width * height);
	const size_t pixelSizeBytes = PixelFormat::sizeBytes(pixelFormat);
	uint8_t * pixelPtr          = data;

	for (size_t i = 0; i < pixelCount; ++i)
	{
		func(pixelPtr, pixelFormat);
		pixelPtr += pixelSizeBytes;
	}
}

void Image::flipVInPlace()
{
	assert(isValid());

	for (uint32_t y = 0; y < (height / 2); ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			swapPixels(x, y, x, ((height - 1) - y));
		}
	}
}

void Image::flipHInPlace()
{
	assert(isValid());

	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < (width / 2); ++x)
		{
			swapPixels(x, y, ((width - 1) - x), y);
		}
	}
}

void Image::flipV(Image & destImage) const
{
	assert(isValid());

	// Allocate a new image:
	destImage.freeImageStorage();
	destImage.allocImageStorage(dataSizeBytes, width, height, pixelFormat);

	// Flip it:
	uint8_t tmpPixel[bigPixelSizeBytes];
	const uint32_t maxWidthIndex  = (width  - 1);
	const uint32_t maxHeightIndex = (height - 1);

	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			getPixelAt((maxWidthIndex - x), y, tmpPixel);
			destImage.setPixelAt((maxWidthIndex - x), (maxHeightIndex - y), tmpPixel);
		}
	}
}

void Image::flipH(Image & destImage) const
{
	assert(isValid());

	// Allocate a new image:
	destImage.freeImageStorage();
	destImage.allocImageStorage(dataSizeBytes, width, height, pixelFormat);

	// Flip it:
	uint8_t tmpPixel[bigPixelSizeBytes];
	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; )
		{
			getPixelAt(x, y, tmpPixel);
			x = x + 1;
			destImage.setPixelAt((width - x), y, tmpPixel);
		}
	}
}

void Image::resizeInPlace(const uint32_t targetWidth, const uint32_t targetHeight)
{
	if ((width == targetWidth) && (height == targetHeight))
	{
		return; // Nothing to be done here...
	}

	// Resulting image may be bigger or smaller.
	// We always need a copy:
	Image resizedImage;
	resize(resizedImage, targetWidth, targetHeight);
	freeImageStorage();
	moveCopy(resizedImage);
}

void Image::resize(Image & destImage, const uint32_t targetWidth, const uint32_t targetHeight) const
{
	assert(isValid());
	assert(targetWidth  > 0);
	assert(targetHeight > 0);

	if ((width == targetWidth) && (height == targetHeight))
	{
		return; // Nothing to be done here...
	}

	const size_t pixelSize = PixelFormat::sizeBytes(pixelFormat);
	destImage.freeImageStorage();
	destImage.allocImageStorage((targetWidth * targetHeight * pixelSize), targetWidth, targetHeight, pixelFormat);

	// Quick resample with no filtering applied:
	const double scaleWidth  = static_cast<double>(targetWidth)  / static_cast<double>(width);
	const double scaleHeight = static_cast<double>(targetHeight) / static_cast<double>(height);
	uint8_t tmpPixel[bigPixelSizeBytes];

	for (uint32_t y = 0; y < targetHeight; ++y)
	{
		for (uint32_t x = 0; x < targetWidth; ++x)
		{
			getPixelAt(static_cast<uint32_t>(x / scaleWidth), static_cast<uint32_t>(y / scaleHeight), tmpPixel);
			destImage.setPixelAt(x, y, tmpPixel);
		}
	}
}

void Image::roundDownToPowerOfTwo()
{
	assert(isValid());

	// Minimum size for rounding.
	// If an image's w|h power of 2 is less than 16, we clamp it to 16.
	constexpr uint32_t minSize = 16;

	if (isPowerOfTwo())
	{
		return; // Nothing to do...
	}

	// Round down sizes:
	uint32_t targetWidth = floorPowerOfTwo(width);
	if (targetWidth < minSize) // Never go lower than the minimum:
	{
		targetWidth = minSize;
	}

	uint32_t targetHeight = floorPowerOfTwo(height);
	if (targetHeight < minSize) // Never go lower than the minimum:
	{
		targetHeight = minSize;
	}

	resizeInPlace(targetWidth, targetHeight);
}

void Image::swizzleRGB()
{
	assert(isValid());

	// Swizzle every pixel:
	const size_t pixelCount         = (width * height);
	const size_t componentsPerPixel = PixelFormat::componentCount(pixelFormat);
	uint8_t * pixelPtr              = data;

	for (size_t i = 0; i < pixelCount; ++i)
	{
		std::swap(pixelPtr[0], pixelPtr[2]);
		pixelPtr += componentsPerPixel;
	}
}

void Image::discardAlphaComponent()
{
	assert(isValid());
	if ((pixelFormat != PixelFormat::RgbaU8) && (pixelFormat != PixelFormat::RgbaF32))
	{
		return;
	}

	// Resulting image will be smaller. We need a new one:
	Image rgbImage;
	rgbImage.allocImageStorage((width * height * PixelFormat::sizeBytes(pixelFormat)), width, height,
			(pixelFormat == PixelFormat::RgbaU8) ? PixelFormat::RgbU8 : PixelFormat::RgbF32);

	uint8_t tmpPixel[bigPixelSizeBytes];
	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			getPixelAt(x, y, tmpPixel);
			rgbImage.setPixelAt(x, y, tmpPixel); // Since rgbImage is RGB, the alpha will be dropped by setPixelAt.
		}
	}

	freeImageStorage();
	moveCopy(rgbImage);
}

void Image::memSet(const int val)
{
	assert(isValid());
	std::memset(data, val, dataSizeBytes);
}

void Image::copyRect(Image & destImage, const uint32_t xOffset, const uint32_t yOffset,
                     const uint32_t rectWidth, const uint32_t rectHeight, const bool topLeft) const
{
	assert(isValid());
	assert(rectHeight != 0 && rectWidth != 0);

	if ((destImage.pixelFormat != pixelFormat) ||
		(destImage.width  < rectWidth)         ||
		(destImage.height < rectHeight))
	{
		destImage.freeImageStorage();
		destImage.allocImageStorage((rectWidth * rectHeight * PixelFormat::sizeBytes(pixelFormat)),
				rectWidth, rectHeight, pixelFormat);
	}

	uint8_t tmpPixel[bigPixelSizeBytes];

	if (topLeft)
	{
		// Use the top-left corner of the image as the (0,0) origin.
		// Invert Y:
		uint32_t maxY = (getHeight() - 1);
		for (uint32_t y = 0; y < rectHeight; ++y)
		{
			for (uint32_t x = 0; x < rectWidth; ++x)
			{
				getPixelAt(x + xOffset, maxY - yOffset, tmpPixel);
				destImage.setPixelAt(x, y, tmpPixel);
			}
			--maxY;
		}
	}
	else
	{
		// Use the bottom-left corner of the image as the (0,0) origin.
		// Default Y:
		for (uint32_t y = 0; y < rectHeight; ++y)
		{
			for (uint32_t x = 0; x < rectWidth; ++x)
			{
				getPixelAt(x + xOffset, y + yOffset, tmpPixel);
				destImage.setPixelAt(x, y, tmpPixel);
			}
		}
	}
}

void Image::setRect(const Image & srcImage, const uint32_t xOffset, const uint32_t yOffset,
                    const uint32_t rectWidth, const uint32_t rectHeight, const bool topLeft)
{
	// Validation:
	assert(isValid());
	assert(srcImage.isValid());
	assert(rectHeight != 0 && rectWidth != 0);

	uint8_t tmpPixel[bigPixelSizeBytes];

	if (topLeft)
	{
		// Use the top-left corner of the image as the (0,0) origin.
		// Invert Y:
		uint32_t maxY = (getHeight() - 1);
		for (uint32_t y = 0; y < rectHeight; ++y)
		{
			for (uint32_t x = 0; x < rectWidth; ++x)
			{
				srcImage.getPixelAt(x, y, tmpPixel);
				setPixelAt(x + xOffset, maxY - yOffset, tmpPixel);
			}
			--maxY;
		}
	}
	else
	{
		// Use the bottom-left corner of the image as the (0,0) origin.
		// Default Y:
		for (uint32_t y = 0; y < rectHeight; ++y)
		{
			for (uint32_t x = 0; x < rectWidth; ++x)
			{
				srcImage.getPixelAt(x, y, tmpPixel);
				setPixelAt(x + xOffset, y + yOffset, tmpPixel);
			}
		}
	}
}

void Image::setRect(const uint8_t * __restrict image, const uint32_t xOffset, const uint32_t yOffset,
                    const uint32_t rectWidth, const uint32_t rectHeight, const bool topLeft)
{
	// Validation:
	assert(isValid());
	assert(image != nullptr);
	assert(rectHeight != 0 && rectWidth != 0);

	const size_t pixelSize = PixelFormat::sizeBytes(pixelFormat);
	uint8_t tmpPixel[bigPixelSizeBytes];

	if (topLeft)
	{
		// Use the top-left corner of the image as the (0,0) origin.
		// Invert Y:
		uint32_t maxY = (getHeight() - 1);
		for (uint32_t y = 0; y < rectHeight; ++y)
		{
			for (uint32_t x = 0; x < rectWidth; ++x)
			{
				std::memcpy(tmpPixel, (image + (x + y * rectWidth) * pixelSize), pixelSize);
				setPixelAt(x + xOffset, maxY - yOffset, tmpPixel);
			}
			--maxY;
		}
	}
	else
	{
		// Use the bottom-left corner of the image as the (0,0) origin.
		// Default Y:
		for (uint32_t y = 0; y < rectHeight; ++y)
		{
			for (uint32_t x = 0; x < rectWidth; ++x)
			{
				std::memcpy(tmpPixel, (image + (x + y * rectWidth) * pixelSize), pixelSize);
				setPixelAt(x + xOffset, y + yOffset, tmpPixel);
			}
		}
	}
}

} // namespace tool {}
} // namespace vt {}
