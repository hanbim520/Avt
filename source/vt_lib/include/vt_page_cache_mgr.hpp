
// ================================================================================================
// -*- C++ -*-
// File: vt_page_cache_mgr.hpp
// Author: Guilherme R. Lampert
// Created on: 05/10/14
// Brief: The page cache manager.
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

#ifndef VTLIB_VT_PAGE_CACHE_MGR_HPP
#define VTLIB_VT_PAGE_CACHE_MGR_HPP

#include <vector>

namespace vt
{

// ======================================================
// CachePageCoord:
// ======================================================

struct CachePageCoord
{
	// No more than 255 pages needed
	// (typical cache size is 32x32 pages)
	using IndexType = uint8_t;

	// Coordinates of the page tile.
	IndexType x;
	IndexType y;
};

// This type must be strictly sized.
static_assert(sizeof(CachePageCoord) == 2, "Expected 2 bytes size!");

// ======================================================
// CacheEntry:
// ======================================================

struct CacheEntry
{
	// Links in the cache usage lists (LRU/MRU):
	struct CacheEntry * prev;
	struct CacheEntry * next;

	// Page position in the VT and mip-level.
	PageId pageId;

	// Position the above page currently occupies in the cache texture.
	CachePageCoord cacheCoord;
};

// ======================================================
// CachePageTree:
// ======================================================

class CachePageTree final
	: public NonCopyable
{
public:

	// Constructor:
	CachePageTree(const int * vtPagesX, const int * vtPagesY, int vtNumLevels);

	// 'memset' every entry to null/zero.
	void clear();

	// Mutable access:
	CacheEntry * get(PageId id);
	CacheEntry * get(int level, int x, int y);

	// Immutable access:
	const CacheEntry * get(PageId id) const;
	const CacheEntry * get(int level, int x, int y) const;

	// Set a given tree entry (entry may be null):
	void set(PageId id, CacheEntry * entry);
	void set(int level, int x, int y, CacheEntry * entry);

	// Inline accessors:
	int getNumLevels() const { return numLevels; }
	int getNumPagesX(int level) const { return numPagesX[level]; }
	int getNumPagesY(int level) const { return numPagesY[level]; }

private:

	// Num mip-levels in the Virtual Texture:
	int numLevels;

	// Flattened matrix of cache entries.
	// This is viewed a 2D array. The 'width' is for the mip-levels
	// while the 'height' is for the individual array of pages for a level.
	//
	// 'levels' is conceptually equivalent to:
	// CacheEntry * levels[num_levels][pages_per_level];
	//
	// All entry pointers share the same backing-store (cacheEntryPtrPool)
	// so that they can be allocated in the same memory block.
	//
	std::vector<CacheEntry *> cacheEntryPtrPool;
	std::array<CacheEntry **, MaxVTMipLevels> levels;

	// Per-level page counts:
	std::array<int, MaxVTMipLevels> numPagesX;
	std::array<int, MaxVTMipLevels> numPagesY;
};

// ======================================================
// PageUpload:
// ======================================================

struct PageUpload
{
	const Pixel4b * pageData;   // The actual RGBA pixel data (size of a page).
	CachePageCoord  cacheCoord; // In tiles/pages.
};

// ======================================================
// CachePageStatus:
// ======================================================

enum class CachePageStatus
{
	Cached,     // Page is in cache and ready to go.
	InFlight,   // Page request still pending completion.
	Unavailable // Not in cache.
};

// ======================================================
// PageCacheMgr:
// ======================================================

class PageCacheMgr final
	: public NonCopyable
{
public:

	static constexpr int CacheSizeInPages = PageTable::TableSizeInPages; // Width or height of the cache
	static constexpr int TotalCachePages  = PageTable::TotalTablePages;  // Total pages (CacheSizeInPages^2)

	struct RuntimeStats
	{
		int servicedRequests;
		int newFrameRequests;
		int droppedRequests;
		int reFrameRequests;
		int totalFrameRequests;
		int hitFrameRequests;
	};

	// Init the page cache manager.
	PageCacheMgr(const int * vtPagesX, const int * vtPagesY, int vtNumLevels);

	// Lookup a page, incrementing its use count if it is already in cache.
	CachePageStatus lookupPage(PageId id);

	// Verify that a previously requested page is still wanted by the cache.
	bool stillWantPage(PageId id) const;

	// Accommodates a freshly loaded page into the cache.
	CachePageCoord accommodatePage(PageId id);

	// Notify the cache that a request previously placed with 'lookupPage()'
	// had to be dropped. This is called by the page resolver.
	void notifyDroppedRequest(PageId id);

	// Purges all pages from the cache.
	void purgeCache();

	// Validate and possibly fix an out of bounds page id.
	PageId sanitizePageId(const PageId pageId) const;

	// Get the full array of pages for the cache. This is used by indirection table updates.
	const CacheEntry * getCacheEntries() const { return cacheEntryPool.data(); }

	// Get the current runtime stats of the cache. Used for debug displaying.
	const RuntimeStats & getCacheStats() const { return stats; }

	// This should be called at the end of a frame, after the cache is updated.
	void clearCacheStats() { clearPodObject(stats); }

private:

	// Allocate a slot in the cache, possibly evicting an older page.
	CacheEntry * allocPageEntry();

	// Validate level,x,y for a page request.
	bool validPageRequest(int level, int pageX, int pageY) const;

	// A pointer set to the value returned by this method is
	// interpreted as a page that was requested but is not fully loaded yet.
	// The entry itself is just a dummy and its contents are meaningless.
	static CacheEntry * getRequestedPageDummy();

private:

	// Runtime stats counters for the cache manager:
	RuntimeStats stats;

	// List heads:
	CacheEntry * mru; // MRU list
	CacheEntry * lru; // LRU list

	// CPU side page table: translates page id -> cache items
	CachePageTree cachePageTree;

	// A pool with all cache entries. Allocated statically
	// as a single contiguous block. 'cachePageTree' pointers refer to this pool.
	std::array<CacheEntry, TotalCachePages> cacheEntryPool;
};

} // namespace vt {}

#endif // VTLIB_VT_PAGE_CACHE_MGR_HPP
