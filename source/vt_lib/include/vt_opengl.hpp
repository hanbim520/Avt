
// ================================================================================================
// -*- C++ -*-
// File: vt_opengl.hpp
// Author: Guilherme R. Lampert
// Created on: 10/09/14
// Brief: OpenGL utilities and helpers.
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

#ifndef VTLIB_VT_OPENGL_HPP
#define VTLIB_VT_OPENGL_HPP

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

#include <string>

namespace vt
{
namespace gl
{

// ======================================================
// Miscellaneous GL helpers:
// ======================================================

// Clear the GL error flag without printing.
void clearGLErrors();

// Checks and prints OpenGL errors.
void checkGLErrors(const char * file, int line);

// Draws an NDC quadrilateral to the screen. This will affect the current VBO binding.
void drawNdcQuadrilateral();

// ======================================================
// GL Shader Programs:
// ======================================================

// Vertex attribute pair <mane_string, index>
struct VertexAttrib
{
	const char * name;
	GLuint index;
};

// Create a GLES Shader Program from shader source code data.
// Returns zero on failure. Program is NOT bound when this function exits.
GLuint createShaderProgram(const char * vsSrc, const char * fsSrc,
                           const VertexAttrib * const * vtxAttribs);

// Deletes a program previously created by createShaderProgram(). Will set shaderProgId to 0.
void deleteShaderProgram(GLuint & shaderProgId);

// Sets the given program as current with the GL context.
void useShaderProgram(GLuint shaderProgId);

// Get the handle (or location) of a shader uniform variable. Returns < 0 if the var is not found.
GLint getShaderProgramUniformLocation(GLuint shaderProgId, const char * varName);

// Set a single integer scalar uniform var (works for samplers too), for the current program.
bool setShaderProgramUniform(GLint uniformLoc, int val);

// Set a single float scalar uniform var, for the current program
bool setShaderProgramUniform(GLint uniformLoc, float val);

// Set a scalar, vector or matrix uniform var.
// 'count' can be:
// 1  = single float
// 2  = vec2
// 3  = vec3
// 4  = vec4
// 16 = mat4
bool setShaderProgramUniform(GLint uniformLoc, const float * val, int count);

// ======================================================
// Physical OpenGL Textures:
// ======================================================

// Creates a physical 2D OpenGL texture. Initial data may be null.
// Texture is NOT bound when this function exits.
GLuint create2DTexture(int w, int h, GLenum pixFormat, GLenum dataType, GLenum wrapS,
                       GLenum wrapT, GLenum minFilter, GLenum magFilter, const void * initData);

// Deletes a texture previously created by create2DTexture(). Will set texId to 0.
void delete2DTexture(GLuint & texId);

// Sets the given texture as current with the GL context.
void use2DTexture(GLuint texId, int texUnit = 0);

// Get the current texture id bound to GL_TEXTURE_2D.
GLuint getCurrent2DTexture();

// Sets both GL_PACK_ALIGNMENT and GL_UNPACK_ALIGNMENT for glPixelStorei().
void setPixelStoreAlignment(int alignment);

// ======================================================
// OpenGL RenderBuffer / FrameBuffer:
// ======================================================

// Create a new renderbuffer object. The RBO is NOT bound once the function exits.
GLuint createRenderBuffer(int w, int h, GLenum pixFormat);

// Delete an RBO previously created by createRenderBuffer(). Will set rboId to 0.
void deleteRenderBuffer(GLuint & rboId);

// Sets the RBO as current.
void useRenderBuffer(GLuint rboId);

// Create a new framebuffer object. The FBO is NOT bound once the function exits.
GLuint createFrameBuffer(int w, int h, bool defaultDepthBuffer, bool defaultStencilBuffer);

// Delete an FBO previously created by createFrameBuffer(). Will set fboId to 0.
void deleteFrameBuffer(GLuint & fboId);

// Sets the FBO as current.
void useFrameBuffer(GLuint fboId);

// Check if the framebuffer object and its attachments are in a valid state for rendering.
// If 'errorString' is null, no error message is generated.
bool validateFrameBuffer(GLuint fboId, std::string * errorString);

// Attaches a texture the FBO. 'level' currently must be 0; 'attachmentPoint' must be GL_COLOR_ATTACHMENT0.
void attachTextureToFrameBuffer(GLuint fboId, GLuint texId, int texLevel,
                                GLenum texTarget, GLenum attachmentPoint);

// Attaches an RBO to the framebuffer.
void attachRenderBufferToFrameBuffer(GLuint fboId, GLuint rboId, GLenum attachmentPoint);

// Same as glReadPixels() but operates on an explicit FBO.
void readFrameBuffer(GLuint fboId, int xOffs, int yOffs, int w, int h,
                     GLenum pixFormat, GLenum dataType, void * destBuffer);

} // namespace gl {}
} // namespace vt {}

#endif // VTLIB_VT_OPENGL_HPP
