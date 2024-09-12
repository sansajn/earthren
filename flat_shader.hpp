/*! \file */
#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <GLES3/gl32.h>

/*! Shader program to draw primitive with color. */
class flat_shader_program {
public:
	flat_shader_program(GLuint program_id);
	void use() const;
	GLint position_location() const;
	void color(glm::vec3 const & rgb);  //!< \param[in] rgb normalized primitive color
	void local_to_screen(glm::mat4 const & T);

private:
	GLuint _prog;  //!< Shader program ID.
	// program::uniform_type _color_u,
		// _local_to_screen_u;
	// TODO: we can have uniform type
	GLint _position,
		_local_to_screen,
		_color;
};
