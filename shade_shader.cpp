#include <string>
#include <filesystem>
#include "fs.hpp"
#include "set_uniform.hpp"
#include "ogl/invalid_value.hpp"
#include "shader.hpp"
#include "shade_shader.hpp"

using std::string;
using std::filesystem::path;
using glm::vec3, glm::mat4, glm::mat3;

// TODO: can we hace constexpr there?
path const VERTEX_SHADER_FILE = "shade_shader.vs",
	FRAGMENT_SHADER_FILE = "shade_shader.fs";

shade_shader_program::shade_shader_program() {
	string const vertex_shader = read_file(VERTEX_SHADER_FILE),
		fragment_shader = read_file(FRAGMENT_SHADER_FILE);
	_prog = get_shader_program(vertex_shader.c_str(), fragment_shader.c_str());

	_position = glGetAttribLocation(_prog, "position");
	assert(_position == 0 && "we are expecting position location ID is set to 0");

	_normal = glGetAttribLocation(_prog, "normal");
	assert(_normal == 1 && "we are expecting normal location ID is set to 1");

	// vertex uniforms
	_local_to_screen = glGetUniformLocation(_prog, "local_to_screen");
	_normal_to_view = glGetUniformLocation(_prog, "normal_to_view");

	// fragment uniforms
	_color = glGetUniformLocation(_prog, "color");
	_light_dir = glGetUniformLocation(_prog, "light_dir");

	// check uniforms are active
	assert(_local_to_screen != ogl::invalid_uniform_value);
	assert(_normal_to_view != ogl::invalid_uniform_value);
	assert(_color!= ogl::invalid_uniform_value);
	assert(_light_dir != ogl::invalid_uniform_value);
}

void shade_shader_program::use() const {
	glUseProgram(_prog);
}

GLint shade_shader_program::position_location() const {
	return _position;
}

void shade_shader_program::color(vec3 const & rgb) {
	set_uniform(_color, rgb);
}

void shade_shader_program::light_direction(vec3 const & d) {
	// TODO: assert if not normalized
	set_uniform(_light_dir, d);
}

void shade_shader_program::local_to_screen(mat4 const & T) {
	set_uniform(_local_to_screen, T);
}

void shade_shader_program::normal_to_view(mat3 const & T) {
	set_uniform(_normal_to_view, T);
}

shade_shader_program::~shade_shader_program() {
	glDeleteProgram(_prog);
}
