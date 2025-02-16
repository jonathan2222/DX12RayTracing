#include "PreCompiled.h"
#include "ImGuiRenderer.h"

#include "GUI/ImGuiAdapter.h"
//#include <backends/imgui_impl_dx12.h>

#include "DX12/Final/DXCore.h"
#include "DX12/Final/DXCommandContext.h"
#include "DX12/Final/DXDisplay.h"
#include "DX12/Final/DXUploadBuffer.h"
#include "Core/Display.h"
#include "Core/Console.h"

#include "DX12/Final/DXShader.h"

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

void RS::ImGuiRenderer::Render(DX12::DXColorBuffer& colorBuffer, DX12::DXGraphicsContext* pContext)
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

	RS::DX12::DXGraphicsContext* context = pContext;
	if (pContext == nullptr)
		context = &RS::DX12::DXGraphicsContext::Begin(L"ImGUI");

	// Rendering
	ImGui::Render();

	context->SetRenderTarget(colorBuffer.GetRTV());

	context->BeginEvent("ImGui");
	ImplDX12RenderDrawData(ImGui::GetDrawData(), *context);
	context->EndEvent();

	if (pContext == nullptr)
		context->Finish();

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

ImTextureID RS::ImGuiRenderer::GetImTextureID(std::shared_ptr<DX12::DXTexture> pTexture)
{
	TrackTextureResource(pTexture);
	return (ImTextureID)pTexture.get();
}

void RS::ImGuiRenderer::InternalInit()
{
	m_IsInitialized = true;
	m_IsReleased = false;

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

	DXGI_FORMAT format = DX12::DXCore::GetDXDisplay()->GetFormat();
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
	faSolid900 = Engine::GetDataFilePath(true) + faSolid900;
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
			//DX12::DXCore::Flush();
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
		vd->renderBuffers.geometry.Destroy();
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
	br->rootSignature.Reset(2, 1);
	br->rootSignature[0].InitAsConstants(0, 16, D3D12_SHADER_VISIBILITY_VERTEX);
	br->rootSignature[1].InitAsDescriptorTable(1, D3D12_SHADER_VISIBILITY_PIXEL);
	br->rootSignature[1].SetTableRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);

	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0.f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor[0] = 0.f;
	samplerDesc.BorderColor[1] = 0.f;
	samplerDesc.BorderColor[2] = 0.f;
	samplerDesc.BorderColor[3] = 0.f;
	samplerDesc.MinLOD = 0.f;
	samplerDesc.MaxLOD = 0.f;
	br->rootSignature.InitStaticSampler(0, samplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	br->rootSignature.Finalize(L"ImGuiRenderer_RootSignature", rootSignatureFlags);
}

void RS::ImGuiRenderer::ImplDX12CreatePipelineState(ImplDX12BackendRendererData* br)
{
	DX12::DXShader shader;
	DX12::DXShader::Description shaderDesc{};
	shaderDesc.isInternalPath = true;
	shaderDesc.path = "ImGui/ImGuiShader.hlsl";
	shaderDesc.typeFlags = DX12::DXShader::TypeFlag::Pixel | DX12::DXShader::TypeFlag::Vertex;
	shaderDesc.customEntryPoints = { {DX12::DXShader::TypeFlag::Pixel, "PixelMain"}, {DX12::DXShader::TypeFlag::Vertex, "VertexMain"} };
	shader.Create(shaderDesc);

	ImplDX12CreateRootSignature(br);

	static D3D12_INPUT_ELEMENT_DESC pLocalLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	DX12::DXGraphicsPSO& pso = br->graphicsPSO;
	pso.SetRootSignature(br->rootSignature);
	pso.SetRasterizerState(RenderCore::RasterizerTwoSided);
	pso.SetBlendState(RenderCore::BlendTraditional);
	pso.SetDepthStencilState(RenderCore::DepthStateDisabled);
	pso.SetSampleMask(0xFFFFFFFF);
	pso.SetInputLayout(3, pLocalLayout);
	pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	pso.SetShader(shader);
	pso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
	pso.Finalize(false);

	shader.Release();

	ImplDX12CreateFontsTexture();
}

void RS::ImGuiRenderer::ImplDX12NewFrame()
{
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImplDX12Init()?");

	if (!bd->graphicsPSO.IsValid())
		ImplDX12CreateDeviceObjects();
}

void RS::ImGuiRenderer::ImplDX12SetupRenderState(ImDrawData* draw_data, DX12::DXGraphicsContext& context, ImplDX12RenderBuffers* fr)
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
	context.SetViewport(vp);
	context.SetScissor(0, 0, vp.Width, vp.Height);

	context.SetVertexBuffer(0, fr->vertexBufferView);
	context.SetIndexBuffer(fr->indexBufferView);

	context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context.SetPipelineState(bd->graphicsPSO);
	context.SetRootSignature(bd->rootSignature);

	context.SetConstantArray(0, 16, &vertex_constant_buffer);

	// Setup blend factor
	Color32 blendFactor(0.f, 0.f, 0.f, 0.f);
	context.SetBlendFactor(blendFactor);
}

