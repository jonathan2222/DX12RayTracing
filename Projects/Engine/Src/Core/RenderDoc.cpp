#include "PreCompiled.h"
#include "RenderDoc.h"

#include "Core/Console.h"
#include "Core/CorePlatform.h"
#include "Core/LaunchArguments.h"
#include "Core/Display.h"

#include "DX12/Final/DXCore.h"

using namespace RS;

void RenderDoc::Init()
{
    RegisterCommands();

    if (!FetchAPI())
        return;

    m_pRenderDocAPI->MaskOverlayBits(RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None, 0);
}

bool RenderDoc::FetchAPI()
{
    if (m_pRenderDocAPI)
        return true;

    HMODULE mod = nullptr;
    if (LaunchArguments::Contains(LaunchParams::injectRenderDoc))
    {
        // TODO: Implement a way of finding this dll.
        const std::string renderDocPath = "C:\\Program Files\\RenderDoc\\renderdoc.dll";
        std::wstring path = Utils::ToWString(renderDocPath);
        mod = LoadLibrary(path.c_str());
        if (mod == nullptr)
        {
            std::string error = CorePlatform::GetLastErrorString();
            LOG_WARNING("Could not load RenderDoc module! {}", error.c_str());
            return false;
        }
    }
    else
    {
        HMODULE mod = GetModuleHandleA("renderdoc.dll");
        if (mod == nullptr)
        {
            LOG_WARNING("Could not get RenderDoc module!");
            return false;
        }
    }

    if (mod)
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        if (RENDERDOC_GetAPI == nullptr)
        {
            LOG_WARNING("Could not load RENDERDOC_GetAPI function!");
            return false;
        }
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&m_pRenderDocAPI);
        if (ret != 1)
        {
            LOG_WARNING("Could not load RenderDoc 1.1.2 API!");
            return false;
        }
    }

    // linux/android.
    // For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
    //if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
    //{
    //    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
    //    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&m_pRenderDocAPI);
    //    assert(ret == 1);
    //}

    return true;
}

void RenderDoc::TakeCapture()
{
    if (m_CaptureFrame != 0)
        return;

    m_MaxCaptureFrames = 1;
    m_CaptureFrame = m_FrameCounter;
}

void RenderDoc::TakeMultiFrameCapture(uint32 numFrames)
{
    if (m_CaptureFrame != 0)
        return;

    m_MaxCaptureFrames = std::max(1u, numFrames);
    m_CaptureFrame = m_FrameCounter;
}

void RS::RenderDoc::StartFrameCapture()
{
    m_FrameCounter++;

    if (m_CaptureFrame == 0)
        return;
    if (!FetchAPI())
        return;

    uint32 frameDiff = m_FrameCounter - m_CaptureFrame;

    if (frameDiff == 0 || frameDiff > m_MaxCaptureFrames)
        return;

    auto pDevice = DX12::DXCore::GetDevice();
    HWND hwnd = Display::Get()->GetHWND();
    m_pRenderDocAPI->StartFrameCapture(pDevice, hwnd);

}

void RS::RenderDoc::EndFrameCapture()
{
    if (m_CaptureFrame == 0)
        return;
    if (!FetchAPI())
        return;

    uint32 frameDiff = m_FrameCounter - m_CaptureFrame;
    if (frameDiff > m_MaxCaptureFrames)
        m_CaptureFrame = 0;

    if (frameDiff == 0 || frameDiff > m_MaxCaptureFrames)
        return;
    
    auto pDevice = DX12::DXCore::GetDevice();
    HWND hwnd = Display::Get()->GetHWND();
    uint32 result = m_pRenderDocAPI->EndFrameCapture(pDevice, hwnd);
    if (result != 1)
        return;

    // Launch GUI
    if (frameDiff == 1 && !m_pRenderDocAPI->IsTargetControlConnected())
    {
        uint32 index = 0;
        char filePath[512];
        uint32 code = m_pRenderDocAPI->GetCapture(index, filePath, NULL, NULL);
        std::string path = std::format("{}.log", filePath);
        m_pRenderDocAPI->LaunchReplayUI(1, path.c_str());
    }
    else
    {
        m_pRenderDocAPI->ShowReplayUI();
    }
}

void RenderDoc::RegisterCommands()
{
    Console::Get()->AddFunction("RenderDoc.Capture", [this](Console::FuncArgs args)->bool
        {
            if (!RS::Console::ValidateFuncArgs(args, { {Console::FuncArg::TypeFlag::Int} }, Console::ValidateFuncArgsFlag::TypeMatchOnly))
                return false;

            bool specifiesNumberOfFrames = !args.empty();
            if (specifiesNumberOfFrames)
                TakeMultiFrameCapture(args[0].value);
            else
                TakeCapture();

            return true;
        },
        Console::Flag::NONE, "Take a render doc capture.\nUse -n <num frames> to capture n frames."
    );

    Console::Get()->AddFunction("RenderDoc.GetAPIVersion", [this](Console::FuncArgs args)->bool
        {
            if (!FetchAPI())
                return false;
            int major = 0;
            int minor = 0;
            int patch = 0;
            m_pRenderDocAPI->GetAPIVersion(&major, &minor, &patch);
            Console::Get()->Print("{}.{}.{}", major, minor, patch);
            return true;
        },
        Console::Flag::NONE, "Returns the RenderDoc API version."
    );

    Console::Get()->AddFunction("RenderDoc.GetFilePathTemplate", [this](Console::FuncArgs args)->bool
        {
            if (!FetchAPI())
                return false;
            const char* filePath = m_pRenderDocAPI->GetCaptureFilePathTemplate();
            Console::Get()->Print(filePath);
            return true;
        },
        Console::Flag::NONE, "Returns the RenderDoc capture file path template."
    );

    Console::Get()->AddFunction("RenderDoc.ShowReplayUI", [this](Console::FuncArgs args)->bool
        {
            if (!FetchAPI())
                return false;
            m_pRenderDocAPI->ShowReplayUI();
            return true;
        },
        Console::Flag::NONE, "Raise the RenderDocUI window to the top."
    );

    Console::Get()->AddFunction("RenderDoc.GetNumCaptures", [this](Console::FuncArgs args)->bool
        {
            if (!FetchAPI())
                return false;
            uint32 numCaptures = m_pRenderDocAPI->GetNumCaptures();
            Console::Get()->Print("{}", numCaptures);
            return true;
        },
        Console::Flag::NONE, "Returns the number of captures."
    );

    Console::Get()->AddFunction("RenderDoc.GetCapture", [this](Console::FuncArgs args)->bool
        {
            if (!FetchAPI())
                return false;
            if (!RS::Console::ValidateFuncArgs(args, { {"-i"} }, false))
                return false;

            bool hasIndex = args.Get("-i").has_value();
            uint32 index = !hasIndex ? 0 : args.Get("-i").value().types.ui32;
            std::string filePath(256, '?');
            uint32 code = m_pRenderDocAPI->GetCapture(index, filePath.data(), NULL, NULL);
            if (code == 1)
                Console::Get()->Print(filePath.c_str());
            else
                Console::Get()->Print("Invalid capture index!");
            return true;
        },
        Console::Flag::NONE, "Get file path for a RenderDoc capture.\nUse -i <index> to get a specific capture, else takes index 0."
    );
}
