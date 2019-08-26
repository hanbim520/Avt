
// ================================================================================================
// -*- C++ -*-
// File: vt_builtin_text_atlas.cpp
// Author: Guilherme R. Lampert
// Created on: 26/10/14
// Brief: Texture atlas used by the built-in text renderer.
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
#include "vt_builtin_text.hpp"

#include <vector>
#include <limits>

namespace vt
{
namespace font
{
namespace {

// ======================================================
// Helper types:
// ======================================================

struct Vec3i
{
	int x, y, z;
};

struct Rect4i
{
	int x, y;
	int width, height;
};

// ======================================================
// TextureAtlas:
// ======================================================

//
// NOTE: Currently, this class is not cleaned up at any point.
// This is not a big concern at the moment since this is only
// used for debugging and development text printing.
//
struct TextureAtlas
{
	// System memory copy of the texture data:
	std::vector<Vec3i> nodes;          // "nodes" of the atlas (each sub-region)
	GLuint    textureId     = 0;       // OpenGL texture id
	uint32_t  usedPixels    = 0;       // Allocated surface size
	uint8_t * pixels        = nullptr; // Atlas system-side data
	bool      needUpdate    = false;   // Set when new data is added to the atlas. Cleared by updateTexture()
	bool      isInitialized = false;   // Simple initialization flag, to avoid duplicates

	// Texture size is fixed.
	static constexpr int width  = 1024;
	static constexpr int height = 512;

	// RGBA texture (4 Bpp).
	static constexpr int bytesPerPixel = 4;

	// Atlas management:
	void init();
	void reserveMemForRegions(int numRegions);
	void updateTexture(bool forceUpdate = false);

	// Region allocation:
	Rect4i allocRegion(int w, int h);
	void setRegion(int x, int y, int w, int h, const uint8_t * newData, int stride);

