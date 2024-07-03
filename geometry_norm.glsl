// vyzualizuje normaly vrcholov

uniform mat4 local_to_screen;
uniform float normal_length = 0.01;

#ifdef _VERTEX_
layout(location = 0) in vec3 position;
layout(location = 2) in vec3 normal;

out Vertex {
	vec3 normal;
} vertex;

void main()
{
	vertex.normal = normal;
	gl_Position = vec4(position,1);
}
#endif  // _VERTEX_

#ifdef _GEOMETRY_
layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in Vertex {
	vec3 normal;
} vertex[];

void main()
{
	for (int i = 0; i < gl_in.length(); ++i)
	{
		vec3 p = gl_in[i].gl_Position.xyz;
		gl_Position = local_to_screen * vec4(p,1);
		EmitVertex();
		
		vec3 n = vertex[i].normal.xyz;
		gl_Position = local_to_screen * vec4(p + n*normal_length, 1);
		EmitVertex();
		
		EndPrimitive();
	}
}
#endif  // _GEOMETRY_

#ifdef _FRAGMENT_
out vec4 color;

void main()
{
	color = vec4(0,1,0,1);
}
#endif  // _FRAGMENT_
