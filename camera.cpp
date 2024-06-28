#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"

using glm::mat4, glm::vec3, glm::translate;

map_camera::map_camera(float d) : theta{0}, phi{0}, distance{d}, look_at{0, 0} {}

void map_camera::update() {
	vec3 const p0 = {look_at, 0};

	vec3 const px = {1, 0, 0},
		py = {0, 1, 0},
		pz = {0, 0, 1};

	float const cp = cos(phi),
		sp = sin(phi),
		ct = cos(theta),
		st = sin(theta);

	vec3 const cx = px*cp + py*sp,
		cy = -px*sp*ct + py*cp*ct + pz*st,
		cz = px*sp*st - py*cp*st + pz*ct;

	vec3 const position = p0 + cz * distance;

	mat4 const V = {
		cx.x, cy.x, cz.x, 0,
		cx.y, cy.y, cz.y, 0,
		cx.z, cy.z, cz.z, 0,
		0, 0, 0, 1
	};

	_view = translate(V, -position);
}
