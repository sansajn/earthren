#version 320 es

// Vertex shader for shade_shader shader implementation. To render primitives with simple shading and color.

/* TODO: There we want to calculate normal in fragment shader and our sample from dynamic_shading
does this in vertex shader. */

precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 local_to_screen;  //!< Local-to-Screen transformation.
uniform mat3 normal_to_view;  //!< Normal transformation. TODO: describe what it is.

out vec3 n;  //!< Calculated normal vector for fragment shader use.

void main() {
	n = normal_to_view * normal;  // TODO: this is wrong, we've mixing transformations to_vew and _to_screen
	gl_Position = local_to_screen * vec4(position, 1.0);
}
