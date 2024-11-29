/* OpenGL ES 3.2, terrain with heights from height map texture and proper scaling.
Usage: terrain_quad [DEM_FILE]
o: show/hide outline
c: reset view
p: set camera to predefined position (so we can compare render result)
t: show/hide terain render (to more focus on outline or normals)
s: show/hide satellite texture
a: toggle shading calculations
l: show hide light direction
f: map/free camera switch, move camera with "wsad" keys
	w: go forward
	s: go backward
	a: go left
	d: go right
i: print transformations info */
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
#include <csignal>
#include <fmt/core.h>  // replacement for std::format
#include <boost/stacktrace.hpp>
#include <string.h>  // for posix strsignal()
#include <SDL.h>
#include <GLES3/gl32.h>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <spdlog/spdlog.h>
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_sdl.h"
#include "geometry/glmprint.hpp"
#include "camera.hpp"
#include "color.hpp"
#include "free_camera.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "io.hpp"
#include "flat_shader.hpp"
#include "four_terrain_ui.hpp"
#include "axes_model.hpp"
#include "quad.hpp"
#include "height_overlap_shader_program.hpp"

using std::vector, std::string, std::pair, std::byte, std::size;
using std::tuple, std::get;
using std::unique_ptr;
using std::filesystem::path, std::filesystem::exists;
using std::ifstream;
using std::cout, std::endl;
using fmt::print;
using std::chrono::steady_clock, std::chrono::duration_cast, 
	std::chrono::milliseconds;
using glm::mat4, glm::mat3,
	glm::vec4, glm::vec3, glm::vec2,
	glm::value_ptr,
	glm::perspective,
	glm::translate,
	glm::inverseTranspose,
	glm::radians;

using namespace std::string_literals;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

constexpr float TERRAIN_SIZE_SCALE = 2.0f;
constexpr float TERRAIN_HEIGHT_SCALE = 10.0f;
constexpr float elevation_pixel_size = 26.063200588611451;  // this is tile dependent, use gdalinfo to figure it out

path const LIGHTDIR_VERTEX_SHADER_FILE = "height_map_lightdir.vs",
	LIGHTDIR_GEOMETRY_SHADER_FILE = "to_line.gs",
	LIGHTDIR_FRAGMENT_SHADER_FILE = "colored.fs",
	OUTLINE_VERTEX_SHADER_FILE = "height_overlap_outline.vs",
	OUTLINE_GEOMETRY_SHADER_FILE = "to_outline.gs",
	OUTLINE_FRAGMENT_SHADER_FILE = "colored.fs";

path const config_file_path = "height_overlap.ini";

path const data_path = "data/gen/height_overlap";
constexpr string elevation_tile_prefix = "plzen_elev_",
	satellite_tile_prefix = "plzen_rgb_";

void verbose_signal_handler(int signal) {
	cout << "signal '" << strsignal(signal) << "' (" << signal << ") caught\n"
		<< "stacktrace:\n"
		<< boost::stacktrace::stacktrace{} 
		<< endl;

	exit(signal);
}

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

struct input_events {
	bool zoom_in;
	bool camera_switch;
	bool info_request;

	input_events() : zoom_in{false}, camera_switch{false}, info_request{false} {}

	void reset() {
		zoom_in = camera_switch = info_request = false;
	}
};

struct render_features {  // list of selected rendering features
	bool show_terrain,
		show_lightdir,
		show_outline,
		show_satellite,
		calculate_shades;
};

/*! Process user input.
\returns false in case user want to quit, otherwise true. */
template <typename Camera>
bool input(Camera & cam, input_mode & mode, render_features & features,
	input_events & events);

// input handling functions
void input_render_features(SDL_Event const & event, render_features & features);
void input_control_mode(SDL_Event const & event, input_mode & mode, input_events & events);  //!< handle pan/rotate modes
void input_camera(SDL_Event const & event, map_camera & cam, input_mode const & mode,
	input_events & events);  //!< handle camera movement, rotations
