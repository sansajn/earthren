#include "vertex_array.hpp"
#include "gl_api.hpp"

namespace ogl {

vertex_array::vertex_array() {
	glGenVertexArrays(1, &_id);
}

void vertex_array::bind() const {
	glBindVertexArray(_id);
}

vertex_array::~vertex_array() {
	glDeleteVertexArrays(1, &_id);
}

}  // ogl