	// Internal helpers:
	int fit(int index, int w, int h);
	void merge();
};

// ======================================================

void TextureAtlas::init()
{
	assert(!isInitialized);

	glGenTextures(1, &textureId);

	// We want a one pixel border around the whole atlas
	// to avoid any artifacts when sampling the texture.
	nodes.push_back({ 1, 1, width - 2 });

	// Allocate a cleared image:
	pixels = new uint8_t[width * height * bytesPerPixel];
	clearArray(pixels, (width * height * bytesPerPixel));

	needUpdate    = true;
	isInitialized = true;
}

void TextureAtlas::reserveMemForRegions(const int numRegions)
{
	nodes.reserve(nodes.size() + numRegions);
}

void TextureAtlas::updateTexture(const bool forceUpdate)
{
	assert(isInitialized);
	assert(pixels != nullptr);

	if (needUpdate || forceUpdate)
	{
		vtLogComment("Uploading built-in text atlas to GL...");

		gl::use2DTexture(textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl::checkGLErrors(__FILE__, __LINE__);
		gl::use2DTexture(0);

		needUpdate = false;
	}
}

Rect4i TextureAtlas::allocRegion(const int w, const int h)
{
	int y, bestWidth, bestHeight, bestIndex;
	Vec3i * node, * prev;
	Rect4i region = { 0, 0, w, h };
	unsigned int i;

	bestWidth  = std::numeric_limits<int>::max();
	bestHeight = std::numeric_limits<int>::max();
	bestIndex  = -1;

	for (i = 0; i < nodes.size(); ++i)
	{
		y = fit(i, w, h);
		if (y >= 0)
		{
			node = &nodes[i];
			if (((y + h) < bestHeight) || (((y + h) == bestHeight) && (node->z < bestWidth)))
			{
				bestHeight = y + h;
				bestIndex  = i;
				bestWidth  = node->z;
				region.x   = node->x;
				region.y   = y;
			}
		}
	}

	if (bestIndex == -1) // No more room!
	{
		region.x      = -1;
		region.y      = -1;
		region.width  =  0;
		region.height =  0;
		return region;
	}

	Vec3i tempNode;
	node = &tempNode;
	node->x = region.x;
	node->y = region.y + h;
	node->z = w;
	nodes.insert(nodes.begin() + bestIndex, *node);

	for (i = bestIndex + 1; i < nodes.size(); ++i)
	{
		node = &nodes[i];
		prev = &nodes[i - 1];

		if (node->x < (prev->x + prev->z))
		{
			int shrink = (prev->x + prev->z - node->x);
			node->x += shrink;
			node->z -= shrink;

			if (node->z <= 0)
			{
				nodes.erase(nodes.begin() + i);
				--i;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	merge();
	usedPixels += (w * h);
	return region;
}

void TextureAtlas::setRegion(const int x, const int y, const int w, const int h, const uint8_t * newData, const int stride)
{
	// Validate the atlas object:
	assert(pixels != nullptr && "Atlas has no image data assigned to it!");

	// Validate input parameters:
	assert(x > 0 && y > 0);
	assert(x < (width - 1));
	assert((x + w) <= (width - 1));
	assert(y < (height - 1));
	assert((y + h) <= (height - 1));
	assert(newData != nullptr);

	// 'stride' is the length in bytes of a row of pixels.
	for (int i = 0; i < h; ++i)
	{
		memcpy(/* dest     = */ (pixels + ((y + i) * width + x) * bytesPerPixel),
		       /* source   = */ (newData + (i * stride)),
		       /* numBytes = */ (w * bytesPerPixel));
	}

	needUpdate = true;
}

int TextureAtlas::fit(const int index, const int w, const int h)
{
	assert(!nodes.empty());

	const Vec3i * node;
	int x, y, widthLeft;
	int i;

	node = &nodes[index];
	x = node->x;
	y = node->y;
	widthLeft = w;
	i = index;

	if ((x + w) > (width - 1))
	{
		return -1;
	}

	y = node->y;

	while (widthLeft > 0)
	{
		node = &nodes[i];

		if (node->y > y)
		{
			y = node->y;
		}
		if ((y + h) > (height - 1))
		{
			return -1;
		}

		widthLeft -= node->z;
		++i;
	}

	return y;
}

void TextureAtlas::merge()
{
	if (nodes.empty()) { return; }

	Vec3i * node, * next;
	for (size_t i = 0; i < nodes.size() - 1; ++i)
	{
		node = &nodes[i];
		next = &nodes[i + 1];
		if (node->y == next->y)
		{
			node->z += next->z;
			auto eraseIterator = nodes.begin() + (i + 1);
			assert(eraseIterator != nodes.end());
			nodes.erase(eraseIterator);
			--i;

			if (nodes.empty()) { break; }
		}
	}
}

// Local atlas used by the built-in font glyphs:
static TextureAtlas atlas;

} // namespace {}

// ======================================================
// ensureTextAtlasCreated():
// ======================================================

void ensureTextAtlasCreated()
{
	if (!atlas.isInitialized)
	{
		atlas.init();
	}
}

// ======================================================
// allocateTextAtlasSpace():
// ======================================================

bool allocateTextAtlasSpace(BuiltInFontBitmap & font)
{
	assert(atlas.isInitialized);
	assert(font.isLoaded());

	// Reserve memory for every glyph, since we know
	// the amount beforehand:
	atlas.reserveMemForRegions(font.numGlyphs + 1);

	const float atlasW = static_cast<float>(atlas.width);
	const float atlasH = static_cast<float>(atlas.height);

	for (int i = 0; i < font.numGlyphs; ++i)
	{
		BuiltInFontGlyph & glyph = font.glyphs[i];
		if (glyph.pixels == nullptr)
		{
			continue;
		}

		// Alloc atlas space (+1 pixel each side for a safety border when sampling):
		const Rect4i region = atlas.allocRegion(glyph.w + 1, glyph.h + 1);
		if (region.x < 0)
		{
			vtLogError("The default text atlas seems to be full! Discarding further text glyphs...");
			return false;
		}

		// Fill it:
		atlas.setRegion(region.x, region.y, glyph.w, glyph.h,
			reinterpret_cast<const uint8_t *>(glyph.pixels), glyph.w * sizeof(uint32_t));

		// Store UVs for this glyph:
		glyph.u0 = region.x / atlasW;
		glyph.v0 = region.y / atlasH;
		glyph.u1 = (region.x + glyph.w) / atlasW;
		glyph.v1 = (region.y + glyph.h) / atlasH;
	}

	return true;
}

// ======================================================
// bindTextAtlasTexture():
// ======================================================

void bindTextAtlasTexture()
{
	if (atlas.needUpdate)
	{
		atlas.updateTexture();
	}

	// Bind the underlaying texture for rendering.
	// Use texture unit 0.
	gl::use2DTexture(atlas.textureId, 0);
}

} // namespace font {}
} // namespace vt {}
