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
#include "color.hpp"
#include "free_camera.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "io.hpp"
#include "terrain_scale_ui.hpp"
#include "axes_model.hpp"
#include "quad.hpp"
#include "flat_shader.hpp"
#include "height_overlap_shader_program.hpp"
#include "above_terrain_outline_shader_program.hpp"
#include "grid_of_terrains_lightdir_shader_program.hpp"
#include "more_details_terrain_grid.hpp"
#include "terrain_camera.hpp"
#include "lod_tiles_user_input.hpp"  // usser input handling (keyboard, mouse)
#include "lod_tiles_draw_terrain.hpp"

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

constexpr unsigned DEFAULT_QUAD_RESOLOTION = 10;  // for 10x10 vertices quad

path const LIGHTDIR_VERTEX_SHADER_FILE = "height_map_lightdir.vs",
	LIGHTDIR_GEOMETRY_SHADER_FILE = "to_line.gs",
	LIGHTDIR_FRAGMENT_SHADER_FILE = "colored.fs";

path const config_file_path = "lod_tiles.ini",
	data_path = "data/gen/lod_tiles";

void verbose_signal_handler(int signal);

constexpr float pi = glm::pi<float>();

/*! Process user input.
\returns false in case user want to quit, otherwise true. */
template <typename Camera>
bool input(Camera & cam, input_mode & mode, render_features & features,
	input_events & events);

