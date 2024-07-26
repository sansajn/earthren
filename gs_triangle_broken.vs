#version 320 es

// vertex shader proram to visualize normals

precision mediump usampler2D;

in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec3 normal;

uniform mat4 local_to_screen;  // local-to-screen (to clip space) transformation

void main() {
	normal = normalize(vec3(0,0,1));  // fake normal in model space
	gl_Position = vec4(position, 1);  // just pass-through position in model space
}
