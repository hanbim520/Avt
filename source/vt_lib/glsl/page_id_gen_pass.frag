
// ======================================================

// NOTE: Currently GL_OES_standard_derivatives is mandatory!
// This shader will not work on a device without the feature available.
#extension GL_OES_standard_derivatives : require

precision mediump float;

// Virtual Texture params:
uniform float u_log2_mip_scale_factor;
uniform float u_vt_max_mip_level;
uniform float u_vt_index;
uniform vec2  u_vt_size_pixels;
uniform vec2  u_vt_size_pages;

// Input from vertex shader:
varying mediump vec2 v_tex_coords;

// ======================================================
// computeMipLevel():
// ======================================================

float computeMipLevel(in vec2 uv)
{
	vec2 coord_pixels = uv * u_vt_size_pixels;

	vec2 x_deriv = dFdx(coord_pixels);
	vec2 y_deriv = dFdy(coord_pixels);

	float d = max(dot(x_deriv, x_deriv), dot(y_deriv, y_deriv));
	float m = max((0.5 * log2(d)) - u_log2_mip_scale_factor, 0.0);

	return floor(min(m, u_vt_max_mip_level));
}

// ======================================================
// main():
// ======================================================

void main()
{
	// Compute mip-level and virtual page coords:
	float mip_level   = computeMipLevel(v_tex_coords);
	vec2  page_coords = floor(v_tex_coords * u_vt_size_pages / exp2(mip_level));

	// Convert to [0,255] RGBA color and return:
	vec4 page_id = vec4(page_coords, mip_level, u_vt_index);
	gl_FragColor = page_id / 255.0;
}
