#version 320 es

// Vertex shader proram to visualize terrain outline.

precision mediump usampler2D;

layout(location = 0) in vec3 position;  // expected to be in a range of [0,1]^2 square

uniform usampler2D heights;  // 16bit UI height texture [0, 65535]
uniform float elevation_scale;  // terrain elevation scale factor calculated from elevation pixel resolution (e.g. = 0.000107174)
uniform float height_scale;  // e.g. 10.0

void main() {
	// read h value from elevation tile
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale;

	vec3 pos = vec3(position.xy, h);
	gl_Position = vec4(pos, 1.0);
}
