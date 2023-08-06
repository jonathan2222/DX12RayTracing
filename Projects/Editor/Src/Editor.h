#pragma once

#include "RSEngine.h"
#include "Render/ImGuiRenderer.h"

#include "Windows/EditorWindow.h"

namespace RSE
{
	class Editor
	{
	public:
		RS_NO_COPY_AND_MOVE(Editor);
		~Editor() = default;

		static Editor* Get();

		void Init();
		void Release();

		void Update();
		void FixedUpdate();
		void Render();

		void Resize(uint32 width, uint32 height);

	private:
		Editor() = default;

		void RenderMenuBar();

		void RegisterEditorWindows();

		template<class T>
		void RegisterEditorWindow(const std::string& name, bool enabled = false);

		void RenderToastDemo();

	private:
		ImGuiDockNodeFlags m_DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

		std::vector<EditorWindow*> m_EditorWindows;

		bool m_ShowImGuiDemoWindow = false;
		bool m_ShowToastDemoWindow = false;
	};

	template<class T>
	inline void Editor::RegisterEditorWindow(const std::string& name, bool enabled)
	{
		m_EditorWindows.push_back(new T(name, enabled));
	}
}