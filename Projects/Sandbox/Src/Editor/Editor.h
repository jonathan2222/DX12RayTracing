#pragma once

#include "Render/ImGuiRenderer.h"

#include "Editor/Windows/ConsoleInspector.h"

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

	private:
		ImGuiDockNodeFlags m_DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

		ConsoleInspector m_ConsoleInspector;
	};
}