#version 320 es

// Vertex shader for `height_overlap` sample to visualize terrain.

precision mediump float;
precision mediump usampler2D;

layout(location = 0) in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec2 st;  // normal texture coordinate in pixels [0, S_normal_size]^2

uniform mat4 local_to_screen;
uniform usampler2D heights;  // 16bit UI height texture
uniform float elevation_scale;  // terrain elevation scale factor calculated from elevation pixel resolution
uniform float height_scale;  // e.g. 10.0
uniform float normal_tile_size;  // size of normal tile in px (e.g. 730)

void main() {
	st = floor(position.xy * normal_tile_size);  // st \in [0, S_normal_tile]^2 in pixels

	// read h value from elevation tile
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale;

	vec3 pos = vec3(position.xy, h);
	gl_Position = local_to_screen * vec4(pos, 1.0);
}
