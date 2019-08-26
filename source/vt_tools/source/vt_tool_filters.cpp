
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_filters.cpp
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

#include "vt_tool_filters.hpp"
#include <cassert>
#include <cmath>

/*
 * Most of the code in this file was based on or copied from the
 * NVidia Texture Tools library: http://code.google.com/p/nvidia-texture-tools/
 *
 * ---- From nvimage/Filters.cpp: ----
 *
 * This code is in the public domain -- castanyo@yahoo.es
 * @file Filter.cpp
 * @brief Image filters.
 *
 * Jonathan Blow articles:
 * http://number-none.com/product/Mipmapping, Part 1/index.html
 * http://number-none.com/product/Mipmapping, Part 2/index.html
 *
 * References from Thacher Ulrich:
 * See _Graphics Gems III_ "General Filtered Image Rescaling", Dale A. Schumacher
 * http://tog.acm.org/GraphicsGems/gemsiii/filter.c
 *
 * References from Paul Heckbert:
 * A.V. Oppenheim, R.W. Schafer, Digital Signal Processing, Prentice-Hall, 1975
 *
 * R.W. Hamming, Digital Filters, Prentice-Hall, Englewood Cliffs, NJ, 1983
 *
 * W.K. Pratt, Digital Image Processing, John Wiley and Sons, 1978
 *
 * H.S. Hou, H.C. Andrews, "Cubic Splines for Image Interpolation and
 *      Digital Filtering", IEEE Trans. Acoustics, Speech, and Signal Proc.,
 *      vol. ASSP-26, no. 6, Dec. 1978, pp. 508-517
 *
 * Paul Heckbert's zoom library.
 * http://www.xmission.com/~legalize/zoom.html
 *
 * Reconstruction Filters in Computer Graphics
 * http://www.mentallandscape.com/Papers_siggraph88.pdf
 *
 * More references:
 * http://www.worldserver.com/turk/computergraphics/ResamplingFilters.pdf
 * http://www.dspguide.com/ch16.htm
 *
 */

