/* Sample to draw triangle to NDC (Normalized Device Coordinate) space (-1,-1), (1,1), can serve as basic sample */
#include <string>
#include <cassert>
#include <iostream>
#include <SDL.h>
#include <GLES3/gl32.h>
#include "shader.hpp"

using std::cout, std::endl;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

char const * vs_src = R"(
#version 320 es
in vec3 position;  // we expect NDC (-1,-1), (1,1) rectangle
void main() {
	gl_Position = vec4(position, 1.0f);
})";

char const * fs_src = R"(
#version 320 es
precision mediump float;
out vec4 frag_color;
void main() {
	frag_color = vec4(1,0,0,1);  // red
})";

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

	GLuint const shader_program = get_shader_program(vs_src, fs_src);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	GLfloat vertices[] = {
		-.5f, -.5f, .0f,
		.5f, -.5f, .0f,
		.0f, .5f, .0f};

	GLfloat colors[] = {
		1.0f, .0f, .0f, 1.0f,
		.0f, 1.0f, .0f, 1.0f,
		.0f, .0f, 1.0f, 1.0f};

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint tribuf;
	glGenBuffers(1, &tribuf);
	glBindBuffer(GL_ARRAY_BUFFER, tribuf);
	glBufferData(GL_ARRAY_BUFFER, 7*3*sizeof(GLfloat), nullptr, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 3*3*sizeof(GLfloat), (GLvoid *)vertices);
	glBufferSubData(GL_ARRAY_BUFFER, 3*3*sizeof(GLfloat), 3*4*sizeof(GLfloat), (GLvoid *)colors);

	GLuint position_attr_id = 0, color_attr_id = 1;
	glVertexAttribPointer(position_attr_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(position_attr_id);

	glVertexAttribPointer(color_attr_id, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(3*3*sizeof(GLfloat)));
	glEnableVertexAttribArray(color_attr_id);

	glUseProgram(shader_program);

	while (true) {
		SDL_Event event;
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		assert(glGetError() == GL_NO_ERROR && "opengl error");

		SDL_GL_SwapWindow(window);
	}

	glDeleteProgram(shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
