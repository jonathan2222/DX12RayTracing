#include "PreCompiled.h"
#include "Dx12Pipeline.h"

#include "DX12/Dx12Core2.h"

void RS::DX12::Dx12Pipeline::Init()
{
	CreatePipeline();
}

void RS::DX12::Dx12Pipeline::Release()
{
	DX12_RELEASE(m_PipelineState);
	DX12_RELEASE(m_RootSignature);
}

void RS::DX12::Dx12Pipeline::CreatePipeline()
{
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> vertexShaderErrorMsg;
	ComPtr<ID3DBlob> pixelShader;
	ComPtr<ID3DBlob> pixelShaderErrorMsg;

#ifdef RS_CONFIG_DEBUG
	// Enable better shader debugging with the graphics debugging tools.
	INT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG_NAME_FOR_SOURCE;
#else
	compileFlags = 0;
#endif

	std::wstring shaderPath = Utils::ToWString(RS_SHADER_PATH + std::string("tmpShaders.hlsl"));
	//DXCall(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_6_0", compileFlags, 0, &vertexShader, &vertexShaderErrorMsg));
	//DXCall(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_6_0", compileFlags, 0, &pixelShader, &pixelShaderErrorMsg));

	DX12_DEVICE_PTR pDevice = Dx12Core2::Get()->GetD3D12Device();

	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
	}

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
	}

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = inputElementDescs.size();
	inputLayoutDesc.pInputElementDescs = inputElementDescs.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_RootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	//psoDesc.DS
	//psoDesc.HS
	//psoDesc.GS
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
}
