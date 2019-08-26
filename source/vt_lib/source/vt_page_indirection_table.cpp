
// ================================================================================================
// -*- C++ -*-
// File: vt_page_indirection_table.cpp
// Author: Guilherme R. Lampert
// Created on: 03/10/14
// Brief: Page indirection table texture.
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

#include "vt.hpp"
#include "vt_tool_image.hpp"
#include <cmath> // std::log2()

namespace vt
{

// Filtering is fixed for the indirection tables.
static constexpr GLenum indirectionTexMinFilter  = GL_NEAREST_MIPMAP_NEAREST;
static constexpr GLenum indirectionTexMagFilter  = GL_NEAREST;
static constexpr GLenum indirectionTexAddressing = GL_REPEAT;

// ======================================================
// PageIndirectionTable:
// ======================================================

PageIndirectionTable::PageIndirectionTable(const int * vtPagesX, const int * vtPagesY, const int vtNumLevels)
	: indirectionTextureId(0)
	, numLevels(vtNumLevels)
{
	assert(numLevels > 0 && numLevels <= MaxVTMipLevels);

	clearArray(numPagesX);
	clearArray(numPagesY);
	for (int l = 0; l < numLevels; ++l)
	{
		assert(vtPagesX[l] > 0);
		assert(vtPagesY[l] > 0);
		numPagesX[l] = vtPagesX[l];
		numPagesY[l] = vtPagesY[l];
	}
}

PageIndirectionTable::~PageIndirectionTable()
{
	gl::delete2DTexture(indirectionTextureId);
}

void PageIndirectionTable::bind(const int texUnit) const
{
	gl::use2DTexture(indirectionTextureId, texUnit);
}

void PageIndirectionTable::visualizeIndirectionTexture(const float overlayScale[2]) const
{
	gl::useShaderProgram(getGlobalShaders().drawIndirectionTable.programId);
	gl::setShaderProgramUniform(getGlobalShaders().drawIndirectionTable.unifNdcQuadScale, overlayScale, 2);

	// Draw a quad with the texture applied to it:
	gl::use2DTexture(indirectionTextureId);
	gl::drawNdcQuadrilateral();
	gl::use2DTexture(0);

	gl::useShaderProgram(0);
}

// ======================================================
// PageIndirectionTableRgba8888:
// ======================================================

PageIndirectionTableRgba8888::PageIndirectionTableRgba8888(const int * vtPagesX, const int * vtPagesY, const int vtNumLevels)
	: PageIndirectionTable(vtPagesX, vtPagesY, vtNumLevels)
	, totalTableEntries(0)
{
	clearArray(tableLevels);
	initTexture();

	vtLogComment("New PageIndirectionTable RGBA-8:8:8:8 instance created...");
}

void PageIndirectionTableRgba8888::initTexture()
{
	assert(indirectionTextureId == 0 && "Duplicate initialization!");
	vtLogComment("Initializing page indirection texture with " << numLevels << " levels...");

	// Count total texture size, including all mip-levels:
	totalTableEntries = 0;
	for (int l = 0; l < numLevels; ++l)
	{
		totalTableEntries += numPagesX[l] * numPagesY[l];
	}

	tableEntryPool.reset(new TableEntry[totalTableEntries]);

	// Set up the pointers:
	totalTableEntries = 0;
	for (int l = 0; l < numLevels; ++l)
	{
		tableLevels[l] = tableEntryPool.get() + totalTableEntries;
		totalTableEntries += numPagesX[l] * numPagesY[l]; // Move to the next level
	}

	glGenTextures(1, &indirectionTextureId);
	if (indirectionTextureId == 0)
	{
		vtFatalError("Failed to generate a non-zero GL texture id for the page indirection texture!");
	}

	gl::use2DTexture(indirectionTextureId);

	// Create/set all the mip-levels:
	for (int l = 0; l < numLevels; ++l)
	{
		assert(tableLevels[l] != nullptr);

		// Default initialize the entries:
		for (int e = 0; e < numPagesX[l] * numPagesY[l]; ++e)
		{
			TableEntry & entry = tableLevels[l][e];
			entry.cachePageX = 0;
			entry.cachePageY = 0;

			const uint16_t scale = (numPagesX[0] * 16) >> l;
			entry.scaleHigh = (scale & 0xFF);
			entry.scaleLow  = (scale >> 8);
		}

		glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA, numPagesX[l], numPagesY[l], 0, GL_RGBA, GL_UNSIGNED_BYTE, tableLevels[l]);
		vtLogComment("Allocated indirection tex level #" << l << ". Size: " << numPagesX[l] << "x" << numPagesY[l] << " pixels.");

		#if VT_EXTRA_GL_ERROR_CHECKING
		gl::checkGLErrors(__FILE__, __LINE__);
		#endif // VT_EXTRA_GL_ERROR_CHECKING
	}

