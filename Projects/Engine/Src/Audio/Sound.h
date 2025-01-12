#pragma once

#include "PCMFunctions.h"
#include "Filters/Filter.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace RS
{
	class PortAudio;
	class ChannelGroup;
	class Sound
	{
	public:
		Sound(PortAudio* pPortAudioPtr);
		~Sound();

		void Init(PCM::UserData* pUserData);
		void Destroy();

		void Play();
		void Pause();
		void Unpause();
		void Stop();

		void SetName(const std::string& name);
		std::string GetName() const;
		glm::vec3 GetPosition() const;

		float GetVolume() const;
		void ApplyVolume(float volumeChange);
		void SetVolume(float volume);
		void SetGroupVolume(float volume);
		void SetMasterVolume(float volume);
		void SetLoop(bool state);

		void AddFilter(Filter* pFilter);
		std::vector<Filter*>& GetFilters();

		void SetSourcePosition(const glm::vec3& sourcePos);
		void SetReceiverPosition(const glm::vec3& receiverPos);
		void SetReceiverDir(const glm::vec3& receiverDir);
		void SetReceiverLeft(const glm::vec3& receiverLeft);
		void SetReceiverUp(const glm::vec3& receiverUp);
		void SetReceiver(const glm::vec3& pos, const glm::vec3& right, const glm::vec3& up);

		/*
		void setCutoffFrequency(float fc);
		float getCutoffFrequency() const;
		uint32_t getSampleRate() const;

		void setEchoData(float delay, float gain);
		*/

		friend class AudioSystem;
	private:
		bool IsCreated() const;

	private:
		float m_Volume = 0.f;
		PaStream* m_pStream = nullptr;
		PortAudio* m_pPortAudioPtr = nullptr;
		PCM::UserData* m_pUserData = nullptr;
		bool m_IsStreamOn = false;
		std::string m_Name = "NO_NAME";

	private:
		void ThreadFunction(const std::string& threadName);

		// Threading (TODO: try to minimize the number of threads! We do not want one thread per sound!)
		std::thread* m_pThread;
		std::mutex m_Mutex;
		std::condition_variable m_CV;
		bool m_Running = true;
		enum class ItemType
		{
			None,
			Play,
			Pause,
			Stop
		};
		struct QueueItem
		{
			ItemType m_Type = ItemType::None;
			Sound* m_pSound = nullptr;
		};
		std::queue<QueueItem> m_TaskQueue;
	};
}