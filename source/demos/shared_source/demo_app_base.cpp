
// ================================================================================================
// -*- C++ -*-
// File: demo_app_base.cpp
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

#include "demo_app_base.hpp"
#include "vt_tool_platform_utils.hpp"
#include <stdexcept>
#include <iostream>

// The app implementation must provide these two
// at the global namespace.
extern vtDemoAppBase * createDemoApp();
extern void destroyDemoApp(vtDemoAppBase * app);

// Singleton app instance.
vtDemoAppBase * vtDemoAppBase::instance(nullptr);

// ======================================================
// vtDemoAppBase:
// ======================================================

vtDemoAppBase::vtDemoAppBase(vt::IndirectionTableFormat tableFormat, const float clearColor[])
	: window(nullptr)
	, renderer(nullptr)
	, screenWidth(0)
	, screenHeight(0)
	, maxTextureSize(0)
	, showFpsCount(true)
{
	clearScrColor[0] = clearColor[0];
	clearScrColor[1] = clearColor[1];
	clearScrColor[2] = clearColor[2];
	clearScrColor[3] = clearColor[3];

	vt::tool::getScreenDimensionsInPixels(screenWidth, screenHeight);
	std::cout << "Creating app with screen res: " << screenWidth << "x" << screenHeight << '\n';

	// Init SDL and OpenGL-ES:
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		throw std::runtime_error("SDL_Init() failed!");
	}

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
	window = SDL_CreateWindow(nullptr, 0, 0, screenWidth, screenHeight, SDL_WINDOW_OPENGL);
	if (window == nullptr)
	{
		throw std::runtime_error("SDL_CreateWindow() failed!");
	}

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (renderer == nullptr)
	{
		throw std::runtime_error("SDL_CreateRenderer() failed!");
	}

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	printSdlRendererInfo();
	setDefaultGLStates();

	// Finally, init the Virtual Texturing library:
	vt::libraryInit(tableFormat);
	std::cout << "VT library version: " << vt::getLibraryVersionString() << '\n';
}

vtDemoAppBase::~vtDemoAppBase()
{
	vt::libraryShutdown();
	SDL_Quit();
}

void vtDemoAppBase::printSdlRendererInfo() const
{
	assert(renderer != nullptr);

	SDL_RendererInfo info;
	std::memset(&info, 0, sizeof(info));
	SDL_GetRendererInfo(renderer, &info);

	std::cout << "------- SDL INFO ------\n";
	std::cout << "Renderer name.......: " << info.name  << '\n';
	std::cout << "Renderer flags......: " << info.flags << '\n';
	std::cout << "Num texture formats.: " << info.num_texture_formats << '\n';
	std::cout << "Max texture width...: " << info.max_texture_width   << '\n';
	std::cout << "Max texture height..: " << info.max_texture_height  << '\n';
	for (unsigned int i = 0; i < info.num_texture_formats; ++i)
	{
		std::cout << "TextureFormat[" << i << "]....: " << SDL_GetPixelFormatName(info.texture_formats[i]) << '\n';
	}
	std::cout << "------- GL INFO -------\n";
	std::cout << "GL_VERSION..........: " << glGetString(GL_VERSION)  << '\n';
	std::cout << "GL_RENDERER.........: " << glGetString(GL_RENDERER) << '\n';
	std::cout << "GL_VENDOR...........: " << glGetString(GL_VENDOR)   << '\n';
	std::cout << "GLSL_VERSION........: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';
	std::cout << "GL_MAX_TEXTURE_SIZE.: " << maxTextureSize << "px\n";
	std::cout << "-----------------------\n";
}

void vtDemoAppBase::setDefaultGLStates()
{
	assert(screenWidth  != 0);
	assert(screenHeight != 0);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_SCISSOR_TEST);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	glFrontFace(GL_CCW); // OpenGL's default is GL_CCW
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glPixelStorei(GL_PACK_ALIGNMENT,   4);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//glViewport(0, 0, screenWidth * 0.5f, screenHeight * 0.5f);
    glViewport(0, 0, screenWidth , screenHeight );
	glClearColor(clearScrColor[0], clearScrColor[1], clearScrColor[2], clearScrColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Cache this since screen size won't change while running.
	orthoProjection = Matrix4::orthographic(
		/* left   = */  0.0f,
		/* right  = */  static_cast<float>(screenWidth),
		/* bottom = */  static_cast<float>(screenHeight),
		/* top    = */  0.0f,
		/* zNear  = */ -1.0f,
		/* zFar   = */  1.0f
	);
}

void vtDemoAppBase::appInit()
{
	assert(instance == nullptr && "Duplicate initialization!");
	if ((instance = createDemoApp()) == nullptr)
	{
		throw std::runtime_error("createDemoApp() returned null!");
	}
}

void vtDemoAppBase::appShutdown()
{
	if (instance != nullptr)
	{
		destroyDemoApp(instance);
		instance = nullptr;
	}
}

void vtDemoAppBase::appFrame(const int64_t currentTimeMs, const double deltaTimeMs)
{
	assert(instance != nullptr);
	assert(instance->renderer != nullptr);

	// Update app logic:
	instance->onFrameUpdate(currentTimeMs, deltaTimeMs);

	// Render a frame / refresh the screen:
	glClearColor(instance->clearScrColor[0],
		instance->clearScrColor[1],
		instance->clearScrColor[2],
		instance->clearScrColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	instance->onFrameRender(currentTimeMs, deltaTimeMs);

	if (instance->showFpsCount)
	{
		vt::ui::drawFpsCounter(instance->screenWidth);
	}

	vt::ui::flush2dSpriteBatches(toFloatPtr(instance->orthoProjection));
	vt::font::flushTextBatches(toFloatPtr(instance->orthoProjection));

	SDL_RenderPresent(instance->renderer);

	#if VT_EXTRA_GL_ERROR_CHECKING
	vt::gl::checkGLErrors(__FILE__, __LINE__);
	#endif // VT_EXTRA_GL_ERROR_CHECKING
}

void vtDemoAppBase::appEvent(const SDL_Event & event)
{
	assert(instance != nullptr);
	instance->onAppEvent(event);
}

// ======================================================
// main():
// ======================================================

int main(int, char *[])
{
	// Command-line is currently being ignored...

	vtDemoAppBase::appInit();

	SDL_Event event {};
	bool done = false;

	int64_t t0ms, t1ms;
	double deltaTimeMs = 1.0 / 30.0; // Assume initial frame-rate of 30fps

	while (!done)
	{
		t0ms = vt::tool::getClockMillisec();

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				done = true;
				break;
			}
			else
			{
				vtDemoAppBase::appEvent(event);
			}
		}

		vtDemoAppBase::appFrame(t0ms, deltaTimeMs);

		t1ms = vt::tool::getClockMillisec();
		deltaTimeMs = static_cast<double>(t1ms - t0ms);
	}

	vtDemoAppBase::appShutdown();
	return 0;
}
