
// ================================================================================================
// -*- C++ -*-
// File: demo_app_simple.cpp
// Author: Guilherme R. Lampert
// Created on: 21/11/14
// Brief: Simple/basic VT demo application.
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
#include <iostream>

// ======================================================
// Local constants:
// ======================================================

// Format of the indirection table we're going to be using for all textures.
// Rgba8888 uses more memory but might be a little faster, while Rgb565 uses
// half the memory at a potentially more elevated runtime cost.
static const vt::IndirectionTableFormat indirectionTableFmt = vt::IndirectionTableFormat::Rgba8888;

// Color passed to glClearColor(). RGBA format.
static const float backgroundColor[4] = { 0.7f, 0.7f, 0.7f, 1.0f };

// ======================================================
// vtDemoAppSimple:
// ======================================================

//
// Simple VT demo.
// Methods of interest are onFrameRender() and initVirtualTextures().
// The rest is mostly UI and basic application logic/house-keeping.
//
class vtDemoAppSimple final
	: public vtDemoAppBase
{
public:

	vtDemoAppSimple();
	~vtDemoAppSimple();

	void onFrameUpdate(int64_t currentTimeMs, double deltaTimeMs) override;
	void onFrameRender(int64_t currentTimeMs, double deltaTimeMs) override;
	void onAppEvent(const SDL_Event & event) override;

private:

	void initVirtualTextures();
	void initDemoUI();

	// UI callbacks:
	void setDebugPageFile();
	void setShowPageInfo();
	void setForceSync();
	void purgeCaches();

private:

	// This demo app draws a sphere, a cube and a ground plane (made of a stretched cube),
	// applying a VirtualTexture to each object. Virtual textures share the same
	// PageProvider and PageResolver. The cube and the ground plane also share the same texture.
	//
	// Objects are rendered with diffuse textures only. No lighting applied.
	//
	vt::PageProvider   * pageProvider = nullptr;
	vt::PageResolver   * pageResolver = nullptr;
	vt::VirtualTexture * vtTexSphere  = nullptr;
	vt::VirtualTexture * vtTexCube    = nullptr;
	Obj3d              * sphereModel  = nullptr;
	Obj3d              * cubeModel    = nullptr;

	// We use these pointers to be able to hot-swap
	// between the "real" page file and the DebugPageFile.
	vt::PageFilePtr vtTexSphereOldPageFile;
	vt::PageFilePtr vtTexCubeOldPageFile;

	// Model transforms:
	Matrix4 sphereModelMatrix;
	Matrix4 cubeModelMatrix;
	Matrix4 groundPlaneModelMatrix;

	// Texture we are currently displaying debug info about.
	// 0=sphere_texture, 1=cube_texture.
	int statsTextureNum = 0;

	// Debug overlay to draw if enabled:
	// 0=page_id_feedback, 1=indirection_table, 2=page_table.
	int debugOverlayNum = 0;

	// Simple UI controls:
	vt::ui::MovementControls * movementCtrls      = nullptr;
	vt::ui::Button           * texStatsButton     = nullptr;
	vt::ui::Button           * debugOverlayButton = nullptr;
	vt::ui::SwitchButton     * switchButtons      = nullptr;
	static constexpr int       numSwitchButtons   = 6;

	// Misc flags:
	bool showUserInterface   = true;
	bool showDebugOverlay    = true;
	bool addDebugInfoToPages = true;
	bool useDebugPageFile    = false;

	// 3D first-person camera:
	Camera camera;
};

// ======================================================
// vtDemoAppSimple Factory:
// ======================================================

vtDemoAppBase * createDemoApp()
{
	return new vtDemoAppSimple();
}

void destroyDemoApp(vtDemoAppBase * app)
{
	delete app;
}

// ======================================================
// vtDemoAppSimple Implementation:
// ======================================================

