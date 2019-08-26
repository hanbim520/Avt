
// ================================================================================================
// -*- C++ -*-
// File: demo_app_planets.cpp
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
#include <memory>

// ======================================================
// Local constants / helpers:
// ======================================================

// Format of the indirection table we're going to be using for all textures.
// Rgba8888 uses more memory but might be a little faster, while Rgb565 uses
// half the memory at a potentially more elevated runtime cost.
static const vt::IndirectionTableFormat indirectionTableFmt = vt::IndirectionTableFormat::Rgb565;

// Color passed to glClearColor(). RGBA format.
static const float backgroundColor[4] = { 0.7f, 0.7f, 0.7f, 1.0f };

// Position of the world's unique light source.
static const Vector3 lightPosWorldSpace = { 0.0f, 7.0f, 1.0f };

// Convert from world (global) coordinates to local model coordinates.
// Input matrix must be the inverse of the model matrix: 'inverse(modelMatrix)'.
inline Vector4 worldPointToObject(const Matrix4 & invModelMatrix, const Vector3 & point)
{
	Vector4 result(invModelMatrix * Point3(point));
	result[3] = 1.0f;
	return result;
}

// Define/un-define this to toggle the drawing of a background
// plane for the planets. The background plane will use its own virtual texture.
#define PLANETS_DEMO_WITH_BACKGROUND_PLANE 1

// ======================================================
// vtDemoAppPlanets:
// ======================================================

//
// More sophisticated VT demo. Shows the use of multiple textures on a per-pixel shaded scene.
// Methods of interest are onFrameRender() and initVirtualTextures().
// The rest is mostly UI and basic application logic/house-keeping.
//
class vtDemoAppPlanets final
	: public vtDemoAppBase
{
public:

	vtDemoAppPlanets();
	~vtDemoAppPlanets();

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

	// This demo renders three spheres (planets) with big 8K by 8K textures.
	// Each planet is shaded using normal+specular maps, which means that the
	// planets have 3 8K virtual textures each.
	//
	// All virtual textures share the same PageProvider and PageResolver.
	//
	struct Planet
	{
		// A sphere model.
		std::unique_ptr<Obj3d> model3d;

		// The virtual texture containing diffuse, normal and specular maps.
		std::unique_ptr<vt::VirtualTexture> virtualTex;

		// We use this to be able to hot-swap between
		// the "real" page file and a DebugPageFile.
		vt::PageFilePtr oldPageFile;

		// Model transform to offset this planet.
		Matrix4 modelMatrix;
		Matrix4 invModelMatrix;
	};
	Planet planets[3];

	// Shared by all virtual textures.
	std::unique_ptr<vt::PageProvider> pageProvider;
	std::unique_ptr<vt::PageResolver> pageResolver;

	// We also draw a flattened cube as a background plane.
	// This object uses its own virtual texture.
	#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
	std::unique_ptr<Obj3d> backgroundPlane;
	std::unique_ptr<vt::VirtualTexture> backgroundPlaneTex;
	vt::PageFilePtr backgroundPlaneTexOldPageFile;
	Matrix4 backgroundPlaneModelMatrix;
	#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE

	// Index to the planet's VT we are drawing debug stats about.
	int statsTexture = 0;

	// 0=diffuse, 1=normal, 2=specular.
	int statsTextureSubIndex = 0;

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
	bool showDebugOverlay    = false;
	bool addDebugInfoToPages = false;
	bool useDebugPageFile    = false;

	// 3D first-person camera:
	Camera camera;
};

// ======================================================
// vtDemoAppPlanets Factory:
// ======================================================

vtDemoAppBase * createDemoApp()
{
	return new vtDemoAppPlanets();
}

void destroyDemoApp(vtDemoAppBase * app)
{
	delete app;
}

// ======================================================
// vtDemoAppPlanets Implementation:
// ======================================================

