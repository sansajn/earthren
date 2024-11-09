#pragma once
#include <tuple>
#include <GLES3/gl32.h>

/*! Creates nxn quad mesh on GPU with a size=1.
\return (vao, vbo, ibo, indices_count) tuple. */
std::tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc, unsigned n = 100);

void destroy_quad_mesh(GLuint vao, GLuint vbo, GLuint ibo);
