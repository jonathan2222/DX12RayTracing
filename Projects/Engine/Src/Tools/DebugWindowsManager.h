#pragma once

#include "Tools/DebugWindow.h"
#include <string>

namespace RS
{
	class DebugWindowsManager
	{
	public:
		void Init();
		void FixedTick();
		void Render();
		void Destory();

		template<class WindowType>
		void RegisterDebugWindow(const std::string& name);

		const std::vector<DebugWindow*>& GetWindows() const { return m_Windows; }

	private:
		std::vector<DebugWindow*> m_Windows;

		bool m_RenderMenuBar = false;
		bool m_InMenus = false;
	};

	template<class WindowType>
	inline void DebugWindowsManager::RegisterDebugWindow(const std::string& name)
	{
		// Validate name
		for (DebugWindow* pWindow : m_Windows)
		{
			if (pWindow->m_Name == name)
				RS_ASSERT(false, "Debug Window cannot have the same name as another!");
		}

		// Register
		m_Windows.push_back(new WindowType(name));
	}
}