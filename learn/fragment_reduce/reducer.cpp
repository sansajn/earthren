// Sample for reduce fragment shader for OpenGL 3.2 ES.
#include <string>
#include <tuple>
#include <vector>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <cassert>
#include <format>
#include <SDL.h>
#include <GLES3/gl32.h>
#include <Magick++.h>
#include "shader.hpp"
#include "fs.hpp"

using std::cout, std::endl;
using std::filesystem::path;
using std::string, std::tuple;
using std::vector;
using namespace std::string_literals;

constexpr GLuint WIDTH = 512,
	HEIGHT = 512;

path const PASSTHROUGH_VERTEX_PROGRAM_PATH = "passthrough.vert",
	REDUCE_FRAGMENT_PROGRAM_PATH = "reduce_fragment.frag";


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

//! \return vao, vbo and ibo (quad data has 6 indices)
tuple<GLuint, GLuint, GLuint> create_quad();

//! Draw the quad created by create_quad function.
void draw_quad(GLuint vao);

// Simple structure to hold texture and maximum values
struct TextureData {
	GLuint textureId;     // OpenGL texture ID
	float maxValues[4];   // Maximum value for each channel (R,G,B,A)
	vector<float> data;  // pixel data in texture (RGBA) format
	GLuint width, 
		height;
};

// Create a texture with deterministic test data and track maximum values.
TextureData create_data_texture(int width, int height);

TextureData load_from_image(string const & file_name);

// TODO: we can remove
vector<float> read_back(GLuint fbo, int width, int height) {
	assert(width >= 1 && height >= 1 && "Width and height must be at least 1");
	
	vector<float> pixels(width * height * 4);  // RGBA format
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, pixels.data());

	return pixels;
}

void save_texture_as_png(GLuint textureID, std::string const & filename);

void save_image_rgba(vector<float> const & pixels_rgba, size_t w, size_t h, string const & fname);

//! Switch to the window framebuffer and render texture.
void draw_texture(GLuint texture_id, GLuint width, GLuint height, GLuint texture_program, GLuint qaud_vao);

//! \returns reduced texture id
GLuint reduce_texture_half(GLuint texture_id, GLuint width, GLuint height, 
	GLuint reduce_program, GLuint quad_vao);

// Reads back RGBA32F texture data from OpenGL framebuffer for OpenGL ES 3.2.
vector<float> read_back_rgba32f(GLuint texture_id, GLuint width, GLuint height);

