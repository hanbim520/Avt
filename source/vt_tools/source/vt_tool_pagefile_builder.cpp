
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_pagefile_builder.cpp
// Author: Guilherme R. Lampert
// Created on: 24/10/14
// Brief: Helper classes used for VT pagefile construction.
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

// Local dependencies:
#include "vt.hpp"
#include "vt_tool_pagefile_builder.hpp"
#include "vt_tool_platform_utils.hpp"
#include "vt_tool_mipmapper.hpp"
#include "vt_tool_image.hpp"
#include "vt_file_format.hpp"

// Standard library:
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <cerrno>

namespace vt
{
namespace tool
{

// ======================================================
// PageFileBuilderOptions:
// ======================================================

void PageFileBuilderOptions::printSelf() const
{
	const char * boolStr[2] = { "false", "true" };
	std::printf("---- PageFileBuilderOptions ----\n");
	std::printf("textureFilter..........: %d\n", int(textureFilter));
	std::printf("pageSizePixels.........: %d\n", pageSizePixels);
	std::printf("pageContentSizePixels..: %d\n", pageContentSizePixels);
	std::printf("pageBorderSizePixels...: %d\n", pageBorderSizePixels);
	std::printf("maxMipLevels...........: %d\n", maxMipLevels);
	std::printf("flipSourceVertically...: %s\n", boolStr[int(flipSourceVertically)]);
	std::printf("flipTilesVertically....: %s\n", boolStr[int(flipTilesVertically)]);
	std::printf("stopOn1PageMip.........: %s\n", boolStr[int(stopOn1PageMip)]);
	std::printf("addDebugInfoToPages....: %s\n", boolStr[int(addDebugInfoToPages)]);
	std::printf("dumpPageImages.........: %s\n", boolStr[int(dumpPageImages)]);
	std::printf("stdoutVerbose..........: %s\n", boolStr[int(stdoutVerbose)]);
}

// ======================================================
// PageFileBuilder::MipMapLevel:
// ======================================================

FloatImageBuffer & PageFileBuilder::MipMapLevel::getTileAt(const uint32_t x, const uint32_t y)
{
	assert(x < tilesX);
	assert(y < tilesY);
	return tiles[x + y * tilesX];
}

const FloatImageBuffer & PageFileBuilder::MipMapLevel::getTileAt(const uint32_t x, const uint32_t y) const
{
	assert(x < tilesX);
	assert(y < tilesY);
	return tiles[x + y * tilesX];
}

bool PageFileBuilder::MipMapLevel::isAllocated() const
{
	return tiles != nullptr;
}

void PageFileBuilder::MipMapLevel::allocate(const uint32_t tx, const uint32_t ty, const uint32_t numComponents, const uint32_t pageSizePixels)
{
	assert(!isAllocated() && "Already allocated! free() first.");
	assert(tx != 0 && ty != 0 && "Number of tiles was invalid! Must not be zero!");

	tilesX = tx;
	tilesY = ty;
	tiles.reset(new FloatImageBuffer[tilesX * tilesY]);

	for (uint32_t i = 0; i < (tilesX * tilesY); ++i)
	{
		tiles[i].allocImageStorage(numComponents, pageSizePixels, pageSizePixels);
	}
}

void PageFileBuilder::MipMapLevel::free()
{
	tilesX = 0;
	tilesY = 0;
	tiles  = nullptr;
}

// ======================================================
// Local helpers:
// ======================================================

namespace {

inline std::string removeExtension(const std::string & filename)
{
	const size_t lastDot = filename.find_last_of(".");
	if (lastDot == std::string::npos)
	{
		return filename;
	}
	return filename.substr(0, lastDot);
}

inline int buildMipSizeList(int size, int mipLevels[], const int tileSize)
{
	int levelNum = 0;
	while ((size > 0) && (size >= tileSize))
	{
		mipLevels[levelNum++] = size;
		size /= 2;
	}
	return levelNum;
}

inline bool mipsAreDivisible(const int mipLevels[], const int numLevels, const int tileSize)
{
	for (int i = 0; i < numLevels; ++i)
	{
		if ((mipLevels[i] % tileSize) != 0)
		{
			return false;
		}
	}
	return true;
}

inline int adjustSize(const int texSize, const int tileSize)
{
	// +1 rounds up
	// -1 rounds down
	//
	// Example:
	//   +1 with 1024 => 1920
	//   -1 with 1024 => 960
	//
	constexpr int increment = 1;

	int mipLevels[32] = {0};
	int numLevels = 0, newSize = texSize;

	if ((newSize % tileSize) != 0)
	{
		while ((newSize % tileSize) != 0)
		{
			newSize += increment;
		}

		numLevels = buildMipSizeList(newSize, mipLevels, tileSize);
		while (!mipsAreDivisible(mipLevels, numLevels, tileSize))
		{
			newSize  += increment;
			numLevels = buildMipSizeList(newSize, mipLevels, tileSize);
		}

		/*
		std::printf("New size: %d\n", newSize);
		for (int i = 0; i < numLevels; ++i)
		{
			std::printf("Level %d = %d ( %d tiles )\n", i, mipLevels[i], (mipLevels[i] / tileSize));
		}
		*/
	}

	return newSize;
}

} // namespace {}

// ======================================================
// PageFileBuilder:
// ======================================================

PageFileBuilder::PageFileBuilder(std::string inputFile, std::string outputFile, PageFileBuilderOptions options)
	: inputFileName(std::move(inputFile))
	, outputFileName(std::move(outputFile))
	, opts(std::move(options))
	, sourcePixelFormat(PixelFormat::RgbaU8)
{
	// Basic input validation:
	if (inputFileName.empty())
	{
		error("No input filename provided!");
	}
	if (outputFileName.empty())
	{
		error("No output filename provided!");
	}
	if (opts.pageSizePixels <= 0)
	{
		error("Invalid page size!");
	}
	if (opts.pageContentSizePixels <= 0)
	{
		error("Invalid page content size!");
	}
	if (opts.maxMipLevels <= 0)
	{
		error("Invalid number of mip-levels!");
	}

	pageFileLevels.resize(opts.maxMipLevels);
}

void PageFileBuilder::generatePageFile()
{
	// Steps:
	// - Open file & load image;
	// - Generate mipmap chain;
	// - Break each mipmap level into tiles, adding borders;
	// - Write output file(s).

	if (opts.stdoutVerbose)
	{
		std::printf("Beginning page file processing... Loading image: %s\n", inputFileName.c_str());
	}

	Image srcImage;
	std::string imageLoadError;

	// NOTE: Currently, the whole VT system is supporting only 8bit RGBA textures,
	// so we force the source file to RGBA as well to ensure the page file is generated
	// using the same format as the rest of the system. This should be changed in the
	// future to allow more varied texture formats.
	//
	if (!srcImage.loadFromFile(inputFileName, &imageLoadError, /* forceRGBA = */ true))
	{
		error("Can't load input image file! " + imageLoadError);
	}

	if (!FloatImageBuffer::isImageFormatCompatible(srcImage.getFormat()))
	{
		error(std::string(PixelFormat::toString(srcImage.getFormat())) + " => image format currently not supported!");
	}

	sourcePixelFormat = srcImage.getFormat();

	// Turn into a float image and dispose the old Image object
	// to reduce pressure on the system memory:
	FloatImageBuffer floatImage(srcImage);
	srcImage.freeImageStorage();

	// Filter used for the mipmap downsampling and eventual upsampling:
	std::unique_ptr<Filter> textureFilter = Filter::createFilter(opts.textureFilter);

	// If the source image dimensions are not evenly divisible by the
	// page content size, we need to upsample it to an adequate size.
	if (((floatImage.getWidth()  % opts.pageContentSizePixels) != 0) ||
	    ((floatImage.getHeight() % opts.pageContentSizePixels) != 0))
	{
		int newWidth  = static_cast<int>(floatImage.getWidth());
		int newHeight = static_cast<int>(floatImage.getHeight());

		newWidth  = adjustSize(newWidth,  opts.pageContentSizePixels);
		newHeight = adjustSize(newHeight, opts.pageContentSizePixels);

		if (opts.stdoutVerbose)
		{
			std::printf("Upsampling source image to size evenly divisible by %d: (%u, %u)...\n",
					opts.pageContentSizePixels, newWidth, newHeight);
		}

		FloatImageBuffer upsampledImage;
		floatImage.resize(upsampledImage, *textureFilter, newWidth, newHeight, FloatImageBuffer::Clamp);
		floatImage = std::move(upsampledImage);
	}

	// Generate mip-chain.
	// The only error that can happen here is an out-of-memory situation,
	// in which case the app will terminate with a core dump and hopefully a crash reporter popup.
	MipMapper mipMapper(std::move(floatImage));
	mipMapper.buildMipMapChain(*textureFilter, FloatImageBuffer::Clamp);

	unsigned int numMipMapLevels = static_cast<unsigned int>(mipMapper.getNumMipMapLevels());
	if (numMipMapLevels > static_cast<unsigned int>(opts.maxMipLevels))
	{
		numMipMapLevels = static_cast<unsigned int>(opts.maxMipLevels);
	}

	for (unsigned int l = 0; l < numMipMapLevels; ++l)
	{
		const FloatImageBuffer * source = mipMapper.getMipMapLevel(l);

		if (source == nullptr)
		{
			error("Null image for mip-level " + std::to_string(l));
		}

		if (opts.stopOn1PageMip)
		{
			if ((source->getWidth()  < static_cast<unsigned int>(opts.pageContentSizePixels)) ||
			    (source->getHeight() < static_cast<unsigned int>(opts.pageContentSizePixels)))
			{
				break;
			}
		}

		processImage(*source, l);
	}

	writePageFile();
}

void PageFileBuilder::error(const std::string & errorMessage) const
{
	throw PageFileBuilderError("PageFileBuilder error (" + inputFileName + "): " + errorMessage);
}

void PageFileBuilder::processImage(const FloatImageBuffer & source, const unsigned int level)
{
	const uint32_t w = source.getWidth();
	const uint32_t h = source.getHeight();
	const uint32_t contentSize = opts.pageContentSizePixels;
	const uint32_t tilesX = static_cast<uint32_t>(std::ceil(static_cast<float>(w) / contentSize));
	const uint32_t tilesY = static_cast<uint32_t>(std::ceil(static_cast<float>(h) / contentSize));

	if (opts.stdoutVerbose)
	{
		std::printf("Processing level %u (tilesX:%u, tilesY:%u), (w:%u, h:%u)\n",
				level, tilesX, tilesY, w, h);
	}

	// Allocate a mipmap level:
	MipMapLevel & vtLevel = pageFileLevels[level];
	vtLevel.allocate(tilesX, tilesY, source.getNumComponents(), opts.pageSizePixels);

	for (uint32_t y = 0; y < tilesY; ++y)
	{
		for (uint32_t x = 0; x < tilesX; ++x)
		{
			FloatImageBuffer & dest = vtLevel.getTileAt(x, y);

			source.copyRect(dest,
				(x * opts.pageContentSizePixels - opts.pageBorderSizePixels),
				(y * opts.pageContentSizePixels - opts.pageBorderSizePixels),
				0, 0,
				(opts.pageSizePixels + opts.pageBorderSizePixels),
				(opts.pageSizePixels + opts.pageBorderSizePixels),
				opts.flipSourceVertically,
				FloatImageBuffer::Clamp);

			if (opts.flipTilesVertically)
			{
				dest.flipVInPlace();
			}
		}
	}
}

void PageFileBuilder::writePageFile() const
{
	writeVTFF();

	// Optionally write each page to an image file.
	// This will attempt to create a directory with outputFileName + "img_dump/"
	// and for each mip-level a "level_x/" dir with the pages for that level in the directory.
	if (opts.dumpPageImages)
	{
		const std::string baseDir = removeExtension(outputFileName);

		if (opts.stdoutVerbose)
		{
			std::printf("Dumping each VT page as a TGA image...\n");
		}

		Image rgbaImage;
		std::string resultMessage;
		char dirname[512];
		char filename[1024];

		for (size_t l = 0; l < pageFileLevels.size(); ++l)
		{
			const MipMapLevel & vtLevel = pageFileLevels[l];
			if (!vtLevel.isAllocated())
			{
				continue;
			}

			// Create a sub-directory for this level, ended with the level number:
			std::snprintf(dirname, sizeof(dirname), "img_dump/level_%u", static_cast<unsigned int>(l));
			createDirectory(baseDir.c_str(), dirname);

			// Write every page for every level:
			for (uint32_t y = 0; y < vtLevel.tilesY; ++y)
			{
				for (uint32_t x = 0; x < vtLevel.tilesX; ++x)
				{
					const FloatImageBuffer & rawImage = vtLevel.getTileAt(x, y);
					rawImage.toImageRgbaU8(rgbaImage);

					if (opts.addDebugInfoToPages)
					{
						addDebugInfoToPageData(x, y, static_cast<int>(l), rgbaImage.getDataPtr<uint8_t>(), rgbaImage.getNumComponents(),
							true, opts.flipTilesVertically, opts.pageSizePixels, opts.pageBorderSizePixels);
					}

					// Number each file with the page x-y coord.
					std::snprintf(filename, sizeof(filename), "%s/%s/page_%u_%u.tga", baseDir.c_str(), dirname, x, y);

					if (!writeTgaImage(filename, rgbaImage.getWidth(), rgbaImage.getHeight(),
						rgbaImage.getNumComponents(), rgbaImage.getDataPtr<uint8_t>(), true, &resultMessage))
					{
						if (!resultMessage.empty())
						{
							error("Failed to write vt level: " + resultMessage);
						}
						else
						{
							error("Failed to write vt level: " + std::to_string(l));
						}
					}
					else
					{
						if (opts.stdoutVerbose && !resultMessage.empty())
						{
							std::printf("%s\n", resultMessage.c_str());
						}
					}
				}
			}
		}
	}
}

void PageFileBuilder::writeVTFF() const
{
	std::ofstream file;

	errno = 0;
	file.exceptions(0); // Don't throw and exception if we have an error.
	file.open(outputFileName, std::ofstream::out | std::ofstream::binary);

	if (!file.is_open())
	{
		error("Failed to create VTFF output file! Reason: " + std::string(std::strerror(errno)));
	}
	if (!file.good())
	{
		error("File stream is in a bad state for IO!");
	}

	// The vector has all the 16 possible levels,
	// but only the first N are allocated if in use.
	uint32_t numLevels = 0;
	for (size_t l = 0; l < pageFileLevels.size(); ++l)
	{
		const MipMapLevel & vtLevel = pageFileLevels[l];
		if (!vtLevel.isAllocated())
		{
			continue;
		}
		++numLevels;
	}

	if (opts.stdoutVerbose)
	{
		std::printf("Writing VTFF output file...\n");
		std::printf("File has %u mipmap levels.\n", numLevels);
		std::printf("Page size: %dpx\n", opts.pageSizePixels);
	}

	VTFF::Header header;
	header.magic           = VTFF::Magic;
	header.version         = VTFF::Version;
	header.pixelFormat     = sourcePixelFormat;
	header.numMipMapLevels = numLevels;
	header.pageContentSize = opts.pageContentSizePixels;
	header.pageSize        = opts.pageSizePixels;
	header.borderSize      = opts.pageBorderSizePixels;

	uint64_t pageDataStart = 0;
	uint64_t pagesSoFar    = 0;
	const uint32_t pageSizeBytes = header.pageSize * header.pageSize * 4; // Fixed to RGBA for now!

	// Write the file header:
	file.write(reinterpret_cast<const char *>(&header), sizeof(header));
	pageDataStart += sizeof(VTFF::Header);

	// First thing we have to do is count the total offset of the headers:
	for (uint32_t l = 0; l < numLevels; ++l)
	{
		const MipMapLevel & vtLevel = pageFileLevels[l];
		assert(vtLevel.isAllocated());

		// Add the size of a mipmap level header and all of its page entries:
		pageDataStart += sizeof(VTFF::MipLevelInfo);
		pageDataStart += sizeof(VTFF::PageInfo) * (vtLevel.tilesX * vtLevel.tilesY);
	}

	if (opts.stdoutVerbose)
	{
		std::printf("VTFF headers use the first %llu bytes of the file.\n", pageDataStart);
	}

	// Now write each mipmap level header:
	for (uint32_t l = 0; l < numLevels; ++l)
	{
		const MipMapLevel & vtLevel = pageFileLevels[l];
		assert(vtLevel.isAllocated());

		VTFF::MipLevelInfo levelInfo;
		levelInfo.width     = vtLevel.tilesX * header.pageSize;
		levelInfo.height    = vtLevel.tilesY * header.pageSize;
		levelInfo.numPagesX = static_cast<uint16_t>(vtLevel.tilesX);
		levelInfo.numPagesY = static_cast<uint16_t>(vtLevel.tilesY);
		file.write(reinterpret_cast<const char *>(&levelInfo), sizeof(levelInfo));

		// Write the individual page headers:
		for (uint16_t y = 0; y < levelInfo.numPagesY; ++y)
		{
			for (uint16_t x = 0; x < levelInfo.numPagesX; ++x)
			{
				VTFF::PageInfo pageInfo;
				pageInfo.sizeInBytes = pageSizeBytes;
				pageInfo.fileOffset  = pageDataStart + (pagesSoFar * pageSizeBytes);
				file.write(reinterpret_cast<const char *>(&pageInfo), sizeof(pageInfo));
				++pagesSoFar;
			}
		}
	}

	// Now the actual page pixels are written:
	for (uint32_t l = 0; l < numLevels; ++l)
	{
		const MipMapLevel & vtLevel = pageFileLevels[l];
		assert(vtLevel.isAllocated());

		// Write the individual pages:
		Image rgbaImage;
		for (uint32_t y = 0; y < vtLevel.tilesY; ++y)
		{
			for (uint32_t x = 0; x < vtLevel.tilesX; ++x)
			{
				const FloatImageBuffer & rawImage = vtLevel.getTileAt(x, y);
				rawImage.toImageRgbaU8(rgbaImage); // Fixed to RGBA!

				file.write(rgbaImage.getDataPtr<char>(), rgbaImage.getDataSizeBytes());
			}
		}
	}

	if (opts.stdoutVerbose)
	{
		std::printf("Finished writing VTFF output.\n");
	}
}

} // namespace tool {}
} // namespace vt {}
