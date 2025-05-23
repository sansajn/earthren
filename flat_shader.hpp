/*! \file */
#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <GLES3/gl32.h>

/*! Shader program to draw primitives with color.

Can be used when shading/lighting is not required and object needs to be rendered just with color like axis.

Flat shader only knows about vertex positions and primitives are rendered with color. There is not any
support for shading/lighting there.

Used with `flat_shader.vs` and `flat_shader.fs` shader programs.

\code
string const flat_vs = read_file("flat_shader.vs"),
	flat_fs = read_file("flat_shader.fs");
GLuint const flat_shader_program_id = get_shader_program(flat_vs.c_str(), flat_fs.c_str());

flat_shader_program flat_shader{flat_shader_program_id};
\endcode */
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
