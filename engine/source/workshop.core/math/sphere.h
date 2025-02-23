// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/aabb.h"

#include <vector>

namespace ws {

template <typename element_type>
class base_sphere
{
public:
	using vector_type = base_vector3<element_type>;

	vector_type origin;
	element_type radius;

	static const base_sphere empty;

	base_sphere() = default;
	base_sphere(const vector_type& origin_, float radius_);
	base_sphere(const vector_type* points, size_t count);
	base_sphere(const std::vector<vector_type>& points);

	bool intersects(const aabb& bounds) const;
	aabb get_bounds() const;
};

using sphere  = base_sphere<float>;
using sphered = base_sphere<double>;

template <typename element_type>
inline const base_sphere<element_type> base_sphere<element_type>::empty = base_sphere(vector_type::zero, static_cast<element_type>(0));

template <typename element_type>
inline base_sphere<element_type>::base_sphere(const vector_type& origin_, float radius_)
	: origin(origin_)
	, radius(radius_)
{
}

template <typename element_type>
inline base_sphere<element_type>::base_sphere(const vector_type* points, size_t count)
{
	origin = points[0];
	for (size_t i = 1; i < count; i++)
	{
		origin += points[i];
	}
	origin /= count;

	radius = 0.0f;
	for (size_t i = 0; i < count; i++)
	{
		float distance = (points[i] - origin).length();
		if (distance > radius)
		{
			radius = distance;
		}
	}
}

template <typename element_type>
inline base_sphere<element_type>::base_sphere(const std::vector<vector_type>& points)
	: sphere(points.data(), points.size())
{
}

template <typename element_type>
inline bool base_sphere<element_type>::intersects(const aabb& bounds) const
{
	vector3 closest = vector3::min(vector3::max(origin, bounds.min), bounds.max);
	element_type distance_squared = (closest - origin).length_squared();
	return distance_squared < (radius * radius);
}

template <typename element_type>
inline aabb base_sphere<element_type>::get_bounds() const
{
	return aabb(origin - radius, origin + radius);
}

}; // namespace ws