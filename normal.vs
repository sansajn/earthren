#version 320 es

// vertex shader proram to visualize normals

precision mediump usampler2D;

in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec3 normal;

uniform usampler2D heights;  // 512x512 16bit UI height texture

const float PI = 3.14159265359;

const vec2 heightMapSize = vec2(512, 512);
const float heightScale = 1.0;

// st \in [0,1]^2, h \in [0,1]
vec3 to_word(vec2 st, float h) {
   return vec3(st * PI, h);  // TODO: this is specific for sinxy height maps
}

// Calculate normal by sampling pos neightbords from height map.
vec3 calculate_normal(vec2 st, usampler2D heights) {
   // calculate heights of neighboring points
	vec2 left = st + vec2(-1.0, 0.0)/heightMapSize;
	vec2 right = st + vec2(1.0, 0.0)/heightMapSize;
	vec2 up = st + vec2(0.0, -1.0)/heightMapSize;
	vec2 down = st + vec2(0.0, 1.0)/heightMapSize;

   float heightL = float(texture(heights, left).r)/65535.0 * heightScale;
	float heightR = float(texture(heights, right).r)/65535.0 * heightScale;
	float heightU = float(texture(heights, up).r)/65535.0 * heightScale;
	float heightD = float(texture(heights, down).r)/65535.0 * heightScale;

   // calculate st point normal from neighbort points
	vec3 p0 = to_word(left, heightL);
	vec3 p1 = to_word(right, heightR);
	vec3 p2 = to_word(up, heightU);
	vec3 p3 = to_word(down, heightD);
	vec3 dx = p1 - p0;
	vec3 dy = p3 - p2;
	vec3 n = normalize(cross(dx, dy));

   return n;
}

void main() {
   vec2 st = position.xy;
	normal = calculate_normal(st, heights);

   // read h value from height map
	float h = (float(texture(heights, st).r)*heightScale)/65535.0;  // pixel values are normalized to 0..1

   gl_Position = vec4(position.xy, h, 1);
}
