
// ================================================================================================
// -*- C++ -*-
// File: MipMapper.cpp
// Author: Guilherme R. Lampert
// Created on: 07/05/14
// Brief: Mip-map construction tool.
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

#include "vt_tool_mipmapper.hpp"
#include <algorithm>
#include <utility>
#include <cassert>

namespace vt
{
namespace tool
{

// ======================================================
// MipMapper:
// ======================================================

MipMapper::MipMapper(const Image & img)
	: mipMapsGenerated(false)
{
	mipMaps.emplace_back(new FloatImageBuffer(img));
}

MipMapper::MipMapper(const FloatImageBuffer & fImgBuf)
	: mipMapsGenerated(false)
{
	mipMaps.emplace_back(new FloatImageBuffer(fImgBuf));
}

MipMapper::MipMapper(FloatImageBuffer && fImgBuf)
	: mipMapsGenerated(false)
{
	mipMaps.emplace_back(new FloatImageBuffer(std::forward<FloatImageBuffer>(fImgBuf)));
}

bool MipMapper::isMipMapChainBuilt() const
{
	return mipMapsGenerated;
}

void MipMapper::buildMipMapChain(const Filter & filter, const FloatImageBuffer::WrapMode wm)
{
	assert(!mipMapsGenerated && "mip-map chain already generated! Call reset() before generating again.");
	assert(!mipMaps.empty());

	const FloatImageBuffer * initialImage = mipMaps[0].get();
	uint32_t targetWidth  = initialImage->getWidth();
	uint32_t targetHeight = initialImage->getHeight();

	// Stop when any of the dimensions reach 1:
	while ((targetWidth > 1) && (targetHeight > 1))
	{
		targetWidth  = std::max(uint32_t(1), (targetWidth  / 2));
		targetHeight = std::max(uint32_t(1), (targetHeight / 2));

		FloatImageBuffer * mip = new FloatImageBuffer();
		initialImage->resize(*mip, filter, targetWidth, targetHeight, wm);

		mipMaps.emplace_back(mip);
	}

	mipMapsGenerated = true;
}

const FloatImageBuffer * MipMapper::getMipMapLevel(const size_t level) const
{
	if (!mipMapsGenerated || (level >= mipMaps.size()))
	{
		return nullptr;
	}

	return mipMaps[level].get();
}

bool MipMapper::getMipMapLevel(const size_t level, FloatImageBuffer & destImage) const
{
	if (!mipMapsGenerated || (level >= mipMaps.size()))
	{
		return false;
	}

	destImage = *mipMaps[level];
	return true;
}

size_t MipMapper::getNumMipMapLevels() const
{
	return mipMaps.size();
}

void MipMapper::reset()
{
	mipMaps.clear();
	mipMapsGenerated = false;
}

} // namespace tool {}
} // namespace vt {}
