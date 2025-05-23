#include "shape_mesh.hpp"

using std::size, std::data;

ogl::mesh make_mesh(mesh_data const & d) {
	return {data(d.vertices), size(d.vertices)*sizeof(d.vertices[0]),
		data(d.indices), size(d.indices), d.layout};
}
