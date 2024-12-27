#pragma once

#include <random>

namespace RS::Utils
{
	namespace _Internal
	{
		class RandomState
		{
		public:
			RandomState() : m_Generator(m_RandomDevice()) {}

			static std::shared_ptr<RandomState> Get()
			{
				static std::shared_ptr<RandomState> s_State = std::make_shared<RandomState>();
				return s_State;
			}

			std::uniform_int_distribution<>& GetUniformDist(uint precision)
			{
				auto entry = m_UniformDists.find(precision);
				if (entry != m_UniformDists.end())
					return entry->second;

				return m_UniformDists[precision] = std::uniform_int_distribution<>(0, precision-1);
			}

			std::mt19937& GetGenerator()
			{
				return m_Generator;
			}

		private:
			std::random_device m_RandomDevice; // Non-deterministic generator
			std::mt19937 m_Generator; // Mersenne Twister engine
			
			std::unordered_map<uint, std::uniform_int_distribution<>> m_UniformDists; // Uniform distribution
		};
	}

	inline float Rand01()
	{
		// TODO: Use std::uniform_real_distribution
		auto rState = _Internal::RandomState::Get();
		int val = rState->GetUniformDist(1000)(rState->GetGenerator());
		return (float)val / 1000.f;
	}

	inline int RandInt(int minVal, int maxVal)
	{
		auto rState = _Internal::RandomState::Get();
		int val = rState->GetUniformDist(maxVal-minVal + 1)(rState->GetGenerator());
		return val + minVal;
	}

	inline int RandInt(int maxVal)
	{
		auto rState = _Internal::RandomState::Get();
		int val = rState->GetUniformDist(maxVal+1)(rState->GetGenerator());
		return val;
	}
}