namespace vt
{
namespace tool
{

// Value of π used in by the filters.
static constexpr float pi = 3.1415926535897931f;

// ======================================================
// Filter:
// ======================================================

Filter::Filter(float w) : width(w)
{
}

float Filter::getWidth() const
{
	return width;
}

float Filter::sampleDelta(float x, float scale) const
{
	return evaluate((x + 0.5f) * scale);
}

float Filter::sampleBox(float x, float scale, int samples) const
{
	float sum = 0;
	float isamples = 1.0f / static_cast<float>(samples);

	for (int s = 0; s < samples; ++s)
	{
		float p = (x + (static_cast<float>(s) + 0.5f) * isamples) * scale;
		float value = evaluate(p);
		sum += value;
	}

	return (sum * isamples);
}

float Filter::sampleTriangle(float x, float scale, int samples) const
{
	float sum = 0;
	float isamples = 1.0f / static_cast<float>(samples);

	for (int s = 0; s < samples; ++s)
	{
		float offset = (2.0f * static_cast<float>(s) + 1.0f) * isamples;
		float p = (x + offset - 0.5f) * scale;
		float value = evaluate(p);

		float weight = offset;
		if (weight > 1.0f)
		{
			weight = 2.0f - weight;
		}

		sum += value * weight;
	}

	return (2.0f * sum * isamples);
}

float Filter::sinc(const float x)
{
	if (std::fabs(x) < 0.0001f)
	{
		return (1.0f + x * x * (-1.0f / 6.0f + x * x * 1.0f / 120.0f));
	}
	else
	{
		return std::sin(x) / x;
	}
}

float Filter::bessel0(const float x)
{
	constexpr float epsilonRatio = 1e-6f;
	float xh, sum, pow, ds;
	int k;

	xh  = 0.5f * x;
	sum = 1.0f;
	pow = 1.0f;
	ds  = 1.0f;
	k   = 0;

	while (ds > (sum * epsilonRatio))
	{
		++k;
		pow = pow * (xh / k);
		ds  = pow * pow;
		sum = sum + ds;
	}

	return sum;
}

std::unique_ptr<Filter> Filter::createFilter(const FilterType type)
{
	switch (type)
	{
	case FilterType::Box       : return std::unique_ptr<Filter>( new BoxFilter       );
	case FilterType::Triangle  : return std::unique_ptr<Filter>( new TriangleFilter  );
	case FilterType::Quadratic : return std::unique_ptr<Filter>( new QuadraticFilter );
	case FilterType::Cubic     : return std::unique_ptr<Filter>( new CubicFilter     );
	case FilterType::BSpline   : return std::unique_ptr<Filter>( new BSplineFilter   );
	case FilterType::Mitchell  : return std::unique_ptr<Filter>( new MitchellFilter  );
	case FilterType::Lanczos   : return std::unique_ptr<Filter>( new LanczosFilter   );
	case FilterType::Sinc      : return std::unique_ptr<Filter>( new SincFilter      );
	case FilterType::Kaiser    : return std::unique_ptr<Filter>( new KaiserFilter    );
	default : assert(false && "Invalid FilterType!");
	} // switch (type)
}

// ======================================================
// BoxFilter:
// ======================================================

BoxFilter::BoxFilter() : Filter(0.5f)
{
}

BoxFilter::BoxFilter(float w) : Filter(w)
{
}

float BoxFilter::evaluate(float x) const
{
	if (std::fabs(x) <= width)
	{
		return 1.0f;
	}
	else
	{
		return 0.0f;
	}
}

// ======================================================
// TriangleFilter:
// ======================================================

TriangleFilter::TriangleFilter() : Filter(1.0f)
{
}

TriangleFilter::TriangleFilter(float w) : Filter(w)
{
}

float TriangleFilter::evaluate(float x) const
{
	x = std::fabs(x);
    if (x < width)
	{
		return width - x;
	}
    return 0.0f;
}

// ======================================================
// QuadraticFilter:
// ======================================================

QuadraticFilter::QuadraticFilter() : Filter(1.5f)
{
}

float QuadraticFilter::evaluate(float x) const
{
	x = std::fabs(x);
	if (x < 0.5f)
	{
		return (0.75f - x * x);
	}
	if (x < 1.5f)
	{
		float t = (x - 1.5f);
		return (0.5f * t * t);
	}
	return 0.0f;
}

// ======================================================
// CubicFilter:
// ======================================================

CubicFilter::CubicFilter() : Filter(1.0f)
{
}

float CubicFilter::evaluate(float x) const
{
	// f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1
	x = std::fabs(x);
	if (x < 1.0f)
	{
		return (2.0f * x - 3.0f) * x * x + 1.0f;
	}
	return 0.0f;
}

// ======================================================
// BSplineFilter:
// ======================================================

BSplineFilter::BSplineFilter() : Filter(2.0f)
{
}

float BSplineFilter::evaluate(float x) const
{
	x = std::fabs(x);
	if (x < 1.0f)
	{
		return (4.0f + x * x * (-6.0f + x * 3.0f)) / 6.0f;
	}
	if (x < 2.0f)
	{
		const float t = (2.0f - x);
		return (t * t * t / 6.0f);
	}
	return 0.0f;
}

// ======================================================
// MitchellFilter:
// ======================================================

MitchellFilter::MitchellFilter() : Filter(2.0f)
{
	setParameters(1.0f / 3.0f, 1.0f / 3.0f);
}

float MitchellFilter::evaluate(float x) const
{
	x = std::fabs(x);
	if (x < 1.0f)
	{
		return (p0 + x * x * (p2 + x * p3));
	}
	if (x < 2.0f)
	{
		return (q0 + x * (q1 + x * (q2 + x * q3)));
	}
	return 0.0f;
}

void MitchellFilter::setParameters(float b, float c)
{
	p0 = (6.0f -  2.0f * b) / 6.0f;
	p2 = (-18.0f + 12.0f * b + 6.0f * c) / 6.0f;
	p3 = (12.0f - 9.0f * b - 6.0f * c) / 6.0f;
	q0 = (8.0f * b + 24.0f * c) / 6.0f;
	q1 = (-12.0f * b - 48.0f * c) / 6.0f;
	q2 = (6.0f * b + 30.0f * c) / 6.0f;
	q3 = (-b - 6.0f * c) / 6.0f;
}

// ======================================================
// LanczosFilter:
// ======================================================

LanczosFilter::LanczosFilter() : Filter(3.0f)
{
}

float LanczosFilter::evaluate(float x) const
{
	x = std::fabs(x);
	if (x < 3.0f)
	{
		return Filter::sinc(pi * x) * Filter::sinc(pi * x / 3.0f);
	}
	return 0.0f;
}

// ======================================================
// SincFilter:
// ======================================================

SincFilter::SincFilter(float w) : Filter(w)
{
}

float SincFilter::evaluate(float x) const
{
	return Filter::sinc(pi * x);
}

// ======================================================
// KaiserFilter:
// ======================================================

KaiserFilter::KaiserFilter(float w) : Filter(w)
{
	setParameters(4.0f, 1.0f);
}

float KaiserFilter::evaluate(float x) const
{
	const float sincValue = Filter::sinc(pi * x * stretch);
	const float t = (x / width);
	if ((1.0f - t * t) >= 0.0f)
	{
		return sincValue * Filter::bessel0(alpha * std::sqrt(1.0f - t * t)) / Filter::bessel0(alpha);
	}
	return 0.0f;
}

void KaiserFilter::setParameters(float a, float s)
{
	alpha   = a;
	stretch = s;
}

// ======================================================
// PolyphaseKernel:
// ======================================================

PolyphaseKernel::PolyphaseKernel(const Filter & f, unsigned int srcLength, unsigned int dstLength, int samples)
{
	assert(samples > 0);

	float scale = static_cast<float>(dstLength) / static_cast<float>(srcLength);
	const float iscale = (1.0f / scale);

	if (scale > 1)
	{
		// Upsampling...
		samples = 1;
		scale   = 1;
	}

	length = dstLength;
	width  = f.getWidth() * iscale;
	windowSize = static_cast<int>(std::ceil(width * 2.0f)) + 1;

	data = new float[windowSize * length];
	std::memset(data, 0, (windowSize * length * sizeof(float)));

	for (unsigned int i = 0; i < length; ++i)
	{
		const float center = (0.5f + i) * iscale;
		const int left  = static_cast<int>(std::floor(center - width));
		const int right = static_cast<int>(std::ceil(center + width));
		assert(right - left <= windowSize);

		float total = 0.0f;
		for (int j = 0; j < windowSize; ++j)
		{
			const float sample = f.sampleBox(left + j - center, scale, samples);
			data[i * windowSize + j] = sample;
			total += sample;
		}

		// Normalize weights:
		for (int j = 0; j < windowSize; ++j)
		{
			data[i * windowSize + j] /= total;
		}
	}
}

PolyphaseKernel::~PolyphaseKernel()
{
	delete[] data;
}

float PolyphaseKernel::getValueAt(unsigned int column, unsigned int x) const
{
	assert(column < length);
	assert(x < static_cast<unsigned int>(windowSize));
	return data[column * windowSize + x];
}

} // namespace tool {}
} // namespace vt {}
