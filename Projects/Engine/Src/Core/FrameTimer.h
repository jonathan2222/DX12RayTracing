#pragma once
#pragma once

#include "Core/FrameStats.h"
#include "Utils/Timer.h"

#include <functional>

namespace RS
{
    class FrameTimer
    {
    public:
        RS_DEFAULT_CONSTRUCTORS(FrameTimer);

        void Init(FrameStats* pFrameStats, float updateDelay);

        void Begin();
        void FixedTick(std::function<void(void)> fixedTickFunction);
        void End();

    private:
        FrameStats* m_pFrameStats = nullptr;

        float       m_FixedDT = 0.f;  // In seconds
        float       m_UpdateFrameDataTime = 0.f;  // In seconds
        Timer       m_Timer;
        TimeStamp   m_FrameTime;

        float   m_Accumulator = 0.0f;
        float   m_DebugTimer = 0.0f;
        uint32  m_DebugFrameCounter = 0;
        uint32  m_UpdateCalls = 0;
        uint32  m_AccUpdateCalls = 0;
    };
}