#include "PreCompiled.h"
#include "PortAudio.h"

RS::PortAudio::PortAudio()
{
}

RS::PortAudio::~PortAudio()
{
}

void RS::PortAudio::Init()
{
    m_ErrorCode = Pa_Initialize();
    PORT_AUDIO_CHECK(m_ErrorCode, "Failed to initialize PortAudio!");

    m_DeviceIndex = Pa_GetDefaultOutputDevice();
    m_pDeviceInfo = Pa_GetDeviceInfo(m_DeviceIndex);
}

void RS::PortAudio::Destroy()
{
    if (m_ErrorCode == paNoError)
    {
        Pa_Terminate();
        RS_LOG_INFO("Destroyed PortAudio");
    }
}

PaDeviceIndex RS::PortAudio::GetDeviceIndex()
{
    return m_DeviceIndex;
}

const PaDeviceInfo* RS::PortAudio::GetDeviceInfo()
{
    return m_pDeviceInfo;
}
