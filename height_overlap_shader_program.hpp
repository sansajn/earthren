/*! \file */
#pragma once
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <GLES3/gl32.h>

class height_overlap_shader_program {
public:
	height_overlap_shader_program();
	~height_overlap_shader_program();
	void use();
	void local_to_screen(glm::mat4 const & T);
	void heights(int texture_unit_id);  //!< Set elevation data.
	void elevation_scale(float scale);
	void height_scale(float scale);
	void satellite_map(int texture_unit_id);
	void use_satellite_map(bool value);
	void use_shading(bool value);
	void terrain_size(float size);  //!< terrain size in real world units e.g. meters
	void elevation_tile_size(float size);
	void normal_tile_size(float size);
	GLint position_location() const;

private:
	GLuint _prog;  //!< Shader program ID.
	GLint _position,
		_local_to_screen,
		_heights,
		_elevation_scale,
		_height_scale,
		_normal_tile_size,
		_satellite_map,
		_height_map_size,
		_use_satellite_map,
		_use_shading,
		_terrain_size,
		_elevation_tile_size;
};