void input_camera(SDL_Event const & event, free_camera & cam, input_mode const & mode,
	input_events & events);  //!< handle camera movement, rotations

void update(free_camera & cam, input_mode const & mode, float dt);

// Draw helpers

//! Draws terrain quad with elevations and sattelite texture.
void draw_terrain(height_overlap_shader_program & shader,
	unsigned int element_count,  // number of quad mesh triengle elements to draw
	mat4 const & local_to_screen,
	GLuint elevation_map, size_t elevation_width, size_t elevation_height,
	GLuint satellite_map,
	float height_scale, float elevation_scale,
	render_features const & features);

//! Draws terrain quad as mesh.
void draw_terrain_outlines(GLint shader,
	unsigned int element_count,  // number of quad mesh triengle elements to draw
	vec3 color, GLint line_color_loc,
	GLint heights_loc, GLuint height_map,
	float height_scale, GLint height_scale_loc,
	float elevation_scale, float elevation_scale_loc,
	mat4 local_to_screen, GLint local_to_screen_loc,
	render_features const & features);


bool is_square(tuple<GLuint, size_t, size_t> const & tile) {
	return get<1>(tile) == get<2>(tile);
}

/*! \returns list of (TID, width, height) tripet for for each terrain tile. */
vector<tuple<GLuint, size_t, size_t>> read_tiles(size_t grid_rows, size_t grid_cols);

// three lines
constexpr float axis_verts[] = {
	0,0,0, 1,0,0,  // x
	0,0,0, 0,1,0,  // y
	0,0,0, 0,0,1  // z
};

GLuint push_data(void const * data, size_t size_in_bytes) {
	// the implementation is not reusable, because we are creating a buffer and also unbins buffer after (this can be slow fo more bufffers).
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size_in_bytes, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
	return vbo;
}

