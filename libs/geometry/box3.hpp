#pragma once
#include <boost/geometry/geometries/box.hpp>
#include <glm/vec3.hpp>
#include "boost_geometry_adapt.hpp"

namespace geom {

using namespace boost::geometry;

//! @ingroup math
using box3 = boost::geometry::model::box<glm::vec3>;

inline float width(box3 const & b)
{
	return get<max_corner, 0>(b) - get<min_corner, 0>(b);
}

inline float height(box3 const & b)
{
	return get<max_corner, 1>(b) - get<min_corner, 1>(b);
}

inline float depth(box3 const & b)
{
	return get<max_corner, 2>(b) - get<min_corner, 2>(b);
}

}  // geom
