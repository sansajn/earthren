#version 320 es

/* Geometry shader program to transform triangles to a line in a `d` direction with a
specified `d_length` length. The program can fe used to visualize triangle normal or a
light direction. */

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

/* Direction to visualise from a vertex shader program (vertex shader needs pass `d`
variable defined as `out vec3 d;`. */
in vec3 d[];  // TODO: need to find a better name, because in vertex shader `d` is easy do colidate with something else
const float d_length = 0.01;

uniform mat4 local_to_screen;   // TODO: should be this word_to_screen instead (normals are in a word space)?

void main() {
	for (int i = 0; i < gl_in.length(); ++i) {
		vec3 p = gl_in[i].gl_Position.xyz;

		// uncoment this to reduce number of points producing artifacts
//		if ((p.x < 3.0/8.0 || p.x > 1.0/2.0) || p.y < 1.0/18.0 || p.y > 1.0/16.0)  // artifacts can be seen (distance=0.94724894)
//			continue;

		gl_Position = local_to_screen * vec4(p,1);
		EmitVertex();

		gl_Position = local_to_screen * vec4(p + d[i]*d_length, 1);
		EmitVertex();

		EndPrimitive();
	}
}
