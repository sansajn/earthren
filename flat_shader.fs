#version 320 es
precision mediump float;

out vec4 frag_color;
uniform vec3 color;  // primitive color

void main() {
	frag_color = vec4(color, 1);
}
