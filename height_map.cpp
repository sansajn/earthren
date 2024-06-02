// OpenGL ES 3.2, terrain with heights from height map texture
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
#include <Magick++.h>
#include <tiffio.hxx>
#include "glmprint.hpp"

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

// TODO: update to calculate normals, we only need position attributes there
GLchar const * vertex_shader_source = R"(
#version 320 es
in vec3 position;  // TODO: this is now not expected to be (0,0), (1,1) square
out vec3 color;  // output vertex color

uniform mat4 local_to_screen;
uniform vec3 light_direction;  // TODO: what should be default value
uniform sampler2D heights;
uniform float scale;  // scale to map geometry with height map (it should be size of height map in pixels) (=1016)
uniform float height_scale;  // constant to scale heights (=1/1000)

float rand(vec2 co) {  // random function generator
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
	// sample height map to calculate vertex normal
	float hxl = textureOffset(heights, position.xy*scale, ivec2(-1, 0)).w;
	float hxr = textureOffset(heights, position.xy*scale, ivec2(1, 0)).w;
	float hyl = textureOffset(heights, position.xy*scale, ivec2(0, -1)).w;
	float hyr = textureOffset(heights, position.xy*scale, ivec2(0, 1)).w;

	// TODO: need to understand
	vec3 u = normalize(vec3(0.05, 0.0, hxr-hxl));
	vec3 v = normalize(vec3(0.0, 0.05, hyr-hyl));
	vec3 n = cross(u, v);

	// compute diffuse lighting
	float diffuse = dot(n, light_direction);

	color = vec3(diffuse);

	// calculate vertex geometry
	// float h = texture(heights, position.xy*scale).w;
	// vec3 pos = vec3(position.xy, 1.0 /*h*height_scale*/);
	vec3 pos = vec3(position.xy, rand(position.xy));

	gl_Position = local_to_screen * vec4(pos, 1.0);
})";

GLchar const * fragment_shader_source = R"(
#version 320 es
precision mediump float;
in vec3 color;
out vec4 frag_color;
void main() {
	frag_color = vec4(color, 1.0);
})";

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source);

//! Creates OpenGL texture from image \c fname file and returns texture ID.
GLuint create_texture(std::string const & fname);

tuple<GLuint, GLuint, GLuint, unsigned> create_quad_mesh(GLint position_loc);

/*! Creates quad mesh on GPU with a size=1.
\return (vao, vbo, vertex_count) tuple. */
tuple<GLuint, GLuint, unsigned> create_mesh(GLint vertices_loc, GLint uv_loc);

tuple<unique_ptr<byte>, size_t, size_t> load_tiff(path const & tiff_file);

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
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	GLuint const shader_program = get_shader_program(vertex_shader_source, fragment_shader_source);
	
	GLint const position_loc = glGetAttribLocation(shader_program, "position");
	GLint const local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen");
	GLint const light_direction_loc = glGetUniformLocation(shader_program, "light_direction");
	GLint const heights_loc = glGetUniformLocation(shader_program, "heights");
	GLint const scale_loc = glGetUniformLocation(shader_program, "scale");
	GLint const height_scale_loc = glGetUniformLocation(shader_program, "height_scale");
	
	glUseProgram(shader_program);

	glUniform3f(light_direction_loc, 0.86f, 0.14f, 0.49f);  // light direction from the sample
	glUniform1f(scale_loc, 1016);  // 1582x1016  TODO: we want to scale in both axis
	glUniform1f(height_scale_loc, 1.0f/100.0f/*535.0f*/);  // max value in height map is 535 based on gdalinfo

	// generate texture
	mat4 const P = perspective(radians(60.f), WIDTH/(float)HEIGHT, 0.01f, 1000.f);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	constexpr float quad_size = 1.0f;
	auto [vao, vbo, ibo, element_count] = create_quad_mesh(position_loc);

	// vector<GLuint> tiles = read_tiles();  // TODO: we only need first tile
	GLuint height_map = create_texture("data/dem.tiff");  // TODO: make configurable

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
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  // clear buffer

		glBindVertexArray(vao);

		// GLuint const & tile = tiles[0];  // TODO: we do not need list of tiles, just a tile
		glUniform1i(heights_loc, 0);  // set sampler s to use texture unit 0
		glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
		glBindTexture(GL_TEXTURE_2D, height_map);  // bind a texture to active texture unit (0)

		float const model_scale = 2.0f;
		vec2 model_pos = vec2{-quad_size/2.0f, -quad_size/2.0f} * model_scale;
		mat4 M = scale(translate(mat4{1}, vec3{model_pos,0}), vec3{model_scale, model_scale, 1});  // T*S
		mat4 local_to_screen = P*V*M;
		glUniformMatrix4fv(local_to_screen_loc, 1, GL_FALSE, value_ptr(local_to_screen));

		glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);  // unbind VAO
		SDL_GL_SwapWindow(window);
	}
	
	// for(auto tile : tiles)  // delete textures
		// glDeleteTextures(1, &tile);

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

//! \return (data, width, height)
tuple<unique_ptr<byte>, size_t, size_t> load_tiff(path const & tiff_file) {  // TODO: height_map
	// TODO: would it be possible to replace this with ImageMagick?

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
	// TODO: we expect SAMPLEFORMAT_UINT (2)

	uint16_t samples_per_pixel = 0;
	TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
	// TODO: we expect 1

	// read strip by strip and store it into whole image
	unsigned const image_size = image_w*image_h*sample_format;
	unique_ptr<byte> image_data{new byte[image_size]};
	memset(image_data.get(), 0xff, image_size);
	for (tstrip_t strip = 0; strip < strip_count; ++strip) {
		unsigned offset = strip*(strip_size/2);
		uint16_t * buf = reinterpret_cast<uint16_t *>(image_data.get()) + offset;
		// cout << "buf=" << std::hex << uint64_t(buf) << ", offset=" << offset << '\n';
		tmsize_t ret = TIFFReadEncodedStrip(tiff, strip, buf, (tsize_t)-1);
		assert(ret != -1 && ret > 0);
	}

	TIFFClose(tiff);
	fin.close();

	return {std::move(image_data), (size_t)image_w, (size_t)image_h};
}

GLuint create_texture(std::string const & fname) {
	// Magick::Image im{fname};
	// im.flip();
	// Magick::Blob imblob;
	// im.write(&imblob, "RGBA");  // load image as rgba array

	auto [image_data, width, height] = load_tiff(fname);

	GLuint tbo;
	glGenTextures(1, &tbo);
	glBindTexture(GL_TEXTURE_2D, tbo);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R16UI, width, height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_SHORT, image_data.get());
	glBindTexture(GL_TEXTURE_2D, 0);  // unbint texture

	return tbo;
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
	auto [vertices, indices] = make_quad(10, 10);  // TODO: we need to figure out proper dimensions there

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
