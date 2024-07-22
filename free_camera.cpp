#include "free_camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

using glm::vec3, glm::mat4, glm::mat3,
	glm::conjugate, glm::translate, glm::angleAxis, glm::perspective, glm::normalize,
	glm::mat3_cast, glm::mat4_cast;

free_camera::free_camera(float fovy, float aspect, float near, float far)
	: _proj{perspective(fovy, aspect, near, far)}
	// TODO: explicitly set position and rotation members, otherwise default produce invalid view transformation matrix
{
	update();
}

vec3 free_camera::right() const {
	mat3 R = mat3_cast(rotation);
	return R[0];
}

vec3 free_camera::up() const {
	mat3 R = mat3_cast(rotation);
	return R[1];
}

vec3 free_camera::forward() const {
	mat3 R = mat3_cast(rotation);
	return R[2];
}

void free_camera::update() {
	// compute view transformation
	mat4 R = mat4_cast(conjugate(rotation));
	mat4 T = translate(mat4{1.0f}, -position);
	_view = R * T;  // inverse matrix, that is why R * T
}

void free_camera::look_at(vec3 const & center) {
	vec3 to_camera_dir = position - center;

	assert(length(to_camera_dir) > 0.001f && "center is too close to position");  // ?we can get incorrect results in that case?

	vec3 f = normalize(to_camera_dir);  // forward
	vec3 u = vec3(0,1,0);  // up
	vec3 r = normalize(cross(u,f));  // right
	u = cross(f,r);

	mat4 M(
		r.x, r.y, r.z, 0,
		u.x, u.y, u.z, 0,
		f.x, f.y, f.z, 0,
		0,   0,   0, 1);

	rotation = quat_cast(M);
}
