#pragma once

#include <random>
#include <numbers>

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

			int SampleUniformIntDist(int minVal, int maxVal)
			{
				return GetUniformIntDist(minVal, maxVal)(m_Generator);
			}

			float SampleUniformFloatDist(float minVal, float maxVal)
			{
				return GetUniformFloatDist(minVal, maxVal)(m_Generator);
			}

		private:
			std::uniform_int_distribution<>& GetUniformIntDist(int minVal, int maxVal)
			{
				uint64 hash = ((uint64)maxVal << 32) | minVal;
				auto entry = m_UniformIntDists.find(hash);
				if (entry != m_UniformIntDists.end())
					return entry->second;

				return m_UniformIntDists[hash] = std::uniform_int_distribution<>(minVal, maxVal);
			}

			std::uniform_real_distribution<float>& GetUniformFloatDist(float minVal, float maxVal)
			{
				uint minValU = *reinterpret_cast<uint*>(&minVal);
				uint maxValU = *reinterpret_cast<uint*>(&maxVal);
				uint64 hash = ((uint64)maxValU << 32) | minValU;
				auto entry = m_UniformFloatDists.find(hash);
				if (entry != m_UniformFloatDists.end())
					return entry->second;

				return m_UniformFloatDists[hash] = std::uniform_real_distribution<float>(minVal, maxVal);
			}

		private:
			std::random_device m_RandomDevice; // Non-deterministic generator
			std::mt19937 m_Generator; // Mersenne Twister engine
			
			std::unordered_map<uint, std::uniform_int_distribution<>> m_UniformIntDists; // Uniform distribution
			std::unordered_map<uint64, std::uniform_real_distribution<float>> m_UniformFloatDists; // Uniform distribution
		};
	}

	inline float Rand01()
	{
		auto rState = _Internal::RandomState::Get();
		return rState->SampleUniformFloatDist(0.f, 1.f);
	}

	inline float RandRad()
	{
		auto rState = _Internal::RandomState::Get();
		return rState->SampleUniformFloatDist(0.f, std::numbers::pi_v<float> * 2.f);
	}

	inline float RandDeg()
	{
		auto rState = _Internal::RandomState::Get();
		return rState->SampleUniformFloatDist(0.f, 360.f);
	}

	inline int RandInt(int minVal, int maxVal)
	{
		auto rState = _Internal::RandomState::Get();
		return rState->SampleUniformIntDist(minVal, maxVal);
	}

	inline int RandInt(int maxVal)
	{
		auto rState = _Internal::RandomState::Get();
		return rState->SampleUniformIntDist(0, maxVal);
	}
}