/*! \file */
#pragma once
#include <boost/noncopyable.hpp>
#include "buffer.hpp"
#include "vertex_array.hpp"
#include "vertex_layout.hpp"

namespace ogl {

/*! GPU mesh support.

\code
mesh sphere = make_sphere();
sphere.bind();
glDrawElements(spehere.topology(), sphere.size(), GL_UNSIGNED_INT, nullptr);
\endcode */
struct mesh : private boost::noncopyable {
	/*! Create vertices only mesh.
	\param[in] size_bytes Vertex array vertices size in bytes. */
	mesh(void const * vertices, size_t size_bytes, size_t vertex_count, vertex_layout const & layout);

	/*! Create indexed mesh.
	\param[in] indices_size Number of indices in indices array. */
	mesh(void const * vertices, size_t vertices_size_bytes, unsigned const * indices,
		size_t indices_size, vertex_layout const & layout);

	void bind() const;
	[[nodiscard]] unsigned size() const;  //!< \return Number of mesh elements.
	[[nodiscard]] unsigned topology() const;  //!< \return Mesh primitives topology e,g, GL_TRIANGLES, ...

	~mesh() = default;

private:
	buffer _vbo,
	  _ebo;  //!< Element indices.
	vertex_array _vao;
	size_t _element_count;  //!< Elements count.
	unsigned _topology;  //!< Primitive topology e,g, GL_TRIANGLES, ...
};

}  // ogl
