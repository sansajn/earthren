#version 320 es
in vec3 position;
uniform mat4 local_to_screen;

void main()	{
	gl_Position = local_to_screen * vec4(position, 1.0);
}
