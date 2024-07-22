#pragma once
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>

// experimental free camera implementation

/* Camera to rotate around local x and y axis (not z). Implemenation
is based around lookAt functon. */
struct free_camera {
	glm::vec3 position;
	glm::quat rotation;  // TODO: this allows unrestricted rotations (alzo z axis rotations allowed)

	free_camera(float fovy, float aspect, float near, float far);
	void look_at(glm::vec3 const & center);

	glm::mat4 const & view() const {return _view;}
	glm::mat4 const & projection() const {return _proj;}

	// TODO: can we make it as a freestanding function?
	glm::mat4 const & camera_to_screen() const {return projection();}  //!< alias for projection()

	void update();

	glm::vec3 right() const;  //!< Get camera right axis direction.
	glm::vec3 up() const;  //!< Get camera up axis direction.
	glm::vec3 forward() const;  //!< Get camera forward axix direction.

private:
	glm::mat4 _proj,
		_view;  //!< Camera view transformation matrix.
};
