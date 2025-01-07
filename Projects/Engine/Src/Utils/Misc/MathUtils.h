#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace RS::Utils
{
	template<typename T, typename B>
	inline T Clamp(T x, B a, B b)
	{
		return x < a ? (b > x ? x : b) : a;
	}

	template<>
	inline glm::vec2 Clamp(glm::vec2 x, float a, float b)
	{
		return { Clamp(x.x, a, b), Clamp(x.y, a, b) };
	}

	template<>
	inline glm::vec3 Clamp(glm::vec3 x, float a, float b)
	{
		return { Clamp(x.x, a, b), Clamp(x.y, a, b), Clamp(x.z, a, b) };
	}

	template<>
	inline glm::vec4 Clamp(glm::vec4 x, float a, float b)
	{
		return { Clamp(x.x, a, b), Clamp(x.y, a, b), Clamp(x.z, a, b), Clamp(x.w, a, b) };
	}

	template<typename T>
	inline T Lerp(T a, T b, float t)
	{
		return a * (1.f - t) + b * t;
	}
}