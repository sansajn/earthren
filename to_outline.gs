#version 320 es

/* Geometry shader program to transform triangles into triangle outline. The program can be used
to draw wireframe for a mesh (of triangles). */

layout(triangles) in;
layout(line_strip, max_vertices = 4) out;

uniform mat4 local_to_screen;   // TODO: should be this word_to_screen instead (normals are in a word space)?

void main() {
	gl_Position = local_to_screen * vec4(gl_in[0].gl_Position.xyz, 1);
	EmitVertex();

	gl_Position = local_to_screen * vec4(gl_in[1].gl_Position.xyz, 1);
	EmitVertex();

	gl_Position = local_to_screen * vec4(gl_in[2].gl_Position.xyz, 1);
	EmitVertex();

	gl_Position = local_to_screen * vec4(gl_in[0].gl_Position.xyz, 1);
	EmitVertex();

	EmitVertex();
}
