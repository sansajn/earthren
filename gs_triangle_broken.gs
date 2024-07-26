#version 320 es

/* geometry shader proram to visualize normals
Implementation expects triangles and for each trianlge vertex it draws a normal calculated by a vertex shader program. */

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in vec3 normal[];  // normals from vertex shader program

uniform mat4 local_to_screen;

const float normal_length = 0.01;

void main() {
	for (int i = 0; i < 3; ++i) {
		vec4 p = gl_in[i].gl_Position;

		gl_Position = local_to_screen * p;
		EmitVertex();

		vec3 n = normal[i];
		gl_Position = local_to_screen * (p + vec4(n*normal_length, 0.0));
		EmitVertex();

		EndPrimitive();
	}
}
