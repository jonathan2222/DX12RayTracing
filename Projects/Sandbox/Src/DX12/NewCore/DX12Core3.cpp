#include "PreCompiled.h"
#include "DX12Core3.h"

#include "Core/EngineLoop.h"

// TODO: Remove/Move these!
#include "Render/ImGuiRenderer.h"
#include "GUI/LogNotifier.h"
#include "Core/Console.h"

RS::DX12Core3::~DX12Core3()
{
}

void RS::DX12Core3::Flush()
{
    m_pDirectCommandQueue->Flush();
}

void RS::DX12Core3::WaitForGPU()
{
    Flush();
    uint64 fenceValue = m_pDirectCommandQueue->Signal();
    m_pDirectCommandQueue->WaitForFenceValue(fenceValue);

    for (uint32 frameIndex = 0; frameIndex < FRAME_BUFFER_COUNT; ++frameIndex)
        ReleasePendingResourceRemovals(frameIndex);
}

RS::DescriptorAllocator* RS::DX12Core3::GetDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return m_pDescriptorHeaps[type].get();
}

RS::DescriptorAllocation RS::DX12Core3::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return GetDescriptorAllocator(type)->Allocate(1);
}

void RS::DX12Core3::AddToLifetimeTracker(uint64 id, const std::string& name)
{
    std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
    s_ResourceLifetimeTrackingData[id] = LifetimeStats(name);
}

void RS::DX12Core3::RemoveFromLifetimeTracker(uint64 id)
{
    std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
    s_ResourceLifetimeTrackingData.erase(id);
}

void RS::DX12Core3::SetNameOfLifetimeTrackedResource(uint64 id, const std::string& name)
{
    std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
    strcpy_s(s_ResourceLifetimeTrackingData[id].name, name.c_str());
}

void RS::DX12Core3::LogLifetimeTracker()
{
    if (LaunchArguments::Contains(LaunchParams::logResources))
    {
        LOG_DEBUG("Resource Lifetime tracker:");
        std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
        uint64 currentTime = Timer::GetCurrentTimeSeconds();
        uint32 index = 0;
        for (auto& entry : s_ResourceLifetimeTrackingData)
        {
            const uint64 id = entry.first;
            LifetimeStats& stats = entry.second;
            LOG_DEBUG("[{}]\tID: {} [{}] AliveTime: {} s", index, id, stats.name, currentTime - stats.startTime);
            ++index;
        }

        uint32 frameIndex = RS::DX12Core3::Get()->GetCurrentFrameIndex();
        std::array<uint32, FRAME_BUFFER_COUNT> numPendingRemovals = RS::DX12Core3::Get()->GetNumberOfPendingPremovals();
        LOG_DEBUG("Penging Removals:");
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
            LOG_DEBUG("{}\t[{}] -> {}", frameIndex == i ? "x" : "", i, numPendingRemovals[i]);
        LOG_DEBUG("Global resource states: {}", RS::ResourceStateTracker::GetNumberOfGlobalResources());
    }
}

std::array<uint32, FRAME_BUFFER_COUNT> RS::DX12Core3::GetNumberOfPendingPremovals()
{
    std::lock_guard<std::mutex> lock(m_ResourceRemovalMutex);
    std::array<uint32, FRAME_BUFFER_COUNT> arrayData;
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
        arrayData[i] = (uint32)m_PendingResourceRemovals[i].size();
    return arrayData;
}

RS::DX12Core3::DX12Core3()
    : m_CurrentFrameIndex(0)
    , m_FenceValues{ 0 }
{
    const D3D_FEATURE_LEVEL d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    const DX12::DXGIFlags dxgiFlags = DX12::DXGIFlag::REQUIRE_TEARING_SUPPORT;
    m_Device.Init(d3dMinFeatureLevel, dxgiFlags);
}

RS::DX12Core3* RS::DX12Core3::Get()
{
    static std::unique_ptr<DX12Core3> pDX12Core(new DX12Core3());
    return pDX12Core.get();
}

void RS::DX12Core3::Init(HWND window, int width, int height)
{
    m_pDirectCommandQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_pDescriptorHeaps[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i), 256);

    // TODO: This should be able to be outside this calss. Think what happens when we have multiple swap chains.
    m_pSwapChain = std::make_unique<SwapChain>(window, width, height, m_Device.GetDXGIFlags());

    // TODO: Remove/Move these!
    {
        // ImGui
        ImGuiRenderer::Get()->Init();
    }
}

void RS::DX12Core3::Release()
{
    m_pSwapChain.reset();
    ImGuiRenderer::Get()->Release();
    WaitForGPU();
    DX12Core3::LogLifetimeTracker();
    m_Device.Release();
}

void RS::DX12Core3::FreeResource(Microsoft::WRL::ComPtr<ID3D12Resource> pResource)
{
    uint32 frameIndex = GetCurrentFrameIndex();
    std::lock_guard<std::mutex> lock(m_ResourceRemovalMutex);
    m_PendingResourceRemovals[frameIndex].push_back(pResource);
}

