/*! \file */
#pragma once
#include <glm/matrix.hpp>
#include <GLES3/gl32.h>
#include "flat_shader.hpp"

class axes_model {
public:
	axes_model(GLuint axes_vbo);
	void draw(flat_shader_program & program, glm::mat4 const & local_to_screen);

private:
	GLuint _axes_vbo;
};
