#include <filesystem>
#include <ranges>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
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

bool is_above(terrain const & trn, float quad_size, float model_scale, vec3 const & pos) {  // TODO: do we want camera instead of pos there? is_above would make more sence in that case
	namespace bg = boost::geometry;

	// calculate terrain bounding box (it is axis aligned)
	// the formula is `(position + quad_size) * model_scale`
	vec2 const min_corner = trn.position * model_scale,
		max_corner = (trn.position + quad_size) * model_scale;

	geom::box2 const tile_area = {min_corner, max_corner};

	return bg::intersects(vec2{pos}, tile_area);
}


float terrain_grid::camera_ground_height = 0.0f;


vector<terrain> terrain_grid::load_level_tiles(path const & data_path, int level) const {
	// TODO: the implementation produce unordered list of terrains (which can be a performance issue during the rendering because you want to access adjacent terrains).
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
		if (file.filename().string().starts_with(_elevation_tile_prefix)) {  // TODO: can we use range algorithm there?
			// TODO: introduce elevation_file

			// - parse grid collumn (C) and row (R) position
			regex const tile_pattern{R"(.+(\d+)_(\d+)\.tif)"};  // (column), (row)
			smatch what;
			string const filename = file.filename().string();
			if (!regex_match(filename, what, tile_pattern))
				continue;  // skip file

			// - check we have coresponding rgb file
			string const column_str = what[1].str(),
				row_str = what[2].str();
			path const satellite_path = tile_directory / path{fmt::format("{}{}_{}.tif", _satellite_tile_prefix, column_str, row_str)};
			if (!exists(satellite_path)) {
				spdlog::info("corresponding satellite data for elevation tile ('{}') not found", file.c_str());
				continue;
			}
			// TODO: this is super slow implementation, we should search in a list of tile files

			// - calculate terrain word position
			int const column = stoi(column_str),
				row = stoi(row_str);

			// TODO: there we need level to proper calculate grid position
			float const level_quad_size = (2.0f*quad_size) / pow(2, level-1);  // TODO: equation works for level 2 and 3, later we neeed to agree on a leveling
			vec2 const word_pos = to_word_position(column, row, level, level_quad_size);

			// - load elevation tile
			auto const elevation_tile = create_texture_16b(file);
			assert(is_square(elevation_tile) && "we expect square elevation tiles");
			// TODO: we do noot have level information to check elevation tile size
			// assert(size_t(_elevation_tile_size) == get<1>(elevation_tile) && "unexpected elevation tile size");

			// - load satellite tile
			auto const satellite_tile = create_texture_8b(satellite_path);
			assert(is_square(satellite_tile));
			// TODO: check satellite tile size

			// - create terrain instance and filll maps and position
			terrain trn;
			trn.elevation_map = get_tid(elevation_tile);
			trn.satellite_map = get_tid(satellite_tile);
			trn.position = word_pos;
			trn.grid_c = column;
			trn.grid_r = row;

			// - calculate elevation max value
			trn.elevation_min = _elevation_tile_max_value.at(file.filename());  // TODO: can thrrow std::out_of_range

			// - add to the terrain quad tree
			terrains.push_back(trn);
		}
	}

	return terrains;
}

size_t terrain_grid::size() const {
	return _terrain_count;
}

struct terrain_finder {
	terrain_finder(int grid_c, int grid_r) : grid_c{grid_c}, grid_r{grid_r} {}

	bool operator()(terrain const & trn) const {
		return trn.grid_c == grid_c && trn.grid_r == grid_r;
	}

	int grid_c, grid_r;
};

