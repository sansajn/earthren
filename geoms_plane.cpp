/* OpenGL ES 3.2, geometry shader usage sample.
f: show/hide face normals */
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <tuple>
#include <utility>
#include <cassert>
#include <cstddef>
#include <fmt/core.h>
#include <boost/filesystem/string_file.hpp>
#include <SDL.h>
#include <GLES3/gl32.h>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <spdlog/spdlog.h>
#include "glmprint.hpp"
#include "camera.h"
#include "color.hpp"

using std::vector, std::string, std::tuple, std::pair, std::byte;
using std::unique_ptr;
using std::filesystem::path, std::ifstream;
using std::cout, std::endl;
using fmt::print;
using std::chrono::steady_clock, std::chrono::duration_cast, 
	std::chrono::milliseconds;
using boost::filesystem::load_string_file;
using glm::mat4,
	glm::vec4, glm::vec3, glm::vec2,
	glm::mat3,
	glm::value_ptr,
	glm::perspective,
	glm::translate,
	glm::inverseTranspose,
	glm::radians;

using namespace std::string_literals;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

path const NORMAL_VERTEX_SHADER_FILE = "geoms_plane_normal.vs",
	NORMAL_GEOMETRY_SHADER_FILE = "to_line_v0.gs",
	NORMAL_FRAGMENT_SHADER_FILE = "colored.fs",
	FACEN_VERTEX_SHADER_FILE = "geoms_plane_facen.vs",
	FACEN_GEOMETRY_SHADER_FILE = "to_normal.gs",
	FACEN_FRAGMENT_SHADER_FILE = "colored.fs";

GLint get_shader_program(char const * vertex_shader_source,
	char const * fragment_shader_source, char const * geometry_shader_source = nullptr);

tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc);

/*! Creates quad mesh on GPU with a size=1.
\return (vao, vbo, vertex_count) tuple. */
tuple<GLuint, GLuint, unsigned> create_mesh(GLint vertices_loc, GLint uv_loc);

/*! Returns vector of data in a (position:3, texcoord:2) format per vertex and array of indices to form a model.
To create a OpenGL object use code
auto [vertices, indices] = make_quad(quad_w, quad_h);
// ...
glBufferData(GL_ARRAY_BUFFER, size(vertices)*sizeof(float), vertices.data(), GL_STATIC_DRAW);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, size(indices)*sizeof(unsigned), indices.data(), GL_STATIC_DRAW);
\endcoode */
pair<vector<float>, vector<unsigned>> make_quad(unsigned w, unsigned h);

constexpr float pi = glm::pi<float>();

struct input_mode {  // list of active input contol modes
	bool pan = false,
		rotate = false;
};

// TODO: rename to map_events?
struct input_events {  // TOOD: we want constructor to init all members to false
	bool zoom_in;
	bool camera_switch;
	bool info_request;  // TODO: this can't be grouped as map_events, it behaves like an event but not map_event
};

struct render_features {  // list of selected rendering features
	bool show_normals,
		show_face_normals;
};

/*! Process user input.
\returns false in case user want to quit, otherwise true. */
template <typename Camera>
bool process_user_events(Camera & cam, input_mode & mode, render_features & features,
	input_events & events);

// input handling functions
void input_render_features(SDL_Event const & event, render_features & features);
void input_control_mode(SDL_Event const & event, input_mode & mode, input_events & events);  //!< handle pan/rotate modes
void input_camera(SDL_Event const & event, map_camera & cam, input_mode const & mode,
	input_events & events);  //!< handle camera movement, rotations


