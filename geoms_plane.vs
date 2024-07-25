#version 320 es

// vertex shader proram to visualize normals

precision mediump usampler2D;

in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec3 normal;

void main() {
	normal = vec3(0,0,1);

	float h = 0.0f;
	gl_Position = vec4(position.xy, h, 1);  // TODO: gl_Position is expected to be in clip space (menas local-to-screen transformation should be applied)
}
