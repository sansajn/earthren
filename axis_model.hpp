/*! \file */
#pragma once
#include <glm/matrix.hpp>
#include <GLES3/gl32.h>
#include <boost/noncopyable.hpp>
#include "flat_shader.hpp"

/*! Axis model.
We want to share geometry (VBO buffers) accross multiple model instances. */
class axis_model : private boost::noncopyable {
public:
	axis_model();
	void draw(flat_shader_program & program, glm::mat4 const & local_to_screen);

	~axis_model();

private:
	static GLuint _axis_vbo;
	static size_t _instance_count;
};
