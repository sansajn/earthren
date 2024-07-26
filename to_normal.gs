#version 320 es

/* Geometry shader program to transform triangle to a face normal.
Trianlge points are expected in a model space, therefore we need local-to-screen transformation
as `local_to_screen` 4x4 matrix. */

layout(triangles) in;
layout(line_strip, max_vertices = 2) out;

/* Line direction calculated by a vertex shader program (vertex shader needs pass `line_direction`
variable defined as `out vec3 line_direction;`. */
const float normal_length = 0.01;

uniform mat4 local_to_screen;  // transforms point to clip sapce (a.k.a. NVP transformation)

bool is_visible(vec4 p_clip) {  //!< \param [in] p_clip Point in clip space.
	vec2 p0_ndc = p_clip.xy / p_clip.w;
	return !(p0_ndc.x < -1.0 || p0_ndc.x > 1.0 || p0_ndc.y < -1.0 || p0_ndc.y > 1.0);
}

void main() {
	vec4 a = gl_in[0].gl_Position;

	vec4 a_clip = local_to_screen * a;
	if (is_visible(a_clip)) {
		vec4 b = gl_in[1].gl_Position;
		vec4 c = gl_in[2].gl_Position;

		// start point
		vec4 center = (a + b + c) / 3.0;
		gl_Position = local_to_screen * center;
		EmitVertex();

		// end point
		vec3 ab = b.xyz - a.xyz;
		vec3 ac = c.xyz - a.xyz;
		vec3 n_face = normalize(cross(ab, ac));
		gl_Position = local_to_screen * (center + vec4(normal_length*n_face, 0.0));
		EmitVertex();

		EndPrimitive();
	}
}
