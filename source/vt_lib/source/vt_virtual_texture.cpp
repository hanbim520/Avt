
// ================================================================================================
// -*- C++ -*-
// File: vt_virtual_texture.cpp
// Author: Guilherme R. Lampert
// Created on: 03/11/14
// Brief: Virtual Texture type.
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
#include "vt_mini_ui.hpp"
#include "vt_tool_platform_utils.hpp"

namespace vt
{

// ======================================================
// VirtualTexture:
// ======================================================

VirtualTexture::VirtualTexture(VTFFPageFilePtr vtffFile, PageIndirectionTablePtr pageIndirection)
	: VirtualTexture(vtffFile->getPageTree().getNumPagesX(), vtffFile->getPageTree().getNumPagesY(),
	                 vtffFile->getPageTree().getNumLevels(), std::move(vtffFile), pageIndirection)
{
}

VirtualTexture::VirtualTexture(VTFFPageFilePtr * vtffFiles, const size_t numFiles, PageIndirectionTablePtr pageIndirection)
	: indirectionTable(pageIndirection)
	, pageProvider(nullptr)
	, pageResolver(nullptr)
	, textureIndex(-1)
	, numPageUploads(0)
	, numIndirectionTableUpdates(0)
{
	assert(vtffFiles != nullptr);
	assert(numFiles  != 0);

	// All files are assumed to point to textures with
	// equal dimensions. We assert that below.
	const int * const vtPagesX = vtffFiles[0]->getPageTree().getNumPagesX();
	const int * const vtPagesY = vtffFiles[0]->getPageTree().getNumPagesY();
	numLevels = vtffFiles[0]->getPageTree().getNumLevels();
	assert(numLevels > 0 && numLevels <= MaxVTMipLevels);

	pageFiles.reserve(numFiles);
	pageTables.reserve(numFiles);
	for (size_t f = 0; f < numFiles; ++f)
	{
		assert(vtffFiles[f]->getPageTree().getNumPagesX()[f] == vtPagesX[f]);
		assert(vtffFiles[f]->getPageTree().getNumPagesY()[f] == vtPagesY[f]);
		assert(vtffFiles[f]->getPageTree().getNumLevels() == numLevels);

		pageFiles.push_back(std::move(vtffFiles[f]));

		// Create a page table texture to back each page file.
		pageTables.push_back(PageTablePtr(new PageTable()));
	}

	// Optional. Supply if missing.
	if (indirectionTable == nullptr)
	{
		indirectionTable = createIndirectionTable(vtPagesX, vtPagesY, numLevels);
	}

	pageCacheMgr.reset(new PageCacheMgr(vtPagesX, vtPagesY, numLevels));

	level0SizePixels[0] = static_cast<float>(vtPagesX[0] * PageTable::PageSizeInPixels);
	level0SizePixels[1] = static_cast<float>(vtPagesY[0] * PageTable::PageSizeInPixels);

	level0SizePages[0]  = static_cast<float>(vtPagesX[0]);
	level0SizePages[1]  = static_cast<float>(vtPagesY[0]);
}

VirtualTexture::VirtualTexture(const int * vtPagesX, const int * vtPagesY, const int vtNumLevels,
                               PageFilePtr pageFile, PageIndirectionTablePtr pageIndirection)
	: indirectionTable(pageIndirection)
	, pageProvider(nullptr)
	, pageResolver(nullptr)
	, textureIndex(-1)
	, numPageUploads(0)
	, numIndirectionTableUpdates(0)
{
	assert(pageFile != nullptr);
	assert(vtNumLevels > 0 && vtNumLevels <= MaxVTMipLevels);

	pageFiles.push_back(std::move(pageFile));
	pageTables.push_back(PageTablePtr(new PageTable()));
	numLevels = vtNumLevels;

	// Optional. Supply if missing.
	if (indirectionTable == nullptr)
	{
		indirectionTable = createIndirectionTable(vtPagesX, vtPagesY, numLevels);
	}

	pageCacheMgr.reset(new PageCacheMgr(vtPagesX, vtPagesY, numLevels));

	level0SizePixels[0] = static_cast<float>(vtPagesX[0] * PageTable::PageSizeInPixels);
	level0SizePixels[1] = static_cast<float>(vtPagesY[0] * PageTable::PageSizeInPixels);

	level0SizePages[0]  = static_cast<float>(vtPagesX[0]);
	level0SizePages[1]  = static_cast<float>(vtPagesY[0]);
}

void VirtualTexture::frameUpdate(const FulfilledPageRequestQueue & pageRequestUploads, const bool updateIndirectionTable)
{
	assert(pageProvider != nullptr && "No PageProvider associated with this VirtualTexture!");
	assert(pageResolver != nullptr && "No PageResolver associated with this VirtualTexture!");
	assert(textureIndex >= 0 && "Bad VT texture index!");
	assert(pageFiles.size() == pageTables.size());

	if (pageRequestUploads.empty())
	{
		// No fulfilled requests ready for upload this frame.
		return;
	}

	// Upload new pages to the page table texture(s):
	const size_t numPageTables = pageTables.size();
	PageTable * currentPageTexture = nullptr;

	for (const auto & request : pageRequestUploads)
	{
		if ((pageIdExtractTextureIndex(request.pageId) != textureIndex) || (request.fileId != 0)
			|| !pageCacheMgr->stillWantPage(request.pageId))
		{
			continue;
		}

		PageUpload mainUpload;
		mainUpload.pageData   = request.pageData;
		mainUpload.cacheCoord = pageCacheMgr->accommodatePage(request.pageId);

		if (currentPageTexture != pageTables[0].get())
		{
			currentPageTexture = pageTables[0].get();
			currentPageTexture->bind();
		}
		pageTables[0]->uploadPage(mainUpload);
		++numPageUploads;

		// If this texture has a hierarchy or sub-texture (normal map,
		// specular map, etc) update theses child page tables now:
		for (size_t t = 1; t < numPageTables; ++t)
		{
			for (const auto & subRequest : pageRequestUploads)
			{
				if ((subRequest.pageId != request.pageId) || (subRequest.fileId != t))
				{
					continue;
				}

				PageUpload subUpload;
				subUpload.pageData   = subRequest.pageData;
				subUpload.cacheCoord = mainUpload.cacheCoord;

				if (currentPageTexture != pageTables[t].get())
				{
					currentPageTexture = pageTables[t].get();
					currentPageTexture->bind();
				}
				pageTables[t]->uploadPage(subUpload);
				++numPageUploads;
			}
		}
	}

	if (updateIndirectionTable)
	{
		indirectionTable->bind();
		indirectionTable->updateIndirectionTexture(pageCacheMgr->getCacheEntries());
		++numIndirectionTableUpdates;
	}
}

void VirtualTexture::drawStats() const
{
	assert(pageProvider != nullptr && pageResolver != nullptr);

	const double secondsSinceStartup  = tool::getClockMillisec() * 0.001;
	const double indrTblUpdatesPerSec = numIndirectionTableUpdates / secondsSinceStartup;
	const double pageUploadsPerSec    = numPageUploads / secondsSinceStartup;

	char headerStr[512];
	std::snprintf(headerStr, sizeof(headerStr), "VT #%d stats ( %dx%dpx )",
			textureIndex, static_cast<int>(level0SizePixels[0]), static_cast<int>(level0SizePixels[1]));

	ui::drawVTStatsPanel(headerStr, *pageCacheMgr, *pageProvider, *pageResolver,
			indrTblUpdatesPerSec, pageUploadsPerSec, numIndirectionTableUpdates, numPageUploads);
}

void VirtualTexture::clearStats()
{
	pageCacheMgr->clearCacheStats();
}

void VirtualTexture::purgeCache()
{
	vtLogComment("Purging VT cache for texture #" << textureIndex);

	pageCacheMgr->purgeCache();

	// Optionally fill the page tables with dummy data:
	#if VT_EXTRA_DEBUG
	for (auto & pageTable : pageTables)
	{
		pageTable->bind();
		pageTable->fillTextureWithDebugData();
		numPageUploads += PageTable::TotalTablePages;
	}
	#endif // VT_EXTRA_DEBUG

	// Reset the indirection texture as well:
	indirectionTable->bind();
	indirectionTable->updateIndirectionTexture(pageCacheMgr->getCacheEntries());
	++numIndirectionTableUpdates;
}

void VirtualTexture::replacePageFile(PageFilePtr & newPageFile, unsigned int index)
{
	std::swap(pageFiles[index], newPageFile);
}

// ======================================================
// Rendering with the VT:
// ======================================================

static GLuint currentShader;

void renderBindPageIdPassShader()
{
	currentShader = getGlobalShaders().pageIdGenPass.programId;
	gl::useShaderProgram(currentShader);
}

void renderBindTextureForPageIdPass(const VirtualTexture & vtTex)
{
	assert(vtTex.getNumLevels()    >= 1);
	assert(vtTex.getTextureIndex() >= 0);

	const GlobalShaders & globShaders = getGlobalShaders();
	gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTMaxMipLevel, static_cast<float>(vtTex.getNumLevels() - 1));
	gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTIndex,       static_cast<float>(vtTex.getTextureIndex()));
	gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTSizePixels,  vtTex.getLevel0SizeInPixels(), 2);
	gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTSizePages,   vtTex.getLevel0SizeInPages(),  2);
}

