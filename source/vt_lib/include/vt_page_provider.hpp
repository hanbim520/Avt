
// ================================================================================================
// -*- C++ -*-
// File: vt_page_provider.hpp
// Author: Guilherme R. Lampert
// Created on: 05/10/14
// Brief: The Page Provider manages the loading of texture pages from the backing-store.
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

#ifndef VTLIB_VT_PAGE_PROVIDER_HPP
#define VTLIB_VT_PAGE_PROVIDER_HPP

#include <atomic>
#include <mutex>
#include <vector>
#include <deque>

namespace vt
{

class VirtualTexture;

struct PageRequestDataPacket
{
	// Number of RGBA pixels in a page:
	static constexpr int TotalPagePixels = (PageTable::PageSizeInPixels * PageTable::PageSizeInPixels);

	// Which page the data refers to:
	PageId pageId;

	// Which file within the virtual texture
	// (some textures manage multiple page files).
	uint32_t fileId;

	// Pixel payload:
	Pixel4b pageData[TotalPagePixels];
};

using FulfilledPageRequestQueue = std::deque<PageRequestDataPacket>;

// ======================================================
// PageProvider:
// ======================================================

class PageProvider final
	: public NonCopyable
{
public:

	// Max requests waiting in the provider queue, per frame. Arbitrary.
	static constexpr int MaxOutstandingPageRequests = 256;

	// Construction:
	PageProvider(bool async = true);

	// Attempt to add a new request to the page request queue.
	// The request might be refused if MaxOutstandingPageRequests limit
	// was already reached for the current frame.
	bool addPageRequest(PageId requestId);

	// Steal the current ready queue.
	// This way the background threads can continue their work while one
	// queue is being consumed, at the cost of some extra memory allocations.
	// This method returns the current size of the ready queue.
	size_t getReadyQueue(FulfilledPageRequestQueue & readyQueueOut);

	// Register/unregister textures that use this resolver.
	// The provider will NOT copy the texture object. It only keeps a weak reference.
	void registerVirtualTexture(VirtualTexture * vtTex);
	void unregisterVirtualTexture(VirtualTexture * vtTex);
	void unregisterAllVirtualTextures();

	// Accessors:
	bool isAsync() const { return !forceSynchronous; }
	void setAsync(bool async) { forceSynchronous = !async; }
	int getNumOutstandingRequests() const { return static_cast<int>(outstandingRequests); }

private:

	// Internal helpers:
	bool runAsyncRequest(PageId requestId);
	bool runImmediateRequest(PageId requestId);
	void pushReadyRequest(const PageRequestDataPacket & readyRequest);

private:

	// Keep track of the number of pending requests.
	std::atomic<int> outstandingRequests;

	// The queue with fulfilled page requests.
	// The font-end thread that consumes the finished requests
	// can "steal" this queue every frame via getReadyQueue() to consume
	// the requests, while the background threads continue work on a new queue.
	// Currently, we use a simple mutex for synchronization.
	// This could be optimized to use lock-free data structure in the future.
	std::mutex readyQueueMutex;
	FulfilledPageRequestQueue readyQueue;

	// Virtual textures using this provider.
	// Just weak references. Textures must outlive the provider.
	std::vector<VirtualTexture *> registeredTextures;

	// Debug flag. False by default.
	// Force all requests to be fulfilled serially form the caller thread.
	volatile bool forceSynchronous;
};

} // namespace vt {}

#endif // VTLIB_VT_PAGE_PROVIDER_HPP
