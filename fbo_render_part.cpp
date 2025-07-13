/* Sample to render triangle into texture (via framebuffer) and then on screen. */
#include <string>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <cassert>
#include <SDL.h>
#include <GLES3/gl32.h>
#include "shader.hpp"

using std::cout, std::endl;
using std::filesystem::path;
using std::string, std::tuple;
using namespace std::string_literals;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600,
	FBO_WIDTH = 512,
	FBO_HEIGHT = 512;

// Shader programs to fill geometry.
char const * passthrough_vert = R"(
#version 320 es
layout(location = 0) in vec3 position;
void main() {
	gl_Position = vec4(position, 1.0f);
})";

char const * fill_frag = R"(
#version 320 es
precision mediump float;
out vec4 frag_color;
void main() {
	frag_color = vec4(1,0,0,1);  // red
})";

// Shader programs to render texture into FBO.
char const * texture_render_vert = R"(
#version 320 es
layout(location = 0) in vec3 position;  // we expect NDC rectangle ((-1,-1), (1,1))
out vec2 st;
void main() {
	st = position.xy/2.0 + 0.5;
	gl_Position = vec4(position, 1.0f);
})";

char const * texture_render_frag = R"(
#version 320 es
precision mediump float;
uniform sampler2D s;
in vec2 st;
out vec4 frag_color;
void main() {
	frag_color = texture(s, st);
})";

// Shader program to sample from texture.
char const * sample_frag = R"(
#version 320 es
precision highp float;

// Input texture from previous pass
uniform sampler2D u_input_texture;
uniform vec2 u_texel_size;  //!< Input texture texel size in pixels (=1/width, 1/height).

// Output
layout(location = 0) out vec4 out_result_pixel;

void main() {
	ivec2 result_pixel_coord = ivec2(gl_FragCoord.xy);  // Get integer coordinates of the output pixel.

	vec2 sample_coord = vec2(result_pixel_coord) * u_texel_size;  // transform uv info input texture space
	out_result_pixel = texture(u_input_texture, sample_coord);  // sample input texture
}
)";


tuple<GLuint, GLuint, GLuint> create_ndc_quad();
void draw_quad(GLuint vao);

tuple<GLuint, GLuint> create_fbo(GLuint width, GLuint height);

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

	GLuint fill_texture,
		sample_texture;  // shared textures

	{ // render into framebuffer
		// render geometry into part of FBO

		GLuint const object_shader_program = get_shader_program(passthrough_vert, fill_frag);
		assert(object_shader_program != 0);

		auto [fbo, tex] = create_fbo(FBO_WIDTH, FBO_HEIGHT);
		fill_texture = tex;
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glClearColor(252.0f/255.0f, 15.0f/255.0f, 192.0f/255.0f, 1.0f);  // pink color
		glClear(GL_COLOR_BUFFER_BIT);

		glViewport(0, 0, FBO_WIDTH/2, FBO_HEIGHT/2);  // render only into part of the FBO texture

		glUseProgram(object_shader_program);

		draw_quad(ndc_vao);

		glDeleteProgram(object_shader_program);
		glDeleteFramebuffers(1, &fbo);
	}  // render into framebuffer


	// copy/render part of color_texture into another texture
	{
		// sample color_texture from previous FBO into new one

		GLuint const sample_shader_program = get_shader_program(passthrough_vert, sample_frag);
		assert(sample_shader_program != 0);

		GLuint const output_width = FBO_WIDTH/2,
			output_height = FBO_HEIGHT/2;
		auto [fbo, tex] = create_fbo(output_width, output_height);
		sample_texture = tex;
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // black color
		glClear(GL_COLOR_BUFFER_BIT);


		glViewport(0, 0, output_width, output_height);  // render half of the input texture into whole output texture

		glUseProgram(sample_shader_program);

		// bind input texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fill_texture);
		glUniform1i(glGetUniformLocation(sample_shader_program, "u_input_texture"), 0);  // 0 is texture-unit index, in our case 0 (GL_TEXTURE0 from glActivateTexture call)

		{
			float const w_texel_size = 1.0f / static_cast<float>(FBO_WIDTH),
				h_texel_size = 1.0f / static_cast<float>(FBO_HEIGHT);
			glUniform2f(glGetUniformLocation(sample_shader_program, "u_texel_size"),
				w_texel_size, h_texel_size);
		}

		// and set textel size uniform

		draw_quad(ndc_vao);

		glDeleteProgram(sample_shader_program);
		glDeleteFramebuffers(1, &fbo);
	}

	// render texture
	GLuint const texture_shader_program = get_shader_program(texture_render_vert, texture_render_frag);
	assert(texture_shader_program != 0);

	{
		// switch to window framebuffer and render texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);  // return to the default FB

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, WIDTH, HEIGHT);

		glUseProgram(texture_shader_program);

		// bind color_texture
		GLint s_loc = glGetUniformLocation(texture_shader_program, "s");
		assert(s_loc != -1 && "unknown uniform");
		glUniform1i(s_loc, 0);
		glActiveTexture(GL_TEXTURE0);
	}  // render texture

	while (true) {  // render loop
		SDL_Event event;
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;

		// render texure
		// we want to draw in a loop to prevent flickering due to double buffering
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, WIDTH/2, HEIGHT/2);
		glBindTexture(GL_TEXTURE_2D, fill_texture);  // bind a texture to active texture unit (0)
		draw_quad(ndc_vao);

		glViewport(WIDTH/2, 0, WIDTH, HEIGHT/2);
		glBindTexture(GL_TEXTURE_2D, sample_texture);  // bind a texture to active texture unit (0)
		draw_quad(ndc_vao);

		// glViewport(0, 0, WIDTH, HEIGHT);
		// glBindTexture(GL_TEXTURE_2D, sample_texture);  // bind a texture to active texture unit (0)
		// draw_quad(ndc_vao);

		SDL_GL_SwapWindow(window);
	}

	glDeleteProgram(texture_shader_program);

	// cleanup
	glDeleteTextures(1, &fill_texture);
	glDeleteTextures(1, &sample_texture);
	glDeleteBuffers(1, &ndc_vbo);
	glDeleteBuffers(1, &ndc_ibo);
	glDeleteVertexArrays(1, &ndc_vao);

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

tuple<GLuint, GLuint> create_fbo(GLuint width, GLuint height) {
	// create framebuffer object (FBO) and bind
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// create color buffer texture
	GLuint color_texture;
	glGenTextures(1, &color_texture);
	glBindTexture(GL_TEXTURE_2D, color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// attach color and depth textures to the FBO
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_texture, 0);

	// tell OpenGL that we want to draw into the framebuffer's color attachement
	GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, draw_buffers);

	[[maybe_unused]] GLenum const fbo_status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	assert(fbo_status == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);  // unbind frambuffer

	return {fbo, color_texture};
}
