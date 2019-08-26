
// ================================================================================================
// -*- C++ -*-
// File: vt_page_indirection_table.hpp
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

#ifndef VTLIB_VT_PAGE_INDIRECTION_TABLE_HPP
#define VTLIB_VT_PAGE_INDIRECTION_TABLE_HPP

namespace vt
{

// ======================================================
// PageIndirectionTable:
// ======================================================

class PageIndirectionTable
{
public:

	// Default constructor initializes the table texture.
	// Might throw and exception if initialization fails.
	PageIndirectionTable(const int * vtPagesX, const int * vtPagesY, int vtNumLevels);

	// Frees the OpenGL texture handle.
	virtual ~PageIndirectionTable();

	// Bind the texture as current OpenGL state.
	void bind(int texUnit = 0) const;

	// Draws the indirection texture as a screen-space quadrilateral for debug visualization.
	// Manually binding the texture is not necessary. 'overlayScale' controls the scale of the overlay quad. From 0 to 1.
	void visualizeIndirectionTexture(const float overlayScale[2]) const;

	// Update the indirection table texture. This is called whenever the page cache changes.
	// Array size must be 'PageTable::TotalTablePages'. Must bind first with PageIndirectionTable::bind().
	virtual void updateIndirectionTexture(const struct CacheEntry * const pages) = 0;

	// Write every mip-level of the indirection table to image files.
	// The file name/path should not include an extension. Each level will be named ( pathname + "_level_number" ).
	virtual bool writeIndirectionTextureToFile(const std::string & pathname, bool recolor) const = 0;

protected:

	// OpenGL texture handle:
	GLuint indirectionTextureId;

	// Num mip-levels in the Virtual Texture and per-level page counts:
	int numLevels;
	std::array<int, MaxVTMipLevels> numPagesX;
	std::array<int, MaxVTMipLevels> numPagesY;
};

using PageIndirectionTablePtr = std::shared_ptr<PageIndirectionTable>;

// ======================================================
// PageIndirectionTableRgba8888:
// ======================================================

// Uses a RGBA 8:8:8:8 texture to store the page indirection table.
class PageIndirectionTableRgba8888 final
	: public PageIndirectionTable, public NonCopyable
{
public:

	PageIndirectionTableRgba8888(const int * vtPagesX, const int * vtPagesY, int vtNumLevels);

	void updateIndirectionTexture(const struct CacheEntry * const pages) override;
	bool writeIndirectionTextureToFile(const std::string & pathname, bool recolor) const override;

private:

	void initTexture();

	// Size of a RGBA 8:8:8:8 pixel.
	struct TableEntry
	{
		uint8_t cachePageX; // R
		uint8_t cachePageY; // G
		uint8_t scaleHigh;  // B
		uint8_t scaleLow;   // A
	};
	static_assert(sizeof(TableEntry) == 4, "Expected 4 bytes size!");

private:

	int totalTableEntries;

	// Pointers to the indirection texture levels of 'tableEntryPool':
	std::array<TableEntry *, MaxVTMipLevels> tableLevels;

	// Data store. 'tableLevels' are pointers to this array.
	// This allows us to perform a single memory allocation.
	std::unique_ptr<TableEntry[]> tableEntryPool;
};

// ======================================================
// PageIndirectionTableRgb565:
// ======================================================

// Uses a RGB 5:6:5 texture to store the page indirection table.
class PageIndirectionTableRgb565 final
	: public PageIndirectionTable, public NonCopyable
{
public:

	PageIndirectionTableRgb565(const int * vtPagesX, const int * vtPagesY, int vtNumLevels);

	void updateIndirectionTexture(const struct CacheEntry * const pages) override;
	bool writeIndirectionTextureToFile(const std::string & pathname, bool recolor) const override;

private:

	void initTexture();

	// Size of a RGB 5:6:5 pixel. Use bit shifting to manipulate the data.
	using TableEntry = uint16_t;
	static_assert(sizeof(TableEntry) == 2, "Expected 2 bytes size!");

private:

	int log2VirtPagesWide;
	int totalTableEntries;

	// Pointers to the indirection texture levels of 'tableEntryPool':
	std::array<TableEntry *, MaxVTMipLevels> tableLevels;

	// Data store. 'tableLevels' are pointers to this array.
	// This allows us to perform a single memory allocation.
	std::unique_ptr<TableEntry[]> tableEntryPool;
};

// ======================================================
// Factory function. Creates based on startup param.
// ======================================================

// Creates a PageIndirectionTable instance based on the
// global indirection table format configuration parameter.
PageIndirectionTablePtr createIndirectionTable(const int * vtPagesX, const int * vtPagesY, int vtNumLevels);

} // namespace vt {}

#endif // VTLIB_VT_PAGE_INDIRECTION_TABLE_HPP
