#include <string>
#include <filesystem>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include "four_terrain_shader_program.hpp"
#include "io.hpp"
#include "shader.hpp"
#include "set_uniform.hpp"

using std::string;
using std::filesystem::path;
using glm::mat4,
	glm::vec2,
	glm::value_ptr;

path const VERTEX_SHADER_FILE = "four_terrain.vs",
	FRAGMENT_SHADER_FILE = "satellite_map.fs";

four_terrain_shader_program::four_terrain_shader_program() {
	string const vertex_shader = read_file(VERTEX_SHADER_FILE),
		fragment_shader = read_file(FRAGMENT_SHADER_FILE);
	_prog = get_shader_program(vertex_shader.c_str(), fragment_shader.c_str());

	_position = glGetAttribLocation(_prog, "position");
	assert(_position == 0 && "we are expecting position location ID is set to 0");

	_local_to_screen = glGetUniformLocation(_prog, "local_to_screen");
	_heights = glGetUniformLocation(_prog, "heights");
	_elevation_scale = glGetUniformLocation(_prog, "elevation_scale");
	_height_scale = glGetUniformLocation(_prog, "height_scale");
	_satellite_map = glGetUniformLocation(_prog, "satellite_map");
	_height_map_size = glGetUniformLocation(_prog, "height_map_size");
	_use_satellite_map = glGetUniformLocation(_prog, "use_satellite_map");
	_use_shading = glGetUniformLocation(_prog, "use_shading");
}

four_terrain_shader_program::~four_terrain_shader_program() {
	glDeleteProgram(_prog);
}

void four_terrain_shader_program::use() {
	glUseProgram(_prog);
}

void four_terrain_shader_program::local_to_screen(mat4 const & T) {
	set_uniform(_local_to_screen, T);
}

void four_terrain_shader_program::heights(int texture_unit_id) {
	set_uniform(_heights, texture_unit_id);
}

void four_terrain_shader_program::elevation_scale(float scale) {
	set_uniform(_elevation_scale, scale);
}

void four_terrain_shader_program::height_scale(float scale) {
	set_uniform(_height_scale, scale);
}

GLint four_terrain_shader_program::position_location() const {
	return _position;
}

void four_terrain_shader_program::satellite_map(int texture_unit_id) {
	set_uniform(_satellite_map, texture_unit_id);
}

void four_terrain_shader_program::use_satellite_map(bool value) {
	set_uniform(_use_satellite_map, value);
}

void four_terrain_shader_program::use_shading(bool value) {
	set_uniform(_use_shading, value);
}

void four_terrain_shader_program::height_map_size(vec2 const & size) {
	set_uniform(_height_map_size, size);
}
