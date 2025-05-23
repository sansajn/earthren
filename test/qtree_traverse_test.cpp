// to develop traverse algorithm from leaf_view
#include <algorithm>
#include <functional>
#include <stack>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <ranges>
#include <iostream>
#include "qtree_traverse.hpp"
#include <catch2/catch_test_macros.hpp>

using std::ranges::all_of;
using std::string, std::array, std::vector;
using std::make_unique, std::unique_ptr;
using std::cout;
using namespace qtree;

namespace {

struct qtree_node {
	using value_type = string;
	string label;
	array<unique_ptr<qtree_node>, 4> children;
	qtree_node(string label) : label{label}, children{nullptr, nullptr, nullptr, nullptr} {}
	bool is_leaf() const {return children[0] == nullptr && children[1] == nullptr && children[2] == nullptr && children[3] == nullptr;}
};

}  // namespace

namespace qtree {  // for a custom is_leaf_parent implementation we need qtree namespace

template <>
bool is_leaf_parent(qtree_node const & node) {
	// one of child is leaf
	return (node.children[0] && node.children[0]->is_leaf())
		|| (node.children[1] && node.children[1]->is_leaf())
		|| (node.children[2] && node.children[2]->is_leaf())
		|| (node.children[3] && node.children[3]->is_leaf());
}

}  // qtree


namespace {

qtree_node create_test_tree() {
	qtree_node root{"r"};
	root.children[0] = make_unique<qtree_node>("a");
	root.children[1] = make_unique<qtree_node>("b");
	root.children[2] = make_unique<qtree_node>("c");
	root.children[3] = make_unique<qtree_node>("d");

	qtree_node & c_root = *root.children[2];
	c_root.children[0] = make_unique<qtree_node>("e");
	c_root.children[1] = make_unique<qtree_node>("f");
	c_root.children[2] = make_unique<qtree_node>("g");
	c_root.children[3] = make_unique<qtree_node>("h");

	return root;
}

}  // namespace


TEST_CASE("we can traverse whole quad tree by handwritten DFS algorithm", "[qtree][traverse]") {
	qtree_node root = create_test_tree();

	vector<string> leaves;

	// hand written DFS traverse
	std::stack<qtree_node *> node_stack;
	node_stack.push(&root);

	while (!node_stack.empty()) {
		qtree_node * n = node_stack.top();;
		node_stack.pop();
		leaves.push_back(n->label);

		for (auto const & ch : n->children)
			if (ch)
				node_stack.push(ch.get());
	}

	string const expected_leaves = "rabcdefgh";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}

TEST_CASE("we can travrse root quad tree", "[qtree][dfs_traverse]") {
	qtree_node root{"r"};

	dfs_traverse node_range{root, &qtree_node::label};

	vector<string> leaves;
	for (string const & node : node_range)
		leaves.push_back(node);

	string const expected_leaves = "r";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}

TEST_CASE("we can travrse whole quad tree", "[qtree][dfs_traverse]") {
	// construct sample tree
	qtree_node root = create_test_tree();

	dfs_traverse node_range{root, &qtree_node::label};

	vector<string> leaves;
	for (string const & node : node_range)
		leaves.push_back(node);

	string const expected_leaves = "rabcdefgh";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}

TEST_CASE("we can travrse via quad tree leaves (handwritten)", "[qtree][dfs_traverse]") {
	// construct sample tree
	qtree_node root = create_test_tree();

	dfs_traverse node_range{root};

	vector<string> leaves;
	for (qtree_node const & node : node_range) {
		if (node.is_leaf())
			leaves.push_back(node.label);
	}

	string const expected_leaves = "abdefgh";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}

// TODO: we can't use our view implementation together with fileter, this not compile, no idea why
// TEST_CASE("we can travrse via quad tree leaves (fileter)", "[dfs_traverse]") {
// 	// construct sample tree
// 	qtree_node root = create_test_tree();

// 	dfs_traverse node_range{root};

// 	auto leaf_range = node_range|std::views::filter(is_leaf);

// 	vector<string> leaves;
// 	for (qtree_node const & node : leaf_range) {
// 		leaves.push_back(node.label);
// 	}

// 	string const expected_leaves = "abdefgh";

// 	REQUIRE(size(expected_leaves) == size(leaves));
// 	REQUIRE(
// 		all_of(leaves, [&expected_leaves](string const & label){
// 			return expected_leaves.find(label) != string::npos;
// 		})
// 	);
// }


TEST_CASE("we can travrse via quad tree leaf parents (handwritten)", "[qtree][dfs_traverse]") {
	// construct sample tree
	qtree_node root = create_test_tree();

	dfs_traverse node_range{root};

	vector<string> leaves;
	for (qtree_node const & node : node_range) {
		if (is_leaf_parent(node))
			leaves.push_back(node.label);
	}

	string const expected_leaves = "rc";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}


TEST_CASE("we can travrse root quad tree leaf", "[qtree][dfs_traverse_if]") {
	qtree_node root{"r"};

	dfs_traverse_if node_range{root, is_leaf<qtree_node>, &qtree_node::label};

	vector<string> leaves;
	for (string const & node : node_range)
		leaves.push_back(node);

	string const expected_leaves = "r";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}


TEST_CASE("we can travrse quad tree leaf nodes with dfs_traverse_if", "[qtree][dfs_traverse_if]") {
	qtree_node root = create_test_tree();

	SECTION("leaf parent nodes") {
		dfs_traverse_if node_range{root, is_leaf_parent<qtree_node>, &qtree_node::label};

		vector<string> leaves;
		for (string const & node : node_range)
			leaves.push_back(node);

		string const expected_leaves = "rc";

		REQUIRE(size(expected_leaves) == size(leaves));
		REQUIRE(
			all_of(leaves, [&expected_leaves](string const & label){
				return expected_leaves.find(label) != string::npos;
			})
		);
	}

	SECTION("leaf nodes") {
		dfs_traverse_if node_range{root, is_leaf<qtree_node>, &qtree_node::label};

		vector<string> leaves;
		for (string const & node : node_range)
			leaves.push_back(node);

		string const expected_leaves = "abdefgh";

		REQUIRE(size(expected_leaves) == size(leaves));
		REQUIRE(
			all_of(leaves, [&expected_leaves](string const & label){
				return expected_leaves.find(label) != string::npos;
			})
		);
	}
}

TEST_CASE("exact traverse view type looks this way", "[qtree][dfs_traverse_if]") {
	qtree_node root = create_test_tree();

	dfs_traverse_if<  // same as dfs_traverse_if r1{root, is_leaf, &qtree_node::label};
		qtree_node,
		decltype(&is_leaf<qtree_node>),
		decltype(&qtree_node::label)> node_range{root, is_leaf, &qtree_node::label};

	vector<string> leaves;
	for (string const & node : node_range)
		leaves.push_back(node);

	string const expected_leaves = "abdefgh";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}

TEST_CASE("we can traverse leaves with leaf_view", "[qtree][leaf_vew]") {
	qtree_node root = create_test_tree();

	leaf_view leaf_range{root, &qtree_node::label};

	vector<string> leaves;
	for (string const & leaf : leaf_range)
		leaves.push_back(leaf);

	string const expected_leaves = "abdefgh";

	REQUIRE(size(expected_leaves) == size(leaves));
	REQUIRE(
		all_of(leaves, [&expected_leaves](string const & label){
			return expected_leaves.find(label) != string::npos;
		})
	);
}

TEST_CASE("we can use copy algorithm with leaf_view", "[qtree][leaf_view]") {
	// TODO: implement
}
