#include <fstream>
#include <limits>
#include <cassert>
#include <cstring>
#include <boost/gil.hpp>
#include <tiffio.hxx>
#include "tiff.hpp"

using std::tuple, std::byte,
	std::unique_ptr,
	std::filesystem::path,
	std::ifstream,
	std::move, std::swap,
	std::numeric_limits;

tuple<unique_ptr<byte>, size_t, size_t> load_tiff(path const & tiff_file) {
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
	// TODO: we expect SAMPLEFORMAT_UINT or SAMPLEFORMAT_INT

	uint16_t samples_per_pixel = 0;
	TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
	// TODO: we expect 1

	// read strip by strip and store it into whole image
	unsigned const image_size = image_w*image_h*(bits_per_sample/8)*samples_per_pixel;
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

// TODO: make proper doxygen documentation
template <typename PixelType>  // e.g. rgb8_pixel_t, gray8_pixel_t, ...
unique_ptr<byte> flip_copy(byte const * pixels, size_t w, size_t h, size_t image_size) {
	using boost::gil::interleaved_view,
		boost::gil::flipped_up_down_view,
		boost::gil::copy_pixels,
		boost::gil::type_from_x_iterator;

	using view_t = typename type_from_x_iterator<PixelType *>::view_t;

	view_t pixel_view = interleaved_view(w, h, (PixelType *)pixels, w*sizeof(PixelType));
	view_t flipped_view = flipped_up_down_view(pixel_view);

	unique_ptr<byte> flipped_pixels{new byte[image_size]};
	copy_pixels(flipped_view, interleaved_view(w, h, (PixelType *)flipped_pixels.get(), w*sizeof(PixelType)));
	return flipped_pixels;
}

tuple<unique_ptr<byte>, tiff_data_desc> load_tiff_desc(path const & tiff_file, bool flip) {
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
	memset(image_data.get(), 0xff, image_size);   // (optional) clear buffer
	for (tstrip_t strip = 0; strip < strip_count; ++strip) {
		unsigned const offset = strip*strip_size;
		uint8_t * buf = reinterpret_cast<uint8_t *>(image_data.get()) + offset;
		tmsize_t ret = TIFFReadEncodedStrip(tiff, strip, buf, (tsize_t)-1);
		assert(ret != -1 && ret > 0);
	}

	TIFFClose(tiff);
	fin.close();

	assert(bits_per_sample/8 <= numeric_limits<uint8_t>::max());
	assert(samples_per_pixel <= numeric_limits<uint8_t>::max());

	tiff_data_desc const desc = {
		.width=image_w,
		.height=image_h,
		.bytes_per_sample=static_cast<uint8_t>(bits_per_sample/8),
		.samples_per_pixel=static_cast<uint8_t>(samples_per_pixel)
	};

	if (flip) {
		// TODO: thiis should be handle by a factory mmethod
		if (bits_per_sample == 8 && samples_per_pixel == 1) {
			unique_ptr<byte> flipped_data = flip_copy<boost::gil::gray8_pixel_t>(image_data.get(), image_w, image_h, image_size);
			swap(flipped_data, image_data);
		}
		else if (bits_per_sample == 8 && samples_per_pixel == 3) {
			unique_ptr<byte> flipped_data = flip_copy<boost::gil::rgb8_pixel_t>(image_data.get(), image_w, image_h, image_size);
			swap(flipped_data, image_data);
		}
		else if (bits_per_sample == 16 && samples_per_pixel == 1) {
			unique_ptr<byte> flipped_data = flip_copy<boost::gil::gray16_pixel_t>(image_data.get(), image_w, image_h, image_size);
			swap(flipped_data, image_data);
		}
		else if (bits_per_sample == 16 && samples_per_pixel == 3) {
			unique_ptr<byte> flipped_data = flip_copy<boost::gil::rgb16_pixel_t>(image_data.get(), image_w, image_h, image_size);
			swap(flipped_data, image_data);
		}
		else
			throw std::runtime_error{"flip algorithm only implemented for 8/16bit RGB/GRAY images"};  // TODO: is there any standard not_yet_implemented exception?
	}

	return {move(image_data), desc};
}
