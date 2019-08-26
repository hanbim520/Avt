
// ================================================================================================
// -*- C++ -*-
// File: vt_page_resolver.cpp
// Author: Guilherme R. Lampert
// Created on: 04/10/14
// Brief: The Page Resolver is responsible for the feedback pass analysis.
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
#include <algorithm>

namespace vt
{

// We are casting Pixel4b to PageId.
static_assert(sizeof(Pixel4b) == sizeof(PageId), "Sizes must match!");

// ======================================================
// PageResolver:
// ======================================================

PageResolver::PageResolver(PageProvider & provider)
	: PageResolver(provider, DefaultPageIdFboSize, DefaultPageIdFboSize, DefaultMaxPageRequestsPerFrame)
{
	// Forwarded to parameterized constructor.
}

PageResolver::PageResolver(PageProvider & provider, const int fboWidth, const int fboHeight, const int maxFrameRequests)
	: pageProvider(provider)
	, maxPageRequestsPerFrame(maxFrameRequests)
	, pageIdFbo(0)
	, fboColorTex(0)
	, pageIdFboWidth(0)
	, pageIdFboHeight(0)
	, originalFbo(0)
	, originalRbo(0)
	, visiblePages(0)
{
	clearArray(originalViewport);
	initFrameBuffer(fboWidth, fboHeight);
}

PageResolver::~PageResolver()
{
	gl::deleteFrameBuffer(pageIdFbo);
	gl::delete2DTexture(fboColorTex);
}

void PageResolver::beginPageIdPass()
{
	gl::useFrameBuffer(pageIdFbo);

	// A white pixel is equivalent to an invalid page.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	glViewport(0, 0, pageIdFboWidth, pageIdFboHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PageResolver::endPageIdPass()
{
	feedbackBufferAnalysis();

	gl::useFrameBuffer(originalFbo);
	glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);
}

void PageResolver::registerVirtualTexture(VirtualTexture * vtTex)
{
	assert(vtTex != nullptr);

	registeredTextures.push_back(vtTex);
	vtTex->setTextureIndex(static_cast<int>(registeredTextures.size()) - 1);
	vtTex->setPageResolver(this);
}

void PageResolver::unregisterVirtualTexture(VirtualTexture * vtTex)
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
		vtTex->setPageResolver(nullptr);
	}
}

void PageResolver::unregisterAllVirtualTextures()
{
	registeredTextures.clear();
}

void PageResolver::visualizePageIds(const float overlayScale[2]) const
{
	// Set shader and scaling.
	//
	// Note: We use the same shader used by the indirection table visualization here
	// because both textures need to be recolored in the same way, to be properly visualized.
	//
	gl::useShaderProgram(getGlobalShaders().drawIndirectionTable.programId);
	gl::setShaderProgramUniform(getGlobalShaders().drawIndirectionTable.unifNdcQuadScale, overlayScale, 2);

	// Draw a quad with the texture applied to it:
	gl::use2DTexture(fboColorTex);
	gl::drawNdcQuadrilateral();
	gl::use2DTexture(0);

	gl::useShaderProgram(0);
}

void PageResolver::feedbackBufferAnalysis()
{
	// Data read-back.
	// This could be asynchronous if we had PBOs on GL-ES.
	// AFAIK, currently there is no other alternative.
	gl::readFrameBuffer(pageIdFbo, 0, 0, pageIdFboWidth, pageIdFboHeight,
			GL_RGBA, GL_UNSIGNED_BYTE, feedbackBuffer.get());

	// Each pixel = 1 page.
	const size_t numPages = pageIdFboWidth * pageIdFboHeight;
	const PageId * __restrict framePages = reinterpret_cast<const PageId *>(feedbackBuffer.get());

	//
	// Notes:
	// We'll deal with integers most of the time because C bitfields
	// are quite slow if compared with shifts and masking.
	//
	// Sorting:
	// This function will order all page requests by mip-level,
	// placing the pages with a larger value first.
	// If two or more pages share the same level,
	// the tie-breaking criteria will be the number of
	// occurrences of each page, placing the ones with a
	// higher frequency first.
	//

	// Sanity checks:
	assert(pageMap.empty());
	assert(sortedPages.empty());

	// Insert into map, counting frequencies.
	// This will result in a table of unique pages.
	for (size_t p = 0; p < numPages; ++p)
	{
		if (framePages[p] != InvalidPageId)
		{
			pageMap[framePages[p]]++;
		}
		// Invalid pages are a result of the background color
		// and parts of the scene that are not using the VT renderer.
	}

	// Copy unique pages to the vector:
	sortedPages.reserve(pageMap.size());
	for (const auto & pagePair : pageMap)
	{
		sortedPages.push_back(pagePair.first); // 'first' is the page id.
	}

	// Actually sort based on mip-level (higher first).
	// Tie-breaking criteria is the number of occurrences.
	//
	// std::sort is roughly O(N*log(N)).
	// std::unordered_map lookup is amortized constant.
	//
	std::sort(std::begin(sortedPages), std::end(sortedPages),
		[this](PageId a, PageId b) -> bool
		{
			// Compare the mip-level:
			const int aMip = pageIdExtractMipLevel(a);
			const int bMip = pageIdExtractMipLevel(b);

			// a goes before b
			if (aMip > bMip)
			{
				return true;
			}

			// a goes before b if its frequency is greater
			if (aMip == bMip)
			{
				return pageMap[a] > pageMap[b];
			}

			// a does not go before b
			return false;
		}
	);

	visiblePages = static_cast<int>(pageMap.size());

	//
	// TODO: When this happens, the cache is oversubscribed.
	// We have to change maxPageRequestsPerFrame as it is done now and
	// also change the mipmap LOD (with renderSetLog2MipScaleFactor)
	// to request less pages on the next frames.
	//
	// This must be done for a while until the system stabilizes.
	// Once stabilized, the older values can should be restored.
	// We will need a timer for this ...
	//
	if (visiblePages >= PageTable::TotalTablePages)
	{
		maxPageRequestsPerFrame = PageTable::TotalTablePages;
	}

	// Ensure that the top page (max_mip - 1) is alway in cache
	// by automatically requesting it every frame.
//	addDefaultRequests();
	// FIXME does not work well enough!
	// Better without, but still gotta fix this somehow...
	// Need to lock pages directly in the PageCacheMgr.

	// Generate the needed page requests:
	// (Up to the max new requests allowed per frame).
	size_t newRequests = 0;
	for (size_t r = 0; (r < sortedPages.size() && newRequests < static_cast<size_t>(maxPageRequestsPerFrame)); ++r)
	{
		// I'm forced to sanitize the page ids here because of out-of-range values.
		// This should not be necessary, assuming everything is implemented correctly.
		// However, I was not able to find the source of the spurious invalid ids.
		// They don't show up always, which makes them pretty hard to track.
		// This could be an issue caused by floating-point precision during the
		// float -> RGBA conversions in the shader??? Clamping the values doesn't
		// seem to produce any noticeable visual artifacts. Worst case you'll get
		// a slightly more blurred texture.

		const PageId requestId = sortedPages[r];
		const size_t textureIndex = std::min(static_cast<size_t>(pageIdExtractTextureIndex(requestId)), registeredTextures.size());

		PageCacheMgr * pageCache = registeredTextures[textureIndex]->getPageCache();
		newRequests += processPageRequest(pageCache->sanitizePageId(requestId), *pageCache);
	}

	#ifndef VT_NO_LOGGING
	if (newRequests < sortedPages.size())
	{
		const size_t dropped = sortedPages.size() - newRequests;
		vtLogComment(dropped << " page requests were dropped from PageResolver this frame...");
	}
	#endif // VT_NO_LOGGING

	// Cleanup for next frame.
	sortedPages.clear();
	pageMap.clear();
}

