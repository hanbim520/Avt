
// **** RGB-5:6:5 indirection table ****

// These constants must be appended to the shader before compiling.
// Can even be uniform variables if runtime change becomes necessary.
//
// const float c_page_width;      -> page size in pixels
// const float c_page_border;     -> border size in pixels
// const float c_phys_pages_wide; -> size of the page cache texture in pages
//
// - VT_HAS_TEX_GRAD    is defined if GL_EXT_shader_texture_lod is available.
// - VT_HAS_DERIVATIVES is defined if GL_OES_standard_derivatives is available.

// ======================================================
// virtualToPhysicalTranslation():
// ======================================================

vec3 virtualToPhysicalTranslation(in sampler2D indirection_table_samp, in vec2 virt_coords, in float mip_sample_bias)
{
	//
	// RGB-5:6:5 page indirection table lookup.
	//
	// This implementation is based on the shader provided by
	// J.M.P. van Waveren in his 2012 paper "Software Virtual Textures"
	//

	// Multipliers to undo the hardware specific conversion from a
	// 5-bit or 6-bit value to a floating-point value in the range [0, 1].
	const float c_float_to_pages_wide = 255.0 / 256.0 * c_phys_pages_wide;
	const float c_float_to_mip        = 255.0 / 256.0 * 64.0;

	// Derived constants:
	const float c_page_frac_scale = (c_page_width - 2.0 * c_page_border) / c_page_width / c_phys_pages_wide;
	const float c_border_offset   = (c_page_border / c_page_width / c_phys_pages_wide);
	const vec4  c_tex_scale       = vec4(c_float_to_pages_wide, c_float_to_mip, c_float_to_pages_wide, 0.0);

	// Small value added to the exponent so we can floor the inaccurate exp2() result.
	const vec4 c_tex_bias = vec4(0.0, (1.0 / 4096.0), 0.0, 0.0);

	// Address lookup and translation:
	vec4 phys_page = texture2D(indirection_table_samp, virt_coords, mip_sample_bias) * c_tex_scale + c_tex_bias;
	phys_page.y = exp2(phys_page.y);
	phys_page   = floor(phys_page); // Account for inaccurate exp2() and strip replicated bits.

	vec2 pag_frac = fract(virt_coords * phys_page.y);

	vec3 result;
	result.xy = pag_frac * c_page_frac_scale + phys_page.xz / c_phys_pages_wide + c_border_offset;

	// The derivative scale is set to zero if
	// we can't use it on texture2DGradEXT().
#if defined(VT_HAS_TEX_GRAD) && defined(VT_HAS_DERIVATIVES)
	result.z = c_page_frac_scale * phys_page.y;
#else // !VT_HAS_TEX_GRAD && !VT_HAS_DERIVATIVES
	result.z = 0.0;
#endif // VT_HAS_TEX_GRAD && VT_HAS_DERIVATIVES

	return result;
}

// ======================================================
// virtualTexture2D():
// ======================================================

vec4 virtualTexture2D(in sampler2D page_table_samp, in vec2 virt_coords, in vec3 phys_coords)
{
#if defined(VT_HAS_TEX_GRAD) && defined(VT_HAS_DERIVATIVES)
	vec2 dx = dFdx(virt_coords) * phys_coords.z;
	vec2 dy = dFdy(virt_coords) * phys_coords.z;
	return texture2DGradEXT(page_table_samp, phys_coords.xy, dx, dy);
#else // !VT_HAS_TEX_GRAD && !VT_HAS_DERIVATIVES
	return texture2D(page_table_samp, phys_coords.xy);
#endif // VT_HAS_TEX_GRAD && VT_HAS_DERIVATIVES
}
