#version 320 es

// Fragment shader proram to visualize with defined color.

precision mediump float;

uniform vec3 fill_color;

out vec4 color;  // output fragment value/color

void main() {
	color = vec4(fill_color, 1);
}
