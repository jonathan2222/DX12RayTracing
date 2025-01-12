#include "PreCompiled.h"
#include "Sound.h"

#include "PortAudio.h"
#include "AudioSystem.h"

#include "Core/LaunchArguments.h"

#include "Core/CorePlatform.h"

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

	// Threading
	static uint s_SoundCounter = 0;
	std::string threadName = Utils::Format("SoundThread #{}", s_SoundCounter++);
	m_pThread = new std::thread(&RS::Sound::ThreadFunction, this, threadName);
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

	// Stop and delete thread
	m_Running = false;
	if (m_pThread == nullptr)
		return;

	m_CV.notify_one();
	m_pThread->join();
	delete m_pThread;
	m_pThread = nullptr;
}

void RS::Sound::Play()
{
	if (LaunchArguments::Contains(LaunchParams::noSound))
		return;

	std::unique_lock<std::mutex> readLock(m_Mutex);
	QueueItem item;
	item.m_Type = ItemType::Play;
	item.m_pSound = this;
	m_TaskQueue.push(item);
	m_CV.notify_one();
}

void RS::Sound::Pause()
{
	if (LaunchArguments::Contains(LaunchParams::noSound))
		return;

	std::unique_lock<std::mutex> readLock(m_Mutex);
	QueueItem item;
	item.m_Type = ItemType::Pause;
	item.m_pSound = this;
	m_TaskQueue.push(item);
	m_CV.notify_one();
}

void RS::Sound::Unpause()
{
	Play();
}

void RS::Sound::Stop()
{
	if (LaunchArguments::Contains(LaunchParams::noSound))
		return;

	std::unique_lock<std::mutex> readLock(m_Mutex);
	QueueItem item;
	item.m_Type = ItemType::Stop;
	item.m_pSound = this;
	m_TaskQueue.push(item);
	m_CV.notify_one();
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

void RS::Sound::ThreadFunction(const std::string& threadName)
{
	CorePlatform::SetCurrentThreadName(threadName);

	while (m_Running)
	{
		Sound::QueueItem item;
		{
			std::unique_lock<std::mutex> readLock(m_Mutex);
			m_CV.wait(readLock, [&] { return !m_TaskQueue.empty() || !m_Running; });
			if (!m_Running)
				break;
			item = m_TaskQueue.front();
			m_TaskQueue.pop();
		}

		Sound* pSound = item.m_pSound;
		if (item.m_Type == Sound::ItemType::None
			|| pSound == nullptr)
			continue;

		if (item.m_Type == Sound::ItemType::Play)
		{
			// Stop previous sound
			if (pSound->m_IsStreamOn)
			{
				PORT_AUDIO_CHECK(Pa_StopStream(pSound->m_pStream), "Failed to stop stream!");
				PCM::SetPos(&pSound->m_pUserData->handle, 0);
				pSound->m_pUserData->finished = false;
				pSound->m_IsStreamOn = false;
			}

			// Play it
			PORT_AUDIO_CHECK(Pa_StartStream(pSound->m_pStream), "Failed to start stream!");
			pSound->m_IsStreamOn = true;
		}
		else if (item.m_Type == Sound::ItemType::Stop)
		{
			// Stop previous sound
			if (pSound->m_IsStreamOn)
			{
				PORT_AUDIO_CHECK(Pa_StopStream(pSound->m_pStream), "Failed to stop stream!");
				PCM::SetPos(&pSound->m_pUserData->handle, 0);
				pSound->m_pUserData->finished = false;
				pSound->m_IsStreamOn = false;
			}
		}
		else if (item.m_Type == Sound::ItemType::Pause)
		{
			// Stop previous sound
			if (pSound->m_IsStreamOn)
			{
				PORT_AUDIO_CHECK(Pa_StopStream(pSound->m_pStream), "Failed to pause stream!");
				PCM::SetPos(&pSound->m_pUserData->handle, pSound->m_pUserData->handle.pos);
				pSound->m_pUserData->finished = false;
				pSound->m_IsStreamOn = false;
			}
		}
	}
}
