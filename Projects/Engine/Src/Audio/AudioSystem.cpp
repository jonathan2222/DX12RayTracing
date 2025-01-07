#include "AudioSystem.h"
#include "PreCompiled.h"

#include "Sound.h"

#define DR_MP3_IMPLEMENTATION
#include "DR/dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "DR/dr_wav.h"

#include "PCMFunctions.h"

#include "Utils/Misc/StringUtils.h"

// For debug settings.
#include <imgui.h>
#include "Audio/Filters/DistanceFilter.h"
#include "Audio/Filters/EchoFilter.h"
#include "Audio/Filters/LowpassFilter.h"
#include "Audio/Filters/HighpassFilter.h"

RS::AudioSystem::AudioSystem()
{
}

RS::AudioSystem::~AudioSystem()
{
}

RS::AudioSystem* RS::AudioSystem::Get()
{
    static AudioSystem audioSystem;
    return &audioSystem;
}

void RS::AudioSystem::Init()
{
    m_pPortAudio = new PortAudio();
    m_pPortAudio->Init();
    RS_LOG_INFO("Initialized audio system.");
}

void RS::AudioSystem::Destroy()
{
	// Remove all sounds create by the system.
	for (Sound*& pSound : m_Sounds)
	{
		pSound->Destroy();
		delete pSound;
	}
	m_Sounds.clear();

	// Remove all soundStreams create by the system.
	for (Sound*& pStream : m_SoundStreams)
	{
		pStream->Destroy();
		delete pStream;
	}
	m_SoundStreams.clear();

	m_pPortAudio->Destroy();
	if (m_pPortAudio)
	{
		delete m_pPortAudio;
		m_pPortAudio = nullptr;
	}

	RS_LOG_INFO("Destroyed audio system.");
}

void RS::AudioSystem::Update()
{
}

void RS::AudioSystem::SetMasterVolume(float volume)
{
	m_MasterVolume = volume;
}

void RS::AudioSystem::SetStreamVolume(float volume)
{
	m_StreamVolume = volume;
}

void RS::AudioSystem::SetEffectsVolume(float volume)
{
	m_EffectsVolume = volume;
}

RS::Sound* RS::AudioSystem::CreateSound(const std::string& filePath)
{
	Sound* pSound = new Sound(m_pPortAudio);
	PCM::UserData* pUserData = new PCM::UserData();
	pUserData->finished = false;
	pUserData->sampleFormat = paFloat32;
	LoadEffectFile(&pUserData->handle, filePath, pUserData->sampleFormat);
	pSound->Init(pUserData);
	pSound->SetName(Utils::GetNameFromPath(filePath));
	m_Sounds.push_back(pSound);
	return pSound;
}

void RS::AudioSystem::RemoveSound(Sound* pSound)
{
	pSound->Destroy();
	m_Sounds.erase(std::remove(m_Sounds.begin(), m_Sounds.end(), pSound), m_Sounds.end());
	delete pSound;
}

RS::Sound* RS::AudioSystem::CreateStream(const std::string& filePath)
{
	Sound* pStream = new Sound(m_pPortAudio);
	PCM::UserData* pUserData = new PCM::UserData();
	pUserData->finished = false;
	pUserData->sampleFormat = paFloat32;
	pUserData->soundData.Loop = true;
	LoadStreamFile(&pUserData->handle, filePath);
	pStream->Init(pUserData);
	pStream->SetName(Utils::GetNameFromPath(filePath));
	m_SoundStreams.push_back(pStream);
	return pStream;
}

void RS::AudioSystem::RemoveStream(Sound* pSoundStream)
{
	pSoundStream->Destroy();
	m_SoundStreams.erase(std::remove(m_SoundStreams.begin(), m_SoundStreams.end(), pSoundStream), m_SoundStreams.end());
	delete pSoundStream;
}

void RS::AudioSystem::LoadStreamFile(SoundHandle* pSoundHandle, const std::string& filePath)
{
	pSoundHandle->asEffect = false;

	// Get file type.

	std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
	if (ext == "mp3") pSoundHandle->type = SoundHandle::Type::TYPE_MP3;
	if (ext == "wav") pSoundHandle->type = SoundHandle::Type::TYPE_WAV;

	// Load file.
	switch (pSoundHandle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
	{
		RS_ASSERT(drmp3_init_file(&pSoundHandle->mp3, filePath.c_str(), NULL), "Failed to load sound {} of type mp3!", filePath.c_str());
		pSoundHandle->nChannels = pSoundHandle->mp3.channels;
		pSoundHandle->sampleRate = pSoundHandle->mp3.sampleRate;
	}
	break;
	case SoundHandle::Type::TYPE_WAV:
	{
		RS_ASSERT(drwav_init_file(&pSoundHandle->wav, filePath.c_str(), NULL), "Failed to load sound {} of type wav!", filePath.c_str());
		pSoundHandle->nChannels = pSoundHandle->wav.channels;
		pSoundHandle->sampleRate = pSoundHandle->wav.sampleRate;
	}
	break;
	default:
		RS_ASSERT(false, "Failed to load sound {}, unrecognized file type!", filePath.c_str());
		break;
	}
}

