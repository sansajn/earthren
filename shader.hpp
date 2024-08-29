#pragma once
#include <GLES3/gl32.h>

GLint get_shader_program(char const * vertex_shader_source,
	char const * fragment_shader_source, char const * geometry_shader_source = nullptr);