vtDemoAppSimple::vtDemoAppSimple()
	: vtDemoAppBase(indirectionTableFmt, backgroundColor)
{
	std::cout << "---- vtDemoAppSimple starting up... ----\n";

	// Adjust the camera:
	camera.viewportWidth     = getScreenWidth();
	camera.viewportHeight    = getScreenHeight();
	camera.eye               = Vector3(0.0f, 1.0f, -10.0f);
	camera.oldCursorPosX     = camera.viewportWidth  / 2;
	camera.oldCursorPosY     = camera.viewportHeight / 2;
	camera.currentCursorPosX = camera.viewportWidth  / 2;
	camera.currentCursorPosY = camera.viewportHeight / 2;

	// Create the demo meshes:
	sphereModel = createSphereSimple();
	cubeModel   = createBoxSimple();

	// Cache the transforms:
	sphereModelMatrix      = Matrix4::translation(Vector3(-1.5f,  0.0f,  0.0f));
	cubeModelMatrix        = Matrix4::translation(Vector3(+1.5f,  0.0f,  0.0f));
	groundPlaneModelMatrix = Matrix4::translation(Vector3(-6.0f, -1.3f, -5.0f)) * Matrix4::scale(Vector3(15.0f, 0.1f, 15.0f));

	// Set up the VTs:
	initVirtualTextures();
	pageResolver->addDefaultRequests();

	// Create a basic UI:
	initDemoUI();
}

vtDemoAppSimple::~vtDemoAppSimple()
{
	delete[] switchButtons;
	delete debugOverlayButton;
	delete texStatsButton;
	delete movementCtrls;
	delete cubeModel;
	delete sphereModel;
	delete vtTexCube;
	delete vtTexSphere;
	delete pageResolver;
	delete pageProvider;
}

void vtDemoAppSimple::initVirtualTextures()
{
	pageProvider = new vt::PageProvider(/* async = */ true);
	pageResolver = new vt::PageResolver(*pageProvider, /* width = */ 256, /* height = */ 128);

	// Virtual texture for the sphere model:
	{
		vtTexSphere = new vt::VirtualTexture(vt::VTFFPageFilePtr(new vt::VTFFPageFile("brick_wall.vt", addDebugInfoToPages)));
		pageResolver->registerVirtualTexture(vtTexSphere);
		pageProvider->registerVirtualTexture(vtTexSphere);

		vtTexSphereOldPageFile.reset(new vt::DebugPageFile(addDebugInfoToPages));
	}

	// Virtual texture for the cube model:
	{
		vtTexCube = new vt::VirtualTexture(vt::VTFFPageFilePtr(new vt::VTFFPageFile("lenna.vt", addDebugInfoToPages)));
		pageResolver->registerVirtualTexture(vtTexCube);
		pageProvider->registerVirtualTexture(vtTexCube);

		vtTexCubeOldPageFile.reset(new vt::DebugPageFile(addDebugInfoToPages));
	}
}

