
// ================================================================================================
// -*- C++ -*-
// File: vt_common.cpp
// Author: Guilherme R. Lampert
// Created on: 09/09/14
// Brief: Miscellaneous functions and glue code.
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
#include "vt_mini_ui.hpp"
#include "vt_builtin_text.hpp"

#include <fstream>
#include <streambuf>
#include <iostream>
#include <cmath>

namespace vt
{
namespace {

// ======================================================
// DefaultLogCallbacks:
// ======================================================

struct DefaultLogCallbacks final
	: public LogCallbacks
{
	void logComment(const std::string & message) override
	{
		std::cout << message << "\n";
	}

	void logWarning(const std::string & message) override
	{
		std::cout << message << "\n";
	}

	void logError(const std::string & message) override
	{
		std::cout << message << std::endl;
	}
};

// ======================================================
// Local data:
// ======================================================

static GlobalShaders          globShaders;
static DefaultLogCallbacks    defaultLogCallbacks;
static LogCallbacks *         currentLogCallbacks;
static IndirectionTableFormat indirectionTableFmt;

// ======================================================
// readTextFile():
// ======================================================

static std::string readTextFile(const std::string & filename)
{
	std::ifstream ifs(filename);
	if (ifs)
	{
		return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	}
	return std::string();
}

// ======================================================
// getIndirectionTableGlslFuncs():
// ======================================================

static std::string getIndirectionTableGlslFuncs(const IndirectionTableFormat pageTblFormat)
{
	const char * filename;
	switch (pageTblFormat)
	{
	case IndirectionTableFormat::Rgb565 :
		filename = "indirection_rgb565.glsl";
		break;

	case IndirectionTableFormat::Rgba8888 :
		filename = "indirection_rgba8888.glsl";
		break;

	default :
		vtFatalError("Invalid IndirectionTableFormat!");
	} // switch (pageTblFormat)

	const std::string src = readTextFile(filename);
	if (src.empty())
	{
		vtFatalError("Failed to load fragment shader file \'" << filename << "\'!");
	}

	const std::string definesStr(
		"#ifdef GL_EXT_shader_texture_lod\n"
		"\t#extension GL_EXT_shader_texture_lod : enable\n"
		"\t#define VT_HAS_TEX_GRAD 1\n"
		"#endif\n\n"
		"#ifdef GL_OES_standard_derivatives\n"
		"\t#extension GL_OES_standard_derivatives : enable\n"
		"\t#define VT_HAS_DERIVATIVES 1\n"
		"#endif\n\n"
		"precision mediump float;\n\n"); // NOTE: Default precision set to medium

	char constsStr[1024];
	snprintf(constsStr, sizeof(constsStr),
		"const float c_page_width      = %.1f;\n"
		"const float c_page_border     = %.1f;\n"
		"const float c_phys_pages_wide = %.1f;\n\n",
		static_cast<float>(PageTable::PageSizeInPixels),
		static_cast<float>(PageTable::PageBorderSizeInPixels),
		static_cast<float>(PageTable::TableSizeInPages));

	// Prepend the VT directives and constants to the source:
	return definesStr + constsStr + src;
}

// ======================================================
// createGLProg():
// ======================================================

static GLuint createGLProg(const std::string & progName, const gl::VertexAttrib * const * vtxAttribs,
                           const char * vsBuiltIn = nullptr, const char * fsBuiltIn = nullptr)
{
	std::string vs;
	if (vsBuiltIn != nullptr)
	{
		vs += vsBuiltIn;
	}
	vs += readTextFile(progName + ".vert");
	if (vs.empty())
	{
		vtFatalError("Failed to load vertex shader file \'" << progName << ".vert\'!");
	}

	std::string fs;
	if (fsBuiltIn != nullptr)
	{
		fs += fsBuiltIn;
	}
	fs += readTextFile(progName + ".frag");
	if (fs.empty())
	{
		vtFatalError("Failed to load fragment shader file \'" << progName << ".frag\'!");
	}

	const GLuint programId = gl::createShaderProgram(vs.c_str(), fs.c_str(), vtxAttribs);
	if (!programId)
	{
		vtFatalError("Failed to create shader program " << progName << "!");
	}

	// Leave with the program still bound!
	gl::useShaderProgram(programId);
	return programId;
}

// ======================================================
// initGlobalShaders():
// ======================================================

static void initGlobalShaders(const IndirectionTableFormat pageTblFormat)
{
	// vtRenderSimple:
	{
		const gl::VertexAttrib attr0 = { "a_position"   , 0 };
		const gl::VertexAttrib attr1 = { "a_tex_coords" , 4 };
		const gl::VertexAttrib * vtxAttribs[] = { &attr0, &attr1, nullptr };

		globShaders.vtRenderSimple.programId = createGLProg("vt_render_simple",
				vtxAttribs, nullptr, getIndirectionTableGlslFuncs(pageTblFormat).c_str());

		globShaders.vtRenderSimple.unifMvpMatrix =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderSimple.programId, "u_mvp_matrix");

		globShaders.vtRenderSimple.unifMipSampleBias =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderSimple.programId, "u_mip_sample_bias");

