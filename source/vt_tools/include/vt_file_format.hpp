
// ================================================================================================
// -*- C++ -*-
// File: vt_file_format.hpp
// Author: Guilherme R. Lampert
// Created on: 04/11/14
// Brief: Virtual Texture File Format ('.vt' extension).
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

#ifndef VT_TOOL_FILE_FORMAT_HPP
#define VT_TOOL_FILE_FORMAT_HPP

#include <cassert>
#include <vector>
#include <array>

namespace vt
{

// ======================================================
// VTFF:
// ======================================================

#pragma pack(push, 1)
struct VTFF
{
	// VT magic and version number:
	static constexpr uint32_t Magic   = 'VTFF';
	static constexpr uint32_t Version = 4;

	struct Header
	{
		uint32_t magic;           // First 4 bytes of file = 'VTFF'
		uint32_t version;         // File version number.
		uint32_t pixelFormat;     // Data format. One of the PixelFormat enum. Applies for every page.
		uint32_t numMipMapLevels; // Total mipmap count for this file.
		uint32_t pageContentSize; // Page size (width & height) without border, in pixels.
		uint32_t pageSize;        // Page size (width & height) with border, in pixels.
		uint32_t borderSize;      // Size of page border, in pixels.

		// Followed by MipLevelInfo[numMipMapLevels] instances
	};

	struct MipLevelInfo
	{
		uint32_t width;     // Width in pixels of mipmap level (sum of all pages, includes border).
		uint32_t height;    // Height in pixels of mipmap level (sum of all pages, includes border).
		uint16_t numPagesX; // Number of pages in the x-axis.
		uint16_t numPagesY; // Number of pages in the y-axis.

		// Followed by PageInfo[numPagesX * numPagesY] instances
	};

	struct PageInfo
	{
		// Offset from the beginning of the file where this page's data starts.
		uint64_t fileOffset;

		// Size in bytes of this page. This is useful if the
		// compression algorithm generates varying sized pages.
		uint32_t sizeInBytes;
	};
};
#pragma pack(pop)

//
// And after the headers just a huge hunk of pages.
// So for example, a VT file with 2 mipmap levels and
// 4 (2x2) pages per mipmap level, would look like:
//
// -------------------------------
// Header
// -------------------------------
// 1st MipLevelInfo
// -------------------------------
// 1st mip-level, 1st PageInfo
// 1st mip-level, 2nd PageInfo
// 1st mip-level, 3rd PageInfo
// 1st mip-level, 4th PageInfo
// -------------------------------
// 2nd MipLevelInfo
// -------------------------------
// 2nd mip-level, 1st PageInfo
// 2nd mip-level, 2nd PageInfo
// 2nd mip-level, 3rd PageInfo
// 2nd mip-level, 4th PageInfo
// ------------------------------- <== pageDataStart
// pixel data of 4 pages
// for 1st mip-level ...
// -------------------------------
// pixel data of 4 pages
// for 2nd mip-level ...
// -------------------------------
// EOF
//

// ======================================================
// VTFFPageTree:
// ======================================================

// Quadtree-like page set. Each texture mipmap level is represented
// by and array of VTFF::PageInfo that allows us to index the Virtual Texture file.
class VTFFPageTree final
	: public NonCopyable
{
public:

	using PageInfo = VTFF::PageInfo;

	VTFFPageTree(const int * vtPagesX, const int * vtPagesY, const unsigned int numLevels)
	{
		assert(numLevels > 0 && numLevels <= MaxVTMipLevels);

		// Clear every slot first, as some might end up unused:
		clearArray(numPagesX);
		clearArray(numPagesY);
		for (unsigned int l = 0; l < numLevels; ++l)
		{
			assert(vtPagesX[l] > 0);
			assert(vtPagesY[l] > 0);
			numPagesX[l] = vtPagesX[l];
			numPagesY[l] = vtPagesY[l];
		}

		// Count total number of entries to allocate exact amount of memory:
		unsigned int totalEntries = 0;
		for (unsigned int l = 0; l < numLevels; ++l)
		{
			totalEntries += numPagesX[l] * numPagesY[l];
		}

		levels.resize(numLevels, nullptr);
		pageInfoPool.resize(totalEntries, {});

		// Set up the pointers:
		totalEntries = 0;
		for (unsigned int l = 0; l < numLevels; ++l)
		{
			levels[l] = pageInfoPool.data() + totalEntries;
			totalEntries += numPagesX[l] * numPagesY[l]; // Move to the next level
		}

		assert(totalEntries == pageInfoPool.size());
		assert(numLevels    == levels.size());
	}

	void set(const int x, const int y, const int level, const uint64_t fileOffset, const uint32_t sizeInBytes)
	{
		assert(level >= 0 && level < getNumLevels());
		PageInfo * levelPages = levels[level];
		const int pageIndex = x + y * numPagesX[level];

		assert(pageIndex < (numPagesX[level] * numPagesY[level]));
		levelPages[pageIndex].fileOffset  = fileOffset;
		levelPages[pageIndex].sizeInBytes = sizeInBytes;
	}

	PageInfo get(const PageId id) const
	{
		const int x = pageIdExtractPageX(id);
		const int y = pageIdExtractPageY(id);
		const int level = pageIdExtractMipLevel(id);

		assert(level >= 0 && level < getNumLevels());
		const PageInfo * levelPages = levels[level];
		const int pageIndex = x + y * numPagesX[level];

		assert(pageIndex < (numPagesX[level] * numPagesY[level]));
		return levelPages[pageIndex];
	}

	int getNumLevels() const
	{
		return static_cast<int>(levels.size());
	}

	int getNumPagesX(const int level) const
	{
		return numPagesX[level];
	}

	int getNumPagesY(const int level) const
	{
		return numPagesY[level];
	}

	const int * getNumPagesX() const
	{
		return numPagesX.data();
	}

	const int * getNumPagesY() const
	{
		return numPagesY.data();
	}

	void getLevelDimensions(int * vtPagesX, int * vtPagesY) const
	{
		for (size_t l = 0; l < levels.size(); ++l)
		{
			vtPagesX[l] = numPagesX[l];
			vtPagesY[l] = numPagesY[l];
		}
	}

	void clear()
	{
		std::memset(pageInfoPool.data(), 0, pageInfoPool.size() * sizeof(PageInfo));
	}

private:

	std::vector<PageInfo *> levels;
	std::vector<PageInfo>   pageInfoPool;

	// Per-level page counts. Unused ones set to zero.
	std::array<int, MaxVTMipLevels> numPagesX;
	std::array<int, MaxVTMipLevels> numPagesY;
};

} // namespace vt {}

#endif // VT_TOOL_FILE_FORMAT_HPP
