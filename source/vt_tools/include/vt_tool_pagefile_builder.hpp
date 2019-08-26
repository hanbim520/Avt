
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_pagefile_builder.hpp
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

#ifndef VT_TOOL_PAGEFILE_BUILDER_HPP
#define VT_TOOL_PAGEFILE_BUILDER_HPP

#include "vt_tool_filters.hpp"
#include "vt_tool_float_image_buffer.hpp"

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>

namespace vt
{
namespace tool
{

// ======================================================
// PageFileBuilderError:
// ======================================================

class PageFileBuilderError final
	: public std::runtime_error
{
public:
	PageFileBuilderError(const char * error)
		: std::runtime_error(error) { }

	PageFileBuilderError(const std::string & error)
		: std::runtime_error(error) { }
};

// ======================================================
// PageFileBuilderOptions:
// ======================================================

struct PageFileBuilderOptions
{
	// Filter used to donwsample the image for mipmap construction.
	FilterType textureFilter  = FilterType::Box;

	// Page size in pixels, with border.
	int pageSizePixels        = 128;

	// Size of a page in pixels without the border.
	int pageContentSizePixels = 120;

	// Size in pixels of just the page border.
	int pageBorderSizePixels  = 4;

	// Maximum number of mipmap levels to generate.
	int maxMipLevels          = 16;

	// Flip the entire source image.
	bool flipSourceVertically = false;

	// Flip each individual tile when copying from source.
	bool flipTilesVertically  = false;

	// Stop generating mipmaps when we reach a single page mip-level.
	bool stopOn1PageMip       = true;

	// Print debug text to the pages.
	bool addDebugInfoToPages  = false;

	// Also dumps each individual page to an image. Image format is implementation defined.
	bool dumpPageImages       = false;

	// Print a few stats about the pagefile generation process to STDOUT.
	bool stdoutVerbose        = true;

	// Prints this structure to STDOUT.
	void printSelf() const;
};

// ======================================================
// PageFileBuilder:
// ======================================================

class PageFileBuilder final
{
public:

	// Default constructor.
	// Collect parameters but doesn't run the pagefile building step yet.
	PageFileBuilder(std::string inputFile,
	                std::string outputFile,
	                PageFileBuilderOptions options);

	// No copy or assignment.
	PageFileBuilder(const PageFileBuilder &) = delete;
	PageFileBuilder & operator = (const PageFileBuilder &) = delete;

	// Performs the expensive mipmap generation and pagefile construction.
	// Throws an exception if any step of the process fails and it cannot recover.
	void generatePageFile();

private:

	// Throws a PageFileBuilderError.
	void error(const std::string & errorMessage) const;

	// Internal helpers.
	void processImage(const FloatImageBuffer & source, unsigned int level);
	void writePageFile() const;
	void writeVTFF() const;

	// Little helper class for a 2D array of tiles/pages:
	class MipMapLevel
	{
	public:
		uint32_t tilesX = 0;
		uint32_t tilesY = 0;

		// Indexed tile/page access:
		const FloatImageBuffer & getTileAt(uint32_t x, uint32_t y) const;
		FloatImageBuffer & getTileAt(uint32_t x, uint32_t y);

		// Memory management:
		bool isAllocated() const;
		void allocate(uint32_t tx, uint32_t ty, uint32_t numComponents, uint32_t pageSizePixels);
		void free();

	private:
		// Pages for this level = tiles[tilesX][tilesY]
		std::unique_ptr<FloatImageBuffer[]> tiles;
	};

private:

	// Input params:
	const std::string inputFileName;
	const std::string outputFileName;
	const PageFileBuilderOptions opts;

	// Pixel format of the source image.
	PixelFormat::Enum sourcePixelFormat;

	// All mip-levels in this pagefile.
	std::vector<MipMapLevel> pageFileLevels;
};

} // namespace tool {}
} // namespace vt {}

#endif // VT_TOOL_PAGEFILE_BUILDER_HPP