int main(int argc, char * argv[]) {
	// process arguments
	Magick::InitializeMagick(*argv);
	
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

	// we want to read texture
	size_t const texture_w = WIDTH, texture_h = HEIGHT;

	string const vs_src = read_file(PASSTHROUGH_VERTEX_PROGRAM_PATH),
		fs_src = read_file(REDUCE_FRAGMENT_PROGRAM_PATH);
	assert(!vs_src.empty() && !fs_src.empty() && "Shader source files must not be empty");

	GLuint const reduce_shader_program = get_shader_program(vs_src.c_str(), fs_src.c_str());
	assert(reduce_shader_program != 0);

/*	
	// Vertex data for a full-screen quad (positions only)
	const float quadVertices[] = {
		// positions (x,y)
		-1.0f,  1.0f,  // top-left
		-1.0f, -1.0f,  // bottom-left
		1.0f, -1.0f,  // bottom-right
		
		-1.0f,  1.0f,  // top-left
		1.0f, -1.0f,  // bottom-right
		1.0f,  1.0f   // top-right
	};

	// Create and bind VAO, VBO
	GLuint quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// unbind for now
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// setup FBOs and textures for reduction

	// Ping-pong FBOs and textures for reduction passes
	GLuint fboA, fboB;
	GLuint textureA, textureB;
	int initialWidth = WIDTH, initialHeight = HEIGHT;

	// Create FBOs
	glGenFramebuffers(1, &fboA);
	glGenFramebuffers(1, &fboB);
	
	// Create textures
	glGenTextures(1, &textureA);
	glGenTextures(1, &textureB);
	
	// Setup texture A
	glBindTexture(GL_TEXTURE_2D, textureA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, initialWidth/2, initialHeight/2, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Setup texture B
	glBindTexture(GL_TEXTURE_2D, textureB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, initialWidth/4, initialHeight/4, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Attach textures to FBOs
	glBindFramebuffer(GL_FRAMEBUFFER, fboA);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureA, 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, fboB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureB, 0);
	
	// Reset bindings
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
*/

	GLuint initialWidth = WIDTH, initialHeight = HEIGHT;

	// create input data texture
	TextureData tex;  // GL_RGBA with GL_RGBA32F
	if (argc > 1 && std::filesystem::exists(argv[1])) {
		tex = load_from_image(argv[1]);
		initialWidth = tex.width;
		initialHeight = tex.height;
		cout << "Loaded image: " << argv[1] << " (" << tex.width << "x" << tex.height << ")\n";
	} 
	else
		tex = create_data_texture(initialWidth, initialHeight);

	GLuint inputTexture = tex.textureId;

	save_image_rgba(tex.data, initialWidth, initialHeight, "reduction_0.png");

	// reduce

	// Save current OpenGL state
/*	
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);
	
	// Use reduction shader
	glUseProgram(reduce_shader_program);

	// Initial dimensions
	int currentWidth = initialWidth;
	int currentHeight = initialHeight;

	// First pass: input texture -> texture A
	glBindFramebuffer(GL_FRAMEBUFFER, fboA);
	glViewport(0, 0, currentWidth/2, currentHeight/2);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	glUniform1i(glGetUniformLocation(reduce_shader_program, "inputTexture"), 0);
	
	{
		float const w_texel_size = 1.0f / static_cast<float>(currentWidth/2),  // TODO: copilot adviced 1/currentWidth
			h_texel_size = 1.0f / static_cast<float>(currentHeight/2);
		
		glUniform2f(glGetUniformLocation(reduce_shader_program, "texelSize"), 
						w_texel_size, h_texel_size);
		cout << "Texel size: (" << w_texel_size << ", " << h_texel_size << ")\n";
	}

	// Draw full-screen quad
	drawQuad(quadVAO);
*/

	// NDC quad can be used to render into for reduce sahder and texture shader
	auto const [quad_vao, quad_vbo, quad_ibo] = create_quad();

/*	
	// Update dimensions
	currentWidth /= 2;
	currentHeight /= 2;

	// Ping-pong between FBOs until we reach a small size
	GLuint currentInputTexture = textureA;
	GLuint currentFBO = fboB;
	GLuint currentOutputTexture = textureB;

	int reduce_iteration = 1;
	while (currentWidth > 1 || currentHeight > 1) {
		// Bind output FBO
		glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
		glViewport(0, 0, std::max(1, currentWidth/2), std::max(1, currentHeight/2));
		
		// Set input texture and uniforms
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, currentInputTexture);
		glUniform1i(glGetUniformLocation(reduce_shader_program, "inputTexture"), 0);  // 0 is texture-unit index, in our case 0 (GL_TEXTURE0 from glActivateTexture call)
		
		{
			float const w_texel_size = 1.0f / static_cast<float>(currentWidth/2),  // TODO: copilot adviced 1/currentWidth
				h_texel_size = 1.0f / static_cast<float>(currentHeight/2);
			
			glUniform2f(glGetUniformLocation(reduce_shader_program, "texelSize"), 
							w_texel_size, h_texel_size);
			cout << "Texel size: (" << w_texel_size << ", " << h_texel_size << ")\n";
		}
		
		drawQuad(quadVAO);
		
		if (reduce_iteration == 2) {
			vector<float> const pixels = read_back(currentFBO, currentWidth/2, currentHeight/2);
			save_image_rgba(pixels, currentWidth/2, currentHeight/2, "reduction_2.png");
		}

		// Swap input and output for next pass
		currentInputTexture = currentOutputTexture;
		currentFBO = (currentFBO == fboA) ? fboB : fboA;
		currentOutputTexture = (currentOutputTexture == textureA) ? textureB : textureA;

		cout << "Reduction iteration: " << reduce_iteration 
			<< ", (width=)" << currentWidth << ", (height=)" << currentHeight
			<< '\n';

		// Update dimensions
		currentWidth = std::max(1, currentWidth/2);
		currentHeight = std::max(1, currentHeight/2);
		
		reduce_iteration += 1;
	}

	// TODO: make a funciton to read back texture data from bellow code

	vector<float> result = read_back(currentFBO == fboA ? fboB : fboA, 1, 1);
	cout << "Reduction result: (" << result[0] << ", " << result[1] << ", "
		<< result[2] << ", " << result[3] << ")" << endl;

*/

	// program for drawing texture
	GLuint const texture_shader_program = get_shader_program(texture_vs_src, texture_fs_src);
		assert(texture_shader_program != 0);

	GLuint current_width = initialWidth, 
		current_height = initialHeight;
	GLuint rendered_texture = inputTexture;
	GLuint reduce_level = 1;

	while (true) {
		SDL_Event event;
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;

		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
			// create rgba32f texture for reduction
			GLuint reduced_texture_id = reduce_texture_half(rendered_texture, 
				current_width, current_height, reduce_shader_program, quad_vao);
			
			// save for debugging
			GLuint const w = current_width/2, 
				h = current_height/2;
			vector<float> const pixels = read_back_rgba32f(reduced_texture_id, w, h);
			save_image_rgba(pixels, w, h, std::format("reduction_{}.png", reduce_level));

			cout << "texture reduced to (" << w << "x" << h << "), level= " << reduce_level << endl;

			if (w == 1 && h == 1) {
				cout << "Final reduction result: (" << pixels[0] << ", "
					<< pixels[1] << ", " << pixels[2] << ", "
					<< pixels[3] << ")" << endl;
			}

			if (rendered_texture != inputTexture)
				glDeleteTextures(1, &rendered_texture);

			rendered_texture = reduced_texture_id;
			current_width /= 2;
			current_height /= 2;
			reduce_level += 1;
		}

		draw_texture(rendered_texture, WIDTH, HEIGHT, texture_shader_program, quad_vao);

		SDL_GL_SwapWindow(window);
	}

	// Restore previous OpenGL state
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	glDeleteBuffers(1, &quad_vbo);
	glDeleteBuffers(1, &quad_ibo);
	glDeleteVertexArrays(1, &quad_vao);
	glDeleteProgram(texture_shader_program);



	// glDeleteVertexArrays(1, &quadVAO);
	// glDeleteFramebuffers(1, &fboA);
	// glDeleteFramebuffers(1, &fboB);
	// glDeleteTextures(1, &textureA);
	// glDeleteTextures(1, &textureB);
	glDeleteProgram(reduce_shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}


