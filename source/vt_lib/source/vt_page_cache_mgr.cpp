
// ================================================================================================
// -*- C++ -*-
// File: vt_page_cache_mgr.cpp
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

#include "vt.hpp"

namespace vt
{

// ======================================================
// CachePageTree:
// ======================================================

CachePageTree::CachePageTree(const int * vtPagesX, const int * vtPagesY, const int vtNumLevels)
	: numLevels(vtNumLevels)
{
	assert(numLevels > 0 && numLevels <= MaxVTMipLevels);

	// Clear every slot first, as some might end up unused:
	clearArray(levels);
	clearArray(numPagesX);
	clearArray(numPagesY);

	for (int l = 0; l < numLevels; ++l)
	{
		assert(vtPagesX[l] > 0);
		assert(vtPagesY[l] > 0);
		numPagesX[l] = vtPagesX[l];
		numPagesY[l] = vtPagesY[l];
	}

	// Count total number of entries:
	size_t totalEntries = 0;
	for (int l = 0; l < numLevels; ++l)
	{
		totalEntries += numPagesX[l] * numPagesY[l];
	}

	// Allocate the exact amount of storage we need:
	cacheEntryPtrPool.resize(totalEntries, nullptr);

	// Set up the pointers:
	totalEntries = 0;
	for (int l = 0; l < numLevels; ++l)
	{
		levels[l] = cacheEntryPtrPool.data() + totalEntries;
		totalEntries += numPagesX[l] * numPagesY[l]; // Move to the next level
	}

	assert(totalEntries == cacheEntryPtrPool.size());
	vtLogComment("New CachePageTree created with a total of " << totalEntries << " entries.");
}

void CachePageTree::clear()
{
	// Nullify every entry:
	std::fill(std::begin(cacheEntryPtrPool), std::end(cacheEntryPtrPool), nullptr);
}

CacheEntry * CachePageTree::get(const PageId id)
{
	return get(pageIdExtractMipLevel(id), pageIdExtractPageX(id), pageIdExtractPageY(id));
}

CacheEntry * CachePageTree::get(const int level, const int x, const int y)
{
	assert(level >= 0 && level < numLevels);

	// Fetch level plane and page index:
	CacheEntry ** levelPages = levels[level];
	const int pageIndex = x + y * numPagesX[level];

	// Validate index and return the page ref:
	assert(pageIndex < (numPagesX[level] * numPagesY[level]));
	return levelPages[pageIndex];
}

const CacheEntry * CachePageTree::get(const PageId id) const
{
	return get(pageIdExtractMipLevel(id), pageIdExtractPageX(id), pageIdExtractPageY(id));
}

const CacheEntry * CachePageTree::get(const int level, const int x, const int y) const
{
	assert(level >= 0 && level < numLevels);

	// Fetch level plane and page index:
	CacheEntry ** levelPages = levels[level];
	const int pageIndex = x + y * numPagesX[level];

	// Validate index and return the page ref:
	assert(pageIndex < (numPagesX[level] * numPagesY[level]));
	return levelPages[pageIndex];
}

void CachePageTree::set(const PageId id, CacheEntry * entry)
{
	set(pageIdExtractMipLevel(id), pageIdExtractPageX(id), pageIdExtractPageY(id), entry);
}

void CachePageTree::set(const int level, const int x, const int y, CacheEntry * entry)
{
	assert(level >= 0 && level < numLevels);

	// Fetch level plane and page index:
	CacheEntry ** levelPages = levels[level];
	const int pageIndex = x + y * numPagesX[level];

	// Validate index and set the page:
	assert(pageIndex < (numPagesX[level] * numPagesY[level]));
	levelPages[pageIndex] = entry;
}

// ======================================================
// PageCacheMgr:
// ======================================================

PageCacheMgr::PageCacheMgr(const int * vtPagesX, const int * vtPagesY, const int vtNumLevels)
	: mru(nullptr)
	, lru(nullptr)
	, cachePageTree(vtPagesX, vtPagesY, vtNumLevels)
{
	clearPodObject(stats);
	clearArray(cacheEntryPool);

	// Set the coordinates for each entry:
	CacheEntry * entries = cacheEntryPool.data();
	for (int x = 0; x < CacheSizeInPages; ++x)
	{
		for (int y = 0; y < CacheSizeInPages; ++y)
		{
			entries[x + y * CacheSizeInPages].cacheCoord.x = static_cast<CachePageCoord::IndexType>(x);
			entries[x + y * CacheSizeInPages].cacheCoord.y = static_cast<CachePageCoord::IndexType>(y);
			entries[x + y * CacheSizeInPages].pageId = InvalidPageId;
		}
	}

	purgeCache();
	vtLogComment("New PageCacheMgr created. Cache size: " << CacheSizeInPages << "x" << CacheSizeInPages << " pages.");
}

CachePageStatus PageCacheMgr::lookupPage(const PageId id)
{
	const int level = pageIdExtractMipLevel(id);
	const int pageX = pageIdExtractPageX(id);
	const int pageY = pageIdExtractPageY(id);
	assert(validPageRequest(level, pageX, pageY));

	stats.totalFrameRequests++;
	CacheEntry * entry = cachePageTree.get(level, pageX, pageY);

	if (entry == nullptr) // Unavailable
	{
		// Not in cache, request it:
		cachePageTree.set(level, pageX, pageY, getRequestedPageDummy());
		stats.newFrameRequests++;

		// Tell the provider to attempt a page load.
		return CachePageStatus::Unavailable;
	}

	if (entry == getRequestedPageDummy()) // In-flight
	{
		// Still waiting for it to be loaded.
		// Use lower mip-level for now...
		stats.reFrameRequests++;
		return CachePageStatus::InFlight;
	}

	// The requested frame is already in cache:
	stats.hitFrameRequests++;

	// Since it is already loaded, put it at the front of the MRU list:
	if (entry != mru)
	{
		if (entry->next != nullptr)
		{
			// Unlink from old position:
			entry->prev->next = entry->next;
			entry->next->prev = entry->prev;
		}
		else
		{
			// This was the LRU.
			assert(entry == lru);
			entry->prev->next = nullptr;
			lru = entry->prev;
		}

		// Link at front:
		assert(mru != nullptr);
		entry->prev = nullptr;
		entry->next = mru;
		mru->prev = entry;

		// Save it for later:
		mru = entry;
	}

	return CachePageStatus::Cached;
}

bool PageCacheMgr::stillWantPage(const PageId id) const
{
	const int level = pageIdExtractMipLevel(id);
	const int pageX = pageIdExtractPageX(id);
	const int pageY = pageIdExtractPageY(id);
	assert(validPageRequest(level, pageX, pageY));

	return cachePageTree.get(level, pageX, pageY) == getRequestedPageDummy();
}

CachePageCoord PageCacheMgr::accommodatePage(const PageId id)
{
	const int level = pageIdExtractMipLevel(id);
	const int pageX = pageIdExtractPageX(id);
	const int pageY = pageIdExtractPageY(id);
	assert(validPageRequest(level, pageX, pageY));

	CacheEntry * entry = allocPageEntry();
	assert(entry != nullptr);

	cachePageTree.set(level, pageX, pageY, entry);
	entry->pageId = id;

	stats.servicedRequests++;
	return entry->cacheCoord;
}

CacheEntry * PageCacheMgr::allocPageEntry()
{
	// If the slot is not empty, clear it:
	if (lru->pageId != InvalidPageId)
	{
		// Mark the page as unavailable and remove it from the level count.
		// This is only necessary after startup or a cache purge.
		cachePageTree.set(lru->pageId, nullptr);
	}

	// Get the LRU and reuse it:
	CacheEntry * entry = lru;
	entry->prev->next = nullptr;
	lru = entry->prev;

	// Move it to the head of the list:
	entry->prev = nullptr;
	entry->next = mru;
	mru->prev = entry;
	mru = entry;

	return entry;
}

void PageCacheMgr::notifyDroppedRequest(const PageId id)
{
	const int level = pageIdExtractMipLevel(id);
	const int pageX = pageIdExtractPageX(id);
	const int pageY = pageIdExtractPageY(id);
	assert(validPageRequest(level, pageX, pageY));

	// Sanity check:
	assert(cachePageTree.get(level, pageX, pageY) == getRequestedPageDummy());

	// Clear this entry:
	cachePageTree.set(level, pageX, pageY, nullptr);

	// Request was dropped by the page provider. Bump the counter:
	stats.droppedRequests++;
}

void PageCacheMgr::purgeCache()
{
	clearPodObject(stats);
	cachePageTree.clear();

	// Mark all pages in the cache as unavailable/invalid:
	CacheEntry * entries = cacheEntryPool.data();
	for (int i = 0; i < TotalCachePages; ++i)
	{
		CacheEntry * page = &entries[i];
		page->prev = (i > 0) ? &entries[i - 1] : nullptr;
		page->next = (i < (TotalCachePages - 1)) ? &entries[i + 1] : nullptr;
		page->pageId = InvalidPageId;
	}

	mru = &entries[0];
	lru = &entries[TotalCachePages - 1];
}

PageId PageCacheMgr::sanitizePageId(const PageId pageId) const
{
	int x = pageIdExtractPageX(pageId);
	int y = pageIdExtractPageY(pageId);
	int level = pageIdExtractMipLevel(pageId);
	int texId = pageIdExtractTextureIndex(pageId);

	if (level >= cachePageTree.getNumLevels())
	{
		level = cachePageTree.getNumLevels() - 1;
	}
	if (x >= cachePageTree.getNumPagesX(level))
	{
		x = cachePageTree.getNumPagesX(level) - 1;
	}
	if (y >= cachePageTree.getNumPagesY(level))
	{
		y = cachePageTree.getNumPagesY(level) - 1;
	}

	return makePageId(x, y, level, texId);
}

bool PageCacheMgr::validPageRequest(const int level, const int pageX, const int pageY) const
{
	return (level < cachePageTree.getNumLevels())      &&
	       (pageX < cachePageTree.getNumPagesX(level)) &&
	       (pageY < cachePageTree.getNumPagesY(level));
}

CacheEntry * PageCacheMgr::getRequestedPageDummy()
{
	static CacheEntry requestedPageDummy;
	return &requestedPageDummy;
}

} // namespace vt {}
