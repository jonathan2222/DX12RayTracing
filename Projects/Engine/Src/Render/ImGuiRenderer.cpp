#include "PreCompiled.h"
#include "ImGuiRenderer.h"

#include "GUI/ImGuiAdapter.h"
//#include <backends/imgui_impl_dx12.h>

#include "DX12/NewCore/DX12Core3.h"
#include "Core/Display.h"
#include "Core/Console.h"

#include "DX12/NewCore/Shader.h"

RS::ImGuiRenderer* RS::ImGuiRenderer::Get()
{
	static std::unique_ptr<RS::ImGuiRenderer> pImGuiRenderer{ std::make_unique<RS::ImGuiRenderer>() };
	return pImGuiRenderer.get();
}

void RS::ImGuiRenderer::Init()
{
	Console::Get()->AddVar("ImGui.ConfigFlags", m_ImGuiConfigFlags, Console::Flag::ReadOnly, "ConfigFlags");

	InternalResize();
}

void RS::ImGuiRenderer::FreeDescriptor()
{
	if (!m_IsInitialized)
	{
		LOG_WARNING("ImGuiRenderer is not initialized!");
		return;
	}
}

void RS::ImGuiRenderer::Release()
{
	ImplDX12Shutdown();
	ImGuiAdapter::Release();
	ImGui::DestroyContext();

	m_IsReleased = true;
	m_IsInitialized = false;
}

void RS::ImGuiRenderer::Draw(std::function<void(void)> callback)
{
	if (!m_IsInitialized)
	{
		LOG_WARNING("ImGuiRenderer is not initialized!");
		return;
	}

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_DrawCalls.emplace_back(callback);
}

void RS::ImGuiRenderer::Render()
{
	if (m_DrawCalls.empty())
		return;

	if (!m_IsInitialized)
	{
		LOG_WARNING("ImGuiRenderer is not initialized!");
		return;
	}

	if (m_ShouldRescale)
	{
		// Resize only, after BeginFrame set s_ShouldRescale to false.
		m_ShouldRescale = false;
		InternalResize();
	}

	ImplDX12NewFrame();
	ImGuiAdapter::NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = m_Scale;
	m_ImGuiConfigFlags = io.ConfigFlags;

	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		for (auto& drawCall : m_DrawCalls)
			drawCall();
	}

	auto pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
	auto pCommandList = pCommandQueue->GetCommandList();

	// Rendering
	ImGui::Render();

	auto pRenderTarget = DX12Core3::Get()->GetSwapChain()->GetCurrentRenderTarget();
	pCommandList->SetRenderTarget(pRenderTarget);

	ImplDX12RenderDrawData(ImGui::GetDrawData(), pCommandList);

	pCommandQueue->ExecuteCommandList(pCommandList);

	// Clear the call stack
	m_DrawCalls.clear();
}

bool RS::ImGuiRenderer::WantKeyInput()
{
	if (!m_IsInitialized)
	{
		LOG_WARNING("ImGuiRenderer is not initialized!");
		return false;
	}

	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

void RS::ImGuiRenderer::Resize()
{
	if (!m_IsInitialized)
		LOG_WARNING("ImGuiRenderer is not initialized!");

	m_ShouldRescale = true;
}

float RS::ImGuiRenderer::GetGuiScale()
{
	if (!m_IsInitialized)
		LOG_WARNING("ImGuiRenderer is not initialized!");

	return m_Scale;
}

ImTextureID RS::ImGuiRenderer::GetImTextureID(std::shared_ptr<Texture> pTexture)
{
	TrackTextureResource(pTexture);
	return (ImTextureID)pTexture.get();
}

void RS::ImGuiRenderer::InternalInit()
{
	m_IsInitialized = true;
	m_IsReleased = false;

	auto pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
	auto pCommandList = pCommandQueue->GetCommandList();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	std::shared_ptr<Display> pDisplay = Display::Get();
	ReScale(pDisplay->GetWidth(), pDisplay->GetHeight());

	ImGuiAdapter::Init(Display::Get()->GetGLFWWindow(), true, ImGuiAdapter::ClientAPI::DX12);

	DXGI_FORMAT format = DX12Core3::Get()->GetSwapChain()->GetFormat();
	ImplDX12Init(format);

	// Add icon font:
	float pixelSize = 13.f;// *GetGuiScale();
	ImFont* pFont = io.Fonts->AddFontDefault();
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 }; // Will not be copied by AddFont* so keep in scope.
	ImFontConfig config;
	config.MergeMode = true;
	config.FontDataOwnedByAtlas = true;
	config.PixelSnapH = true;
	config.GlyphMinAdvanceX = pixelSize;
	std::string faSolid900 = RS_FONT_PATH FONT_ICON_FILE_NAME_FAS;
	faSolid900 = Engine::GetInternalDataFilePath() + faSolid900;
	io.Fonts->AddFontFromFileTTF(faSolid900.c_str(), pixelSize, &config, icons_ranges);
	io.Fonts->Build();
}