void renderBindTexturedPassShader(const bool simpleRender)
{
	currentShader = simpleRender ? getGlobalShaders().vtRenderSimple.programId : getGlobalShaders().vtRenderLit.programId;
	gl::useShaderProgram(currentShader);
}

void renderBindTextureForTexturedPass(const VirtualTexture & vtTex)
{
	assert(vtTex.getNumPageFiles() == vtTex.getNumPageTables());

	if (vtTex.getNumPageFiles() == 1)
	{
		// Page table sampler at TMU 0
		vtTex.getPageTable()->bind(0);

		// Indirection sampler at TMU 1
		vtTex.getPageIndirectionTable()->bind(1);
	}
	else
	{
		// Multi-textured object being rendered.
		vtTex.getPageIndirectionTable()->bind(0);

		const unsigned int numTextures = vtTex.getNumPageFiles();
		for (unsigned int t = 0; t < numTextures; ++t)
		{
			vtTex.getPageTable(t)->bind(t + 1);
		}
	}
}

void renderSetMvpMatrix(const float * const mvpMatrix)
{
	const GlobalShaders & globShaders = getGlobalShaders();

	if (currentShader == globShaders.pageIdGenPass.programId)
	{
		gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifMvpMatrix, mvpMatrix, 16);
	}
	else
	{
		if (currentShader == globShaders.vtRenderSimple.programId)
		{
			gl::setShaderProgramUniform(globShaders.vtRenderSimple.unifMvpMatrix, mvpMatrix, 16);
		}
		else if (currentShader == globShaders.vtRenderLit.programId)
		{
			gl::setShaderProgramUniform(globShaders.vtRenderLit.unifMvpMatrix, mvpMatrix, 16);
		}
		else
		{
			vtFatalError("currentShader is invalid!");
		}
	}
}

