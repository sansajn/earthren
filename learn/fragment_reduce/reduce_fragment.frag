#version 320 es
precision highp float;

// Input texture from previous pass
uniform sampler2D u_input_texture;
uniform vec2 u_texel_size;  //!< Input texture texel size in pixels (=1/width, 1/height).

// Output
layout(location = 0) out vec4 out_result_pixel;

void main() {
	// Get integer coordinates of the output pixel
	ivec2 result_pixel_coord = ivec2(gl_FragCoord.xy);

	// Compute the top-left input texture texel for this 2x2 block. Note, input texture is 2Wx2H size of the output texture.
	vec2 base_coord = (vec2(result_pixel_coord) * 2.0 + 0.5) * u_texel_size;

	// Sample the 2x2 block
	vec4 value1 = texture(u_input_texture, base_coord);
	vec4 value2 = texture(u_input_texture, base_coord + vec2(u_texel_size.x, 0.0));
	vec4 value3 = texture(u_input_texture, base_coord + vec2(0.0, u_texel_size.y));
	vec4 value4 = texture(u_input_texture, base_coord + u_texel_size);

	out_result_pixel = max(max(value1, value2), max(value3, value4));
}
