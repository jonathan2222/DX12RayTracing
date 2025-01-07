#pragma once

#include "Types.h"

#include <utility>
#include <string>

namespace RS
{
	struct SoundData;
	class Filter
	{
	public:
		virtual void Init(uint64 sampleRate) = 0;
		virtual void Destroy() = 0;
		/*
			Called before processing current block of data.
		*/
		virtual void Begin() = 0;
		
		/*
			Called wen one pair of data should be processed.
		*/
		virtual std::pair<float, float> Process(SoundData* pData, float left, float right) = 0;

		virtual std::string GetName() const = 0;
	};
}