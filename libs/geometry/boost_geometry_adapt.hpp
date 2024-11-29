/*! \file
Adapts GLM vector types (vec1, vec2 and vec3) for Boost.Geometry.

Supported GLM versions are 0.9.9.8 (Ubuntu 24.04 LTS), 0.9.9.0 and 0.9.6.3. */
#pragma once
#include <boost/mpl/int.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <glm/detail/setup.hpp>

#if GLM_VERSION >= 998  // for Ubuntu 24.04 LTS
	#include <glm/detail/type_vec1.hpp>
	#include <glm/detail/type_vec2.hpp>
	#include <glm/detail/type_vec3.hpp>
#elif GLM_VERSION >= 990  // for Ubuntu 18.04 LTS
	#include <glm/detail/type_vec.hpp>
#else  // for glm 0.9.6.3
	#include <glm/vec2.hpp>
	#include <glm/vec3.hpp>
#endif

#if GLM_VERSION >= 990

namespace boost::geometry::traits {

// glm::vec adaptation
template <glm::length_t L, typename T, glm::qualifier Q>
struct tag<glm::vec<L, T, Q>>
{
	typedef point_tag type;
};

template <glm::length_t L, typename T, glm::qualifier Q>
struct coordinate_type<glm::vec<L, T, Q>>
{
	typedef typename glm::vec<L, T, Q>::value_type type;
};

template <glm::length_t L, typename T, glm::qualifier Q>
struct coordinate_system<glm::vec<L, T, Q>>
{
	typedef cs::cartesian type;
};

template <glm::length_t L, typename T, glm::qualifier Q>
struct dimension<glm::vec<L, T, Q>> : boost::mpl::int_<L>
{};

template <glm::length_t L, typename T, glm::qualifier Q, std::size_t Dimension>
struct access<glm::vec<L, T, Q>, Dimension>
{
	typedef glm::vec<L, T, Q> point_type;
	typedef typename glm::vec<L, T, Q>::value_type value_type;

	static value_type get(point_type const & p) {return p[Dimension];}
	static void set(point_type & p, value_type const & v) {p[Dimension] = v;}
};

}  // boost::geometry::traits

#else  // glm 0.9.6.3

namespace boost { namespace geometry { namespace traits {

// glm::vec2 adaptation
template <typename T, glm::precision P>
struct tag<glm::tvec2<T, P>>
{
	typedef point_tag type;
};

template <typename T, glm::precision P>
struct coordinate_type<glm::tvec2<T, P>>
{
	typedef typename glm::tvec2<T, P>::value_type type;
};

template <typename T, glm::precision P>
struct coordinate_system<glm::tvec2<T, P>>
{
	typedef cs::cartesian type;
};

template <typename T, glm::precision P>
struct dimension<glm::tvec2<T, P>> : boost::mpl::int_<2>
{};

template <typename T, glm::precision P, std::size_t Dimension>
struct access<glm::tvec2<T, P>, Dimension>
{
	typedef glm::tvec2<T, P> point_type;
	typedef typename glm::tvec2<T, P>::value_type value_type;

	static value_type get(point_type const & p) {return p[Dimension];}
	static void set(point_type & p, value_type const & v) {p[Dimension] = v;}
};

// glm::vec3 adaptation
template <typename T, glm::precision P>
struct tag<glm::tvec3<T, P>>
{
	typedef point_tag type;
};

template <typename T, glm::precision P>
struct coordinate_type<glm::tvec3<T, P>>
{
	typedef typename glm::tvec3<T, P>::value_type type;
};

template <typename T, glm::precision P>
struct coordinate_system<glm::tvec3<T, P>>
{
	typedef cs::cartesian type;
};

template <typename T, glm::precision P>
struct dimension<glm::tvec3<T, P>> : boost::mpl::int_<3>
{};

template <typename T, glm::precision P, std::size_t Dimension>
struct access<glm::tvec3<T, P>, Dimension>
{
	typedef glm::tvec3<T, P> point_type;
	typedef typename glm::tvec3<T, P>::value_type value_type;

	static value_type get(point_type const & p) {return p[Dimension];}
	static void set(point_type & p, value_type const & v) {p[Dimension] = v;}
};

}}}  // geometry, boost, traits

#endif
