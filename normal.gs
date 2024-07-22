#version 320 es

/* geometry shader proram to visualize normals
Implementation expects triangles and for each trianlge vertex it draws a normal calculated by a vertex shader program. */

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in vec3 normal[];  // normals from vertex shader program

uniform mat4 local_to_screen;

const float normal_length = 0.01;

void main() {
   for (int i = 0; i < gl_in.length(); ++i) {
		vec3 p = gl_in[i].gl_Position.xyz;

		// uncoment this to reduce number of points producing artifacts
//		if ((p.x < 3.0/8.0 || p.x > 1.0/2.0) || p.y < 1.0/18.0 || p.y > 1.0/16.0)  // artifacts can be seen (distance=0.94724894)
//			continue;

		gl_Position = local_to_screen * vec4(p,1);
		EmitVertex();

		vec3 n = normal[i];
		gl_Position = local_to_screen * vec4(p + n*normal_length, 1);
		EmitVertex();

		EndPrimitive();
	}
}