int main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
	spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("OpenGL ES 3.2", SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GLContext context = SDL_GL_CreateContext(window);

	cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n" 
		<< "GL_VERSION: " << glGetString(GL_VERSION) << "\n"
		<< "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	
	// glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// load shader program for visualize normals
	string normal_vs,
		normal_gs,
		normal_fs;
	load_string_file(NORMAL_VERTEX_SHADER_FILE.c_str(), normal_vs);
	load_string_file(NORMAL_GEOMETRY_SHADER_FILE.c_str(), normal_gs);

	/* TODO: in case file do not exists it throws `std::__ios_failure` which is time consuming
	to debug (It is not clear that file do not exists), we need a wrappeer for that function to
	speedup development. */
	load_string_file(NORMAL_FRAGMENT_SHADER_FILE.c_str(), normal_fs);

	GLuint const normal_shader_program = get_shader_program(normal_vs.c_str(),
		normal_fs.c_str(), normal_gs.c_str());

	GLint const normal_position_loc = glGetAttribLocation(normal_shader_program, "position");
	GLint const normal_local_to_screen_loc = glGetUniformLocation(normal_shader_program, "local_to_screen");
	GLint const normal_line_color_loc = glGetUniformLocation(normal_shader_program, "fill_color");

	// face normals
	// load shader program for visualize normals
	string facen_vs,
		facen_gs,
		facen_fs;
	load_string_file(FACEN_VERTEX_SHADER_FILE.c_str(), facen_vs);
	load_string_file(FACEN_GEOMETRY_SHADER_FILE.c_str(), facen_gs);

	/* TODO: in case file do not exists it throws `std::__ios_failure` which is time consuming
	to debug (It is not clear that file do not exists), we need a wrappeer for that function to
	speedup development. */
	load_string_file(FACEN_FRAGMENT_SHADER_FILE.c_str(), facen_fs);

	GLuint const facen_shader_program = get_shader_program(facen_vs.c_str(),
		facen_fs.c_str(), facen_gs.c_str());

	GLint const facen_position_loc = glGetAttribLocation(facen_shader_program, "position");
	GLint const facen_local_to_screen_loc = glGetUniformLocation(facen_shader_program, "local_to_screen");
	GLint const facen_line_color_loc = glGetUniformLocation(facen_shader_program, "fill_color");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// create terrain mash
	constexpr float quad_size = 1.0f;
	auto const [vao, vbo, ibo, element_count] = create_quad_mesh(normal_position_loc);

	// camera related stuff
	map_camera cam{0.44189858f};
	cam.look_at = vec2{0, 0};

	input_mode mode;  // TODO: we want either pan or rotate not both on the same time, simplify with enum-class

	render_features features = {
		.show_normals = true,
		.show_face_normals = false
	};

	auto t_prev = steady_clock::now();

	while (true) {
		// compute dt
		auto t_now = steady_clock::now();
		float const dt = duration_cast<milliseconds>(t_now - t_prev).count() / 1000.0f;
		t_prev = t_now;

		input_events events;

		// input
		if (!process_user_events(cam, mode, features, events))
			break;  // user wants to quit

		// update
		cam.update();

		mat4 const P = perspective(radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f),
			V = cam.view();

		if (events.camera_switch)
			cout << with_label{"V", V};

		// render
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  // clear buffer

		glBindVertexArray(vao);  // VAO is independent of used program

		vec2 const model_pos = vec2{-quad_size/2.0f, -quad_size/2.0f};
		mat4 const M = translate(mat4{1}, vec3{model_pos,0});
		mat4 const local_to_screen = P*V*M;

		if (events.info_request) {
			cout << "info:\n"
				// << "model_scale=" << model_scale << '\n'
				<< "model_pos=" << model_pos << '\n'
				<< with_label{"M", M} << '\n'
				<< with_label{"V", V} << '\n'
				<< with_label{"P", P} << '\n'
				<< with_label{"local_to_screen", local_to_screen};
		}

		if (features.show_normals) { // render normals
			glUseProgram(normal_shader_program);

			glUniform3fv(normal_line_color_loc, 1, value_ptr(rgb::lime));
			glUniformMatrix4fv(normal_local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

			glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
		}

		if (features.show_face_normals) {
			glUseProgram(facen_shader_program);

			glUniform3fv(facen_line_color_loc, 1, value_ptr(rgb::blue));
			glUniformMatrix4fv(facen_local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

			glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
		}

		glBindVertexArray(0);  // unbind VAO

		SDL_GL_SwapWindow(window);
	}
	
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(normal_shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}

// handling render features
void input_render_features(SDL_Event const & event, render_features & features) {
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_n:
			features.show_normals = !features.show_normals;
			spdlog::info("show_normals={}", features.show_normals);
			break;

		case SDLK_f:
			features.show_face_normals = !features.show_face_normals;
			spdlog::info("show_faace_normals={}", features.show_face_normals);
			break;
		}
	}
}

void input_control_mode(SDL_Event const & event, input_mode & mode, input_events & events) {
	//  handle pan mode
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		mode.pan = true;
		spdlog::info("left mouse pressed");
	}

	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		mode.pan = false;
		spdlog::info("left mouse released");
	}

	// rotation mode or detail camera
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_LCTRL:  // enable camera rotation state
			mode.rotate = true;
			break;

		case SDLK_f:  // switch detail camera mode on/off
			// mode.detail_camera = !mode.detail_camera;
			// events.camera_switch = true;
			// spdlog::info("detail-camera={}", mode.detail_camera);
			break;

		// info request event
		case SDLK_i:
			events.info_request = true;
			break;
		}
	}
	else if (event.type == SDL_KEYUP) {
		switch (event.key.keysym.sym) {
		case SDLK_LCTRL:  // disable camera rotation state
			mode.rotate = false;
			break;
		}
	}
}