void RS::AudioSystem::LoadEffectFile(SoundHandle* pSoundHandle, const std::string& filePath, PaSampleFormat format)
{
	pSoundHandle->asEffect = true;

	// Get file type.
	std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
	if (ext == "mp3") pSoundHandle->type = SoundHandle::Type::TYPE_MP3;
	if (ext == "wav") pSoundHandle->type = SoundHandle::Type::TYPE_WAV;

	// Load file.
	switch (pSoundHandle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
	{
		drmp3_config config = {};
		if (format == paFloat32)
			pSoundHandle->directDataF32 = drmp3_open_file_and_read_pcm_frames_f32(filePath.c_str(), &config, &pSoundHandle->totalFrameCount, NULL);
		else if (format == paInt16)
			pSoundHandle->directDataI16 = drmp3_open_file_and_read_pcm_frames_s16(filePath.c_str(), &config, &pSoundHandle->totalFrameCount, NULL);
		pSoundHandle->nChannels = config.channels;
		pSoundHandle->sampleRate = config.sampleRate;
	}
	break;
	case SoundHandle::Type::TYPE_WAV:
	{
		drmp3_config config = {};
		if (format == paFloat32)
			pSoundHandle->directDataF32 = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &pSoundHandle->nChannels, &pSoundHandle->sampleRate, &pSoundHandle->totalFrameCount, NULL);
		if (format == paInt16)
			pSoundHandle->directDataI16 = drwav_open_file_and_read_pcm_frames_s16(filePath.c_str(), &pSoundHandle->nChannels, &pSoundHandle->sampleRate, &pSoundHandle->totalFrameCount, NULL);
		RS_ASSERT(drwav_init_file(&pSoundHandle->wav, filePath.c_str(), NULL), "Failed to load sound {} of type wav!", filePath.c_str());
	}
	break;
	default:
		RS_ASSERT(false, "Failed to load sound {}, unrecognized file type!", filePath.c_str());
		break;
	}
}

void RS::AudioSystem::DrawAudioSettings()
{
	static bool my_tool_active = true;
	ImGui::Begin("Audio settings", &my_tool_active, ImGuiWindowFlags_MenuBar);
	ImGui::SliderFloat("Master volume", &m_MasterVolume, 0.0f, 1.f, "%.3f");

	uint32_t index = 0;
	if (ImGui::CollapsingHeader("Effects"))
	{
		ImGui::SliderFloat("Effects volume", &m_EffectsVolume, 0.0f, 1.f, "%.3f");
		for (Sound* pSound : m_Sounds)
		{
			pSound->SetGroupVolume(m_EffectsVolume);
			pSound->SetMasterVolume(m_MasterVolume);
			DrawSoundSettings(pSound, index);
			index++;
		}
	}

	if (ImGui::CollapsingHeader("Streams"))
	{
		ImGui::SliderFloat("Streams Volume", &m_StreamVolume, 0.0f, 1.f, "%.3f");
		for (Sound* pSound : m_SoundStreams)
		{
			pSound->SetGroupVolume(m_StreamVolume);
			pSound->SetMasterVolume(m_MasterVolume);
			DrawSoundSettings(pSound, index);
			index++;
		}
	}

	ImGui::End();
}

void RS::AudioSystem::DrawSoundSettings(Sound* pSound, uint32_t index)
{
	std::string name = "[" + std::to_string(index) + "]" + pSound->GetName();
	if (ImGui::TreeNode(name.c_str()))
	{
		float volume = pSound->GetVolume();
		ImGui::SliderFloat("Volume", &volume, 0.0f, 1.f, "%.3f");
		pSound->SetVolume(volume);

		auto& filters = pSound->GetFilters();
		for (Filter* pFilter : filters)
		{
			if (ImGui::CollapsingHeader(pFilter->GetName().c_str()))
			{
				if (LowpassFilter* pLowpassF = dynamic_cast<LowpassFilter*>(pFilter))
				{
					float fcLow = pLowpassF->GetCutoffFrequency();
					ImGui::SliderFloat("[Low]Cutoff frequency", &fcLow, 0.0f, pLowpassF->GetSampleRate() * 0.5f, "%.0f Hz");
					pLowpassF->SetCutoffFrequency(fcLow);
				}

				if (HighpassFilter* pHighpassF = dynamic_cast<HighpassFilter*>(pFilter))
				{
					float fcHigh = pHighpassF->GetCutoffFrequency();
					ImGui::SliderFloat("[High]Cutoff frequency", &fcHigh, 0.0f, pHighpassF->GetSampleRate() * 0.5f, "%.0f Hz");
					pHighpassF->SetCutoffFrequency(fcHigh);
				}

				if (EchoFilter* pEchoF = dynamic_cast<EchoFilter*>(pFilter))
				{
					float gain = pEchoF->GetGain();
					ImGui::SliderFloat("Gain", &gain, 0.0f, 1.f, "%.3f");
					pEchoF->SetGain(gain);
					float delay = pEchoF->GetDelay();
					ImGui::SliderFloat("Delay", &delay, 0.0f, (float)MAX_CIRCULAR_BUFFER_SIZE, "%.3f sec");
					pEchoF->SetDelay(delay);
				}

				if (DistanceFilter* pDistF = dynamic_cast<DistanceFilter*>(pFilter))
				{
					ImGui::Text("Uses factor = 1/distance^2 for sound attenuation.");
					glm::vec3 pos = pSound->GetPosition();
					ImGui::Text("Source position: (%f, %f, %f)", pos.x, pos.y, pos.z);
				}
			}
		}
		ImGui::TreePop();
	}
}
