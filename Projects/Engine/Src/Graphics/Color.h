#pragma once

#include <glm/vec4.hpp>

#include "Maths/Maths.h"

namespace RS
{
	struct Color32
	{
		union
		{
			struct
			{
				uint8 Red;
				uint8 Green;
				uint8 Blue;
				uint8 Alpha;
			};
			uint32 Data;
		};

		Color32() : Data(0u) {}
		Color32(uint32 data) : Data(data) {}
		Color32(uint8 red, uint8 green, uint8 blue, uint8 alpha)
			: Red(red), Green(green), Blue(blue), Alpha(alpha) {}
		Color32(float red, float green, float blue, float alpha)
			: Red((uint8)(Maths::Saturate(red)*0xFF))
			, Green((uint8)(Maths::Saturate(green) * 0xFF))
			, Blue((uint8)(Maths::Saturate(blue) * 0xFF))
			, Alpha((uint8)(Maths::Saturate(alpha) * 0xFF)) {}
		operator uint32() { return Data; }
	};

	namespace Color
	{
		static const Color32 BLACK = 0x000000FF;
		static const Color32 WHITE = 0xFFFFFFFF;
		static const Color32 RED = 0xFF0000FF;
		static const Color32 GREEN = 0x00FF00FF;
		static const Color32 BLUE = 0x0000FFFF;

		static inline Color32 ToColor32(float red, float green, float blue, float alpha)
		{
			uint r = (uint)(Maths::Saturate(red) * 255u);
			uint g = (uint)(Maths::Saturate(green) * 255u);
			uint b = (uint)(Maths::Saturate(blue) * 255u);
			uint a = (uint)(Maths::Saturate(alpha) * 255u);
			return (r << 24) | (g << 16) | (b << 8) | a;
		}

		static inline Color32 ToColor32(const glm::vec4& color)
		{
			return ToColor32(color.r, color.g, color.b, color.a);
		}

		static inline Color32 ToColor32(const glm::vec3& color)
		{
			return ToColor32(color.r, color.g, color.b, 1.0f);
		}

		static inline uint8 GetRed(Color32 color)
		{
			return (color >> 24) & 0xFF;
		}

		static inline uint8 GetGreen(Color32 color)
		{
			return (color >> 16) & 0xFF;
		}

		static inline uint8 GetBlue(Color32 color)
		{
			return (color >> 8) & 0xFF;
		}

		static inline uint8 GetAlpha(Color32 color)
		{
			return color & 0xFF;
		}

		static inline glm::vec4 ToVec4(Color32 color)
		{
			glm::vec4 res(
				GetRed(color),
				GetGreen(color),
				GetBlue(color),
				GetAlpha(color)
			);
			return res / 255.f;
		}

		static inline glm::vec3 ToVec3(Color32 color)
		{
			glm::vec3 res(
				GetRed(color),
				GetGreen(color),
				GetBlue(color)
			);
			return res / 255.f;
		}
	}
}