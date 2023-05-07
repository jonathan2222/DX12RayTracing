#include "PreCompiled.h"
#include "ImGuiRenderer.h"

#include "GUI/ImGuiAdapter.h"
#include <backends/imgui_impl_dx12.h>

#include "DX12/Dx12Core2.h"
#include "Core/Display.h"

RS::ImGuiRenderer* RS::ImGuiRenderer::Get()
{
	static std::unique_ptr<RS::ImGuiRenderer> pImGuiRenderer{ std::make_unique<RS::ImGuiRenderer>() };
	return pImGuiRenderer.get();
}

void RS::ImGuiRenderer::Init()
{
	InternalResize();
}

void RS::ImGuiRenderer::FreeDescriptor()
{
	DX12::Dx12DescriptorHeap* srvHeap = DX12::Dx12Core2::Get()->GetDescriptorHeapGPUResources();
	srvHeap->Free(m_ImGuiFontDescriptorHandle);
	m_ImGuiFontDescriptorHandle = {};
}

void RS::ImGuiRenderer::Release()
{
	ImGui_ImplDX12_Shutdown();
	ImGuiAdapter::Release();
	ImGui::DestroyContext();

	m_IsReleased = true;
}

void RS::ImGuiRenderer::Draw(std::function<void(void)> callback)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_DrawCalls.emplace_back(callback);
}

void RS::ImGuiRenderer::Render()
{
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

	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		for (auto& drawCall : m_DrawCalls)
			drawCall();
	}

	DX12::Dx12DescriptorHeap* srvHeap = DX12::Dx12Core2::Get()->GetDescriptorHeapGPUResources();
	ID3D12GraphicsCommandList* pCommandList = DX12::Dx12Core2::Get()->GetFrameCommandList()->GetCommandList();

	// Rendering
	ImGui::Render();

	// This is necessary if it has already been set.
	ID3D12DescriptorHeap* pHeaps[1] = { srvHeap->GetHeap() };
	pCommandList->SetDescriptorHeaps(1, pHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCommandList);

	// Clear the call stack
	m_DrawCalls.clear();
}

bool RS::ImGuiRenderer::WantKeyInput()
{
	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

void RS::ImGuiRenderer::Resize()
{
	m_ShouldRescale = true;
}

float RS::ImGuiRenderer::GetGuiScale()
{
	return m_Scale;
}

void RS::ImGuiRenderer::InternalInit()
{
	m_IsReleased = false;

	DXGI_FORMAT format = DX12::Dx12Core2::Get()->GetMainSurface()->GetFormat();
	DX12::Dx12DescriptorHeap* srvHeap = DX12::Dx12Core2::Get()->GetDescriptorHeapGPUResources();

	m_ImGuiFontDescriptorHandle = srvHeap->Allocate();
#ifdef RS_CONFIG_DEBUG
	m_ImGuiFontDescriptorHandle.SetDebugName("ImGui Font descriptor");
#endif

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
	ImGui_ImplDX12_Init(DX12::Dx12Core2::Get()->GetD3D12Device(), FRAME_BUFFER_COUNT, format, srvHeap->GetHeap(), m_ImGuiFontDescriptorHandle.m_Cpu, m_ImGuiFontDescriptorHandle.m_Gpu);

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
			DX12::Dx12Core2::Get()->GetFrameCommandList()->WaitForGPUQueue();
			FreeDescriptor();
			Release();
		}
		InternalInit();

		m_OldWidth = width;
		m_OldHeight = height;
	}
}

void RS::ImGuiRenderer::ReScale(uint32 width, uint32 height)
{
	ImGuiStyle& style = ImGui::GetStyle();
	float widthScale = width / 1920.f;
	float heightScale = height / 1080.f;
	m_Scale = widthScale > heightScale ? widthScale : heightScale;
	style.ScaleAllSizes(m_Scale);

	LOG_INFO("ImGui Scale: {}", m_Scale);
}
