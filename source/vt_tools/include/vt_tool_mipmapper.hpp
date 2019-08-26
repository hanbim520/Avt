
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_mipmapper.hpp
// Author: Guilherme R. Lampert
// Created on: 24/10/14
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

#ifndef VT_TOOL_MIPMAPPER_HPP
#define VT_TOOL_MIPMAPPER_HPP

#include "vt_tool_float_image_buffer.hpp"
#include <memory>
#include <vector>

namespace vt
{
namespace tool
{

// ======================================================
// MipMapper:
// ======================================================

//
// Mip-map construction tool.
//
// Depending on the filtering used to generate the mipmaps
// and the initial image size, buildMipMapChain() can take
// a long time to complete. Therefore, this tool is usually
// not suited for real-time mipmaps construction.
//
// The original image stops being halved when any of its
// mip-levels reach 1 pixel in size.
//
class MipMapper final
{
public:

	// Constructors:
	MipMapper(const Image & img);                // Will deep copy the source
	MipMapper(const FloatImageBuffer & fImgBuf); // Will deep copy the source
	MipMapper(FloatImageBuffer && fImgBuf);

	// No copy or assignment.
	MipMapper(const MipMapper &) = delete;
	MipMapper & operator = (const MipMapper &) = delete;

	// Test if a chain of mipmaps was generated for the initial image.
	// Set after a call to buildMipMapChain(). Cleared by reset().
	bool isMipMapChainBuilt() const;

	// Generates a mipmap chain for the initial image provided at construction, using a user defined filter.
	void buildMipMapChain(const Filter & filter, FloatImageBuffer::WrapMode wm = FloatImageBuffer::Clamp);

	/// Get a read-only reference to one of the mipmap levels generated by a previous call to buildMipMapChain().
	const FloatImageBuffer * getMipMapLevel(size_t level) const;

	// Get a copy of one of the mipmap levels generated by a previous call to buildMipMapChain().
	bool getMipMapLevel(size_t level, FloatImageBuffer & destImage) const;

	// Number of mipmap levels generated. The original image counts as level 0.
	size_t getNumMipMapLevels() const;

	// Discards all previously generated mipmap levels.
	void reset();

private:

	// Array of mipmaps generated by a call to buildMipMapChain().
	// mipMaps[0] will always have a copy of the source image as a FloatImageBuffer.
	std::vector<std::unique_ptr<FloatImageBuffer>> mipMaps;

	// Set by buildMipMapChain(). Cleared by reset().
	bool mipMapsGenerated;
};

} // namespace tool {}
} // namespace vt {}

#endif // VT_TOOL_MIPMAPPER_HPP
