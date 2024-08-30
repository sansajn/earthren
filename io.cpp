#include <fstream>
#include <sstream>
#include <exception>
#include <fmt/format.h>
#include "io.hpp"

using std::string,
	std::ifstream, std::stringstream,
	std::runtime_error,
	std::filesystem::path;

using fmt::format;

string read_file(path const & fname) {
	ifstream in(fname);
	if (!in.is_open())
		throw runtime_error{format("can't open '{}' file", fname.c_str())};

	stringstream ss;
	ss << in.rdbuf();
	in.close();
	return ss.str();
}
