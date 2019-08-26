
// ======================================================

// The correct virtualToPhysicalTranslation() function
// for the underlaying indirection table will be included
// by the VT library when this shader is loaded, together
// with a virtualTexture2D() function.

// Render params:
uniform float     u_mip_sample_bias;
uniform sampler2D u_page_table_samp;        // tmu:0
uniform sampler2D u_indirection_table_samp; // tmu:1

// Vertex Shader outputs:
varying mediump vec2 v_tex_coords;

// ======================================================
// main():
// ======================================================

void main()
{
	// Lookup the indirection texture and convert a virtual texture address
	// to an address suitable for sampling the physical page table/cache texture.
	vec3 phys_coords = virtualToPhysicalTranslation(u_indirection_table_samp, v_tex_coords, u_mip_sample_bias);

	// With the physical address we can now sample as many physical textures as we like,
	// as long as they all have the same dimensions. In this demo, we only
	// sample a diffuse texture. In a more complex scenario, phys_coords would
	// index the diffuse, normal, specular, whatnot maps, translating the virtual coords only once.
	gl_FragColor = virtualTexture2D(u_page_table_samp, v_tex_coords, phys_coords);
}
