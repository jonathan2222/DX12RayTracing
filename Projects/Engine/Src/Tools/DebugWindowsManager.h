#pragma once

#include "Tools/DebugWindow.h"
#include <string>
#include <Core/Input.h>
#include <Core/Key.h>

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

		template<class WindowType>
		void RegisterDebugWindow(const std::string& name, Key shortcutKey, Input::ModFlags shortcutMods = Input::IGNORED);

		const std::vector<DebugWindow*>& GetWindows() const { return m_Windows; }

	private:
		std::vector<DebugWindow*> m_Windows;

		bool m_RenderMenuBar = false;
		bool m_InMenus = false;
	};

	template<class WindowType>
	inline void DebugWindowsManager::RegisterDebugWindow(const std::string& name)
	{
		RegisterDebugWindow<WindowType>(name, Key::UNKNOWN, Input::ModFlag::IGNORED);
	}

	template<class WindowType>
	inline void DebugWindowsManager::RegisterDebugWindow(const std::string& name, Key shortcutKey, Input::ModFlags shortcutMods)
	{
		// Validate name
		for (DebugWindow* pWindow : m_Windows)
		{
			if (pWindow->m_Name == name)
				RS_ASSERT(false, "Debug Window cannot have the same name as another!");
		}

		// Register
		DebugWindow* pWindow = new WindowType(name);
		pWindow->m_ShortcutKey = shortcutKey;
		pWindow->m_ShortcutMods = shortcutMods;
		pWindow->UpdateShortcut();
		m_Windows.push_back(pWindow);
	}
}