#include "grid_of_terrains_lightdir_shader_program.hpp"
#include "io.hpp"
#include "shader.hpp"
#include "set_uniform.hpp"

using std::string, std::empty;
using std::filesystem::path;

path const VERTEX_SHADER_FILE = "grid_of_terrains_lightdir.vs",
	GEOMETRY_SHADER_FILE = "to_line.gs",
	FRAGMENT_SHADER_FILE = "colored.fs";

grid_of_terrains_lightdir_shader_program::grid_of_terrains_lightdir_shader_program() {
	string const vertex_shader = read_file(VERTEX_SHADER_FILE),
		geometry_shader = read_file(GEOMETRY_SHADER_FILE),
		fragment_shader = read_file(FRAGMENT_SHADER_FILE);

	assert(!empty(vertex_shader) && !empty(geometry_shader) && !empty(fragment_shader));

	_prog = get_shader_program(vertex_shader.c_str(), fragment_shader.c_str(), geometry_shader.c_str());
	assert(_prog != 0);

	// vertex
	_position = glGetAttribLocation(_prog, "position");
	assert(_position == 0 && "we are expecting position location ID is set to 0");

	_heights = glGetUniformLocation(_prog, "heights");
	_elevation_scale = glGetUniformLocation(_prog, "elevation_scale");
	_height_scale = glGetUniformLocation(_prog, "height_scale");

	// geometry
	_local_to_screen = glGetUniformLocation(_prog, "local_to_screen");

	// fragment
	_fill_color = glGetUniformLocation(_prog, "fill_color");

	// check uniforms are active
	assert(_heights != -1);
	assert(_elevation_scale!= -1);
	assert(_height_scale!= -1);
	assert(_local_to_screen != -1);
	assert(_fill_color != -1);
}

grid_of_terrains_lightdir_shader_program::~grid_of_terrains_lightdir_shader_program() {}

void grid_of_terrains_lightdir_shader_program::use() const {
	glUseProgram(_prog);
}

void grid_of_terrains_lightdir_shader_program::local_to_screen(glm::mat4 const & T) {
	set_uniform(_local_to_screen, T);
}

void grid_of_terrains_lightdir_shader_program::elevation_map(int texture_unit_id) {
	set_uniform(_heights, texture_unit_id);
}

void grid_of_terrains_lightdir_shader_program::elevation_scale(float scale) {
	set_uniform(_elevation_scale, scale);
}

void grid_of_terrains_lightdir_shader_program::height_scale(float scale) {
	set_uniform(_height_scale, scale);
}

void grid_of_terrains_lightdir_shader_program::fill_color(glm::vec3 const & color) {
	set_uniform(_fill_color, color);
}

GLint grid_of_terrains_lightdir_shader_program::position_location() const {
	return _position;
}