// Reads back RGBA32F texture data from OpenGL framebuffer for OpenGL ES 3.2.
vector<float> read_back_rgba32f(GLuint texture_id, GLuint width, GLuint height) {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
								GL_COLOR_ATTACHMENT0,
								GL_TEXTURE_2D,
								texture_id,
								0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("FBO incomplete");

	// prepare storage
	vector<float> data(width*height*4);
	// read back floats
	glReadPixels(0, 0, width, height,
					GL_RGBA, GL_FLOAT,
					data.data());

	// cleanup
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);

	return data;
}

//! \returns reduced texture id
GLuint reduce_texture_half(GLuint texture_id, GLuint width, GLuint height, 
	GLuint reduce_program, GLuint quad_vao) {
	
	// prepare framebuffer with texture attachement for reduction
	GLuint fbo;
	glGenFramebuffers(1, &fbo);

	GLuint const reduced_w = width/2, reduced_h = height/2;

	GLuint reduced_texture;
	glGenTextures(1, &reduced_texture);
	glBindTexture(GL_TEXTURE_2D, reduced_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, reduced_w, reduced_h, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reduced_texture, 0);
	
	glViewport(0, 0, reduced_w, reduced_h);

	// initialize shader program for reduction
	glUseProgram(reduce_program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glUniform1i(glGetUniformLocation(reduce_program, "inputTexture"), 0);

	float const w_texel_size = 1.0f / static_cast<float>(width),
		h_texel_size = 1.0f / static_cast<float>(height);
	
	glUniform2f(glGetUniformLocation(reduce_program, "texelSize"), 
					w_texel_size, h_texel_size);

	draw_quad(quad_vao);

	glDeleteFramebuffers(1, &fbo);

	return reduced_texture;
}

/*! Switch to the window framebuffer and render texture. 
\param [in] width window width in pixels (not texture width)
\param [in] height window height in pixels */
void draw_texture(GLuint texture_id, GLuint width, GLuint height, 
	GLuint texture_program, GLuint quad_vao) {
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);  // return to the default FB

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, width, height);

	// bind color_texture
	// render texture
	glUseProgram(texture_program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);  // bind a texture to active texture unit (0)

	GLint s_loc = glGetUniformLocation(texture_program, "s");
	assert(s_loc != -1 && "unknown uniform");
	glUniform1i(s_loc, 0);  // GL_TEXTURE0 + 0
	
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	draw_quad(quad_vao);
}


