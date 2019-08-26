
// ================================================================================================
// -*- C++ -*-
// File: vt_pixfont.cpp
// Author: Guilherme R. Lampert
// Created on: 05/10/14
// Brief: Very basic built-in pixelated front used internally for debug drawing.
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

#include <cstdint>
#include <cassert>
#include <cstdio>

namespace vt
{
namespace tool
{

// ======================================================
// Char bitmap tables:
// ======================================================

// Pixelated font (PixFont) char tables:
static const unsigned char pixFontChr20[3] = { 0x00, 0x00, 0x00 };
static const unsigned char pixFontChr21[3] = { 0x00, 0x5F, 0x00 };
static const unsigned char pixFontChr22[3] = { 0x07, 0x00, 0x07 };
static const unsigned char pixFontChr23[5] = { 0x14, 0x7F, 0x14, 0x7F, 0x14 };
static const unsigned char pixFontChr24[5] = { 0x26, 0x49, 0x7F, 0x49, 0x32 };
static const unsigned char pixFontChr25[5] = { 0x63, 0x13, 0x08, 0x64, 0x63 };
static const unsigned char pixFontChr26[5] = { 0x36, 0x49, 0x00, 0x22, 0x50 };
static const unsigned char pixFontChr27[3] = { 0x04, 0x03, 0x00 };
static const unsigned char pixFontChr28[3] = { 0x1C, 0x22, 0x41 };
static const unsigned char pixFontChr29[3] = { 0x41, 0x22, 0x1C };
static const unsigned char pixFontChr2A[5] = { 0x14, 0x2A, 0x1C, 0x2A, 0x14 };
static const unsigned char pixFontChr2B[5] = { 0x08, 0x08, 0x3E, 0x08, 0x08 };
static const unsigned char pixFontChr2C[2] = { 0x80, 0x60 };
static const unsigned char pixFontChr2D[5] = { 0x08, 0x08, 0x08, 0x08, 0x08 };
static const unsigned char pixFontChr2E[1] = { 0x40 };
static const unsigned char pixFontChr2F[5] = { 0x60, 0x10, 0x08, 0x04, 0x03 };
static const unsigned char pixFontChr30[5] = { 0x3E, 0x51, 0x49, 0x45, 0x3E };
static const unsigned char pixFontChr31[5] = { 0x00, 0x42, 0x7F, 0x40, 0x00 };
static const unsigned char pixFontChr32[5] = { 0x62, 0x51, 0x49, 0x49, 0x46 };
static const unsigned char pixFontChr33[5] = { 0x22, 0x41, 0x49, 0x49, 0x36 };
static const unsigned char pixFontChr34[5] = { 0x18, 0x14, 0x12, 0x7F, 0x10 };
static const unsigned char pixFontChr35[5] = { 0x27, 0x49, 0x49, 0x49, 0x31 };
static const unsigned char pixFontChr36[5] = { 0x3C, 0x4A, 0x49, 0x49, 0x30 };
static const unsigned char pixFontChr37[5] = { 0x01, 0x71, 0x09, 0x05, 0x03 };
static const unsigned char pixFontChr38[5] = { 0x36, 0x49, 0x49, 0x49, 0x36 };
static const unsigned char pixFontChr39[5] = { 0x06, 0x49, 0x49, 0x29, 0x1E };
static const unsigned char pixFontChr3A[1] = { 0x14 };
static const unsigned char pixFontChr3B[2] = { 0x80, 0x68 };
static const unsigned char pixFontChr3C[4] = { 0x08, 0x14, 0x22, 0x41 };
static const unsigned char pixFontChr3D[4] = { 0x14, 0x14, 0x14, 0x14 };
static const unsigned char pixFontChr3E[4] = { 0x41, 0x22, 0x14, 0x08 };
static const unsigned char pixFontChr3F[5] = { 0x02, 0x01, 0x51, 0x09, 0x06 };
static const unsigned char pixFontChr40[5] = { 0x3E, 0x41, 0x5D, 0x00, 0x4E };
static const unsigned char pixFontChr41[5] = { 0x7C, 0x12, 0x11, 0x12, 0x7C };
static const unsigned char pixFontChr42[5] = { 0x7F, 0x49, 0x49, 0x49, 0x36 };
static const unsigned char pixFontChr43[5] = { 0x3E, 0x41, 0x41, 0x41, 0x22 };
static const unsigned char pixFontChr44[5] = { 0x7F, 0x41, 0x41, 0x22, 0x1C };
static const unsigned char pixFontChr45[5] = { 0x7F, 0x49, 0x49, 0x49, 0x41 };
static const unsigned char pixFontChr46[5] = { 0x7F, 0x09, 0x09, 0x09, 0x01 };
static const unsigned char pixFontChr47[5] = { 0x3E, 0x41, 0x49, 0x29, 0x72 };
static const unsigned char pixFontChr48[5] = { 0x7F, 0x08, 0x08, 0x08, 0x7F };
static const unsigned char pixFontChr49[3] = { 0x41, 0x7F, 0x41 };
static const unsigned char pixFontChr4A[5] = { 0x20, 0x40, 0x41, 0x3F, 0x01 };
static const unsigned char pixFontChr4B[5] = { 0x7F, 0x08, 0x14, 0x22, 0x41 };
static const unsigned char pixFontChr4C[4] = { 0x7F, 0x40, 0x40, 0x40 };
static const unsigned char pixFontChr4D[5] = { 0x7F, 0x02, 0x0C, 0x02, 0x7F };
static const unsigned char pixFontChr4E[5] = { 0x7F, 0x06, 0x08, 0x30, 0x7F };
static const unsigned char pixFontChr4F[5] = { 0x3E, 0x41, 0x41, 0x41, 0x3E };
static const unsigned char pixFontChr50[5] = { 0x7F, 0x09, 0x09, 0x09, 0x06 };
static const unsigned char pixFontChr51[5] = { 0x3E, 0x41, 0x51, 0x21, 0x5E };
static const unsigned char pixFontChr52[5] = { 0x7F, 0x09, 0x19, 0x29, 0x46 };
static const unsigned char pixFontChr53[5] = { 0x26, 0x49, 0x49, 0x49, 0x32 };
static const unsigned char pixFontChr54[5] = { 0x01, 0x01, 0x7F, 0x01, 0x01 };
static const unsigned char pixFontChr55[5] = { 0x3F, 0x40, 0x40, 0x40, 0x3F };
static const unsigned char pixFontChr56[5] = { 0x0F, 0x30, 0x40, 0x30, 0x0F };
static const unsigned char pixFontChr57[7] = { 0x0F, 0x30, 0x40, 0x38, 0x40, 0x30, 0x0F };
static const unsigned char pixFontChr58[5] = { 0x63, 0x14, 0x08, 0x14, 0x63 };
static const unsigned char pixFontChr59[5] = { 0x07, 0x08, 0x70, 0x08, 0x07 };
static const unsigned char pixFontChr5A[5] = { 0x61, 0x51, 0x49, 0x45, 0x43 };
static const unsigned char pixFontChr5B[3] = { 0x7F, 0x41, 0x41 };
static const unsigned char pixFontChr5C[5] = { 0x03, 0x04, 0x08, 0x10, 0x60 };
static const unsigned char pixFontChr5D[3] = { 0x41, 0x41, 0x7F };
static const unsigned char pixFontChr5E[5] = { 0x08, 0x04, 0x02, 0x04, 0x08 };
static const unsigned char pixFontChr5F[5] = { 0x40, 0x40, 0x40, 0x40, 0x40 };
static const unsigned char pixFontChr60[3] = { 0x03, 0x04, 0x00 };
static const unsigned char pixFontChr61[4] = { 0x20, 0x54, 0x54, 0x78 };
static const unsigned char pixFontChr62[5] = { 0x7F, 0x28, 0x44, 0x44, 0x38 };
static const unsigned char pixFontChr63[4] = { 0x38, 0x44, 0x44, 0x28 };
static const unsigned char pixFontChr64[5] = { 0x38, 0x44, 0x44, 0x28, 0x7F };
static const unsigned char pixFontChr65[4] = { 0x38, 0x54, 0x54, 0x48 };
static const unsigned char pixFontChr66[4] = { 0x08, 0x7E, 0x09, 0x02 };
static const unsigned char pixFontChr67[4] = { 0x98, 0xA4, 0xA4, 0x58 };
static const unsigned char pixFontChr68[5] = { 0x7F, 0x08, 0x04, 0x04, 0x78 };
static const unsigned char pixFontChr69[2] = { 0x3D, 0x40 };
static const unsigned char pixFontChr6A[3] = { 0x80, 0x84, 0x7D };
static const unsigned char pixFontChr6B[4] = { 0x7F, 0x10, 0x28, 0x44 };
static const unsigned char pixFontChr6C[3] = { 0x01, 0x7F, 0x00 };
static const unsigned char pixFontChr6D[6] = { 0x7C, 0x08, 0x04, 0x78, 0x04, 0x78 };
static const unsigned char pixFontChr6E[5] = { 0x7C, 0x08, 0x04, 0x04, 0x78 };
static const unsigned char pixFontChr6F[4] = { 0x38, 0x44, 0x44, 0x38 };
static const unsigned char pixFontChr70[5] = { 0xFC, 0x18, 0x24, 0x24, 0x18 };
static const unsigned char pixFontChr71[5] = { 0x18, 0x24, 0x24, 0x18, 0xFC };
static const unsigned char pixFontChr72[5] = { 0x7C, 0x08, 0x04, 0x04, 0x08 };
static const unsigned char pixFontChr73[4] = { 0x48, 0x54, 0x54, 0x24 };
static const unsigned char pixFontChr74[3] = { 0x04, 0x3E, 0x44 };
static const unsigned char pixFontChr75[5] = { 0x3C, 0x40, 0x40, 0x20, 0x7C };
static const unsigned char pixFontChr76[5] = { 0x1C, 0x20, 0x40, 0x20, 0x1C };
static const unsigned char pixFontChr77[5] = { 0x3C, 0x40, 0x30, 0x40, 0x3C };
static const unsigned char pixFontChr78[5] = { 0x44, 0x28, 0x10, 0x28, 0x44 };
static const unsigned char pixFontChr79[4] = { 0x1C, 0xA0, 0xA0, 0x7C };
static const unsigned char pixFontChr7A[4] = { 0x64, 0x54, 0x54, 0x4C };
static const unsigned char pixFontChr7B[3] = { 0x08, 0x36, 0x41 };
static const unsigned char pixFontChr7C[3] = { 0x00, 0x7F, 0x00 };
static const unsigned char pixFontChr7D[3] = { 0x41, 0x36, 0x08 };
static const unsigned char pixFontChr7E[2] = { 0x00, 0x00 };
static const unsigned char pixFontChr7F[1] = { 0x00 };
static const unsigned char pixFontChr80[7] = { 0x1C, 0x22, 0x41, 0x4F, 0x41, 0x22, 0x1C };
static const unsigned char pixFontChr81[7] = { 0x1C, 0x22, 0x41, 0x49, 0x45, 0x22, 0x1C };
static const unsigned char pixFontChr82[7] = { 0x1C, 0x22, 0x41, 0x49, 0x49, 0x2A, 0x1C };
static const unsigned char pixFontChr83[7] = { 0x1C, 0x22, 0x41, 0x49, 0x51, 0x22, 0x1C };
static const unsigned char pixFontChr84[7] = { 0x1C, 0x22, 0x41, 0x79, 0x41, 0x22, 0x1C };
static const unsigned char pixFontChr85[7] = { 0x1C, 0x22, 0x51, 0x49, 0x41, 0x22, 0x1C };
static const unsigned char pixFontChr86[7] = { 0x1C, 0x2A, 0x49, 0x49, 0x41, 0x22, 0x1C };
static const unsigned char pixFontChr87[7] = { 0x1C, 0x22, 0x45, 0x49, 0x41, 0x22, 0x1C };

// PixFont metrics:
const int pixFontCharCount      = 104;
const int pixFontFirstGraphChar = 32;
const int pixFontCharWidth      = 8;

// Char lengths:
const unsigned char pixFontCharLenTable[104] = {
	3, 3, 3, 5, 5, 5, 5, 3, 3,
	3, 5, 5, 2, 5, 1, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 1,
	2, 4, 4, 4, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 3, 5, 5, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 7, 5, 5, 5, 3, 5, 3, 5,
	5, 3, 4, 5, 4, 5, 4, 4, 4,
	5, 2, 3, 4, 3, 6, 5, 4, 5,
	5, 5, 4, 3, 5, 5, 5, 5, 4,
	4, 3, 3, 3, 2, 0, 7, 7, 7,
	7, 7, 7, 7, 7
};

// Char table:
const unsigned char * pixFontCharTable[104] = {
	pixFontChr20, pixFontChr21, pixFontChr22, pixFontChr23, pixFontChr24, pixFontChr25, pixFontChr26,
	pixFontChr27, pixFontChr28, pixFontChr29, pixFontChr2A, pixFontChr2B, pixFontChr2C, pixFontChr2D,
	pixFontChr2E, pixFontChr2F, pixFontChr30, pixFontChr31, pixFontChr32, pixFontChr33, pixFontChr34,
	pixFontChr35, pixFontChr36, pixFontChr37, pixFontChr38, pixFontChr39, pixFontChr3A, pixFontChr3B,
	pixFontChr3C, pixFontChr3D, pixFontChr3E, pixFontChr3F, pixFontChr40, pixFontChr41, pixFontChr42,
	pixFontChr43, pixFontChr44, pixFontChr45, pixFontChr46, pixFontChr47, pixFontChr48, pixFontChr49,
	pixFontChr4A, pixFontChr4B, pixFontChr4C, pixFontChr4D, pixFontChr4E, pixFontChr4F, pixFontChr50,
	pixFontChr51, pixFontChr52, pixFontChr53, pixFontChr54, pixFontChr55, pixFontChr56, pixFontChr57,
	pixFontChr58, pixFontChr59, pixFontChr5A, pixFontChr5B, pixFontChr5C, pixFontChr5D, pixFontChr5E,
	pixFontChr5F, pixFontChr60, pixFontChr61, pixFontChr62, pixFontChr63, pixFontChr64, pixFontChr65,
	pixFontChr66, pixFontChr67, pixFontChr68, pixFontChr69, pixFontChr6A, pixFontChr6B, pixFontChr6C,
	pixFontChr6D, pixFontChr6E, pixFontChr6F, pixFontChr70, pixFontChr71, pixFontChr72, pixFontChr73,
	pixFontChr74, pixFontChr75, pixFontChr76, pixFontChr77, pixFontChr78, pixFontChr79, pixFontChr7A,
	pixFontChr7B, pixFontChr7C, pixFontChr7D, pixFontChr7E, pixFontChr7F, pixFontChr80, pixFontChr81,
	pixFontChr82, pixFontChr83, pixFontChr84, pixFontChr85, pixFontChr86, pixFontChr87
};

// ======================================================
// pixFontDrawChar():
// ======================================================

int pixFontDrawChar(uint8_t * image, const int colorComps, int pitch, const int theChar, const uint8_t * color, const bool flipV)
{
	//
	// Render PixFont char to RGB/RGBA pixel buffer.
	//

	assert(image != nullptr);
	assert(color != nullptr);
	assert(colorComps <= 4 && colorComps > 0);

	int cAdj, columns;
	int yStart, yEnd, yInc;

	if ((theChar < pixFontFirstGraphChar) || (theChar > (pixFontCharCount + pixFontFirstGraphChar)))
	{
		return 0;
	}

	if (flipV)
	{
		yStart =  7;
		yEnd   = -1;
		yInc   = -1;
	}
	else
	{
		yStart = 0;
		yEnd   = 8;
		yInc   = 1;
	}

	pitch  /= 4;
	cAdj    = theChar - pixFontFirstGraphChar;
	columns = pixFontCharLenTable[cAdj];

	// Draw the character:
	for (int i = 0; i < columns; ++i)
	{
		int bits = pixFontCharTable[cAdj][i];
		uint8_t * dest = image + (i * colorComps);
		for (int y = yStart; y != yEnd; y += yInc, dest += (pitch * colorComps))
		{
			int pix = bits & (1 << y);
			if (pix)
			{
				for (int c = 0; c < colorComps; ++c)
				{
					dest[c] = color[c];
				}
			}
		}
	}

	return columns;
}

// ======================================================
// RGBA colors used for the page debug text:
// ======================================================

static const uint8_t mipLevelColors[][4] = {
	{ 255, 255, 165, 255 }, // light yellow
	{  70, 225, 165, 255 }, // light green
	{  70, 140, 200, 255 }, // mild blue
	{  70, 110,  40, 255 }, // olive green
	{ 160,  70,  40, 255 }, // clay/brown
	{ 190,  40,  30, 255 }, // copper/red
	{ 255,   0,   0, 255 }, // pure red
	{ 190,   0, 255, 255 }, // violet
	{ 100,   0, 255, 255 }, // bluish
	{   0,   0, 255, 255 }, // pure blue
	{   0, 190,  20, 255 }, // dark green
	{   0, 200, 200, 255 }, // teal
	{  55,  85,  90, 255 }, // grayish-blue
	{  80,  40,  40, 255 }, // grayish-red
	{   0,  80,  40, 255 }, // grayish-green
	{   0,   0,   0, 255 }  // pure black
};
static const int numMipLevelColors = sizeof(mipLevelColors) / sizeof(mipLevelColors[0]);

// ======================================================
// addDebugInfoToPageData():
// ======================================================

void addDebugInfoToPageData(const int x, const int y, const int lvl, uint8_t * pageData, const int colorComps,
                            const bool drawPageBorder, const bool flipText, const int pageSizePx, const int pageBorderSizePx)
{
	const int colorNum = (lvl < numMipLevelColors) ? lvl : (numMipLevelColors - 1);
	const uint8_t * color = mipLevelColors[colorNum];

	// Print the following text over every page:
	//
	// L: level
	// X: tileX
	// Y: tileY
	//
	char str[256];
	std::snprintf(str, sizeof(str), "L: %d\nX: %d\nY: %d", lvl, x, y);

	int startX = (pageBorderSizePx + 3);
	int startY = (pageSizePx - 1) - pageBorderSizePx - (pixFontCharWidth + 3); // Add 3 pixels between lines

	// Draw chars:
	for (const char * c = str; *c != '\0'; ++c)
	{
		if (*c == '\n')
		{
			// Break line
			startX  = (pageBorderSizePx + 3);
			startY -= (pixFontCharWidth + 3);
			continue;
		}

		const int col = pixFontDrawChar(pageData + (startX + startY * pageSizePx) * colorComps,
		                                colorComps, (pageSizePx * 4), *c, color, flipText);
		startX += (col + 1);
	}

	// Draw outline to make border padding stand out:
	if (drawPageBorder)
	{
		const int firstPix = pageBorderSizePx;
		const int lastPix  = (pageSizePx - pageBorderSizePx - 1);
		uint8_t *p0, *p1, *p2, *p3;

		for (int i = firstPix; i <= lastPix; ++i)
		{
			p0 = &pageData[(firstPix * pageSizePx + i) * colorComps];
			p1 = &pageData[(lastPix  * pageSizePx + i) * colorComps];
			p2 = &pageData[(i * pageSizePx + firstPix) * colorComps];
			p3 = &pageData[(i * pageSizePx + lastPix)  * colorComps];

			for (int c = 0; c < colorComps; ++c)
			{
				p0[c] = color[c];
				p1[c] = color[c];
				p2[c] = color[c];
				p3[c] = color[c];
			}
		}
	}
}

} // namespace tool {}
} // namespace vt {}
