
// ================================================================================================
// -*- C++ -*-
// File: vt_mini_ui.cpp
// Author: Guilherme R. Lampert
// Created on: 31/10/14
// Brief: Mini GUI used by the VT demos.
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
#include "vt_tool_image.hpp"
#include "vt_tool_platform_utils.hpp"

namespace vt
{
namespace ui
{

// ======================================================
// Button:
// ======================================================

Button::Button()
{
	layer        = Layer::Layer0;
	textureId    = 0;
	ownsTexture  = false;
	rect.x       = 0;
	rect.y       = 0;
	rect.width   = 0;
	rect.height  = 0;
	textOffset.x = 0;
	textOffset.y = 0;
	textFont     = font::Consolas24;
	onClick      = [](Button &, const Point &) { return false; };

	setColor4f(btnColor,  1.0f, 1.0f, 1.0f, 1.0f);
	setColor4f(textColor, 1.0f, 1.0f, 1.0f, 1.0f);
}

Button::~Button()
{
	if (ownsTexture && (textureId != 0))
	{
		gl::delete2DTexture(textureId);
	}
}

void Button::draw() const
{
	if (textureId != 0)
	{
		const float defaultUVs[][2] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};
		draw2dSprite(rect, btnColor, defaultUVs, textureId, layer);
	}