GLuint push_axes() {
	return push_data(axis_verts, sizeof(axis_verts));
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
	signal(SIGSEGV, verbose_signal_handler);
	spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");

	// process arguments
	string const title = string{path{argv[0]}.stem()} + " (OpenGL ES 3.2)"s;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window * window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GLContext context = SDL_GL_CreateContext(window);

	cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n" 
		<< "GL_VERSION: " << glGetString(GL_VERSION) << "\n"
		<< "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	
	four_terrain_ui ui;
	ui.height_scale = TERRAIN_HEIGHT_SCALE;
	ui.init(config_file_path);
	ui.setup(window, context);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	height_overlap_shader_program shader;

	// load shader program to visualize light direction
	string const lightdir_vs = read_file(LIGHTDIR_VERTEX_SHADER_FILE),
		lightdir_gs = read_file(LIGHTDIR_GEOMETRY_SHADER_FILE),
		lightdir_fs = read_file(LIGHTDIR_FRAGMENT_SHADER_FILE);

	GLint const lightdir_shader_program = get_shader_program(lightdir_vs.c_str(), lightdir_fs.c_str(), lightdir_gs.c_str());
	assert(shader.position_location() == glGetAttribLocation(lightdir_shader_program, "position"));

	GLint const lightdir_heights_loc = glGetUniformLocation(lightdir_shader_program, "heights");
	GLint const lightdir_height_map_size_loc = glGetUniformLocation(lightdir_shader_program, "height_map_size");
	GLint const lightdir_height_scale_loc = glGetUniformLocation(lightdir_shader_program, "height_scale");
	GLint const lightdir_local_to_screen_loc = glGetUniformLocation(lightdir_shader_program, "local_to_screen");
	GLint const lightdir_line_color_loc = glGetUniformLocation(lightdir_shader_program, "fill_color");

	// load shader program for triangle outline
	string const outline_vs = read_file(OUTLINE_VERTEX_SHADER_FILE),
		outline_gs = read_file(OUTLINE_GEOMETRY_SHADER_FILE),
		outline_fs = read_file(OUTLINE_FRAGMENT_SHADER_FILE);

	GLint const outline_shader_program = get_shader_program(outline_vs.c_str(), outline_fs.c_str(), outline_gs.c_str());
	assert(shader.position_location() == glGetAttribLocation(outline_shader_program, "position"));

	GLint const outline_heights_loc = glGetUniformLocation(outline_shader_program, "heights");
	GLint const outline_elevation_scale_loc = glGetUniformLocation(outline_shader_program, "elevation_scale");
	GLint const outline_height_scale_loc = glGetUniformLocation(outline_shader_program, "height_scale");
	GLint const outline_local_to_screen_loc = glGetUniformLocation(outline_shader_program, "local_to_screen");
	GLint const outline_line_color_loc = glGetUniformLocation(outline_shader_program, "fill_color");

	string const flat_vs = read_file("flat_shader.vs"),
		flat_fs = read_file("flat_shader.fs");
	GLuint const flat_shader_program_id = get_shader_program(flat_vs.c_str(), flat_fs.c_str());

	flat_shader_program flat_shader{flat_shader_program_id};

	// load axes model
	GLuint const axes_position_vbo = push_axes();
	axes_model axes{axes_position_vbo};

	// load tiles
	constexpr size_t grid_cols = 2;
	vector<tuple<GLuint, size_t, size_t>> tiles = read_tiles(1, grid_cols);  // list of [elevation, satelite, ...] tiles for each terrain

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// create terrain mash
	constexpr float quad_size = 1.0f;
	auto const [vao, vbo, ibo, element_count] = create_quad_mesh(shader.position_location());

	// camera related stuff
	map_camera cam{20.0f};
	cam.look_at = vec2{0, 0};

	free_camera cam_detail{radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f};
	cam_detail.position = vec3{0, 0, 5.f};
	cam_detail.look_at(vec3{0, 0, 0});

	input_mode mode;

	render_features features = {
		.show_terrain = true,
		.show_lightdir = false,
		.show_outline = false,
		.show_satellite = true,
		.calculate_shades = true
	};

	float const model_scale = TERRAIN_SIZE_SCALE;

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
			if (!input(cam, mode, features, events))
				break;  // user wants to quit

			// update
			cam.update();
			P = perspective(radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f);
			V = cam.view();
		}
		else {  // free_camera
			// input
			if (!input(cam_detail, mode, features, events))
				break;  // user wants to quit

			// update
			update(cam_detail, mode, dt);
			P = cam_detail.projection();
			V = cam_detail.view();
		}

		if (events.camera_switch)
			cout << with_label{"V", V};

		ui.create();  // do we need to do this every frame?

		// render
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  // clear buffer

		glBindVertexArray(vao);  // VAO is independent of used program

		if (events.info_request) {
			cout << "info:\n"
				<< "model_scale=" << model_scale << '\n'
				<< with_label{"V", V} << '\n'
				<< "cammera: theta=" << cam.theta << ", phi=" << cam.phi << ", distance=" << cam.distance << '\n';
		}

		assert(size(tiles) > 2 && "we expect at least one elevation and one satellite tiles");

		auto const & first_elevation_tile = *begin(tiles);
		size_t const texture_width = get<1>(first_elevation_tile);
		size_t const texture_height = get<2>(first_elevation_tile);
		float const elevation_scale = model_scale / (elevation_pixel_size * texture_width);

		// draw tiles
		for (int col = 0; col < static_cast<int>(grid_cols); ++col) {
			size_t const tile_idx = 2 * col;
			GLuint const height_map = get<0>(tiles[tile_idx]),
				satellite_map = get<0>(tiles[tile_idx+1]);

			vec2 model_pos = vec2{col, 0} * quad_size * model_scale - vec2{grid_cols, 0} * quad_size*model_scale*0.5f;
			mat4 const M = scale(translate(mat4{1}, vec3{model_pos,0}), vec3{model_scale, model_scale, 1});  // T*S
			mat4 const local_to_screen = P*V*M;

			if (features.show_terrain) {  // render terrain
				draw_terrain(shader,
					element_count,
					local_to_screen,
					height_map, texture_width, texture_height,
					satellite_map,
					ui.height_scale, elevation_scale, features);
			}

			// render light directions
			if (features.show_lightdir) {
				glUseProgram(lightdir_shader_program);

				glUniform3fv(lightdir_line_color_loc, 1, value_ptr(rgb::yellow));

				// bind height map
				glUniform1i(lightdir_heights_loc, 0);  // set sampler s to use texture unit 0
				glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
				glBindTexture(GL_TEXTURE_2D, height_map);  // bind a texture to active texture unit (0)

				glUniform2f(lightdir_height_map_size_loc, texture_width, texture_height);
				glUniform1f(lightdir_height_scale_loc, ui.height_scale);
				glUniformMatrix4fv(lightdir_local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

				glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
			}

			if (features.show_outline) {  // render wireframe
				draw_terrain_outlines(outline_shader_program,
					element_count,
					rgb::blue, outline_line_color_loc,
					outline_heights_loc, height_map,
					ui.height_scale, outline_height_scale_loc,
					elevation_scale, outline_elevation_scale_loc,
					local_to_screen, outline_local_to_screen_loc,
					features);
			}
		}  // for (col ...

		glBindVertexArray(0);  // unbind VAO

		flat_shader.use();  // render axis there

		// we want to put axes to the left bottom corner in a camera space (so the position never change, just the rotation)
		mat4 const cam_rot = mat4{glm::mat3{V}};

		// T*S*R
		mat4 const M_axes = scale(translate(mat4{1}, vec3{-3.25,-2.45,-5}), vec3{0.5, 0.5, 0.5}),  // put axis into the middle
			axes_local_to_screen = P*M_axes*cam_rot;  //=P*V*V'*M_axes

		axes.draw(flat_shader, axes_local_to_screen);

		ui.render();

		SDL_GL_SwapWindow(window);
	}
	
	for(auto tile : tiles)  // delete textures
		glDeleteTextures(1, &get<0>(tile));

	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	ui.shutdown();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}

