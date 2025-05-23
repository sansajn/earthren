#include <cmath>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "ogl/gl_api.hpp"
#include "shape.hpp"
using std::vector;

mesh_data make_sphere(float r, size_t hsegments, size_t vsegments) {
	float dp = M_PI / (float)vsegments;  // delta phi
	float dt = 2.0*M_PI / (float)hsegments;  // delta theta

	vector<float> verts;  // position:3, texcoord:2, normal:3
	for (size_t i = 0; i <= vsegments; ++i)
	{
		float phi = i*dp;
		for (size_t j = 0; j <= hsegments; ++j)  // horizontalne kruhy
		{
			float theta = j*dt;
			glm::vec3 n{sin(phi)*sin(theta), cos(phi), sin(phi)*cos(theta)};
			glm::vec3 p = r*n;
			glm::vec2 uv{1.0f - j/(float)hsegments, 1.0f - i/(float)vsegments};
			verts.push_back(p.x);
			verts.push_back(p.y);
			verts.push_back(p.z);
			verts.push_back(uv.x);
			verts.push_back(uv.y);
			verts.push_back(n.x);
			verts.push_back(n.y);
			verts.push_back(n.z);
		}
	}

	vector<unsigned> inds;
	for (size_t i = 0; i < vsegments*(hsegments+1); ++i) {
		inds.push_back(i + hsegments + 1);
		inds.push_back(i+1);
		inds.push_back(i);

		inds.push_back(i);
		inds.push_back(i + hsegments);
		inds.push_back(i + hsegments + 1);
	}

	ogl::vertex_layout layout;
	layout.attributes = {
		{3, GL_FLOAT, 0},  // position
		{2, GL_FLOAT, 3*sizeof(float)},  // texture-coordinates (uv)
		{3, GL_FLOAT, (3+2)*sizeof(float)}  // normal
	};
	layout.stride = (3+2+3)*sizeof(float);
	layout.topology = GL_TRIANGLES;

	return mesh_data{verts, inds, layout};
}

mesh_data make_box(glm::vec3 const & half_extents) {
	glm::vec3 const & h = half_extents;

	vector<float> verts = {
	// float verts[6*4*(3+3)] = {  // position:3, normal:3
		// front
		-h.x, -h.y, h.z,  0, 0, 1,
		h.x, -h.y, h.z,  0, 0, 1,
		h.x, h.y, h.z,  0, 0, 1,
		-h.x, h.y, h.z,  0, 0, 1,
		// right
		h.x, -h.y, h.z,  1, 0, 0,
		h.x, -h.y, -h.z,  1, 0, 0,
		h.x, h.y, -h.z,  1, 0, 0,
		h.x, h.y, h.z,  1, 0, 0,
		// top
		-h.x, h.y, h.z,  0, 1, 0,
		h.x, h.y, h.z,  0, 1, 0,
		h.x, h.y, -h.z,  0, 1, 0,
		-h.x, h.y, -h.z,  0, 1, 0,
		// bottom
		-h.x, -h.y, -h.z,  0, -1, 0,
		h.x, -h.y, -h.z,  0, -1, 0,
		h.x, -h.y, h.z,  0, -1, 0,
		-h.x, -h.y, h.z,  0, -1, 0,
		// back
		h.x, -h.y, -h.z,  0, 0, -1,
		-h.x, -h.y, -h.z,  0, 0, -1,
		-h.x, h.y, -h.z,  0, 0, -1,
		h.x, h.y, -h.z,  0, 0, -1,
		// left
		-h.x, -h.y, -h.z,  -1, 0, 0,
		-h.x, -h.y, h.z,  -1, 0, 0,
		-h.x, h.y, h.z,  -1, 0, 0,
		-h.x, h.y, -h.z,  -1, 0, 0
	};

	// unsigned indices[] = {
	vector<unsigned> indices = {
		0, 1, 2,  2, 3, 0,
		4, 5, 6,  6, 7, 4,
		8, 9, 10, 10, 11, 8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};

	// Mesh m{verts, sizeof(verts), indices, 2*6*3};
	// m.attach_attributes({
	// 	typename Mesh::vertex_attribute_type{0, 3, GL_FLOAT, (3+3)*sizeof(GLfloat), 0},
	// 	typename Mesh::vertex_attribute_type{2, 3, GL_FLOAT, (3+3)*sizeof(GLfloat), 3*sizeof(GLfloat)}});

	// return m;

	ogl::vertex_layout layout;
	layout.attributes = {
		{3, GL_FLOAT, 0},  // position
		{3, GL_FLOAT, (3)*sizeof(float)}  // normal
	};
	layout.stride = (3+3)*sizeof(float);
	layout.topology = GL_TRIANGLES;

	return mesh_data{verts, indices, layout};
}
