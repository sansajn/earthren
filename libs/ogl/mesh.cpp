#include "mesh.hpp"
#include "gl_api.hpp"

namespace ogl {

mesh::mesh(void const * vertices, size_t size_bytes, size_t vertex_count, vertex_layout const & layout)
	: _element_count{vertex_count}, _topology{layout.topology}
{
	_vao.bind();
	_vbo.data(vertices, size_bytes, GL_ARRAY_BUFFER);

	for (unsigned attr_idx = 0; vertex_attribute const & attr : layout.attributes) {
		// WARNING: integer types not supported
		glVertexAttribPointer(attr_idx, attr.size, attr.type, GL_FALSE, layout.stride, reinterpret_cast<void *>(attr.offset));
		glEnableVertexAttribArray(attr_idx);
		++attr_idx;
	}
}

mesh::mesh(void const * vertices, size_t vertices_size_bytes, unsigned const * indices,
	size_t indices_size, vertex_layout const & layout)
		: _element_count{indices_size}, _topology{layout.topology}
{
	// TODO: can we do partial constructor delegation there?

	_vao.bind();
	_vbo.data(vertices, vertices_size_bytes, GL_ARRAY_BUFFER);

	for (unsigned attr_idx = 0; vertex_attribute const & attr : layout.attributes) {
		// WARNING: integer types not supported
		glVertexAttribPointer(attr_idx, attr.size, attr.type, GL_FALSE, layout.stride, reinterpret_cast<void *>(attr.offset));
		glEnableVertexAttribArray(attr_idx);
		++attr_idx;
	}

	_ebo.data(indices, indices_size*sizeof(unsigned), GL_ELEMENT_ARRAY_BUFFER);
}

void mesh::bind() const {
	_vao.bind();
}

unsigned mesh::size() const {
	return _element_count;
}

unsigned mesh::topology() const {
	return _topology;
}

}  // ogl
