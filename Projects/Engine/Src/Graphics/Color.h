#pragma once

#include <glm/vec4.hpp>

namespace RS
{
	using Color = uint;

	static const Color BLACK = 0x000000FF;
	static const Color WHITE = 0xFFFFFFFF;
	static const Color RED = 0xFF0000FF;
	static const Color GREEN = 0x00FF00FF;
	static const Color BLUE = 0x0000FFFF;

	static inline Color PackColor(float red, float green, float blue, float alpha)
	{
		auto saturate = [](float a) { if (a < 0.f) return 0.f; else if (a > 1.f) return 1.f; else return a; };
		uint r = (uint)(saturate(red) * 255u);
		uint g = (uint)(saturate(green) * 255u);
		uint b = (uint)(saturate(blue) * 255u);
		uint a = (uint)(saturate(alpha) * 255u);
		return (r << 24) | (g << 16) | (b << 8) | a;
	}

	static inline Color PackColor(const glm::vec4& color)
	{
		return PackColor(color.r, color.g, color.b, color.a);
	}

	static inline Color PackColor(const glm::vec3& color)
	{
		return PackColor(color.r, color.g, color.b, 1.0f);
	}

	static inline uint8 UnpackRed(Color color)
	{
		return (color >> 24) & 0xFF;
	}

	static inline uint8 UnpackGreen(Color color)
	{
		return (color >> 16) & 0xFF;
	}
	
	static inline uint8 UnpackBlue(Color color)
	{
		return (color >> 8) & 0xFF;
	}

	static inline uint8 UnpackAlpha(Color color)
	{
		return color & 0xFF;
	}

	static inline glm::vec4 UnpackColor(Color color)
	{
		glm::vec4 res(
			UnpackRed(color),
			UnpackGreen(color),
			UnpackBlue(color),
			UnpackAlpha(color)
		);
		return res / 255.f;
	}
}