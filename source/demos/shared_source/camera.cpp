
// ================================================================================================
// -*- C++ -*-
// File: camera.cpp
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

#include "camera.hpp"
#include <cassert>
#include <cstring>
#include <cmath>

// ======================================================
// Camera:
// ======================================================

Point3 Camera::getTarget() const
{
	return Point3(eye[0] + forward[0], eye[1] + forward[1], eye[2] + forward[2]);
}

Matrix4 Camera::getViewMatrix() const
{
	return Matrix4::lookAt(Point3(eye), getTarget(), up);
}

Matrix4 Camera::getProjectionMatrix() const
{
	return Matrix4::perspective(degToRad(fovyDegrees),
		static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight), zNear, zFar);
}

void Camera::reset(const Vector3 & rightVec, const Vector3 & upVec, const Vector3 & forwardVec, const Vector3 & eyeVec)
{
	right   = rightVec;
	up      = upVec;
	forward = forwardVec;
	eye     = eyeVec;
}

void Camera::pitch(float degrees)
{
	// Calculate new forward:
	forward = rotateAroundAxis(forward, right, degrees);

	// Calculate new camera up vector:
	up = cross(forward, right);
}

void Camera::rotate(const float degrees)
{
	const float radians = degToRad(degrees);
	const float sinAng  = std::sin(radians);
	const float cosAng  = std::cos(radians);

	// Save off forward components for computation:
	float xxx = forward[0];
	float zzz = forward[2];

	// Rotate forward vector:
	forward[0] = xxx *  cosAng + zzz * sinAng;
	forward[2] = xxx * -sinAng + zzz * cosAng;

	// Save off up components for computation:
	xxx = up[0];
	zzz = up[2];

	// Rotate up vector:
	up[0] = xxx *  cosAng + zzz * sinAng;
	up[2] = xxx * -sinAng + zzz * cosAng;

	// Save off right components for computation:
	xxx = right[0];
	zzz = right[2];

	// Rotate right vector:
	right[0] = xxx *  cosAng + zzz * sinAng;
	right[2] = xxx * -sinAng + zzz * cosAng;
}

void Camera::move(const MoveDir dir, const float x, const float y, const float z)
{
	if (dir == MoveDir::Forward) // Move along the camera's forward vector:
	{
		eye[0] += (forward[0] * movementSpeed) * x;
		eye[1] += (forward[1] * movementSpeed) * y;
		eye[2] += (forward[2] * movementSpeed) * z;
	}
	else if (dir == MoveDir::Back) // Move along the camera's negative forward vector:
	{
		eye[0] -= (forward[0] * movementSpeed) * x;
		eye[1] -= (forward[1] * movementSpeed) * y;
		eye[2] -= (forward[2] * movementSpeed) * z;
	}
	else if (dir == MoveDir::Left) // Move along the camera's negative right vector:
	{
		eye[0] += (right[0] * movementSpeed) * x;
		eye[1] += (right[1] * movementSpeed) * y;
		eye[2] += (right[2] * movementSpeed) * z;
	}
	else if (dir == MoveDir::Right) // Move along the camera's right vector:
	{
		eye[0] -= (right[0] * movementSpeed) * x;
		eye[1] -= (right[1] * movementSpeed) * y;
		eye[2] -= (right[2] * movementSpeed) * z;
	}
	else
	{
		assert(false && "Invalid camera move direction!");
	}
}

Vector3 Camera::rotateAroundAxis(const Vector3 & vec, const Vector3 & axis, const float degrees) const
{
	const float radians = degToRad(degrees);
	const float sinAng  = std::sin(radians);
	const float cosAng  = std::cos(radians);

	const float oneMinusCosAng = (1.0f - cosAng);
	const float aX = axis[0];
	const float aY = axis[1];
	const float aZ = axis[2];

	// Calculate X component:
	float xxx = (aX * aX * oneMinusCosAng + cosAng)      * vec[0] +
				(aX * aY * oneMinusCosAng + aZ * sinAng) * vec[1] +
				(aX * aZ * oneMinusCosAng - aY * sinAng) * vec[2];

	// Calculate Y component:
	float yyy = (aX * aY * oneMinusCosAng - aZ * sinAng) * vec[0] +
				(aY * aY * oneMinusCosAng + cosAng)      * vec[1] +
				(aY * aZ * oneMinusCosAng + aX * sinAng) * vec[2];

	// Calculate Z component:
	float zzz = (aX * aZ * oneMinusCosAng + aY * sinAng) * vec[0] +
				(aY * aZ * oneMinusCosAng - aX * sinAng) * vec[1] +
				(aZ * aZ * oneMinusCosAng + cosAng)      * vec[2];

	return Vector3(xxx, yyy, zzz);
}

