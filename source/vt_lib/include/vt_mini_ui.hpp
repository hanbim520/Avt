
// ================================================================================================
// -*- C++ -*-
// File: vt_mini_ui.hpp
// Author: Guilherme R. Lampert
// Created on: 30/10/14
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

#ifndef VTLIB_MINI_UI_HPP
#define VTLIB_MINI_UI_HPP

#include "vt_builtin_text.hpp"
#include <functional>

namespace vt
{
namespace ui
{

// ======================================================
// Miscellaneous struct / constants / helpers:
// ======================================================

struct Point
{
	int x, y;
};

struct Rect
{
	int x, y;
	int width, height;

	bool containsPoint(const Point & p) const noexcept
	{
		if (p.x < x)            { return false; }
		if (p.y < y)            { return false; }
		if (p.x > (x + width))  { return false; }
		if (p.y > (y + height)) { return false; }
		return true;
	}

	bool intersects(const Rect & other) const noexcept
	{
		return ((y + height) > other.y) && (y < (other.y + other.height)) &&
		       ((x + width)  > other.x) && (x < (other.x + other.width));
	}
};

enum class Layer
{
	// Drawn first (at the bottom)
	Layer0,
	Layer1,
	Layer2,
	Layer3,
	Layer4,
	// Drawn last (on top)

	// Layer count; Used internally.
	Count,

	// Topmost layer:
	Topmost = Layer4
};

inline void setColor4f(float color[4], float r, float g, float b, float a) noexcept
{
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;
}

// ======================================================
// Button:
// ======================================================

class Button
{
public:

	using BuiltInFont = font::BuiltInFontId;

	// Button properties:
	Rect        rect;
	float       btnColor[4];
	Layer       layer;
	GLuint      textureId;
	bool        ownsTexture;

	// Text label properties:
	std::string textLabel;
	float       textColor[4];
	Point       textOffset;
	BuiltInFont textFont;

	// Button click callback.
	// Receives the clicked button plus the clicked point.
	// Should return if the event was handled by the callback or not.
	std::function<bool(Button &, const Point &)> onClick;

public:

	// Default construction is an invalid button.
	Button();

	// Will destroy the texture if 'ownsTexture' is set.
	~Button();

	// Add the button to the underlaying sprite batch.
	void draw() const;

	// Returns true if the event was handled and should
	// not be forwarded to other UI elements any longer.
	bool mouseClickEvent(const Point & where);
};

// ======================================================
// SwitchButton:
// ======================================================

// A switch button is a binary on/off switch.
class SwitchButton
{
public:

	SwitchButton();
	SwitchButton(const Rect & rect, Layer layer = Layer::Layer0);

	void draw() const;
	bool mouseClickEvent(const Point & where);
	bool checkClick(const Point & where) const;

	// Get/set switch button state:
	bool isSwitchOn() const { return isOn; }
	void setSwitchState(bool on) { isOn = on; }

	// Set a text label that is placed under the button.
	// Text color and font size are optional.
	void setTextLabel(const std::string & text,
	                  Button::BuiltInFont fontId = font::Consolas24,
	                  const float * color = nullptr);

	// Set the button position and dimensions.
	void setRect(const Rect & rect);

	// Callback fired when the switch button state changes.
	// Receives the button reference and the new state.
	std::function<void(SwitchButton &, bool)> onStateChange;

private:

	static GLuint createButtonTexture(const int which);

	// Underlaying buttons and switch state.
	// Button 0 is the OFF button and 1 is the ON button.
	Button buttons[2];
	bool   isOn;
};

// ======================================================
// MovementControls:
// ======================================================

// Movement Controls is a set of four arrow buttons that
// can be used to implement up, down, right, left camera movement.
class MovementControls
{
public:

	// Up, down, right, left arrows.
	// Use these to set the action delegates (e.g.: onClick).
	enum { ArrowUp, ArrowDown, ArrowRight, ArrowLeft };
	Button buttons[4];

	MovementControls(const Point & centerPos, int size,
	                 int spacing, Layer layer = Layer::Layer0);

	void draw() const;
	bool mouseClickEvent(const Point & where);
	bool checkClick(const Point & where) const;

private:

	// Loads/creates the button sprites.
	static GLuint createArrowTexture(int arrow);
};

// ======================================================
// Sprite batch management / GL drawing:
// ======================================================

// Draw a sprite quadrilateral to the screen with texture applied.
// This will actually only add the sprite to the batch. The real drawing happens at flush2dSpriteBatches().
void draw2dSprite(const Rect & rect, const float color[4], const float uvs[][2], GLuint textureId, Layer layer);

// Draws a developer overlay at the top-left corner of the window for VT stats printing.
void drawVTStatsPanel(const char * title, const PageCacheMgr & cacheMgr,
                      const PageProvider & provider, const PageResolver & resolver,
                      double indrTblUpdatesPerSec, double pageUploadsPerSec,
                      unsigned int numIndirectionTableUpdates, unsigned int numPageUploads);

// Draws the Frames-Per-Second counter at the top-right corner of the window.
void drawFpsCounter(int viewportWidth);

// 2D draw calls are always batched. Yo must call this at the end
// of a frame to actually output the sprites to the screen.
void flush2dSpriteBatches(const float * mvpMatrix);

// This is automatically called by vt::libraryShutdown()
// to cleanup all dynamically allocated resources.
void shutdownUI();

// Scales width and height of rectangles drawn with draw2dSprite().
// Default value is 1 (no scaling).
void setGlobalUIScale(float scale) noexcept;

// Get/set the default text color used by VT UI elements.
void setDefaultUITextColor(float r, float g, float b, float a) noexcept;
const float * getDefaultUITextColor() noexcept;

} // namespace ui {}
} // namespace vt {}

#endif // VTLIB_MINI_UI_HPP
