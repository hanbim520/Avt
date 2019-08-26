
// ======================================================

precision mediump float;

// xy = vertex position in normalized device coordinates ([-1,+1] range).
attribute vec2 a_vertex_position_ndc;

// Output tex coordinates:
varying vec2 v_tex_coords;

// NDC vertex positions are multiplied by this value.
uniform vec2 u_ndc_quad_scale;

// ======================================================
// main():
// ======================================================

void main()
{
	const vec2 scale = vec2(0.5, 0.5);
	v_tex_coords = a_vertex_position_ndc * scale + scale; // scale vertex attribute to [0,1] range

	// This branch is a simple way to reposition the quadrilateral
	// at the lower left corner of the screen. This way we don't
	// have to re-specify the vertexes every time.
	vec2 pos = a_vertex_position_ndc;
	if (pos.x > 0.0) { pos.x = 0.0; }
	if (pos.y > 0.0) { pos.y = 0.0; }

	gl_Position = vec4(pos * u_ndc_quad_scale, 0.0, 1.0);
}
