
// ================================================================================================
// -*- C++ -*-
// File: vt_page_table.cpp
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

#include "vt.hpp"
#include "vt_tool_image.hpp"
#include <vector>

namespace vt
{

// ======================================================
// PageTable:
// ======================================================

Pixel4b PageTable::halfPageData[PageTable::HalfPageSizeInPixels * PageTable::HalfPageSizeInPixels];

PageTable::PageTable()
	: pageTextureId(0)
	, pageTexMinFilter(GL_LINEAR_MIPMAP_LINEAR)
	, pageTexMagFilter(GL_LINEAR)
{
	initTexture();
	vtLogComment("New PageTable instance created...");
}

PageTable::~PageTable()
{
	gl::delete2DTexture(pageTextureId);
}

void PageTable::bind(const int texUnit) const
{
	gl::use2DTexture(pageTextureId, texUnit);
}

void PageTable::visualizePageTableTexture(const float overlayScale[2]) const
{
	gl::useShaderProgram(getGlobalShaders().drawPageTable.programId);
	gl::setShaderProgramUniform(getGlobalShaders().drawPageTable.unifNdcQuadScale, overlayScale, 2);

	// Draw a quad with the texture applied to it:
	gl::use2DTexture(pageTextureId);
	gl::drawNdcQuadrilateral();
	gl::use2DTexture(0);

	gl::useShaderProgram(0);
}

void PageTable::uploadPage(const PageUpload & upload)
{
	assert(upload.cacheCoord.x < TableSizeInPages);
	assert(upload.cacheCoord.y < TableSizeInPages);
	assert(upload.pageData != nullptr);

	// Texture must be already bound!
	// GL_UNPACK_ALIGNMENT should ideally be set to 4.
	assert(pageTextureId == gl::getCurrent2DTexture());

	// mip-level 0:
	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		(upload.cacheCoord.x * PageSizeInPixels),
		(upload.cacheCoord.y * PageSizeInPixels),
		PageSizeInPixels,
		PageSizeInPixels,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		upload.pageData);

	#if VT_EXTRA_GL_ERROR_CHECKING
	gl::checkGLErrors(__FILE__, __LINE__);
	#endif // VT_EXTRA_GL_ERROR_CHECKING

	// Dowsample for mip-level 1 (half the original size):
	int nw = PageSizeInPixels;
	int nh = PageSizeInPixels;

	halveImageBoxFilter(reinterpret_cast<const uint8_t *>(upload.pageData),
		reinterpret_cast<uint8_t *>(halfPageData), nw, nh, 4 /* RGBA */);

	assert(nw == HalfPageSizeInPixels);
	assert(nh == HalfPageSizeInPixels);

	// Send downsampled page to mip 1:
	glTexSubImage2D(
		GL_TEXTURE_2D,
		1,
		(upload.cacheCoord.x * HalfPageSizeInPixels),
		(upload.cacheCoord.y * HalfPageSizeInPixels),
		HalfPageSizeInPixels,
		HalfPageSizeInPixels,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		halfPageData);

	#if VT_EXTRA_GL_ERROR_CHECKING
	gl::checkGLErrors(__FILE__, __LINE__);
	#endif // VT_EXTRA_GL_ERROR_CHECKING
}

void PageTable::initTexture()
{
	assert(pageTextureId == 0 && "Duplicate initialization!");

	glGenTextures(1, &pageTextureId);
	if (pageTextureId == 0)
	{
		vtFatalError("Failed to generate a non-zero GL texture id for the page table texture!");
	}

	gl::use2DTexture(pageTextureId);

	// Allocate two levels:
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		TableSizeInPixels,
		TableSizeInPixels,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr);

	#if VT_EXTRA_GL_ERROR_CHECKING
	gl::checkGLErrors(__FILE__, __LINE__);
	#endif // VT_EXTRA_GL_ERROR_CHECKING

	// Second one is half the size of the previous:
	glTexImage2D(
		GL_TEXTURE_2D,
		1,
		GL_RGBA,
		(TableSizeInPixels / 2),
		(TableSizeInPixels / 2),
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr);

	#if VT_EXTRA_GL_ERROR_CHECKING
	gl::checkGLErrors(__FILE__, __LINE__);
	#endif // VT_EXTRA_GL_ERROR_CHECKING

	// iOS specific: Set max level (would probably have to use glGenerateMipmap() otherwise...)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_APPLE, 1);

	// Set addressing mode:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Set filtering:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pageTexMinFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, pageTexMagFilter);

	// Optional data initialization, used for debugging:
	#if VT_EXTRA_DEBUG
	fillTextureWithDebugData();
	#endif // VT_EXTRA_DEBUG

	gl::use2DTexture(0);
	gl::checkGLErrors(__FILE__, __LINE__);

	vtLogComment("Page cache table texture #" << pageTextureId << " created. Tex size: "
			<< TableSizeInPixels << "x" << TableSizeInPixels << " pixels.");
}

