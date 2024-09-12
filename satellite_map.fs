#version 320 es

// fragment shader for `terrain_quad` sample

precision mediump float;
precision mediump sampler2D;
precision mediump usampler2D;  // TODO: there is also `highp` (32bit flaot) precision there

uniform usampler2D heights;  // 16bit INT height texture
uniform sampler2D satellite_map;  // 8bit UINT texture

uniform bool use_satellite_map;
uniform bool use_shading;

in vec2 st;  // normal texture coordinate in pixels [0, S_normal_size]^2
out vec4 frag_color;

//const vec3 light_direction = vec3(0.86, 0.14, 0.49);
const vec3 light_direction = vec3(0.0, 0.0, 1.0);

// TODO: make elevation_tile_size and normal_tile_size configurable (uniform)
const float pixel_size = 26.063200588611451;  // elevation pixel size in meters
const float normal_tile_size = 712.0;  // the value is calculated as elevation_tile_size-4 (where 4 are for 2px border for each side)
const float elevation_tile_size = 716.0;  // height_map_size
const vec2 elevation_tile_offset = vec2((2.0 + 0.25)/elevation_tile_size, (2.0 + 0.25)/elevation_tile_size);
const float terrain_size = pixel_size * elevation_tile_size;
const vec2 terrain_offset = vec2(0.0, 0.0);

const float satellite_tile_size = 622.0;  // TODO: hardcoded value

// uv_p \in [0,S_tile]^2 in pixels, h in meters 
vec3 to_word(vec2 uv_p, float h) {
	vec2 uv = uv_p / (normal_tile_size - 1.0);
	return vec3(terrain_offset.xy + uv * terrain_size, h);
}

vec3 calculate_normal(vec2 st, usampler2D heights) {
   // calculate heights of neighboring points
	vec2 left = (st + vec2(-1.0, 0.0))/elevation_tile_size + elevation_tile_offset.xy;
	vec2 right = (st + vec2(1.0, 0.0))/elevation_tile_size + elevation_tile_offset.xy;
	vec2 up = (st + vec2(0.0, -1.0))/elevation_tile_size + elevation_tile_offset.xy;
	vec2 down = (st + vec2(0.0, 1.0))/elevation_tile_size + elevation_tile_offset.xy;

	float height_left = float(texture(heights, left).r);
	float height_right = float(texture(heights, right).r);
	float height_up = float(texture(heights, up).r);
	float height_down = float(texture(heights, down).r);

   // calculate uv point normal from neighbort points
	vec3 p0 = to_word(st + vec2(-1.0, 0.0), height_left);
	vec3 p1 = to_word(st + vec2(1.0, 0.0), height_right);
	vec3 p2 = to_word(st + vec2(0.0, -1.0), height_up);
	vec3 p3 = to_word(st + vec2(0.0, 1.0), height_down);
	vec3 dx = p1 - p0;
	vec3 dy = p3 - p2;
	vec3 n = normalize(cross(dx, dy));  // TODO: it looks like normal is in a world space, shoul not be rather that in a view/camera space?
	return n;
}

void main() {
   vec2 uv_p = floor(st);
   vec3 n = calculate_normal(uv_p, heights);

	vec3 satellite_color = vec3(texture(satellite_map, uv_p / (normal_tile_size - 1.0)).rgb);
	if (!use_satellite_map)
		satellite_color = vec3(0.8, 0.8, 0.8);

	vec3 color = satellite_color;
	if (use_shading)
		color *= max(0.2, dot(n, light_direction));

   frag_color = vec4(color, 1.0);
}
