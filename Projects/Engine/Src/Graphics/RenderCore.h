#pragma once

//#include "DX12/NewCore/Resources.h"
//#include <DX12/NewCore/RootSignature.h>

#include "DX12/Final/DXRootSignature.h"
#include "DX12/Final/DXSamplerDesc.h"
#include "DX12/Final/DXPipelineState.h"
#include "DX12/Final/DXCommandSignature.h"

namespace RS
{
	class RenderCore
	{
	public:
		static std::shared_ptr<RenderCore> Get();

		//std::shared_ptr<Texture> pTextureBlack;
		//std::shared_ptr<Texture> pTextureWhite;

	public:
		void Init();
		void Destory();

		//std::shared_ptr<RootSignature> GetMainRootSignature();

		static void InitCommonStates();

		inline static DX12::DXRootSignature& GetCommonRootSignature()
		{
			return m_sCommonRootSignature;
		}

		inline static DX12::DXSamplerDesc SamplerLinearWrapDesc;
		inline static DX12::DXSamplerDesc SamplerLinearClampDesc;
		inline static DX12::DXSamplerDesc SamplerPointClampDesc;
		inline static DX12::DXSamplerDesc SamplerPointBorderDesc;
		inline static DX12::DXSamplerDesc SamplerLinearBorderDesc;

		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;

		inline static D3D12_RASTERIZER_DESC RasterizerDefault;	// Counter-clockwise
		inline static D3D12_RASTERIZER_DESC RasterizerDefaultMsaa;
		inline static D3D12_RASTERIZER_DESC RasterizerDefaultCw;	// Clockwise winding
		inline static D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa;
		inline static D3D12_RASTERIZER_DESC RasterizerTwoSided;
		inline static D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa;

		inline static D3D12_BLEND_DESC BlendNoColorWrite;
		inline static D3D12_BLEND_DESC BlendPreMultiplied;
		inline static D3D12_BLEND_DESC BlendDisable;
		inline static D3D12_BLEND_DESC BlendTraditional;
		inline static D3D12_BLEND_DESC BlendAdditive;
		inline static D3D12_BLEND_DESC BlendTraditionalAdditive;

		inline static D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

		inline static DX12::DXCommandSignature DispatchIndirectCommandSignature = DX12::DXCommandSignature(1);
		inline static DX12::DXCommandSignature DrawIndirectCommandSignature = DX12::DXCommandSignature(1);

		inline static DX12::DXComputePSO GenerateMipsLinearPSO[4] =
		{
			{L"Generate Mips Linear CS"},
			{L"Generate Mips Linear Odd X CS"},
			{L"Generate Mips Linear Odd Y CS"},
			{L"Generate Mips Linear Odd CS"},
		};

		inline static DX12::DXComputePSO GenerateMipsGammaPSO[4] =
		{
			{ L"Generate Mips Gamma CS" },
			{ L"Generate Mips Gamma Odd X CS" },
			{ L"Generate Mips Gamma Odd Y CS" },
			{ L"Generate Mips Gamma Odd CS" },
		};

	private:
		inline static DX12::DXRootSignature m_sCommonRootSignature;
	};

	inline static std::shared_ptr<RenderCore> GetRenderCore()
	{
		return RenderCore::Get();
	}
}