void RS::ImGuiRenderer::InternalResize()
{
	auto pDisplay = RS::Display::Get();
	uint32 width = pDisplay->GetWidth();
	uint32 height = pDisplay->GetHeight();
	if (width != 0 && height != 0 && width != m_OldWidth && height != m_OldHeight)
	{
		if (!m_IsReleased)
		{
			DX12Core3::Get()->Flush();
			FreeDescriptor();
			Release();
		}
		InternalInit();

		if (additionalResizeFunction)
			additionalResizeFunction(width, height);

		m_OldWidth = width;
		m_OldHeight = height;
	}
}

void RS::ImGuiRenderer::ReScale(uint32 width, uint32 height)
{
	if (!m_IsInitialized)
	{
		LOG_WARNING("ImGuiRenderer is not initialized!");
		return;
	}

	ImGuiStyle& style = ImGui::GetStyle();
	float widthScale = width / 1920.f;
	float heightScale = height / 1080.f;
	m_Scale = widthScale > heightScale ? widthScale : heightScale;
	style.ScaleAllSizes(m_Scale);

	LOG_INFO("ImGui Scale: {}", m_Scale);
}

bool RS::ImGuiRenderer::ImplDX12Init(DXGI_FORMAT rtvFormat)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImplDX12BackendRendererData* bd = IM_NEW(ImplDX12BackendRendererData)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "imgui_impl_dx12";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	//	ImplDX12InitPlatformInterface();

	bd->rtvFormat = rtvFormat;

	// Create a dummy ImGui_ImplDX12_ViewportData holder for the main viewport,
	// Since this is created and managed by the application, we will only use the ->Resources[] fields.
	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	main_viewport->RendererUserData = IM_NEW(ImplDX12ViewportData);// (bd->numFramesInFlight);

	return true;
}

void RS::ImGuiRenderer::ImplDX12Shutdown()
{
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	// Manually delete main viewport render resources in-case we haven't initialized for viewports
	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	if (ImplDX12ViewportData* vd = (ImplDX12ViewportData*)main_viewport->RendererUserData)
	{
		// We could just call ImGui_ImplDX12_DestroyWindow(main_viewport) as a convenience but that would be misleading since we only use data->Resources[]
		vd->renderBuffers.pVertexBuffer.reset();
		vd->renderBuffers.pIndexBuffer.reset();
		IM_DELETE(vd);
		main_viewport->RendererUserData = nullptr;
	}

	// Clean up windows and device objects
	ImplDX12ShutdownPlatformInterface();
	ImplDX12InvalidateDeviceObjects();

	FreeAllTextureResources();
	bd->pFontTexture.reset();

	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports);
	IM_DELETE(bd);
}

void RS::ImGuiRenderer::ImplDX12CreateRootSignature(ImplDX12BackendRendererData* br)
{
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	br->pRootSignature = std::make_shared<RootSignature>(rootSignatureFlags);

	static const uint32 MAX_NUM_GPU_TEXTURES = 1; // TODO: Add support for more textures!

	auto& rootSignature = *br->pRootSignature.get();
	rootSignature[0].Constants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootSignature[1][0].SRV(MAX_NUM_GPU_TEXTURES, 0);
	rootSignature[1].SetVisibility(D3D12_SHADER_VISIBILITY_PIXEL);

	{
		CD3DX12_STATIC_SAMPLER_DESC staticSampler{};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.MipLODBias = 0.f;
		staticSampler.MaxAnisotropy = 0;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampler.MinLOD = 0.f;
		staticSampler.MaxLOD = 0.f;
		staticSampler.ShaderRegister = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootSignature.AddStaticSampler(staticSampler);
	}

	rootSignature.Bake();
}

