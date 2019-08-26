
// ======================================================

precision mediump float;

// xy = vertex position, zx = texture coordinates.
attribute vec4 a_vertex_position_tex_coord;
attribute vec4 a_vertex_color;

// Output tex coordinates and color:
varying vec2 v_tex_coords;
varying vec4 v_color;

// Usually with an orthographic projection.
uniform mat4 u_mvp_matrix;

// ======================================================
// main():
// ======================================================

void main()
{
	gl_Position  = u_mvp_matrix * vec4(a_vertex_position_tex_coord.xy, 0.0, 1.0);
	v_tex_coords = a_vertex_position_tex_coord.zw;
	v_color      = a_vertex_color;
}
