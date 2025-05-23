// Shape sample.
#include <string>
#include <cassert>
#include <iostream>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include "ogl/gl_api.hpp"
#include "shape_mesh.hpp"

using std::cout, std::endl;
using glm::vec3;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

char const * vs_src = R"(
#version 320 es
precision mediump float;
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
uniform mat4 T[3];  // M, V, P transformation
uniform mat3 normal_to_view;
out vec3 n;
void main() {
	n = normal_to_view * normal;
	gl_Position = T[2] * T[1] * T[0] * vec4(position, 1);
}
)";

char const * fs_src = R"(
#version 320 es
precision mediump float;
uniform vec3 color;
in vec3 n;
out vec4 fcolor;
vec3 light_dir = normalize(vec3(1,1,1));  // in view space
void main() {
	fcolor = vec4(max(dot(n, light_dir), 0.2) * color, 1);
}
)";

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source);


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

	GLuint const prog = get_shader_program(vs_src, fs_src);

	ogl::mesh sphere = make_mesh(make_sphere(1.0f, 16, 16));

	while (true) {
		SDL_Event event;
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;

		// rendering ...
		glUseProgram(prog);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		glm::mat4 P = glm::perspective(glm::radians(60.0f), 800.0f/600.0f, 0.1f, 100.0f);
		glm::vec3 campos(5.0f, 5.0f, 5.0f);
		glm::vec3 origin(0.0f, 0.0f, 0.0f);
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::mat4 V = glm::lookAt(campos, origin, up);
		glm::mat4 M(1.0f);
		glm::mat4 T[3] = {M, V, P};
		glm::mat3 normal_to_view = glm::mat3{glm::inverseTranspose(V*M)};

		glm::vec3 color{0.6, 0.2, 0.3};

		GLint T_loc = glGetUniformLocation(prog, "T[0]");
		assert(T_loc != -1 && "unknown uniform");
		glUniformMatrix4fv(T_loc, 3, GL_FALSE, glm::value_ptr(*T));  // upload 3 matrices
		assert(glGetError() == GL_NO_ERROR);

		GLint normal_transform_loc = glGetUniformLocation(prog, "normal_to_view");
		assert(normal_transform_loc != -1 && "unknown uniform");
		glUniformMatrix3fv(normal_transform_loc, 1, GL_FALSE, glm::value_ptr(normal_to_view));
		assert(glGetError() == GL_NO_ERROR);

		GLint color_loc = glGetUniformLocation(prog, "color");
		assert(color_loc != -1 && "unknown uniform");
		glUniform3fv(color_loc, 1, glm::value_ptr(color));
		assert(glGetError() == GL_NO_ERROR);

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		sphere.bind();
		glDrawElements(sphere.topology(), sphere.size(), GL_UNSIGNED_INT, 0);

		assert(glGetError() == GL_NO_ERROR);

		SDL_GL_SwapWindow(window);
	}

	glDeleteProgram(prog);

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