void Camera::look()
{
	// Cap the mouse delta to avoid abrupt view movement.
	constexpr int deltaCap = 50;
	constexpr int edgeRotationSpeed = 6;

	int deltaX, deltaY;

	if (currentCursorPosX >= (viewportWidth - 50))
	{
		deltaX = edgeRotationSpeed;
	}
	else if (currentCursorPosX <= 50)
	{
		deltaX = -edgeRotationSpeed;
	}
	else
	{
		deltaX = currentCursorPosX - oldCursorPosX;
	}

	if (currentCursorPosY >= (viewportHeight - 50))
	{
		deltaY = edgeRotationSpeed;
	}
	else if (currentCursorPosY <= 50)
	{
		deltaY = -edgeRotationSpeed;
	}
	else
	{
		deltaY = currentCursorPosY - oldCursorPosY;
	}

	if ((deltaX == 0) && (deltaY == 0))
	{
		// No change since last update.
		return;
	}

	auto clamp = [](const int x, const int minimum, const int maximum) -> int
	{
		return (x < minimum) ? minimum : (x > maximum) ? maximum : x;
	};

	// If the user clicks/touches the edges of the screen, the
	// delta will be huge and the camera would jump. This is a
	// crude but effective way of preventing that.
	deltaX = clamp(deltaX, -deltaCap, +deltaCap);
	deltaY = clamp(deltaY, -deltaCap, +deltaCap);

	oldCursorPosX = currentCursorPosX;
	oldCursorPosY = currentCursorPosY;

	constexpr float maxAngle = 89.0f; // Max pitch angle to avoid a "Gimbal Lock"
	float amt;

	// Rotate left/right:
	amt = static_cast<float>(deltaX) * rotationSpeed;
	rotate(-amt);

	// Calculate amount to rotate up/down:
	amt = static_cast<float>(deltaY) * rotationSpeed;

	// Clamp pitch amount:
	if ((pitchAmt + amt) <= -maxAngle)
	{
		amt = -maxAngle - pitchAmt;
		pitchAmt = -maxAngle;
	}
	else if ((pitchAmt + amt) >= maxAngle)
	{
		amt = maxAngle - pitchAmt;
		pitchAmt = maxAngle;
	}
	else
	{
		pitchAmt += amt;
	}

	// Pitch camera:
	pitch(-amt);
}

void Camera::clearInput()
{
	std::memset(&keyMap, 0, sizeof(keyMap));
}

void Camera::keyInput(const char keyChar, const bool isKeyDown)
{
	switch (keyChar)
	{
	case 'w' : keyMap.w = isKeyDown; break;
	case 'a' : keyMap.a = isKeyDown; break;
	case 's' : keyMap.s = isKeyDown; break;
	case 'd' : keyMap.d = isKeyDown; break;
	default  : break;
	} // switch (keyChar)
}

void Camera::mouseInput(const int mx, const int my)
{
	currentCursorPosX = mx;
	currentCursorPosY = my;
}

void Camera::update()
{
	if (keyMap.w)
	{
		move(MoveDir::Forward, 1.0f, 1.0f, 1.0f);
	}
	if (keyMap.a)
	{
		move(MoveDir::Left, 1.0f, 1.0f, 1.0f);
	}
	if (keyMap.s)
	{
		move(MoveDir::Back, 1.0f, 1.0f, 1.0f);
	}
	if (keyMap.d)
	{
		move(MoveDir::Right, 1.0f, 1.0f, 1.0f);
	}

	look();
}
