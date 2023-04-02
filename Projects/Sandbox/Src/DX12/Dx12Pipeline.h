#pragma once

#include "Dx12Device.h"

namespace RS::DX12
{
	class Dx12Pipeline
	{
	public:
		void Init();
		void Release();

		ID3D12PipelineState* GetPipelineState() const { return m_PipelineState; }

	private:
		void CreatePipeline();

	private:
		ID3D12PipelineState* m_PipelineState = nullptr;
		ID3D12RootSignature* m_RootSignature = nullptr;
	};
}