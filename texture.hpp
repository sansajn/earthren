/*! /file
OpenGL textures support. */
#pragma once
#include <tuple>
#include <filesystem>
#include <GLES3/gl32.h>

/*! Creates 16bit INT GRAY or RGB OpenGL texture from TIFF image \c fname file and returns
OpenGL texture ID.
\return (TBO, width, height) triplet. */
std::tuple<GLuint, size_t, size_t> create_texture_16b(std::filesystem::path const & fname);
std::tuple<GLuint, size_t, size_t> create_texture_8b(std::filesystem::path const & fname);
