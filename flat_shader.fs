#version 320 es
precision mediump float;

uniform vec3 color;  // primitive color

void main() {
	gl_FragColor = vec4(color, 1);
}
