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
		EditorWindow* RegisterEditorWindow(const std::string& name, bool enabled = false);

		void RenderToastDemo();

		void LoadPersistentData();
		void SavePersistentData();
	private:
		ImGuiDockNodeFlags m_DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

		std::vector<EditorWindow*> m_EditorWindows;

		bool m_ShowImGuiDemoWindow;
		bool m_ShowToastDemoWindow;
		inline static const std::string m_sToastDemoWindowName = "Toast Demo";
		inline static const std::string m_sImGuiDemoWindowName = "ImGui Demo";

		inline static const std::string m_sPersistentDataPath = RS_PROJECT_NAME "/PersistentData.cfg";
		RS::VMap m_PersistentData;
	};

	template<class T>
	inline EditorWindow* Editor::RegisterEditorWindow(const std::string& name, bool enabled)
	{
		EditorWindow* pWindow = new T(name, enabled);
		m_EditorWindows.push_back(pWindow);
		return pWindow;
	}
}