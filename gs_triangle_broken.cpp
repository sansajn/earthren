/* OpenGL ES 3.2, geometry shader usaaage sample. */
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
#include "camera.hpp"
#include "color.hpp"
#include "free_camera.hpp"

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

path const NORMAL_VERTEX_SHADER_FILE = "gs_triangle_broken.vs",
	NORMAL_GEOMETRY_SHADER_FILE = "gs_triangle_broken.gs",
	NORMAL_FRAGMENT_SHADER_FILE = "colored.fs";

GLint get_shader_program(char const * vertex_shader_source,
	char const * fragment_shader_source, char const * geometry_shader_source = nullptr);

tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc);

/*! Creates quad mesh on GPU with a size=1.
\return (vao, vbo, vertex_count) tuple. */
tuple<GLuint, GLuint, unsigned> create_mesh(GLint vertices_loc, GLint uv_loc);

constexpr float pi = glm::pi<float>();

struct input_mode {  // list of active input contol modes
	bool pan = false,
		rotate = false;
	bool detail_camera = false;

	// in case of detail camera used (free_camera)
	bool move_forward = false,  // w
		move_backkward = false,  // s
		move_left = false,  // a
		move_right = false;  // d
};

// TODO: rename to map_events?
struct input_events {  // TOOD: we want constructor to init all members to false
	bool zoom_in;
	bool camera_switch;
	bool info_request;  // TODO: this can't be grouped as map_events, it behaves like an event but not map_event
};

struct render_features {  // list of selected rendering features
	bool show_terrain,
		show_normals,
		show_lightdir,
		show_outline;
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
void input_camera(SDL_Event const & event, free_camera & cam, input_mode const & mode,
	input_events & events);  //!< handle camera movement, rotations

// TODO: maybe we want to also apply mouse movement there (based on app physics)
void update(free_camera & cam, input_mode const & mode, float dt);


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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// create terrain mash
	constexpr float quad_size = 1.0f;
	auto const [vao, vbo, ibo, element_count] = create_quad_mesh(normal_position_loc);

	// camera related stuff
	map_camera cam{0.44189858f};
	cam.look_at = vec2{0, 0};

	free_camera cam_detail{radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f};
	cam_detail.position = vec3{0, 0, 5.f};
	cam_detail.look_at(vec3{0, 0, 0});

	input_mode mode;  // TODO: we want either pan or rotate not both on the same time, simplify with enum-class

	render_features features = {
		.show_terrain = true,
		.show_normals = false,
		.show_lightdir = false,
		.show_outline = false
	};

	auto t_prev = steady_clock::now();

	while (true) {
		// compute dt
		auto t_now = steady_clock::now();
		float const dt = duration_cast<milliseconds>(t_now - t_prev).count() / 1000.0f;
		t_prev = t_now;

		input_events events;

		mat4 P, V;
		if (!mode.detail_camera) {
			// input
			if (!process_user_events(cam, mode, features, events))
				break;  // user wants to quit

			// update
			cam.update();
			P = perspective(radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f);
			V = cam.view();
		}
		else {  // free_camera
			// input
			if (!process_user_events(cam_detail, mode, features, events))
				break;  // user wants to quit

			// update
			update(cam_detail, mode, dt);
			P = cam_detail.projection();
			V = cam_detail.view();
		}

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

		// render normals
		glUseProgram(normal_shader_program);

		glUniform3fv(normal_line_color_loc, 1, value_ptr(rgb::lime));
		glUniformMatrix4fv(normal_local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

		glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);

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
		case SDLK_l:
			features.show_lightdir = !features.show_lightdir;
			spdlog::info("show_lightdir={}", features.show_lightdir);
			break;
		case SDLK_o:
			features.show_outline = !features.show_outline;
			spdlog::info("show_outline={}", features.show_outline);
			break;
		case SDLK_t:
			features.show_terrain = !features.show_terrain;
			spdlog::info("show_terrain={}", features.show_terrain);
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
			mode.detail_camera = !mode.detail_camera;
			events.camera_switch = true;
			spdlog::info("detail-camera={}", mode.detail_camera);
			break;

		// detail camera input modes
		case SDLK_w:
			mode.move_forward = true;
			break;

		case SDLK_s:
			mode.move_backkward = true;
			break;

		case SDLK_a:
			mode.move_left = true;
			break;

		case SDLK_d:
			mode.move_right = true;
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

		// detail camera input modes
		case SDLK_w:
			mode.move_forward = false;
			break;

		case SDLK_s:
			mode.move_backkward = false;
			break;

		case SDLK_a:
			mode.move_left = false;
			break;

		case SDLK_d:
			mode.move_right = false;
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

void input_camera(SDL_Event const & event, free_camera & cam, input_mode const & mode,
	input_events & events) {

	constexpr float dt = 0.1f;  // TODO: this should be function argument
	constexpr float angular_speed = 1.0f/100.0f;  // rad/s

	// handle mouse movement when ctrl is pressed to rotate camera
	if (mode.pan && event.type == SDL_MOUSEMOTION) {
		vec2 ds = vec2{event.motion.xrel, -event.motion.yrel};
		if (mode.rotate) {
			vec3 const up = vec3{0,0,1};

			if (ds.x != 0.0f) {
				float const angle = ds.x * angular_speed * dt;
				cam.rotation = normalize(angleAxis(-angle, up) * cam.rotation);
			}
			if (ds.y != 0.0f) {  // TODO: we do not want to rotate above y
				float angle = ds.y * angular_speed * dt;
				cam.rotation = normalize(angleAxis(-angle, cam.right()) * cam.rotation);
			}
		}

		// note: pan is not supported for free_camera
	}

	// handle reset
	// note: we do not want to handle mouse wheel
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

void update(free_camera & cam, input_mode const & mode, float dt) {
	constexpr float speed = 1.0f;  // TODO: not velocity?

	// update cam position based on mode
	if (mode.move_forward)
		cam.position -= cam.forward() * speed * dt;

	if (mode.move_backkward)
		cam.position += cam.forward() * speed * dt;

	if (mode.move_left)
		cam.position -= cam.right() * speed * dt;

	if (mode.move_right)
		cam.position += cam.right() * speed * dt;

	cam.update();  // update camera
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

tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc) {
	vector<float> const vertices = {  // simple triangle
		0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,  1.0f, 1.0f
	};

	vector<unsigned> const indices = {
		0, 1, 2
	};

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