void save_image_rgba(vector<float> const & pixels_rgba, size_t w, size_t h, string const & fname) {
	Magick::Image im;
	im.read(w, h, "RGBA", Magick::StorageType::FloatPixel, pixels_rgba.data());
	im.write(fname);
}

// Reads from the current FBO and saves texture as PNG file
void save_texture_as_png(GLuint textureID, std::string const & filename) {
	// 1) Bind the texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// 2) Query its dimensions
	GLint width=0, height=0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	if(width <= 0 || height <= 0) {
		throw std::runtime_error("Texture has invalid dimensions");
	}

	// 3) Read back as floats
	vector<float> floatData(width * height * 4);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, floatData.data());

	// 4) Convert floats to bytes [0..255]
	vector<unsigned char> byteData(width * height * 4);
	for(size_t i = 0; i < floatData.size(); ++i) {
		float c = std::min(1.0f, std::max(0.0f, floatData[i]));
		byteData[i] = static_cast<unsigned char>(c * 255.0f);
	}

	// 5) Flip vertically
	vector<unsigned char> flipped(width * height * 4);
	for(int y = 0; y < height; ++y) {
		const unsigned char* srcRow = &byteData[(height - 1 - y) * width * 4];
		unsigned char*       dstRow = &flipped[ y                * width * 4];
		memcpy(dstRow, srcRow, width * 4);
	}

	// 6) Create Magick::Image from raw RGBA bytes
	Magick::Image image(
		width, height,
		"RGBA"s,                 // pixel order
		Magick::CharPixel,      // each channel is an unsigned char
		flipped.data()          // pointer to your pixel buffer
	);

	// 7) Write out as PNG
	image.write(filename);
}

