#version 320 es
precision highp float;

// Only position attribute
layout(location = 0) in vec3 position;

void main() {
	gl_Position = vec4(position, 1.0);
}
