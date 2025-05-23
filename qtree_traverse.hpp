/*! \file
Implements quad tree traverse algorithms. */
#pragma once
#include <algorithm>
#include <functional>
#include <stack>
#include <memory>
#include <string>
#include <vector>
#include <ranges>

//! Quad tree stuff namespace.
namespace qtree {

/*! Returns true if node is a parent of leaf node.
To provide custom implementation tailored to your type we need to enclose inside
qtree namespace this way
\code
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
\endcode

for more details see `qtree_traverse_test.cpp`. */
template <typename Node>
bool is_leaf_parent(Node const & node) {
	return node.is_leaf_parent();
}

template <typename Node>
bool is_leaf(Node const & node) {
	return node.is_leaf();
}

/*! Quadtree depth-first search (DSF) traverse view implementation.
With a default Proj projection (=std::itentity) we can view `Node &` type elements. To view e.g.
`terrain &` type elements use `proj=&terrain_quad::trn` projection. Projection proj is
used in `*` and `->` operator implementations.

There is terrain_quad as an Node implementation there.

During the iteratiion you are not allowed to modify a tree (insert, remove nodes).

\code
dsf_traverse{root};  // view `Node &&` element
dsf_traverse{root, &terrain_quad::trn};  // view `terrain::trn` type member elements (e.g. `terrain &`)
\endcode */
template <typename Node, typename Proj = std::identity>
class dfs_traverse : public std::ranges::view_interface<dfs_traverse<Node, Proj>> {
 public:
	explicit dfs_traverse(Node & root, Proj proj = {}) : _root(&root), _proj{std::move(proj)} {}

	struct iterator {
		using iterator_category = std::input_iterator_tag;

		std::stack<Node *> nodes;

		explicit iterator(Node * root, dfs_traverse * parent) : _parent{parent} {
			if (root) {
				nodes.push(root);
			}
		}

		/*! Dereference leaf node.
		\returns leaf node reference of `Node &` type by defaul. Return value and it's type can be changed by a view projection (=`Proj=std::indentity`).
		\code
		leaf_view{root, &terrain_quad::trn};  // view `terrain::trn` type member elements (e.g. `terrain &`)
		for (terrain const & trn : leaf_view) ...
		\endcode */
		decltype(auto) operator*() const {  // decltype(auto) because we want to return exact type returned by invoke in that case auto drops references
			return std::invoke(_parent->_proj, std::ref(*nodes.top()));
		}

		/*! Access leaf node.
		\returns an address of leaf node of type `Node *` */
		decltype(auto) operator->() const {
			return std::addressof(std::invoke(_parent->_proj, nodes.top()));
		}

		iterator & operator++() {
			Node * current = nodes.top();
			nodes.pop();
			for (auto const & child : current->children) {
				if (child)
					nodes.push(child.get());
			}
			return * this;
		}

		bool operator!=([[maybe_unused]] iterator const & other) const {
			return !nodes.empty();
		}

	private:
		dfs_traverse * _parent;
	};  // iterator

	auto begin() { return iterator(_root, this); }
	auto end() { return iterator(nullptr, this); }

	auto begin() const requires std::is_const_v<Node> { return iterator(_root, this); }
	auto end() const requires std::is_const_v<Node> { return iterator(nullptr, this); }

 private:
	Node * _root;
	Proj _proj;
};  // dfs_traverse

/*! Quadtree depth-first search (DSF) traverse view implementation with a fileter like unary predicate.

With UnaryPredicate predicate we can customize wwhat quad tree nodees are iterated.
With is_leaf predicate we can iterate over leaves or with is_leaff_parent we can iterate
over leaf parent nodes (e.g. in case of node merge).

With default Proj projection (=std::itentity) we can view `Node &` type elements. To view e.g.
`terrain &` type elements use `proj=&terrain_quad::trn` projection. Projection proj is
used in `*` and `->` operator implementations.

There is terrain_quad as an Node implementation there.

During the iteratiion you are not allowed to modify a tree (insert, remove nodes).

\code
dfs_traverse_if leaf_view{root, is_leaf};  // view `Node &` type leaf elements
dfs_traverse_if parent_leaf_view{root, is_leaf_parent};  // view `Node &` type parent leaf elements
\endcode */
template <typename Node, typename UnaryPredicate, typename Proj = std::identity>
struct dfs_traverse_if : public std::ranges::view_interface<dfs_traverse_if<Node, UnaryPredicate, Proj>> {
	using view_type = dfs_traverse_if;

	explicit dfs_traverse_if(Node & root, UnaryPredicate pred, Proj proj = {}) : _root(&root), _proj{std::move(proj)}, _pred{pred} {}

	struct iterator {
		using iterator_category = std::input_iterator_tag;

		std::stack<Node *> nodes;

		explicit iterator(Node * root, view_type * parent) : _parent{parent} {
			if (root) {
				nodes.push(root);
				advance_to_next();
			}
		}

		/*! Dereference leaf node.
		\returns leaf node reference of `Node &` type by defaul. Return value and it's type can be changed by a view projection (=`Proj=std::indentity`).
		\code
		leaf_view{root, &terrain_quad::trn};  // view `terrain::trn` type member elements (e.g. `terrain &`)
		for (terrain const & trn : leaf_view) ...
		\endcode */
		decltype(auto) operator*() const {  // decltype(auto) because we want to return exact type returned by invoke in that case auto drops references
			return std::invoke(_parent->_proj, std::ref(*nodes.top()));
		}

		/*! Access leaf node.
		\returns an address of leaf node of type `Node *` */
		decltype(auto) operator->() const {
			return std::addressof(std::invoke(_parent->_proj, nodes.top()));
		}

		iterator & operator++() {
			if (Node * current = nodes.top(); std::invoke(_parent->_pred, std::ref(*current))) {
				nodes.pop();
				remember_children(*current);
			}
			advance_to_next();
			return * this;
		}

		bool operator!=([[maybe_unused]] iterator const & other) const {
			return !nodes.empty();
		}

	private:
		void advance_to_next() {  // the name chosen based to std::advance() function
			while (!nodes.empty() && !std::invoke(_parent->_pred, std::ref(*nodes.top()))) {
				Node * current = nodes.top();
				nodes.pop();
				remember_children(*current);
			}
		}

		void remember_children(Node & parent) {
			for (auto const & child : parent.children)
				if (child)
					nodes.push(child.get());
		}

		view_type * _parent;
	};  // iterator

	auto begin() { return iterator(_root, this); }
	auto end() { return iterator(nullptr, this); }

	auto begin() const requires std::is_const_v<Node> { return iterator(_root, this); }
	auto end() const requires std::is_const_v<Node> { return iterator(nullptr, this); }

 private:
	Node * _root;
	Proj _proj;
	UnaryPredicate _pred;
};  // dfs_traverse


/*! TODO: put some description there */
template<typename Node, typename Proj = std::identity>
struct leaf_view
	: public dfs_traverse_if<
		Node,
		bool(*)(Node const&),   // Pred = pointer-to-free-function
		Proj>
{
	using Base = dfs_traverse_if<Node, bool(*)(Node const &), Proj>;

	leaf_view(Node & root, Proj proj = {})
		: Base{root,
			&qtree::is_leaf<Node>,  // free-standing is_leaf function, we can also use &Node::is_leaf there instead
			proj}
	{}
};  // leaf_view

}  // qtree
