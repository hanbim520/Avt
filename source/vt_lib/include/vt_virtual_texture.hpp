
// ================================================================================================
// -*- C++ -*-
// File: vt_virtual_texture.hpp
// Author: Guilherme R. Lampert
// Created on: 03/11/14
// Brief: Virtual Texture object.
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

#ifndef VTLIB_VIRTUAL_TEXTURE_HPP
#define VTLIB_VIRTUAL_TEXTURE_HPP

namespace vt
{

// ======================================================
// VirtualTexture:
// ======================================================

class VirtualTexture final
	: public NonCopyable
{
public:

	// Construct from a VTFF page file. If 'pageIndirection' is null a new table is created.
	VirtualTexture(VTFFPageFilePtr vtffFile, PageIndirectionTablePtr pageIndirection = nullptr);

	// Construct with a set of page files. This is a common case for objects that render using a
	// diffuse + normal + specular texture set. For each page file, a unique page table texture is created.
	// All page files share the same indirection table and cache manager.
	VirtualTexture(VTFFPageFilePtr * vtffFiles, size_t numFiles, PageIndirectionTablePtr pageIndirection = nullptr);

	// Construct from an exiting page file with user provided texture dimensions.
	// Can optionally provide an exiting indirection table. If 'pageIndirection' is null a new table is created.
	VirtualTexture(const int * vtPagesX, const int * vtPagesY, int vtNumLevels,
	               PageFilePtr pageFile, PageIndirectionTablePtr pageIndirection);

	// Per frame update of the virtual texture.
	// Must be called every rendering frame of the game loop to upload new texture pages to the GPU.
	void frameUpdate(const FulfilledPageRequestQueue & pageRequestUploads, bool updateIndirectionTable = true);

	// Draws a developer stats panel for this texture and its cache. Uses the built-in VT GUI.
	void drawStats() const;

	// Clear the cache and texture debug stats after a draw with drawStats().
	void clearStats();

	// Purges the underlaying page cache and updates the indirection table.
	// Will also fill the page table texture with debug data if VT_EXTRA_DEBUG is set.
	void purgeCache();

	// Replaces the current page file with the new one and sets the new one
	// to point to the old page file that this texture had at the given index.
	void replacePageFile(PageFilePtr & newPageFile, unsigned int index = 0);

	// Global index of this VT, necessary when rendering with multiple VTs.
	int getTextureIndex() const { return textureIndex; }

	// This is called internally by the PageProvider when the texture is linked to it.
	void setTextureIndex(unsigned int index) { textureIndex = index; }

	// This is called internally by PageProvider when the texture is linked to it.
	// You cannot render with a VirtualTexture until it is linked to a PageProvider.
	void setPageProvider(PageProvider * provider) { pageProvider = provider; }

	// This is called internally by PageResolver when the texture is linked to it.
	// You cannot render with a VirtualTexture until it is linked to a PageResolver.
	void setPageResolver(PageResolver * resolver) { pageResolver = resolver; }

	// Get PageFile:
	const PageFile * getPageFile(unsigned int index = 0) const { return pageFiles[index].get(); }
	PageFile * getPageFile(unsigned int index = 0) { return pageFiles[index].get(); }
	unsigned int getNumPageFiles() const { return static_cast<unsigned int>(pageFiles.size()); }

	// Get PageProvider:
	const PageProvider * getPageProvider() const { return pageProvider; }
	PageProvider * getPageProvider() { return pageProvider; }

	// Get PageResolver:
	const PageResolver * getPageResolver() const { return pageResolver; }
	PageResolver * getPageResolver() { return pageResolver; }

	// Get PageTable:
	const PageTable * getPageTable(unsigned int index = 0) const { return pageTables[index].get(); }
	PageTable * getPageTable(unsigned int index = 0) { return pageTables[index].get(); }
	unsigned int getNumPageTables() const { return static_cast<unsigned int>(pageTables.size()); }

	// Get PageCacheMgr:
	const PageCacheMgr * getPageCache() const { return pageCacheMgr.get(); }
	PageCacheMgr * getPageCache() { return pageCacheMgr.get(); }

	// Get PageIndirectionTable:
	const PageIndirectionTablePtr getPageIndirectionTable() const { return indirectionTable; }
	PageIndirectionTablePtr getPageIndirectionTable() { return indirectionTable; }

	// <width, height> pair of this VT, in pixels and pages:
	const float * getLevel0SizeInPixels() const { return level0SizePixels; }
	const float * getLevel0SizeInPages()  const { return level0SizePages;  }

	// Number of mipmap levels for this VT. A value between 1 and MaxVTMipLevels - 1.
	int getNumLevels() const { return numLevels; }

private:

	using PageTablePtr = std::unique_ptr<PageTable>;
	using PageCachePtr = std::unique_ptr<PageCacheMgr>;

	// The texture data sources we stream from.
	// Need at least one, but can have many.
	std::vector<PageFilePtr>  pageFiles;
	std::vector<PageTablePtr> pageTables;

	// The virtual texture coordinate translation table.
	// Can be shared by several virtual textures that have the same dimensions.
	PageIndirectionTablePtr indirectionTable;

	// Each texture requires an exclusive page cache manager for the page table(s).
	PageCachePtr pageCacheMgr;

	// Weak references to the page provider and resolver that service this texture.
	// These pointers are only set when the texture is linked to them.
	PageProvider * pageProvider;
	PageResolver * pageResolver;

	// VT texture index in the provider & resolver list.
	int textureIndex;

	// These are required by the PageResolver. Cached for easy access.
	int   numLevels;
	float level0SizePixels[2];
	float level0SizePages[2];

	// Debug counters:
	unsigned int numPageUploads;
	unsigned int numIndirectionTableUpdates;
};

// ======================================================
// Rendering with the VT:
// ======================================================

// Page id generation pass:
void renderBindPageIdPassShader();
void renderBindTextureForPageIdPass(const VirtualTexture & vtTex);

// Final/textured render pass:
void renderBindTexturedPassShader(bool simpleRender);
void renderBindTextureForTexturedPass(const VirtualTexture & vtTex);

// Render params (require the proper shader to be bound):
void renderSetMvpMatrix(const float * mvpMatrix);
void renderSetMipDebugBias(float mipDebugBias);
void renderSetLog2MipScaleFactor(float log2MipScaleFactor);
void renderSetLightPosObjectSpace(const float pos[4]);
void renderSetViewPosObjectSpace(const float pos[4]);

} // namespace vt {}

#endif // VTLIB_VIRTUAL_TEXTURE_HPP