void RS::ImGuiRenderer::ImplDX12CreatePipelineState(ImplDX12BackendRendererData* br)
{
	Shader shader;
	Shader::Description shaderDesc{};
	shaderDesc.path = "ImGui/ImGuiShader.hlsl";
	shaderDesc.typeFlags = Shader::TypeFlag::Pixel | Shader::TypeFlag::Vertex;
	shaderDesc.customEntryPoints = { {Shader::TypeFlag::Pixel, "PixelMain"}, {Shader::TypeFlag::Vertex, "VertexMain"} };
	shader.Create(shaderDesc);

	ImplDX12CreateRootSignature(br);

	auto pDevice = DX12Core3::Get()->GetD3D12Device();

	auto GetShaderBytecode = [&](IDxcBlob* shaderObject)->CD3DX12_SHADER_BYTECODE {
		if (shaderObject)
			return CD3DX12_SHADER_BYTECODE(shaderObject->GetBufferPointer(), shaderObject->GetBufferSize());
		return CD3DX12_SHADER_BYTECODE(nullptr, 0);
	};

	static D3D12_INPUT_ELEMENT_DESC local_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { local_layout, 3 };
	psoDesc.pRootSignature = br->pRootSignature->GetRootSignature().Get();
	psoDesc.VS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Vertex, true));
	psoDesc.PS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Pixel, true));
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.NodeMask = 0; // Single GPU -> Set to 0.
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// Create the blending setup
	{
		D3D12_BLEND_DESC& desc = psoDesc.BlendState;
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	// Create the rasterizer state
	{
		D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = D3D12_CULL_MODE_NONE;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.DepthClipEnable = true;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	// Create depth-stencil State
	{
		D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.StencilEnable = false;
		desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.BackFace = desc.FrontFace;
	}

	DXCall(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&br->pPipelineState)));

	shader.Release();

	ImplDX12CreateFontsTexture();
}

void RS::ImGuiRenderer::ImplDX12NewFrame()
{
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImplDX12Init()?");

	if (!bd->pPipelineState)
		ImplDX12CreateDeviceObjects();
}

void RS::ImGuiRenderer::ImplDX12SetupRenderState(ImDrawData* draw_data, std::shared_ptr<CommandList> pCommandList, ImplDX12RenderBuffers* fr)
{
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
	VERTEX_CONSTANT_BUFFER_DX12 vertex_constant_buffer;
	{
		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&vertex_constant_buffer.mvp, mvp, sizeof(mvp));
	}

	// Setup viewport
	D3D12_VIEWPORT vp;
	memset(&vp, 0, sizeof(D3D12_VIEWPORT));
	vp.Width = draw_data->DisplaySize.x;
	vp.Height = draw_data->DisplaySize.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = vp.TopLeftY = 0.0f;
	pCommandList->SetViewport(vp);

	pCommandList->SetVertexBuffers(0, fr->pVertexBuffer);
	pCommandList->SetIndexBuffer(fr->pIndexBuffer);

	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetPipelineState(bd->pPipelineState);
	pCommandList->SetRootSignature(bd->pRootSignature);

	pCommandList->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer);

	// Setup blend factor
	const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
	pCommandList->SetBlendFactor(blend_factor);
}

