#pragma once
#include <tuple>
#include <GLES3/gl32.h>

/*! Creates quad mesh on GPU with a size=1.
\return (vao, vbo, ibo, indices_count) tuple. */
std::tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc);
