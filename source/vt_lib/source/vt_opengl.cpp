
// ================================================================================================
// -*- C++ -*-
// File: vt_opengl.cpp
// Author: Guilherme R. Lampert
// Created on: 21/09/14
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

#include "vt.hpp"

namespace vt
{
namespace gl
{

// ======================================================
// errorToString():
// ======================================================

#ifndef VT_NO_LOGGING
static const char * errorToString(const GLenum errCode) noexcept
{
	switch (errCode)
	{
	case GL_NO_ERROR                      : return "GL_NO_ERROR";
	case GL_INVALID_ENUM                  : return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE                 : return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION             : return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION : return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY                 : return "GL_OUT_OF_MEMORY";
	default                               : return "Unknown OpenGL error";
	} // switch (errCode)
}
#endif // VT_NO_LOGGING

// ======================================================
// fboStatusToString();
// ======================================================

static const char * fboStatusToString(const GLenum fboStatus) noexcept
{
	switch (fboStatus)
	{
	case GL_FRAMEBUFFER_COMPLETE                      : return "GL_FRAMEBUFFER_COMPLETE";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT         : return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS         : return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT : return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
	case GL_FRAMEBUFFER_UNSUPPORTED                   : return "GL_FRAMEBUFFER_UNSUPPORTED";
	default                                           : return "Unknown GL framebuffer status";
	} // switch (fboStatus)
}

// ======================================================
// printShaderInfoLogs():
// ======================================================

static void printShaderInfoLogs(const GLuint progId, const GLuint vsId, const GLuint fsId)
{
	GLsizei charsWritten;
	static char infoLogBuf[4096];

	// Shader log is printed as warnings.

	charsWritten = 0;
	glGetShaderInfoLog(vsId, arrayLength(infoLogBuf) - 1, &charsWritten, infoLogBuf);
	if (charsWritten > 0)
	{
		infoLogBuf[arrayLength(infoLogBuf) - 1] = '\0';
		vtLogWarning("---------- VS INFO LOG ----------\n" << infoLogBuf);
	}

	charsWritten = 0;
	glGetShaderInfoLog(fsId, arrayLength(infoLogBuf) - 1, &charsWritten, infoLogBuf);
	if (charsWritten > 0)
	{
		infoLogBuf[arrayLength(infoLogBuf) - 1] = '\0';
		vtLogWarning("---------- FS INFO LOG ----------\n" << infoLogBuf);
	}

	charsWritten = 0;
	glGetProgramInfoLog(progId, arrayLength(infoLogBuf) - 1, &charsWritten, infoLogBuf);
	if (charsWritten > 0)
	{
		infoLogBuf[arrayLength(infoLogBuf) - 1] = '\0';
		vtLogWarning("-------- PROGRAM INFO LOG -------\n" << infoLogBuf);
	}
}

// ======================================================
// checkGLErrors():
// ======================================================

void checkGLErrors(const char * file, const int line)
{
#ifndef VT_NO_LOGGING
	GLenum errCode;
	while ((errCode = glGetError()) != GL_NO_ERROR)
	{
		vtLogWarning("OpenGL reported error: \'" << errorToString(errCode)
			<< "\' (0x" << errCode << ") at file " << file << "(" << line << ")");
	}
#else // VT_NO_LOGGING defined
	(void)file;
	(void)line;
#endif // VT_NO_LOGGING
}

// ======================================================
// clearGLErrors():
// ======================================================

void clearGLErrors()
{
	while (glGetError() != GL_NO_ERROR)
	{
	}
}

// ======================================================
// drawNdcQuadrilateral():
// ======================================================

void drawNdcQuadrilateral()
{
	static GLuint screenQuadVBO = 0;

	// Only created once:
	if (screenQuadVBO == 0)
	{
		const float verts[] = {
			// First triangle:
			 1.0,  1.0,
			-1.0,  1.0,
			-1.0, -1.0,
			// Second triangle:
			-1.0, -1.0,
			 1.0, -1.0,
			 1.0,  1.0
		};

		// NOTE: This VBO is never deleted.
		// Is this a problem worth fixing? It is such a tiny buffer...
		glGenBuffers(1, &screenQuadVBO);
		glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

		vtLogComment("Created VBO for a fullscreen quadrilateral...");
	}

	glDisable(GL_DEPTH_TEST);
	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);

