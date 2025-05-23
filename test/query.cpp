// this iis to inntroduce point queries into quad tree
#include <algorithm>
#include <string>
#include <array>
#include <memory>
#include <vector>
#include <glm/vec2.hpp>
#include <catch2/catch_test_macros.hpp>

// for geometry stuff
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/geometry.hpp>
#include "geometry/box2.hpp"
namespace bg = boost::geometry;

using std::string, std::to_string;
using std::array, std::unique_ptr, std::make_unique, std::vector;
using std::ranges::find_if;
using glm::vec2;

namespace {

using geom::box2;

struct qtree_node {
	using value_type = string;
	string label;
	box2 bounds;
	array<unique_ptr<qtree_node>, 4> children;  // 0:NW 1:NE 2:SW 3:SE

	qtree_node(box2 const & bounds)
		: bounds{bounds}, children{nullptr, nullptr, nullptr, nullptr}
	{}

	bool is_leaf() const {return children[0] == nullptr && children[1] == nullptr && children[2] == nullptr && children[3] == nullptr;}

	void split() {
		float const w2 = geom::width(bounds),
			w = w2/2.0f,
			h2 = geom::height(bounds),
			h = h2/2.0f;
		vec2 const a = bounds.min_corner();
		children[0] = make_unique<qtree_node>(box2{a+vec2{0, h}, a+vec2{w, h2}});  // NW
		children[1] = make_unique<qtree_node>(box2{a+vec2{w, h}, a+vec2{w2, h2}});  // NE
		children[2] = make_unique<qtree_node>(box2{a, a+vec2{w, h}});  // SW
		children[3] = make_unique<qtree_node>(box2{a+vec2{w, 0}, a+vec2{w2, h}});  // SE
	}
};  // qtree_node

void split_node(qtree_node & node, array<string, 4> const & labels) {
	node.split();
	for (size_t idx = 0; idx < 4; ++idx)
		node.children[idx]->label = labels[idx];
}

qtree_node const * range_query(qtree_node const & root, vec2 pt) {  // NOTE: optional doesnt make a sence there we can't wrap reference
	if (!bg::intersects(root.bounds, pt))
		return nullptr;

	qtree_node const * n = &root;
	while (!n->is_leaf()) {
		auto it = find_if(n->children, [pt](auto const & child){
			return bg::intersects(child->bounds, pt);
		});

		assert(it != end(n->children) && "there must be some inteersection there");

		n = it->get();
	}
	return n;
}

}  // namespace

TEST_CASE("check intersects function behaviour", "[qtree][range-query]") {
	box2 const b = {vec2{0,0}, vec2{1,1}};
	vec2 const bottom_left = {0,0},
		bottom_right = {1,0},
		top_right = {1,1},
		top_left = {0,1};

	REQUIRE(bg::intersects(b, bottom_left));
	REQUIRE(bg::intersects(b, bottom_right));
	REQUIRE(bg::intersects(b, top_right));
	REQUIRE(bg::intersects(b, top_left));
}

TEST_CASE("points on boundaries intersects two quads", "[qtree][range-query]") {
	box2 const a = {vec2{0,0},  vec2{0.5, 0.5}},
		b = {vec2{0.5, 0}, vec2{1,1}};
	vec2 const pt = {0.5, 0.25};

	REQUIRE(bg::intersects(a, pt));
	REQUIRE(bg::intersects(b, pt));
}

TEST_CASE("we can raange-query quad tree with point", "[qtree][range-quary][x]") {
	// create test tree
	qtree_node root{box2{vec2{0,0}, vec2{1,1}}};
	root.label = "r";
	split_node(root, {"a", "b", "c", "d"});

	qtree_node & c = *root.children[2];
	split_node(c, {"e", "f", "g", "h"});

	// test range_query
	vector<vec2> const test_points = {
		{0.4f, 0.8f},  // for a
		{0.7f, 0.6f},  // for b
		{0.9f, 0.1f},  // for d
		{0.1f, 0.3f},  // for e
		{0.4f, 0.3f},  // for f
		{0.1f, 0.1f},  // for g
		{0.4f, 0.1f},  // for h
		{2.0f, 2.0f},  // outside tree
		// edge points
		{0, 0},  // g
		{1, 0},  // d
		{1, 1},  // b
		{0, 1},  // a
		{-1, 0},  // outside
		// inner edge points
		{0.5, 0.4}, // f&d=f
		{0.7, 0.5},  // b&d=b
		{0.5, 0.7},  // a&b=a
		{0.2, 0.5},  // a&e=a
		{0.5f, 0.5f}  // a
	};

	string const expected_nodes = "abdefgh_gdba_fbaaa";

	assert(size(test_points) <= size(expected_nodes));

	for (size_t idx = 0; idx < size(test_points); ++idx) {
		if (qtree_node const * q = range_query(root, test_points[idx]); q) {
			CAPTURE(idx);
			REQUIRE(q->label == string{expected_nodes[idx]});
		}
		else
			REQUIRE(string{expected_nodes[idx]} == "_");
	}
}
