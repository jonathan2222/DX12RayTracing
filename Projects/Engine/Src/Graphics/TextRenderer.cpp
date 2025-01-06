#include "TextRenderer.h"
#include "PreCompiled.h"

#include "Maths/GLMDefines.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

#include "GUI/IconsFontAwesome6.h"

#include "DX12/NewCore/DX12Core3.h"

#include "Utils/Utils.h"

std::shared_ptr<RS::TextRenderer> RS::TextRenderer::Get()
{
    static std::shared_ptr<TextRenderer> s_Freetype = std::make_shared<TextRenderer>();
    return s_Freetype;
}

void RS::TextRenderer::Init()
{
    FT_Error error = FT_Init_FreeType(&m_pLibrary);
    RS_ASSERT(error == FT_Err_Ok, "Coult not initialize freetype!");

    std::string faSolid900 = RS_FONT_PATH FONT_ICON_FILE_NAME_FAS;
    faSolid900 = Engine::GetInternalDataFilePath() + faSolid900;
    AddFont(faSolid900);

    InitRenderData();
}

void RS::TextRenderer::Release()
{
    FT_Done_FreeType(m_pLibrary);
}

bool RS::TextRenderer::AddFont(const std::string& fontPath)
{
    uint fontCount = (uint)m_FontNameToTextureListIndex.size();

    FT_Face pFace;
    FT_Error error = FT_New_Face(m_pLibrary, fontPath.c_str(), 0, &pFace);
    if (error != FT_Err_Ok)
    {
        RS_LOG_ERROR("Coult not load font '{}' for freetype!", fontPath);
        return false;
    }

    FT_Set_Pixel_Sizes(pFace, 0, 48); // Set size of this font.

    FT_GlyphSlot slot = pFace->glyph;

    auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();

    std::unordered_map<char, std::shared_ptr<Texture>> textures;
    m_Characters.clear();
    for (unsigned char c = 32; c < 255; c++)
    {
        error = FT_Load_Char(pFace, c, FT_LOAD_RENDER);
        if (error != FT_Err_Ok)
        {
            RS_LOG_WARNING("Failed to load Glyph '{}'", (char)c);
            continue;
        }

        error = FT_Render_Glyph(slot, FT_RENDER_MODE_SDF);
        if (error != FT_Err_Ok)
        {
            RS_LOG_WARNING("Failed to render SDF glyph '{}'!", (char)c);
            continue;
        }

        // TODO: Render to an atlas instead of multiple textures!
        std::string name = Utils::Format("Font {} {} Texture", fontCount, (char)c);
        auto pTexture = pCommandList->CreateTexture(slot->bitmap.width, slot->bitmap.rows, (const uint8*)slot->bitmap.buffer,
            DXGI_FORMAT_R8_UNORM, name);
        textures[(char)c] = pTexture;
        Character character = {
            -1,
            slot->bitmap.width, slot->bitmap.rows,
            slot->bitmap_left, slot->bitmap_top,
            slot->advance.x
        };
        m_Characters.insert(std::pair<char, Character>((char)c, character));
    }

    m_FontNameToTextureListIndex.insert(std::pair<std::string, uint>(fontPath, (uint)m_TextureMapList.size()));
    m_TextureMapList.push_back(textures);
    FT_Done_Face(pFace);

    // Wait for load to finish.
    uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);
    pCommandQueue->WaitForFenceValue(fenceValue);
    return true;
}

void RS::TextRenderer::RenderText(const std::string& txt, uint posX, uint posY, float scale, const glm::vec3& color)
{
    RenderItem item;
    item.text = txt;
    item.x = posX;
    item.y = posY;
    item.scale = scale;
    item.color = PackColor(color);
    m_RenderItems.push_back(item);
}

