
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_filters.hpp
// Author: Guilherme R. Lampert
// Created on: 23/10/14
// Brief: Image/texture filters for texture scaling and mip-map construction.
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

#ifndef VT_TOOL_FILTERS_HPP
#define VT_TOOL_FILTERS_HPP

#include <memory>

namespace vt
{
namespace tool
{

// ======================================================
// FilterType:
// ======================================================

//
// Types of image filters available.
// Can be used with the Filter::createFilter() factory.
//
enum class FilterType
{
	Box,
	Triangle,
	Quadratic,
	Cubic,
	BSpline,
	Mitchell,
	Lanczos,
	Sinc,
	Kaiser
};

// ======================================================
// Filter:
// ======================================================

//
// Base class for the concrete filter types.
// Also has a factory method that creates filters by id.
//
class Filter
{
public:

	Filter(float w);
	virtual ~Filter() = default;

	float getWidth() const;
	float sampleDelta(float x, float scale) const;
	float sampleBox(float x, float scale, int samples) const;
	float sampleTriangle(float x, float scale, int samples) const;

	// Template method implemented by the specialized filters.
	virtual float evaluate(float x) const = 0;

public:

	// Creates concrete Filter instances from the enum id.
	static std::unique_ptr<Filter> createFilter(FilterType type);

	// Common 'sinc' function.
	// See: http://en.wikipedia.org/wiki/Sinc_function
	static float sinc(float x);

	// Bessel function of the first kind from Jon Blow's article.
	// http://mathworld.wolfram.com/BesselFunctionoftheFirstKind.html
	// http://en.wikipedia.org/wiki/Bessel_function
	static float bessel0(float x);

protected:

	const float width;
};

// ======================================================
// Box filter:
// ======================================================

class BoxFilter : public Filter
{
public:
	BoxFilter();
	BoxFilter(float w);
	float evaluate(float x) const override;
};

// ======================================================
// Triangle (bilinear/tent) filter:
// ======================================================

class TriangleFilter : public Filter
{
public:
	TriangleFilter();
	TriangleFilter(float w);
	float evaluate(float x) const override;
};

// ======================================================
// Quadratic (bell) filter:
// ======================================================

class QuadraticFilter : public Filter
{
public:
	QuadraticFilter();
	float evaluate(float x) const override;
};

// ======================================================
// Cubic filter:
// ======================================================

class CubicFilter : public Filter
{
public:
	CubicFilter();
	float evaluate(float x) const override;
};

// ======================================================
// Cubic B-Spline filter:
// ======================================================

class BSplineFilter : public Filter
{
public:
	BSplineFilter();
	float evaluate(float x) const override;
};

// ======================================================
// Mitchell & Netravali's two-param cubic:
// ======================================================

class MitchellFilter : public Filter
{
public:
	MitchellFilter();
	float evaluate(float x) const override;
	void setParameters(float b, float c);

private:
	float p0, p2, p3;
	float q0, q1, q2, q3;
};

// ======================================================
// Lanczos-3 filter:
// ======================================================

class LanczosFilter : public Filter
{
public:
	LanczosFilter();
	float evaluate(float x) const override;
};

// ======================================================
// Sinc filter:
// ======================================================

class SincFilter : public Filter
{
public:
	SincFilter(float w = 3.0f);
	float evaluate(float x) const override;
};

// ======================================================
// Kaiser filter:
// ======================================================

class KaiserFilter : public Filter
{
public:
	KaiserFilter(float w = 3.0f);
	float evaluate(float x) const override;
	void setParameters(float a, float stretch);

private:
	float alpha;
	float stretch;
};

// ======================================================
// PolyphaseKernel: A 1D polyphase kernel
// ======================================================

class PolyphaseKernel
{
public:
	PolyphaseKernel(const Filter & f, unsigned int srcLength, unsigned int dstLength, int samples = 32);
	~PolyphaseKernel();

	// No copy or assignment.
	PolyphaseKernel(const PolyphaseKernel &) = delete;
	PolyphaseKernel & operator = (const PolyphaseKernel &) = delete;

	// Accessors:
	unsigned int getLength() const { return length;     }
	int   getWindowSize()    const { return windowSize; }
	float getWidth()         const { return width;      }

	// Bounds checked with assert().
	float getValueAt(unsigned int column, unsigned int x) const;

private:

	int windowSize;
	unsigned int length;
	float width;
	float * data;
};

} // namespace tool {}
} // namespace vt {}

#endif // VT_TOOL_FILTERS_HPP