	if (!textLabel.empty())
	{
		float pos[] = {
			static_cast<float>(rect.x + textOffset.x),
			static_cast<float>(rect.y + textOffset.y)
		};
		font::drawText(textLabel.c_str(), pos, textColor, textFont);
	}
}

bool Button::mouseClickEvent(const Point & where)
{
	if (rect.containsPoint(where))
	{
		return onClick(*this, where);
	}
	return false;
}

// ======================================================
// SwitchButton:
// ======================================================

// Simple caching of the two switch button texture:
static GLuint switchBtnTexOff;
static GLuint switchBtnTexOn;

SwitchButton::SwitchButton()
	: onStateChange([](SwitchButton &, bool) { }) // Set a default no-op
	, isOn(false)
{
	buttons[0].rect        = {0,0,0,0};
	buttons[0].textureId   = createButtonTexture(0);
	buttons[0].ownsTexture = false;
	buttons[0].layer       = Layer::Layer0;
	buttons[0].onClick     = [](Button &, const Point &) { return true; };

	buttons[1].rect        = {0,0,0,0};
	buttons[1].textureId   = createButtonTexture(1);
	buttons[1].ownsTexture = false;
	buttons[1].layer       = Layer::Layer0;
	buttons[1].onClick     = [](Button &, const Point &) { return true; };
}

SwitchButton::SwitchButton(const Rect & rect, const Layer layer)
	: SwitchButton()
{
	buttons[0].rect  = rect;
	buttons[1].rect  = rect;

	buttons[0].layer = layer;
	buttons[1].layer = layer;
}

void SwitchButton::draw() const
{
	// Draw the correct button based on state:
	buttons[int(isOn)].draw();
}

bool SwitchButton::mouseClickEvent(const Point & where)
{
	if (buttons[int(isOn)].mouseClickEvent(where))
	{
		isOn = !isOn;
		onStateChange(*this, isOn);
		return true;
	}
	return false;
}

bool SwitchButton::checkClick(const Point & where) const
{
	if (buttons[0].rect.containsPoint(where))
	{
		return true;
	}
	if (buttons[1].rect.containsPoint(where))
	{
		return true;
	}
	return false;
}

GLuint SwitchButton::createButtonTexture(const int which)
{
	const char * textureName;
	GLuint     * texId;

	if (which == 0)
	{
		if (switchBtnTexOff != 0)
		{
			return switchBtnTexOff; // Already loaded.
		}
		textureName = "switch_btn_off.png";
		texId       = &switchBtnTexOff;
	}
	else
	{
		if (switchBtnTexOn != 0)
		{
			return switchBtnTexOn; // Already loaded.
		}
		textureName = "switch_btn_on.png";
		texId       = &switchBtnTexOn;
	}

	tool::Image img;
	std::string resultMessage;

	img.loadFromFile(textureName, &resultMessage, true);
	if (!resultMessage.empty())
	{
		vtLogComment(resultMessage);
	}

	*texId = gl::create2DTexture(
		img.getWidth(), img.getHeight(), GL_RGBA,
		GL_UNSIGNED_BYTE, GL_REPEAT, GL_REPEAT,
		GL_LINEAR, GL_LINEAR, img.getDataPtr<uint8_t>());

	return *texId;
}

void SwitchButton::setTextLabel(const std::string & text, const Button::BuiltInFont fontId, const float * color)
{
	buttons[0].textLabel = text;
	buttons[1].textLabel = text;

	buttons[0].textFont = fontId;
	buttons[1].textFont = fontId;

	const int glyphWidth = font::getBuiltInFontGlyph(fontId, 'A').w;
	buttons[0].textOffset.y = buttons[0].rect.height + glyphWidth;
	buttons[1].textOffset.y = buttons[1].rect.height + glyphWidth;

	float advance[2];
	font::drawTextLength(text.c_str(), fontId, advance);
	buttons[0].textOffset.x -= (static_cast<int>(advance[0]) / 2) - (buttons[0].rect.width / 2);
	buttons[1].textOffset.x -= (static_cast<int>(advance[0]) / 2) - (buttons[1].rect.width / 2);

	if (color != nullptr)
	{
		setColor4f(buttons[0].textColor, color[0], color[1], color[2], color[3]);
		setColor4f(buttons[1].textColor, color[0], color[1], color[2], color[3]);
	}
}

void SwitchButton::setRect(const Rect & rect)
{
	buttons[0].rect = rect;
	buttons[1].rect = rect;
}

// ======================================================
// MovementControls:
// ======================================================

MovementControls::MovementControls(const Point & centerPos, const int size, const int spacing, const Layer layer)
{
	buttons[ArrowUp].rect           = { centerPos.x, centerPos.y - size - spacing, size, size };
	buttons[ArrowUp].layer          = layer;
	buttons[ArrowUp].textureId      = createArrowTexture(ArrowUp);
	buttons[ArrowUp].ownsTexture    = true;

	buttons[ArrowDown].rect         = { centerPos.x, centerPos.y + size + spacing, size, size };
	buttons[ArrowDown].layer        = layer;
	buttons[ArrowDown].textureId    = createArrowTexture(ArrowDown);
	buttons[ArrowDown].ownsTexture  = true;

	buttons[ArrowRight].rect        = { centerPos.x + size + spacing, centerPos.y, size, size };
	buttons[ArrowRight].layer       = layer;
	buttons[ArrowRight].textureId   = createArrowTexture(ArrowRight);
	buttons[ArrowRight].ownsTexture = true;

	buttons[ArrowLeft].rect         = { centerPos.x - size - spacing, centerPos.y, size, size };
	buttons[ArrowLeft].layer        = layer;
	buttons[ArrowLeft].textureId    = createArrowTexture(ArrowLeft);
	buttons[ArrowLeft].ownsTexture  = true;

	vtLogComment("New MovementControls instance created...");
}

void MovementControls::draw() const
{
	for (size_t b = 0; b < arrayLength(buttons); ++b)
	{
		buttons[b].draw();
	}
}

bool MovementControls::mouseClickEvent(const Point & where)
{
	for (size_t b = 0; b < arrayLength(buttons); ++b)
	{
		if (buttons[b].mouseClickEvent(where))
		{
			return true; // Event handled
		}
	}
	return false;
}

bool MovementControls::checkClick(const Point & where) const
{
	for (size_t b = 0; b < arrayLength(buttons); ++b)
	{
		if (buttons[b].rect.containsPoint(where))
		{
			return true; // Point intersects one of the buttons
		}
	}
	return false;
}

GLuint MovementControls::createArrowTexture(const int arrow)
{
	tool::Image img;
	std::string resultMessage;

	// Lame but easy way of making the different arrows
	// for the movement keys: one texture for each direction.
	// With 2 base images, we can flip them to make the other two.
	if (arrow == ArrowUp)
	{
		img.loadFromFile("arrow_up.png", &resultMessage, true);
	}
	else if (arrow == ArrowDown)
	{
		img.loadFromFile("arrow_up.png", &resultMessage, true);
		img.flipVInPlace(); // Make it point down
	}
	else if (arrow == ArrowRight)
	{
		img.loadFromFile("arrow_right.png", &resultMessage, true);
	}
	else if (arrow == ArrowLeft)
	{
		img.loadFromFile("arrow_right.png", &resultMessage, true);
		img.flipHInPlace(); // Make it point to the left
	}
	else
	{
		vtFatalError("Invalid enum!");
	}

	if (!resultMessage.empty())
	{
		vtLogComment(resultMessage);
	}

	const GLuint texId = gl::create2DTexture(
		img.getWidth(), img.getHeight(), GL_RGBA,
		GL_UNSIGNED_BYTE, GL_REPEAT, GL_REPEAT,
		GL_LINEAR, GL_LINEAR, img.getDataPtr<uint8_t>());

	return texId;
}

// ======================================================
// Local data / helpers:
// ======================================================

namespace {

struct Sprite2d
{
	GLuint   textureId;
	uint32_t firstIndex;
};

struct SpriteVertex
{
	float x, y, u, v;
	float r, g, b, a;
};

// Batches ordered by layer or, one batch per layer:
static constexpr int NumBatches = int(Layer::Count);
static std::vector<Sprite2d> spriteBatch[NumBatches];

// Geometry for the batched sprites:
static std::vector<SpriteVertex> spriteVerts;
static std::vector<uint16_t>     spriteIndexes;

// GL buffers for 2d/sprite drawing:
static GLuint spriteVbo;
static GLuint spriteIbo;
static bool spriteBuffersInitialized;

// Will scale width and height of draw2dSprite().
static float globalUIscale = 1.0f;

// Default color used for debug text rendering.
static float defaultUItexColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

// ======================================================

static void ensure2dVboAndIboCreated()
{
	// One time initialization.
	if (spriteBuffersInitialized)
	{
		return;
	}

	glGenBuffers(1, &spriteVbo);
	glGenBuffers(1, &spriteIbo);
	spriteBuffersInitialized = true;

	vtLogComment("Created VBO/IBO for 2D/UI drawing...");
}

} // namespace {}

// ======================================================
// draw2dSprite():
// ======================================================

void draw2dSprite(const Rect & rect, const float color[4], const float uvs[][2], const GLuint textureId, const Layer layer)
{
	ensure2dVboAndIboCreated();

	assert(static_cast<int>(layer) < NumBatches);
	assert(rect.width  > 0.0f);
	assert(rect.height > 0.0f);

	const uint16_t indexes[] = { 0, 1, 2, 2, 3, 0 };
	SpriteVertex verts[4];

	// Store position with scaling:
	const float sw = rect.width  * globalUIscale;
	const float sh = rect.height * globalUIscale;
	verts[0].x = rect.x;      verts[0].y = rect.y;
	verts[1].x = rect.x + sw; verts[1].y = rect.y;
	verts[2].x = rect.x + sw; verts[2].y = rect.y + sh;
	verts[3].x = rect.x;      verts[3].y = rect.y + sh;

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

	spriteBatch[static_cast<int>(layer)].push_back({ textureId, static_cast<uint32_t>(spriteIndexes.size()) });

	// Add indexes offseting the base vertex:
	const uint16_t baseVertex = static_cast<uint16_t>(spriteVerts.size());
	for (size_t i = 0; i < arrayLength(indexes); ++i)
	{
		spriteIndexes.push_back(indexes[i] + baseVertex);
	}

	// Add vertexes to triangle batch:
	spriteVerts.insert(spriteVerts.end(), verts, verts + arrayLength(verts));
}

// ======================================================
// drawVTStatsPanel():
// ======================================================

void drawVTStatsPanel(const char * title, const PageCacheMgr & cacheMgr, const PageProvider & provider, const PageResolver & resolver,
                      double indrTblUpdatesPerSec, double pageUploadsPerSec, unsigned int numIndirectionTableUpdates, unsigned int numPageUploads)
{
	const PageCacheMgr::RuntimeStats & cacheStats = cacheMgr.getCacheStats();

	float textPos[] = { 45.0f, 65.0f };
	const float * textColor = getDefaultUITextColor();
	font::drawTextF(textPos, textColor, font::Consolas40, "%s\n", title);

	// Cache stats:
	font::drawTextF(textPos, textColor, font::Consolas36, "\n--- cache ---\n");
	font::drawTextF(textPos, textColor, font::Consolas36, "new.......: %d\n", cacheStats.newFrameRequests);
	font::drawTextF(textPos, textColor, font::Consolas36, "repeated..: %d\n", cacheStats.reFrameRequests);
	font::drawTextF(textPos, textColor, font::Consolas36, "serviced..: %d\n", cacheStats.servicedRequests);
	font::drawTextF(textPos, textColor, font::Consolas36, "dropped...: %d\n", cacheStats.droppedRequests);
	font::drawTextF(textPos, textColor, font::Consolas36, "total.....: %d\n", cacheStats.totalFrameRequests);
	font::drawTextF(textPos, textColor, font::Consolas36, "hits......: %d\n", cacheStats.hitFrameRequests);
	font::drawTextF(textPos, textColor, font::Consolas36, "miss......: %d\n", cacheStats.totalFrameRequests - cacheStats.hitFrameRequests);

	// Global/Provider stats:
	font::drawTextF(textPos, textColor, font::Consolas36, "\n--- global ---\n");
	font::drawTextF(textPos, textColor, font::Consolas36, "pending......: %d\n", provider.getNumOutstandingRequests());
	font::drawTextF(textPos, textColor, font::Consolas36, "max requests.: %d\n", resolver.getMaxPageRequestsPerFrame());
	font::drawTextF(textPos, textColor, font::Consolas36, "vis pages....: %d\n", resolver.getNumVisiblePages());
	font::drawTextF(textPos, textColor, font::Consolas36, "indr updates.: %u (%.1f/s)\n", numIndirectionTableUpdates, indrTblUpdatesPerSec);
	font::drawTextF(textPos, textColor, font::Consolas36, "page uploads.: %u (%.1f/s)\n", numPageUploads, pageUploadsPerSec);
}

// ======================================================
// drawFpsCounter():
// ======================================================

void drawFpsCounter(const int viewportWidth)
{
	unsigned int fps = tool::getFramesPerSecondCount();

	float textPos[] = { viewportWidth - 220.0f, 65.0f };
	const float * textColor = getDefaultUITextColor();
	font::drawTextF(textPos, textColor, font::Consolas48, "FPS:%u", fps);
}

// ======================================================
// flush2dSpriteBatches():
// ======================================================

void flush2dSpriteBatches(const float * mvpMatrix)
{
	assert(mvpMatrix != nullptr);

	if (spriteVerts.empty() || spriteIndexes.empty())
	{
		return;
	}

	// Check for internal consistency:
	assert(spriteBuffersInitialized);
	assert(spriteVbo != 0);
	assert(spriteIbo != 0);

	// Update the buffers:

	glBindBuffer(GL_ARRAY_BUFFER, spriteVbo);
	glBufferData(GL_ARRAY_BUFFER, spriteVerts.size() * sizeof(SpriteVertex), spriteVerts.data(), GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spriteIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, spriteIndexes.size() * sizeof(uint16_t), spriteIndexes.data(), GL_STREAM_DRAW);

	// Vertex format / render states:

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

	const GlobalShaders & shaders = getGlobalShaders();
	gl::useShaderProgram(shaders.drawText2D.programId);
	gl::setShaderProgramUniform(shaders.drawText2D.unifMvpMatrix, mvpMatrix, 16);

	// Try to avoid texture changes with a simple caching:
	GLuint currentTextId = 0;

	// Draw each layer separately.
	// Currently, each sprite is also a single draw call.
	for (int i = 0; i < NumBatches; ++i)
	{
		for (const auto & sprite : spriteBatch[i])
		{
			if (currentTextId != sprite.textureId)
			{
				gl::use2DTexture(sprite.textureId);
				currentTextId = sprite.textureId;
			}

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT,
				reinterpret_cast<GLvoid *>(sprite.firstIndex * sizeof(uint16_t)));
		}

		spriteBatch[i].clear();
	}

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// Cleanup:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Also free all the geometry batches:
	spriteVerts.clear();
	spriteIndexes.clear();
}

