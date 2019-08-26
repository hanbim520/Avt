
// ================================================================================================
// -*- C++ -*-
// File: vt_common.hpp
// Author: Guilherme R. Lampert
// Created on: 10/09/14
// Brief: Miscellaneous utilities used internally by the VT library.
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

#ifndef VTLIB_VT_COMMON_HPP
#define VTLIB_VT_COMMON_HPP

#ifndef VT_EXTRA_DEBUG
	#define VT_EXTRA_DEBUG 1
#endif // VT_EXTRA_DEBUG

#ifndef VT_EXTRA_GL_ERROR_CHECKING
	#define VT_EXTRA_GL_ERROR_CHECKING 1
#endif // VT_EXTRA_GL_ERROR_CHECKING

#ifndef VT_THREAD_SAFE_VTFF_PAGE_FILE
	#define VT_THREAD_SAFE_VTFF_PAGE_FILE 1
#endif // VT_THREAD_SAFE_VTFF_PAGE_FILE

// ======================================================
// Common includes:
// ======================================================

// Standard C library:
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// Standard C++ library / STL:
#include <type_traits>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <string>
#include <array>
#include <tuple>

// ======================================================
// VT log macros:
// ======================================================

// Logging can be permanently disabled by defining
// VT_NO_LOGGING at the global scope.

#ifndef VT_NO_LOGGING

#define vtLogComment(x)\
{\
	std::ostringstream ostr;\
	ostr << "VT-LOG...: " << x;\
	::vt::getLogCallbacks().logComment(ostr.str());\
}

#define vtLogWarning(x)\
{\
	std::ostringstream ostr;\
	ostr << "VT-WARN..: " << x;\
	::vt::getLogCallbacks().logWarning(ostr.str());\
}

#define vtLogError(x)\
{\
	std::ostringstream ostr;\
	ostr << "VT-ERROR.: " << x;\
	::vt::getLogCallbacks().logError(ostr.str());\
}

#define vtFatalError(x)\
{\
	std::ostringstream ostr;\
	ostr << "VT-FATAL.: " << x;\
	::vt::getLogCallbacks().logError(ostr.str());\
	::vt::gl::checkGLErrors(__FILE__, __LINE__);\
	throw ::vt::Exception(ostr.str());\
}

#else // VT_NO_LOGGING defined

// No-ops:
#define vtLogComment(x)
#define vtLogWarning(x)
#define vtFatalError(x)
#define vtLogError(x)

#endif // VT_NO_LOGGING

// ======================================================
//
// The debug macros
//
//   VT_EXTRA_GL_ERROR_CHECKING
//   VT_EXTRA_DEBUG
//
// Can be defined to enable extra debugging facilities
// in the VT library. They are both recommended to be
// globally defined for a debug build of the library.
//
// Additionally, VT_NO_LOGGING can be defined to
// completely disable internal library logging.
//
// ======================================================

//
// VT library types and functions are always
// defined inside this namespace.
//
namespace vt
{

// ======================================================
// User callbacks:
// ======================================================

//
// Allows the user to provide custom log callbacks
// for the internal VT library log. If not custom log
// is set, a default one is provided, which writes
// normal output to STDOUT and errors to STDERR.
//
struct LogCallbacks
{
	virtual void logComment(const std::string & message) = 0;
	virtual void logWarning(const std::string & message) = 0;
	virtual void logError(const std::string & message) = 0;
	virtual ~LogCallbacks() = default;
};

// Get the currently installed LogCallbacks.
LogCallbacks & getLogCallbacks() noexcept;

// ======================================================
// Shared VT shader programs:
// ======================================================

//
// All the shader programs used by the VT library.
// They are created by libraryInit() and destroyed by libraryShutdown().
// Each shader entry is the GL program id plus its uniform variables.
//
struct GlobalShaders
{
	// Used to render a quadrilateral for debug
	// visualization of the VT indirection table.
	// Does a re-coloring of the texture to aid visualization.
	struct {
		GLuint programId;
		GLint  unifTextureSamp;          // sampler2D
		GLint  unifNdcQuadScale;         // vec2
	} drawIndirectionTable;

	// Used to render a quadrilateral for debug
	// visualization of the VT page cache table.
	struct {
		GLuint programId;
		GLint  unifTextureSamp;          // sampler2D
		GLint  unifNdcQuadScale;         // vec2
	} drawPageTable;

	// Used for debug text rendering.
	struct {
		GLuint programId;
		GLint  unifTextureSamp;          // sampler2D
		GLint  unifMvpMatrix;            // mat4
	} drawText2D;

	// Used to render the page-id pre-pass.
	struct {
		GLuint programId;
		GLint  unifMvpMatrix;            // mat4
		// Fragment Shader params:
		GLuint unifLog2MipScaleFactor;   // float
		GLuint unifVTMaxMipLevel;        // float
		GLuint unifVTIndex;              // float
		GLuint unifVTSizePixels;         // vec2
		GLuint unifVTSizePages;          // vec2
	} pageIdGenPass;

