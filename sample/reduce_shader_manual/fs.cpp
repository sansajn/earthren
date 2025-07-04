#include <fstream>
#include <sstream>
#include <stdexcept>
#include <fmt/format.h>
#include "fs.hpp"

using std::string,
	std::ifstream, std::stringstream,
	std::invalid_argument,
	std::filesystem::path;

using fmt::format;

string read_file(path const & fname) {
	ifstream in(fname);
	if (!in.is_open())
		throw invalid_argument{format("can't open '{}' file", fname.c_str())};

	stringstream ss;
	ss << in.rdbuf();
	in.close();
	return ss.str();
}