void vtDemoAppSimple::initDemoUI()
{
	// NOTE: This UI is adjusted for the iPad retina and
	// will probably be misaligned or broken on other device models!
	//
	movementCtrls = new vt::ui::MovementControls({ getScreenWidth() - 300, getScreenHeight() - 300 }, 120, 30);

	// First person camera controls (emulate keyboard WSAD):
	movementCtrls->buttons[vt::ui::MovementControls::ArrowUp].onClick =
		[this](vt::ui::Button &, const vt::ui::Point &) { camera.keyInput('w', true); return true; };

	movementCtrls->buttons[vt::ui::MovementControls::ArrowDown].onClick =
		[this](vt::ui::Button &, const vt::ui::Point &) { camera.keyInput('s', true); return true; };

	movementCtrls->buttons[vt::ui::MovementControls::ArrowRight].onClick =
		[this](vt::ui::Button &, const vt::ui::Point &) { camera.keyInput('d', true); return true; };

	movementCtrls->buttons[vt::ui::MovementControls::ArrowLeft].onClick =
		[this](vt::ui::Button &, const vt::ui::Point &) { camera.keyInput('a', true); return true; };

	// Hidden button than toggles between the available textures
	// for display in the stats panel in the upper-left corner of the screen.
	// Touching the text area changes which texture stats are displayed.
	texStatsButton = new vt::ui::Button();
	texStatsButton->rect = { 0, 0, 450, 600 };
	texStatsButton->onClick = [this](vt::ui::Button &, const vt::ui::Point &)
	{
		statsTextureNum = (statsTextureNum + 1) % 2;
		return true;
	};

	// This invisible button cycles through the debug overlays
	// for the currently selected virtual texture and page id feedback pass.
	debugOverlayButton = new vt::ui::Button();
	debugOverlayButton->rect = { 0, getScreenHeight() - 600, 600, 600 };
	debugOverlayButton->onClick = [this](vt::ui::Button &, const vt::ui::Point &)
	{
		debugOverlayNum = (debugOverlayNum + 1) % 3;
		return true;
	};

	// On/off switches:
	{
		auto cb0 = [this](vt::ui::SwitchButton &, bool) { showUserInterface = !showUserInterface; };
		auto cb1 = [this](vt::ui::SwitchButton &, bool) { showDebugOverlay  = !showDebugOverlay;  };
		auto cb2 = [this](vt::ui::SwitchButton &, bool) { setDebugPageFile(); };
		auto cb3 = [this](vt::ui::SwitchButton &, bool) { setShowPageInfo();  };
		auto cb4 = [this](vt::ui::SwitchButton &, bool) { purgeCaches();      };
		auto cb5 = [this](vt::ui::SwitchButton &, bool) { setForceSync();     };

		struct {
			const char * labelText;
			std::function<void(vt::ui::SwitchButton &, bool)> onStateChange;
			bool initialState;
		} switches[numSwitchButtons] = {
			{ "show/hide ui"        , cb0, showUserInterface        }, // show/hide UI buttons/panels
			{ "show/hide tables"    , cb1, showDebugOverlay         }, // show/hide indirection table/page cache overlays
			{ "use debug page file" , cb2, useDebugPageFile         }, // toggle use of a DebugPageFile
			{ "add debug to pages"  , cb3, addDebugInfoToPages      }, // add debug info the every loaded page
			{ "purge vt caches"     , cb4, false                    }, // purge the VT cache once for each texture
			{ "force sync operation", cb5, !pageProvider->isAsync() }  // use synchronous PageProvider and glFinish every frame
		};

		switchButtons = new vt::ui::SwitchButton[numSwitchButtons];

		int startPos[] = { getScreenWidth() - 220, 160 };
		for (int b = 0; b < numSwitchButtons; ++b)
		{
			switchButtons[b].setRect({ startPos[0], startPos[1], 100, 30 });
			switchButtons[b].setTextLabel(switches[b].labelText, vt::font::Consolas30, vt::ui::getDefaultUITextColor());
			switchButtons[b].setSwitchState(switches[b].initialState);
			switchButtons[b].onStateChange = switches[b].onStateChange;
			startPos[1] += 100;
		}
	}
}

void vtDemoAppSimple::onFrameUpdate(const int64_t /* currentTimeMs */, const double /* deltaTimeMs */)
{
	camera.update();
}

void vtDemoAppSimple::onFrameRender(const int64_t /* currentTimeMs */, const double /* deltaTimeMs */)
{
 
	const Matrix4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();

	// ** PAGE ID FEEDBACK PASS **
	//
	vt::renderBindPageIdPassShader();
	pageResolver->beginPageIdPass(); // Set proper render states
	{
		// Render all geometry using vtTexCube:
		vt::renderBindTextureForPageIdPass(*vtTexCube);
		{
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * cubeModelMatrix));
			draw3dObjectSimple(*cubeModel);

			// A stretched cube making the ground plane:
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * groundPlaneModelMatrix));
			draw3dObjectSimple(*cubeModel);
		}

		// Render all geometry using vtTexSphere:
		vt::renderBindTextureForPageIdPass(*vtTexSphere);
		{
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * sphereModelMatrix));
			draw3dObjectSimple(*sphereModel);
		}
	}
	pageResolver->endPageIdPass(); // Generate the page requests

	// Get the current queue of new pages to be uploaded:
	{
		vt::FulfilledPageRequestQueue readyQueue;
		if (pageProvider->getReadyQueue(readyQueue) != 0)
		{
			// Only update the textures if any new pages were loaded.
			vtTexCube->frameUpdate(readyQueue);
			vtTexSphere->frameUpdate(readyQueue);
		}
	}

	// ** TEXTURED RENDER PASS **
	//
	vt::renderBindTexturedPassShader(/* simpleRender = */ true);
	{
		// Render all geometry using vtTexCube:
		vt::renderBindTextureForTexturedPass(*vtTexCube);
		{
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * cubeModelMatrix));
			draw3dObjectSimple(*cubeModel);

			// A stretched cube making the ground plane:
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * groundPlaneModelMatrix));
			draw3dObjectSimple(*cubeModel);
		}

		// Render all geometry using vtTexSphere:
		vt::renderBindTextureForTexturedPass(*vtTexSphere);
		{
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * sphereModelMatrix));
			draw3dObjectSimple(*sphereModel);
		}
	}
 
	// 2D overlays:
	{
		if (showDebugOverlay)
		{
            const float scale[2] = { 0.98f, 0.98f };
          //  const float scale[2] = { 0.5f, 0.5f };
			if (debugOverlayNum == 0)
			{
				pageResolver->visualizePageIds(scale);
			}
			else if (debugOverlayNum == 1)
			{
				if (statsTextureNum == 0)
				{
                    vtTexSphere->getPageIndirectionTable()->visualizeIndirectionTexture(scale);
				}
				else
				{
					vtTexCube->getPageIndirectionTable()->visualizeIndirectionTexture(scale);
				}
			}
			else if (debugOverlayNum == 2)
			{
				if (statsTextureNum == 0)
				{
					vtTexSphere->getPageTable()->visualizePageTableTexture(scale);
				}
				else
				{
					vtTexCube->getPageTable()->visualizePageTableTexture(scale);
				}
			}
		}
    
		if (showUserInterface)
		{
			if (statsTextureNum == 0)
			{
				vtTexSphere->drawStats();
			}
			else
			{
				vtTexCube->drawStats();
			}

			movementCtrls->draw();
			for (int b = 0; b < numSwitchButtons; ++b)
			{
				switchButtons[b].draw();
			}
		}
		else
		{
			// Still draw button zero, which is the UI on/off switch.
			switchButtons[0].draw();
		}
	}

	// This is only necessary if you are displaying the texture
	// and cache statistics (VirtualTexture::drawStats()).
	vtTexSphere->clearStats();
	vtTexCube->clearStats();

	// Make it extra synchronous if the PageProvider is also not threaded.
	// This is used mainly for profiling and testing.
	if (!pageProvider->isAsync())
	{
		glFinish();
	}
}

