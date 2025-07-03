// Sample for reduce fragment shader for OpenGL 3.2 ES.
#include <string>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <cassert>
#include <SDL.h>
#include <GLES3/gl32.h>
#include "shader.hpp"
#include "fs.hpp"

using std::cout, std::endl;
using std::filesystem::path;
using std::string, std::tuple;
using namespace std::string_literals;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;


// Function to draw the quad
void drawQuad(GLuint quadVAO) {
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

// Simple structure to hold texture and maximum values
struct TextureData {
    GLuint textureId;     // OpenGL texture ID
    float maxValues[4];   // Maximum value for each channel (R,G,B,A)
};

// Create a texture with deterministic test data and track maximum values.
TextureData createDataTexture(int width, int height) {
	TextureData result;
	
	// Initialize max values to minimum possible float
	for (int i = 0; i < 4; i++) {
		result.maxValues[i] = -std::numeric_limits<float>::max();
	}
	
	// Generate texture ID
	glGenTextures(1, &result.textureId);
	glBindTexture(GL_TEXTURE_2D, result.textureId);
	
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Allocate storage for the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	
	// Generate test data on CPU
	std::vector<float> data(width * height * 4);
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int index = (y * width + x) * 4;
			
			// R: horizontal gradient (0 to 100)
			float r = static_cast<float>(x) / (width - 1) * 100.0f;
			data[index + 0] = r;
			result.maxValues[0] = std::max(result.maxValues[0], r);
			
			// G: vertical gradient (0 to 100)
			float g = static_cast<float>(y) / (height - 1) * 100.0f;
			data[index + 1] = g;
			result.maxValues[1] = std::max(result.maxValues[1], g);
			
			// B: checkerboard pattern (0 or 50)
			float b = ((x + y) % 2 == 0) ? 50.0f : 0.0f;
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
		int specialIndex = (7 * width + 3) * 4;
		data[specialIndex + 0] = 150.0f;  // Higher than any other red value
		result.maxValues[0] = 150.0f;
		
		// Set a specific maximum value for green at position (8,2)
		specialIndex = (2 * width + 8) * 4;
		data[specialIndex + 1] = 175.0f;  // Higher than any other green value
		result.maxValues[1] = 175.0f;
		
		// Set a specific maximum value for blue at position (5,5)
		specialIndex = (5 * width + 5) * 4;
		data[specialIndex + 2] = 225.0f;  // Higher than any other blue value
		result.maxValues[2] = 225.0f;
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

int main(int argc, char * argv[]) {
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

	// we want to read texture
	size_t const texture_w = WIDTH, texture_h = HEIGHT;

	string const vs_src = read_file("minimal_vertex.glsl"),
		fs_src = read_file("reduce_fragment.glsl");
	assert(!vs_src.empty() && !fs_src.empty() && "Shader source files must not be empty");

	GLuint const reduce_shader_program = get_shader_program(vs_src.c_str(), fs_src.c_str());
	assert(reduce_shader_program != 0);



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


	// create input data texture

	TextureData tex = createDataTexture(initialWidth, initialHeight);  // GL_RGBA with GL_RGBA32F
	GLuint inputTexture = tex.textureId;
	 // Q: What is maximum value?

	// reduce

	// Save current OpenGL state
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
	glUniform2f(glGetUniformLocation(reduce_shader_program, "texelSize"), 
				1.0f / static_cast<float>(currentWidth), 
				1.0f / static_cast<float>(currentHeight));

	// Draw full-screen quad
	drawQuad(quadVAO);

	// Update dimensions
	currentWidth /= 2;
	currentHeight /= 2;

	// Ping-pong between FBOs until we reach a small size
	GLuint currentInputTexture = textureA;
	GLuint currentFBO = fboB;
	GLuint currentOutputTexture = textureB;

	while (currentWidth > 1 || currentHeight > 1) {
		// Bind output FBO
		glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
		glViewport(0, 0, std::max(1, currentWidth/2), std::max(1, currentHeight/2));
		
		// Set input texture and uniforms
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, currentInputTexture);
		glUniform1i(glGetUniformLocation(reduce_shader_program, "inputTexture"), 0);
		glUniform2f(glGetUniformLocation(reduce_shader_program, "texelSize"), 
						1.0f / static_cast<float>(currentWidth), 
						1.0f / static_cast<float>(currentHeight));
		
		// Draw full-screen quad
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		
		// Swap input and output for next pass
		currentInputTexture = currentOutputTexture;
		currentFBO = (currentFBO == fboA) ? fboB : fboA;
		currentOutputTexture = (currentOutputTexture == textureA) ? textureB : textureA;
		
		// Update dimensions
		currentWidth = std::max(1, currentWidth/2);
		currentHeight = std::max(1, currentHeight/2);
	}


	// Read back final result (1x1 texture)
	float result[4];
	glBindFramebuffer(GL_FRAMEBUFFER, currentFBO == fboA ? fboB : fboA);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, result);
	
	// Restore previous OpenGL state
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	// while (true) {
	// 	SDL_Event event;
	// 	if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
	// 		break;

	// 	drawQuad(quadVAO);  // render quad there

	// 	SDL_GL_SwapWindow(window);
	// }

	cout << "Reduction result: (" << result[0] << ", " << result[1] << ", "
		<< result[2] << ", " << result[3] << ")" << endl;

	glDeleteVertexArrays(1, &quadVAO);
	glDeleteFramebuffers(1, &fboA);
	glDeleteFramebuffers(1, &fboB);
	glDeleteTextures(1, &textureA);
	glDeleteTextures(1, &textureB);
	glDeleteProgram(reduce_shader_program);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
