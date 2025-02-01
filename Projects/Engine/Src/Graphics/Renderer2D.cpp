#include "PreCompiled.h"
#include "Renderer2D.h"

void RS::Renderer2D::Init()
{
}

void RS::Renderer2D::Destory()
{
}

void RS::Renderer2D::DrawRect(float x, float y, float width, float height, Color color)
{
}

void RS::Renderer2D::DrawCircle(float x, float y, float radius, Color color)
{
}

void RS::Renderer2D::Render()
{
}

void RS::Renderer2D::CreateRootSignature()
{
}

void RS::Renderer2D::CreateGraphicsPSO()
{
    RS::Shader shader;
    RS::Shader::Description shaderDesc{};
    shaderDesc.path = "Game/EntityShader.hlsl";
    shaderDesc.typeFlags = RS::Shader::TypeFlag::Pixel | RS::Shader::TypeFlag::Vertex;
    shader.Create(shaderDesc);

    std::vector<RS::InputElementDesc> inputElementDescs;
    // TODO: Move this, and don't hardcode it!
    {
        RS::InputElementDesc inputElementDesc = {};
        inputElementDesc.SemanticName = "SV_POSITION";
        inputElementDesc.SemanticIndex = 0;
        inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputElementDesc.InputSlot = 0;
        inputElementDesc.AlignedByteOffset = 0;
        inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputElementDesc.InstanceDataStepRate = 0;
        inputElementDescs.push_back(inputElementDesc);

        inputElementDescs.push_back({ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    }

    m_GraphicsPSO.SetDefaults();
    m_GraphicsPSO.SetInputLayout(inputElementDescs);
    m_GraphicsPSO.SetRootSignature(m_pRootSignature);
    m_GraphicsPSO.SetShader(&shader);
    m_GraphicsPSO.SetRTVFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
    m_GraphicsPSO.SetDSVFormat(DXGI_FORMAT_UNKNOWN);
    D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FrontCounterClockwise = false;
    m_GraphicsPSO.SetRasterizerState(rasterizerDesc);
    m_GraphicsPSO.Create();
}
