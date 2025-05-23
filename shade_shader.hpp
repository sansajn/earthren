/*! \file */
#pragma once
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include "ogl/gl_api.hpp"

/*! Shader program to draw primitives with basic shading and color.

Shade shader knows about position and normals vertex data. Normals are used to shade primitives
in fragment shader program. Primitives are rendered with shaded color.

TODO: sample code is missing */
struct shade_shader_program {
	shade_shader_program();
	void use() const;
	[[nodiscard]] GLint position_location() const;
	void color(glm::vec3 const & rgb);  //!< \param[in] rgb normalized primitive color
	void light_direction(glm::vec3 const & d);  //!< \param[in] Normalized light direction.
	void local_to_screen(glm::mat4 const & T);  //!< \param[in] Local-to-Screen transformation matrix.
	void normal_to_view(glm::mat3 const & T);
	~shade_shader_program();

private:
	GLuint _prog;  //!< Shader program ID.
	GLint _position,
		_normal,
		_local_to_screen,
		_normal_to_view,  // TODO: rename to normal_to_view
		_color,
		_light_dir;
};
