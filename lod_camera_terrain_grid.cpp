#include <filesystem>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <cmath>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include "geometry/glmprint.hpp"
#include "texture.hpp"
#include "lod_camera_terrain_grid.hpp"

// to implement is_above()
#include <boost/geometry/algorithms/intersects.hpp>
#include "geometry/box2.hpp"

// to load dataset description file
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using std::map, std::set;
using std::string, std::to_string;
using std::filesystem::path;
using std::pair;
using std::unique_ptr, std::make_unique;
using std::tuple, std::get;
using glm::vec2, glm::vec3;

namespace {  //!< Helper functions.

//! Helper function to calculate word position from grid (coumn, row) coordinates.
vec2 to_word_position(int column, int row, int grid_size, float quad_size);

bool is_square(tuple<GLuint, size_t, size_t> const & tile) {
	return get<1>(tile) == get<2>(tile);
}

GLuint get_tid(tuple<GLuint, size_t, size_t> const & tile) {  //!< get OpenGL texture ID
	return get<0>(tile);
}

}  // namespace

bool is_above(terrain const & trn, float quad_size, float model_scale, vec3 const & pos) {
	namespace bg = boost::geometry;

	// calculate terrain bounding box (it is axis aligned)
	// the formula is `(position + quad_size) * model_scale`
	vec2 const min_corner = trn.position * model_scale,
		max_corner = (trn.position + quad_size) * model_scale;

	geom::box2 const tile_area = {min_corner, max_corner};

	return bg::intersects(vec2{pos}, tile_area);
}


float terrain_grid::camera_ground_height = 0.0f;


int terrain_grid::grid_size(int level) const {  //!< Number of tiles in one direction.
	return std::pow(2, level);
}

// TODO: Is this case for optional as return value?
// loading textures is anyway slow so we can return unique_ptr there
std::unique_ptr<terrain> terrain_grid::load_tile(path const & data_path, int level, int grid_c, int grid_r) {
	path const tile_directory = data_path;
	if (!exists(tile_directory)) {
		spdlog::error("tile directory '{}' does not exists", tile_directory.c_str());
		return {};
	}

	// In a tile directory there are `.+_elev_C_R.tif` and `.+_rgb_C_R.tif` tile files.
	string const column_str = to_string(grid_c),
		row_str = to_string(grid_r);

	// - find out tile texture names
	path const elevation_path = tile_directory / path{fmt::format("{}{}_{}.tif", _elevation_tile_prefix, column_str, row_str)};
	if (!exists(elevation_path)) {
		spdlog::info("corresponding elevation data for tile ('{}') not found", elevation_path.c_str());
		return {};
	}

	path const satellite_path = tile_directory / path{fmt::format("{}{}_{}.tif", _satellite_tile_prefix, column_str, row_str)};
	if (!exists(satellite_path)) {
		spdlog::info("corresponding satellite data for tile ('{}') not found", satellite_path.c_str());
		return {};
	}

	// - calculate terrain word position

	// level_quad_size is set in a way that for LOD level 1 level_quad_size=1.0 that works because we are not rendering level 0 tile in the sample
	float const level_quad_size = (2.0f*quad_size) / grid_size(level);
	vec2 const world_pos = to_word_position(grid_c, grid_r, grid_size(level), level_quad_size);

	string const elevation_tile_name = elevation_path.filename().string();
	spdlog::debug("{}: level={}, level_quad_size={}, word_pos={}", elevation_tile_name, level, level_quad_size, to_string(world_pos));

	// - load elevation tile
	auto const elevation_tile = create_texture_16b(elevation_path);
	assert(is_square(elevation_tile) && "we expect square elevation tiles");
	assert(size_t(elevation_tile_size(level)) == get<1>(elevation_tile) && "unexpected elevation tile size");

	// - load satellite tile
	auto const satellite_tile = create_texture_8b(satellite_path);
	assert(is_square(satellite_tile));
	assert(size_t(satellite_tile_size(level)) == get<1>(satellite_tile) && "unexpected satellite tile size");

	// - create terrain instance and filll maps and position
	unique_ptr trn = make_unique<terrain>();
	trn->elevation_map = get_tid(elevation_tile);
	trn->satellite_map = get_tid(satellite_tile);
	trn->position = world_pos;
	trn->grid_c = grid_c;
	trn->grid_r = grid_r;
	trn->level = level;

	// - calculate elevation max value
	trn->elevation_min = _elevation_tile_max_value.at(level).at(elevation_path.filename());  // can throw std::out_of_range

	return trn;
}