// TODO: this is ugly solution, too many arguments there
void draw_terrain(height_overlap_shader_program & shader,
	unsigned int element_count,  // number of quad mesh triengle elements to draw
	mat4 const & local_to_screen,
	GLuint elevation_map, size_t elevation_width, size_t elevation_height,  // TODO: we only need size not w and h
	GLuint satellite_map,
	float height_scale, float elevation_scale,
	render_features const & features) {

	shader.use();

	// bind height map texture
	shader.heights(0);  // set height map sampler to use texture unit 0
	glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
	glBindTexture(GL_TEXTURE_2D, elevation_map);  // bind a height texture to active texture unit (0)

	if (features.show_satellite) {
		shader.use_satellite_map(true);
		shader.satellite_map(1);  // set satellite map sampler to use texture unit 1
		glActiveTexture(GL_TEXTURE1);  // activate texture unit 1
		glBindTexture(GL_TEXTURE_2D, satellite_map);  // bind a satellite texture to active texture unit (1)
	}
	else
		shader.use_satellite_map(false);

	if (features.calculate_shades)
		shader.use_shading(true);
	else
		shader.use_shading(false);

	constexpr float elevation_tile_pixel_size = 26.063200588611451;  // see gdalinfo

	assert(elevation_width == elevation_height && "we expect square elevation tiles");
	shader.terrain_size(elevation_width * elevation_tile_pixel_size);
	shader.elevation_tile_size(elevation_width);
	shader.normal_tile_size(elevation_width - 4);  // 2px border
	shader.height_scale(height_scale);
	shader.elevation_scale(elevation_scale);
	shader.local_to_screen(local_to_screen);

	glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
}

