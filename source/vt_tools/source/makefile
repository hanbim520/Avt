
#
# This makefile compiles the vtmake command line tool.
# It references the vt_tools library and the vt_make.cpp file
# to generate the MacOS command-line executable.
#

CXXFLAGS =\
	-Wall\
	-Weffc++\
	-Wextra\
	-Winit-self\
	-Wmissing-braces\
	-Wparentheses\
	-Wpointer-arith\
	-Wreturn-type\
	-Wsequence-point\
	-Wshadow\
	-Wstrict-aliasing\
	-Wswitch-default\
	-Wswitch\
	-Wuninitialized\
	-Wunknown-pragmas\
	-Wunused\
	-Wwrite-strings\
	-std=c++11\
	-O3

SOURCE_FILES =\
	vt_tool_filters.cpp\
	vt_tool_float_image_buffer.cpp\
	vt_tool_image.cpp\
	vt_tool_mipmapper.cpp\
	vt_tool_pagefile_builder.cpp\
	vt_tool_pixfont.cpp\
	vt_tool_platform_utils.mm\
	vt_tool_write_tga.cpp\
	vt_make.cpp

COMPILER     = clang++
OUTPUT_FILE  = vtmake
FRAMEWORKS   = -framework Foundation
INCLUDE_DIRS = -I../include/ -I../../vt_lib/include/

all:
	$(COMPILER) $(CXXFLAGS) $(FRAMEWORKS) $(INCLUDE_DIRS) $(SOURCE_FILES) -o $(OUTPUT_FILE)

clean:
	rm -f *.o *.a $(OUTPUT_FILE)

