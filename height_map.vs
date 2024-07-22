#version 320 es

// vertex shader for `height_map` sample

precision mediump usampler2D;  // TODO: what are other precisions and how they differ each other?

in vec3 position;  // expected to be in a range of ((0,0), (1,1)) square
out vec3 color;  // output vertex color

uniform mat4 local_to_screen;
uniform usampler2D heights;  // 16bit UI height texture (see heightMapSize for dimensions)
uniform vec2 heightMapSize;  // height map (width, height) in pixels
uniform float height_scale;  // e.g. 1 for PNG files or 100 for TIFF (elevation) files

const vec3 light_direction = vec3(0.86, 0.14, 0.49);

const float PI = 3.14159265359;

// st \in [0,1]^2, h \in [0,1]
vec3 to_word(vec2 st, float h) {
	return vec3(st * PI, h);  // TODO: this is specific for sinxy height maps
}

void main() {
	// calculate normal by sampling height map
	vec2 st = position.xy;

   // calculate heights of neighboring points
	vec2 left = st + vec2(-1.0, 0.0)/heightMapSize;
	vec2 right = st + vec2(1.0, 0.0)/heightMapSize;
	vec2 up = st + vec2(0.0, -1.0)/heightMapSize;
	vec2 down = st + vec2(0.0, 1.0)/heightMapSize;

	float heightL = float(texture(heights, left).r)/65535.0 * height_scale;
	float heightR = float(texture(heights, right).r)/65535.0 * height_scale;
	float heightU = float(texture(heights, up).r)/65535.0 * height_scale;
	float heightD = float(texture(heights, down).r)/65535.0 * height_scale;

   // calculate st point normal from neighbort points
	vec3 p0 = to_word(left, heightL);
	vec3 p1 = to_word(right, heightR);
	vec3 p2 = to_word(up, heightU);
	vec3 p3 = to_word(down, heightD);
	vec3 dx = p1 - p0;
	vec3 dy = p3 - p2;
	vec3 n = normalize(cross(dx, dy));  // TODO: it looks like normal is in a world space, shoul not be rather that in a view/camera space?

   // calculate color
	color = vec3(max(0.2, dot(n, light_direction)));

	// read h value from height map
	float h = float(texture(heights, st).r)/65535.0 * height_scale;  // pixel values are normalized to 0..1

	vec3 pos = vec3(position.xy, h);
	gl_Position = local_to_screen * vec4(pos, 1.0);
}
