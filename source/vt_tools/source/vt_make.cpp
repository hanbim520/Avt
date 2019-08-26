
// ================================================================================================
// -*- C++ -*-
// File: vt_make.cpp
// Author: Guilherme R. Lampert
// Created on: 22/11/14
// Brief: Simple command line tool that creates a VT page file from a source image.
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

// Virtual Texturing Library:
#include "vt_tool_pagefile_builder.hpp"

// Standard Library:
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

/*
 * Usage:
 *
 * $ vtmake <input_file> <output_file> [--flags]
 * (Currently, args have to be in this specific order!)
 *
 * Flags accepted:
 *
 * --help           : prints help text with list of commands
 * --filter         : PageFileBuilderOptions::textureFilter         (str)
 * --page_size      : PageFileBuilderOptions::pageSizePixels        (int)
 * --content_size   : PageFileBuilderOptions::pageContentSizePixels (int)
 * --border_size    : PageFileBuilderOptions::pageBorderSizePixels  (int)
 * --max_levels     : PageFileBuilderOptions::maxMipLevels          (int)
 * --flip_v_src     : PageFileBuilderOptions::flipSourceVertically  (bool)
 * --flip_v_tiles   : PageFileBuilderOptions::flipTilesVertically   (bool)
 * --stop_on_1_mip  : PageFileBuilderOptions::stopOn1PageMip        (bool)
 * --add_debug_info : PageFileBuilderOptions::addDebugInfoToPages   (bool)
 * --dump_images    : PageFileBuilderOptions::dumpPageImages        (bool)
 * --verbose        : PageFileBuilderOptions::stdoutVerbose         (bool)
 */

