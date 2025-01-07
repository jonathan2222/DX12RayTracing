#pragma once

#include "Filter.h"

namespace RS
{
	class DistanceFilter : public Filter
	{
	public:
		void Init(uint64 sampleRate) override;
		void Destroy() override;
		void Begin() override;
		std::pair<float, float> Process(SoundData* pData, float left, float right) override;
		std::string GetName() const override;
	};
}