int PageResolver::processPageRequest(const PageId requestId, PageCacheMgr & pageCache)
{
	// Lookup the page, returning 'Cached' if available and incrementing its use count.
	// If the page is not in cache, 'Unavailable' is returned.
	const CachePageStatus pageStatus = pageCache.lookupPage(requestId);

	// Fire the load request if necessary:
	if (pageStatus == CachePageStatus::Unavailable)
	{
		if (pageProvider.addPageRequest(requestId))
		{
			return 1; // One new request added.
		}

		// PageProvider couldn't fit another request.
		// Tell to cache to restore the reference count:
		pageCache.notifyDroppedRequest(requestId);

		// Dropped requests don't add to the success count.
		return 0;
	}

	return 1; // Already cached/in-flight. Counts as a successful request.
}

void PageResolver::addDefaultRequests()
{
	// For every texture, add the default page
	// request according to its mip-level count.
	for (auto vtTex : registeredTextures)
	{
		const unsigned int maxMip = vtTex->getNumLevels() - 1;
		const unsigned int texId  = vtTex->getTextureIndex();
		processPageRequest(makePageId(0, 0, maxMip, texId), *vtTex->getPageCache());
	}
}

void PageResolver::initFrameBuffer(const int w, const int h)
{
	assert(w > 0);
	assert(h > 0);

	// Save width/height:
	pageIdFboWidth  = w;
	pageIdFboHeight = h;

	// And GL viewport.
	// NOTE: Would it be safer/needed to do this inside beginPageIdPass()?
	glGetIntegerv(GL_VIEWPORT, originalViewport.data());
	vtLogComment("Saved original viewport: ("
		<< originalViewport[0] << ", " << originalViewport[1] << ", "
		<< originalViewport[2] << ", " << originalViewport[3] << ").");

	// Save old buffers so we can restore then later:
	glGetIntegerv(GL_FRAMEBUFFER_BINDING,  &originalFbo);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &originalRbo);
	vtLogComment("Saved original framebuffer  id #" << originalFbo);
	vtLogComment("Saved original renderbuffer id #" << originalRbo);

	// Allocate a color render target texture:
	fboColorTex = gl::create2DTexture(
		pageIdFboWidth,
		pageIdFboHeight,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		GL_CLAMP_TO_EDGE,
		GL_CLAMP_TO_EDGE,
		GL_NEAREST,
		GL_NEAREST,
		nullptr);

	if (!fboColorTex)
	{
		vtFatalError("Failed to allocate a color render-target texture for the feedback framebuffer!");
	}

	// Allocate the framebuffer GL object:
	pageIdFbo = gl::createFrameBuffer(
		pageIdFboWidth, pageIdFboHeight,
		/* defaultDepthBuffer   = */ true,
		/* defaultStencilBuffer = */ false);

	if (!pageIdFbo)
	{
		vtFatalError("Failed to create OpenGL framebuffer! Null id.");
	}

	// Attach the color render-target texture:
	gl::attachTextureToFrameBuffer(pageIdFbo, fboColorTex, 0, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);

	std::string errStr;
	if (!gl::validateFrameBuffer(pageIdFbo, &errStr))
	{
		vtFatalError("Failed to create OpenGL framebuffer: " << errStr);
	}

	gl::useRenderBuffer(originalRbo);
	gl::useFrameBuffer(originalFbo);

	feedbackBuffer.reset(new Pixel4b[pageIdFboWidth * pageIdFboHeight]);

	vtLogComment("PageResolver feedback framebuffer initialized! Size: "
			<< pageIdFboWidth << "x" << pageIdFboHeight << " pixels.");
}

} // namespace vt {}