void terrain_grid::split_node(terrain_quad & parent) {
	assert(parent.is_leaf() && "we can only split leaf notes");

	// NOTE: The implementation would be much faster if we could allocate 4 quads in one command and not call new for each quad.

	auto & child0 = parent.children[0];
	child0 = make_unique<terrain_quad>();
	child0->parent = &parent;
	child0->depth = parent.depth + 1;
	child0->grid_c = 2*parent.grid_c;
	child0->grid_r = 2*parent.grid_r;

	auto & child1 = parent.children[1];
	child1 = make_unique<terrain_quad>();
	child1->parent = &parent;
	child1->depth = parent.depth + 1;
	child1->grid_c = 2*parent.grid_c + 1;
	child1->grid_r = 2*parent.grid_r;

	auto & child2 = parent.children[2];
	child2 = make_unique<terrain_quad>();
	child2->parent = &parent;
	child2->depth = parent.depth + 1;
	child2->grid_c = 2*parent.grid_c;
	child2->grid_r = 2*parent.grid_r + 1;

	auto & child3 = parent.children[3];
	child3 = make_unique<terrain_quad>();
	child3->parent = &parent;
	child3->depth = parent.depth + 1;
	child3->grid_c = 2*parent.grid_c + 1;
	child3->grid_r = 2*parent.grid_r + 1;
}

void terrain_grid::merge_node(terrain_quad & parent) {
	parent.children[0].reset();
	parent.children[1].reset();
	parent.children[2].reset();
	parent.children[3].reset();
}

terrain_grid::terrain_grid() {
	_root.depth = 0;
	_root.grid_c = 0;
	_root.grid_r = 0;
}


void terrain_grid::load_tiles(path const & data_path) {
	// level 0 is not read (we expect four tiles, but there is just one tile in level 0 directory)

	_data_path = data_path;

	// load terrains, for now let's assume level 1 and 2 only
	auto const data_l1_path = data_path/"level1";

	load_description(data_l1_path, 1);  // read dataset description file for level 1 tiles

	// construct level 1 quad tree
	split_node(_root);

	// TODO: it would be easier to store pointer in terain_quad instead of move from pointer
	for (unique_ptr<terrain_quad> & child : _root.children) {
		unique_ptr<terrain> trn = load_tile(data_l1_path, child->depth, child->grid_c, child->grid_r);
		assert(trn && "we expect valid terrain tile");
		child->trn = std::move(*trn);
	}

	// check level 1 childs
	assert(std::ranges::all_of(_root.children, [](auto const & q){return q != nullptr;}) && "we expect all children quads are populated");

	_terrain_count = 4;  // leaf terrains
}