namespace {

// ======================================================
// printHelpAndExit()
// ======================================================

void printHelpAndExit(const char * progName)
{
	std::printf("\n"
	"Usage:\n"
	"$ %s <input_file> <output_file> [--flags=]\n"
	"\n"
	"Flags accepted:\n"
	" --help           : prints help text with list of commands.\n"
	" --filter         : (str)  type of mipmapping filter: box, tri, quad, cubic, bspline, mitchell, lanczos, sinc, kaiser.\n"
	" --page_size      : (int)  total page size in pixels, including border.\n"
	" --content_size   : (int)  size in pixels of page content, not including border.\n"
	" --border_size    : (int)  size in pixels of the page border.\n"
	" --max_levels     : (int)  max mipmap levels to generate.\n"
	" --flip_v_src     : (bool) flip the source image vertically.\n"
	" --flip_v_tiles   : (bool) flip each individual tile/page vertically.\n"
	" --stop_on_1_mip  : (bool) stop subdividing when mip 0 is reached.\n"
	" --add_debug_info : (bool) print debug text to each page.\n"
	" --dump_images    : (bool) dump each page as an image file (TGA format).\n"
	" --verbose        : (bool) print stuff to STDOUT while running.\n"
	"\n", progName);
	std::exit(0);
}

// ======================================================
// errorExit():
// ======================================================

void errorExit(const char * format, ...)
{
	va_list vaList;
	char buffer[1024];

	va_start(vaList, format);
	std::vsnprintf(buffer, sizeof(buffer), format, vaList);
	va_end(vaList);

	buffer[sizeof(buffer) - 1] = '\0'; // Ensure a null at the end
	throw vt::tool::PageFileBuilderError(buffer);
}

// ======================================================
// skipToValue():
// ======================================================

const char * skipToValue(const char * str)
{
	// Find the value after the '=' sign, if any:
	while ((*str != '=') && (*str != '\0'))
	{
		++str;
	}
	if (*str == '=')
	{
		++str;
	}

	// Return either what was after the '=' or an empty string.
	return str;
}

// ======================================================
// parseFilterName():
// ======================================================

vt::tool::FilterType parseFilterName(const char * str)
{
	str = skipToValue(str);

	if (std::strcmp(str, "box"     ) == 0) { return vt::tool::FilterType::Box;       }
	if (std::strcmp(str, "tri"     ) == 0) { return vt::tool::FilterType::Triangle;  }
	if (std::strcmp(str, "quad"    ) == 0) { return vt::tool::FilterType::Quadratic; }
	if (std::strcmp(str, "cubic"   ) == 0) { return vt::tool::FilterType::Cubic;     }
	if (std::strcmp(str, "bspline" ) == 0) { return vt::tool::FilterType::BSpline;   }
	if (std::strcmp(str, "mitchell") == 0) { return vt::tool::FilterType::Mitchell;  }
	if (std::strcmp(str, "lanczos" ) == 0) { return vt::tool::FilterType::Lanczos;   }
	if (std::strcmp(str, "sinc"    ) == 0) { return vt::tool::FilterType::Sinc;      }
	if (std::strcmp(str, "kaiser"  ) == 0) { return vt::tool::FilterType::Kaiser;    }

	std::printf("WARNING: Unknown filter '%s'! Defaulting to box filter.\n", str);
	return vt::tool::FilterType::Box;
}

// ======================================================
// parseInt():
// ======================================================

int parseInt(const char * str)
{
	return std::stoi(skipToValue(str));
}

// ======================================================
// parseBool():
// ======================================================

bool parseBool(const char * str)
{
	str = skipToValue(str);

	if ((std::strcmp(str, "false") == 0) ||
		(std::strcmp(str, "no")    == 0) ||
		(std::strcmp(str, "0")     == 0))
	{
		return false;
	}

	// Assume true for anything else, including an invalid value or an empty string.
	// (results in true for "--flag" with no "=value" part)
	return true;
}

// ======================================================
// startsWith():
// ======================================================

bool startsWith(const char * str, const char * prefix)
{
	const size_t prefixLen = std::strlen(prefix);
	if (prefixLen == 0)
	{
		return false;
	}
	return std::strncmp(str, prefix, prefixLen) == 0;
}

// ======================================================
// parseCmdLine():
// ======================================================

void parseCmdLine(const int argc, const char * argv[], std::string & inputFile,
                  std::string & outputFile, vt::tool::PageFileBuilderOptions & cmdLineOpts)
{
	// Possible "--help" call
	if ((argc == 2) && startsWith(argv[1], "--help"))
	{
		printHelpAndExit(argv[0]);
	}

	// Must have at least argv[0], in_file and out_file
	if (argc < 3)
	{
		errorExit("Not enough arguments!");
	}

	inputFile  = argv[1];
	outputFile = argv[2];

	for (int i = 3; i < argc; ++i)
	{
		if (startsWith(argv[i], "--help"))
		{
			printHelpAndExit(argv[0]);
		}
		else if (startsWith(argv[i], "--filter"))
		{
			cmdLineOpts.textureFilter = parseFilterName(argv[i]);
		}
		else if (startsWith(argv[i], "--page_size"))
		{
			cmdLineOpts.pageSizePixels = parseInt(argv[i]);
		}
		else if (startsWith(argv[i], "--content_size"))
		{
			cmdLineOpts.pageContentSizePixels = parseInt(argv[i]);
		}
		else if (startsWith(argv[i], "--border_size"))
		{
			cmdLineOpts.pageBorderSizePixels = parseInt(argv[i]);
		}
		else if (startsWith(argv[i], "--max_levels"))
		{
			cmdLineOpts.maxMipLevels = parseInt(argv[i]);
		}
		else if (startsWith(argv[i], "--flip_v_src"))
		{
			cmdLineOpts.flipSourceVertically = parseBool(argv[i]);
		}
		else if (startsWith(argv[i], "--flip_v_tiles"))
		{
			cmdLineOpts.flipTilesVertically = parseBool(argv[i]);
		}
		else if (startsWith(argv[i], "--stop_on_1_mip"))
		{
			cmdLineOpts.stopOn1PageMip = parseBool(argv[i]);
		}
		else if (startsWith(argv[i], "--add_debug_info"))
		{
			cmdLineOpts.addDebugInfoToPages = parseBool(argv[i]);
		}
		else if (startsWith(argv[i], "--dump_images"))
		{
			cmdLineOpts.dumpPageImages = parseBool(argv[i]);
		}
		else if (startsWith(argv[i], "--verbose"))
		{
			cmdLineOpts.stdoutVerbose = parseBool(argv[i]);
		}
		else
		{
			std::printf("WARNING: Unknown command line argument: '%s'\n", argv[i]);
		}
	}

	if (cmdLineOpts.stdoutVerbose)
	{
		std::printf("Input  file: \"%s\"\n", inputFile.c_str());
		std::printf("Output file: \"%s\"\n", outputFile.c_str());
		cmdLineOpts.printSelf();
	}
}

// ======================================================
// runPageFileBuilder():
// ======================================================

void runPageFileBuilder(const std::string & inputFile, const std::string & outputFile, const vt::tool::PageFileBuilderOptions & cmdLineOpts)
{
	if (inputFile.empty())
	{
		errorExit("No input filename!");
	}
	if (outputFile.empty())
	{
		errorExit("No output filename!");
	}

	vt::tool::PageFileBuilder pageFileBuilder(inputFile, outputFile, cmdLineOpts);
	pageFileBuilder.generatePageFile();

	if (cmdLineOpts.stdoutVerbose)
	{
		std::printf("Done!\n");
	}
}

} // namespace {}

// ======================================================
// main():
// ======================================================

int main(const int argc, const char * argv[])
{
	try
	{
		vt::tool::PageFileBuilderOptions cmdLineOpts;
		std::string inputFile, outputFile;

		parseCmdLine(argc, argv, inputFile, outputFile, cmdLineOpts);
		runPageFileBuilder(inputFile, outputFile, cmdLineOpts);

		return 0;
	}
	catch (std::exception & e)
	{
		std::printf("ERROR: %s\n", e.what());
		return -1;
	}
}