bool RS::DX12Core3::WindowSizeChanged(uint32 width, uint32 height, bool isFullscreen, bool windowed, bool minimized)
{
    bool isWindowVisible = width != 0 && height != 0 && !minimized;
    if (!isWindowVisible)
        return false;

    LOG_WARNING("Resize to: ({}, {}) isFullscreen: {}, windowed: {}", width, height, isFullscreen, windowed);

    m_pSwapChain->Resize(width, height, isFullscreen);
    ImGuiRenderer::Get()->Resize();
    return true;
}

void RS::DX12Core3::Render()
{
    const uint32 frameIndex = GetCurrentFrameIndex();
    ReleasePendingResourceRemovals(frameIndex);

    {
        static bool show_demo_window = true;
        static bool show_notification_window = true;
        static uint32_t position = ImGui::GetDefaultNotificationPosition();
        ImGuiRenderer::Get()->Draw([&]() {
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            if (show_notification_window)
            {
                ImGui::Begin("Notification Test", &show_notification_window);

                if (ImGui::Button("Warning"))
                    ImGui::InsertNotification({ ImGuiToastType_Warning, 3000, "Hello World! This is a warning! {}", 0x1337 });
                if (ImGui::Button("Success"))
                    ImGui::InsertNotification({ ImGuiToastType_Success, 3000, "Hello World! This is a success! {}", "We can also format here:)" });
                if (ImGui::Button("Error"))
                    ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "Hello World! This is an error! {:#10X}", 0xDEADBEEF });
                if (ImGui::Button("Info"))
                    ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Hello World! This is an info!" });
                if (ImGui::Button("Info Long"))
                    ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation" });

                if (ImGui::Button("Critical"))
                    ImGui::InsertNotification({ ImGuiToastType_Critical, "Hello World! This is an info!" });
                if (ImGui::Button("Debug"))
                    ImGui::InsertNotification({ ImGuiToastType_Debug, "Hello World! This is an info!" });

                if (ImGui::Button("Custom title"))
                {
                    // Now using a custom title...
                    ImGuiToast toast(ImGuiToastType_Success, 3000); // <-- content can also be passed here as above
                    toast.set_title("This is a {} title", "wonderful");
                    toast.set_content("Lorem ipsum dolor sit amet");
                    ImGui::InsertNotification(toast);
                }

                if (ImGui::Button("No dismiss"))
                {
                    ImGui::InsertNotification({ ImGuiToastType_Error, IMGUI_NOTIFY_NO_DISMISS, "Test 0x%X", 0xDEADBEEF }).bind([]()
                        {
                            ImGui::InsertNotification({ ImGuiToastType_Info, "Signaled" });
                        }
                    ).dismissOnSignal();
                }

                if (ImGui::BeginPopupContextWindow())
                {
                    if (ImGui::MenuItem("Top-left", NULL, position == ImGuiToastPos_TopLeft)) { position = ImGuiToastPos_TopLeft; ImGui::SetNotificationPosition(position); }
                    if (ImGui::MenuItem("Top-Center", NULL, position == ImGuiToastPos_TopCenter)) { position = ImGuiToastPos_TopCenter; ImGui::SetNotificationPosition(position); }
                    if (ImGui::MenuItem("Top-right", NULL, position == ImGuiToastPos_TopRight)) { position = ImGuiToastPos_TopRight; ImGui::SetNotificationPosition(position); }
                    if (ImGui::MenuItem("Bottom-left", NULL, position == ImGuiToastPos_BottomLeft)) { position = ImGuiToastPos_BottomLeft; ImGui::SetNotificationPosition(position); }
                    if (ImGui::MenuItem("Bottom-Center", NULL, position == ImGuiToastPos_BottomCenter)) { position = ImGuiToastPos_BottomCenter; ImGui::SetNotificationPosition(position); }
                    if (ImGui::MenuItem("Bottom-right", NULL, position == ImGuiToastPos_BottomRight)) { position = ImGuiToastPos_BottomRight; ImGui::SetNotificationPosition(position); }
                    ImGui::EndPopup();
                }

                ImGui::End();
            }

            // Render toasts on top of everything, at the end of your code!
            // You should push style vars here
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f); // Round borders
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
            ImGui::RenderNotifications(); // <-- Here we render all notifications
            ImGui::PopStyleVar(1); // Don't forget to Pop()
            ImGui::PopStyleColor(1);

            Console::Get()->Render();
            });

        // ImGui
        ImGuiRenderer::Get()->Render();
    }

    // Frame index is the same as the back buffer index.
    m_CurrentFrameIndex = m_pSwapChain->Present(nullptr);
}

void RS::DX12Core3::ReleaseStaleDescriptors()
{
    const uint64 frameNumber = EngineLoop::GetCurrentFrameNumber();

    for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_pDescriptorHeaps[i]->ReleaseStaleDescriptors(frameNumber);
}

uint32 RS::DX12Core3::GetCurrentFrameIndex() const
{
    return m_CurrentFrameIndex;
}

void RS::DX12Core3::ReleasePendingResourceRemovals(uint32 frameIndex)
{
    m_ResourceRemovalMutex.lock();
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingRemovals = m_PendingResourceRemovals[frameIndex];
    m_PendingResourceRemovals[frameIndex].clear();
    m_ResourceRemovalMutex.unlock();

    // Remove resources.
    for (auto resource : pendingRemovals)
    {
        ResourceStateTracker::RemoveGlobalResourceState(resource.Get());
        //resource->Release();
        resource.Reset();
    }
}
