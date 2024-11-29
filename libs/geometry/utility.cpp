#include "utility.hpp"

using glm::vec3;
using glm::quat;
using glm::mat3_cast;
using glm::axis;
using glm::normalize;

namespace geom {

vec3 right(quat const & r)
{
	return mat3_cast(r)[0];
}

vec3 up(quat const & r)
{
	return mat3_cast(r)[1];
}

vec3 forward(quat const & r)
{
	return normalize(axis(r));
}

}  // geom
