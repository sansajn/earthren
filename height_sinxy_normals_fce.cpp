// OpenGL ES 3.2, plot of z=sin(x)*sin(y) surface
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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
#include <Magick++.h>
// #include "glmprint.hpp"  // uncomment in case of debug
#include "camera.hpp"

using std::vector, std::string, std::tuple, std::pair, std::byte;
using std::unique_ptr;
using std::filesystem::path, std::ifstream;
using std::cout, std::endl;
using std::numeric_limits;
using fmt::print;
using std::chrono::steady_clock, std::chrono::duration_cast, 
	std::chrono::milliseconds;
using boost::filesystem::load_string_file;
using glm::mat4,
	glm::vec4, glm::vec3, glm::vec2,
	glm::value_ptr,
	glm::perspective,
	glm::translate,
	glm::radians;
using namespace Magick;

using namespace std::string_literals;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

path const VERTEX_SHADER_FILE = "height_sinxy_normals_fce.vs",
	FRAGMENT_SHADER_FILE = "height_sinxy_normals_fce.fs";
path const HEIGHT_MAP_TEXTURE = "data/sinxy512_16.png";

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source);

//! Creates 16bit gray OpenGL texture from image \c fname file and returns texture ID.
GLuint create_texture_16b(path const & fname);

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

struct input_mode {
	bool pan, rotate;
};

/*! Process user input.
\returns false in case user want to quit, otherwise true. */
bool process_user_events(map_camera & cam, input_mode & mode);

void test_save_heightmap();

int main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
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
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// laod shader programs
	string vertex_shader,
		fragment_shader;
	load_string_file(VERTEX_SHADER_FILE.c_str(), vertex_shader);
	load_string_file(FRAGMENT_SHADER_FILE.c_str(), fragment_shader);

	GLuint const shader_program = get_shader_program(vertex_shader.c_str(), fragment_shader.c_str());
	
	GLint const position_loc = glGetAttribLocation(shader_program, "position");
	GLint const local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen");
	GLint const heights_loc = glGetUniformLocation(shader_program, "heights");
	
	glUseProgram(shader_program);

	// load texture
	GLuint const heights_tex = create_texture_16b(HEIGHT_MAP_TEXTURE);
	// GLuint const heights_tex = create_height_texture();

	mat4 const P = perspective(radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	constexpr float quad_size = 1.0f;
	auto [vao, vbo, ibo, element_count] = create_quad_mesh(position_loc);

	// camera related stuff
	map_camera cam{20.0f};
	cam.look_at = vec2{0, 0};

	input_mode mode = {  // TODO: we want either pan or rotate not both on the same time, simplify with enum-class
		.pan = false,
		.rotate = false
	};

	while (true) {
		if (!process_user_events(cam, mode))
			break;  // user wants to quit

		cam.update();

		mat4 V = cam.view();

		// render
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  // clear buffer

		glBindVertexArray(vao);

		// bind height map
		glUniform1i(heights_loc, 0);  // set sampler s to use texture unit 0
		glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
		glBindTexture(GL_TEXTURE_2D, heights_tex);  // bind a texture to active texture unit (0)

		float const model_scale = 2.0f;
		vec2 model_pos = vec2{-quad_size/2.0f, -quad_size/2.0f} * model_scale;
		mat4 M = scale(translate(mat4{1}, vec3{model_pos,0}), vec3{model_scale, model_scale, 1});  // T*S
		mat4 local_to_screen = P*V*M;
		glUniformMatrix4fv(local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

		glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);  // unbind VAO
		SDL_GL_SwapWindow(window);
	}
	
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	glDeleteTextures(1, &heights_tex);

	glDeleteProgram(shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}

GLuint create_texture_16b(path const & fname) {
	Magick::Image im{fname};
	im.flip();
	Magick::Blob imblob;
	im.write(&imblob, "GRAY");  // load image as grayscale

	GLuint tbo;
	glGenTextures(1, &tbo);
	glBindTexture(GL_TEXTURE_2D, tbo);

	// note: 16bit I/UI textures can not be interpolated and so filter needs to be set to GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );  // or GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R16UI, im.columns(), im.rows());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, im.columns(), im.rows(), GL_RED_INTEGER, GL_UNSIGNED_SHORT, imblob.data());
	glBindTexture(GL_TEXTURE_2D, 0);  // unbint texture

	return tbo;
}

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source) {
	enum Consts {INFOLOG_LEN = 512};
	GLchar infoLog[INFOLOG_LEN];
	GLint fragment_shader;
	GLint shader_program;
	GLint success;
	GLint vertex_shader;

	/* Vertex shader */
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" 
			<< infoLog << endl;
	}

	/* Fragment shader */
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" 
			<< infoLog << endl;
	}

	/* Link shaders */
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
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

bool process_user_events(map_camera & cam, input_mode & mode) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {  // we want to fully process the event queue, update state and then render
		if (event.type == SDL_QUIT)
			return 0;

		// update
		if (event.type == SDL_MOUSEWHEEL) {
			assert(event.wheel.direction == SDL_MOUSEWHEEL_NORMAL);

			if (event.wheel.y > 0)  // scroll up
				cam.distance /= 1.1;
			else if (event.wheel.y < 0)  // scroll down
				cam.distance *= 1.1;

			print("distance={}\n", cam.distance);
		}

		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
			print("Left mouse pressed\n");
			mode.pan = true;
		}

		if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
			print("Left mouse released\n");
			mode.pan = false;
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

		// handling keyboard actions (camera rotations, reset)
		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
			case SDLK_f:
				// TODO: the view is changed oposite way if elevation increased, for now I do not understand why.
				cam.theta += pi/180.0f;  // elevate +1 deg
				print("theta={}\n", cam.theta);
				break;
			case SDLK_v:
				cam.theta -= pi/180.0f;  // elevate -1 deg
				print("theta={}\n", cam.theta);
				break;

			// phi
			case SDLK_q:
				cam.phi -= pi/180.0f;
				print("phi={}\n", cam.phi);
				break;
			case SDLK_e:
				cam.phi += pi/180.0f;
				print("phi={}\n", cam.phi);
				break;

			// reset camera
			case SDLK_c:
				cam = map_camera{20.0f};
				cam.look_at = {0, 0};
				break;

			case SDLK_LCTRL:  // enable camera rotation state
				mode.rotate = true;
				break;
			}
		} else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_LCTRL:  // disable camera rotation state
				mode.rotate = false;
				break;
			}
		}
	}
	return 1;
}
