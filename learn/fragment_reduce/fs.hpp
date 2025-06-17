/*! \file
Filesystem helpers. */
#pragma once
#include <filesystem>
#include <string_view>
#include <string>
#include <regex>
#include <ranges>

namespace fs = std::filesystem;

//! Read file content.
std::string read_file(fs::path const & fname);

/*! Find files in specified directory (not recursive).
\return Range (view) of files satisfy criteria.

\code
for (fs::directory_entry const & de : find_files(tile_directory, _elevation_tile_prefix)) {
	path const & p = de.path();  // transform to path
	// ...
}
\endcode */
inline auto find_files(fs::path const & path, std::string_view pattern) {
	using std::ranges::subrange;
	using std::ranges::views::filter;

	return subrange{fs::directory_iterator{path}, fs::directory_iterator{}}
		|filter([pattern](fs::directory_entry const & x){
			return fs::is_regular_file(x.path())
				&& x.path().filename().string().starts_with(pattern);
			});
}

inline auto find_files(fs::path const & path, std::regex const & pattern) {
	using std::ranges::subrange;
	using std::ranges::views::filter;

	return subrange{fs::directory_iterator{path}, fs::directory_iterator{}}
		|filter([&pattern](fs::directory_entry const & x){
			return fs::is_regular_file(x.path())
				&& regex_match(x.path().filename().string(), pattern);
			});
}
