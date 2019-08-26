
// ================================================================================================
// -*- C++ -*-
// File: vt_page_table.hpp
// Author: Guilherme R. Lampert
// Created on: 03/10/14
// Brief: Page cache table texture.
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

#ifndef VTLIB_VT_PAGE_TABLE_HPP
#define VTLIB_VT_PAGE_TABLE_HPP

namespace vt
{

// Maximum number of mip-map levels for a Virtual Texture.
// Indexing range: [0, MaxVTMipLevels - 1]
constexpr int MaxVTMipLevels = 16;

// OpenGL page upload packet.
struct PageUpload;

// ======================================================
// PageTable:
// ======================================================

class PageTable final
	: public NonCopyable
{
public:

	// Size in pixels of a single logical page:
	static constexpr int PageSizeInPixels       = 128;
	static constexpr int HalfPageSizeInPixels   = (PageSizeInPixels / 2);
	static constexpr int PageBorderSizeInPixels = 4; // Border on each side

	// Texture size in logical pages and total page count:
	static constexpr int TableSizeInPages  = 16;
	static constexpr int TotalTablePages   = (TableSizeInPages * TableSizeInPages);

	// Texture size in pixels and total pixel count:
	static constexpr int TableSizeInPixels = (TableSizeInPages  * PageSizeInPixels);
	static constexpr int TotalTablePixels  = (TableSizeInPixels * TableSizeInPixels);

	// Default constructor initializes the table texture.
	// Might throw and exception if initialization fails.
	PageTable();

	// Frees the page table texture.
	~PageTable();

	// Bind the texture as current OpenGL state.
	void bind(int texUnit = 0) const;

	// Uploads a page to the OpenGL texture. Texture must be bound first via PageTable::Bind().
	void uploadPage(const PageUpload & upload);

	// Set the OpenGL texture filtering mode for the page table texture.
	void setGLTextureFilter(GLenum minFilter, GLenum magFilter);

	// Get current filter. Tuple index 0 is the min-fiter. Index 1 is the mag-filter.
	std::tuple<GLenum, GLenum> getGLTextureFilter() const;

	// Draws the page table texture as a screen-space quadrilateral for debug visualization.
	// Manually binding the texture is not necessary. 'overlayScale' controls the scale of the overlay quad. From 0 to 1.
	void visualizePageTableTexture(const float overlayScale[2]) const;

	// Write the top level (mip:0) of the page table to an image file.
	// The file name/path should not include an extension. This is useful for debugging the VT system.
	bool writePageTableTextureToFile(const std::string & pathname) const;

	// Fills the entire texture with tiles colored using a gradient
	// easy to identify. This is useful when debugging the VT system.
	void fillTextureWithDebugData();

private:

	void initTexture();
	static void halveImageBoxFilter(const uint8_t * src, uint8_t * dest, int & width, int & height, int components);

private:

	GLuint pageTextureId;
	GLenum pageTexMinFilter;
	GLenum pageTexMagFilter;

	// Scratch data used for downsampling page uploads.
	// This can only be run from the main thread, due to OpenGL constraints, so it can be static.
	static Pixel4b halfPageData[HalfPageSizeInPixels * HalfPageSizeInPixels];
};

} // namespace vt {}

#endif // VTLIB_VT_PAGE_TABLE_HPP
