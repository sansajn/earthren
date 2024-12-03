#include <filesystem>
#include <ranges>
#include <vector>
#include <glm/vec2.hpp>
#include <GLES3/gl32.h>

/* - we are expecting that all terrains has the same size textures so no reason to store texture w/h
- grid_size is also the same for all terrain */
struct terrain {
	GLuint elevation_map,
		satellite_map;
	glm::vec2 position;  //!< Terrain word position (within thee grid).
	float elevation_min;  // TODO: use terrain related value there, TODO: rename to eelevation_max
};

// TODO: elevation_min data are missing during load_tiles in a grid
struct terrain_grid {
	/* TODO: should be read_tiles member of terrain_grid? I think in the first step it is easier to
	implement due to undestricted access and as a second step we can make it non member funnction if
	it makes any sence. */

	// TODO: check that elevation tiles are all the same (width, height), the same for satellite tiles
	// TODO: we want to get rid og elevation_tile_prefix and satellite_tile_prefix they should be read from data_path config file
	void load_tiles(std::filesystem::path const & data_path, std::string const & elevation_tile_prefix,
		std::string const & satellite_tile_prefix);
	[[nodiscard]] size_t size() const {return std::size(_terrains);}

	/*! \returns Range to iterate through list of terrains.
	\code
	terrain_grid terrains;
	// ...
	for (terrain const & terrains.iterate()) {...}
	\endcode */
	auto iterate() const {  //!< \returns list of terrains as range
		return std::ranges::subrange{std::begin(_terrains), std::end(_terrains)};
	}

	// TODO: some basic tile informations
	int elevation_tile_size;  //= 716, TODO: this is set during load_tiles()

	int grid_column_count = 2;
	float quad_size = 1.0f;

	/* TODO: This is how wee work with elevations in a vertx shader program
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale; */
	constexpr static int elevation_tile_max_value[4] = {
		564, 726,
		625, 805
	};

	~terrain_grid() {
		for (terrain const & trn : _terrains) {  // TODO: terrain is now owner of textures so it is terrain responsibility to delete textures
			glDeleteTextures(1, &trn.elevation_map);
			glDeleteTextures(1, &trn.satellite_map);
		}
	}

private:
	std::vector<terrain> _terrains;  // TODO: we need cleanup of textures in destructor
};
