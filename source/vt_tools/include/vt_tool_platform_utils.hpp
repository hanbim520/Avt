
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_platform_utils.hpp
// Author: Guilherme R. Lampert
// Created on: 27/10/14
// Brief: Platform dependent utilities and helpers.
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

#ifndef VT_TOOL_PLATFORM_UTILS_HPP
#define VT_TOOL_PLATFORM_UTILS_HPP

namespace vt
{
namespace tool
{

// ======================================================
// Miscellaneous helper functions:
// ======================================================

// Path & File System helpers:
std::string getUserHomePath();
bool createDirectory(const char * basePath, const char * directoryName);

// Timing / FPS:
int64_t getClockMillisec();
unsigned int getFramesPerSecondCount();

// Screen management helpers:
void getScreenDimensionsInPixels(int & screenWidth, int & screenHeight);

} // namespace tool {}
} // namespace vt {}

#endif // VT_TOOL_PLATFORM_UTILS_HPP
