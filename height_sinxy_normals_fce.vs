#version 320 es

// vertex shader for `height_sinxy_normals_fce` sample

precision mediump usampler2D;  // TODO: what are other precisions and how they differ each other?

in vec3 position;  // expected to be in a range of ((0,0), (1,1)) square
out vec3 color;  // output vertex color

uniform mat4 local_to_screen;
uniform usampler2D heights;  // 512x512 16bit UI height texture

vec3 light_direction = vec3(0.86, 0.14, 0.49);

const float PI = 3.14159265359;

void main() {
	// calculate normal by sampling height map
	vec2 st = position.xy;

   // calculate normal, be aware that we are in a model space
	vec2 cosxy = cos(st * PI);
	vec2 sinxy = sin(st * PI);  // from 0..pi range
	vec3 dF = vec3(cosxy.x*sinxy.y, sinxy.x*cosxy.y, -1);
	vec3 n = normalize(-dF);

   // calculate color
	color = vec3(max(0.2, dot(n, light_direction)));

	// read h value from height map
	float h = float(texture(heights, st).r)/65535.0;  // pixel values are normalized to 0..1

	vec3 pos = vec3(position.xy, h);
	gl_Position = local_to_screen * vec4(pos, 1.0);
}