void RS::ImGuiRenderer::ImplDX12RenderDrawData(ImDrawData* draw_data, std::shared_ptr<CommandList> pCommandList)
{
	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	ImplDX12ViewportData* vd = (ImplDX12ViewportData*)draw_data->OwnerViewport->RendererUserData;
	ImplDX12RenderBuffers* fr = &vd->renderBuffers;

	// Create and grow vertex/index buffers if needed
	if (fr->pVertexBuffer == nullptr || (fr->pVertexBuffer && fr->pVertexBuffer->GetCount() < draw_data->TotalVtxCount))
	{
		uint64 newCount = 0;
		if (fr->pVertexBuffer == nullptr)
			newCount = 5000; // Initial vertex count.
		if (fr->pVertexBuffer)
		{
			newCount = fr->pVertexBuffer->GetCount();
			fr->pVertexBuffer.reset();
		}
		newCount = draw_data->TotalVtxCount + 5000;

		uint32 stride = sizeof(ImDrawVert);
		D3D12_RESOURCE_DESC desc;
		memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = newCount * sizeof(ImDrawVert);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		fr->pVertexBuffer = std::make_shared<VertexBuffer>(stride, desc, nullptr, "ImGui Vertex Buffer", D3D12_HEAP_TYPE_UPLOAD);
	}

	if (fr->pIndexBuffer == nullptr || (fr->pIndexBuffer && (fr->pIndexBuffer->GetSize() / sizeof(ImDrawIdx)) < draw_data->TotalIdxCount))
	{
		uint64 newCount = 0;
		if (fr->pIndexBuffer == nullptr)
			newCount = 10000; // Initial index count.
		if (fr->pIndexBuffer)
		{
			newCount = fr->pIndexBuffer->GetSize() / sizeof(ImDrawIdx);
			fr->pIndexBuffer.reset();
		}
		newCount = draw_data->TotalIdxCount + 10000;

		D3D12_RESOURCE_DESC desc;
		memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = newCount * sizeof(ImDrawIdx);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		fr->pIndexBuffer = std::make_shared<IndexBuffer>(false, desc, nullptr, "ImGui Index Buffer", D3D12_HEAP_TYPE_UPLOAD);
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	void* vtx_resource, * idx_resource;
	if (!fr->pVertexBuffer->Map(0, &vtx_resource))
	{
		pCommandList->FlushResourceBarriers();
		FreeAllTextureResources();
		TrackTextureResource(bd->pFontTexture); // Font texture should always be tracked.
		return;
	}
	if (!fr->pIndexBuffer->Map(0, &idx_resource))
	{
		pCommandList->FlushResourceBarriers();
		FreeAllTextureResources();
		TrackTextureResource(bd->pFontTexture); // Font texture should always be tracked.
		return;
	}
	ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
	ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	fr->pVertexBuffer->Unmap(0);
	fr->pIndexBuffer->Unmap(0);

	// Setup desired DX state
	ImplDX12SetupRenderState(draw_data, pCommandList, fr);

	bool calledDraw = false;

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_vtx_offset = 0;
	int global_idx_offset = 0;
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImplDX12SetupRenderState(draw_data, pCommandList, fr);
				else
					pcmd->UserCallback(cmd_list, pcmd);
				calledDraw = true;
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;

				// Apply Scissor/clipping rectangle, Bind texture, Draw
				const D3D12_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
				Texture* pTexture = (Texture*)pcmd->GetTexID();
				if (pTexture) // Binding at offset 0 works because we only bind one texture at a time, and because we update the gpu descriptor heap for each draw call.
					pCommandList->BindTexture(1, 0, m_TrackedTextureResources[pTexture]);
				pCommandList->SetScissorRect(r);
				pCommandList->DrawIndexInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
				calledDraw = true;
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}

	if (!calledDraw)
	{
		pCommandList->FlushResourceBarriers();
		FreeAllTextureResources();
		TrackTextureResource(bd->pFontTexture); // Font texture should always be tracked.
	}
}