// TODO: Calculations based on pos are wrong we ignore camera rotation, so even we are close we can look somwhere else and not see terrain.
void terrain_grid::update_camera(glm::vec3 const & pos) {
	std::set<terrain_quad *> nodes_to_merge;  // TODO: use unordered_set<> we expect just a few merged nodes there

	// - iterate leafs
	for (terrain_quad & leaf : qtree::leaf_view{_root}) {
		// - for each leaf calculate distance d
		float const level_quad_size = (2.0f*quad_size) / grid_size(leaf.depth);
		// TODO: we want to use tile center position there
		vec2 const leaf_world_pos = to_word_position(leaf.grid_c, leaf.grid_r, grid_size(leaf.depth), level_quad_size);
		vec3 d_vec = pos - vec3{leaf_world_pos, 0};   // TODO: calculate with real height (not with 0)
		float const d = glm::length(d_vec);

		constexpr float level2_distance_trigger = 3.f;

		// - split to level 2 if close
		if (d <= level2_distance_trigger && leaf.depth == 1) {
			split_node(leaf);

			// load tile
			int level = leaf.depth+1;
			auto const data_level_path = _data_path / fmt::format("level{}", level);
			load_description(data_level_path, level);  // read dataset description file for new level tiles

			// TODO: we can use leaf_view there after it will be fixed to return references
			for (unique_ptr<terrain_quad> & child : leaf.children) {
				unique_ptr<terrain> trn = load_tile(data_level_path, child->depth, child->grid_c, child->grid_r);
				assert(trn && "we expect valid terrain tile");
				child->trn = std::move(*trn);
			}

			spdlog::debug("split: terrain quad tree node (L={},C={},R={}) splited, dist={}", leaf.depth, leaf.grid_c, leaf.grid_r, d);

			_terrain_count += 4;
		}
		else if (d > level2_distance_trigger && leaf.depth == 2) {  // merge back to level 1 if far
			nodes_to_merge.insert(leaf.parent);  // TODO: merge is not working at all, all childend needs to be far to merge
		}
	}

	for (auto * node : nodes_to_merge) {  // merge all nodes
		merge_node(*node);  // TODO: merge only if all children are far enough otherwise infinite split is followed by merge sequence
		_terrain_count -= 4;
		spdlog::debug("merge: terrain quad tree node (L={},C={},R={}) merged", node->depth, node->grid_c, node->grid_r);
	}
}

void terrain_grid::load_description(path const & data_path, int level) {
	if (_data_desc.contains(level))
		return;  // nothing new, return

	boost::property_tree::ptree dataset;
	boost::property_tree::read_json(data_path/"dataset.json", dataset);

	/* following properties are mandatory otherwise terminate called after
	throwing an instance of 'boost::wrapexcept<boost::property_tree::ptree_bad_path>'
		what():  No such node (elevation.tile_prefix) */

	_elevation_tile_prefix = dataset.get<string>("elevation.tile_prefix");
	_satellite_tile_prefix = dataset.get<string>("satellite.tile_prefix");

	dataset_description desc;
	desc.elevation_tile_size = dataset.get<int>("elevation.tile_size");
	desc.satellite_tile_size = dataset.get<int>("satellite.tile_size");
	desc.elevation_pixel_size = dataset.get<double>("elevation.pixel_size");
	_data_desc[level] = desc;

	// create list of elevation max values
	auto & elevation_max_values = _elevation_tile_max_value[level];
	for (auto const & kv : dataset.get_child("files"))
		elevation_max_values.emplace(path{kv.first}, kv.second.get<int>("maxval"));
}

size_t terrain_grid::size() const {
	return _terrain_count;
}

int terrain_grid::elevation_tile_size(int level) const {
	return _data_desc.at(level).elevation_tile_size;
}

double terrain_grid::elevation_pixel_size(int level) const {
	return _data_desc.at(level).elevation_pixel_size;
}

int terrain_grid::satellite_tile_size(int level) const {
	return _data_desc.at(level).satellite_tile_size;
}

terrain_grid::~terrain_grid() {
	for (terrain const & trn : iterate()) {
		glDeleteTextures(1, &trn.elevation_map);
		glDeleteTextures(1, &trn.satellite_map);
	}
}


namespace {

vec2 to_word_position(int column, int row, int grid_size, float quad_size) {
	// TODO: What does grid_size-(2*1) does?
	vec2 const position = vec2{column, -row} * quad_size - vec2{grid_size, -(grid_size-(2*1))} * quad_size*0.5f;
	return position;
}

}  // namespace
