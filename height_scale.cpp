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
#include <fmt/core.h>
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
#include "imgui/examples/imgui_impl_opengl3.h"
#include "geometry/glmprint.hpp"
#include "camera.hpp"
#include "color.hpp"
#include "free_camera.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "io.hpp"

using std::vector, std::string, std::tuple, std::pair, std::byte;
using std::unique_ptr;
using std::filesystem::path, std::ifstream;
using std::cout, std::endl;
using fmt::print;
using std::chrono::steady_clock, std::chrono::duration_cast, 
	std::chrono::milliseconds;
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

constexpr float TERRAIN_SIZE_SCALE = 2.0f;
constexpr float TERRAIN_HEIGHT_SCALE = 10.0f;
constexpr float elevation_pixel_size = 26.063200588611451;  // this is tile dependent, use gdalinfo to figure it out

path const VERTEX_SHADER_FILE = "terrain_quad.vs",
	FRAGMENT_SHADER_FILE = "satellite_map.fs",
	LIGHTDIR_VERTEX_SHADER_FILE = "height_map_lightdir.vs",
	LIGHTDIR_GEOMETRY_SHADER_FILE = "to_line.gs",
	LIGHTDIR_FRAGMENT_SHADER_FILE = "colored.fs",
	OUTLINE_VERTEX_SHADER_FILE = "height_map_outline.vs",
	OUTLINE_GEOMETRY_SHADER_FILE = "to_outline.gs",
	OUTLINE_FRAGMENT_SHADER_FILE = "colored.fs";
path const HEIGHT_MAP_TEXTURE = "data/tile_1_1.tif";  // 16bit GRAY bitmap
path const SATELLITE_MAP_TEXTURE = "data/tile_1_1_rgb.tif";  // 8bit RGB bitmap

constexpr char const * config_file_name = "height_scale.ini";

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

// TODO: rename to map_events?
struct input_events {  // TOOD: we want constructor to init all members to false
	bool zoom_in;
	bool camera_switch;
	bool info_request;  // TODO: this can't be grouped as map_events, it behaves like an event but not map_event
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
	signal(SIGSEGV, verbose_signal_handler);
	spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");

