#pragma once

#include <renderdoc/renderdoc_app.h>

namespace RS
{
    class RenderDoc
    {
    public:
        void Init();
        void TakeCapture();
        void TakeMultiFrameCapture(uint32 numFrames);

        void StartFrameCapture();
        void EndFrameCapture();

    private:
        bool FetchAPI();
        void RegisterCommands();

        // RenderDoc
        RENDERDOC_API_1_1_2* m_pRenderDocAPI = NULL;

        uint32 m_FrameCounter = 0;
        uint32 m_CaptureFrame = 0;
        uint32 m_MaxCaptureFrames = 1;
    };
}