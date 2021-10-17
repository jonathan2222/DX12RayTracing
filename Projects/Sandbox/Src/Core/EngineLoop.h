#pragma once

#include "Core/FrameTimer.h"

namespace RS
{
	class EngineLoop
	{
	public:
		EngineLoop() = default;
		~EngineLoop() = default;

		static std::shared_ptr<EngineLoop> Get();

		void Init();
		void Release();

		void Run();

		void FixedTick();

		void Tick(const RS::FrameStats& frameStats);

	private:
		FrameStats m_FrameStats = {};
		FrameTimer m_FrameTimer;
	};
}