vtDemoAppPlanets::vtDemoAppPlanets()
	: vtDemoAppBase(indirectionTableFmt, backgroundColor)
{
	std::cout << "---- vtDemoAppPlanets starting up... ----\n";

	// Adjust the camera:
	camera.viewportWidth     = getScreenWidth();
	camera.viewportHeight    = getScreenHeight();
	camera.eye               = Vector3(0.0f, 1.0f, -15.0f);
	camera.oldCursorPosX     = camera.viewportWidth  / 2;
	camera.oldCursorPosY     = camera.viewportHeight / 2;
	camera.currentCursorPosX = camera.viewportWidth  / 2;
	camera.currentCursorPosY = camera.viewportHeight / 2;

	// Create the demo meshes:
	planets[0].model3d.reset(createSphereTBN());
	planets[1].model3d.reset(createSphereTBN());
	planets[2].model3d.reset(createSphereTBN());

	// Position the objects:
	planets[0].modelMatrix = Matrix4::translation(Vector3(-10.0f, 0.0f, 5.0f)) * Matrix4::rotationX(degToRad(90.0f)) * Matrix4::rotationZ(degToRad(45.0f));
	planets[1].modelMatrix = Matrix4::translation(Vector3(  0.0f, 0.0f, 5.0f)) * Matrix4::rotationX(degToRad(90.0f)) * Matrix4::rotationZ(degToRad(60.0f));
	planets[2].modelMatrix = Matrix4::translation(Vector3(+10.0f, 0.0f, 5.0f)) * Matrix4::rotationX(degToRad(90.0f));

	planets[0].invModelMatrix = inverse(planets[0].modelMatrix);
	planets[1].invModelMatrix = inverse(planets[1].modelMatrix);
	planets[2].invModelMatrix = inverse(planets[2].modelMatrix);

	// A flattened cube for the background plane:
	#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
	backgroundPlane.reset(createBoxSimple());
	backgroundPlaneModelMatrix = Matrix4::translation(Vector3(-100.0f, -100.0f, 50.0f)) *
		Matrix4::rotationX(degToRad(-90.0f)) * Matrix4::scale(Vector3(200.0f, 0.1f, 200.0f));
	#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE

	// Set up the VTs:
	initVirtualTextures();
	pageResolver->addDefaultRequests();

	// Create a basic UI:
	initDemoUI();
}

vtDemoAppPlanets::~vtDemoAppPlanets()
{
	delete[] switchButtons;
	delete debugOverlayButton;
	delete texStatsButton;
	delete movementCtrls;
}

void vtDemoAppPlanets::initVirtualTextures()
{
	pageProvider.reset(new vt::PageProvider(/* async = */ true));
	pageResolver.reset(new vt::PageResolver(*pageProvider, /* width = */ 256, /* height = */ 128));

	// Planet 0 "Dante":
	{
		vt::VTFFPageFilePtr pageFiles[3] = {
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("dante_diff.vt", addDebugInfoToPages)),
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("dante_norm.vt", addDebugInfoToPages)),
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("dante_spec.vt", addDebugInfoToPages)) };

		planets[0].virtualTex.reset(new vt::VirtualTexture(pageFiles, 3));

		pageResolver->registerVirtualTexture(planets[0].virtualTex.get());
		pageProvider->registerVirtualTexture(planets[0].virtualTex.get());

		planets[0].oldPageFile.reset(new vt::DebugPageFile(addDebugInfoToPages));
	}

	// Planet 1 "Reststop":
	{
		vt::VTFFPageFilePtr pageFiles[3] = {
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("reststop_diff.vt", addDebugInfoToPages)),
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("reststop_norm.vt", addDebugInfoToPages)),
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("reststop_spec.vt", addDebugInfoToPages)) };

		planets[1].virtualTex.reset(new vt::VirtualTexture(pageFiles, 3));

		pageResolver->registerVirtualTexture(planets[1].virtualTex.get());
		pageProvider->registerVirtualTexture(planets[1].virtualTex.get());

		planets[1].oldPageFile.reset(new vt::DebugPageFile(addDebugInfoToPages));
	}

	// Planet 2 "Serendip":
	{
		vt::VTFFPageFilePtr pageFiles[3] = {
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("serendip_diff.vt", addDebugInfoToPages)),
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("serendip_norm.vt", addDebugInfoToPages)),
			vt::VTFFPageFilePtr(new vt::VTFFPageFile("serendip_spec.vt", addDebugInfoToPages)) };

		planets[2].virtualTex.reset(new vt::VirtualTexture(pageFiles, 3));

		pageResolver->registerVirtualTexture(planets[2].virtualTex.get());
		pageProvider->registerVirtualTexture(planets[2].virtualTex.get());

		planets[2].oldPageFile.reset(new vt::DebugPageFile(addDebugInfoToPages));
	}

	// Background plane:
	#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
	{
		backgroundPlaneTex.reset(new vt::VirtualTexture(vt::VTFFPageFilePtr(new vt::VTFFPageFile("space_background.vt", addDebugInfoToPages))));

		pageResolver->registerVirtualTexture(backgroundPlaneTex.get());
		pageProvider->registerVirtualTexture(backgroundPlaneTex.get());

		backgroundPlaneTexOldPageFile.reset(new vt::DebugPageFile(addDebugInfoToPages));
	}
	#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE
}

