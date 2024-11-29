#include <stdexcept>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLES3/gl32.h>
#include "set_uniform.hpp"

template <>
void set_uniform<glm::mat3>(int location, glm::mat3 const & v) {
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(v));
}

template <>
void set_uniform<glm::mat4>(int location, glm::mat4 const & v) {
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(v));
}

template <>
void set_uniform<glm::vec2>(int location, glm::vec2 const & v) {
	glUniform2fv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::vec3>(int location, glm::vec3 const & v) {
	glUniform3fv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::vec4>(int location, glm::vec4 const & v) {
	glUniform4fv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::ivec2>(int location, glm::ivec2 const & v) {
	glUniform2iv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::ivec3>(int location, glm::ivec3 const & v) {
	glUniform3iv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::ivec4>(int location, glm::ivec4 const & v) {
	glUniform4iv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<int>(int location, int const & v) {
	glUniform1i(location, v);
}

// TODO: is this still true in OpenGL ES 3?
template <>
void set_uniform<unsigned>([[maybe_unused]] int location, [[maybe_unused]] unsigned const & v) {
	//	glUniform1ui(location, v);
	throw std::logic_error{"unsigned uniform is not supported in opengl es 2"};
}

template<>
void set_uniform<float>(int location, float const & v) {
	glUniform1f(location, v);
}

template <>
void set_uniform<bool>(int location, bool const & v) {
	glUniform1i(location, v);
}


template <>  // pre pole float-ou
void set_uniform<float>(int location, float const * a, int n) {
	glUniform1fv(location, n, a);
}

template <>  // pre pole vec4 vektorov
void set_uniform<glm::vec4>(int location, glm::vec4 const * a, int n) {
	glUniform4fv(location, n, glm::value_ptr(*a));
}

template <>  // pre pole matic
void set_uniform<glm::mat4>(int location, glm::mat4 const * a, int n) {
	glUniformMatrix4fv(location, n, GL_FALSE, glm::value_ptr(*a));
}