void RS::ImGuiRenderer::ImplDX12RenderDrawData(ImDrawData* draw_data, DX12::DXGraphicsContext& context)
{
	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	ImplDX12ViewportData* vd = (ImplDX12ViewportData*)draw_data->OwnerViewport->RendererUserData;
	ImplDX12RenderBuffers* fr = &vd->renderBuffers;

	// Create and grow vertex/index buffers if needed
	if (!fr->geometry.IsValid() || (fr->allocatedVertexCount < draw_data->TotalVtxCount) || (fr->allocatedIndexCount < draw_data->TotalIdxCount))
	{
		const uint64 addedVertexCount = 5000;
		const uint64 newVertexCount = draw_data->TotalVtxCount + addedVertexCount;
		const uint64 addedIndexCount = 10000;
		const uint64 newIndexCount = draw_data->TotalIdxCount + addedIndexCount;

		DX12::DXUploadBuffer geometryUploadBuffer;
		const uint64 geometryBinarySize = newVertexCount * sizeof(ImDrawVert) + newIndexCount * sizeof(ImDrawIdx);
		fr->geometry.Create(L"ImGUI Geometry Buffer", geometryBinarySize, 1, nullptr, D3D12_HEAP_TYPE_UPLOAD);

		fr->allocatedVertexCount = newVertexCount;
		fr->allocatedIndexCount = newIndexCount;
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	uint8* pGeometryData = nullptr;
	if (!fr->geometry.Map(0, (void**)&pGeometryData, DX12::DataAccess::Write))
	{
		context.FlushResourceBarriers();
		FreeAllTextureResources();
		TrackTextureResource(bd->pFontTexture); // Font texture should always be tracked.
		return;
	}

	uint32 vertexCount = 0u;
	uint32 indexCount = 0u;
	ImDrawVert* vtx_dst = (ImDrawVert*)pGeometryData;
	ImDrawIdx* idx_dst = (ImDrawIdx*)(pGeometryData + fr->allocatedVertexCount * sizeof(ImDrawVert));
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
		vertexCount += cmd_list->VtxBuffer.Size;
		indexCount += cmd_list->IdxBuffer.Size;
	}
	fr->geometry.Unmap(0);

	// Update views
	fr->vertexBufferView = fr->geometry.VertexBufferView(0, vertexCount * sizeof(ImDrawVert), sizeof(ImDrawVert));
	fr->indexBufferView = fr->geometry.IndexBufferView(fr->allocatedVertexCount * sizeof(ImDrawVert), indexCount * sizeof(ImDrawIdx), false);

	// Setup desired DX state
	ImplDX12SetupRenderState(draw_data, context, fr);

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
					ImplDX12SetupRenderState(draw_data, context, fr);
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
				DX12::DXTexture* pTexture = (DX12::DXTexture*)pcmd->GetTexID();
				if (pTexture) // Binding at offset 0 works because we only bind one texture at a time, and because we update the gpu descriptor heap for each draw call.
					context.SetDynamicDescriptor(1, 0, m_TrackedTextureResources[pTexture]->GetSRV());
				context.SetScissor(r);
				context.DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
				calledDraw = true;
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}

	if (!calledDraw)
	{
		context.FlushResourceBarriers();
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
		RS::DX12::DXGraphicsContext& context = RS::DX12::DXGraphicsContext::Begin(L"ImGUI Init Font Context");
		bd->pFontTexture = std::make_shared<DX12::DXTexture>();
		bd->pFontTexture->Create2D((uint64)width * 4, (uint64)width, (uint64)height, DXGI_FORMAT_R8G8B8A8_UNORM, pixels);
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
	if (bd->graphicsPSO.IsValid())
		ImplDX12InvalidateDeviceObjects();

	ImplDX12CreatePipelineState(bd);
}

void RS::ImGuiRenderer::ImplDX12InvalidateDeviceObjects()
{
	ImplDX12BackendRendererData* bd = ImplDX12GetBackendData();
	if (!bd)
		return;

	ImGuiIO& io = ImGui::GetIO();
	//bd->rootSignature.DestroyAll();
	bd->pFontTexture.reset();
	//if (bd->graphicsPSO.IsValid())
	//{
	//	bd->pPipelineState->Release();
	//	bd->pPipelineState = nullptr;
	//}
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

void RS::ImGuiRenderer::TrackTextureResource(std::shared_ptr<RS::DX12::DXTexture> pTexture)
{
	m_TrackedTextureResources[pTexture.get()] = pTexture;
}

void RS::ImGuiRenderer::FreeAllTextureResources()
{
	m_TrackedTextureResources.clear();
}