void vtDemoAppPlanets::initDemoUI()
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

	// Since in this demo the background is darker, white text is more visible.
	vt::ui::setDefaultUITextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// Hidden button than toggles between the available textures
	// for display in the stats panel in the upper-left corner of the screen.
	// Touching the text area changes which texture stats are displayed.
	texStatsButton = new vt::ui::Button();
	texStatsButton->rect = { 0, 0, 450, 600 };
	texStatsButton->onClick = [this](vt::ui::Button &, const vt::ui::Point &)
	{
		// Cycle through diffuse, normal and specular
		// before switching to the next texture/planet.
		if (statsTextureSubIndex < 2)
		{
			++statsTextureSubIndex;
		}
		else
		{
			statsTextureSubIndex = 0;
			if ((++statsTexture) == 3)
			{
				statsTexture = 0;
			}
		}
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
			switchButtons[b].setRect({ startPos[0], startPos[1], 126, 64 });
			switchButtons[b].setTextLabel(switches[b].labelText, vt::font::Consolas30, vt::ui::getDefaultUITextColor());
			switchButtons[b].setSwitchState(switches[b].initialState);
			switchButtons[b].onStateChange = switches[b].onStateChange;
			startPos[1] += 150;
		}
	}
}

void vtDemoAppPlanets::onFrameUpdate(const int64_t /* currentTimeMs */, const double /* deltaTimeMs */)
{
	camera.update();
}

