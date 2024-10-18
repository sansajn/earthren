#include <utility>
#include <vector>

template <typename T>
void set_uniform(int location, T const & v);

template <typename T>
void set_uniform(int location, T const * a, int n);

template <typename T>
void set_uniform(int location, std::vector<T> const & arr) {
	set_uniform(location, arr.data(), arr.size());
}

//! for array where pair::first is array pointer and pair::second is number of array elements
template <typename T>
void set_uniform(int location, std::pair<T *, int> const & a) {
	set_uniform(location, a.first, a.second);
}
