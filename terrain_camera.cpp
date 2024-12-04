#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "terrain_grid.hpp"
#include "terrain_camera.hpp"
using std::max;
using glm::vec3, glm::mat4, glm::translate;


terrain_camera::terrain_camera(float d)
	: theta{0},
	phi{0},
	distance{d},
	look_at{0, 0},
	_position{0},
	_prev_ground_height{terrain_grid::camera_ground_height} {}

void terrain_camera::update() {
	constexpr float height_offset = 0.1f;

	//  to prevent popping camera in case ground is closer
	float const ground_height = max(_prev_ground_height, terrain_grid::camera_ground_height);
	_prev_ground_height = ground_height;

	// we do not want to let distance < 0
	distance = std::max(0.0f, distance);

	vec3 const p0 = {look_at, ground_height};

	vec3 const px = {1, 0, 0},
		py = {0, 1, 0},
		pz = {0, 0, 1};

	float const cp = cos(phi),
		sp = sin(phi),
		ct = cos(theta),
		st = sin(theta);

	vec3 const cx = px*cp + py*sp,
		cy = -px*sp*ct + py*cp*ct + pz*st,
		cz = px*sp*st - py*cp*st + pz*ct;  // z-axis directional vector

	_position = p0 + cz * distance;
	_position.z = std::max(_position.z, ground_height + height_offset);  // clamp position.z
	// TODO: we've changed position.z, maybe we need to adapt distance also

	mat4 const V = {
		cx.x, cy.x, cz.x, 0,
		cx.y, cy.y, cz.y, 0,
		cx.z, cy.z, cz.z, 0,
		0, 0, 0, 1
	};

	_view = translate(V, -_position);
}

vec3 terrain_camera::forward() const {
	return normalize(-vec3{_view[2]});
}