void vtDemoAppPlanets::onFrameRender(const int64_t /* currentTimeMs */, const double /* deltaTimeMs */)
{
	const Matrix4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();

	// ** PAGE ID FEEDBACK PASS **
	//
	vt::renderBindPageIdPassShader();
	pageResolver->beginPageIdPass(); // Set proper render states
	{
		// Background plane:
		#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
		vt::renderBindTextureForPageIdPass(*backgroundPlaneTex);
		vt::renderSetMvpMatrix(toFloatPtr(viewProj * backgroundPlaneModelMatrix));
		draw3dObjectSimple(*backgroundPlane);
		#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE

		// Planets:
		for (size_t p = 0; p < vt::arrayLength(planets); ++p)
		{
			vt::renderBindTextureForPageIdPass(*planets[p].virtualTex);

			// This is a quick render that only references the mesh UVs.
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * planets[p].modelMatrix));
			draw3dObjectTBN(*planets[p].model3d);
		}
	}
	pageResolver->endPageIdPass(); // Generate the page requests

	// Get the current queue of new pages to be uploaded:
	{
		vt::FulfilledPageRequestQueue readyQueue;
		if (pageProvider->getReadyQueue(readyQueue) != 0)
		{
			// Only update the textures if any new pages were loaded.
			for (size_t p = 0; p < vt::arrayLength(planets); ++p)
			{
				planets[p].virtualTex->frameUpdate(readyQueue);
			}

			#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
			backgroundPlaneTex->frameUpdate(readyQueue);
			#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE
		}
	}

	// ** TEXTURED RENDER PASS **
	//
	#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
	vt::renderBindTexturedPassShader(/* simpleRender = */ true);
	{
		// Background plane:
		vt::renderBindTextureForTexturedPass(*backgroundPlaneTex);
		vt::renderSetMvpMatrix(toFloatPtr(viewProj * backgroundPlaneModelMatrix));
		draw3dObjectSimple(*backgroundPlane);
	}
	#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE

	// Planets:
	vt::renderBindTexturedPassShader(/* simpleRender = */ false);
	{
		for (size_t p = 0; p < vt::arrayLength(planets); ++p)
		{
			vt::renderBindTextureForTexturedPass(*planets[p].virtualTex);

			// Render using normals, tangent and bi-tangent and the per-pixel shaded path.
			//
			vt::renderSetMvpMatrix(toFloatPtr(viewProj * planets[p].modelMatrix));

			const Vector4 lightPosObjectSpace = worldPointToObject(planets[p].invModelMatrix, lightPosWorldSpace);
			const Vector4 viewPosObjectSpace  = worldPointToObject(planets[p].invModelMatrix, camera.eye);

			vt::renderSetLightPosObjectSpace(toFloatPtr(lightPosObjectSpace));
			vt::renderSetViewPosObjectSpace(toFloatPtr(viewPosObjectSpace));

			draw3dObjectTBN(*planets[p].model3d);
		}
	}

	// 2D overlays:
	{
		if (showDebugOverlay)
		{
			const float scale[2] = { 0.98f, 0.98f };
			if (debugOverlayNum == 0)
			{
				pageResolver->visualizePageIds(scale);
			}
			else if (debugOverlayNum == 1)
			{
				planets[statsTexture].virtualTex->getPageIndirectionTable()->visualizeIndirectionTexture(scale);
			}
			else if (debugOverlayNum == 2)
			{
				planets[statsTexture].virtualTex->getPageTable(statsTextureSubIndex)->visualizePageTableTexture(scale);
			}
			planets[statsTexture].virtualTex->drawStats();
		}

		if (showUserInterface)
		{
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

	// Clear stats counters for each texture.
	// This is only necessary if you are displaying the texture
	// and cache statistics (VirtualTexture::drawStats()).
	for (size_t p = 0; p < vt::arrayLength(planets); ++p)
	{
		planets[p].virtualTex->clearStats();
	}

	// Make it extra synchronous if the PageProvider is also not threaded.
	// This is used mainly for profiling and testing.
	if (!pageProvider->isAsync())
	{
		glFinish();
	}
}

void vtDemoAppPlanets::setDebugPageFile()
{
	useDebugPageFile = !useDebugPageFile;

	for (size_t p = 0; p < vt::arrayLength(planets); ++p)
	{
		// Replace for the diffuse map only.
		planets[p].virtualTex->replacePageFile(planets[p].oldPageFile);
	}

	#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
	backgroundPlaneTex->replacePageFile(backgroundPlaneTexOldPageFile);
	#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE

	// Implies a flush to reset all current pages.
	purgeCaches();
}

void vtDemoAppPlanets::setShowPageInfo()
{
	addDebugInfoToPages = !addDebugInfoToPages;

	for (size_t p = 0; p < vt::arrayLength(planets); ++p)
	{
		planets[p].virtualTex->getPageFile()->setAddDebugInfoToPages(addDebugInfoToPages);
		planets[p].oldPageFile->setAddDebugInfoToPages(addDebugInfoToPages);
	}

	#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
	backgroundPlaneTex->getPageFile()->setAddDebugInfoToPages(addDebugInfoToPages);
	backgroundPlaneTexOldPageFile->setAddDebugInfoToPages(addDebugInfoToPages);
	#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE

	// Implies a flush to reset all current pages.
	purgeCaches();
}

void vtDemoAppPlanets::setForceSync()
{
	pageProvider->setAsync(!pageProvider->isAsync());
}

void vtDemoAppPlanets::purgeCaches()
{
	for (size_t p = 0; p < vt::arrayLength(planets); ++p)
	{
		planets[p].virtualTex->purgeCache();
	}

	#if PLANETS_DEMO_WITH_BACKGROUND_PLANE
	backgroundPlaneTex->purgeCache();
	#endif // PLANETS_DEMO_WITH_BACKGROUND_PLANE
}

void vtDemoAppPlanets::onAppEvent(const SDL_Event & event)
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