void RS::TextRenderer::Render(std::shared_ptr<RS::CommandList> pCommandList, std::shared_ptr<RenderTarget> pRenderTarget)
{
    if (m_RenderItems.empty())
        return;

    std::shared_ptr<Texture> colorTexture = pRenderTarget->GetAttachment(AttachmentPoint::Color0);
    D3D12_RESOURCE_DESC desc = colorTexture->GetD3D12ResourceDesc();

    glm::mat4 projection = glm::transpose(glm::orthoRH(0.f, (float)desc.Width, 0.f, (float)desc.Height, -10.f, 10.f));

    //D3D12_VIEWPORT viewport{};
    //viewport.MaxDepth = 0;
    //viewport.MinDepth = 1;
    //viewport.TopLeftX = viewport.TopLeftY = 0;
    //viewport.Width = desc.Width;
    //viewport.Height = desc.Height;
    //D3D12_RECT scissorRect{};
    //scissorRect.left = scissorRect.top = 0;
    //scissorRect.right = viewport.Width;
    //scissorRect.bottom = viewport.Height;

    //auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    //auto pCommandList = pCommandQueue->GetCommandList();
    //pCommandList->SetViewport(viewport);
    //pCommandList->SetScissorRect(scissorRect);
    pCommandList->SetRootSignature(m_pRootSignature);
    pCommandList->SetPipelineState(m_GraphicsPSO);
    pCommandList->SetRenderTarget(pRenderTarget, CommandList::RenderTargetMode::ColorOnly);
    pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (RenderItem& item : m_RenderItems)
    {
        glm::vec3 color = UnpackColor(item.color);
        struct PixelData
        {
            glm::mat4 projection;
            glm::vec3 textColor;
            float threshold;
        } pixelData = {
            projection,
            color,
            0.5f
        };
        pCommandList->SetGraphicsDynamicConstantBuffer(0, sizeof(pixelData), (void*)& pixelData);

        uint fontIndex = 0; // TODO: Make this customizable!
        std::unordered_map<char, std::shared_ptr<Texture>>& textureMap = m_TextureMapList[fontIndex];
        for (char c : item.text)
        {
            auto it = m_Characters.find(c);
            if (it == m_Characters.end())
                continue;

            Character& ch = it->second;

            float xpos = item.x + ch.BearingX * item.scale;
            float ypos = item.y - (ch.SizeY - ch.BearingY) * item.scale;

            float w = ch.SizeX * item.scale;
            float h = ch.SizeY * item.scale;

            glm::vec4 triangleVertices[] =
            {
                { xpos    , ypos + h, 0.0f, 0.0f }, // TL
                { xpos + w, ypos + h, 1.0f, 0.0f }, // TR
                { xpos    , ypos    , 0.0f, 1.0f }, // BL

                { xpos + w, ypos + h, 1.0f, 0.0f }, // TR
                { xpos + w, ypos    , 1.0f, 1.0f }, // BR
                { xpos    , ypos    , 0.0f, 1.0f }  // BL
            };
            pCommandList->UploadToBuffer(m_pVertexBufferResource, sizeof(glm::vec4) * 6, (void*)&triangleVertices[0]);
            pCommandList->SetVertexBuffers(0, m_pVertexBufferResource);
            std::shared_ptr<Texture>& pTexture = textureMap[c];
            pCommandList->BindTexture(1, 0, pTexture);
            pCommandList->DrawInstanced(6, 1, 0, 0);
            //pCommandQueue->ExecuteCommandList(pCommandList);
            //break;
            item.x += (ch.Advance >> 6) * item.scale;
        }
    }

    m_RenderItems.clear();
}

void RS::TextRenderer::InitRenderData()
{
    m_pRootSignature = std::make_shared<RS::RootSignature>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    auto& rootSignature = *m_pRootSignature.get();
    rootSignature[0].CBV(0, 0);
    rootSignature[1][0].SRV(1, 0, 0); // Character Texture
    {
        CD3DX12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR; // Linear sample for min, max, mag, and mip.
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        rootSignature.AddStaticSampler(samplerDesc);
    }
    rootSignature.Bake("TextRenderer_RootSignature");

    RS::Shader shader;
    RS::Shader::Description shaderDesc{};
    shaderDesc.isInternalPath = true;
    shaderDesc.path = "Core/TextRenderShader.hlsl";
    shaderDesc.typeFlags = RS::Shader::TypeFlag::Pixel | RS::Shader::TypeFlag::Vertex;
    shader.Create(shaderDesc);

    auto pDevice = RS::DX12Core3::Get()->GetD3D12Device();

    std::vector<RS::InputElementDesc> inputElementDescs;
    {
        RS::InputElementDesc inputElementDesc = {};
        inputElementDesc.SemanticName = "SV_POSITION";
        inputElementDesc.SemanticIndex = 0;
        inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputElementDesc.InputSlot = 0;
        inputElementDesc.AlignedByteOffset = 0;
        inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputElementDesc.InstanceDataStepRate = 0;
        inputElementDescs.push_back(inputElementDesc);
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

    shader.Release();

    auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();

    glm::vec2 minP(50, 500);
    glm::vec2 maxP(600, 800);
    glm::vec4 triangleVertices[] =
    {
        { minP.x, maxP.y, 0.0f, 1.0f }, // TL
        { maxP.x, maxP.y, 1.0f, 1.0f }, // TR
        { minP.x, minP.y, 0.0f, 0.0f }, // BL
        { maxP.x, maxP.y, 1.0f, 1.0f }, // TR
        { maxP.x, minP.y, 1.0f, 0.0f }, // BR
        { minP.x, minP.y, 0.0f, 0.0f }  // BL
    };
    const UINT vertexBufferSize2 = sizeof(triangleVertices);
    m_pVertexBufferResource = pCommandList->CreateVertexBufferResource(vertexBufferSize2, sizeof(glm::vec4), "Text Renderer Vertex Buffer");
    pCommandList->UploadToBuffer(m_pVertexBufferResource, vertexBufferSize2, (void*)&triangleVertices[0]);

    // Wait for load to finish.
    uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);
    pCommandQueue->WaitForFenceValue(fenceValue);
}

