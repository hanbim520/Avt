
// ================================================================================================
// -*- C++ -*-
// File: vt_builtin_text.cpp
// Author: Guilherme R. Lampert
// Created on: 26/10/14
// Brief: Helper functions for debug text printing to the screen.
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

#include <cctype>
#include <cstdio>

namespace vt
{
namespace font
{
namespace {

// ======================================================
// Built-in fonts of several sizes and families:
// ======================================================

// "Consolas" fonts (see builtin_fonts/ dir):
#include "builtin_fonts/consolas10.hpp"
#include "builtin_fonts/consolas12.hpp"
#include "builtin_fonts/consolas16.hpp"
#include "builtin_fonts/consolas20.hpp"
#include "builtin_fonts/consolas24.hpp"
#include "builtin_fonts/consolas30.hpp"
#include "builtin_fonts/consolas36.hpp"
#include "builtin_fonts/consolas40.hpp"
#include "builtin_fonts/consolas48.hpp"

// All built-in fonts must be registered here,
// in the same order they appear in the BuiltInFontId enum!
static const uint32_t * const compressedBuiltInFonts[] = {
	font_10Con,
	font_12Con,
	font_16Con,
	font_20Con,
	font_24Con,
	font_30Con,
	font_36Con,
	font_40Con,
	font_48Con
};
static_assert(arrayLength(compressedBuiltInFonts) == NumBuiltInFonts, "Keep this array in sync with BuiltInFontId enum!");

// ======================================================
// decompressBuiltInFont():
// ======================================================

static void decompressBuiltInFont(const uint32_t * source, BuiltInFontBitmap * font)
{
	//
	// Based on code found at: http://silverspaceship.com/inner/imgui/grlib/gr_extra.c
	// Originally written by Sean Barrett.
	//

#define skipBit()\
	if ((buffer >>= 1), (--bitsLeft == 0)) { buffer = *source++; bitsLeft = 32; }

#define makeRGBA(r, g, b, a)\
	(uint32_t)((((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((r) & 0xFF))

	assert(source != nullptr && font != nullptr);
	assert(font->bitmap == nullptr && "Built-in bitmap font already allocated!");

	int32_t i, j;
	int32_t effectiveCount;
	int32_t bitsLeft;
	int32_t buffer;

	const int32_t start = (source[0] >>  0) & 0xFF;
	const int32_t count = (source[0] >>  8) & 0xFF;
	const int32_t h     = (source[0] >> 16) & 0xFF;

	BuiltInFontGlyph * glyphs = &font->glyphs[0];
	int32_t numGlyphs = font->numGlyphs;

	if (count > numGlyphs)
	{
		effectiveCount = numGlyphs;
	}
	else
	{
		effectiveCount = count;
	}

	font->charStart = start;
	font->bmpHeight = h; // Height is the same for every glyph
	font->bmpWidth  = 0;
	++source;

	// Count bitmap size:
	for (i = 0; i < effectiveCount; ++i)
	{
		int32_t w = (source[i >> 2] >> ((i & 3) << 3)) & 0xFF;
		font->bmpWidth += w;
	}

	// Allocate memory:
	font->bitmap = new uint32_t[font->bmpWidth * font->bmpHeight];

	// Set glyphs:
	uint32_t * pBitmap = font->bitmap;
	for (i = 0; i < effectiveCount; ++i)
	{
		int32_t w = (source[i >> 2] >> ((i & 3) << 3)) & 0xFF;
		glyphs[i].w = w;
		glyphs[i].h = h;
		glyphs[i].pixels = pBitmap;
		pBitmap += (w * h);
	}

	source += (count + 3) >> 2;
	buffer = *source++;
	bitsLeft = 32;

	while (--numGlyphs >= 0)
	{
		uint32_t * c = glyphs->pixels;
		for (j = 0; j < glyphs->h; ++j)
		{
			int32_t z = (buffer & 1);
			skipBit();

			if (!z)
			{
				for (i = 0; i < glyphs->w; ++i)
				{
					*c++ = makeRGBA(255, 255, 255, 0);
				}
			}
			else
			{
				for (i = 0; i < glyphs->w; ++i)
				{
					z = (buffer & 1);
					skipBit();

					if (!z)
					{
						*c++ = makeRGBA(255, 255, 255, 0);
					}
					else
					{
						int32_t n = 0;
						n += n + (buffer & 1); skipBit();
						n += n + (buffer & 1); skipBit();
						n += n + (buffer & 1); skipBit();
						n += 1;
						*c++ = makeRGBA(255, 255, 255, ((255 * n) >> 3));
					}
				}
			}
		}
		++glyphs;
	}

	font->numGlyphs = count;

#undef makeRGBA
#undef skipBit
}

} // namespace {}

// ======================================================
// BuiltInFontBitmap:
// ======================================================

BuiltInFontBitmap::BuiltInFontBitmap()
	: numGlyphs(0)
	, charStart(0)
	, bmpWidth(0)
	, bmpHeight(0)
	, bitmap(nullptr)
{
	clearArray(glyphs);
}

void BuiltInFontBitmap::load(const uint32_t * compressedData)
{
	assert(!isLoaded());

	// Expand data and allocate the bitmap:
	numGlyphs = MaxGlyphsPerBuiltInFont;
	decompressBuiltInFont(compressedData, this);
}

void BuiltInFontBitmap::unload()
{
	if (!isLoaded())
	{
		return;
	}

	delete[] bitmap;

	numGlyphs = 0;
	charStart = 0;
	bmpWidth  = 0;
	bmpHeight = 0;
	bitmap    = nullptr;

	clearArray(glyphs);
}

bool BuiltInFontBitmap::isLoaded() const
{
	return bitmap != nullptr;
}

// ======================================================
// Local data / helpers:
// ======================================================

// Texture atlas management:
extern void ensureTextAtlasCreated();
extern bool allocateTextAtlasSpace(BuiltInFontBitmap & font);
extern void bindTextAtlasTexture();

namespace {

struct SpriteVertex
{
	float x, y, u, v;
	float r, g, b, a;
};

// Batched geometry:
static std::vector<SpriteVertex> vertexBatch;
static std::vector<uint16_t>     indexBatch;

// GL buffers for text drawing:
static GLuint textVbo;
static GLuint textIbo;
static bool textBuffersInitialized;

// Null/empty glyph. Used as a safe return value for getBuiltInFontGlyph().
static const BuiltInFontGlyph nullGlyph = { 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, nullptr };

// Expanded fonts, loaded on demand.
static std::array<BuiltInFontBitmap, NumBuiltInFonts> builtInFonts;

// Will scale width and height of addGlyphQuadrilateral().
static float globalTextScale = 1.0f;

// ======================================================

static int getWhiteSpaceWidth(const BuiltInFontId fontId)
{
	// Get with in pixels of a white space.
	// Index zero should always be the white space.
	return builtInFonts[fontId].glyphs[0].w;
}

static void ensureTextVboAndIboCreated()
{
	// One time initialization.
	if (textBuffersInitialized)
	{
		return;
	}

	glGenBuffers(1, &textVbo);
	glGenBuffers(1, &textIbo);
	textBuffersInitialized = true;

	vtLogComment("Created VBO/IBO for debug text drawing...");
}

static void addGlyphQuadrilateral(const float x, const float y, const float w, const float h, const float color[4], const float uvs[][2])
{
	assert(w > 0.0f);
	assert(h > 0.0f);

	const uint16_t indexes[] = { 0, 1, 2, 2, 3, 0 };
	SpriteVertex verts[4];

	// Store scaled position:
	verts[0].x = x;     verts[0].y = y;
	verts[1].x = x + w; verts[1].y = y;
	verts[2].x = x + w; verts[2].y = y + h;
	verts[3].x = x;     verts[3].y = y + h;

	// And texture coords:
	verts[0].u = uvs[0][0]; verts[0].v = uvs[0][1];
	verts[1].u = uvs[1][0]; verts[1].v = uvs[1][1];
	verts[2].u = uvs[2][0]; verts[2].v = uvs[2][1];
	verts[3].u = uvs[3][0]; verts[3].v = uvs[3][1];

	// Store vertex color:
	verts[0].r = color[0]; verts[0].g = color[1]; verts[0].b = color[2]; verts[0].a = color[3];
	verts[1].r = color[0]; verts[1].g = color[1]; verts[1].b = color[2]; verts[1].a = color[3];
	verts[2].r = color[0]; verts[2].g = color[1]; verts[2].b = color[2]; verts[2].a = color[3];
	verts[3].r = color[0]; verts[3].g = color[1]; verts[3].b = color[2]; verts[3].a = color[3];

	// Add indexes offseting the base vertex:
	const uint16_t baseVertex = static_cast<uint16_t>(vertexBatch.size());
	for (size_t i = 0; i < arrayLength(indexes); ++i)
	{
		indexBatch.push_back(indexes[i] + baseVertex);
	}

	// Add vertexes to triangle batch:
	vertexBatch.insert(vertexBatch.end(), verts, verts + arrayLength(verts));
}

} // namespace {}

// ======================================================
// loadBuiltInFont():
// ======================================================

bool loadBuiltInFont(const BuiltInFontId fontId)
{
	if (builtInFonts[fontId].isLoaded())
	{
		vtLogWarning("Built-in bitmap font #" << static_cast<int>(fontId)
				<< " already loaded!");
		return true;
	}

	// First of all, ensure that other resources needed
	// for the text rendering are also initialized.
	// These functions are no-ops if the resources are already available.
	ensureTextAtlasCreated();
	ensureTextVboAndIboCreated();

	// Load & decompress the glyphs then copy bitmaps to the atlas:
	builtInFonts[fontId].load(compressedBuiltInFonts[fontId]);
	if (allocateTextAtlasSpace(builtInFonts[fontId]))
	{
		vtLogComment("Successfully loaded built-in font #" << static_cast<int>(fontId));
		return true;
	}

	vtLogWarning("Some glyphs from built-in font #" << static_cast<int>(fontId)
			<< " didn't fit in the default text atlas!");
	return false;
}

// ======================================================
// unloadBuiltInFont():
// ======================================================

void unloadBuiltInFont(const BuiltInFontId fontId)
{
	//
	// Note that this will free system memory associated with the font
	// but won't deallocate the glyphs from the texture atlas.
	// If the atlas is eventually unloaded and later restored,
	// then the new atlas wont have the unloaded font's glyphs.
	//
	builtInFonts[fontId].unload();
}

// ======================================================
// loadAllBuiltInFonts():
// ======================================================

void loadAllBuiltInFonts()
{
	for (int i = 0; i < NumBuiltInFonts; ++i)
	{
		if (!loadBuiltInFont(static_cast<BuiltInFontId>(i)))
		{
			vtLogError("Failed to load built-in font #" << i);
		}
	}
}

// ======================================================
// unloadAllBuiltInFonts():
// ======================================================

void unloadAllBuiltInFonts()
{
	for (int i = 0; i < NumBuiltInFonts; ++i)
	{
		unloadBuiltInFont(static_cast<BuiltInFontId>(i));
	}

	vertexBatch.clear();
	vertexBatch.shrink_to_fit();

	indexBatch.clear();
	indexBatch.shrink_to_fit();

	glDeleteBuffers(1, &textVbo);
	textVbo = 0;

	glDeleteBuffers(1, &textIbo);
	textIbo = 0;

	textBuffersInitialized = false;
}

// ======================================================
// isBuiltInFontLoaded():
// ======================================================

bool isBuiltInFontLoaded(const BuiltInFontId fontId)
{
	return builtInFonts[fontId].isLoaded();
}

// ======================================================
// getBuiltInFontGlyph():
// ======================================================

BuiltInFontGlyph getBuiltInFontGlyph(const BuiltInFontId fontId, const int c)
{
	assert(fontId >= 0 && fontId < NumBuiltInFonts);

	if (!builtInFonts[fontId].isLoaded()) // Load font if needed
	{
		if (!loadBuiltInFont(fontId))
		{
			return nullGlyph;
		}
	}

	if (c == ' ')
	{
		return builtInFonts[fontId].glyphs[0];
	}

	const int charIndex = (c - builtInFonts[fontId].charStart);
	if ((charIndex >= 0) && (charIndex < MaxGlyphsPerBuiltInFont))
	{
		return builtInFonts[fontId].glyphs[charIndex];
	}

	// Glyph not available.
	return nullGlyph;
}

// ======================================================
// drawChar():
// ======================================================

int drawChar(const int c, float pos[2], const float color[4], const BuiltInFontId fontId)
{
	assert(fontId >= 0 && fontId < NumBuiltInFonts);

	if (!builtInFonts[fontId].isLoaded()) // Load font if needed
	{
		if (!loadBuiltInFont(fontId))
		{
			return 0;
		}
	}

	//
	// - Text is aligned to the left.
	// - pos[9] always incremented by glyph.with.
	// - Spaces increment pos[0] by glyph.with; Tabs increment it by glyph.width*4 (4 spaces).
	// - New lines are not handled, since we don't know the initial line x to return to.
	//
	int charWidth = 0;
	switch (c)
	{
	case ' ' :
		{
			charWidth = getWhiteSpaceWidth(fontId);
			pos[0] += charWidth;
			break;
		}
	case '\t' :
		{
			charWidth = getWhiteSpaceWidth(fontId);
			pos[0] += charWidth * 4;
			break;
		}
	default :
		if (std::isgraph(c)) // Only add if char is printable
		{
			const int charIndex = c - builtInFonts[fontId].charStart;
			if ((charIndex >= 0) && (charIndex < MaxGlyphsPerBuiltInFont))
			{
				const BuiltInFontGlyph & glyph = builtInFonts[fontId].glyphs[charIndex];

				float uvs[4][2];
				uvs[0][0] = glyph.u0; uvs[0][1] = glyph.v0;
				uvs[1][0] = glyph.u1; uvs[1][1] = glyph.v0;
				uvs[2][0] = glyph.u1; uvs[2][1] = glyph.v1;
				uvs[3][0] = glyph.u0; uvs[3][1] = glyph.v1;

				// Add to the current batch.
				addGlyphQuadrilateral(pos[0], pos[1], glyph.w * globalTextScale, glyph.h * globalTextScale, color, uvs);

				charWidth = glyph.w;
				pos[0] += charWidth * globalTextScale;
			}
		}
		break;
	} // switch (c)

	return charWidth;
}

// ======================================================
// drawText():
// ======================================================

void drawText(const char * str, float pos[2], const float color[4], const BuiltInFontId fontId)
{
	assert(fontId >= 0 && fontId < NumBuiltInFonts);
	assert(str != nullptr);

	//
	// Unlike drawChar(), this function handles a \n by setting
	// pos[0] back to its initial value, and pos[1] += glyph.height.
	// A \r restores pos[0] but leaves pos[1] unchanged.
	//
	const float initialX = pos[0];
	for (size_t c = 0; str[c] != '\0'; ++c)
	{
		switch (str[c])
		{
		case '\n' :
			{
				pos[0] = initialX;
				pos[1] += builtInFonts[fontId].bmpHeight; // Every glyph of a font has the same height
				break;
			}
		case '\r' :
			{
				pos[0] = initialX;
				break;
			}
		default :
			{
				drawChar(str[c], pos, color, fontId);
				break;
			}
		} // switch (str[c])
	}
}

// ======================================================
// drawTextF():
// ======================================================

void drawTextF(float pos[2], const float color[4], const BuiltInFontId fontId, const char * format, ...)
{
	assert(fontId >= 0 && fontId < NumBuiltInFonts);

	va_list vaList;
	char tmpBuffer[1024];

	va_start(vaList, format);
	const int charsWritten = std::vsnprintf(tmpBuffer, sizeof(tmpBuffer), format, vaList);
	va_end(vaList);

	if (charsWritten > 0)
	{
		drawText(tmpBuffer, pos, color, fontId);
	}
}

// ======================================================
// drawTextLength():
// ======================================================

void drawTextLength(const char * str, const BuiltInFontId fontId, float advance[2])
{
	assert(fontId >= 0 && fontId < NumBuiltInFonts);
	assert(str != nullptr);

	if (!builtInFonts[fontId].isLoaded()) // Load font if needed
	{
		if (!loadBuiltInFont(fontId))
		{
			return;
		}
	}

	advance[0] = 0.0f;
	advance[1] = 0.0f;

	// Same rules of drawText() and drawChar().
	const float initialX = advance[0];
	for (size_t c = 0; str[c] != '\0'; ++c)
	{
		switch (str[c])
		{
		case '\n' :
			{
				advance[0] = initialX;
				advance[1] += builtInFonts[fontId].bmpHeight; // Every glyph of a font has the same height
				break;
			}
		case '\r' :
			{
				advance[0] = initialX;
				break;
			}
		case '\t' :
			{
				advance[0] += getWhiteSpaceWidth(fontId) * 4; // 4 spaces for each TAB
				break;
			}
		case ' ' :
			{
				advance[0] += getWhiteSpaceWidth(fontId);
				break;
			}
		default :
			if (std::isgraph(str[c])) // Only increment if char is printable
			{
				const int charIndex = str[c] - builtInFonts[fontId].charStart;
				if ((charIndex >= 0) && (charIndex < MaxGlyphsPerBuiltInFont))
				{
					const BuiltInFontGlyph & glyph = builtInFonts[fontId].glyphs[charIndex];
					advance[0] += glyph.w;
				}
			}
			break;
		} // switch (str[c])
	}
}

// ======================================================
// flushTextBatches():
// ======================================================

void flushTextBatches(const float * mvpMatrix)
{
	assert(mvpMatrix != nullptr);

	if (vertexBatch.empty() || indexBatch.empty())
	{
		return;
	}

	assert(textBuffersInitialized);
	assert(textVbo != 0);
	assert(textIbo != 0);

	// Update the buffers:

	glBindBuffer(GL_ARRAY_BUFFER, textVbo);
	glBufferData(GL_ARRAY_BUFFER, vertexBatch.size() * sizeof(SpriteVertex), vertexBatch.data(), GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBatch.size() * sizeof(uint16_t), indexBatch.data(), GL_STREAM_DRAW);

	// Draw call:

	// First vec4 are position + tex coords:
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), nullptr);

	// Second vec4 is the vertex color:
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<GLvoid *>(sizeof(float) * 4));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	bindTextAtlasTexture();

	const GlobalShaders & shaders = getGlobalShaders();
	gl::useShaderProgram(shaders.drawText2D.programId);
	gl::setShaderProgramUniform(shaders.drawText2D.unifMvpMatrix, mvpMatrix, 16);

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexBatch.size()), GL_UNSIGNED_SHORT, nullptr);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// Cleanup:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	vertexBatch.clear();
	indexBatch.clear();
}

// ======================================================
// setGlobalTextScale():
// ======================================================

void setGlobalTextScale(const float scale) noexcept
{
	globalTextScale = scale;
}

} // namespace font {}
} // namespace vt {}
