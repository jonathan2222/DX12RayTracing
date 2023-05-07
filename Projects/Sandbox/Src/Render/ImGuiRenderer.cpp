#include "PreCompiled.h"
#include "ImGuiRenderer.h"

#include "GUI/ImGuiAdapter.h"
#include <backends/imgui_impl_dx12.h>

#include "DX12/Dx12Core2.h"
#include "Core/Display.h"

RS::ImGuiRenderer* RS::ImGuiRenderer::Get()
{
	static std::unique_ptr<RS::ImGuiRenderer> pImGuiRenderer{ std::make_unique<RS::ImGuiRenderer>() };
#ifdef RS_CONFIG_DEBUG
	RS_ASSERT(pImGuiRenderer.get()->m_IsReleased == false, "Trying to access a released ImGuiRenderer!");
#endif
	return pImGuiRenderer.get();
}

void RS::ImGuiRenderer::Init()
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

	ImGuiAdapter::Init(Display::Get()->GetGLFWWindow(), true, ImGuiAdapter::ClientAPI::DX12);
	ImGui_ImplDX12_Init(DX12::Dx12Core2::Get()->GetD3D12Device(), FRAME_BUFFER_COUNT, format, srvHeap->GetHeap(), m_ImGuiFontDescriptorHandle.m_Cpu, m_ImGuiFontDescriptorHandle.m_Gpu);
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
}

void RS::ImGuiRenderer::Draw(std::function<void(void)> callback)
{
	std::lock_guard<std::mutex> lock(s_Mutex);
	s_DrawCalls.emplace_back(callback);
}

void RS::ImGuiRenderer::Render()
{
	if (s_ShouldRescale)
	{
		// Resize only, after BeginFrame set s_ShouldRescale to false.
		s_ShouldRescale = false;
		InternalResize();
	}

	ImGui_ImplDX12_NewFrame();
	ImGuiAdapter::NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = s_Scale;

	{
		std::lock_guard<std::mutex> lock(s_Mutex);
		for (auto& drawCall : s_DrawCalls)
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
	s_DrawCalls.clear();
}

bool RS::ImGuiRenderer::WantKeyInput()
{
	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

void RS::ImGuiRenderer::Resize()
{
	s_ShouldRescale = true;
}

float RS::ImGuiRenderer::GetGuiScale()
{
	return s_Scale;
}

void RS::ImGuiRenderer::InternalResize()
{
	auto pDisplay = RS::Display::Get();
	uint32 width = pDisplay->GetWidth();
	uint32 height = pDisplay->GetHeight();
	if (width != 0 && height != 0)
	{
		Release();
		Init();
	}
}

void RS::ImGuiRenderer::ReScale(uint32 width, uint32 height)
{
	ImGuiStyle& style = ImGui::GetStyle();
	float widthScale = width / 1920.f;
	float heightScale = height / 1080.f;
	s_Scale = widthScale > heightScale ? widthScale : heightScale;
	style.ScaleAllSizes(s_Scale);

	LOG_INFO("ImGui Scale: {}", s_Scale);
}
