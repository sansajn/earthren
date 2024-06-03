#version 320 es

// fragment shader for `height_sinxy_map` sample

precision mediump float;

in float h;
vec3 color_a = vec3(0, 0, 1);  // blue
vec3 color_b = vec3(1, 0, 0);  // red
out vec4 frag_color;

void main() {
	frag_color = vec4(mix(color_a, color_b, h), 1.0);
}