void RS::ImGuiRenderer::ImplDX12CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Upload texture to graphics system
	{

		auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
		auto pCommandList = pCommandQueue->GetCommandList();

		bd->pFontTexture = pCommandList->CreateTexture((uint32)width, (uint32)height, pixels, DXGI_FORMAT_R8G8B8A8_UNORM, "ImGui Font Texture");

		uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);
		// Wait for load to finish.
		pCommandQueue->WaitForFenceValue(fenceValue);
	}

	// Store our identifier
	// READ THIS IF THE STATIC_ASSERT() TRIGGERS:
	// - Important: to compile on 32-bit systems, this backend requires code to be compiled with '#define ImTextureID ImU64'.
	// - This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
	// [Solution 1] IDE/msbuild: in "Properties/C++/Preprocessor Definitions" add 'ImTextureID=ImU64' (this is what we do in the 'example_win32_direct12/example_win32_direct12.vcxproj' project file)
	// [Solution 2] IDE/msbuild: in "Properties/C++/Preprocessor Definitions" add 'IMGUI_USER_CONFIG="my_imgui_config.h"' and inside 'my_imgui_config.h' add '#define ImTextureID ImU64' and as many other options as you like.
	// [Solution 3] IDE/msbuild: edit imconfig.h and add '#define ImTextureID ImU64' (prefer solution 2 to create your own config file!)
	// [Solution 4] command-line: add '/D ImTextureID=ImU64' to your cl.exe command-line (this is what we do in the example_win32_direct12/build_win32.bat file)
	static_assert(sizeof(ImTextureID) >= sizeof(bd->pFontTexture.get()), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
	ImTextureID fontTextureID = ImGuiRenderer::Get()->GetImTextureID(bd->pFontTexture);
	io.Fonts->SetTexID(fontTextureID);
}

bool RS::ImGuiRenderer::ImplDX12CreateDeviceObjects()
{
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	if (!bd)
		return false;
	if (bd->pPipelineState)
		ImplDX12InvalidateDeviceObjects();

	ImplDX12CreatePipelineState(bd);
}

void RS::ImGuiRenderer::ImplDX12InvalidateDeviceObjects()
{
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	if (!bd)
		return;

	ImGuiIO& io = ImGui::GetIO();
	bd->pRootSignature.reset();
	bd->pFontTexture.reset();
	if (bd->pPipelineState)
	{
		bd->pPipelineState->Release();
		bd->pPipelineState = nullptr;
	}
	io.Fonts->SetTexID(nullptr); // We copied bd->pFontTexture to io.Fonts->TexID so let's clear that as well.

}

RS::ImGuiRenderer::ImplDX12BackendRendererData* RS::ImGuiRenderer::ImplDX12GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImplDX12BackendRendererData*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

void RS::ImGuiRenderer::ImplDX12InitPlatformInterface()
{
	LOG_CRITICAL("ImplDX12InitPlatformInterface is not implemented");
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	platform_io.Renderer_CreateWindow = ImplDX12CreateWindow;
	platform_io.Renderer_DestroyWindow = ImplDX12DestroyWindow;
	platform_io.Renderer_SetWindowSize = ImplDX12SetWindowSize;
	platform_io.Renderer_RenderWindow = ImplDX12RenderWindow;
	platform_io.Renderer_SwapBuffers = ImplDX12SwapBuffers;
}

void RS::ImGuiRenderer::ImplDX12ShutdownPlatformInterface()
{
	ImGui::DestroyPlatformWindows();
}

void RS::ImGuiRenderer::ImplDX12SwapBuffers(ImGuiViewport* viewport, void*)
{
	LOG_CRITICAL("ImplDX12SwapBuffers is not implemented");
}

void RS::ImGuiRenderer::ImplDX12RenderWindow(ImGuiViewport* viewport, void*)
{
	LOG_CRITICAL("ImplDX12RenderWindow is not implemented");
}

void RS::ImGuiRenderer::ImplDX12SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
	LOG_CRITICAL("ImplDX12SetWindowSize is not implemented");
}

void RS::ImGuiRenderer::ImplDX12DestroyWindow(ImGuiViewport* viewport)
{
	LOG_CRITICAL("ImplDX12DestroyWindow is not implemented");
}

void RS::ImGuiRenderer::WaitForPendingOperations(ImplDX12ViewportData* vd)
{
	LOG_CRITICAL("WaitForPendingOperations is not implemented");
}

void RS::ImGuiRenderer::ImplDX12CreateWindow(ImGuiViewport* viewport)
{
	LOG_CRITICAL("ImplDX12CreateWindow is not implemented");
}

void RS::ImGuiRenderer::TrackTextureResource(std::shared_ptr<RS::Texture> pTexture)
{
	m_TrackedTextureResources[pTexture.get()] = pTexture;
}

void RS::ImGuiRenderer::FreeAllTextureResources()
{
	m_TrackedTextureResources.clear();
}
