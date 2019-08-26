
// ======================================================

precision mediump float;

// Texture applied to the quadrilateral:
uniform sampler2D u_texture_samp; // tmu:0

// Texture coordinates forwarded by the Vertex Shader.
varying vec2 v_tex_coords;

// ======================================================
// main():
// ======================================================

void main()
{
	// Just the plain texture color.
	gl_FragColor = texture2D(u_texture_samp, v_tex_coords);
}
