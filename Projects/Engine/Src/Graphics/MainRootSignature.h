#pragma once

#include "Types.h"

#include "DX12/NewCore/RootSignature.h"

namespace RS
{

	enum class RootParams : uint32
	{
		// Local Params
		LocalConstants = 0,
		LocalRBuffers,
		LocalRWBuffers,
		LocalRTextures,
		LocalRWTexture,

		// Global Params
		LinearSampler, // 0
		PointSampler,  // 1

	};

	class MainRootSignature
	{
	public:
		static std::shared_ptr<MainRootSignature> Get();

		void Init();
		void Destroy();

		std::shared_ptr<RS::RootSignature> m_pRootSignature;
	};
}
