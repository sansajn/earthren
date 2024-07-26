#version 320 es

// vertex shader proram to visualize normals

precision mediump usampler2D;

in vec3 position;  // expected to be in a range of [0,1]^2 square

void main() {
	gl_Position = vec4(position, 1);  // just pass-through position in model space
}
