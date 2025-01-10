/*! \file
Terrain (\ref terrain) draw functions. */
#pragma once
#include <glm/matrix.hpp>
#include "more_details_terrain_grid.hpp"
#include "lod_tiles_user_input.hpp"
#include "height_overlap_shader_program.hpp"
#include "above_terrain_outline_shader_program.hpp"
#include "grid_of_terrains_lightdir_shader_program.hpp"

//! Draws terrain quad with elevations and sattelite texture.
void draw_terrain(height_overlap_shader_program & shader,
	terrain const & trn,
	unsigned int element_count,  //!< number of quad mesh triengle elements to draw
	glm::mat4 const & local_to_screen,
	size_t elevation_size,  //!< elevaation texture size in pixels
	float height_scale, float elevation_scale,
	render_features const & features);

//! Draws terrain quad as mesh.
void draw_terrain_outlines(above_terrain_outline_shader_program & shader,
	terrain const & trn,
	unsigned int element_count,  // number of quad mesh triengle elements to draw
	glm::vec3 color,
	float height_scale,
	float elevation_scale,
	glm::mat4 local_to_screen);

//! Draws terrain light directions.
void draw_terrain_light_directions(grid_of_terrains_lightdir_shader_program & shader,
	terrain const & trn,
	unsigned int element_count,
	float height_scale,
	float elevation_scale,
	glm::mat4 local_to_screen);
