#include "PreCompiled.h"
#include "Sound.h"

#include "PortAudio.h"
#include "AudioSystem.h"

RS::Sound::Sound(PortAudio* pPortAudioPtr)
    : m_pPortAudioPtr(pPortAudioPtr)
{
}

RS::Sound::~Sound()
{
    Destroy();
}

void RS::Sound::Init(PCM::UserData* pUserData)
{
	m_pUserData = pUserData;
	//m_pUserData->delayBuffer.Init(pUserData->handle.sampleRate);

	PaStreamParameters outputParameters;
	outputParameters.device = m_pPortAudioPtr->GetDeviceIndex();
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = m_pUserData->sampleFormat;
	outputParameters.suggestedLatency = m_pPortAudioPtr->GetDeviceInfo()->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	// Create stream
	PaError err = Pa_OpenStream(
		&m_pStream,
		NULL,
		&outputParameters,
		m_pUserData->handle.sampleRate,
		paFramesPerBufferUnspecified,
		paClipOff,
		PCM::PaCallbackPCM,
		m_pUserData);
	PORT_AUDIO_CHECK(err, "Failed to open default stream!");
}

void RS::Sound::Destroy()
{
	if (IsCreated())
	{
		Stop();
		PORT_AUDIO_CHECK(Pa_CloseStream(m_pStream), "Failed to close stream!");
		m_pStream = nullptr;

		// Delete sound data.
		if (m_pUserData->handle.asEffect)
		{
			if (m_pUserData->handle.directDataF32 != nullptr)
			{
				if (m_pUserData->handle.type == SoundHandle::TYPE_MP3)
					drmp3_free(m_pUserData->handle.directDataF32, nullptr);
				if (m_pUserData->handle.type == SoundHandle::TYPE_WAV)
					drwav_free(m_pUserData->handle.directDataF32, nullptr);
			}
			if (m_pUserData->handle.directDataI16 != nullptr)
			{
				if (m_pUserData->handle.type == SoundHandle::TYPE_MP3)
					drmp3_free(m_pUserData->handle.directDataI16, nullptr);
				if (m_pUserData->handle.type == SoundHandle::TYPE_WAV)
					drwav_free(m_pUserData->handle.directDataI16, nullptr);
			}
		}
		else
		{
			if (m_pUserData->handle.type == SoundHandle::TYPE_MP3)
				drmp3_uninit(&m_pUserData->handle.mp3);
			if (m_pUserData->handle.type == SoundHandle::TYPE_WAV)
				drwav_uninit(&m_pUserData->handle.wav);
		}

		// Clear filters.
		for (Filter* filter : m_pUserData->filters)
		{
			filter->Destroy();
			delete filter;
		}
		m_pUserData->filters.clear();

		//m_pUserData->delayBuffer.destroy();
		if (m_pUserData)
		{
			delete m_pUserData;
			m_pUserData = nullptr;
		}
	}
}

void RS::Sound::Play()
{
	Stop();
	//m_pUserData->delayBuffer.clear();
	PORT_AUDIO_CHECK(Pa_StartStream(m_pStream), "Failed to start stream!");
	m_IsStreamOn = true;
}

void RS::Sound::Pause()
{
	if (m_IsStreamOn)
	{
		PORT_AUDIO_CHECK(Pa_StopStream(m_pStream), "Failed to stop stream!");
		PCM::SetPos(&m_pUserData->handle, m_pUserData->handle.pos);
		m_pUserData->finished = false;
		m_IsStreamOn = false;
	}
}

void RS::Sound::Unpause()
{
	Play();
}

void RS::Sound::Stop()
{
	if (m_IsStreamOn)
	{
		PORT_AUDIO_CHECK(Pa_StopStream(m_pStream), "Failed to stop stream!");
		PCM::SetPos(&m_pUserData->handle, 0);
		m_pUserData->finished = false;
		m_IsStreamOn = false;
	}
}

void RS::Sound::SetName(const std::string& name)
{
	m_Name = name;
}

std::string RS::Sound::GetName() const
{
    return m_Name;
}

glm::vec3 RS::Sound::GetPosition() const
{
    return m_pUserData->soundData.sourcePos;
}

float RS::Sound::GetVolume() const
{
    return m_Volume;
}

void RS::Sound::ApplyVolume(float volumeChange)
{
	m_Volume += volumeChange;
	SetVolume(std::clamp(m_Volume, 0.f, 2.f));
}

void RS::Sound::SetVolume(float volume)
{
	m_Volume = volume;
	if (m_pUserData != nullptr)
	{
		m_pUserData->soundData.Volume = volume;
	}
}

void RS::Sound::SetGroupVolume(float volume)
{
	m_pUserData->soundData.GroupVolume = volume;
}

void RS::Sound::SetMasterVolume(float volume)
{
	m_pUserData->soundData.MasterVolume = volume;
}

void RS::Sound::SetLoop(bool state)
{
	m_pUserData->soundData.Loop = state;
}

void RS::Sound::AddFilter(Filter* pFilter)
{
	pFilter->Init(m_pUserData->handle.sampleRate);
	m_pUserData->filters.push_back(pFilter);
}

std::vector<RS::Filter*>& RS::Sound::GetFilters()
{
	return m_pUserData->filters;
}

void RS::Sound::SetSourcePosition(const glm::vec3& sourcePos)
{
	m_pUserData->soundData.sourcePos = sourcePos;
}

void RS::Sound::SetReceiverPosition(const glm::vec3& receiverPos)
{
	m_pUserData->soundData.receiverPos = receiverPos;
}

void RS::Sound::SetReceiverDir(const glm::vec3& receiverDir)
{
	RS_LOG_WARNING("SetReceiverDir is not implemented!");
	//m_pUserData->receiverDir = receiverDir;
}

void RS::Sound::SetReceiverLeft(const glm::vec3& receiverLeft)
{
	m_pUserData->soundData.receiverLeft = receiverLeft;
}

void RS::Sound::SetReceiverUp(const glm::vec3& receiverUp)
{
	m_pUserData->soundData.receiverUp = receiverUp;
}

void RS::Sound::SetReceiver(const glm::vec3& pos, const glm::vec3& right, const glm::vec3& up)
{
	SetReceiverPosition(pos);
	SetReceiverLeft(-right);
	SetReceiverUp(up);
}

bool RS::Sound::IsCreated() const
{
	return m_pStream != nullptr;
}
