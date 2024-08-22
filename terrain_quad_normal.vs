#version 320 es

// vertex shader proram to visualize normals

precision mediump usampler2D;

in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec3 normal;

uniform usampler2D heights;  // 16bit UI height texture
uniform vec2 height_map_size;  // height map (width, height) in pixels
uniform float height_scale;  // e.g. 1 for PNG files or 100 for TIFF (elevation) files

const float INT16_MAX_F = 65535.0;

const float pixel_size = 26.063200588611451;  // elevation pixel size in meters
const float normal_tile_size = 712.0;  // 716 - 
const float elevation_tile_size = 716.0;  // height_map_size
const vec2 elevation_tile_offset = vec2((2.0 + 0.25)/elevation_tile_size, (2.0 + 0.25)/elevation_tile_size);
const float terrain_size = pixel_size * elevation_tile_size;
const vec2 terrain_offset = vec2(0.0, 0.0);


// uv_p \in [0,S_tile]^2 in pixels, h in meters 
vec3 to_word(vec2 uv_p, float h) {
	vec2 uv = uv_p / (normal_tile_size - 1.0);
	return vec3(terrain_offset.xy + uv * terrain_size, h);
}

// Calculate normal by sampling pos neightbords from height map.
vec3 calculate_normal(vec2 st, usampler2D heights) {
	vec2 uv = floor(st * normal_tile_size);  // uv in [0,S_normal_tile]^2 pixels

   // calculate heights of neighboring points
	vec2 left = (uv + vec2(-1.0, 0.0))/elevation_tile_size + elevation_tile_offset.xy;
	vec2 right = (uv + vec2(1.0, 0.0))/elevation_tile_size + elevation_tile_offset.xy;
	vec2 up = (uv + vec2(0.0, -1.0))/elevation_tile_size + elevation_tile_offset.xy;
	vec2 down = (uv + vec2(0.0, 1.0))/elevation_tile_size + elevation_tile_offset.xy;

	float height_left = float(texture(heights, left).r);
	float height_right = float(texture(heights, right).r);
	float height_up = float(texture(heights, up).r);
	float height_down = float(texture(heights, down).r);

   // calculate uv point normal from neighbort points
	vec3 p0 = to_word(uv + vec2(-1.0, 0.0), height_left);
	vec3 p1 = to_word(uv + vec2(1.0, 0.0), height_right);
	vec3 p2 = to_word(uv + vec2(0.0, -1.0), height_up);
	vec3 p3 = to_word(uv + vec2(0.0, 1.0), height_down);
	vec3 dx = p1 - p0;
	vec3 dy = p3 - p2;
	vec3 n = normalize(cross(dx, dy));  // TODO: it looks like normal is in a world space, shoul not be rather that in a view/camera space?

   return n;
}

void main() {
   vec2 st = position.xy;
	normal = calculate_normal(st, heights);

   // read h value from height map
	float h = float(texture(heights, st).r)/INT16_MAX_F * height_scale;  // pixel values are normalized to 0..1

   gl_Position = vec4(position.xy, h, 1);
}
