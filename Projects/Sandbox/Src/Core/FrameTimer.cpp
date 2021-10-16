#include "PreCompiled.h"
#include "FrameTimer.h"

using namespace RS;

void FrameTimer::Init(FrameStats* pFrameStats, float updateDelay)
{
    m_pFrameStats = pFrameStats;
    m_FixedDT = 1.f / pFrameStats->fixedUpdate.fixedFPS;
    m_UpdateFrameDataTime = updateDelay;

    m_Timer.Start();
}

void FrameTimer::Begin()
{
    m_FrameTime = m_Timer.CalcDelta();
    m_pFrameStats->frame.currentDT = m_FrameTime.GetDeltaTimeSec();
    m_Accumulator += m_pFrameStats->frame.currentDT;
}

void FrameTimer::FixedTick(std::function<void(void)> fixedTickFunction)
{
    m_UpdateCalls = 0;
    while (m_Accumulator > m_FixedDT && m_UpdateCalls < m_pFrameStats->fixedUpdate.maxUpdateCalls)
    {
        fixedTickFunction();
        m_Accumulator -= m_FixedDT;
        m_UpdateCalls++;
    }
    m_AccUpdateCalls += m_UpdateCalls;
}

void FrameTimer::End()
{
    m_pFrameStats->frame.minDT = std::min(m_pFrameStats->frame.minDT, m_pFrameStats->frame.currentDT * 1000.f);
    m_pFrameStats->frame.maxDT = std::max(m_pFrameStats->frame.maxDT, m_pFrameStats->frame.currentDT * 1000.f);
    m_DebugFrameCounter++;
    m_DebugTimer += m_pFrameStats->frame.currentDT;
    if (m_DebugTimer >= m_UpdateFrameDataTime)
    {
        m_pFrameStats->frame.avgFPS = 1.0f / (m_DebugTimer / (float)m_DebugFrameCounter);
        m_pFrameStats->frame.avgDTMs = (m_DebugTimer / (float)m_DebugFrameCounter) * 1000.f;
        m_pFrameStats->fixedUpdate.updateCallsRatio = ((float)m_AccUpdateCalls / (float)m_DebugFrameCounter) * 100.f;
        m_DebugTimer = 0.0f;
        m_DebugFrameCounter = 0;
        m_AccUpdateCalls = 0;
    }
}
