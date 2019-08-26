
// ================================================================================================
// -*- C++ -*-
// File: vt.hpp
// Author: Guilherme R. Lampert
// Created on: 09/09/14
// Brief: Master include for the C++ Virtual Texturing library.
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

#ifndef VTLIB_VT_HPP
#define VTLIB_VT_HPP

// Library misc:
#include "vt_opengl.hpp"
#include "vt_common.hpp"

// Texture tables (backed by GL textures):
#include "vt_page_table.hpp"
#include "vt_page_indirection_table.hpp"

// Other auxiliary components:
#include "vt_page_file.hpp"
#include "vt_page_provider.hpp"
#include "vt_page_resolver.hpp"
#include "vt_page_cache_mgr.hpp"

// The texture interface:
#include "vt_virtual_texture.hpp"

namespace vt
{

// ======================================================
// Library version queries:
// ======================================================

// Printable library version string. Formatted as "major.minor.build".
std::string getLibraryVersionString();

// Get library version numbers.
void getLibraryVersionNumbers(int & major, int & minor, int & build) noexcept;

// ======================================================
// IndirectionTableFormat:
// ======================================================

// Currently, the underlaying indirection table format has
// to be selected at startup, because that's when shader programs
// are loaded. All indirection tables created during the program
// lifetime will have to use the format selected at startup.
//
// - Rgb565 format is more compact, using less memory, at a more
//   elevated address translation cost.
//
// - Rgba8888 format uses more memory than the previous,
//   but address translation is a little cheaper with this format.
//
enum class IndirectionTableFormat
{
	Rgb565,
	Rgba8888
};

// ======================================================
// Global library initialization and shutdown:
// ======================================================

// Initialize the VT library. This must be called before performing any other library operation.
// This function is not thread safe and must be called from the main thread.
// 'logCallbacks' may be null to use the default (STDOUT). If provided, the pointer
// must to remain valid until libraryShutdown() is called.
void libraryInit(IndirectionTableFormat format, LogCallbacks * logCallbacks = nullptr);

// Shutdown the VT library. This must be called on the main thread,
// usually before terminating the application.
void libraryShutdown();

} // namespace vt {}

#endif // VTLIB_VT_HPP
