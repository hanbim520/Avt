
// ======================================================

precision mediump float;

// Texture applied to the quadrilateral:
uniform sampler2D u_texture_samp; // tmu:0

// Texture coordinates forwarded by the Vertex Shader.
varying vec2 v_tex_coords;

// ======================================================
// reverseBytes():
// ======================================================

vec4 reverseBytes(vec4 a) // Input is a [0,255] color
{
	/*
	 * Base algorithm provided by the good fellows of the
	 * StackOverflow community. The loop was unrolled to improve
	 * performance on the limited mobile GPUs.
	 *
	 * vec4 b = vec4(0.0);
	 * for (int i = 0; i < 8; ++i)
	 * {
	 *     b *= 2.0;
	 *     b += mod(a, 2.0);
	 *     a /= 2.0;
	 * }
	 * return b;
	 *
	 * http://stackoverflow.com/a/26289054/1198654
	 */
	vec4 b = vec4(0.0);
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [0]
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [1]
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [2]
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [3]
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [4]
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [5]
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [6]
	b *= 2.0; b += mod(a, 2.0); a /= 2.0; // [7]
	return b;
}

// ======================================================
// main():
// ======================================================

void main()
{
	//
	// Recolor the texture data so we can
	// better visualize the page ids:
	//
	vec4 c = texture2D(u_texture_samp, v_tex_coords);

	// Scale to [0,255]:
	c *= 255.0;

	// Reverse bits in every color byte:
	c = reverseBytes(c);

	// Mix alpha (the texture index) it with blue (the mip-level):
	c.b = min(c.a + c.b, 255.0);

	// This shader always draws opaque geometry.
	c.a = 255.0;

	// Back to [0,1] float and return.
	gl_FragColor = (c / 255.0);
}
