#pragma once
#include <cstddef>
#include <boost/noncopyable.hpp>

namespace ogl {

constexpr unsigned invalid_buffer_value = 0;

/*! GPU buffer object support. */
struct buffer : private boost::noncopyable {
	buffer();

	void bind_to_array_buffer();  //!< Bind vertex attributes (GL_ARRAY_BUFFER).
	void bind_to_element_array_buffer();  //!< Bind vertex array indices (GL_ELEMENT_ARRAY_BUFFER).

	/*! Bind buffer to target target.
	See glBindBuffer function for more details.
	\param[in] target Target to which the buffer object is bound.
		See target argument of glBindBuffer function for full list of targets.
	\sa bind_to_array_buffer, bind_to_element_array_buffer */
	void bind(unsigned target) const;

	/*! Creates and initializes a buffer object's data store.
	See glBufferData function for more details.
	\param[in] target The same as for bind.
	\sa bind */
	void data(void const * data, size_t size, unsigned target);

	unsigned id() const;
	~buffer();

private:
	unsigned _id = invalid_buffer_value;
};

}  // ogl
