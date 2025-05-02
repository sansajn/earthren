/*! \file */
#pragma once
#include <filesystem>
#include <map>
#include <ranges>
#include <vector>
#include <glm/vec2.hpp>
#include <GLES3/gl32.h>

/* - we are expecting that all terrains has the same size textures so thre is no reason to store texture w/h
- grid_size is also the same for all terrain */
struct terrain {
	GLuint elevation_map,
		satellite_map;
	glm::vec2 position;  //!< Terrain word position (within thee grid).
	float elevation_min;  // TODO: use terrain related value there, TODO: rename to eelevation_max

	int grid_c, grid_r; // TODO: grid position for debug
};

/*! Function to find out whether position is above a terrain.
E.g. To figure out whether camera is above a terrain. */
bool is_above(terrain const & trn, float quad_size, float model_scale, glm::vec3 const & pos);

// TODO: elevation_min data are missing during load_tiles in a grid
struct terrain_grid {
	/* TODO: should be read_tiles member of terrain_grid? I think in the first step it is easier to
	implement due to undestricted access and as a second step we can make it non member funnction if
	it makes any sence. */

	// TODO: check that elevation tiles are all the same (width, height), the same for satellite tiles
	// TODO: we want to get rid og elevation_tile_prefix and satellite_tile_prefix they should be read from data_path config file
	void load_tiles(std::filesystem::path const & data_path);
	[[nodiscard]] size_t size() const {return std::size(_terrains);}

	/*! \returns Range to iterate through list of terrains.
	\code
	terrain_grid terrains;
	// ...
	for (terrain const & terrains.iterate()) {...}
	\endcode */
	[[nodiscard]] auto iterate() const {  //!< \returns list of terrains as range
		return std::ranges::subrange{std::begin(_terrains), std::end(_terrains)};
	}

	[[nodiscard]] int grid_size() const {return _grid_size;}
	[[nodiscard]] int elevation_tile_size() const {return _elevation_tile_size;}
	[[nodiscard]] double elevation_pixel_size() const {return _elevation_pixel_size;}

	float quad_size = 1.0f;

	static float camera_ground_height;  //!< Terrain ground height bellow camera. Camera needs to have an access to the property.

	~terrain_grid() {
		for (terrain const & trn : _terrains) {  // TODO: terrain is now owner of textures so it is terrain responsibility to delete textures
			glDeleteTextures(1, &trn.elevation_map);
			glDeleteTextures(1, &trn.satellite_map);
		}
	}

private:
	void load_description(std::filesystem::path const & data_path);
	int elevation_maxval(std::filesystem::path const & filename) const;

	std::vector<terrain> _terrains;
	int _grid_size,
		_elevation_tile_size,
		_satellite_tile_size;
	std::string _elevation_tile_prefix,
		_satellite_tile_prefix;
	double _elevation_pixel_size;

	/* TODO: This is how wee work with elevations in a vertx shader program
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale; */
	std::map<std::filesystem::path, int> _elevation_tile_max_value;
};
