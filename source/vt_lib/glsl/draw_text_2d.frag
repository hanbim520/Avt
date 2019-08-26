
// ======================================================

precision mediump float;

// Texture applied to the quadrilateral of the text glyph:
uniform sampler2D u_texture_samp; // tmu:0

varying vec2 v_tex_coords;
varying vec4 v_color;

// ======================================================
// main():
// ======================================================

void main()
{
	// Just the plain texture color modulated with vertex color.
	gl_FragColor = texture2D(u_texture_samp, v_tex_coords) * v_color;
}