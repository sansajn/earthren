// OpenGL ES 3.2, xy plane sample with zoom and pan
#include <chrono>
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <tuple>
#include <fmt/core.h>
#include <SDL.h>
#include <GLES3/gl32.h>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Magick++.h>
#include "glmprint.hpp"

using std::vector, std::string, std::tuple;
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
uniform mat4 local_to_screen;
in vec3 position;
in vec2 uv;  // texture coordinates
out vec2 tex_coord;
void main() {
	gl_Position = local_to_screen * vec4(position, 1.0);
	tex_coord = uv;
})";

GLchar const * fragment_shader_source = R"(
#version 320 es
precision mediump float;
in vec2 tex_coord;
out vec4 frag_color;
uniform vec3 color;
uniform sampler2D s;
void main() {
	frag_color = mix(texture(s, tex_coord), vec4(color, 1.0), 0.15);
})";

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source);

//! Creates OpenGL texture from image \c fname file and returns texture ID.
GLuint create_texture(std::string const & fname);

/*! Creates application specific mesh on GPU.
\return (vao, vbo, vertex_count) tuple. */
tuple<GLuint, GLuint, unsigned> create_mesh(GLint vertices_loc, GLint uv_loc);

vector<GLuint> read_tiles();

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
	
	GLuint const shader_program = get_shader_program(vertex_shader_source, fragment_shader_source);
	
	GLint const position_loc = glGetAttribLocation(shader_program, "position");
	GLint const uv_loc = glGetAttribLocation(shader_program, "uv");
	GLint const local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen");
	GLint const color_loc = glGetUniformLocation(shader_program, "color");
	GLint const s_loc = glGetUniformLocation(shader_program, "s");
	
	glUseProgram(shader_program);

	// generate texture
	mat4 const P = perspective(radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	auto [vao, vbo, vertex_count] = create_mesh(position_loc, uv_loc);

	// grid dimensions
	constexpr unsigned row_count = 5,
		col_count = 5;

	vector<GLuint> tiles = read_tiles();

	bool pan = false;
	bool rotate = false;

	// camera related stuff
	map_camera cam{20.0f};
	cam.look_at = vec2{0, 0};

	auto t_prev = steady_clock::now();

	while (true) {
		auto t_now = steady_clock::now();
		
		float const dt = duration_cast<milliseconds>(t_now - t_prev).count() / 1000.0f;
		t_prev = t_now;

		SDL_Event event;
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;

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
			pan = true;
		}

		if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
			print("Left mouse released\n");
			pan = false;
		}

		// pan and rotation
		if (pan && event.type == SDL_MOUSEMOTION) {
			vec2 ds = vec2{event.motion.xrel, -event.motion.yrel};
			if (rotate) {
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
				rotate = true;
				break;
			}
		} else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_LCTRL:  // disable camera rotation state
				rotate = false;
				break;
			}
		}

		cam.update();

		mat4 V = cam.view();

		// render
		glClear(GL_COLOR_BUFFER_BIT);  // clear buffer

		glBindVertexArray(vao);

		// draw grid of xy planes
		for (unsigned row = 0; row < row_count; ++row) {
			for (unsigned col = 0; col < col_count; ++col) {
				// bind texture to a sampler (this is not changing)
				GLuint const & tile = tiles[col + row*col_count];
				glUniform1i(s_loc, 0);  // set sampler s to use texture unit 0
				glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
				glBindTexture(GL_TEXTURE_2D, tile);  // bind a texture to active texture unit (0)

				float const model_scale = 1.0f;
				vec2 model_pos = (vec2{col, row} - vec2(col_count, row_count)/2.0f) * model_scale;
				mat4 M = scale(translate(mat4{1}, vec3{model_pos,0}), vec3{model_scale, model_scale, 1});  // T*S
				mat4 local_to_screen = P*V*M;
				glUniformMatrix4fv(local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));
				
				vec3 color = (col + (row*col_count)) % 2 ? vec3{1, 0, 0} : vec3{0, 0, 1};
				glUniform3f(color_loc, color.r, color.g, color.b);

				glDrawArrays(GL_TRIANGLES, 0, vertex_count);
			}
		}

		glBindVertexArray(0);  // unbind VAO
		SDL_GL_SwapWindow(window);
	}
	
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}

vector<GLuint> read_tiles() {
	// OSM tiles are are addresed from left-bottom corner (so we need to flip y coordinate)
	constexpr unsigned from_x = 2210,
		to_x = 2215,
		from_y = 1386,
		to_y = 1391;

	vector<GLuint> tiles;
	tiles.reserve(25);
	for (unsigned y = to_y-1; y >= from_y; --y) {
		for (unsigned x = from_x; x < to_x; ++x) {
			string tile_name = fmt::format("{}_{}.png", x, y);
			tiles.push_back(create_texture("data/tiles/"s + tile_name));
		}
	}
	return tiles;
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

GLuint create_texture(std::string const & fname) {
	Magick::Image im{fname};
	im.flip();
	Magick::Blob imblob;
	im.write(&imblob, "RGBA");  // load image as rgba array

	GLuint tbo;
	glGenTextures(1, &tbo);
	glBindTexture(GL_TEXTURE_2D, tbo);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, im.columns(), im.rows());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, im.columns(), im.rows(), GL_RGBA, GL_UNSIGNED_BYTE, imblob.data());
	glBindTexture(GL_TEXTURE_2D, 0);  // unbint texture

	return tbo;
}

tuple<GLuint, GLuint, unsigned> create_mesh(GLint vertices_loc, GLint uv_loc) {
	constexpr GLfloat vertices[] = {  // x, y, z, u, v
		// triangle 1
		0.f, 0.f, 0.f, 0.f, 0.f,
		1.f, 0.f, 0.f, 1.f, 0.f,
		1.f, 1.f, 0.f, 1.f, 1.f,

		// triangle 2
		1.f, 1.f, 0.f, 1.f, 1.f,
		0.f, 1.f, 0.f, 0.f, 1.f,
		0.f, 0.f, 0.f, 0.f, 0.f
	};

	constexpr unsigned vertex_count = 6;

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// x,y,z
	glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, (3+2)*sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(vertices_loc);

	// u,v
	glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, (3+2)*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(uv_loc);

	glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind buffer
	glBindVertexArray(0);  // unbind vertex array

	return {vao, vbo, vertex_count};  // TODO: remove vbo_tex
}
