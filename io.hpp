/*! \file
Input/output helpers. */

/* TODO: we introduced input.hpp for input handling functions (mouse keyboard), but that
is kind of collidate with io.hpp. Find a bethed harbor for `read_file` function. */

#pragma once
#include <string>
#include <filesystem>

std::string read_file(std::filesystem::path const & fname);
