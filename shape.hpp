#pragma once
#include <vector>
#include <cstddef>
#include <glm/vec3.hpp>
#include "ogl/vertex_layout.hpp"

struct mesh_data {
	std::vector<float> vertices;
	std::vector<unsigned> indices;
	ogl::vertex_layout layout;
	// TODO: there in layout we are missing information about vertex attibutes type e.g. positions, uv, normals, ...
};

mesh_data make_sphere(float r, size_t hsegments, size_t vsegments);
mesh_data make_box(glm::vec3 const & half_extents);
