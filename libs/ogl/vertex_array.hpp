#pragma once
#include <boost/noncopyable.hpp>

namespace ogl {

constexpr unsigned invalid_vao_value = 0;

//! Vertex array object support.
struct vertex_array : private boost::noncopyable {
	vertex_array();
	void bind() const;
	~vertex_array();

private:
	unsigned _id = invalid_vao_value;
};

}  // ogl