	// process arguments
	path const height_map_path = (argc > 1) ? path{argv[1]} : HEIGHT_MAP_TEXTURE;

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
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	[[maybe_unused]] ImGuiIO & io = ImGui::GetIO();
	io.IniFilename = config_file_name;
	// we can configure ImGuiIO via io variable ...

	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, &context);
	ImGui_ImplOpenGL3_Init();

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// load shader program for terrain rendering
	string const vertex_shader = read_file(VERTEX_SHADER_FILE),
		fragment_shader = read_file(FRAGMENT_SHADER_FILE);

	GLuint const shader_program = get_shader_program(vertex_shader.c_str(), fragment_shader.c_str());
	GLint const position_loc = glGetAttribLocation(shader_program, "position");
	GLint const local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen");
	GLint const heights_loc = glGetUniformLocation(shader_program, "heights");
	GLint const satellite_map_loc = glGetUniformLocation(shader_program, "satellite_map");
	GLint const height_map_size_loc = glGetUniformLocation(shader_program, "height_map_size");
	GLint const height_scale_loc = glGetUniformLocation(shader_program, "height_scale");
	GLint const eleveation_scale_loc = glGetUniformLocation(shader_program, "elevation_scale");
	GLint const use_satellite_map_loc = glGetUniformLocation(shader_program, "use_satellite_map");
	GLint const use_shading_loc = glGetUniformLocation(shader_program, "use_shading");

	// load shader program to visualize light direction
	string const lightdir_vs = read_file(LIGHTDIR_VERTEX_SHADER_FILE),
		lightdir_gs = read_file(LIGHTDIR_GEOMETRY_SHADER_FILE),
		lightdir_fs = read_file(LIGHTDIR_FRAGMENT_SHADER_FILE);

	GLint const lightdir_shader_program = get_shader_program(lightdir_vs.c_str(), lightdir_fs.c_str(), lightdir_gs.c_str());

	// TODO: position unused, should I use different VAO in case of new shader program?
	GLint const lightdir_position_loc = glGetAttribLocation(lightdir_shader_program, "position");
	assert(position_loc == lightdir_position_loc);  // TODO: this is temporary

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

	// TODO: position unused, should I use different VAO in case of new shader program?
	GLint const outline_position_loc = glGetAttribLocation(outline_shader_program, "position");
	assert(position_loc == outline_position_loc);  // TODO: this is temporary

	GLint const outline_heights_loc = glGetUniformLocation(outline_shader_program, "heights");
	GLint const outline_height_map_size_loc = glGetUniformLocation(outline_shader_program, "height_map_size");
	GLint const outline_height_scale_loc = glGetUniformLocation(outline_shader_program, "height_scale");
	GLint const outline_local_to_screen_loc = glGetUniformLocation(outline_shader_program, "local_to_screen");
	GLint const outline_line_color_loc = glGetUniformLocation(outline_shader_program, "fill_color");

	// load texture
	// vector<GLuint> tiles = read_tiles();  // TODO: we only need first tile
	auto const [height_map, texture_width, texture_height] = create_texture_16b(height_map_path);
	assert(texture_width == texture_height);  // we are expecting square elevation tiles

	auto const [satellite_map, satellite_width, satellite_height] = create_texture_8b(SATELLITE_MAP_TEXTURE);
	assert(satellite_width == satellite_height);  // we are expecting square elevation tiles

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// create terrain mash
	constexpr float quad_size = 1.0f;
	auto const [vao, vbo, ibo, element_count] = create_quad_mesh(position_loc);

	// camera related stuff
	map_camera cam{20.0f};
	cam.look_at = vec2{0, 0};

	free_camera cam_detail{radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f};
	cam_detail.position = vec3{0, 0, 5.f};
	cam_detail.look_at(vec3{0, 0, 0});

	input_mode mode;  // TODO: we want either pan or rotate not both on the same time, simplify with enum-class

	render_features features = {
		.show_terrain = true,
		.show_lightdir = false,
		.show_outline = false,
		.show_satellite = true,
		.calculate_shades = true
	};

	float height_scale = TERRAIN_HEIGHT_SCALE;

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

		// TODO: we want GUI stuff to remove out of the loop

		// create gui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		ImGui::Begin("Options");  // begin window

		ImGui::DragFloat("Height Scale", &height_scale, 0.1f, 1.0f, 20.0f);

		ImGui::End();  // end window

		ImGui::SetWindowFocus(nullptr);

		// render
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  // clear buffer

		glBindVertexArray(vao);  // VAO is independent of used program

		float const model_scale = TERRAIN_SIZE_SCALE;
		vec2 const model_pos = vec2{-quad_size/2.0f, -quad_size/2.0f} * model_scale;
		mat4 const M = scale(translate(mat4{1}, vec3{model_pos,0}), vec3{model_scale, model_scale, 1});  // T*S
		mat4 const local_to_screen = P*V*M;

		if (events.info_request) {
			cout << "info:\n"
				<< "model_scale=" << model_scale << '\n'
				<< "model_pos=" << model_pos << '\n'
				<< with_label{"V", V} << '\n'
				<< with_label{"local_to_screen", local_to_screen} << '\n'
				<< "cammera: theta=" << cam.theta << ", phi=" << cam.phi << ", distance=" << cam.distance << '\n';
		}

		float const elevation_scale = model_scale / (elevation_pixel_size * texture_width);
		
		// render terrain
		if (features.show_terrain) {
			glUseProgram(shader_program);

			// bind height map texture
			// GLuint const & tile = tiles[0];  // TODO: we do not need list of tiles, just a tile
			glUniform1i(heights_loc, 0);  // set height map sampler to use texture unit 0
			glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
			glBindTexture(GL_TEXTURE_2D, height_map);  // bind a height texture to active texture unit (0)

			if (features.show_satellite) {
				glUniform1i(use_satellite_map_loc, 1);  // set use_satellite_map to true
				glUniform1i(satellite_map_loc, 1);  // set satellite map sampler to use testure unit 1
				glActiveTexture(GL_TEXTURE1);  // activate texture unit 1
				glBindTexture(GL_TEXTURE_2D, satellite_map);  // bind a satellite texture to active texture unit (1)
			}
			else {
				glUniform1i(use_satellite_map_loc, 0);  // just set use_satellite_map to false
			}

			if (features.calculate_shades) {
				glUniform1i(use_shading_loc, 1);  // just set use_shading to true
			}
			else
				glUniform1i(use_shading_loc, 0);  // just set use_shading to false

			glUniform2f(height_map_size_loc, texture_width, texture_height);
			glUniform1f(height_scale_loc, height_scale);  // TODO: can we set uniform before we use a program?
			glUniform1f(eleveation_scale_loc, elevation_scale);  // TODO: can we set uniform before we use a program?

			glUniformMatrix4fv(local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

			glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
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
			glUniform1f(lightdir_height_scale_loc, height_scale);  // TODO: can we set uniform before we use a program?
			glUniformMatrix4fv(lightdir_local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

			glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
		}

		// render triangle outlines
		if (features.show_outline) {
			glUseProgram(outline_shader_program);

			glUniform3fv(outline_line_color_loc, 1, value_ptr(rgb::blue));

			// bind height map
			glUniform1i(outline_heights_loc, 0);  // set sampler s to use texture unit 0
			glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
			glBindTexture(GL_TEXTURE_2D, height_map);  // bind a texture to active texture unit (0)

			glUniform2f(outline_height_map_size_loc, texture_width, texture_height);
			glUniform1f(outline_height_scale_loc, height_scale);  // TODO: can we set uniform before we use a program?
			glUniformMatrix4fv(outline_local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

			glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);
		}

		glBindVertexArray(0);  // unbind VAO

		ImGui::Render();  // render ImGui
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
	}
	
	// for(auto tile : tiles)  // delete textures
		// glDeleteTextures(1, &tile);

	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(shader_program);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
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

pair<vector<float>, vector<unsigned>> make_quad(unsigned w, unsigned h) {
	assert(w > 1 && h > 1 && "invalid dimensions");

	// vertices
	float const dx = 1.0f/(w-1),
		dy = 1.0f/(h-1);
	vector<float> verts((3+2+3)*w*h);  // position:3, texcoord:2

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
			*(idata++) = n;
			*(idata++) = n+1;
			*(idata++) = n+1+w;
			*(idata++) = n+1+w;
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