void update(free_camera & cam, input_mode const & mode, float dt);

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
	spdlog::set_level(spdlog::level::debug);

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
	
	terrain_scale_ui ui;
	ui.height_scale = TERRAIN_HEIGHT_SCALE;
	ui.quad_scale = TERRAIN_SIZE_SCALE;
	ui.quad_resolution = DEFAULT_QUAD_RESOLOTION;
	ui.init(config_file_path);
	ui.setup(window, context);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	height_overlap_shader_program shader;

	// load shader program to visualize light direction
	grid_of_terrains_lightdir_shader_program lightdir_shader;
	assert(shader.position_location() == lightdir_shader.position_location() && "we expect the same position attribute locations (=0)");

	// load shader program for wirefraame rendering
	above_terrain_outline_shader_program outline_shader;
	assert(shader.position_location() == outline_shader.position_location() && "we expect the same position attribute locations (=0)");

	string const flat_vs = read_file("flat_shader.vs"),
		flat_fs = read_file("flat_shader.fs");
	GLuint const flat_shader_program_id = get_shader_program(flat_vs.c_str(), flat_fs.c_str());

	flat_shader_program flat_shader{flat_shader_program_id};

	// load axes model
	GLuint const axes_position_vbo = push_axes();
	axes_model axes{axes_position_vbo};

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// create terrain mash
	constexpr float quad_size = 1.0f;
	auto [vao, vbo, ibo, element_count] = create_quad_mesh(shader.position_location(), ui.quad_resolution);

	// camera related stuff
	terrain_camera cam{20.0f};
	cam.look_at = vec2{0, 0};

	free_camera cam_detail{radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f};
	cam_detail.position = vec3{0, 0, 5.f};
	cam_detail.look_at(vec3{0, 0, 0});

	input_mode mode;

	render_features features = {
		.show_terrain = false,
		.show_lightdir = false,
		.show_outline = true,
		.show_satellite = true,
		.calculate_shades = true
	};

	unsigned quad_resolution = ui.quad_resolution;  // save quad resolution to detect resolution changes

	// create grid of terrains (load textures, ...)
	terrain_grid terrains;
	terrains.load_tiles(data_path);
	spdlog::info("we have {} terrains loaded", terrains.size());

	auto t_prev = steady_clock::now();

	vec3 prev_cam_pos = cam.position();  // to detect camera grid_column_countposition change between the looop iteratoins

	terrain const * camera_terrain = nullptr;

	while (true) {  // the loop
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

		ui.create();

		// detect quad resolution change
		if (quad_resolution != static_cast<unsigned>(ui.quad_resolution)) {
			assert(ui.quad_resolution > 1);
			destroy_quad_mesh(vao, vbo, ibo);
			std::tie(vao, vbo, ibo, element_count) = create_quad_mesh(shader.position_location(), ui.quad_resolution);
			quad_resolution = ui.quad_resolution;
		}

		// render
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  // clear buffer

		glBindVertexArray(vao);  // VAO is independent of used program

		float const model_scale = ui.quad_scale;

		if (events.info_request) {
			cout << "info:\n"
				<< "model_scale=" << model_scale << '\n'
				<< with_label{"V", V} << '\n'
				<< "cammera: theta=" << cam.theta << ", phi=" << cam.phi << ", distance=" << cam.distance << ", position=" << cam.position() << '\n';

			cout << "terrain_grid: ";
			for (terrain const & t : terrains.iterate())
				cout << "(" <<  t.grid_c << "," << t.grid_r << ") -> " << t.position * model_scale << " ";
			cout << "\n";
		}

		assert(size(terrains) > 0 && "we expect at least one terrain to render something");

		if (prev_cam_pos != cam.position()) {  // on camera move
			for (terrain const & trn : terrains.iterate()) {  // find terrain under camera and set ground_height
				if (is_above(trn, quad_size, model_scale, cam.position())) {
					if (&trn != camera_terrain) {  // we want to change only when we are over new terrain
						int const elevation_size = terrains.elevation_tile_size(trn.level);  //= 716px
						// TODO: we have two diferrent computatios of elevation_scale and tha is not right, unite
						float const elevation_scale = model_scale / (terrains.elevation_pixel_size(trn.level) * elevation_size);  //= 0.000107174

						terrain_grid::camera_ground_height = trn.elevation_min * elevation_scale * ui.height_scale;
						camera_terrain = &trn;  // save for later comparison
						cout << "camera-ground-height=" << terrain_grid::camera_ground_height << '\n';
					}
					break;
				}
			}
			prev_cam_pos = cam.position();
		}

		int rendered_tile_count = 0;

		for (terrain const & trn : terrains.iterate()) {  // draw terrain grid
			rendered_tile_count += 1;

			float const level_scale = 1.0f / (terrains.grid_size(trn.level) / 2.0f);  // TODO: this works only for level 2 and 3
			vec2 const model_pos = trn.position * model_scale;
			mat4 const M = scale(translate(mat4{1}, vec3{model_pos,0}), vec3{model_scale*level_scale, model_scale*level_scale, 1});  // T*S
			mat4 const local_to_screen = P*V*M;

			int const elevation_size = terrains.elevation_tile_size(trn.level);  //= 716px
			float const elevation_scale = (model_scale*level_scale) / (terrains.elevation_pixel_size(trn.level) * elevation_size);  //= 0.000107174

			if (features.show_terrain) {  // render terrain
				draw_terrain(shader, trn,
					element_count,
					local_to_screen,
					elevation_size,
					ui.height_scale, elevation_scale, features);
			}

			if (features.show_lightdir) {  // render light directions
				draw_terrain_light_directions(lightdir_shader, trn,
					element_count,
					ui.height_scale, elevation_scale, local_to_screen);
			}

			if (features.show_outline) {  // render wireframe
				draw_terrain_outlines(outline_shader, trn,
					element_count,
					rgb::blue,
					ui.height_scale, elevation_scale, local_to_screen,
					features);
			}			
		}  // for (trn ...

		glBindVertexArray(0);  // unbind VAO

		// render axis
		flat_shader.use();

		// we want to put axes to the left bottom corner in a camera space (so the position never change, just the rotation)
		mat4 const cam_rot = mat4{glm::mat3{V}};

		// T*S*R
		mat4 const M_axes = scale(translate(mat4{1}, vec3{-3.25, -2.45, -5}), vec3{0.5, 0.5, 0.5}),  // put axis into the middle
			axes_local_to_screen = P*M_axes*cam_rot;  //= P*V*V'*M_axes

		axes.draw(flat_shader, axes_local_to_screen);

		ui.render();

		SDL_GL_SwapWindow(window);
	}  // while
	
	destroy_quad_mesh(vao, vbo, ibo);

	ui.shutdown();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
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

void verbose_signal_handler(int signal) {
	cout << "signal '" << strsignal(signal) << "' (" << signal << ") caught\n"
		<< "stacktrace:\n"
		<< boost::stacktrace::stacktrace{}
		<< endl;

	exit(signal);
}
