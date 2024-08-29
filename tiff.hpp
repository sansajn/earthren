/*! \file
TIFF image files manipulation support.
Dependencies: libtiff, Boost.GIL */

#pragma once
#include <memory>
#include <tuple>
#include <filesystem>
#include <cstddef>

/*! Loads striped TIFF file.
\return (data, width, height) triplet. */
std::tuple<std::unique_ptr<std::byte>, size_t, size_t> load_tiff(std::filesystem::path const & tiff_file);

struct tiff_data_desc {
	size_t width, 
		height;
	uint8_t bytes_per_sample, 
		samples_per_pixel;
};

// TODO: Can we garantee noexcept there? Should we do that?
inline bool is_grayscale(tiff_data_desc const & desc) {
	return desc.samples_per_pixel == 1;
}

inline bool is_rgb(tiff_data_desc const & desc) {
	return desc.samples_per_pixel == 3;
}

std::tuple<std::unique_ptr<std::byte>, tiff_data_desc> load_tiff_exp(
	std::filesystem::path const & tiff_file, bool flip = false);
