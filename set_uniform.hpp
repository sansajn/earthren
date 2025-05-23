/*! \file */
#include <utility>
#include <vector>

// TODO: missing description how set_uniform is meant to be used

// TODO: define doxygen setter group there

/*! Uniform setter helpers.
\code
mat4 T = transform_matrix();
GLuint prog_id = create_shader_program();
GLint _local_to_screen = glGetUniformLocation(prog_id, "local_to_screen");
set_uniform(_local_to_screen, T);
\endcode */
template <typename T>
void set_uniform(int location, T const & v);

template <typename T>
void set_uniform(int location, T const * a, int n);

template <typename T>
void set_uniform(int location, std::vector<T> const & arr) {
	set_uniform(location, arr.data(), arr.size());
}

//! for array where pair::first is array pointer and pair::second is number of array elements
template <typename T>
void set_uniform(int location, std::pair<T *, int> const & a) {
	set_uniform(location, a.first, a.second);
}
