#include "color.hpp"
#include "lod_tiles_draw_terrain.hpp"

using glm::mat4, glm::vec3;

void draw_terrain(height_overlap_shader_program & shader,
	terrain const & trn,
	unsigned int element_count,
	mat4 const & local_to_screen,
	size_t elevation_size,
	float height_scale, float elevation_scale,
	render_features const & features) {

	shader.use();

	// bind height map texture
	shader.heights(0);  // set height map sampler to use texture unit 0
	glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
	glBindTexture(GL_TEXTURE_2D, trn.elevation_map);  // bind a height texture to active texture unit (0)

	if (features.show_satellite) {
		shader.use_satellite_map(true);
		shader.satellite_map(1);  // set satellite map sampler to use texture unit 1
		glActiveTexture(GL_TEXTURE1);  // activate texture unit 1
		glBindTexture(GL_TEXTURE_2D, trn.satellite_map);  // bind a satellite texture to active texture unit (1)
	}
	else
		shader.use_satellite_map(false);

	if (features.calculate_shades)
		shader.use_shading(true);
	else
		shader.use_shading(false);

	constexpr float elevation_tile_pixel_size = 26.063200588611451;  // see gdalinfo

	shader.terrain_size(elevation_size * elevation_tile_pixel_size);
	shader.elevation_tile_size(elevation_size);
	shader.normal_tile_size(elevation_size - 4);  // 2px border
	shader.height_scale(height_scale);
	shader.elevation_scale(elevation_scale);
	shader.local_to_screen(local_to_screen);

	glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
}

void draw_terrain_outlines(above_terrain_outline_shader_program & shader,
	terrain const & trn,
	unsigned int element_count,  // number of quad mesh triengle elements to draw
	vec3 color,
	float height_scale,
	float elevation_scale,
	mat4 local_to_screen) {

	shader.use();

	shader.fill_color(color);

	// bind height map
	shader.elevation_map(0);  // set sampler s to use texture unit 0
	glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
	glBindTexture(GL_TEXTURE_2D, trn.elevation_map);  // bind a texture to active texture unit (0)

	shader.elevation_scale(elevation_scale);
	shader.height_scale(height_scale);
	shader.local_to_screen(local_to_screen);

	glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
}

void draw_terrain_light_directions(grid_of_terrains_lightdir_shader_program & shader,
	terrain const & trn,
	unsigned int element_count,
	float height_scale,
	float elevation_scale,
	mat4 local_to_screen) {

	shader.use();
	shader.fill_color(rgb::yellow);

	// bind height map
	shader.elevation_map(0);  // set sampler s to use texture unit 0
	glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
	glBindTexture(GL_TEXTURE_2D, trn.elevation_map);  // bind a texture to active texture unit (0)

	shader.elevation_scale(elevation_scale);
	shader.height_scale(height_scale);
	shader.local_to_screen(local_to_screen);

	glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
}
