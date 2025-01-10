#include <filesystem>
#include <ranges>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <cmath>
#include <spdlog/spdlog.h>
#include "geometry/glmprint.hpp"
#include "texture.hpp"
#include "lod_tiles_terrain_grid.hpp"

// to implement is_above()
#include <boost/geometry/algorithms/intersects.hpp>
#include "geometry/box2.hpp"

// to load dataset description file
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using std::map, std::vector;
using std::string, std::to_string;
using std::filesystem::path;
using std::pair;
using std::unique_ptr, std::make_unique;
using std::tuple, std::get;
using glm::vec2, glm::vec3;

namespace {  //!< Helper functions.

//! Helper function to calculate word position from grid (coumn, row) position.
vec2 to_word_position(int column, int row, int level, float quad_size);

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


int terrain_grid::grid_size(int level) const {
	return std::pow(2, level);
}

vector<terrain> terrain_grid::load_level_tiles(path const & data_path, int level) const {
	using std::filesystem::directory_iterator;
	using std::regex, std::smatch, std::regex_match;

	path const tile_directory = data_path;
	if (!exists(tile_directory)) {
		spdlog::error("tile directory '{}' does not exists", tile_directory.c_str());
		return {};
	}

	vector<terrain> terrains;

	// - make a list of tiles froom tiles_directory
	// - we expect `.+_elev_C_R.tif` and `.+_rgb_C_R.tif` tile files there
	for (auto const & dir_entry: directory_iterator{tile_directory}) {
		path const & file = dir_entry.path();  // full path tile filename

		// - for each elevation tile
		if (file.filename().string().starts_with(_elevation_tile_prefix)) {
			// - parse grid collumn (C) and row (R) position
			regex const tile_pattern{R"(.+(\d+)_(\d+)\.tif)"};  // (column), (row)
			smatch what;
			string const tile_name = file.filename().string();
			if (!regex_match(tile_name, what, tile_pattern))
				continue;  // skip file

			// - check we have coresponding rgb file
			string const column_str = what[1].str(),
				row_str = what[2].str();
			path const satellite_path = tile_directory / path{fmt::format("{}{}_{}.tif", _satellite_tile_prefix, column_str, row_str)};
			if (!exists(satellite_path)) {
				spdlog::info("corresponding satellite data for elevation tile ('{}') not found", file.c_str());
				continue;
			}

			// - calculate terrain word position
			int const column = stoi(column_str),
				row = stoi(row_str);

			// level_quad_size is set in a way that LOD level 1 quad_size=1.0 that works because we are not rendering level 0 tile in the sample
			float const level_quad_size = (2.0f*quad_size) / grid_size(level);
			vec2 const world_pos = to_word_position(column, row, grid_size(level), level_quad_size);
			spdlog::debug("{}: level={}, level_quad_size={}, word_pos={}", tile_name, level, level_quad_size, to_string(world_pos));

			// - load elevation tile
			auto const elevation_tile = create_texture_16b(file);
			assert(is_square(elevation_tile) && "we expect square elevation tiles");
			assert(size_t(elevation_tile_size(level)) == get<1>(elevation_tile) && "unexpected elevation tile size");

			// - load satellite tile
			auto const satellite_tile = create_texture_8b(satellite_path);
			assert(is_square(satellite_tile));
			assert(size_t(satellite_tile_size(level)) == get<1>(satellite_tile) && "unexpected satellite tile size");

			// - create terrain instance and filll maps and position
			terrain trn;
			trn.elevation_map = get_tid(elevation_tile);
			trn.satellite_map = get_tid(satellite_tile);
			trn.position = world_pos;
			trn.grid_c = column;
			trn.grid_r = row;

			// - calculate elevation max value
			trn.elevation_min = _elevation_tile_max_value.at(file.filename());  // can thrrow std::out_of_range

			// - add to the terrain quad tree
			terrains.push_back(trn);
		}
	}

	return terrains;
}

struct terrain_finder {
	terrain_finder(int grid_c, int grid_r) : grid_c{grid_c}, grid_r{grid_r} {}

	bool operator()(terrain const & trn) const {
		return trn.grid_c == grid_c && trn.grid_r == grid_r;
	}

	int grid_c, grid_r;
};

void terrain_grid::load_tiles(path const & data_path) {
	// load terrains, for now let's assume level 1 and 2 only
	auto const data_l1_path = data_path/"level1";

	load_description(data_l1_path, 1);  // read dataset description file for level 2 tiles

	vector<terrain> terrains_l1 = load_level_tiles(data_l1_path, 1);

	// construct level 1 quad tree
	assert(std::size(terrains_l1) == 4 && "this sample expect 4 level 2 quadtree tiles");
	for (terrain & trn : terrains_l1) {
		int const idx = trn.grid_c + trn.grid_r * 2;
		assert(idx <= 4 && "four terrains are expected, not more");
		trn.level = 1;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->trn = trn;
		_root.children[idx] = std::move(quad);
	}
	assert(std::ranges::all_of(_root.children, [](auto const & e){return e != nullptr;}) && "we expect all children quads are assigned");

	_elevation_tile_max_value.clear();  // before we can load next level, clear temporary max-values

	auto const data_l2_path = data_path/"level2";
	load_description(data_l2_path, 2);

	// construct level 2 quad tree
	vector<terrain> terrains_l2 = load_level_tiles(data_l2_path, 2);
	auto & root = _root.children[0];

	{
		auto trn_it = std::ranges::find_if(terrains_l2, terrain_finder{0, 0});
		assert(trn_it != end(terrains_l2) && "terrain (0,0) is missing");
		trn_it->level = 2;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->trn = *trn_it;
		root->children[0] = std::move(quad);
	}

	{
		auto trn_it = std::ranges::find_if(terrains_l2, terrain_finder{1, 0});
		assert(trn_it != end(terrains_l2) && "terrain (1,0) is missing");
		trn_it->level = 2;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->trn = *trn_it;
		root->children[1] = std::move(quad);
	}

	{
		auto trn_it = std::ranges::find_if(terrains_l2, terrain_finder{0, 1});
		assert(trn_it != end(terrains_l2) && "terrain (0,1) is missing");
		trn_it->level = 2;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->trn = *trn_it;
		root->children[2] = std::move(quad);
	}

	{
		auto trn_it = std::ranges::find_if(terrains_l2, terrain_finder{1, 1});
		assert(trn_it != end(terrains_l2) && "terrain (1,1) is missing");
		trn_it->level = 2;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->trn = *trn_it;
		root->children[3] = std::move(quad);
	}

	_terrain_count = 7;  // leeaf  terrains
}

void terrain_grid::load_description(path const & data_path, int level) {
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
	for (auto const & kv : dataset.get_child("files"))
		_elevation_tile_max_value.emplace(path{kv.first}, kv.second.get<int>("maxval"));
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
	for (terrain const & trn : leaf_view{_root}) {
		glDeleteTextures(1, &trn.elevation_map);
		glDeleteTextures(1, &trn.satellite_map);
	}
}


namespace {

vec2 to_word_position(int column, int row, int grid_size, float quad_size) {
	vec2 const position = vec2{column, -row} * quad_size - vec2{grid_size, -(grid_size-(2*1))} * quad_size*0.5f;
	return position;
}

}  // namespace
