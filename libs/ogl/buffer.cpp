#include "gl_api.hpp"
#include "buffer.hpp"

namespace ogl {

buffer::buffer() {
	glGenBuffers(1, &_id);
}

void buffer::bind_to_array_buffer() {
	bind(GL_ARRAY_BUFFER);
}

void buffer::bind_to_element_array_buffer() {
	bind(GL_ELEMENT_ARRAY_BUFFER);
}

void buffer::bind(unsigned target) const {
	glBindBuffer(target, _id);
}

void buffer::data(void const * data, size_t size, unsigned target) {
	bind(target);
	glBufferData(target, size, data, GL_STATIC_DRAW);
}

unsigned buffer::id() const {
	return _id;
}

buffer::~buffer() {
	glDeleteBuffers(1, &_id);
}

}  // ogl
