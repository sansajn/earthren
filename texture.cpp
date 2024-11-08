#include <spdlog/spdlog.h>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "tiff.hpp"
#include "texture.hpp"
#include "color.hpp"

using std::tuple;
using std::filesystem::path;
using glm::vec4, glm::value_ptr;

bool is_tiff(path const & fname) {
	return (fname.extension() == ".tiff") || (fname.extension() == ".tif");
}

tuple<GLuint, size_t, size_t> create_texture_16b(path const & fname) {
	auto const [image_data, image_desc] = [&fname](){
		if (is_tiff(fname))
			return load_tiff_desc(fname, true);
		else
			throw std::runtime_error("unsupported texture format (only TIFF (*.tif) suported");
	}();

	spdlog::info("{} ({}x{}) image loaded", fname.c_str(), image_desc.width, image_desc.height);

	GLuint tbo;
	glGenTextures(1, &tbo);
	glBindTexture(GL_TEXTURE_2D, tbo);

	// note: 16bit I/UI textures can not be interpolated and so filter needs to be set to GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);  // or GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// clam to the edge outside of [0,1]^2 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (is_grayscale(image_desc)) {
		// TODO: elevation data seems to be INTEGER not UNSIGNED INTEGET
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R16UI /*GL_R16I*/, image_desc.width, image_desc.height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_desc.width, image_desc.height, GL_RED_INTEGER, GL_UNSIGNED_SHORT /*GL_SHORT*/, image_data.get());
	}
	else if (is_rgb(image_desc)) {
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16UI, image_desc.width, image_desc.height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_desc.width, image_desc.height, GL_RGB_INTEGER, GL_UNSIGNED_SHORT, image_data.get());
	}
	else  // TODO: provide info about channels
		throw std::runtime_error{"unsupported number of channels (?), only GRAY and RGB images are supported"};

	glBindTexture(GL_TEXTURE_2D, 0);  // unbint texture

	return {tbo, image_desc.width, image_desc.height};
}


tuple<GLuint, size_t, size_t> create_texture_8b(path const & fname) {
	// TODO: implement ...
	auto const [image_data, image_desc] = [&fname](){
		if (is_tiff(fname))
			return load_tiff_desc(fname, true);
		else
			throw std::runtime_error("unsupported texture format (only TIFF (*.tif) suported");
	}();

	// TODO: we need to flip the data there

	spdlog::info("{} ({}x{}) image loaded", fname.c_str(), image_desc.width, image_desc.height);

	if (!is_rgb(image_desc))
		throw std::runtime_error{"only 8bit RGB textures are supported"};

	GLuint tbo;
	glGenTextures(1, &tbo);
	glBindTexture(GL_TEXTURE_2D, tbo);

	// TODO: this is inefficient, we want to get rid of that ALIGNMENT
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // tell OpenGL to change aligment from 4 to 1 to read RGB textures

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, image_desc.width, image_desc.height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_desc.width, image_desc.height, GL_RGB, GL_UNSIGNED_BYTE, image_data.get());

	// in case of wrapping, we want to be able to easilly see it in scene as red color
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, value_ptr(vec4{rgb::red, 1.0}));

	// set texture filtering (interpolation) mode (if GL_LINEAR is set, we can see artifacts in a scene)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);  // unbint texture

	return {tbo, image_desc.width, image_desc.height};
}

