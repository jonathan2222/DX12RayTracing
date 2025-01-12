#pragma once

#include <thread>

namespace RS
{
	class SoundThread
	{
	public:
		static std::shared_ptr<SoundThread> Get();

		void Init();
		void Destroy();

	private:
		std::thread* m_pThread;
		std::mutex m_SoundMutex;
	};
}