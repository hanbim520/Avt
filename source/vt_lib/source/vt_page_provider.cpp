
// ================================================================================================
// -*- C++ -*-
// File: vt_page_provider.cpp
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

#include "vt.hpp"
#include <dispatch/dispatch.h> // Apple's GCD

namespace vt
{

// ======================================================
// PageProvider:
// ======================================================

PageProvider::PageProvider(const bool async)
	: outstandingRequests(0)
	, forceSynchronous(!async)
{
	if (isAsync())
	{
		vtLogComment("New asynchronous PageProvider created...");
	}
	else
	{
		vtLogComment("New serial PageProvider created...");
	}
}

bool PageProvider::addPageRequest(const PageId requestId)
{
	if (outstandingRequests == MaxOutstandingPageRequests)
	{
		vtLogComment("Max outstanding page requests limit (" <<
				MaxOutstandingPageRequests << ") reached! Dropping request...");

		return false;
	}

	if (!forceSynchronous)
	{
		return runAsyncRequest(requestId);
	}
	else
	{
		return runImmediateRequest(requestId);
	}
}

size_t PageProvider::getReadyQueue(FulfilledPageRequestQueue & readyQueueOut)
{
	std::lock_guard<std::mutex> lock(readyQueueMutex);

	readyQueueOut = std::move(readyQueue);
	assert(readyQueue.empty());

	return readyQueueOut.size();
}

bool PageProvider::runAsyncRequest(const PageId requestId)
{
	const size_t textureIndex = std::min(static_cast<size_t>(pageIdExtractTextureIndex(requestId)), registeredTextures.size());

	// Place a request for each file in the texture.
	const unsigned int numPageFiles = registeredTextures[textureIndex]->getNumPageFiles();
	for (unsigned int f = 0; f < numPageFiles; ++f)
	{
		PageFile * pageFile = registeredTextures[textureIndex]->getPageFile(f);
		assert(pageFile != nullptr);

		struct WorkerContext
		{
			PageProvider * provider;  // Provider that started the request
			PageFile     * pageFile;  // Page file where to fetch the page from
			PageId         requestId; // Page to be loaded
			uint32_t       fileId;    // Page file index within the VT
		};

		// Would love to get rid of this memory allocation somehow...
		WorkerContext * context = new WorkerContext{ this, pageFile, requestId, f };

		// Run the request asynchronously using GCD:
		dispatch_async_f(
			dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
			context, [](void * param)
			{
				// 'param' points to the dynamically allocated WorkerContext:
				WorkerContext * __restrict workerCtx = reinterpret_cast<WorkerContext *>(param);

				assert(workerCtx            != nullptr);
				assert(workerCtx->provider  != nullptr);
				assert(workerCtx->pageFile  != nullptr);
				assert(workerCtx->requestId != InvalidPageId);

				PageRequestDataPacket pageRequest;
				pageRequest.pageId = workerCtx->requestId;
				pageRequest.fileId = workerCtx->fileId;

				workerCtx->pageFile->loadPage(workerCtx->requestId, pageRequest);
				workerCtx->provider->pushReadyRequest(pageRequest);

				delete workerCtx;
			}
		);

		++outstandingRequests;
	}

	//
	// TODO I have noticed some visual artifacts
	// at times when running with this async path...
	// Should review this sometime to figure out where
	// the problem is originating...
	//
	return true;
}

bool PageProvider::runImmediateRequest(const PageId requestId)
{
	const size_t textureIndex = std::min(static_cast<size_t>(pageIdExtractTextureIndex(requestId)), registeredTextures.size());

	// Place a request for each file in the texture.
	const unsigned int numPageFiles = registeredTextures[textureIndex]->getNumPageFiles();
	for (unsigned int f = 0; f < numPageFiles; ++f)
	{
		PageFile * pageFile = registeredTextures[textureIndex]->getPageFile(f);
		assert(pageFile != nullptr);

		// Load the page data immediately, from this thread:
		PageRequestDataPacket pageRequest;
		pageRequest.pageId = requestId;
		pageRequest.fileId = f;

		++outstandingRequests;
		pageFile->loadPage(requestId, pageRequest);

		pushReadyRequest(pageRequest);
	}

	return true;
}

void PageProvider::pushReadyRequest(const PageRequestDataPacket & readyRequest)
{
	std::lock_guard<std::mutex> lock(readyQueueMutex);
	readyQueue.push_back(readyRequest);

	--outstandingRequests;
}

void PageProvider::registerVirtualTexture(VirtualTexture * vtTex)
{
	assert(vtTex != nullptr);

	registeredTextures.push_back(vtTex);
	vtTex->setTextureIndex(static_cast<int>(registeredTextures.size()) - 1);
	vtTex->setPageProvider(this);
}

void PageProvider::unregisterVirtualTexture(VirtualTexture * vtTex)
{
	assert(vtTex != nullptr);

	if (!registeredTextures.empty())
	{
		auto it = std::find(std::begin(registeredTextures), std::end(registeredTextures), vtTex);
		if (it != std::end(registeredTextures))
		{
			registeredTextures.erase(it);
		}
		vtTex->setTextureIndex(-1);
		vtTex->setPageProvider(nullptr);
	}
}

void PageProvider::unregisterAllVirtualTextures()
{
	registeredTextures.clear();
}

} // namespace vt {}
