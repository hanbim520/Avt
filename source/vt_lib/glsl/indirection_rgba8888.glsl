
// **** RGBA-8:8:8:8 indirection table ****

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
	// RGBA-8:8:8:8 page indirection table lookup.
	//
	// This implementation is based on the shader provided by
	// J.M.P. van Waveren in his 2012 paper "Software Virtual Textures"
	//

	const float c_float_to_byte   = 255.0;
	const float c_page_frac_scale = (c_page_width - 2.0 * c_page_border) / c_page_width / c_phys_pages_wide;
	const float c_border_offset   = (c_page_border / c_page_width / c_phys_pages_wide);

	const vec4 c_tex_bias  = vec4(c_border_offset, c_border_offset, 0.0, 0.0);
	const vec4 c_tex_scale = vec4(
		(c_float_to_byte / c_phys_pages_wide),
		(c_float_to_byte / c_phys_pages_wide),
		(c_float_to_byte * 1.0   / 16.0),
		(c_float_to_byte * 256.0 / 16.0));

	vec4 phys_page = texture2D(indirection_table_samp, virt_coords, mip_sample_bias) * c_tex_scale + c_tex_bias;
	vec2 page_frac = fract(virt_coords * (phys_page.z + phys_page.w));

	vec3 result;
	result.xy = page_frac * c_page_frac_scale + phys_page.xy;

	// The derivative scale is set to zero if
	// we can't use it on texture2DGradEXT().
#if defined(VT_HAS_TEX_GRAD) && defined(VT_HAS_DERIVATIVES)
	result.z = c_page_frac_scale * (phys_page.z + phys_page.w);
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