	// Set vertex format:
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Draw 6 vertexes => 2 triangles:
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

// ======================================================
// createShaderProgram():
// ======================================================

GLuint createShaderProgram(const char * vsSrc, const char * fsSrc, const VertexAttrib * const * vtxAttribs)
{
	assert(vsSrc != nullptr);
	assert(fsSrc != nullptr);
	assert(vtxAttribs != nullptr);

	GLuint progId = glCreateProgram();
	GLuint vsId   = glCreateShader(GL_VERTEX_SHADER);
	GLuint fsId   = glCreateShader(GL_FRAGMENT_SHADER);

	if ((progId == 0) || (vsId == 0) || (fsId == 0))
	{
		vtLogError("Problems creating new GL shader program! Failed to allocate GL ids!");
		checkGLErrors(__FILE__, __LINE__);

		if (glIsProgram(progId)) { glDeleteProgram(progId); }
		if (glIsShader(vsId))    { glDeleteShader(vsId);    }
		if (glIsShader(fsId))    { glDeleteShader(fsId);    }

		return 0;
	}

	// Compile & attach Vertex Shader:
	glShaderSource(vsId, 1, (const GLchar **)&vsSrc, nullptr);
	glCompileShader(vsId);
	glAttachShader(progId, vsId);

	// Compile & attach Fragment Shader:
	glShaderSource(fsId, 1, (const GLchar **)&fsSrc, nullptr);
	glCompileShader(fsId);
	glAttachShader(progId, fsId);

	// Set vertex attributes. The list MUST be null terminated:
	for (int i = 0; vtxAttribs[i] != nullptr; ++i)
	{
		glBindAttribLocation(progId, vtxAttribs[i]->index, vtxAttribs[i]->name);
	}

	// Link the shader program:
	glLinkProgram(progId);

	// Print errors/warnings for the just compiled shaders and program:
	printShaderInfoLogs(progId, vsId, fsId);

	// After attached to a program the shader objects can be deleted.
	glDeleteShader(vsId);
	glDeleteShader(fsId);

	checkGLErrors(__FILE__, __LINE__);
	vtLogComment("New GL shader program #" << progId << " created...");

	return progId;
}

// ======================================================
// deleteShaderProgram():
// ======================================================

void deleteShaderProgram(GLuint & shaderProgId)
{
	glDeleteProgram(shaderProgId);
	shaderProgId = 0;
}

// ======================================================
// useShaderProgram():
// ======================================================

void useShaderProgram(const GLuint shaderProgId)
{
	glUseProgram(shaderProgId);
}

// ======================================================
// getShaderProgramUniformLocation():
// ======================================================

GLint getShaderProgramUniformLocation(const GLuint shaderProgId, const char * varName)
{
	assert(varName != nullptr);
	GLint loc = glGetUniformLocation(shaderProgId, varName);
	if (loc < 0)
	{
		vtLogError("Failed to get location for shader uniform \'" << varName << "\'. shaderProgId: " << shaderProgId);
	}
	return loc;
}

// ======================================================
// setShaderProgramUniform(int):
// ======================================================

bool setShaderProgramUniform(const GLint uniformLoc, const int val)
{
	if (uniformLoc >= 0)
	{
		glUniform1i(uniformLoc, val);
		return true;
	}
	return false;
}

// ======================================================
// setShaderProgramUniform(float):
// ======================================================

bool setShaderProgramUniform(const GLint uniformLoc, const float val)
{
	if (uniformLoc >= 0)
	{
		glUniform1f(uniformLoc, val);
		return true;
	}
	return false;
}

// ======================================================
// setShaderProgramUniform(float[]):
// ======================================================

bool setShaderProgramUniform(const GLint uniformLoc, const float * val, const int count)
{
	assert(val != nullptr);
	assert(count > 0);

	if (uniformLoc < 0)
	{
		return false;
	}

	switch (count)
	{
	case 1 : // float
		glUniform1f(uniformLoc, val[0]);
		return true;

	case 2 : // vec2
		glUniform2f(uniformLoc, val[0], val[1]);
		return true;

	case 3 : // vec3
		glUniform3f(uniformLoc, val[0], val[1], val[2]);
		return true;

	case 4 : // vec4
		glUniform4f(uniformLoc, val[0], val[1], val[2], val[3]);
		return true;

	case 16 : // mat4
		glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, val);
		return true;

	default :
		vtLogError("setShaderProgramUniform(float[]) => array length is invalid!");
		return false;
	} // switch (fv.length)
}

// ======================================================
// create2DTexture():
// ======================================================

GLuint create2DTexture(const int w, const int h, const GLenum pixFormat, const GLenum dataType, const GLenum wrapS,
                       const GLenum wrapT, const GLenum minFilter, const GLenum magFilter, const void * const initData)
{
	assert(w > 0);
	assert(h > 0);

	GLuint texId = 0;
	glGenTextures(1, &texId);
	if (texId == 0)
	{
		vtLogError("Failed to generate non-zero GL texture id!");
		checkGLErrors(__FILE__, __LINE__);
		return 0;
	}

	use2DTexture(texId);

	glTexImage2D(
		/* target   = */ GL_TEXTURE_2D,
		/* level    = */ 0,
		/* internal = */ pixFormat,
		/* width    = */ w,
		/* height   = */ h,
		/* border   = */ 0,
		/* format   = */ pixFormat,
		/* type     = */ dataType,
		/* data     = */ initData
	);

	// Set addressing mode:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

	// Set filtering:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	use2DTexture(0);

	checkGLErrors(__FILE__, __LINE__);
	vtLogComment("New physical GL texture #" << texId << " created...");

	// Return the texture handle (still bound to tex2D target!):
	return texId;
}

// ======================================================
// delete2DTexture():
// ======================================================

void delete2DTexture(GLuint & texId)
{
	glDeleteTextures(1, &texId);
	texId = 0;
}

// ======================================================
// use2DTexture():
// ======================================================

void use2DTexture(const GLuint texId, const int texUnit)
{
	assert(texUnit >= 0);
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, texId);
}

