#include <glm/matrix.hpp>

// TODO: give a sample how camera is meant to be used
struct map_camera {
	float theta,  //!< x-axis camera rotation in rad
		phi;  //!< z-axis camera rotation in rad
	float distance;  //!< camera distance from ground

	glm::vec2 look_at;  //!< look-at point on a map

	explicit map_camera(float d = 1.0f);
	glm::mat4 const & view() const {return _view;}

	void update();

 private:
	glm::mat4 _view;  //!< Camera view transformation matrix.
};
