
// ================================================================================================
// -*- C++ -*-
// File: vt_tool_platform_utils.mm
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

// C++ library:
#import <string>
#import <cassert>
#import <cstdint>

// CoreFoundation/iOS/NS stuff:
#import <Foundation/Foundation.h>
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR || defined(__IPHONEOS__)
#import <CoreGraphics/CGGeometry.h>
#import <UIKit/UIKit.h>
#endif

// mach_absolute_time()/mach_timebase_info() (OSX an iOS).
#include <mach/mach_time.h>

namespace vt
{
namespace tool
{

// ======================================================
// getUserHomePath():
// ======================================================

std::string getUserHomePath()
{
	NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	return [paths[0] cStringUsingEncoding:NSUTF8StringEncoding];
}

// ======================================================
// createDirectory():
// ======================================================

bool createDirectory(const char * basePath, const char * directoryName)
{
	assert(basePath      != nullptr);
	assert(directoryName != nullptr);

	NSString * dir  = [NSString stringWithUTF8String:directoryName];
	NSString * path = [NSString stringWithUTF8String:basePath];

	NSString * filePathAndDirectory = [path stringByAppendingPathComponent:dir];
	NSError  * error;

	if (![[NSFileManager defaultManager] createDirectoryAtPath:filePathAndDirectory
		withIntermediateDirectories:YES attributes:nil error:&error])
	{
		NSLog(@"Create directory error: %@", error);
		return false;
	}

	return true;
}

// ======================================================
// getClockMillisec():
// ======================================================

int64_t getClockMillisec()
{
	static int64_t clockFreq;
	static int64_t baseTime;
	static bool    timerInitialized = false;

	// Get the clock frequency once. This value never
	// changes while the system is running.
	if (!timerInitialized)
	{
		baseTime = static_cast<int64_t>(mach_absolute_time());

		mach_timebase_info_data_t timeBaseInfo;
		mach_timebase_info(&timeBaseInfo);
		clockFreq = (timeBaseInfo.numer / timeBaseInfo.denom);

		timerInitialized = true;
	}

	// clockFreq is in nanosecond precision (Not Bad!)
	return ((static_cast<int64_t>(mach_absolute_time()) - baseTime) * clockFreq) / 1000000;
}

// ======================================================
// getFramesPerSecondCount():
// ======================================================

unsigned int getFramesPerSecondCount()
{
	// Average multiple frames together to smooth changes out a bit.
	static const int MaxFPSFrames = 4;
	static int64_t previousTimes[MaxFPSFrames];
	static int64_t previousTime, fps;
	static int index;

	const int64_t timeMillisec = getClockMillisec(); // Real time clock
	const int64_t frameTime    = (timeMillisec - previousTime);

	previousTimes[index++] = frameTime;
	previousTime = timeMillisec;

	if (index == MaxFPSFrames)
	{
		int64_t total = 0;
		for (int i = 0; i < MaxFPSFrames; ++i)
		{
			total += previousTimes[i];
		}

		if (total == 0)
		{
			total = 1;
		}

		fps = (10000 * MaxFPSFrames / total);
		fps = (fps + 5) / 10;
		index = 0;
	}

	return static_cast<unsigned int>(fps);
}

// ======================================================
// getScreenDimensionsInPixels():
// ======================================================

void getScreenDimensionsInPixels(int & screenWidth, int & screenHeight)
{
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR || defined(__IPHONEOS__)
	const CGRect  screenBounds = [[UIScreen mainScreen] bounds];
//    const CGFloat screenScale  = [[UIScreen mainScreen] scale];
//    const CGSize  screenSize   = CGSizeMake(screenBounds.size.width * screenScale, screenBounds.size.height * screenScale);
    const CGSize  screenSize   = CGSizeMake(screenBounds.size.width , screenBounds.size.height);
    screenWidth  = screenSize.width;
	screenHeight = screenSize.height;
#else
	// TODO: This is only available for iOS and simulator, for the time being.
	screenWidth = screenHeight = 0;
	assert(false && "getScreenDimensionsInPixels() not implemented!");
#endif
}

} // namespace tool {}
} // namespace vt {}