// ======================================================
// getCurrent2DTexture():
// ======================================================

GLuint getCurrent2DTexture()
{
	GLint texId = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &texId);
	return texId;
}

// ======================================================
// setPixelStoreAlignment():
// ======================================================

void setPixelStoreAlignment(const int alignment)
{
	glPixelStorei(GL_PACK_ALIGNMENT,   alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}

// ======================================================
// createRenderBuffer():
// ======================================================

GLuint createRenderBuffer(const int w, const int h, const GLenum pixFormat)
{
	assert(w > 0);
	assert(h > 0);

	GLuint rboId = 0;
	glGenRenderbuffers(1, &rboId);
	if (rboId == 0)
	{
		vtLogError("Failed to generate non-zero GL renderbuffer id!");
		checkGLErrors(__FILE__, __LINE__);
		return 0;
	}

	glBindRenderbuffer(GL_RENDERBUFFER, rboId);
	glRenderbufferStorage(GL_RENDERBUFFER, pixFormat, w, h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	checkGLErrors(__FILE__, __LINE__);
	vtLogComment("New GL renderbuffer #" << rboId << " created...");

	return rboId;
}

// ======================================================
// deleteRenderBuffer():
// ======================================================

void deleteRenderBuffer(GLuint & rboId)
{
	glDeleteRenderbuffers(1, &rboId);
	rboId = 0;
}

// ======================================================
// useRenderBuffer():
// ======================================================

void useRenderBuffer(const GLuint rboId)
{
	glBindRenderbuffer(GL_RENDERBUFFER, rboId);
}

// ======================================================
// createFrameBuffer():
// ======================================================

GLuint createFrameBuffer(const int w, const int h, const bool defaultDepthBuffer, const bool defaultStencilBuffer)
{
	assert(w > 0);
	assert(h > 0);

	GLuint fboId = 0;
	glGenFramebuffers(1, &fboId);
	if (fboId == 0)
	{
		vtLogError("Failed to generate non-zero GL framebuffer id!");
		checkGLErrors(__FILE__, __LINE__);
		return 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);

	if (defaultDepthBuffer)
	{
		const GLuint depthRBO = createRenderBuffer(w, h, GL_DEPTH_COMPONENT16);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
	}

	if (defaultStencilBuffer)
	{
		const GLuint stencilRBO = createRenderBuffer(w, h, GL_STENCIL_INDEX8);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilRBO);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	checkGLErrors(__FILE__, __LINE__);
	vtLogComment("New GL framebuffer #" << fboId << " created...");

	return fboId;
}

// ======================================================
// deleteFrameBuffer():
// ======================================================

void deleteFrameBuffer(GLuint & fboId)
{
	glDeleteFramebuffers(1, &fboId);
	fboId = 0;
}

// ======================================================
// useFrameBuffer():
// ======================================================

void useFrameBuffer(const GLuint fboId)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
}

// ======================================================
// validateFrameBuffer():
// ======================================================

bool validateFrameBuffer(const GLuint fboId, std::string * errorString)
{
	if (fboId == 0)
	{
		if (errorString != nullptr)
		{
			(*errorString) = "Null FBO id!";
		}
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	const GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status == GL_FRAMEBUFFER_COMPLETE)
	{
		if (errorString != nullptr)
		{
			errorString->clear();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return true;
	}
	else
	{
		if (errorString != nullptr)
		{
			(*errorString) = fboStatusToString(status);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}
}

// ======================================================
// attachTextureToFrameBuffer():
// ======================================================

void attachTextureToFrameBuffer(const GLuint fboId, const GLuint texId, const int texLevel, const GLenum texTarget, const GLenum attachmentPoint)
{
	assert(fboId != 0);
	assert(texId != 0);

	// According to the documentation here:
	// https://www.khronos.org/opengles/sdk/docs/man/xhtml/glFramebufferTexture2D.xml
	// level must be 0 for GL ES v2.0.
	assert(texLevel == 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, texTarget, texId, texLevel);
		checkGLErrors(__FILE__, __LINE__);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ======================================================
// attachRenderBufferToFrameBuffer():
// ======================================================

void attachRenderBufferToFrameBuffer(const GLuint fboId, const GLuint rboId, const GLenum attachmentPoint)
{
	assert(fboId != 0);
	assert(rboId != 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, rboId);
		checkGLErrors(__FILE__, __LINE__);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ======================================================
// readFrameBuffer():
// ======================================================

void readFrameBuffer(const GLuint fboId, const int xOffs, const int yOffs, const int w, const int h, const GLenum pixFormat, const GLenum dataType, void * destBuffer)
{
	assert(w > 0);
	assert(h > 0);
	assert(fboId != 0);
	assert(destBuffer != nullptr);

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	{
		glReadPixels(xOffs, yOffs, w, h, pixFormat, dataType, destBuffer);
		checkGLErrors(__FILE__, __LINE__);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace gl {}
} // namespace vt {}
