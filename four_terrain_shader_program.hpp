/*! \file */
#pragma once
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <GLES3/gl32.h>

class four_terrain_shader_program {
public:
	four_terrain_shader_program();
	~four_terrain_shader_program();
	void use();
	void local_to_screen(glm::mat4 const & T);
	void heights(int texture_unit_id);  //!< Set elevation data.
	void elevation_scale(float scale);
	void height_scale(float scale);
	void satellite_map(int texture_unit_id);
	void use_satellite_map(bool value);
	void use_shading(bool value);
	void height_map_size(glm::vec2 const & size);
	GLint position_location() const;

private:
	GLuint _prog;  //!< Shader program ID.
	GLint _position,
		_local_to_screen,
		_heights,
		_elevation_scale,
		_height_scale,
		_satellite_map,
		_height_map_size,
		_use_satellite_map,
		_use_shading;
};
