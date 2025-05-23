/*! \file */
#pragma once
#include <vector>
#include <cstddef>

namespace ogl {

/*! Vertex array attribute description.
To support mesh or shape generators together with vertex_layout.

See glVertexAttribPointer OpenGL function for detailed members description.

TODO: put some code sample there ...
\code
\endcode

\sa glVertexAttribPointer, vertex_layout */
struct vertex_attribute {
	int size; //<! Number of vertex components 1,2,3 or 4.
	unsigned type;  //!< Vertex data type e.g. GL_FLOAT, GL_UNSIGNED_INT, ...
	std::ptrdiff_t offset;  //!< Byte offset for the first vertex into vertex data array.
};

//! Vertex array layout description.
struct vertex_layout {
	std::vector<vertex_attribute> attributes;
	int stride;  //!< Byte offset between two vertices in vertex data array.
	unsigned topology;  //!< Type of primitives to render e.g. GL_TRIANGLES, see mode parameter from glDrawElements OpenGL function for full list.
	// TODO: we want to force all members are initialized, it is common source of mistakes that some is forgotten
};

}  // ogl
