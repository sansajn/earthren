#include "axes_model.hpp"

using glm::mat4, glm::vec3;

axes_model::axes_model(GLuint axes_vbo)
	: _axes_vbo{axes_vbo}
{}

void axes_model::draw(flat_shader_program & program, mat4 const & local_to_screen) {
	program.local_to_screen(local_to_screen);

	GLint const position_loc = program.position_location();

	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, _axes_vbo);

	// x
	program.color(vec3{1,0,0});
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glDrawArrays(GL_LINES, 0, 2);

	// y
	program.color(vec3{0,1,0});
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glDrawArrays(GL_LINES, 2, 2);

	// z
	program.color(vec3{0,0,1});
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glDrawArrays(GL_LINES, 4, 2);
}