// TODO: also change level for load_description()
void terrain_grid::load_tiles(path const & data_path) {
	// load terrains, for now let's assume level 1 and 2 only
	auto const data_l2_path = data_path/"level1";

	load_description(data_l2_path, 2);  // read dataset description file for level 2 tiles

	vector<terrain> terrains_l2 = load_level_tiles(data_l2_path, 2);

	// construct level 2 quad tree
	assert(std::size(terrains_l2) == 4 && "this sample expect 4 level 2 quadtree tiles");
	for (terrain & trn : terrains_l2) {
		int const idx = trn.grid_c + trn.grid_r * 2;
		assert(idx <= 4 && "four terrains are expected, not more");
		trn.level = 2;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->data = trn;
		_root.children[idx] = std::move(quad);
	}
	_terrain_count += std::size(terrains_l2);
	// TODO: cheeck all children are assigned (we need to do that, becaause grid_c or grid_r can goes wrong

	// TODO: before we can load next level we somehow need to deal with description data from previous level stored as _elevation_tile_size, _satellite_tile_size, ...
	_elevation_tile_max_value.clear();  // TODO: here we do not realy want to free resources there (this is sloow, we only want to set map size to 0)

	auto const data_l3_path = data_path/"level2";
	load_description(data_l3_path, 3);

	// construct level 3 quad tree
	vector<terrain> terrains_l3 = load_level_tiles(data_l3_path, 3);
	// assert(std::size(terrains_l3) == 4 && "this sample expect 4 level 3 quadtree tiles");
	auto & root = _root.children[0];
	// for (terrain & trn : terrains_l3) {
	// 	int const idx = trn.grid_c + trn.grid_r * 2;
	// 	assert(idx <= 4 && "four terrains are expected, not more");
	// 	trn.level = 3;
	// 	unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
	// 	quad->data = trn;
	// 	root->children[idx] = std::move(quad);
	// }
	// we need to assign `plzen_elev_0_0.tif`, `plzen_elev_0_1.tif`, `plzen_elev_0_2.tif`, `plzen_elev_0_3.tif`


	{
		auto trn_it = std::ranges::find_if(terrains_l3, terrain_finder{0, 0});
		assert(trn_it != end(terrains_l3) && "terrain (0,0) is missing");
		trn_it->level = 3;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->data = *trn_it;
		root->children[0] = std::move(quad);
	}

	{
		auto trn_it = std::ranges::find_if(terrains_l3, terrain_finder{1, 0});
		assert(trn_it != end(terrains_l3) && "terrain (1,0) is missing");
		trn_it->level = 3;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->data = *trn_it;
		root->children[1] = std::move(quad);
	}

	{
		auto trn_it = std::ranges::find_if(terrains_l3, terrain_finder{0, 1});
		assert(trn_it != end(terrains_l3) && "terrain (0,1) is missing");
		trn_it->level = 3;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->data = *trn_it;
		root->children[2] = std::move(quad);
	}

	{
		auto trn_it = std::ranges::find_if(terrains_l3, terrain_finder{1, 1});
		assert(trn_it != end(terrains_l3) && "terrain (1,1) is missing");
		trn_it->level = 3;
		unique_ptr<terrain_quad> quad = make_unique<terrain_quad>();
		quad->data = *trn_it;
		root->children[3] = std::move(quad);
	}
}

void terrain_grid::load_description(path const & data_path, int level) {
	boost::property_tree::ptree config;  // TODO: rename to dataset
	boost::property_tree::read_json(data_path/"dataset.json", config);

	/* TODO properties are mandatory otherwisee
	terminate called after throwing an instance of 'boost::wrapexcept<boost::property_tree::ptree_bad_path>'
		what():  No such node (elevation.tile_prefix) */

	_elevation_tile_prefix = config.get<string>("elevation.tile_prefix");
	_satellite_tile_prefix = config.get<string>("satellite.tile_prefix");

	dataset_description desc;
	desc.elevation_tile_size = config.get<int>("elevation.tile_size");
	desc.satellite_tile_size = config.get<int>("satellite.tile_size");
	desc.elevation_pixel_size = config.get<double>("elevation.pixel_size");
	_data_desc[level] = desc;

	// create list of elevation max values
	for (auto const & kv : config.get_child("files"))  // TODO: this is work for transsform
		_elevation_tile_max_value.insert(pair{path{kv.first}, kv.second.get<int>("maxval")});  // TODO: emplace?
}

terrain_grid::~terrain_grid() {
	for (terrain const & trn : leaf_view{_root}) {  // TODO: terrain is now owner of textures so it is terrain responsibility to delete textures
		glDeleteTextures(1, &trn.elevation_map);
		glDeleteTextures(1, &trn.satellite_map);
	}
}


namespace {

vec2 to_word_position(int column, int row, int level, float quad_size) {
	int const grid_size = pow(2, level-1);
	vec2 const position = vec2{column, -row} * quad_size - vec2{grid_size, -(grid_size-(2*1))} * quad_size*0.5f;
	return position;
}

}  // namespace
