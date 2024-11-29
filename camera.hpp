#pragma once
#include <glm/matrix.hpp>

/*! Orbital camera over a terrain.
\code
map_camera cam{20.0f};
cam.look_at = vec2{0, 0};

while (true) {  // loop
	// handle events
	cam.update();  // update

	mat4 V = cam.vew();
	// render
}
\endcode */
struct map_camera {
	float theta,  //!< x-axis camera rotation in rad
		phi;  //!< z-axis camera rotation in rad
	float distance;  //!< camera distance from ground

	glm::vec2 look_at;  //!< look-at point on a map

	explicit map_camera(float d = 1.0f);
	[[nodiscard]] glm::mat4 const & view() const {return _view;}
	[[nodiscard]] glm::vec3 forward() const;  //!< gets camera forward direction

	void update();

private:
	glm::mat4 _view;  //!< Camera view transformation matrix.
};
