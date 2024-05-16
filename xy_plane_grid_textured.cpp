// OpenGL ES 3.2, xy plane sample with zoom and pan
#include <chrono>
#include <iostream>
#include <cassert>
#include <fmt/core.h>
#include <SDL.h>
#include <GLES3/gl32.h>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Magick++.h>

using std::cout, std::endl;
using fmt::print;
using std::chrono::steady_clock, std::chrono::duration_cast, 
	std::chrono::milliseconds;
using glm::mat4, 
	glm::vec3, glm::vec2,
	glm::value_ptr,
	glm::perspective,
	glm::translate,
	glm::radians;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

GLchar const * vertex_shader_source = R"(
#version 320 es
uniform mat4 local_to_screen;
in vec3 position;
in vec2 st;  // texture coordinates TODO: why not uv?
out vec2 tex_coord;
void main() {
	gl_Position = local_to_screen * vec4(position, 1.0);
    tex_coord = st;
})";

GLchar const * fragment_shader_source = R"(
#version 320 es
precision mediump float;
in vec2 tex_coord;
out vec4 frag_color;
uniform vec3 color;
uniform sampler2D s;
void main() {
    frag_color = mix(texture(s, tex_coord), vec4(color, 1.0), 0.5);
})";

GLfloat const xy_plane_verts[] = {
	// triangle 1
	0.f, 0.f, 0.f,
	1.f, 0.f, 0.f,
	1.f, 1.f, 0.f,
	
	// triangle 2
	1.f, 1.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 0.f, 0.f
};

GLfloat const xy_plane_texcoords[] = {
    0, 0,  // triangle 1
    1, 0,
    1, 1,
    1, 1,  // triangle 2
    0, 1,
    0, 0
};

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source);

//! Creates OpenGL texture from image \c fname file and returns texture ID.
GLuint create_texture(std::string const & fname);

// TODO: can we benefit there with create_mesh function?

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
    GLint const st_loc = glGetAttribLocation(shader_program, "st");
	GLint const local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen");
	GLint const color_loc = glGetUniformLocation(shader_program, "color");
    GLint const s_loc = glGetUniformLocation(shader_program, "s");
	
	glUseProgram(shader_program);

    // generate texture
    GLuint texture = create_texture("lena.jpg");

	mat4 P = perspective(radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

    GLuint vbo[2];  // vertices, texcoords
    glGenBuffers(2, vbo);

    // for position attributes
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(xy_plane_verts), xy_plane_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(position_loc);

    // for st attributes
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(xy_plane_texcoords), xy_plane_texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(st_loc, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(st_loc);

    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbid current buffer



	float distance = 2.0f;
	vec2 xy_offset = vec2{0, 0};
	bool pan = false;

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
				print("Mouse Wheel Up (y={})\n", event.wheel.y);
			else if (event.wheel.y < 0)  // scroll down
				print("Mouse Wheel Down (y={})\n", event.wheel.y);

			distance -= event.wheel.y;
		}

		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
			print("Left mouse pressed\n");
			pan = true;
		}

		if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
			print("Left mouse released\n");
			pan = false;
		}

		if (pan && event.type == SDL_MOUSEMOTION) {
			print("relative movement ({},{})\n", event.motion.xrel, event.motion.yrel);
			xy_offset -= vec2{event.motion.xrel, -event.motion.yrel} * dt;
		}

		vec3 position = vec3{xy_offset,0} + vec3{0, 0, distance};
		mat4 V = inverse(translate(mat4{1}, position));  // we can also use lookAt there
		
		// render
		glClear(GL_COLOR_BUFFER_BIT);  // clear buffer

        // grid dimensions
        constexpr unsigned row_count = 5,
            col_count = 4;

		// draw grid of xy planes
        for (unsigned row = 0; row < row_count; ++row) {
            for (unsigned col = 0; col < col_count; ++col) {
                // bind texture to a sampler (this is not changing)
                glUniform1i(s_loc, 0);  // set sampler s to use texture unit 0
                glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
                glBindTexture(GL_TEXTURE_2D, texture);  // bind a texture to active texture unit (0)
                // TODO: there we want to bind a proper texture

				float const model_scale = 2.0f;
                vec2 model_pos = (vec2{col, row} - vec2(col_count, row_count)/2.0f) * model_scale;
				mat4 M = scale(translate(mat4{1}, vec3{model_pos,0}), vec3{model_scale, model_scale, 1});  // T*S
				mat4 local_to_screen = P*V*M;
				glUniformMatrix4fv(local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));
				
                vec3 color = (col + (row*col_count)) % 2 ? vec3{1, 0, 0} : vec3{0, 0, 1};
				glUniform3f(color_loc, color.r, color.g, color.b);
				
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		SDL_GL_SwapWindow(window);
	}
	
    glDeleteBuffers(2, vbo);
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
