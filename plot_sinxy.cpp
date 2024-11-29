// OpenGL ES 3.2, plot of z=sin(x)*sin(y) surface
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
#include <SDL.h>
#include <GLES3/gl32.h>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "geometry/glmprint.hpp"

using std::vector, std::string, std::tuple, std::pair, std::byte;
using std::unique_ptr;
using std::filesystem::path, std::ifstream;
using std::cout, std::endl;
using fmt::print;
using std::chrono::steady_clock, std::chrono::duration_cast, 
	std::chrono::milliseconds;
using glm::mat4,
	glm::vec4, glm::vec3, glm::vec2,
	glm::value_ptr,
	glm::perspective,
	glm::translate,
	glm::radians;

using namespace std::string_literals;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

GLchar const * vertex_shader_source = R"(
#version 320 es
in vec3 position;  // expected to be in a range of ((0,0), (1,1)) square
out float h;  // forward height value

uniform mat4 local_to_screen;
uniform mat3 normal_to_view;

const float PI = 3.14159265359;

void main() {
	// calculate h value
	vec2 sinxy = sin(position.xy * PI);  // from 0..pi range
	h = sinxy.x * sinxy.y;
	vec3 pos = vec3(position.xy, h);

	gl_Position = local_to_screen * vec4(pos, 1.0);
})";

GLchar const * fragment_shader_source = R"(
#version 320 es
precision mediump float;

in float h;
vec3 color_a = vec3(0, 0, 1);  // blue
vec3 color_b = vec3(1, 0, 0);  // red
out vec4 frag_color;

void main() {
	frag_color = vec4(mix(color_a, color_b, h), 1.0);
})";

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source);

tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc);

/*! Creates quad mesh on GPU with a size=1.
\return (vao, vbo, vertex_count) tuple. */
tuple<GLuint, GLuint, unsigned> create_mesh(GLint vertices_loc, GLint uv_loc);



constexpr float pi = glm::pi<float>();


struct map_camera {
	float theta,  //!< x-axis camera rotation in rad
		phi;  //!< z-axis camera rotation in rad
	float distance;  //!< camera distance from ground

	vec2 look_at;  //!< look-at point on a map

	map_camera(float d = 1.0f) : theta{0}, phi{0}, distance{d}, look_at{0, 0} {}
	mat4 const & view() const {return _view;}

	void update() {
		vec3 const p0 = {look_at, 0};

		vec3 const px = {1, 0, 0},
			py = {0, 1, 0},
			pz = {0, 0, 1};

		float const cp = cos(phi),
			sp = sin(phi),
			ct = cos(theta),
			st = sin(theta);

		vec3 const cx = px*cp + py*sp,
			cy = -px*sp*ct + py*cp*ct + pz*st,
			cz = px*sp*st - py*cp*st + pz*ct;

		vec3 const position = p0 + cz * distance;

		mat4 const V = {
			cx.x, cy.x, cz.x, 0,
			cx.y, cy.y, cz.y, 0,
			cx.z, cy.z, cz.z, 0,
			0, 0, 0, 1
		};

		_view = glm::translate(V, -position);
	}

 private:
	mat4 _view;  //!< Camera view transformation matrix.
};

struct input_mode {
	bool pan, rotate;
};

/*! Process user input.
\returns false in case user want to quit, otherwise true. */
bool process_user_events(map_camera & cam, input_mode & mode);


int main(int argc, char * argv[]) {
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

	GLuint const shader_program = get_shader_program(vertex_shader_source, fragment_shader_source);
	
	GLint const position_loc = glGetAttribLocation(shader_program, "position");
	GLint const local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen");
	GLint const normal_to_view_loc = glGetUniformLocation(shader_program, "normal_to_view");
	
	glUseProgram(shader_program);

	// generate texture
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
	glDeleteProgram(shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
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

// TODO: we do not need normals and texcoords
/*! Returns vector of data in a (position:3, texcoord:2, normal:3) format per vertex and array of indices to form a model. */
pair<vector<float>, vector<unsigned>> make_quad(unsigned w, unsigned h) {
	assert(w > 1 && h > 1 && "invalid dimensions");

	// vertices
	float const dx = 1.0f/(w-1),
		dy = 1.0f/(h-1);
	std::vector<float> verts((3+2+3)*w*h);  // position:3, texcoord:2, normal:3

	float * vdata = verts.data();
	for (int j = 0; j < h; ++j) {
		float py = j*dy;
		for (int i = 0; i < w; ++i) {
			float px = i*dx;
			*vdata++ = px;  // position
			*vdata++ = py;
			*vdata++ = 0;
			*vdata++ = px;  // texcoord
			*vdata++ = py;
			*vdata++ = 0;  // normal
			*vdata++ = 0;
			*vdata++ = 1;
		}
	}

	// indices
	unsigned const nindices = 2*(w-1)*(h-1)*3;
	std::vector<unsigned> indices(nindices);
	unsigned * idata = indices.data();
	for (int j = 0; j < h-1; ++j) {
		unsigned yoffset = j*w;
		for (int i = 0; i < w-1; ++i) {
			int n = i + yoffset;
			*(idata++) = n;
			*(idata++) = n+1;
			*(idata++) = n+1+w;
			*(idata++) = n+1+w;
			*(idata++) = n+w;
			*(idata++) = n;
		}
	}

	// TODO: I don't want to use ash there. What are other options?
	// Mesh m{verts.data(), (3+2+3)*verts.size(), indices.data(), indices.size()};
	// unsigned stride = (3+2+3)*sizeof(float);
	// m.attach_attributes({
	// 	typename Mesh::vertex_attribute_type{0, 3, GL_FLOAT, stride, 0},
	// 	typename Mesh::vertex_attribute_type{1, 2, GL_FLOAT, stride, 3*sizeof(float)},
	// 	typename Mesh::vertex_attribute_type{2, 3, GL_FLOAT, stride, (3+2)*sizeof(float)}});

	// return m;

	return {verts, indices};
}

tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc) {
	auto [vertices, indices] = make_quad(100, 100);  // TODO: we need to figure out proper dimensions there

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

	// x,y,z
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, (3+2+3)*sizeof(float), (GLvoid*)0);
	glEnableVertexAttribArray(position_loc);

	glBindVertexArray(0);  // unbind vertex array

	return {vao, vbo, ibo, size(indices)};
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
