#version 320 es

/* Geometry shader program to transform triangle to a line started in a first triangle vertex
with a direction of `line_direction` and a `line_length` of length. The program can be used to
visualize triangle normal or a light direction.

Trianlge points are expected in a model space, therefore we need local-to-screen transformation
as `local_to_screen` 4x4 matrix. */

layout(triangles) in;
layout(line_strip, max_vertices = 2) out;

/* Line direction calculated by a vertex shader program (vertex shader needs pass `line_direction`
variable defined as `out vec3 line_direction;`. */
in vec3 line_direction[];
const float line_length = 0.01;

uniform mat4 local_to_screen;  // transforms point to clip sapce (a.k.a. NVP transformation)

bool is_visible(vec4 p_clip) {  //!< \param [in] p_clip Point in clip space.
	vec2 p0_ndc = p_clip.xy / p_clip.w;
	return !(p0_ndc.x < -1.0 || p0_ndc.x > 1.0 || p0_ndc.y < -1.0 || p0_ndc.y > 1.0);
}

void main() {
	vec4 p = gl_in[0].gl_Position;
	vec4 p0 = local_to_screen * gl_in[0].gl_Position;
	if (is_visible(p0)) {  // cull out points outside of NDC area (otherwise we can see artifacts)
		gl_Position = p0;
		EmitVertex();

		vec3 d = line_direction[0];
		gl_Position = local_to_screen * (p + vec4(d*line_length, 0.0));
		EmitVertex();

		EndPrimitive();
	}
}
