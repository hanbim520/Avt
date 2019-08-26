
// ================================================================================================
// -*- C++ -*-
// File: vectormath.hpp
// Author: Guilherme R. Lampert
// Created on: 07/08/14
// Brief: Interface header for Sony's VectorMath library.
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

#ifndef VECTORMATH_HPP
#define VECTORMATH_HPP

#include <cmath>
#include "vectormath/scalar/cpp/vectormath_aos.h"

// Makes all VectorMath types visible.
using namespace ::Vectormath::Aos;

// Get the name of the VectorMath implementation currently in use.
inline const char * VectorMathImplementationString()
{
#if defined (VMATH_SPU)
	return "Sony's VectorMath, SPU - AoS format";
#elif defined (VMATH_ALTIVEC)
	return "Sony's VectorMath, ALTIVEC - AoS format";
#elif defined (VMATH_SSE)
	return "Sony's VectorMath, SSE - AoS format";
#else // VMATH_SCALAR
	return "Sony's VectorMath, Scalar - AoS format";
#endif
}

//
// Cast vectors/matrices/quaternions to float pointers (float *):
//
inline float * toFloatPtr(Point3  & p) { return reinterpret_cast<float *>(&p); }
inline float * toFloatPtr(Vector3 & v) { return reinterpret_cast<float *>(&v); }
inline float * toFloatPtr(Vector4 & v) { return reinterpret_cast<float *>(&v); }
inline float * toFloatPtr(Quat    & q) { return reinterpret_cast<float *>(&q); }
inline float * toFloatPtr(Matrix3 & m) { return reinterpret_cast<float *>(&m); }
inline float * toFloatPtr(Matrix4 & m) { return reinterpret_cast<float *>(&m); }

//
// Cast vectors/matrices/quaternions to const float pointers (const float *):
//
inline const float * toFloatPtr(const Point3  & p) { return reinterpret_cast<const float *>(&p); }
inline const float * toFloatPtr(const Vector3 & v) { return reinterpret_cast<const float *>(&v); }
inline const float * toFloatPtr(const Vector4 & v) { return reinterpret_cast<const float *>(&v); }
inline const float * toFloatPtr(const Quat    & q) { return reinterpret_cast<const float *>(&q); }
inline const float * toFloatPtr(const Matrix3 & m) { return reinterpret_cast<const float *>(&m); }
inline const float * toFloatPtr(const Matrix4 & m) { return reinterpret_cast<const float *>(&m); }

//
// degToRad/radToDeg: degrees <=> radians conversion:
//
inline constexpr float degToRad(const float degrees)
{
	return (degrees * (M_PI / 180.0f));
}
inline constexpr float radToDeg(const float radians)
{
	return (radians * (180.0f / M_PI));
}

//
// Time scale conversions:
//
inline constexpr double msecToSec(const double ms)
{
	return (ms * 0.001);
}
inline constexpr double secToMsec(const double sec)
{
	return (sec * 1000.0);
}

#endif // VECTORMATH_HPP
