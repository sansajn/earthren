/*! \file */
#pragma once
#include <filesystem>
#include <map>
#include <ranges>
#include <stack>
#include <vector>
#include <glm/vec2.hpp>
#include <GLES3/gl32.h>

/* - we are expecting that all terrains has the same size textures so thre is no reason to store texture w/h
- grid_size is also the same for all terrain */
struct terrain {
	GLuint elevation_map,
		satellite_map;
	glm::vec2 position;  //!< Terrain word position (within thee grid).
	float elevation_min;

	int grid_c, grid_r;  //!< Grid position.
	int level = -1;  //!< Terrain level of detail (it is actually quadtree level/depth).
};

/*! Function to find out whether position is above a terrain.
E.g. To figure out whether camera is above a terrain. */
bool is_above(terrain const & trn, float quad_size, float model_scale, glm::vec3 const & pos);

struct terrain_quad {
	using value_type = terrain;

	std::array<std::unique_ptr<terrain_quad>, 4> children;
	bool is_leaf() const {return children[0] == nullptr;}
	value_type trn;
};

//! Quadtree leaf traversal (depth-first search (DFS)) view implementation.
template <typename Node>
class leaf_view : public std::ranges::view_interface<leaf_view<Node>> {
public:
	explicit leaf_view(Node & root) : root(&root) {}

	struct iterator {
		using value_type = Node::value_type;
		using reference = std::conditional_t<std::is_const_v<Node>, value_type const &, value_type &>;
		using pointer = std::conditional_t<std::is_const_v<Node>, value_type const *, value_type *>;
		using iterator_category = std::input_iterator_tag;

		std::stack<Node *> nodes;

		explicit iterator(Node * root) {
			if (root) {
				nodes.push(root);
				advance_to_leaf();
			}
		}

		reference operator*() const {
			return nodes.top()->trn;  // Dereference the leaf node's data
		}

		pointer operator->() const {
			return &nodes.top()->trn; // Access the data of the leaf node
		}

		iterator & operator++() {
			nodes.pop();
			advance_to_leaf();
			return * this;
		}

		bool operator!=([[maybe_unused]] iterator const & other) const {
			return !nodes.empty();
		}

	private:
		void advance_to_leaf() {
			while (!nodes.empty() && !nodes.top()->is_leaf()) {
				Node * current = nodes.top();
				nodes.pop();
				for (auto const & child : current->children)
					nodes.push(child.get());
			}
		}
	};  // iterator

	auto begin() { return iterator(root); }
	auto end() { return iterator(nullptr); }

	auto begin() const requires std::is_const_v<Node> { return iterator(root); }
	auto end() const requires std::is_const_v<Node> { return iterator(nullptr); }

private:
	Node * root;
};

struct terrain_grid {
	/* TODO: should be read_tiles member of terrain_grid? I think in the first step it is easier to
	implement due to unrestricted access and as a second step we can make it non member funnction if
	it makes any sence. */

	void load_tiles(std::filesystem::path const & data_path);
	[[nodiscard]] size_t size() const;

	/*! \returns Range to iterate through list of terrains.
	\code
	terrain_grid terrains;
	// ...
	for (terrain const & terrains.iterate()) {...}
	\endcode */
	[[nodiscard]] auto iterate() const {
		return leaf_view{_root};
	}

	[[nodiscard]] int grid_size(int level) const;  // TODO: this should not be member function
	[[nodiscard]] int elevation_tile_size(int level) const {return _data_desc.at(level).elevation_tile_size;}
	[[nodiscard]] double elevation_pixel_size(int level) const {return _data_desc.at(level).elevation_pixel_size;}

	float quad_size = 1.0f;

	static float camera_ground_height;  //!< Terrain ground height bellow camera. Camera needs to have an access to the property.

	~terrain_grid();

private:
	void load_description(std::filesystem::path const & data_path, int level);  // TODO: implementation of this needs to be changed, because it create a state
	int elevation_maxval(std::filesystem::path const & filename) const;

	//! \returns list of loaded terrains (meant to load quadtree level data, e.g. level 2 or 3)
	std::vector<terrain> load_level_tiles(std::filesystem::path const & data_path, int level) const;

	terrain_quad _root;  //!< terrains in a quadtree structure to allow LOD

	std::string _elevation_tile_prefix,
		_satellite_tile_prefix;

	struct dataset_description {
		int elevation_tile_size,
			satellite_tile_size;
		double elevation_pixel_size;
	};

	std::map<int, dataset_description> _data_desc;  //!< level based dataset descriptions

	size_t _terrain_count = 0;

	/* TODO: This is how we work with elevations in a vertx shader program
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale; */
	std::map<std::filesystem::path, int> _elevation_tile_max_value;  // this serves as a temporary variable for load_description function
};  // terrain_grid
