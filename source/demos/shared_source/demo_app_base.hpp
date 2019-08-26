
// ================================================================================================
// -*- C++ -*-
// File: demo_app_base.hpp
// Author: Guilherme R. Lampert
// Created on: 21/11/14
// Brief: Base class for our VT iOS demo applications.
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

#ifndef DEMO_APP_BASE_HPP
#define DEMO_APP_BASE_HPP

// Common includes:
#include "vectormath.hpp"
#include "camera.hpp"
#include "obj3d.hpp"
#include "SDL.h"

// Virtual Texturing Library:
#include "vt.hpp"
#include "vt_mini_ui.hpp"
#include "vt_builtin_text.hpp"

// ======================================================
// vtDemoAppBase:
// ======================================================

class vtDemoAppBase
{
public:

	vtDemoAppBase(vt::IndirectionTableFormat tableFormat, const float clearColor[]);
	virtual ~vtDemoAppBase();

	// App callbacks, implemented by the client application:
	virtual void onFrameUpdate(int64_t currentTimeMs, double deltaTimeMs) = 0;
	virtual void onFrameRender(int64_t currentTimeMs, double deltaTimeMs) = 0;
	virtual void onAppEvent(const SDL_Event & event) = 0;

	// Getters/setters:
	int getScreenWidth()    const { return screenWidth;    }
	int getScreenHeight()   const { return screenHeight;   }
	int getMaxTextureSize() const { return maxTextureSize; }
	void setShowFpsCount(bool show) { showFpsCount = show; }
	static vtDemoAppBase & getInstance() { return *instance; }

	// Internal helpers. These are not meant to be called by client applications.
	static void appInit();
	static void appShutdown();
	static void appFrame(int64_t currentTimeMs, double deltaTimeMs);
	static void appEvent(const SDL_Event & event);

protected:

	void printSdlRendererInfo() const;
	void setDefaultGLStates();

private:

	SDL_Window   * window;
	SDL_Renderer * renderer;
	int            screenWidth;
	int            screenHeight;
	int            maxTextureSize;
	float          clearScrColor[4];
	Matrix4        orthoProjection;
	bool           showFpsCount;

	static vtDemoAppBase * instance;
};

#endif // DEMO_APP_BASE_HPP
