#include "axis_model.hpp"

using glm::mat4, glm::vec3;

namespace {

// three lines
constexpr float axis_verts[] = {
	0,0,0, 1,0,0,  // x
	0,0,0, 0,1,0,  // y
	0,0,0, 0,0,1  // z
};

//! Push data into a new VBO.
GLuint push_data(void const * data, size_t size_in_bytes) {
	// the implementation is not reusable, because we are creating a buffer and also unbins buffer after (this can be slow for more bufffers).
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size_in_bytes, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
	return vbo;
}

GLuint push_axis() {
	return push_data(axis_verts, sizeof(axis_verts));
}

}  // make sure helpers are local

constexpr GLuint invalid_vbo_value = 0;

GLuint axis_model::_axis_vbo = 0;
size_t axis_model::_instance_count = invalid_vbo_value;

axis_model::axis_model() {
	++_instance_count;
	if (_instance_count == 1)
		_axis_vbo = push_axis();
}

void axis_model::draw(flat_shader_program & program, mat4 const & local_to_screen) {
	program.local_to_screen(local_to_screen);

	GLint const position_loc = program.position_location();

	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, _axis_vbo);

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

axis_model::~axis_model() {
	--_instance_count;
	if (_instance_count == 0)
		glDeleteBuffers(1, &_axis_vbo);
}
