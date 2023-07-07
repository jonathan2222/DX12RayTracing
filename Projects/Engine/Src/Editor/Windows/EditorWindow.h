#pragma once

// All editor windows are rendered using ImGui.
#include "Render/ImGuiRenderer.h"

namespace RSE
{
	class EditorWindow
	{
	public:
		explicit EditorWindow(const std::string& name, bool enabled = false)
			: m_Name(name)
			, m_Enabled(enabled)
		{}

		virtual void Init() {};
		virtual void Release() {};
	protected:
		virtual void Update() {};
		virtual void FixedUpdate() {};
		virtual void Render() = 0; // All editor windows must implement a render function.

	public:
		std::string GetName() const { return m_Name; }
		bool& GetEnable() { return m_Enabled; }
		virtual bool GetEnableRequirements() const { return true; };

		// These are the functions that are called by the editor.
		void SuperUpdate() { if (!CanProcess()) return; Update(); }
		void SuperRender() { if (!CanProcess()) return; Render(); }
		void SuperFixedUpdate() { if (!CanProcess()) return; FixedUpdate(); }

	private:
		bool CanProcess() const { return m_Enabled && GetEnableRequirements(); }

	protected:
		std::string m_Name;
		bool m_Enabled;
	};
}