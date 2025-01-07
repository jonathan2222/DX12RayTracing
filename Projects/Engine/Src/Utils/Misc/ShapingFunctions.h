#pragma once

#include "Utils/Misc/MathUtils.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace RS::Utils
{
	template<typename T, typename B>
	inline T Smoothstep(B edge0, B edge1, T x)
	{
		T xx = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.0f);
		return xx * xx * (3.f - 2.f * xx);
	}

	template<>
	inline glm::vec2 Smoothstep(float edge0, float edge1, glm::vec2 x)
	{
		return {Smoothstep(edge0, edge1, x.x), Smoothstep(edge0, edge1, x.y) };
	}

	template<>
	inline glm::vec3 Smoothstep(float edge0, float edge1, glm::vec3 x)
	{
		return { Smoothstep(edge0, edge1, x.x), Smoothstep(edge0, edge1, x.y), Smoothstep(edge0, edge1, x.z) };
	}
	
	template<>
	inline glm::vec4 Smoothstep(float edge0, float edge1, glm::vec4 x)
	{
		return { Smoothstep(edge0, edge1, x.x), Smoothstep(edge0, edge1, x.y), Smoothstep(edge0, edge1, x.z), Smoothstep(edge0, edge1, x.w) };
	}
}