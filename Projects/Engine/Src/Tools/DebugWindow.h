#pragma once

#include <string>

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

	private:
		friend class DebugWindowsManager;

		std::string m_Name;
		bool m_Active;
	};
}