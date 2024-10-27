#pragma once

#include <string>
#include "Core/Input.h"

namespace RS
{
	class DebugWindow
	{
	public:
		DebugWindow(const std::string& name) : m_Name(name), m_Active(false) { Init(); }
		virtual ~DebugWindow() { Destory(); }

		virtual void Init() {};
		virtual void Destory() {};
		virtual void Activate() {};
		virtual void Deactivate() {};
		virtual void FixedTick() {};
		virtual void Render() = 0;
		virtual bool GetEnableRequirements() const { return true; };

	private:
		void SuperActivate() { m_Active = true; return Activate(); }
		void SuperDeactivate() { m_Active = false; return Deactivate(); }
		void SuperFixedTick() { return FixedTick(); }
		void SuperRender();

		void UpdateShortcut();
	private:
		friend class DebugWindowsManager;

		std::string m_Name;
		bool m_Active;

		Key m_ShortcutKey = Key::UNKNOWN;
		Input::ModFlags m_ShortcutMods = Input::IGNORED;
		std::string m_Shortcut;
	};
}