		globShaders.vtRenderSimple.unifPageTableSamp =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderSimple.programId, "u_page_table_samp");

		globShaders.vtRenderSimple.unifIndirectionTableSamp =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderSimple.programId, "u_indirection_table_samp");

		// - Page cache will be at tex unit 0
		// - Page indirection table at tex unit 1
		gl::setShaderProgramUniform(globShaders.vtRenderSimple.unifPageTableSamp,        int(0)); // tmu:0
		gl::setShaderProgramUniform(globShaders.vtRenderSimple.unifIndirectionTableSamp, int(1)); // tmu:1

		// Mip sample bias:
		const float pageSizeLog2 = std::log2(PageTable::PageSizeInPixels);
		/* const float mipDebugBias = 0.1f; -> optional */
		const float mipSampleBias = pageSizeLog2 - 0.5f /* + mipDebugBias */;
		gl::setShaderProgramUniform(globShaders.vtRenderSimple.unifMipSampleBias, mipSampleBias);
	}

	// vtRenderLit:
	{
		const gl::VertexAttrib attr0 = { "a_position"   , 0 };
		const gl::VertexAttrib attr1 = { "a_normal"     , 1 };
		const gl::VertexAttrib attr2 = { "a_tangent"    , 2 };
		const gl::VertexAttrib attr3 = { "a_bitangent"  , 3 };
		const gl::VertexAttrib attr4 = { "a_tex_coords" , 4 };
		const gl::VertexAttrib * vtxAttribs[] = { &attr0, &attr1, &attr2, &attr3, &attr4, nullptr };

		globShaders.vtRenderLit.programId = createGLProg("vt_render_lit",
				vtxAttribs, nullptr, getIndirectionTableGlslFuncs(pageTblFormat).c_str());

		// Vertex Shader params:
		globShaders.vtRenderLit.unifMvpMatrix =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_mvp_matrix");

		globShaders.vtRenderLit.unifLightPosObjectSpace =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_light_pos_object_space");

		globShaders.vtRenderLit.unifViewPosObjectSpace =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_view_pos_object_space");

		// Fragment Shader params:
		globShaders.vtRenderLit.unifMipSampleBias =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_mip_sample_bias");

		globShaders.vtRenderLit.unifDiffuseSamp =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_diffuse_samp");

		globShaders.vtRenderLit.unifNormalSamp =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_normal_samp");

		globShaders.vtRenderLit.unifSpecularSamp =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_specular_samp");

		globShaders.vtRenderLit.unifIndirectionTableSamp =
			gl::getShaderProgramUniformLocation(globShaders.vtRenderLit.programId, "u_indirection_table_samp");

		// - Page indirection table at tex unit 0
		// - Page table textures starting from 1
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifIndirectionTableSamp, int(0)); // tmu:0
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifDiffuseSamp,          int(1)); // tmu:1
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifNormalSamp,           int(2)); // tmu:2
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifSpecularSamp,         int(3)); // tmu:3

		// Set to safe defaults.
		const float zero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifLightPosObjectSpace, zero, 4);
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifViewPosObjectSpace,  zero, 4);

		// Mip sample bias:
		const float pageSizeLog2 = std::log2(PageTable::PageSizeInPixels);
		/* const float mipDebugBias = 0.1f; -> optional */
		const float mipSampleBias = pageSizeLog2 - 0.5f /* + mipDebugBias */;
		gl::setShaderProgramUniform(globShaders.vtRenderLit.unifMipSampleBias, mipSampleBias);
	}

	// pageIdGenPass:
	{
		const gl::VertexAttrib attr0 = { "a_position"   , 0 };
		const gl::VertexAttrib attr2 = { "a_tex_coords" , 4 };
		const gl::VertexAttrib * vtxAttribs[] = { &attr0, &attr2, nullptr };

		globShaders.pageIdGenPass.programId = createGLProg("page_id_gen_pass", vtxAttribs);

		globShaders.pageIdGenPass.unifMvpMatrix = gl::getShaderProgramUniformLocation(
				globShaders.pageIdGenPass.programId, "u_mvp_matrix");

		globShaders.pageIdGenPass.unifLog2MipScaleFactor = gl::getShaderProgramUniformLocation(
				globShaders.pageIdGenPass.programId, "u_log2_mip_scale_factor");

		globShaders.pageIdGenPass.unifVTMaxMipLevel = gl::getShaderProgramUniformLocation(
				globShaders.pageIdGenPass.programId, "u_vt_max_mip_level");

		globShaders.pageIdGenPass.unifVTIndex = gl::getShaderProgramUniformLocation(
				globShaders.pageIdGenPass.programId, "u_vt_index");

		globShaders.pageIdGenPass.unifVTSizePixels = gl::getShaderProgramUniformLocation(
				globShaders.pageIdGenPass.programId, "u_vt_size_pixels");

		globShaders.pageIdGenPass.unifVTSizePages = gl::getShaderProgramUniformLocation(
				globShaders.pageIdGenPass.programId, "u_vt_size_pages");

		const float zero[] = { 0.0f, 0.0f };
		gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifLog2MipScaleFactor, 3.0f);
		gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTMaxMipLevel,      0.0f);
		gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTIndex,            0.0f);
		gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTSizePixels, zero, 2);
		gl::setShaderProgramUniform(globShaders.pageIdGenPass.unifVTSizePages,  zero, 2);
	}

	// drawIndirectionTable:
	{
		const gl::VertexAttrib attr0 = { "a_vertex_position_ndc", 0 };
		const gl::VertexAttrib * vtxAttribs[] = { &attr0, nullptr };

		globShaders.drawIndirectionTable.programId = createGLProg("draw_indirection_table", vtxAttribs);

		globShaders.drawIndirectionTable.unifTextureSamp = gl::getShaderProgramUniformLocation(
				globShaders.drawIndirectionTable.programId, "u_texture_samp");

		globShaders.drawIndirectionTable.unifNdcQuadScale = gl::getShaderProgramUniformLocation(
				globShaders.drawIndirectionTable.programId, "u_ndc_quad_scale");

		const float one[] = { 1.0f, 1.0f };
		gl::setShaderProgramUniform(globShaders.drawIndirectionTable.unifNdcQuadScale, one, 2);
		gl::setShaderProgramUniform(globShaders.drawIndirectionTable.unifTextureSamp,  int(0)); // tmu:0
	}

	// drawPageTable:
	{
		const gl::VertexAttrib attr0 = { "a_vertex_position_ndc", 0 };
		const gl::VertexAttrib * vtxAttribs[] = { &attr0, nullptr };

		globShaders.drawPageTable.programId = createGLProg("draw_page_table", vtxAttribs);

		globShaders.drawPageTable.unifTextureSamp = gl::getShaderProgramUniformLocation(
				globShaders.drawPageTable.programId, "u_texture_samp");

		globShaders.drawPageTable.unifNdcQuadScale = gl::getShaderProgramUniformLocation(
				globShaders.drawPageTable.programId, "u_ndc_quad_scale");

		const float one[] = { 1.0f, 1.0f };
		gl::setShaderProgramUniform(globShaders.drawPageTable.unifNdcQuadScale, one, 2);
		gl::setShaderProgramUniform(globShaders.drawPageTable.unifTextureSamp,  int(0)); // tmu:0
	}

	// drawText2D:
	{
		const gl::VertexAttrib attr0 = { "a_vertex_position_tex_coord", 0 };
		const gl::VertexAttrib attr1 = { "a_vertex_color",              1 };
		const gl::VertexAttrib * vtxAttribs[] = { &attr0, &attr1, nullptr };

		globShaders.drawText2D.programId = createGLProg("draw_text_2d", vtxAttribs);

		globShaders.drawText2D.unifTextureSamp = gl::getShaderProgramUniformLocation(
				globShaders.drawText2D.programId, "u_texture_samp");

		globShaders.drawText2D.unifMvpMatrix = gl::getShaderProgramUniformLocation(
				globShaders.drawText2D.programId, "u_mvp_matrix");

		gl::setShaderProgramUniform(globShaders.drawText2D.unifTextureSamp, int(0)); // tmu:0
	}

	// Cleanup:
	gl::useShaderProgram(0);
	vtLogComment("VT shaders initialized...");
}

} // namespace {}