// ======================================================
// shutdownUI():
// ======================================================

void shutdownUI()
{
	for (int i = 0; i < NumBatches; ++i)
	{
		spriteBatch[i].clear();
		spriteBatch[i].shrink_to_fit();
	}

	spriteVerts.clear();
	spriteVerts.shrink_to_fit();

	spriteIndexes.clear();
	spriteIndexes.shrink_to_fit();

	glDeleteBuffers(1, &spriteVbo);
	spriteVbo = 0;

	glDeleteBuffers(1, &spriteIbo);
	spriteIbo = 0;

	gl::delete2DTexture(switchBtnTexOff);
	gl::delete2DTexture(switchBtnTexOn);

	spriteBuffersInitialized = false;
}

// ======================================================
// setGlobalUIScale():
// ======================================================

void setGlobalUIScale(const float scale) noexcept
{
	globalUIscale = scale;
}

// ======================================================
// setDefaultUITextColor():
// ======================================================

void setDefaultUITextColor(const float r, const float g, const float b, const float a) noexcept
{
	setColor4f(defaultUItexColor, r, g, b, a);
}

// ======================================================
// getDefaultUITextColor():
// ======================================================

const float * getDefaultUITextColor() noexcept
{
	return defaultUItexColor;
}

} // namespace ui {}
} // namespace vt {}
