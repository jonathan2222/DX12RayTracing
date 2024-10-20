#pragma once

#include "DX12/NewCore/DX12Core3.h" // ID3D12PipelineState
#include "DX12/NewCore/RootSignature.h"
#include "DX12/NewCore/Shader.h"
#include <dxcapi.h> // IDxcBlob

#include "Utils/Misc/xxhash.h"

#include "Core/FileWatcher.h"

namespace RS
{
	struct InputElementDesc
	{
		std::string SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D12_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
	};

	/*
	* Pipeline State Object
	*  It has default values already set. It creates a unique key for each change, thus can be used as a key for fetching cache.
	*/
	// TODO: The PSO should be a Resource, because it depends on the frames it is used in. Thus cannot just remove it without waiting on it to have been used.
	class Shader;
	class GraphicsPSO
	{
	public:
		GraphicsPSO();
		~GraphicsPSO();

		void SetDefaults();

		// These are mandatory
		void SetInputLayout(const std::vector<InputElementDesc>& inputElements);
		void SetRootSignature(std::shared_ptr<RootSignature> pRootSignature);
		void SetRTVFormats(const std::vector<DXGI_FORMAT>& formats);
		void SetDSVFormat(DXGI_FORMAT format);
		void SetVS(Shader* pShader, bool supressWarnings);
		void SetPS(Shader* pShader, bool supressWarnings);
		void SetDS(Shader* pShader, bool supressWarnings);
		void SetHS(Shader* pShader, bool supressWarnings);
		void SetGS(Shader* pShader, bool supressWarnings);
		void SetShader(Shader* pShader);

		// These have defaults
		void SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc);
		void SetBlendState(D3D12_BLEND_DESC blendDesc);
		void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC depthStencilDesc);
		void SetIndexBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE cutValue);
		void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
		void SetPSOFlags(D3D12_PIPELINE_STATE_FLAGS flags);
		void SetSampleMask(uint32 sampleMask);
		void SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc);

		// Call this before use if anyone of the above has been called (or only the constructor).
		uint64 Create();

		// Implicit conversion to ID3D12PipelineState*
		operator ID3D12PipelineState* () { std::lock_guard<std::mutex> lock(m_Mutex); return m_pPipelineState; }

		uint64 GetKey() const;

	private:
		void InitDefaults();

		void ComputeHash();
		void UpdateHash(const D3D12_INPUT_LAYOUT_DESC& inputLayoutDesc);
		void UpdateHash(std::shared_ptr<RootSignature> pRootSignature);
		void UpdateHash(const D3D12_SHADER_BYTECODE& byteCode);
		void UpdateHash(const D3D12_RASTERIZER_DESC& rasterizerDesc);
		void UpdateHash(const D3D12_BLEND_DESC& blendDesc);
		void UpdateHash(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);
		void UpdateHash(const D3D12_INDEX_BUFFER_STRIP_CUT_VALUE& cutValue);
		void UpdateHash(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& topologyType);
		void UpdateHash(const D3D12_PIPELINE_STATE_FLAGS& flags);
		void UpdateHash(const uint32& sampleMask);
		void UpdateHash(const DXGI_SAMPLE_DESC& sampleDesc);
		void UpdateHash(DXGI_FORMAT formats[8]);
		void UpdateHash(const DXGI_FORMAT& format);

		void ValidateInputLayoutMatchingShader();

	private:
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElementDescs;
		std::vector<InputElementDesc> m_InputElements;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
		std::shared_ptr<RootSignature> m_pRootSignature;
		ID3D12PipelineState* m_pPipelineState = nullptr;
		xxh::hash3_state_t<64> m_HashStream;
		uint64 m_Key = 0;

		// For validation
		struct ShaderReflectionValidationInfo
		{
			ID3D12ShaderReflection* pReflection;
			std::string				shaderPath;
			std::string				shaderType;
		};
		std::array<ShaderReflectionValidationInfo, Shader::TypeFlag::COUNT> m_ShaderReflections;

		// Debug
		std::mutex m_Mutex;
		FileWatcher m_ShaderFileWatcher;
		Shader::Description m_ShaderDescription;
	};
}