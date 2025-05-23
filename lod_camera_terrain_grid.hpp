/*! \file */
#pragma once
#include <functional>
#include <filesystem>
#include <memory>
#include <map>
#include <ranges>
#include <stack>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <GLES3/gl32.h>
#include "qtree_traverse.hpp"

/* - we are expecting that all terrains has the same size textures so thre is no reason to store texture w/h
- grid_size is also the same for all terrain */
struct terrain {
	GLuint elevation_map,
		satellite_map;
	glm::vec2 position;  //!< Terrain word position (within the grid).
	float elevation_min;

	// We need following during rendering to calculate terrain position. These are the same values as in terrain_quad.
	int grid_c, grid_r;  //!< Grid position.
	int level = -1;  //!< Terrain level of detail (it is actually quadtree level/depth).
};

/*! Function to find out whether position is above a terrain.
E.g. To figure out whether camera is above a terrain. */
bool is_above(terrain const & trn, float quad_size, float model_scale, glm::vec3 const & pos);

/*! Terrain quad tree element.
\sa terrain_grid */
struct terrain_quad {
	using value_type = terrain;

	terrain_quad * parent = nullptr;
	std::array<std::unique_ptr<terrain_quad>, 4> children;
	bool is_leaf() const {return children[0] == nullptr;}
	value_type trn;  // TODO: make a uniqueptr from this

	// We need following during reading tiles to know what tile should we read.
	int depth,  //!< Tree depth.
		grid_c, grid_r;  //!< Grid coordinates.
};

/*! Represents terrain grid.
Despite the name internal implemenation use quad-tree (not grid).

Funcition update_camera needs to be called on every camera change to adapt grid LOD.

To render terrain_grid instance use iterate() method to get terrain type instances in the first step.
Then to render terrain call draw_terrain function with terrain instance as an argument.*/
struct terrain_grid {
	/* TODO: should be load_tiles member of terrain_grid? I think in the first step it is easier to
	implement it due to unrestricted access and as a second step we can make it non member funnction if
	it still makes sence. */

	terrain_grid();

	void load_tiles(std::filesystem::path const & data_path);

	/*! Call every time camera positon change.
	Adds or removes details by split/merge operatoin on underlaying terrain quad-tree. */
	void update_camera(glm::vec3 const & pos);  // TODO: rename to update(camera_pos)

	[[nodiscard]] size_t size() const;  //!< \returns number of leaf terrains in grid

	/*! \returns Range to iterate through list of terrains.
	\code
	terrain_grid terrains;
	// ...
	for (terrain const & terrains.iterate()) {...}
	\endcode */
	[[nodiscard]] auto iterate() const {
		return qtree::leaf_view{_root, &terrain_quad::trn};
	}

	[[nodiscard]] int grid_size(int level) const;
	[[nodiscard]] int elevation_tile_size(int level) const;
	[[nodiscard]] double elevation_pixel_size(int level) const;
	[[nodiscard]] int satellite_tile_size(int level) const;

	float quad_size = 1.0f;  // TODO: Document who is responsible to change this value?

	static float camera_ground_height;  //!< Terrain ground height bellow camera. Camera needs to have an access to the property.

	~terrain_grid();

private:
	void load_description(std::filesystem::path const & data_path, int level);  // TODO: implementation of this needs to be changed, because it create a state
	int elevation_maxval(std::filesystem::path const & filename) const;

	std::unique_ptr<terrain> load_tile(std::filesystem::path const & data_path, int level, int grid_c, int grid_r);

	void split_node(terrain_quad & parent);  //!< Split quad-tree node.
	void merge_node(terrain_quad & parent);  //!< Merge children nodes.

	terrain_quad _root;  //!< terrains in a quadtree structure to allow LOD

	std::string _elevation_tile_prefix,
		_satellite_tile_prefix;

	struct dataset_description {
		int elevation_tile_size,
			satellite_tile_size;
		double elevation_pixel_size;
	};

	std::map<int, dataset_description> _data_desc;  //!< level based dataset descriptions

	size_t _terrain_count = 0;

	/*! Serves as a temporary variable for load_description function and used during creating terrain in
	load_tile function.
	\note This is how we work with elevations in a vertx shader program
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale; */
	std::map<int,
		std::map<std::filesystem::path, int>> _elevation_tile_max_value;

	std::filesystem::path _data_path;  // TODO: temporary, used in update_camera implementation
};  // terrain_grid
