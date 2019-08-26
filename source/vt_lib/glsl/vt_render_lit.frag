
// ======================================================

// Texture params:
uniform float     u_mip_sample_bias;
uniform sampler2D u_indirection_table_samp; // tmu:0
uniform sampler2D u_diffuse_samp;           // tmu:1
uniform sampler2D u_normal_samp;            // tmu:2
uniform sampler2D u_specular_samp;          // tmu:3

// Inputs from previous stage:
varying vec3 v_view_dir_tangent_space;  // Tangent-space view direction.
varying vec3 v_light_dir_tangent_space; // Tangent-space light direction.
varying vec3 v_position_object_space;   // Forwarded vertex-position in object coordinates.
varying vec3 v_light_pos_object_space;  // Forwarded light-position  in object coordinates.
varying vec2 v_tex_coords;

// Light params. Ideally, these should be uniform variables.
const highp float c_point_light_radius    = 20.0;
const highp float c_light_atten_const     = 1.0;
const highp float c_light_atten_linear    = 2.0 / c_point_light_radius;
const highp float c_light_atten_quadratic = 1.0 / (c_point_light_radius * c_point_light_radius);
const vec4 c_light_color = vec4(0.45, 0.45, 0.45, 1.0);

// Material params. Ideally, these should be uniform variables.
const highp float c_mat_shininess = 50.0;
const vec4 c_specular_color = vec4(0.6, 0.6, 0.6, 1.0);
const vec4 c_diffuse_color  = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 c_emissive_color = vec4(0.0, 0.0, 0.0, 1.0);
const vec4 c_ambient_color  = vec4(0.2, 0.2, 0.2, 1.0);

// ======================================================
// shade():
// ======================================================

vec4 shade(
	in vec3 N,             // Surface normal.
	in vec3 H,             // Half-vector.
	in vec3 L,             // Light direction vector.
	in float shininess,    // Surface shininess (Phong-power).
	in vec4 specular,      // Surface's specular reflection color, modulated with specular map sample.
	in vec4 diffuse,       // Surface's diffuse  reflection color, modulated with diffuse  map sample.
	in vec4 emissive,      // Surface's emissive contribution. Emission color modulated with an emission map.
	in vec4 ambient,       // Ambient contribution.
	in vec4 light_contrib) // Light contribution, computed based on light source type.
{
	// Shade a fragment using the Blinn-Phong light model:
	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	float spec_contrib = (NdotL > 0.0) ? 1.0 : 0.0;
	vec4 K = emissive + (diffuse * ambient);
	K += light_contrib * (diffuse * NdotL + (specular * pow(NdotH, shininess) * spec_contrib));

	return K;
}

// ======================================================
// main():
// ======================================================

void main()
{
	// Lookup the indirection texture and convert a virtual texture address
	// to an address suitable for sampling a physical page table/cache texture.
	vec3 phys_coords = virtualToPhysicalTranslation(u_indirection_table_samp, v_tex_coords, u_mip_sample_bias);

	// Now we can use the physical coordinates to sample as many textures as we need.
	vec4 diffuseMap  = virtualTexture2D(u_diffuse_samp,  v_tex_coords, phys_coords);
	vec4 normalMap   = virtualTexture2D(u_normal_samp,   v_tex_coords, phys_coords);
	vec4 specularMap = virtualTexture2D(u_specular_samp, v_tex_coords, phys_coords);

	// Point light attenuation and contribution:
	float d = length(v_position_object_space - v_light_pos_object_space);
	vec4 light_contrib = c_light_color * (1.0 / (c_light_atten_const + c_light_atten_linear * d + c_light_atten_quadratic * (d * d)));

	// Compute half-vector / fetch normal-map:
	vec3 V = v_view_dir_tangent_space;
	vec3 L = v_light_dir_tangent_space;
	vec3 H = normalize(L + V);
	vec3 N = normalize((normalMap.rgb * 2.0) - 1.0);

	// Apply shading algorithm:
	gl_FragColor = shade(
		N,
		H,
		L,
		c_mat_shininess,
		specularMap * c_specular_color,
		diffuseMap  * c_diffuse_color,
		c_emissive_color,
		c_ambient_color,
		light_contrib);

	// Set alpha to 1 (fully-opaque).
	gl_FragColor.a = 1.0;
}
