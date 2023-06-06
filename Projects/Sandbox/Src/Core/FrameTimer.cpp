#include "PreCompiled.h"
#include "FrameTimer.h"

#include "Core/Console.h"

using namespace RS;

void FrameTimer::Init(FrameStats* pFrameStats, float updateDelay)
{
    m_pFrameStats = pFrameStats;
    m_FixedDT = 1.f / pFrameStats->fixedUpdate.fixedFPS;
    m_UpdateFrameDataTime = updateDelay;

    Console::Get()->AddVar("FrameTimer.UpdateFrameDataTime", m_UpdateFrameDataTime, Console::Flag::NONE, "Time interval between each update of frame stats [seconds]");
    Console::Get()->AddVar("FrameStats.FixedUpdate.FixedFPS", m_pFrameStats->fixedUpdate.fixedFPS, Console::Flag::NONE, "Fixed update rate.");
    Console::Get()->AddVar("FrameStats.FixedUpdate.MaxUpdateCallsPerFrame", m_pFrameStats->fixedUpdate.maxUpdateCalls, Console::Flag::NONE, "Maximum number of fixed update calls per frame.");
    Console::Get()->AddVar("FrameStats.Info.FixedUpdate.UpdateCallsRatio", m_pFrameStats->fixedUpdate.updateCallsRatio, Console::Flag::ReadOnly, "");
    Console::Get()->AddVar("FrameStats.Info.Frame.AvrageFPS", m_pFrameStats->frame.avgFPS, Console::Flag::ReadOnly, "Average FPS [frames/s]");
    Console::Get()->AddVar("FrameStats.Info.Frame.AvrageDeltaTimeMs", m_pFrameStats->frame.avgDTMs, Console::Flag::ReadOnly, "Average delta time [ms]");
    Console::Get()->AddVar("FrameStats.Info.Frame.CurrentDeltaTime", m_pFrameStats->frame.currentDT, Console::Flag::ReadOnly, "Current delta time [s]");
    Console::Get()->AddVar("FrameStats.Info.Frame.MinDeltaTime", m_pFrameStats->frame.minDT, Console::Flag::ReadOnly, "Minimum delta time [s]");
    Console::Get()->AddVar("FrameStats.Info.Frame.MaxDeltaTime", m_pFrameStats->frame.maxDT, Console::Flag::ReadOnly, "Maximum delta time [s]");

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
    m_FixedDT = 1.f / m_pFrameStats->fixedUpdate.fixedFPS;

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