	// iOS specific: Set max level (would probably have to use glGenerateMipmap() otherwise...)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_APPLE, numLevels - 1);

	// Set addressing mode:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, indirectionTexAddressing);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, indirectionTexAddressing);

	// Set filtering:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, indirectionTexMinFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, indirectionTexMagFilter);

	gl::use2DTexture(0);
	gl::checkGLErrors(__FILE__, __LINE__);

	vtLogComment("Page indirection texture #" << indirectionTextureId << " created. Num entries: "
			<< totalTableEntries << ". " << numLevels << " levels.");
}

void PageIndirectionTableRgba8888::updateIndirectionTexture(const CacheEntry * __restrict const pages)
{
	assert(pages != nullptr);

	// Texture must be already bound!
	// GL_UNPACK_ALIGNMENT should ideally be set to 4.
	assert(indirectionTextureId == gl::getCurrent2DTexture());

	// Assemble the indirection table, one mip-level at a time,
	// stating from the lowest resolution one:
	for (int l = (numLevels - 1); l >= 0; --l)
	{
		// Write all pages in a level:
		for (int p = 0; p < PageCacheMgr::TotalCachePages; ++p)
		{
			const CacheEntry & cacheEntry = pages[p];
			if ((pageIdExtractMipLevel(cacheEntry.pageId) != l) || (cacheEntry.pageId == InvalidPageId))
			{
				continue;
			}

			const int x = pageIdExtractPageX(cacheEntry.pageId);
			const int y = pageIdExtractPageY(cacheEntry.pageId);
			const int index = (x + y * numPagesX[l]);

			assert(index >= 0);
			assert(index < (numPagesX[l] * numPagesY[l]));
			TableEntry & entry = tableLevels[l][index];

			entry.cachePageX = cacheEntry.cacheCoord.x;
			entry.cachePageY = cacheEntry.cacheCoord.y;

			const uint16_t scale = (numPagesX[0] * 16) >> l;
			entry.scaleHigh = (scale & 0xFF);
			entry.scaleLow  = (scale >> 8);
		}

		// Upsample for next level:
		if (l != 0)
		{
			uint32_t * __restrict src  = reinterpret_cast<uint32_t *>(tableLevels[l]);
			uint32_t * __restrict dest = reinterpret_cast<uint32_t *>(tableLevels[l - 1]);

			const int srcW  = numPagesX[l];
			const int destW = numPagesX[l - 1];
			const int destH = numPagesY[l - 1];

			for (int y = 0; y < destH; ++y)
			{
				for (int x = 0; x < destW; ++x)
				{
					dest[x + y * destW] = src[(x >> 1) + (y >> 1) * srcW];
				}
			}
		}
	}

	for (int l = 0; l < numLevels; ++l)
	{
		glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA, numPagesX[l], numPagesY[l], 0, GL_RGBA, GL_UNSIGNED_BYTE, tableLevels[l]);

		#if VT_EXTRA_GL_ERROR_CHECKING
		gl::checkGLErrors(__FILE__, __LINE__);
		#endif // VT_EXTRA_GL_ERROR_CHECKING
	}
}

bool PageIndirectionTableRgba8888::writeIndirectionTextureToFile(const std::string & pathname, const bool recolor) const
{
	int levelsWritten = 0;
	std::string levelNameStr, result;

	for (int l = 0; l < numLevels; ++l)
	{
		Pixel4b * __restrict pixels = reinterpret_cast<Pixel4b *>(tableLevels[l]);

		// Reverse the bits in the image pixels to make them stand out.
		// Most of the pixels would be very dark otherwise.
		if (recolor)
		{
			const size_t numPixels = numPagesX[l] * numPagesY[l];
			for (size_t p = 0; p < numPixels; ++p)
			{
				Pixel4b & pix = pixels[p];
				pix.r = reverseByte(pix.r);
				pix.g = reverseByte(pix.g);
				// Mix alpha (the texture index) it with blue:
				uint8_t b = reverseByte(pix.b);
				uint8_t a = reverseByte(pix.a);
				pix.b = clampByte(a + b);
				pix.a = 0xFF;
			}
		}

		levelNameStr = pathname + "_" + std::to_string(l) + ".tga";
		if (tool::writeTgaImage(levelNameStr, numPagesX[l], numPagesY[l], 4, reinterpret_cast<uint8_t *>(pixels), true, &result))
		{
			vtLogComment(result);
			levelsWritten++;
		}
	}

	return levelsWritten == numLevels;
}