	// Used to render the final textured scene using the Virtual Texture.
	// This is a simple test shader that renders diffuse colors only; unlit.
	struct {
		GLuint programId;
		GLint  unifMvpMatrix;            // mat4
		// Fragment Shader params:
		GLint  unifMipSampleBias;        // float
		GLint  unifPageTableSamp;        // sampler2D
		GLint  unifIndirectionTableSamp; // sampler2D
	} vtRenderSimple;

	struct {
		GLuint programId;
		// Vertex Shader params:
		GLint  unifMvpMatrix;            // mat4
		GLint  unifLightPosObjectSpace;  // vec4
		GLint  unifViewPosObjectSpace;   // vec4
		// Fragment Shader params:
		GLint  unifMipSampleBias;        // float
		GLint  unifDiffuseSamp;          // sampler2D
		GLint  unifNormalSamp;           // sampler2D
		GLint  unifSpecularSamp;         // sampler2D
		GLint  unifIndirectionTableSamp; // sampler2D
	} vtRenderLit;
};

// Get a reference to the set of shader programs used by the VT library.
const GlobalShaders & getGlobalShaders() noexcept;

// ======================================================
// NonCopyable:
// ======================================================

//
// Any class/struct inheriting from this little helper
// won't be copyable nor movable.
//
struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable & operator = (const NonCopyable &) = delete;
};

// ======================================================
// VT Exception type:
// ======================================================

//
// Exception type thrown by vtFatalError().
//
class Exception : public std::runtime_error
{
public:
	Exception(const char * error)
		: std::runtime_error(error) { }

	Exception(const std::string & error)
		: std::runtime_error(error) { }
};

// ======================================================
// Pixel4b:
// ======================================================

//
// RGBA 8:8:8:8 pixel.
// Each channel ranges from 0 to 255.
//
struct Pixel4b
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

// This type must be strictly sized.
static_assert(sizeof(Pixel4b) == 4, "Expected 4 bytes size!");

// ======================================================
// PageId type:
// ======================================================

// Type alias:
using PageId = uint32_t;
constexpr PageId InvalidPageId = 0xFFFFFFFF;

// Access each component of a page id:
inline int pageIdExtractPageX        (const PageId pageId) noexcept { return ((pageId & 0x000000FF) >>  0); } // R
inline int pageIdExtractPageY        (const PageId pageId) noexcept { return ((pageId & 0x0000FF00) >>  8); } // G
inline int pageIdExtractMipLevel     (const PageId pageId) noexcept { return ((pageId & 0x00FF0000) >> 16); } // B
inline int pageIdExtractTextureIndex (const PageId pageId) noexcept { return ((pageId & 0xFF000000) >> 24); } // A

// Pack values into a page id:
inline PageId makePageId(unsigned int x, unsigned int y, unsigned int level, unsigned int texId) noexcept
{
	return static_cast<PageId>(((texId & 0xFF) << 24) | ((level & 0xFF) << 16) | ((y & 0xFF) << 8) | (x & 0xFF));
}

// ======================================================
// Inline functions / miscellaneous helpers:
// ======================================================

// Length of statically allocated C++ arrays.
template<typename T, size_t N>
constexpr size_t arrayLength(const T (&)[N]) noexcept
{
	return N;
}

// Zero fills a POD type, such as a C struct or union.
template<typename T>
void clearPodObject(T & s) noexcept
{
	static_assert(std::is_pod<T>::value, "Type must be Plain Old Data!");
	std::memset(&s, 0, sizeof(T));
}

// Zero fills a statically allocated array of POD or built-in types. Array length inferred by the compiler.
template<typename T, size_t N>
void clearArray(T (&arr)[N]) noexcept
{
	static_assert(std::is_pod<T>::value, "Type must be Plain Old Data!");
	std::memset(arr, 0, sizeof(T) * N);
}

// Zero fills an array of POD or built-in types, with array length provided by the caller.
template<typename T>
void clearArray(T * arrayPtr, const size_t arrayLength) noexcept
{
	static_assert(std::is_pod<T>::value, "Type must be Plain Old Data!");
	assert(arrayPtr != nullptr && arrayLength != 0);
	std::memset(arrayPtr, 0, sizeof(T) * arrayLength);
}

// Zero fills (memsets) an std::array. This method performs a bitwise zero-fill
// on the array, therefore, can only be used on arrays of POD objects.
template<typename T, size_t N>
void clearArray(std::array<T, N> & arr) noexcept
{
	static_assert(std::is_pod<T>::value, "Type must be Plain Old Data!");
	std::memset(arr.data(), 0, sizeof(T) * N);
}

// Clamp any value within min/max range, inclusive.
template<typename T>
constexpr T clamp(const T x, const T minimum, const T maximum) noexcept
{
	return (x < minimum) ? minimum : (x > maximum) ? maximum : x;
}

// Clamp an integer to 255 max.
inline uint8_t clampByte(const unsigned int b) noexcept
{
	if (b > 255)
	{
		return 255;
	}
	return static_cast<uint8_t>(b);
}

// Reverse all bits in a byte (not the same as ~).
inline uint8_t reverseByte(const unsigned int b) noexcept
{
	// Algorithm from:
	// https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
	//
	return static_cast<uint8_t>(((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
}

} // namespace vt {}

#endif // VTLIB_VT_COMMON_HPP
