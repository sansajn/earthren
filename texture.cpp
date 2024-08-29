#include <spdlog/spdlog.h>
#include "tiff.hpp"
#include "texture.hpp"

using std::tuple;
using std::filesystem::path;

bool is_tiff(path const & fname) {
	return (fname.extension() == ".tiff") || (fname.extension() == ".tif");
}

tuple<GLuint, size_t, size_t> create_texture_16b(path const & fname) {
	auto const [image_data, image_desc] = [&fname](){
		if (is_tiff(fname))
			return load_tiff_exp(fname, true);
		else
			throw std::runtime_error("unsupported height map image format (only TIFF (*.tiff) suported");
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
			return load_tiff_exp(fname, true);
		else
			throw std::runtime_error("unsupported heightt map image format (only TIFF (*.tiff) suported");
	}();

	// TODO: we need to flip the data there

	spdlog::info("{} ({}x{}) image loaded", fname.c_str(), image_desc.width, image_desc.height);

	if (!is_rgb(image_desc))
		throw std::runtime_error{"only 8bit RGB textures are supported"};

	GLuint tbo;
	glGenTextures(1, &tbo);
	glBindTexture(GL_TEXTURE_2D, tbo);

	// note: 16bit I/UI textures can not be interpolated and so filter needs to be set to GL_NEAREST
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);  // or GL_NEAREST
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// clam to the edge outside of [0,1]^2 range
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// TODO:  this is inefficient, we want to get rid of that
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // tell OpenGL to change aligment from 4 to 1 to read RGB textures

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, image_desc.width, image_desc.height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_desc.width, image_desc.height, GL_RGB, GL_UNSIGNED_BYTE, image_data.get());

	glBindTexture(GL_TEXTURE_2D, 0);  // unbint texture

	return {tbo, image_desc.width, image_desc.height};
}

