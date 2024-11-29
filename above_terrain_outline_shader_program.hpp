/*! \file */
#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <GLES3/gl32.h>

/* TODO: there are three shader program implementationss there flat_shader_program, ...
we should think to reuse common code. */
struct above_terrain_outline_shader_program {
	above_terrain_outline_shader_program();
	~above_terrain_outline_shader_program();
	void use() const;
	void local_to_screen(glm::mat4 const & T);
	void elevation_map(int texture_unit_id);  //!< Set elevation data as OpenGL texture.
	void elevation_scale(float scale);
	void height_scale(float scale);  // TODO: what is difference between elevation_sace and height_sacel?
	void fill_color(glm::vec3 const & color);
	GLint position_location() const;

private:
	GLuint _prog;  //!< Shader program ID.
	GLint _position,
		_heights,
		_elevation_scale,
		_height_scale,
		_local_to_screen,
		_fill_color;
};
