#pragma once
#include <boost/geometry/geometries/box.hpp>
#include <glm/vec2.hpp>
#include "boost_geometry_adapt.hpp"

namespace geom {

using namespace boost::geometry;

//! @ingroup math
using box2 = boost::geometry::model::box<glm::vec2>;

inline float width(box2 const & b)
{
	return get<max_corner, 0>(b) - get<min_corner, 0>(b);
}

inline float height(box2 const & b)
{
	return get<max_corner, 1>(b) - get<min_corner, 1>(b);
}

}  // geom