void draw_terrain_outlines(GLint shader,
	unsigned int element_count,  // number of quad mesh triengle elements to draw
	vec3 color, GLint line_color_loc,
	GLint heights_loc, GLuint height_map,
	float height_scale, GLint height_scale_loc,
	float elevation_scale, float elevation_scale_loc,
	mat4 local_to_screen, GLint local_to_screen_loc,
	render_features const & features) {

	// render triangle outlines
	if (features.show_outline) {
		glUseProgram(shader);

		glUniform3fv(line_color_loc, 1, value_ptr(color));

		// bind height map
		glUniform1i(heights_loc, 0);  // set sampler s to use texture unit 0
		glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
		glBindTexture(GL_TEXTURE_2D, height_map);  // bind a texture to active texture unit (0)

		glUniform1f(elevation_scale_loc, elevation_scale);
		glUniform1f(height_scale_loc, height_scale);
		glUniformMatrix4fv(local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

		glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
	}
}

// handling render features
void input_render_features(SDL_Event const & event, render_features & features) {
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
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
		case SDLK_s:
			features.show_satellite = !features.show_satellite;
			spdlog::info("show_satellite={}", features.show_satellite);
			break;
		case SDLK_a:
			features.calculate_shades = !features.calculate_shades;
			spdlog::info("calculate_shadess={}", features.calculate_shades);
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
		case SDLK_p:  // set camera to predefined position
			cam = map_camera{2.97287f};
			cam.look_at = {0, 0};
			cam.theta = 0.599999f;
			cam.phi = 0.62;
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
	[[maybe_unused]] input_events & events) {

	constexpr float dt = 0.1f;
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
bool input(Camera & cam, input_mode & mode, render_features & features,
	input_events & events) {

	events.reset();

	SDL_Event event;
	while (SDL_PollEvent(&event)) {  // we want to fully process the event queue, update state and then render
		if (event.type == SDL_QUIT)
			return 0;

		// handle imgui events
		ImGui_ImplSDL2_ProcessEvent(&event);

		// Check if ImGui wants to capture the mouse or keyboard input
		bool const imgui_mouse_captured = ImGui::GetIO().WantCaptureMouse;
		bool const imgui_keyboard_captured = ImGui::GetIO().WantCaptureKeyboard;

		if (imgui_mouse_captured || imgui_keyboard_captured)
			continue; // If ImGui wants to capture the event, skip further processing


		input_control_mode(event, mode, events);
		input_camera(event, cam, mode, events);
		input_render_features(event, features);
	}  // while

	return 1;
}

void update(free_camera & cam, input_mode const & mode, float dt) {
	constexpr float speed = 1.0f;

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

vector<tuple<GLuint, size_t, size_t>> read_tiles(size_t grid_rows, size_t grid_cols) {
	vector<tuple<GLuint, size_t, size_t>> tiles;  // list of [elevation, satelite, ...] tiles for each terrain
	for (size_t row = 0; row < grid_rows; ++row) {
		for (size_t col = 0; col < grid_cols; ++col) {
			string const height_tile_name = fmt::format("{}/{}{}_{}.tif", data_path.c_str(), elevation_tile_prefix, col, row);
			assert(exists(height_tile_name) && "elevation tile not found");

			auto const height_tile = create_texture_16b(height_tile_name);
			assert(is_square(height_tile));
			tiles.push_back(height_tile);

			string const satellite_tile_name = fmt::format("{}/{}{}_{}.tif", data_path.c_str(), satellite_tile_prefix, col, row);
			assert(exists(satellite_tile_name) && "satellite tile not found");

			auto const satellite_tile = create_texture_8b(satellite_tile_name);
			assert(is_square(satellite_tile));
			tiles.push_back(satellite_tile);
		}
	}
	return tiles;
}
