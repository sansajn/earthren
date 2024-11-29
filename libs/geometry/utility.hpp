#pragma once
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>

namespace geom {

// funkcie pracujuce s rotaciou
inline glm::vec3 const & right(glm::mat3 const & r) {return r[0];}
inline glm::vec3 const & up(glm::mat3 const & r) {return r[1];}
inline glm::vec3 const & forward(glm::mat3 const & r) {return r[2];}

inline glm::vec3 right(glm::mat4 const & r) {return glm::vec3{r[0]};}
inline glm::vec3 up(glm::mat4 const & r) {return glm::vec3{r[1]};}
inline glm::vec3 forward(glm::mat4 const & r) {return glm::vec3{r[2]};}

glm::vec3 right(glm::quat const & r);
glm::vec3 up(glm::quat const & r);
glm::vec3 forward(glm::quat const & r);

}  // geom
