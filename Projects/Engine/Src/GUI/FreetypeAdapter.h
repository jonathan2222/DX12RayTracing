#pragma once

#include "Types.h"

#include <unordered_map>

namespace RS
{
	class FreetypeAdapter
	{
	public:
		struct Character
		{
			uint TextureID;
			int SizeX;
			int SizeY;
			int BearingX;
			int BearingY;
			uint Advance;
		};

	public:
		static std::shared_ptr<FreetypeAdapter> Get();

		void Init();
		void Release();

	private:
		std::unordered_map<char, Character> m_Characters;
	};
}