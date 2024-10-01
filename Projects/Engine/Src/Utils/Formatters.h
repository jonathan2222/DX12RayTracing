#pragma once

#include <format>
#include "glm/glm.hpp"

template<>
struct std::formatter<glm::vec2> : std::formatter<std::string_view>
{
	auto format(const glm::vec2& value, std::format_context& ctx) const
	{
		std::string tmp;
		std::format_to(std::back_inserter(tmp), "({0}, {1})", value.x, value.y);
		return std::formatter<std::string_view>::format(tmp, ctx);
	}
};

template<>
struct std::formatter<glm::vec3> : std::formatter<std::string_view>
{
	auto format(const glm::vec3& value, std::format_context& ctx) const
	{
		std::string tmp;
		std::format_to(std::back_inserter(tmp), "({0}, {1}, {2})", value.x, value.y, value.z);
		return std::formatter<std::string_view>::format(tmp, ctx);
	}
};

template<>
struct std::formatter<glm::vec4> : std::formatter<std::string_view>
{
	auto format(const glm::vec4& value, std::format_context& ctx) const
	{
		std::string tmp;
		std::format_to(std::back_inserter(tmp), "({0}, {1}, {2}, {3})", value.x, value.y, value.z, value.w);
		return std::formatter<std::string_view>::format(tmp, ctx);
	}
};
