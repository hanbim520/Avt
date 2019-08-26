
// ================================================================================================
// -*- C++ -*-
// File: obj3d.hpp
// Author: Guilherme R. Lampert
// Created on: 03/11/14
// Brief: Basic loading of 3D meshes and some procedural generation as well. Used by the demos.
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

#ifndef OBJ3D_HPP
#define OBJ3D_HPP

#include <TargetConditionals.h>

#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
	#include <OpenGL/gl.h>
#elif defined(__IPHONEOS__) || TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>
#else
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#endif

#include <cstdint>
#include <cstdlib>

// ======================================================
// Generic 3D object:
// ======================================================

struct Obj3d
{
	GLsizei numVertexes = 0;
	GLsizei numIndexes  = 0;
	GLuint vb = 0;
	GLuint ib = 0;

	~Obj3d()
	{
		if (vb != 0) { glDeleteBuffers(1, &vb); }
		if (ib != 0) { glDeleteBuffers(1, &ib); }
	}
};

// Obj3d factories:
Obj3d * createSphereSimple(); // Just normals and TC
Obj3d * createBoxSimple();    // Just normals and TC
Obj3d * createSphereTBN();    // With TBN verts + TC

// Create an Obj3d with user defined geometry:
Obj3d * create3dObject(const float vertexes[][3], size_t numVertexes,
                       const float normals[][3],  size_t numNormals,
                       const float uvs[][2],      size_t numTexCoords,
                       const uint16_t indexes[],  size_t numIndexes);

// Drawing:
void draw3dObjectSimple(const Obj3d & obj); // Draw with position, normal and TexCoord.
void draw3dObjectTBN(const Obj3d & obj);    // Draw with position, normal, TC and tangent + bi-tangent.

#endif // OBJ3D_HPP