// ======================================================
// PageIndirectionTableRgb565:
// ======================================================

PageIndirectionTableRgb565::PageIndirectionTableRgb565(const int * vtPagesX, const int * vtPagesY, const int vtNumLevels)
	: PageIndirectionTable(vtPagesX, vtPagesY, vtNumLevels)
	, log2VirtPagesWide(static_cast<int>(std::log2(vtPagesX[0])))
	, totalTableEntries(0)
{
	clearArray(tableLevels);
	initTexture();

	vtLogComment("New PageIndirectionTable RGB-5:6:5 instance created. log2VirtPagesWide = " << log2VirtPagesWide);
}

void PageIndirectionTableRgb565::initTexture()
{
	assert(indirectionTextureId == 0 && "Duplicate initialization!");
	vtLogComment("Initializing page indirection texture with " << numLevels << " levels...");

	// Count total texture size, including all mip-levels:
	totalTableEntries = 0;
	for (int l = 0; l < numLevels; ++l)
	{
		totalTableEntries += numPagesX[l] * numPagesY[l];
	}

	tableEntryPool.reset(new TableEntry[totalTableEntries]);

	// Set up the pointers:
	totalTableEntries = 0;
	for (int l = 0; l < numLevels; ++l)
	{
		tableLevels[l] = tableEntryPool.get() + totalTableEntries;
		totalTableEntries += numPagesX[l] * numPagesY[l]; // Move to the next level
	}

	glGenTextures(1, &indirectionTextureId);
	if (indirectionTextureId == 0)
	{
		vtFatalError("Failed to generate a non-zero GL texture id for the page indirection texture!");
	}

	gl::use2DTexture(indirectionTextureId);

	// Create/set all the mip-levels:
	for (int l = 0; l < numLevels; ++l)
	{
		assert(tableLevels[l] != nullptr);

		// Default initialize the entries:
		for (int e = 0; e < numPagesX[l] * numPagesY[l]; ++e)
		{
			TableEntry & entry = tableLevels[l][e];

			entry = 0;
			entry = ((log2VirtPagesWide - l) << 5);
		}

		glTexImage2D(GL_TEXTURE_2D, l, GL_RGB, numPagesX[l], numPagesY[l], 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, tableLevels[l]);
		vtLogComment("Allocated indirection tex level #" << l << ". Size: " << numPagesX[l] << "x" << numPagesY[l] << " pixels.");

		#if VT_EXTRA_GL_ERROR_CHECKING
		gl::checkGLErrors(__FILE__, __LINE__);
		#endif // VT_EXTRA_GL_ERROR_CHECKING
	}

	// iOS specific: Set max level (would probably have to use glGenerateMipmap() otherwise...)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_APPLE, numLevels - 1);

	// Set addressing mode:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, indirectionTexAddressing);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, indirectionTexAddressing);

	// Set filtering:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, indirectionTexMinFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, indirectionTexMagFilter);

	gl::use2DTexture(0);
	gl::checkGLErrors(__FILE__, __LINE__);

	vtLogComment("Page indirection texture #" << indirectionTextureId << " created. Num entries: "
			<< totalTableEntries << ". " << numLevels << " levels.");
}

