#include <string>
#include <filesystem>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include "height_overlap_shader_program.hpp"
#include "io.hpp"
#include "shader.hpp"
#include "set_uniform.hpp"

using std::string;
using std::filesystem::path;
using glm::mat4,
	glm::vec2,
	glm::value_ptr;

path const VERTEX_SHADER_FILE = "height_overlap.vs",
	FRAGMENT_SHADER_FILE = "height_overlap.fs";

height_overlap_shader_program::height_overlap_shader_program() {
	string const vertex_shader = read_file(VERTEX_SHADER_FILE),
		fragment_shader = read_file(FRAGMENT_SHADER_FILE);
	_prog = get_shader_program(vertex_shader.c_str(), fragment_shader.c_str());

	_position = glGetAttribLocation(_prog, "position");
	assert(_position == 0 && "we are expecting position location ID is set to 0");

	// vertex
	_local_to_screen = glGetUniformLocation(_prog, "local_to_screen");
	_heights = glGetUniformLocation(_prog, "heights");
	_elevation_scale = glGetUniformLocation(_prog, "elevation_scale");
	_height_scale = glGetUniformLocation(_prog, "height_scale");
	_normal_tile_size = glGetUniformLocation(_prog, "normal_tile_size");

	// fragment
	_satellite_map = glGetUniformLocation(_prog, "satellite_map");
	_use_satellite_map = glGetUniformLocation(_prog, "use_satellite_map");
	_use_shading = glGetUniformLocation(_prog, "use_shading");
	_terrain_size = glGetUniformLocation(_prog, "terrain_size");
	_elevation_tile_size = glGetUniformLocation(_prog, "elevation_tile_size");

	// check uniform active
	assert(_local_to_screen != -1);
	assert(_heights != -1);
	assert(_elevation_scale != -1);
	assert(_height_scale != -1);
	assert(_satellite_map != -1);
	assert(_use_satellite_map != -1);
	assert(_use_shading != -1);
	assert(_elevation_tile_size != -1);
	assert(_normal_tile_size != -1);
}

height_overlap_shader_program::~height_overlap_shader_program() {
	glDeleteProgram(_prog);
}

void height_overlap_shader_program::use() {
	glUseProgram(_prog);
}

void height_overlap_shader_program::local_to_screen(mat4 const & T) {
	set_uniform(_local_to_screen, T);
}

void height_overlap_shader_program::heights(int texture_unit_id) {
	set_uniform(_heights, texture_unit_id);
}

void height_overlap_shader_program::elevation_scale(float scale) {
	set_uniform(_elevation_scale, scale);
}

void height_overlap_shader_program::height_scale(float scale) {
	set_uniform(_height_scale, scale);
}

GLint height_overlap_shader_program::position_location() const {
	return _position;
}

void height_overlap_shader_program::satellite_map(int texture_unit_id) {
	set_uniform(_satellite_map, texture_unit_id);
}

void height_overlap_shader_program::use_satellite_map(bool value) {
	set_uniform(_use_satellite_map, value);
}

void height_overlap_shader_program::use_shading(bool value) {
	set_uniform(_use_shading, value);
}

void height_overlap_shader_program::terrain_size(float size) {
	set_uniform(_terrain_size, size);
}

void height_overlap_shader_program::elevation_tile_size(float size) {
	set_uniform(_elevation_tile_size, size);
}

void height_overlap_shader_program::normal_tile_size(float size) {
	set_uniform(_normal_tile_size, size);
}
