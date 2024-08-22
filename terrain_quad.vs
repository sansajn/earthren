#version 320 es

// vertex shader for `terrain_quad` sample

precision mediump usampler2D;  // TODO: there is also `highp` (32bit flaot) precision there

in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec2 st;  // normal texture coordinate in pixels [0, S_normal_size]^2

uniform mat4 local_to_screen;
uniform usampler2D heights;  // 16bit UI height texture (see heightMapSize for dimensions)
uniform float elevation_scale;  // terrain elevation scale factor 
uniform float height_scale;  // e.g. 1 for PNG files or 100 for TIFF (elevation) files

const float normal_tile_size = 712.0;  // the value is calculated as elevation_tile_size-4 (where 4 are for 2px border for each side)

void main() {
	st = floor(position.xy * normal_tile_size);  // st \in [0,S_normal_tile]^2 in pixels

	// read h value from elevation tile
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale;

	vec3 pos = vec3(position.xy, h);
	gl_Position = local_to_screen * vec4(pos, 1.0);
}
