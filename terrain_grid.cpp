#include <filesystem>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <spdlog/spdlog.h>
#include "geometry/glmprint.hpp"
#include "texture.hpp"
#include "terrain_grid.hpp"

// to implement is_above()
#include <boost/geometry/algorithms/intersects.hpp>
#include "geometry/box2.hpp"

// to load dataset description file
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using std::map;
using std::string, std::to_string;
using std::filesystem::path;
using std::pair;
using std::tuple, std::get;
using glm::vec2, glm::vec3;

namespace {  //!< Helper functions.

//! Helper function to calculate word position from grid (coumn, row) position.
vec2 to_word_position(int column, int row, int grid_size, float quad_size);

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

void terrain_grid::load_tiles(path const & data_path) {

	// TODO: the implementation produce unordered list of terrains (which can be a performance issue during the rendering because you want to access adjacent terrains).
	using std::filesystem::directory_iterator;
	using std::regex, std::smatch, std::regex_match;

	path const tile_directory = data_path;
	if (!exists(tile_directory)) {
		spdlog::error("tile directory '{}' does not exists", tile_directory.c_str());
		return;
	}

	load_description(data_path);  // read dataset description file first

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
			vec2 const word_pos = to_word_position(column, row, _grid_size, quad_size);

			// - load elevation tile
			auto const elevation_tile = create_texture_16b(file);
			assert(is_square(elevation_tile) && "we expect square elevation tiles");
			assert(_elevation_tile_size == get<1>(elevation_tile) && "unexpected elevation tile size");

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

			// - add to the list of terrains
			_terrains.push_back(trn);
		}
	}
}

void terrain_grid::load_description(path const & data_path) {
	boost::property_tree::ptree config;  // TODO: rename to dataset
	boost::property_tree::read_json(data_path/"dataset.json", config);

	/* TODO properties are mandatory otherwisee
	terminate called after throwing an instance of 'boost::wrapexcept<boost::property_tree::ptree_bad_path>'
		what():  No such node (elevation.tile_prefix) */

	_grid_size = config.get<int>("grid_size");
	_elevation_tile_prefix = config.get<string>("elevation.tile_prefix");
	_elevation_pixel_size = config.get<double>("elevation.pixel_size");
	_elevation_tile_size = config.get<int>("elevation.tile_size");
	_satellite_tile_prefix = config.get<string>("satellite.tile_prefix");
	_satellite_tile_size = config.get<int>("satellite.tile_size");

	// create list of elevation max values
	for (auto const & kv : config.get_child("files"))  // TODO: this is work for transsform
		_elevation_tile_max_value.insert(pair{path{kv.first}, kv.second.get<int>("maxval")});  // TODO: emplace?
}


namespace {

vec2 to_word_position(int column, int row, int grid_size, float quad_size) {
	vec2 const position = vec2{column, -row} * quad_size - vec2{grid_size, -(grid_size-(2*1))} * quad_size*0.5f;
	return position;
}

}  // namespace
