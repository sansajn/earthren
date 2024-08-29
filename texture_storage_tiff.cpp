// rendering texture with glTexStorage2D() sample
#include <string>
#include <tuple>
#include <cassert>
#include <iostream>
#include <memory>
#include <filesystem>
#include <fstream>
#include <SDL.h>
#include <GLES3/gl32.h>
#include <tiffio.hxx>

using std::cout, std::endl;
using std::byte;
using std::tuple, std::unique_ptr, std::move;
using std::filesystem::path;
using std::ifstream;

std::string const texture_path = "data/output_cropped_stretched.tif";  // TODO: hardcoded

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

char const * vs_src = R"(
#version 320 es
in vec3 position;  // we expect (-1,-1), (1,1) rectangle
out vec2 st;
void main() {
	st = position.xy/2.0 + 0.5;
	gl_Position = vec4(position, 1.0f);
})";

char const * fs_src = R"(
#version 320 es
precision mediump float;
uniform sampler2D s;
in vec2 st;
out vec4 frag_color;
void main() {
	frag_color = vec4(texture(s, st).rgb, 1.0);
})";

tuple<GLuint, GLuint, GLuint, unsigned> create_mesh();

//! Creates OpenGL texture from image \c fname file and returns texture ID.
GLuint create_texture_tiff_rgb8(std::string const & fname);

struct tiff_data_desc {
	size_t width, 
		height;
	uint8_t bytes_per_sample, 
		samples_per_pixel;
};

tuple<unique_ptr<byte>, tiff_data_desc> load_tiff(path const & tiff_file);


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
	
	GLuint const shader_program = get_shader_program(vs_src, fs_src);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	auto [vao, vbo, ibo, index_count] = create_mesh();
	GLuint tbo = create_texture_tiff_rgb8(texture_path);

	// rendering ...
	glUseProgram(shader_program);

	GLuint position_attr_id = 0;
	glVertexAttribPointer(position_attr_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(position_attr_id);

	GLint s_loc = glGetUniformLocation(shader_program, "s");
	assert(s_loc != -1 && "unknown uniform");
	glUniform1i(s_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tbo);  // bind a texture to active texture unit (0)

	while (true) {
		SDL_Event event;
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
		assert(glGetError() == GL_NO_ERROR && "opengl error");

		SDL_GL_SwapWindow(window);
	}

	glDeleteTextures(1, &tbo);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

GLuint create_texture_tiff_rgb8(std::string const & fname) {
	auto [pixels, desc] = load_tiff(fname);
	// TODO: we need to flip image
	assert(desc.bytes_per_sample == 1); // 8bit
	assert(desc.samples_per_pixel == 3); // RGB

	// To load RGB images charbot says that image rows must be aligned by 4 (which is not the case of our pixel data).
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint tbo;
	glGenTextures(1, &tbo);
	glBindTexture(GL_TEXTURE_2D, tbo);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, desc.width, desc.height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc.width, desc.height, GL_RGB, GL_UNSIGNED_BYTE, pixels.get());
	
	// TODO: automitic conversion to avoid changing alignment to 1 doesn't seems to working
	// glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, desc.width, desc.height);
	// glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc.width, desc.height, GL_RGB, GL_UNSIGNED_BYTE, pixels.get());
	
	glBindTexture(GL_TEXTURE_2D, 0);  // unbint texture

	return tbo;
}

tuple<unique_ptr<byte>, tiff_data_desc> load_tiff(path const & tiff_file) {
	ifstream fin{tiff_file};
	assert(fin.is_open());

	TIFF * tiff = TIFFStreamOpen("memory", &fin);
	assert(tiff);

	// image w, h
	uint32_t image_w = 0,
		image_h = 0;
	TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &image_w);
	TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &image_h);

	// strip related stuff
	tstrip_t const strip_count = TIFFNumberOfStrips(tiff);
	tmsize_t const strip_size = TIFFStripSize(tiff);

	uint32_t rows_per_strip = 0;
	TIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);

	uint16_t bits_per_sample = 0;
	TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);

	uint16_t sample_format = 0;
	TIFFGetField(tiff, TIFFTAG_SAMPLEFORMAT, &sample_format);
	assert(sample_format == SAMPLEFORMAT_UINT || sample_format == SAMPLEFORMAT_INT);

	uint16_t samples_per_pixel = 0;
	TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
	assert(samples_per_pixel == 1 || samples_per_pixel == 3);  // expect GRAY or RGB images

	// read strip by strip and store it into whole image
	unsigned const image_size = image_w*image_h*(bits_per_sample/8)*samples_per_pixel;
	unique_ptr<byte> image_data{new byte[image_size]};
	memset(image_data.get(), 0xff, image_size);   // clear buffer
	for (tstrip_t strip = 0; strip < strip_count; ++strip) {
		unsigned const offset = strip*strip_size;
		uint8_t * buf = reinterpret_cast<uint8_t *>(image_data.get()) + offset;
		tmsize_t ret = TIFFReadEncodedStrip(tiff, strip, buf, (tsize_t)-1);
		assert(ret != -1 && ret > 0);
	}

	TIFFClose(tiff);
	fin.close();

	tiff_data_desc const desc = {
		.width=image_w,
		.height=image_h,
		.bytes_per_sample=bits_per_sample/8,  // TODO: deal with warning there
		.samples_per_pixel=samples_per_pixel};  

	return {std::move(image_data), desc};
}


tuple<GLuint, GLuint, GLuint, unsigned> create_mesh() {
	constexpr GLfloat vertices[] = {
		-1, -1, 0,
		 1, -1, 0,
		 1,  1, 0,
		-1,  1, 0};

	constexpr GLuint indices[] = {
		0, 1, 2,  2, 3, 0
	};

	unsigned index_count = sizeof(indices)/sizeof(GLuint);

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

	return {vao, vbo, ibo, index_count};
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
