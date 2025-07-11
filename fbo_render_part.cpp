/* Sample to render triangle into texture (via framebuffer) and then on screen. */
#include <string>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <cassert>
#include <SDL.h>
#include <GLES3/gl32.h>
#include <Magick++.h>
#include "shader.hpp"

using std::cout, std::endl;
using std::filesystem::path;
using std::string, std::tuple;
using namespace std::string_literals;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600,
	FBO_WIDTH = 512,
	FBO_HEIGHT = 512;

char const * object_vs_src = R"(
#version 320 es
layout(location = 0) in vec3 position;
void main() {
	gl_Position = vec4(position, 1.0f);
})";

char const * object_fs_src = R"(
#version 320 es
precision mediump float;
out vec4 frag_color;
void main() {
	frag_color = vec4(1,0,0,1);  // red
})";

// Shader programs to render texture into FBO.
char const * texture_vs_src = R"(
#version 320 es
layout(location = 0) in vec3 position;  // we expect NDC rectangle ((-1,-1), (1,1))
out vec2 st;
void main() {
	st = position.xy/2.0 + 0.5;
	gl_Position = vec4(position, 1.0f);
})";

char const * texture_fs_src = R"(
#version 320 es
precision mediump float;
uniform sampler2D s;
in vec2 st;
out vec4 frag_color;
void main() {
	frag_color = texture(s, st);
})";

tuple<GLuint, GLuint, GLuint> create_ndc_quad();
void draw_quad(GLuint vao);

int main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
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

	auto [ndc_vao, ndc_vbo, ndc_ibo] = create_ndc_quad();

	GLuint color_texture;  // shared texture

	{ // render into framebuffer
		// create framebuffer object (FBO) and bind
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// create color buffer texture
		glGenTextures(1, &color_texture);
		glBindTexture(GL_TEXTURE_2D, color_texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, FBO_WIDTH, FBO_HEIGHT);

		// turn off mipmaps for color texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// attach color and depth textures to the FBO
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_texture, 0);

		// tell OpenGL that we want to draw into the framebuffer's color attachement
		GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, draw_buffers);

		GLenum fbo_status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		assert(fbo_status == GL_FRAMEBUFFER_COMPLETE);

		// render triangle info FBO

		GLuint const object_shader_program = get_shader_program(object_vs_src, object_fs_src);
		assert(object_shader_program != 0);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, FBO_WIDTH/2, FBO_HEIGHT/2);  // render only into part of the FBO texture

		glUseProgram(object_shader_program);

		glClear(GL_COLOR_BUFFER_BIT);
		draw_quad(ndc_vao);
		assert(glGetError() == GL_NO_ERROR && "opengl error");

		glDeleteProgram(object_shader_program);
	}  // render into framebuffer

	{  // render texture
		// switch to window framebuffer and render texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);  // return to the default FB

		GLuint const texture_shader_program = get_shader_program(texture_vs_src, texture_fs_src);
		assert(texture_shader_program != 0);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, WIDTH, HEIGHT);

		// bind color_texture
		// render texture
		glUseProgram(texture_shader_program);

		GLint s_loc = glGetUniformLocation(texture_shader_program, "s");
		assert(s_loc != -1 && "unknown uniform");
		glUniform1i(s_loc, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, color_texture);  // bind a texture to active texture unit (0)

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		draw_quad(ndc_vao);

		glDeleteProgram(texture_shader_program);
	}  // render texture


	glDeleteTextures(1, &color_texture);

	glDeleteBuffers(1, &ndc_vbo);
	glDeleteBuffers(1, &ndc_ibo);
	glDeleteVertexArrays(1, &ndc_vao);
	// glDeleteProgram(texture_shader_program);
	// TODO: cleanup

	while (true) {
		SDL_Event event;
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;

		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

tuple<GLuint, GLuint, GLuint> create_ndc_quad() {
	constexpr GLfloat vertices[] = {
		-1, -1, 0,
		 1, -1, 0,
		 1,  1, 0,
		-1,  1, 0};

	constexpr GLuint indices[] = {
		0, 1, 2,  2, 3, 0
	};

	//unsigned index_count = sizeof(indices)/sizeof(GLuint);  //=6

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLuint), indices, GL_STATIC_DRAW);

	constexpr GLuint position_attr_id = 0;  // position attribute in shader program is expected to be 0, use `layout(location = 0)` syntax
	glVertexAttribPointer(position_attr_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(position_attr_id);

	glBindVertexArray(0);  // unbind VAO

	return {vao, vbo, ibo};
}

void draw_quad(GLuint vao) {
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	assert(glGetError() == GL_NO_ERROR && "opengl error");
	glBindVertexArray(0);
}