void input_camera(SDL_Event const & event, map_camera & cam, input_mode const & mode,
	input_events & events) {
	// distance
	if (event.type == SDL_MOUSEWHEEL) {
		assert(event.wheel.direction == SDL_MOUSEWHEEL_NORMAL);

		if (event.wheel.y > 0) {  // scroll up
			cam.distance /= 1.1;
			events.zoom_in = true;
		}
		else if (event.wheel.y < 0)  // scroll down
			cam.distance *= 1.1;

		spdlog::info("distance={}", cam.distance);
	}

	// reset
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_c:  // reset camera
			cam = map_camera{20.0f};
			cam.look_at = {0, 0};
			break;
		}
	}

	// pan and rotation
	if (mode.pan && event.type == SDL_MOUSEMOTION) {
		vec2 ds = vec2{event.motion.xrel, -event.motion.yrel};
		if (mode.rotate) {
			cam.phi -= ds.x / 500.0f;
			cam.theta += ds.y / 500.0f;
		}
		else  // pan
			cam.look_at -= ds * 0.01f;
	}
}

template <typename Camera>
bool process_user_events(Camera & cam, input_mode & mode, render_features & features,
	input_events & events) {  // TODO: rename to input()

	events = {  // resets all input events
		.zoom_in = false,
		.camera_switch = false,
		.info_request = false
	};

	SDL_Event event;
	while (SDL_PollEvent(&event)) {  // we want to fully process the event queue, update state and then render
		if (event.type == SDL_QUIT)
			return 0;

		input_control_mode(event, mode, events);
		input_camera(event, cam, mode, events);
		input_render_features(event, features);
	}  // while

	return 1;
}

// TODO: rewrite to string_view, glShaderSource call needs to be changed
GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source, char const * geometry_shader_source) {
	enum Consts {INFOLOG_LEN = 512};
	GLchar infoLog[INFOLOG_LEN];
	GLint success;

	/* Vertex shader */
	GLint vertex_shader;
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		// TODO: we do not have file names so can't provide
		glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
			  << infoLog << endl;
	}

	/* Fragment shader */
	GLint fragment_shader;
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
			  << infoLog << endl;
	}

	// geometry shader
	GLint geometry_shader = -1;
	if (geometry_shader_source) {
		geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry_shader, 1, &geometry_shader_source, nullptr);
		glCompileShader(geometry_shader);
		glGetShaderiv(geometry_shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(geometry_shader, INFOLOG_LEN, NULL, infoLog);
			cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n"
				  << infoLog << endl;
		}
	}

	/* Link shaders */
	GLint shader_program;
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	if (geometry_shader_source)
		glAttachShader(shader_program, geometry_shader);

	glLinkProgram(shader_program);
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_program, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
			  << infoLog << endl;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader_program;
}

pair<vector<float>, vector<unsigned>> make_quad(unsigned w, unsigned h) {
	assert(w > 1 && h > 1 && "invalid dimensions");

	// vertices
	float const dx = 1.0f/(w-1),
		dy = 1.0f/(h-1);
	vector<float> verts((3+2)*w*h);  // position:3, texcoord:2

	float * vdata = verts.data();
	for (unsigned j = 0; j < h; ++j) {
		float py = j*dy;
		for (unsigned i = 0; i < w; ++i) {
			float px = i*dx;
			*vdata++ = px;  // position
			*vdata++ = py;
			*vdata++ = 0;
			*vdata++ = px;  // texcoord
			*vdata++ = py;
		}
	}

	// indices
	unsigned const nindices = 2*(w-1)*(h-1)*3;
	vector<unsigned> indices(nindices);
	unsigned * idata = indices.data();
	for (unsigned j = 0; j < h-1; ++j) {
		unsigned yoffset = j*w;
		for (unsigned i = 0; i < w-1; ++i) {
			unsigned n = i + yoffset;
			*(idata++) = n;      // triangle 1
			*(idata++) = n+1;
			*(idata++) = n+1+w;
			*(idata++) = n+1+w;  // triangle 2
			*(idata++) = n+w;
			*(idata++) = n;
		}
	}

	return {verts, indices};
}

tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc) {
	constexpr unsigned quad_w = 100,
		quad_h = 100;

	auto [vertices, indices] = make_quad(quad_w, quad_h);

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;  // create vertex bufffer object
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size(vertices)*sizeof(float), vertices.data(), GL_STATIC_DRAW);

	GLuint ibo = 0;  // create index bufffer object
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size(indices)*sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

	// bind (x,y,z) data
	constexpr size_t stride = (3+2)*sizeof(float);
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
	glEnableVertexAttribArray(position_loc);

	glBindVertexArray(0);  // unbind vertex array

	return {vao, vbo, ibo, size(indices)};
}
