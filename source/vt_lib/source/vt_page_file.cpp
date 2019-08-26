
// ================================================================================================
// -*- C++ -*-
// File: vt_page_file.cpp
// Author: Guilherme R. Lampert
// Created on: 23/10/14
// Brief: Page data backing store, usually implemented as a file on disk.
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
#include "vt_file_format.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace vt
{

// ======================================================
// UnpackedImagesPageFile:
// ======================================================

void UnpackedImagesPageFile::loadPage(const PageId pageId, PageRequestDataPacket & pageRequest)
{
	char path[512];
	const int x = pageIdExtractPageX(pageId);
	const int y = pageIdExtractPageY(pageId);
	const int level = pageIdExtractMipLevel(pageId);
	std::snprintf(path, sizeof(path), "%s/level_%d/page_%d_%d.tga", baseDir.c_str(), level, x, y);

	std::string imageLoadError;
	tool::Image pageImage;
	if (!pageImage.loadFromFile(path, &imageLoadError, /* forceRGBA = */ true))
	{
		vtLogError("UnpackedImagesPageFile: Failed to load a page! " << imageLoadError);
		std::memset(pageRequest.pageData, 0, sizeof(pageRequest.pageData));
		return;
	}

	size_t pageBytes = sizeof(pageRequest.pageData);
	if (pageImage.getDataSizeBytes() < pageBytes)
	{
		vtLogWarning("UnpackedImagesPageFile: Page image too small! (" << pageImage.getDataSizeBytes()
				<< ", " << tool::PixelFormat::toString(pageImage.getFormat()) << ")");
		pageBytes = pageImage.getDataSizeBytes();
	}

	std::memcpy(pageRequest.pageData, pageImage.getDataPtr<uint8_t>(), pageBytes);

	// Write page number and level to the tile:
	if (addDebugInfo)
	{
		tool::addDebugInfoToPageData(
			x, y, level,
			reinterpret_cast<uint8_t *>(pageRequest.pageData),
			/* colorComps     = */ 4, // RGBA
			/* drawPageBorder = */ true,
			/* flipText       = */ false,
			PageTable::PageSizeInPixels,
			PageTable::PageBorderSizeInPixels);
	}
}

// ======================================================
// DebugPageFile:
// ======================================================

void DebugPageFile::loadPage(const PageId pageId, PageRequestDataPacket & pageRequest)
{
	// This PageFile implementation is used for basic testing and debugging.
	// It does not perform any file IO, just writes the pageId to every
	// pixel of the page and adds some debug text on top of it.

	const int x = pageIdExtractPageX(pageId);
	const int y = pageIdExtractPageY(pageId);
	const int level = pageIdExtractMipLevel(pageId);

	const Pixel4b color = {
		reverseByte(x), reverseByte(y),
		reverseByte(clampByte(level + pageIdExtractTextureIndex(pageId))),
		0xFF
	};

	for (int p = 0; p < PageRequestDataPacket::TotalPagePixels; ++p)
	{
		pageRequest.pageData[p] = color;
	}

	// Write page number and level to the tile:
	if (addDebugInfo)
	{
		tool::addDebugInfoToPageData(
			x, y, level,
			reinterpret_cast<uint8_t *>(pageRequest.pageData),
			/* colorComps     = */ 4, // RGBA
			/* drawPageBorder = */ true,
			/* flipText       = */ false,
			PageTable::PageSizeInPixels,
			PageTable::PageBorderSizeInPixels);
	}
}

// ======================================================
// VTFFPageFile:
// ======================================================

VTFFPageFile::VTFFPageFile(std::string filename, const bool debug)
	: VTFFPageFile(tryOpenFile(filename), std::move(filename), debug)
{
	// tryOpenFile() may throw if fopen() fails.
}

VTFFPageFile::VTFFPageFile(FILE * fileStream, std::string filename, const bool debug)
	: pageFile(fileStream)
	, inputFileName(std::move(filename))
	, addDebugInfo(debug)
{
	assert(pageFile != nullptr);
	assert(!std::ferror(pageFile));

	// Read file header and validate it:
	VTFF::Header header;
	if (std::fread(&header, sizeof(header), 1, pageFile) != 1)
	{
		vtFatalError("VTFF \"" << inputFileName << "\": Unable to read file header!");
	}

	if ((header.magic != VTFF::Magic) || (header.version != VTFF::Version))
	{
		vtFatalError("VTFF \"" << inputFileName <<  "\": Wrong file type / bad file version!");
	}

	if ((header.numMipMapLevels == 0) ||
		(header.numMipMapLevels > MaxVTMipLevels) ||
		(header.pageSize != PageTable::PageSizeInPixels) ||
		(header.borderSize != PageTable::PageBorderSizeInPixels) ||
		(header.pageContentSize != PageTable::PageSizeInPixels - (PageTable::PageBorderSizeInPixels * 2)))
	{
		vtFatalError("VTFF \"" << inputFileName << "\": Bad file format! Data layout is incompatible.");
	}

	if (header.pixelFormat != tool::PixelFormat::RgbaU8)
	{
		vtFatalError("VTFF \"" << inputFileName << "\": Currently, we only support 8bits RGBA page files!");
	}

	vtLogComment("VTFF file \"" << inputFileName << "\" has " << header.numMipMapLevels << " mipmap levels.");

	int vtPagesX[MaxVTMipLevels] = {0};
	int vtPagesY[MaxVTMipLevels] = {0};
	const unsigned int numLevels = header.numMipMapLevels;

	fpos_t oldFilePos;
	if (std::fgetpos(pageFile, &oldFilePos) != 0)
	{
		vtFatalError("fgetpos() failed!");
	}

	// Now read mip-map levels and validate the headers:
    for (unsigned int level = 0; level < numLevels; ++level)
	{
		VTFF::MipLevelInfo levelInfo;
		if (std::fread(&levelInfo, sizeof(levelInfo), 1, pageFile) != 1)
		{
			vtFatalError("VTFF \"" << inputFileName << "\": Unable to read mipmap information for level " << level);
		}

		// We expect a power-of-two number of pages in both axes!
		if (!isPowerOfTwo(levelInfo.numPagesX))
		{
			vtFatalError("VTFF \"" << inputFileName << "\": Mipmap level " << level
					<< ": numPagesX (" << levelInfo.numPagesX << ") is not a power-of-2!");
		}
		if (!isPowerOfTwo(levelInfo.numPagesY))
		{
			vtFatalError("VTFF \"" << inputFileName << "\": Mipmap level " << level
					<< ": numPagesY (" << levelInfo.numPagesY << ") is not a power-of-2!");
		}

		// Width/height must also be evenly divisible by the page size!
		if ((levelInfo.width % header.pageSize) != 0 || (levelInfo.height % header.pageSize) != 0)
		{
			vtFatalError("VTFF \"" << inputFileName << "\": Bad mipmap level layout for level " << level);
		}

		// Read information for all pages belonging this level and validate 'em:
		for (uint16_t y = 0; y < levelInfo.numPagesY; ++y)
		{
			for (uint16_t x = 0; x < levelInfo.numPagesX; ++x)
			{
				VTFF::PageInfo pageInfo;
				if (std::fread(&pageInfo, sizeof(pageInfo), 1, pageFile) != 1)
				{
					vtFatalError("VTFF \"" << inputFileName << "\": Unable to read page("
							<< x << ", " << y << ") info for mipmap level " << level);
				}

				if (pageInfo.sizeInBytes != (PageTable::PageSizeInPixels * PageTable::PageSizeInPixels * 4))
				{
					vtFatalError("VTFF \"" << inputFileName << "\": Bad page size in bytes! We currently only support RgbaU8 format!");
				}

				// Don't save this PageInfo now. There will be a second pass to
				// collect these after we finish gathering the mipmap level sizes.
			}
		}

		vtPagesX[level] = levelInfo.numPagesX;
		vtPagesY[level] = levelInfo.numPagesY;
    }

	// Restore the file position to just before the main file header:
	if (std::fsetpos(pageFile, &oldFilePos) != 0)
	{
		vtFatalError("fsetpos() failed!");
	}

	// Allocate the page quad-tree:
	pageTree.reset(new VTFFPageTree(vtPagesX, vtPagesY, numLevels));

	// Now we are going to read the MipLevelInfo and PageInfo
	// headers again to construct the VTFFPageTree.
	for (uint32_t level = 0; level < numLevels; ++level)
	{
		VTFF::MipLevelInfo levelInfo;
		if (std::fread(&levelInfo, sizeof(levelInfo), 1, pageFile) != 1)
		{
			vtFatalError("VTFF \"" << inputFileName << "\": Unable to read mipmap information for level " << level);
		}

		for (uint16_t y = 0; y < levelInfo.numPagesY; ++y)
		{
			for (uint16_t x = 0; x < levelInfo.numPagesX; ++x)
			{
				VTFF::PageInfo pageInfo;
				if (std::fread(&pageInfo, sizeof(pageInfo), 1, pageFile) != 1)
				{
					vtFatalError("VTFF \"" << inputFileName << "\": Unable to read page("
							<< x << ", " << y << ") info for mipmap level " << level);
				}

				pageTree->set(x, y, level, pageInfo.fileOffset, pageInfo.sizeInBytes);
			}
		}
	}
}

VTFFPageFile::~VTFFPageFile()
{
	if (pageFile != nullptr)
	{
		std::fclose(pageFile);
	}
}

bool VTFFPageFile::isPowerOfTwo(const uint32_t size)
{
	return (size & (size - 1)) == 0;
}

FILE * VTFFPageFile::tryOpenFile(const std::string & filename)
{
	errno = 0;
	FILE * pageFile = std::fopen(filename.c_str(), "rb");
	if (pageFile == nullptr)
	{
		vtFatalError("Failed to open VTFF page file \"" << filename << "\"! Sys err: " << std::strerror(errno));
	}
	return pageFile;
}

void VTFFPageFile::loadPage(const PageId pageId, PageRequestDataPacket & pageRequest)
{
	assert(pageFile != nullptr);
	assert(pageTree != nullptr);

	#if VT_THREAD_SAFE_VTFF_PAGE_FILE
	std::lock_guard<std::mutex> lock(fileLock);
	#endif // VT_THREAD_SAFE_VTFF_PAGE_FILE

	if (pageId == InvalidPageId)
	{
		vtLogError("VTFFPageFile: Invalid page id!");
		std::memset(pageRequest.pageData, 0, sizeof(pageRequest.pageData));
		return;
	}

	const VTFFPageTree::PageInfo & pageInfo = pageTree->get(pageId);

	if (std::fseek(pageFile, pageInfo.fileOffset, SEEK_SET) != 0)
	{
		vtLogError("VTFFPageFile: Failed to seek file offset " << pageInfo.fileOffset << "! fseek() failed!");
		std::memset(pageRequest.pageData, 0, sizeof(pageRequest.pageData));
		return;
	}

	if (std::fread(pageRequest.pageData, sizeof(pageRequest.pageData), 1, pageFile) != 1)
	{
		vtLogWarning("VTFFPageFile: fread() failed to read " << sizeof(pageRequest.pageData)
				<< " bytes of page file \"" << inputFileName << "\"!");
		std::memset(pageRequest.pageData, 0, sizeof(pageRequest.pageData));
		return;
	}

	if (addDebugInfo)
	{
		tool::addDebugInfoToPageData(
			pageIdExtractPageX(pageId),
			pageIdExtractPageY(pageId),
			pageIdExtractMipLevel(pageId),
			reinterpret_cast<uint8_t *>(pageRequest.pageData),
			/* colorComps     = */ 4, // RGBA
			/* drawPageBorder = */ true,
			/* flipText       = */ false,
			PageTable::PageSizeInPixels,
			PageTable::PageBorderSizeInPixels);
	}
}

} // namespace vt {}
