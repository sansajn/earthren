#include "flat_shader.hpp"
#include <glm/gtc/type_ptr.hpp>

using glm::vec3,
	glm::mat4,
	glm::value_ptr;

flat_shader_program::flat_shader_program(GLuint program_id)
	: _prog{program_id}
{
	_position = glGetAttribLocation(_prog, "position");
	_local_to_screen = glGetUniformLocation(_prog, "local_to_screen");
	_color = glGetUniformLocation(_prog, "color");
}

void flat_shader_program::use() const {
	glUseProgram(_prog);
}

GLint flat_shader_program::position_location() const {
	return _position;
}

void flat_shader_program::color(vec3 const & rgb) {
	glUniform3fv(_color, 1, value_ptr(rgb));
}

void flat_shader_program::local_to_screen(glm::mat4 const & T) {
	glUniformMatrix4fv(_local_to_screen, 1, GL_FALSE, value_ptr(T));
}
