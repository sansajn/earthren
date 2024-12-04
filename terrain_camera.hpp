/*! \file
Terrain camera implementation. */
#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

/*! Orbital camera over a terrain.
\code
terrain_camera cam{20.0f};
cam.look_at = vec2{0, 0};

while (true) {  // loop
// handle events
cam.update();  // update

mat4 V = cam.vew();
// render
}
\endcode */
struct terrain_camera {
	float theta,  //!< x-axis camera rotation in rad
		phi;  //!< z-axis camera rotation in rad
	float distance;  //!< camera distance from ground

	glm::vec2 look_at;  //!< look-at point on a map

	explicit terrain_camera(float d = 1.0f);
	[[nodiscard]] glm::mat4 const & view() const {return _view;}
	[[nodiscard]] glm::vec3 forward() const;  //!< gets camera forward direction
	[[nodiscard]] glm::vec3 position() const {return _position;}

	/*! \note view(), forward() or position() result available after the first update() called. */
	void update();

 private:
	glm::vec3 _position;
	glm::mat4 _view;  //!< Camera view transformation matrix.
	float _prev_ground_height;
};
