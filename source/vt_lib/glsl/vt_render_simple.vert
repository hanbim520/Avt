
// ======================================================

precision mediump float;

attribute mediump vec3 a_position;
attribute mediump vec2 a_tex_coords;

uniform mediump mat4 u_mvp_matrix;
varying mediump vec2 v_tex_coords;

// ======================================================
// main():
// ======================================================

void main()
{
	gl_Position  = u_mvp_matrix * vec4(a_position, 1.0);
	v_tex_coords = a_tex_coords;
}
