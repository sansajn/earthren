#include "mesh_draw.hpp"
#include "ogl/gl_api.hpp"

void mesh_draw(ogl::mesh const & obj) {
	// WARNING: not indexed meshes are not supported
	glDrawElements(obj.topology(), obj.size(), GL_UNSIGNED_INT, 0);
}