void renderSetMipDebugBias(const float mipDebugBias)
{
	// Mip sample bias:
	const float pageSizeLog2  = std::log2(PageTable::PageSizeInPixels);
	const float mipSampleBias = pageSizeLog2 - 0.5f + mipDebugBias;

	const GlobalShaders & globShaders = getGlobalShaders();

	if (currentShader == globShaders.vtRenderSimple.programId)
	{
		gl::setShaderProgramUniform(globShaders.vtRenderSimple.unifMipSampleBias, mipSampleBias);
	}
	else if (currentShader == globShaders.vtRenderLit.programId)
	{
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifMipSampleBias, mipSampleBias);
	}
	else
	{
		vtFatalError("currentShader is invalid!");
	}
}

void renderSetLog2MipScaleFactor(const float log2MipScaleFactor)
{
	// This is a pageIdGenPass param.
	assert(currentShader == getGlobalShaders().pageIdGenPass.programId);
	gl::setShaderProgramUniform(getGlobalShaders().pageIdGenPass.unifLog2MipScaleFactor, log2MipScaleFactor);
}

void renderSetLightPosObjectSpace(const float pos[4])
{
	// This is a vtRenderLit param.
	assert(currentShader == getGlobalShaders().vtRenderLit.programId);
	gl::setShaderProgramUniform(getGlobalShaders().vtRenderLit.unifLightPosObjectSpace, pos, 4);
}

void renderSetViewPosObjectSpace(const float pos[4])
{
	// This is a vtRenderLit param.
	assert(currentShader == getGlobalShaders().vtRenderLit.programId);
	gl::setShaderProgramUniform(getGlobalShaders().vtRenderLit.unifViewPosObjectSpace, pos, 4);
}

} // namespace vt {}