// ======================================================
// getGlobalShaders():
// ======================================================

const GlobalShaders & getGlobalShaders() noexcept
{
	return globShaders;
}

// ======================================================
// getIndirectionTableFormat():
// ======================================================

IndirectionTableFormat getIndirectionTableFormat() noexcept
{
	return indirectionTableFmt;
}

// ======================================================
// getLogCallbacks():
// ======================================================

LogCallbacks & getLogCallbacks() noexcept
{
	assert(currentLogCallbacks != nullptr);
	return *currentLogCallbacks;
}

// ======================================================
// getLibraryVersionNumbers():
// ======================================================

void getLibraryVersionNumbers(int & major, int & minor, int & build) noexcept
{
	major = 1;
	minor = 0;
	build = 0;
}

// ======================================================
// getLibraryVersionString():
// ======================================================

std::string getLibraryVersionString()
{
	return "1.0.0";
}

// ======================================================
// libraryInit():
// ======================================================

void libraryInit(const IndirectionTableFormat format, LogCallbacks * logCallbacks)
{
	currentLogCallbacks = (logCallbacks != nullptr) ? logCallbacks : &defaultLogCallbacks;
	indirectionTableFmt = format;

	// The application might have already produced GL errors
	// that didn't get checked, so clear the error queue now
	// to avoid spurious warnings from errors we didn't cause.
	gl::clearGLErrors();

	// Since we are dealing exclusively with RGBA,
	// the ideal pixel alignment is 4.
	gl::setPixelStoreAlignment(4);

	// Init all the VT GL programs.
	// This will throw an exception is a fatal error happens.
	initGlobalShaders(format);

	vtLogComment("VT library initialized! Indirection table format is: "
		<< ((indirectionTableFmt == IndirectionTableFormat::Rgb565) ? "RGB 5:6:5" : "RGBA 8:8:8:8"));
}

// ======================================================
// libraryShutdown():
// ======================================================

void libraryShutdown()
{
	vtLogComment("VT library shutting down...");

	currentLogCallbacks = nullptr;

	ui::shutdownUI();
	font::unloadAllBuiltInFonts();

	gl::deleteShaderProgram(globShaders.drawText2D.programId);
	gl::deleteShaderProgram(globShaders.drawPageTable.programId);
	gl::deleteShaderProgram(globShaders.drawIndirectionTable.programId);
	gl::deleteShaderProgram(globShaders.pageIdGenPass.programId);
	gl::deleteShaderProgram(globShaders.vtRenderSimple.programId);
	clearPodObject(globShaders);
}

} // namespace vt {}
