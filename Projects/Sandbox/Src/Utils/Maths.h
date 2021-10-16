#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace RS
{
	struct Maths
	{
		/*
		*	Return a new vector with the lowest of the two elements.
		*	Example:
		*		a = (4, 2, 5)
		*		b = (1, 6, 2)
		*		result = (1, 2, 2)
		*/
		static glm::vec3 GetMinElements(const glm::vec3& a, const glm::vec3& b)
		{
			glm::vec3 v;
			v.x = a.x < b.x ? a.x : b.x;
			v.y = a.y < b.y ? a.y : b.y;
			v.z = a.z < b.z ? a.z : b.z;
			return v;
		}

		/*
		*	Return a new vector with the highest of the two elements.
		*	Example:
		*		a = (4, 2, 5)
		*		b = (1, 6, 2)
		*		result = (4, 6, 5)
		*/
		static glm::vec3 GetMaxElements(const glm::vec3& a, const glm::vec3& b)
		{
			glm::vec3 v;
			v.x = a.x > b.x ? a.x : b.x;
			v.y = a.y > b.y ? a.y : b.y;
			v.z = a.z > b.z ? a.z : b.z;
			return v;
		}
	};
}