// Catch2 (v2) hello sample.
#include <vector>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("vectors can be sized and resized", "[tag_a][tag_b]") {
	std::vector<int> v(5);
	REQUIRE(v.size() == 5);
	REQUIRE(v.capacity() >= 5);

	SECTION("resizing bigger changes size and capacity") 	{
		v.resize(10);
		REQUIRE(v.size() == 10);
		REQUIRE(v.capacity() >= 10);
	}

	SECTION("resizing smaller changes size but not capacity") {
		v.resize(0);
		REQUIRE(v.size() == 0);
		REQUIRE(v.capacity() >= 5);
	}

	SECTION("reserving bigger changes capcaity but not size") {
		v.reserve(10);
		REQUIRE(v.size() == 5);
		REQUIRE(v.capacity() >= 10);
	}

	SECTION("reserving smaller does not chnge size or capacity") {
		v.reserve(0);
		REQUIRE(v.size() == 5);
		REQUIRE(v.capacity() >= 5);
	}
}
