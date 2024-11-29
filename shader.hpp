#pragma once
#include <GLES3/gl32.h>

//! \return OpenGL program object ID, 0 if an error ocurs.
GLuint get_shader_program(char const * vertex_shader_source,
	char const * fragment_shader_source, char const * geometry_shader_source = nullptr);
