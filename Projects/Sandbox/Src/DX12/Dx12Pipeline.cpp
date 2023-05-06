#include "PreCompiled.h"
#include "Dx12Pipeline.h"

#include "DX12/Dx12Core2.h"
#include "Shader.h"

void RS::DX12::Dx12Pipeline::Init()
{
	CreatePipeline();
}

void RS::DX12::Dx12Pipeline::Release()
{
	Dx12Core2::Get()->DeferredRelease(m_PipelineState);
	Dx12Core2::Get()->DeferredRelease(m_RootSignature);
}

void RS::DX12::Dx12Pipeline::CreatePipeline()
{
	Shader shader;
	Shader::Description shaderDesc{};
	shaderDesc.path = "tmpShaders.hlsl";
	shaderDesc.typeFlags = Shader::TypeFlag::Pixel | Shader::TypeFlag::Vertex;
	shader.Create(shaderDesc);

	// TODO: Remove this
	{ // Test reflection
		ID3D12ShaderReflection* reflection = shader.GetReflection(Shader::TypeFlag::Pixel);

		D3D12_SHADER_DESC d12ShaderDesc{};
		DXCall(reflection->GetDesc(&d12ShaderDesc));

		D3D12_SHADER_INPUT_BIND_DESC desc{};
		DXCall(reflection->GetResourceBindingDesc(0, &desc));

		D3D12_SHADER_INPUT_BIND_DESC desc2{};
		DXCall(reflection->GetResourceBindingDesc(1, &desc2));
	}

	CreateRootSignature();

	DX12_DEVICE_PTR pDevice = Dx12Core2::Get()->GetD3D12Device();

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	// TODO: Move this, and don't hardcode it!
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDesc = {};
		inputElementDesc.SemanticName = "POSITION";
		inputElementDesc.SemanticIndex = 0;
		inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDesc.InputSlot = 0;
		inputElementDesc.AlignedByteOffset = 0;
		inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDesc.InstanceDataStepRate = 0;
		inputElementDescs.push_back(inputElementDesc);

		inputElementDescs.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		inputElementDescs.push_back({ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
	}

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = inputElementDescs.size();
	inputLayoutDesc.pInputElementDescs = inputElementDescs.data();

	auto GetShaderBytecode = [&](IDxcBlob* shaderObject)->CD3DX12_SHADER_BYTECODE {
		if (shaderObject)
			return CD3DX12_SHADER_BYTECODE(shaderObject->GetBufferPointer(), shaderObject->GetBufferSize());
		return CD3DX12_SHADER_BYTECODE(nullptr, 0);
	};

	// TODO: Change the pipelinestate to use subobject like below (this saves memory by only passing subobjects that we need):
	//struct
	//{
	//	struct alignas(void*)
	//	{
	//		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{ D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE };
	//		ID3D12RootSignature* rootSigBlob;
	//	} rootSig;
	//} streams;
	//streams.rootSig.rootSigBlob = m_RootSignature;
	//
	//D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
	//streamDesc.pPipelineStateSubobjectStream = &streams;
	//streamDesc.SizeInBytes = sizeof(streams);
	//DXCall(pDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_PipelineState)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_RootSignature;
	psoDesc.VS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Vertex, true));
	psoDesc.PS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Pixel, true));
	psoDesc.DS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Domain, true));
	psoDesc.HS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Hull, true));
	psoDesc.GS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Geometry, true));
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	//psoDesc.DSVFormat
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	//psoDesc.CachedPSO
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	psoDesc.NodeMask = 0; // Single GPU -> Set to 0.
	//psoDesc.StreamOutput;
#ifdef RS_CONFIG_DEBUG
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
#else
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
	DXCall(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)));

	shader.Release();
}

void RS::DX12::Dx12Pipeline::CreateRootSignature()
{
	DX12_DEVICE_PTR pDevice = Dx12Core2::Get()->GetD3D12Device();

	uint32 registerSpace = 0;
	uint32 currentShaderRegisterCBV = 0;
	uint32 currentShaderRegisterSRV = 0;
	uint32 currentShaderRegisterUAV = 0;
	uint32 currentShaderRegisterSampler = 0;

	const uint32 cbvRegSpace = 1;
	const uint32 srvRegSpace = 2;
	const uint32 uavRegSpace = 3;
	const uint32 BINDLESS_RESOURCE_MAX_SIZE = 10;

	CD3DX12_ROOT_PARAMETER1 rootParameters[3] { };
	rootParameters[0].InitAsConstants(4*2, currentShaderRegisterCBV++, registerSpace, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(currentShaderRegisterCBV++, registerSpace, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_DESCRIPTOR_RANGE1 descRanges[3];
	descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, BINDLESS_RESOURCE_MAX_SIZE, 0, cbvRegSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
	descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BINDLESS_RESOURCE_MAX_SIZE, 0, srvRegSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
	descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, BINDLESS_RESOURCE_MAX_SIZE, 0, uavRegSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
	// Cannot have samplers in the same table as other resource types. Think about how descriptor heaps work.
	rootParameters[2].InitAsDescriptorTable(3u, static_cast<const D3D12_DESCRIPTOR_RANGE1*>(descRanges), D3D12_SHADER_VISIBILITY_ALL);

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1];
	{
		D3D12_STATIC_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; // Point sample for min, max, mag, mip.
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.RegisterSpace = registerSpace;
		samplerDesc.ShaderRegister = currentShaderRegisterSampler++;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[0] = samplerDesc;
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc{};
	versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	versionedRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	versionedRootSignatureDesc.Desc_1_1.NumParameters = 3;
	versionedRootSignatureDesc.Desc_1_1.pParameters = rootParameters;
	versionedRootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
	versionedRootSignatureDesc.Desc_1_1.pStaticSamplers = staticSamplers;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedRootSignatureDesc, &signature, &error);

	if (FAILED(hr))
	{
		if (error)
		{
			std::string str((char*)error->GetBufferPointer());
			LOG_ERROR("{}", str);
		}

		DXCall(hr);
	}

	DXCall(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
}