void vtDemoAppSimple::setDebugPageFile()
{
	useDebugPageFile = !useDebugPageFile;

	vtTexSphere->replacePageFile(vtTexSphereOldPageFile);
	vtTexCube->replacePageFile(vtTexCubeOldPageFile);

	// Implies a flush to reset all current pages.
	purgeCaches();
}

void vtDemoAppSimple::setShowPageInfo()
{
	addDebugInfoToPages = !addDebugInfoToPages;

	vtTexSphere->getPageFile()->setAddDebugInfoToPages(addDebugInfoToPages);
	vtTexCube->getPageFile()->setAddDebugInfoToPages(addDebugInfoToPages);

	vtTexSphereOldPageFile->setAddDebugInfoToPages(addDebugInfoToPages);
	vtTexCubeOldPageFile->setAddDebugInfoToPages(addDebugInfoToPages);

	// Implies a flush to reset all current pages.
	purgeCaches();
}

void vtDemoAppSimple::setForceSync()
{
	pageProvider->setAsync(!pageProvider->isAsync());
}

void vtDemoAppSimple::purgeCaches()
{
	vtTexSphere->purgeCache();
	vtTexCube->purgeCache();
}

void vtDemoAppSimple::onAppEvent(const SDL_Event & event)
{
	switch (event.type)
	{
	case SDL_MOUSEBUTTONDOWN :
		{
			bool eventHandled = false;
			const vt::ui::Point p = { event.button.x, event.button.y };
			for (int b = 0; b < numSwitchButtons; ++b)
			{
				if (switchButtons[b].mouseClickEvent(p))
				{
					eventHandled = true;
					break;
				}
			}
			if (!eventHandled)
			{
				if (!texStatsButton->mouseClickEvent(p) && !debugOverlayButton->mouseClickEvent(p))
				{
					movementCtrls->mouseClickEvent(p);
				}
			}
			break;
		}
	case SDL_MOUSEMOTION :
		{
			bool buttonClick = false;
			const vt::ui::Point p = { event.motion.x, event.motion.y };
			for (int b = 0; b < numSwitchButtons; ++b)
			{
				if (switchButtons[b].checkClick(p))
				{
					// Don't attempt to move the camera if clicking an UI button.
					buttonClick = true;
					break;
				}
			}
			if (!buttonClick)
			{
				if (!texStatsButton->mouseClickEvent(p) && !debugOverlayButton->mouseClickEvent(p))
				{
					if (!movementCtrls->checkClick(p))
					{
						camera.mouseInput(event.motion.x, event.motion.y);
					}
				}
			}
			break;
		}
	case SDL_MOUSEBUTTONUP :
		{
			camera.clearInput();
			break;
		}
	default :
		{
			// Event not handled here...
			break;
		}
	} // switch (event.type)
}
