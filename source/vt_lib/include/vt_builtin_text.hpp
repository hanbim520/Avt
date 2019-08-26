
// ================================================================================================
// -*- C++ -*-
// File: vt_builtin_text.hpp
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

#ifndef VTLIB_BUILTIN_TEXT_HPP
#define VTLIB_BUILTIN_TEXT_HPP

namespace vt
{
namespace font
{

// ======================================================
// Generic built-in fonts:
// ======================================================

//
// Standard built-in fonts, used primarily for debugging.
//
// Each font id is named <FontFaceName><GlyphHeight>, for example:
// "Consolas24": Consolas font face, height 24.
//
enum BuiltInFontId
{
	// Consolas font-face, several sizes:
	Consolas10,
	Consolas12,
	Consolas16,
	Consolas20,
	Consolas24,
	Consolas30,
	Consolas36,
	Consolas40,
	Consolas48,
	// Built-in font count. Internal use.
	NumBuiltInFonts
};

// ======================================================
// BuiltInFontGlyph:
// ======================================================

// Glyph info for a built-in font.
struct BuiltInFontGlyph
{
	// Texture coordinates to use when rendering:
	float u0, v0;
	float u1, v1;

	// Dimensions of the glyph's bitmap:
	int32_t w, h;

	// Pointer to glyph bitmap data (builtInFontBitmap->bitmap).
	uint32_t * pixels;
};

// ======================================================
// BuiltInFontBitmap:
// ======================================================

// Built-in bitmap font with all glyphs:
constexpr int MaxGlyphsPerBuiltInFont = 96;

// Built-in bitmap font structure:
struct BuiltInFontBitmap
{
	BuiltInFontBitmap();
	bool isLoaded() const;

	// Load/unload all glyphs for the built-in bitmap.
	void load(const uint32_t * compressedData);
	void unload();

	// Glyphs for this font:
	std::array<BuiltInFontGlyph, MaxGlyphsPerBuiltInFont> glyphs;

	// Glyphs in use / first char:
	int32_t numGlyphs;
	int32_t charStart;

	// Dimensions of the bitmap, in pixels:
	int32_t bmpWidth;
	int32_t bmpHeight;

	// Bitmap allocated to store decompressed glyphs.
	// Bitmap glyphs are stored as a string of glyphs,
	// forming a very wide image, but with height equal
	// to the glyph's height. E.g.:
	// +--------------------
	// |1|2|3|...|A|B|C|...
	// +--------------------
	uint32_t * bitmap;
};

/*
 * Basic text rendering via a set of predefined built-in fonts.
 * The built-in fonts are lightweight and fast to render but have
 * limited types and sizes. Only the ANSI char set is supported.
 */

// ======================================================
// Built-in font initialization:
// ======================================================

// Loads one of the built-in fonts if not already loaded.
bool loadBuiltInFont(BuiltInFontId fontId);

// Unloads one of the built-in fonts, if it is loaded.
void unloadBuiltInFont(BuiltInFontId fontId);

// Loads all the built-in fonts.
void loadAllBuiltInFonts();

// unloads all the built-in fonts.
void unloadAllBuiltInFonts();

// Check if a given built in font is already loaded or not.
bool isBuiltInFontLoaded(BuiltInFontId fontId);

// Get glyph info. Font-face will be loaded if not yet in memory.
// Returns a null/invalid glyph if 'c' is outside the supported character range.
BuiltInFontGlyph getBuiltInFontGlyph(BuiltInFontId fontId, int c);

// ======================================================
// Text rendering with built-in fonts:
// ======================================================

// Draw a single char using a built-in font. Loads the font if not loaded yet.
// Returns the number of units that pos.x was advanced by. This is the effective width of the rendered char.
// Zero is returned if no char was rendered. Text origin is the top-left corner of the screen. Measured in pixels.
int drawChar(int c, float pos[2], const float color[4], BuiltInFontId fontId);

// Draws a text string using a built-in font. Loads the font if not loaded yet.
// Text origin is the top-left corner of the screen; measured in pixels. Recognizes tabs and newlines ('\t' and '\n').
void drawText(const char * str, float pos[2], const float color[4], BuiltInFontId fontId);

// Same as DrawText(), but modified to receive a C-style format string.
void drawTextF(float pos[2], const float color[4], BuiltInFontId fontId, const char * format, ...);

// Query the length in pixels a string would occupy if it were to be renderer with DrawText/DrawTextF.
// On completion, 'advance' is set to the amount 'pos' would be advanced in the x and y for the
// given DrawText[F] call with the same string. This method does not draw any text.
void drawTextLength(const char * str, BuiltInFontId fontId, float advance[2]);

// Text draw calls are always batched. Yo must call this at the end
// of a frame to actually output the text to the screen.
void flushTextBatches(const float * mvpMatrix);

// Can be used to automatically scale the width and height
// of text glyphs. Default value is 1 (no scaling).
void setGlobalTextScale(float scale) noexcept;

} // namespace font {}
} // namespace vt {}

#endif // VTLIB_BUILTIN_TEXT_HPP
