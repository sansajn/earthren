#version 320 es

// vertex shader for `height_sinxy_map` sample

in vec3 position;  // expected to be in a range of ((0,0), (1,1)) square
out float h;  // forward height value to fragment shader

uniform mat4 local_to_screen;
uniform sampler2D heights;  // 512x512 height texture

const float PI = 3.14159265359;

void main() {
	// read h value from height map
	vec2 st = position.xy;
	h = texture(heights, st).r;  // pixel values are normalized to 0..1

	vec3 pos = vec3(position.xy, h);
	gl_Position = local_to_screen * vec4(pos, 1.0);
}
