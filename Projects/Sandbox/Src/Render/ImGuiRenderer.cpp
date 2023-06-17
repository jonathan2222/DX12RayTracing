#include "PreCompiled.h"
#include "ImGuiRenderer.h"

#include "GUI/ImGuiAdapter.h"
#include <backends/imgui_impl_dx12.h>

#include "DX12/NewCore/DX12Core3.h"
#include "Core/Display.h"
#include "Core/Console.h"

#include "Editor/Editor.h"

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

	//DX12::Dx12DescriptorHeap* srvHeap = DX12Core2::Get()->GetDescriptorHeapGPUResources();
	//srvHeap->Free(m_ImGuiFontDescriptorHandle);
	//m_ImGuiFontDescriptorHandle = {};
}

void RS::ImGuiRenderer::Release()
{
	ImGui_ImplDX12_Shutdown();
	ImGuiAdapter::Release();
	ImGui::DestroyContext();
	if (m_SRVHeap)
	{
		m_SRVHeap->Release();
		m_SRVHeap = nullptr;
	}

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

	ImGui_ImplDX12_NewFrame();
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

	//DX12::Dx12DescriptorHeap* srvHeap = DX12::Dx12Core2::Get()->GetDescriptorHeapGPUResources();
	auto pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
	auto pCommandList = pCommandQueue->GetCommandList();
	auto pD3D12CommandList = pCommandList->GetGraphicsCommandList().Get();

	// Rendering
	ImGui::Render();

	// This is necessary if it has already been set.
	pD3D12CommandList->SetDescriptorHeaps(1, &m_SRVHeap);

	auto pRenderTarget = DX12Core3::Get()->GetSwapChain()->GetCurrentRenderTarget();
	pCommandList->SetRenderTarget(pRenderTarget);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pD3D12CommandList);

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

void RS::ImGuiRenderer::InternalInit()
{
	m_IsInitialized = true;
	m_IsReleased = false;

	DescriptorAllocation alloc = DX12Core3::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
	auto pCommandList = pCommandQueue->GetCommandList();
	//pCommandList->

	DXGI_FORMAT format = DX12Core3::Get()->GetSwapChain()->GetFormat();

	{
		auto pDevice = DX12Core3::Get()->GetD3D12Device();
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.NodeMask = 0;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		DXCall(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SRVHeap)));
	}

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
	ImGui_ImplDX12_Init(DX12Core3::Get()->GetD3D12Device(), FRAME_BUFFER_COUNT, format, m_SRVHeap, m_SRVHeap->GetCPUDescriptorHandleForHeapStart(), m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

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

		RSE::Editor::Get()->Resize(width, height);

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