tuple<GLuint, GLuint, GLuint> create_quad() {
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

// Create a texture with deterministic test data and track maximum values.
TextureData create_data_texture(int width, int height) {
	TextureData result;
	result.width = width;
	result.height = height;
	
	// Initialize max values to minimum possible float
	for (int i = 0; i < 4; i++) {
		result.maxValues[i] = -std::numeric_limits<float>::max();
	}

	// create RGBA-32F texture to store 4 float values per pixel there
	
	// Generate texture ID
	glGenTextures(1, &result.textureId);
	glBindTexture(GL_TEXTURE_2D, result.textureId);
	
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Allocate storage for the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	
	// Generate test data on CPU
	vector<float> & data = result.data;
	data.resize(width * height * 4);
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int index = (y * width + x) * 4;
			
			// R: horizontal gradient (0 to 100)
			float const r = 0.0f;  //static_cast<float>(x) / (width - 1);
			data[index + 0] = r;
			result.maxValues[0] = std::max(result.maxValues[0], r);
			
			// G: vertical gradient (0 to 100)
			float const g = 0.0f;  //static_cast<float>(y) / (height - 1);
			data[index + 1] = g;
			result.maxValues[1] = std::max(result.maxValues[1], g);
			
			// B: checkerboard pattern (0 or 50)
			float const b = 0.0f; //((x + y) % 2 == 0) ? 0.5f : 0.0f;
			data[index + 2] = b;
			result.maxValues[2] = std::max(result.maxValues[2], b);
			
			// A: constant 1.0
			data[index + 3] = 1.0f;
			result.maxValues[3] = 1.0f;
		}
	}

	// Add a few specific high values to test max reduction
	if (width >= 10 && height >= 10) {
		// Set a specific maximum value for red at position (3,7)
		float const magick_r = 0.5f,
			magick_g = 0.75f,
			magick_b = 0.6f;

		int specialIndex = (7 * width + 3) * 4;
		data[specialIndex + 0] = magick_r;  // Higher than any other red value
		result.maxValues[0] = magick_r;
		
		// Set a specific maximum value for green at position (8,2)
		specialIndex = (2 * width + 8) * 4;
		data[specialIndex + 1] = magick_g;  // Higher than any other green value
		result.maxValues[1] = magick_g;
		
		// Set a specific maximum value for blue at position (5,5)
		specialIndex = (5 * width + 5) * 4;
		data[specialIndex + 2] = magick_b;  // Higher than any other blue value
		result.maxValues[2] = magick_b;

		// int const specialIndex = (2 * width + 8) * 4;
		// data[specialIndex + 0] = magick_r;
		// data[specialIndex + 1] = magick_g;  // Higher than any other green value
		// data[specialIndex + 2] = magick_b;
		// result.maxValues[0] = magick_r;
		// result.maxValues[1] = magick_g;
		// result.maxValues[2] = magick_b;

		// int const special_index_2 = (8 + 10 * width) * 4;
		// data[special_index_2] = magick_r+0.2f;
		// result.maxValues[0] = magick_r+0.2f;
	}
	
	// Upload data to the texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data.data());
	
	// Print maximum values for reference
	printf("Texture Maximum Values:\n");
	printf("  Red:   %.2f\n", result.maxValues[0]);
	printf("  Green: %.2f\n", result.maxValues[1]);
	printf("  Blue:  %.2f\n", result.maxValues[2]);
	printf("  Alpha: %.2f\n", result.maxValues[3]);
	
	return result;
}

TextureData load_from_image(string const & file_name) {
	TextureData result;

	// Load image using Magick++
	Magick::Image image;
	image.read(file_name);

	// Get image dimensions
	int const w = image.columns(), h = image.rows();

	// Resize data vector to hold RGBA values
	result.data.resize(w * h * 4);
	
	// Copy pixel data from Magick++ Image to our vector
	image.write(0, 0, w, h, "RGBA", Magick::StorageType::FloatPixel, result.data.data());

	// Set maximum values for each channel
	for (int i = 0; i < 4; i++) {
		result.maxValues[i] = -std::numeric_limits<float>::max();
	}

	// create OpenGL texture
	glGenTextures(1, &result.textureId);
	glBindTexture(GL_TEXTURE_2D, result.textureId);
	
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Allocate storage for the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
	
	// Upload data to the texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_FLOAT, result.data.data());
	
	result.width = w;
	result.height = h;

	return result;
}
