/* Sample to generate 16bit sinxy height map so we can avoid using PNG files in some samples and
directly generate heightmap. */
#include <vector>
#include <cstdint>
#include <limits>
#include <iostream>
#include <cmath>
#include <Magick++.h>
using std::vector;
using std::numeric_limits;
using std::sin;
using std::cout;
using namespace Magick;

const double PI = 3.14159265359;

size_t const image_w = 512,
	image_h = 512;

uint16_t sinxy(double x, double y) {
	return static_cast<uint16_t>(sin(x)*sin(y)*numeric_limits<uint16_t>::max());
}

vector<uint16_t> generate_sinxy_image(size_t w, size_t h) {
	vector<uint16_t> pixels(w*h);
	for (size_t r = 0; r < h; ++r) {
		for (size_t c = 0; c < w; ++c) {
			double x = c * PI/(w-1);
			double y = r * PI/(h-1);
			pixels[r*w + c] = sinxy(x,y);
		}
	}
	return pixels;
}

void test_save_heightmap() {
	constexpr int image_w = 512,
		image_h = 512;
	vector<uint16_t> const pixels = generate_sinxy_image(image_w, image_h);  // TODO: make width/height configurable

	try {
		// Create an Image object
		Image image(Geometry(image_w, image_h), "gray(0)");
		image.depth(16);
		image.type(GrayscaleType);
		image.read(image_w, image_h, "I", ShortPixel, pixels.data());
		image.write("output.png");
	} catch (Exception &error_) {
		std::cout << "Caught exception: " << error_.what() << std::endl;
	}
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
	vector<uint16_t> const pixels = generate_sinxy_image(image_w, image_h);
	return 0;
}
