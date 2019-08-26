
// ================================================================================================
// -*- C++ -*-
// File: camera.hpp
// Author: Guilherme R. Lampert
// Created on: 31/10/14
// Brief: Simple first-person demo camera.
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

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "vectormath.hpp"

// ======================================================
// Camera:
// ======================================================

//
// Camera -- Simple first-person camera.
//
// Initial Camera Axes:
//
//  (up)
//  +Y   +Z (forward)
//  |   /
//  |  /
//  | /
//  + ------ +X (right)
//
// The methods you will want to call in the application are:
//  - update()
//  - keyInput()
//  - mouseInput()
//  - clearInput()
// The others are mostly for internal use.
//
class Camera
{
public:

	Vector3 right   = { 1.0f, 0.0f, 0.0f }; // The normalized axis that points to the "right"
	Vector3 up      = { 0.0f, 1.0f, 0.0f }; // The normalized axis that points "up"
	Vector3 forward = { 0.0f, 0.0f, 1.0f }; // The normalized axis that points "forward"
	Vector3 eye     = { 0.0f, 1.0f, 0.0f }; // The position of the camera (i.e. the camera's eye and origin of the camera's coordinate system)

	// Camera movement and rotation speed:
	float rotationSpeed = 4.0f * (1.0f / 30.0f);
	float movementSpeed = 7.0f * (1.0f / 30.0f);

	// Projection parameters:
	float fovyDegrees    = 60.0f;
	float zNear          = 0.1f;
	float zFar           = 500.0f;
	int   viewportWidth  = 1024;
	int   viewportHeight = 768;

	// Virtual keys we care about:
	struct {
		bool w;
		bool a;
		bool s;
		bool d;
	} keyMap = { false, false, false, false };

	// Current mouse/cursor position:
	int oldCursorPosX     = 0;
	int oldCursorPosY     = 0;
	int currentCursorPosX = 0;
	int currentCursorPosY = 0;

	// Pitch angle history:
	float pitchAmt = 0.0f;

	// Directions to by with Camera::move().
	enum class MoveDir
	{
		Forward, Back,
		Left,    Right
	};

public:

	// This function returns what the camera is looking at. Our eye is ALWAYS the origin
	// of camera's coordinate system and we are ALWAYS looking straight down the "forward" axis
	// so to calculate the target it's just a matter of adding the eye plus the forward.
	Point3 getTarget() const;

	// Build and return the 4x4 camera view matrix.
	Matrix4 getViewMatrix() const;

	// Get a perspective projection matrix that can be combined with the camera's view matrix.
	Matrix4 getProjectionMatrix() const;

	// Resets to a starting position.
	void reset(const Vector3 & rightVec, const Vector3 & upVec, const Vector3 & forwardVec, const Vector3 & eyeVec);

	// Pitches camera by an angle in degrees. (tilt it up/down)
	void pitch(float degrees);

	// Rotates around world Y-axis by the given angle (in degrees).
	void rotate(float degrees);

	// Moves the camera by the given direction, using the default movement speed.
	// The last three parameters indicate in which axis to move.
	// If it is equal to 1, move in that axis, if it is zero don't move.
	void move(MoveDir dir, float x, float y, float z);

	// This allows us to rotate 'vec' around an arbitrary axis by an angle in degrees.
	// Used internally.
	Vector3 rotateAroundAxis(const Vector3 & vec, const Vector3 & axis, float degrees) const;

	// Updates camera based on mouse movement.
	// Called internally by update().
	void look();

	// Reset the camera input.
	void clearInput();

	// Receives a key up/down event and moves the camera accordingly.
	void keyInput(char keyChar, bool isKeyDown);

	// Receive new mouse coordinates.
	void mouseInput(int mx, int my);

	// Updates the camera based on input received.
	// Should be called every frame.
	void update();
};

#endif // CAMERA_HPP