void PageTable::fillTextureWithDebugData()
{
	std::vector<Pixel4b> page(PageSizeInPixels * PageSizeInPixels);

	// Fill the page with a red gradient, making borders easy to spot:
	for (int y = 0; y < PageSizeInPixels; ++y)
	{
		for (int x = 0; x < PageSizeInPixels; ++x)
		{
			Pixel4b & pixel = page[x + y * PageSizeInPixels];
			pixel.r = static_cast<uint8_t>(std::min(x, 127) + std::min(y, 127));
			pixel.g = 10;
			pixel.b = 10;
			pixel.a = 0xFF;
		}
	}

	PageUpload upload;
	upload.pageData = page.data();
	for (int y = 0; y < TableSizeInPages; ++y)
	{
		for (int x = 0; x < TableSizeInPages; ++x)
		{
			upload.cacheCoord.x = static_cast<CachePageCoord::IndexType>(x);
			upload.cacheCoord.y = static_cast<CachePageCoord::IndexType>(y);
			uploadPage(upload);
		}
	}
}

void PageTable::halveImageBoxFilter(const uint8_t * __restrict src, uint8_t * __restrict dest,
                                    int & width, int & height, const int components)
{
	if ((width <= 1) || (height <= 1))
	{
		return;
	}

	// Calculate new width and height:
	const int halfWidth  = (width  / 2);
	const int halfHeight = (height / 2);

	// Downsample the image using a fast box-filter:
	const int idx1 = (width * components);
	const int idx2 = (width + 1) * components;
	for (int m = 0; m < halfHeight; ++m)
	{
		for (int n = 0; n < halfWidth; ++n)
		{
			for (int k = 0; k < components; ++k)
			{
				*dest++ = (uint8_t)(((int)*src +
				                     (int)src[components] +
				                     (int)src[idx1] +
				                     (int)src[idx2] + 2) >> 2);
				src++;
			}
			src += components;
		}
		src += (components * width);
	}

	// Return new width and height:
	width  = halfWidth;
	height = halfHeight;
}

bool PageTable::writePageTableTextureToFile(const std::string & pathname) const
{
	//
	// Amazingly complicated workaround to compensate for the
	// lack of glGetTexImage() on GLES 2.0:
	//
	// Create a framebuffer object, attach the texture to it,
	// read-back from the framebuffer, write the image, free the FBO.
	//
	// This will be very slow, but should be OK, since it is only
	// intended for debugging.
	//
	GLuint fbo = gl::createFrameBuffer(TableSizeInPixels, TableSizeInPixels, false, false);
	gl::attachTextureToFrameBuffer(fbo, pageTextureId, 0, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);
	assert(gl::validateFrameBuffer(fbo, nullptr) && "Bad framebuffer object!");

	std::vector<uint8_t> image(TableSizeInPixels * TableSizeInPixels * 4); // RGBA
	gl::readFrameBuffer(fbo, 0, 0, TableSizeInPixels, TableSizeInPixels, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

	const bool result = tool::writeTgaImage(pathname + ".tga", TableSizeInPixels, TableSizeInPixels, 4, image.data(), true);

	gl::deleteFrameBuffer(fbo);
	return result;
}

void PageTable::setGLTextureFilter(const GLenum minFilter, const GLenum magFilter)
{
	assert(magFilter == GL_NEAREST || magFilter == GL_LINEAR);

	pageTexMinFilter = minFilter;
	pageTexMagFilter = magFilter;

	gl::use2DTexture(pageTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pageTexMinFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, pageTexMagFilter);

	#if VT_EXTRA_GL_ERROR_CHECKING
	gl::checkGLErrors(__FILE__, __LINE__);
	#endif // VT_EXTRA_GL_ERROR_CHECKING

	gl::use2DTexture(0);
}

std::tuple<GLenum, GLenum> PageTable::getGLTextureFilter() const
{
	return std::make_tuple(pageTexMinFilter, pageTexMagFilter);
}

} // namespace vt {}
