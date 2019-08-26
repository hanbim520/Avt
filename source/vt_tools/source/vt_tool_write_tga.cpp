
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_write_tga.cpp
// Author: Guilherme R. Lampert
// Created on: 24/10/14
// Brief: Helper function for TGA image output.
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

#include <cstdint>
#include <cassert>
#include <cstdio>
#include <string>

namespace vt
{
namespace tool
{

// ======================================================
// writeTgaImage():
// ======================================================

bool writeTgaImage(const std::string & filename, const int width, const int height, const int colorComps,
                   const uint8_t * imageData, const bool swizzle, std::string * resultMessage)
{
	assert(!filename.empty());
	assert(width  != 0);
	assert(height != 0);
	assert(imageData != nullptr);

	// Supported formats: 3 or 4 components
	if ((colorComps != 3) && (colorComps != 4))
	{
		if (resultMessage != nullptr)
		{
			resultMessage->assign("Currently writeTgaImage() can only handle RGB/BGR (u8) and RGBA/BGRA (u8) images!");
		}
		return false;
	}

	FILE * file = std::fopen(filename.c_str(), "wb");
	if (file == nullptr)
	{
		if (resultMessage != nullptr)
		{
			resultMessage->assign("Unable to open file \"" + filename + "\" for writing!");
		}
		return false;
	}

	struct TGAImageDescriptor {
		uint8_t alpha    : 4;
		uint8_t right    : 1;
		uint8_t top      : 1;
		uint8_t reserved : 2;
	};

	static_assert(sizeof(TGAImageDescriptor) == 1, "Bad TGAImageDescriptor size!");
	TGAImageDescriptor imageDesc = {0,0,0,0};
	uint8_t tgaType = 2, bpp;

	// RGB/RGBA
	if (colorComps == 3)
	{
		bpp = 24;
	}
	else // RGBA/BGRA
	{
		imageDesc.alpha = 8; // 8bit alpha
		bpp = 32;
	}

	// Other unused fields of the header:
	const uint8_t  idLenght     = 0; // unused
	const uint8_t  hasColormap  = 0; // unused
	const uint16_t cmFirstEntry = 0; // unused
	const uint16_t cmLength     = 0; // unused
	const uint8_t  cmSize       = 0; // unused
	const uint16_t xOrigin      = 0; // start at x:0
	const uint16_t yOrigin      = 0; // start at y:0
	const uint16_t dimX         = static_cast<uint16_t>(width);
	const uint16_t dimY         = static_cast<uint16_t>(height);

	// Write the image TGA header:
	std::fwrite(&idLenght,     sizeof(idLenght),     1, file);
	std::fwrite(&hasColormap,  sizeof(hasColormap),  1, file);
	std::fwrite(&tgaType,      sizeof(tgaType),      1, file);
	std::fwrite(&cmFirstEntry, sizeof(cmFirstEntry), 1, file);
	std::fwrite(&cmLength,     sizeof(cmLength),     1, file);
	std::fwrite(&cmSize,       sizeof(cmSize),       1, file);
	std::fwrite(&xOrigin,      sizeof(xOrigin),      1, file);
	std::fwrite(&yOrigin,      sizeof(yOrigin),      1, file);
	std::fwrite(&dimX,         sizeof(dimX),         1, file);
	std::fwrite(&dimY,         sizeof(dimY),         1, file);
	std::fwrite(&bpp,          sizeof(bpp),          1, file);
	std::fwrite(&imageDesc,    sizeof(imageDesc),    1, file);

	// Check for IO problems...
	if (std::ferror(file))
	{
		if (resultMessage != nullptr)
		{
			resultMessage->assign("Error while writing TGA header! \"" + filename + "\"");
		}
		std::fclose(file);
		return false;
	}

	// Now write the pixels:
	const unsigned int pixelCount = (width * height);
	if (colorComps == 3)
	{
		if (swizzle)
		{
			uint8_t pixel[3];
			for (unsigned int i = 0; i < pixelCount; ++i)
			{
				// RGB <=> BGR
				pixel[0] = imageData[2];
				pixel[1] = imageData[1];
				pixel[2] = imageData[0];
				imageData += 3;
				if (std::fwrite(pixel, 3, 1, file) != 1)
				{
					if (resultMessage != nullptr)
					{
						resultMessage->assign("Failed to write image pixels! \"" + filename + "\"");
					}
					std::fclose(file);
					return false;
				}
			}
		}
		else
		{
			// No need to swizzle pixels, write data as it is:
			for (unsigned int i = 0; i < pixelCount; ++i, imageData += 3)
			{
				if (std::fwrite(imageData, 3, 1, file) != 1)
				{
					if (resultMessage != nullptr)
					{
						resultMessage->assign("Failed to write image pixels! \"" + filename + "\"");
					}
					std::fclose(file);
					return false;
				}
			}
		}
	}
	else
	{
		assert(colorComps == 4);
		if (swizzle)
		{
			uint8_t pixel[4];
			for (unsigned int i = 0; i < pixelCount; ++i)
			{
				// RGBA <=> BGRA
				pixel[0] = imageData[2];
				pixel[1] = imageData[1];
				pixel[2] = imageData[0];
				pixel[3] = imageData[3];
				imageData += 4;
				if (std::fwrite(pixel, 4, 1, file) != 1)
				{
					if (resultMessage != nullptr)
					{
						resultMessage->assign("Failed to write image pixels! \"" + filename + "\"");
					}
					std::fclose(file);
					return false;
				}
			}
		}
		else
		{
			// No need to swizzle pixels, write data as it is:
			for (unsigned int i = 0; i < pixelCount; ++i, imageData += 4)
			{
				if (std::fwrite(imageData, 4, 1, file) != 1)
				{
					if (resultMessage != nullptr)
					{
						resultMessage->assign("Failed to write image pixels! \"" + filename + "\"");
					}
					std::fclose(file);
					return false;
				}
			}
		}
	}

	if (resultMessage != nullptr)
	{
		resultMessage->assign("Successfully written TGA image to file \"" + filename + "\".");
	}

	std::fclose(file);
	return true;
}

} // namespace tool {}
} // namespace vt {}