void PageIndirectionTableRgb565::updateIndirectionTexture(const CacheEntry * __restrict const pages)
{
	assert(pages != nullptr);

	// Texture must be already bound!
	// GL_UNPACK_ALIGNMENT should ideally be set to 4.
	assert(indirectionTextureId == gl::getCurrent2DTexture());

	// Assemble the indirection table, one mip-level at a time,
	// stating from the lowest resolution one:
	for (int l = (numLevels - 1); l >= 0; --l)
	{
		// Write all pages in a level:
		for (int p = 0; p < PageCacheMgr::TotalCachePages; ++p)
		{
			const CacheEntry & cacheEntry = pages[p];
			if ((pageIdExtractMipLevel(cacheEntry.pageId) != l) || (cacheEntry.pageId == InvalidPageId))
			{
				continue;
			}

			const int x = pageIdExtractPageX(cacheEntry.pageId);
			const int y = pageIdExtractPageY(cacheEntry.pageId);
			const int index = (x + y * numPagesX[l]);

			assert(index >= 0);
			assert(index < (numPagesX[l] * numPagesY[l]));
			TableEntry & entry = tableLevels[l][index];

			entry = ((cacheEntry.cacheCoord.x * 32 / PageTable::TableSizeInPages) << 11) |
				((log2VirtPagesWide - l) << 5) | (cacheEntry.cacheCoord.y * 32 / PageTable::TableSizeInPages);
		}

		// Upsample for next level:
		if (l != 0)
		{
			TableEntry * __restrict src  = tableLevels[l];
			TableEntry * __restrict dest = tableLevels[l - 1];

			const int srcW  = numPagesX[l];
			const int destW = numPagesX[l - 1];
			const int destH = numPagesY[l - 1];

			for (int y = 0; y < destH; ++y)
			{
				for (int x = 0; x < destW; ++x)
				{
					dest[x + y * destW] = src[(x >> 1) + (y >> 1) * srcW];
				}
			}
		}
	}

	for (int l = 0; l < numLevels; ++l)
	{
		glTexImage2D(GL_TEXTURE_2D, l, GL_RGB, numPagesX[l], numPagesY[l], 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, tableLevels[l]);

		#if VT_EXTRA_GL_ERROR_CHECKING
		gl::checkGLErrors(__FILE__, __LINE__);
		#endif // VT_EXTRA_GL_ERROR_CHECKING
	}
}

bool PageIndirectionTableRgb565::writeIndirectionTextureToFile(const std::string & pathname, const bool recolor) const
{
	int levelsWritten = 0;
	std::string levelNameStr, result;
	std::vector<Pixel4b> tempImage(numPagesX[0] * numPagesY[0]);

	// Convert the 5:6:5 texture to 8bits RGBA first, to make writing the image simpler.
	auto makeRGBA = [&tempImage, recolor](const TableEntry * data, size_t numPixels) -> Pixel4b *
	{
		for (size_t p = 0; p < numPixels; ++p)
		{
			TableEntry src  = data[p];
			Pixel4b &  dest = tempImage[p];

			// Unpack RGB 565 to RGBA fixing alpha to 255:
			dest.r = ((src & 0x7800) >> 11);
			dest.g = ((src & 0x07E0) >>  5);
			dest.b = ((src & 0x001F) >>  0);
			dest.a = 0xFF;

			// Reverse the bits in the image pixels to make them stand out.
			// Most of the pixels would be very dark otherwise.
			if (recolor)
			{
				dest.r = reverseByte(dest.r);
				dest.g = reverseByte(dest.g);
				dest.b = reverseByte(dest.b);
			}
		}
		return tempImage.data();
	};

	for (int l = 0; l < numLevels; ++l)
	{
		const size_t numPixels = numPagesX[l] * numPagesY[l];
		const Pixel4b * pixels = makeRGBA(tableLevels[l], numPixels);

		levelNameStr = pathname + "_" + std::to_string(l) + ".tga";
		if (tool::writeTgaImage(levelNameStr, numPagesX[l], numPagesY[l], 4, reinterpret_cast<const uint8_t *>(pixels), true, &result))
		{
			vtLogComment(result);
			levelsWritten++;
		}
	}

	return levelsWritten == numLevels;
}

// ======================================================
// createIndirectionTable():
// ======================================================

PageIndirectionTablePtr createIndirectionTable(const int * vtPagesX, const int * vtPagesY, const int vtNumLevels)
{
	extern IndirectionTableFormat getIndirectionTableFormat() noexcept;

	switch (getIndirectionTableFormat())
	{
	case IndirectionTableFormat::Rgb565 :
		return std::make_shared<PageIndirectionTableRgb565>(vtPagesX, vtPagesY, vtNumLevels);

	case IndirectionTableFormat::Rgba8888 :
		return std::make_shared<PageIndirectionTableRgba8888>(vtPagesX, vtPagesY, vtNumLevels);

	default :
		vtFatalError("Invalid IndirectionTableFormat!");
	} // switch (getIndirectionTableFormat())
}

} // namespace vt {}
