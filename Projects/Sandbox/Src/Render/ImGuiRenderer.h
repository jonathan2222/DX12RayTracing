#pragma once

#include <imgui.h>
#include <mutex>
#include <functional>

#include "DX12/Dx12Resources.h"

namespace RS
{
	class ImGuiRenderer
	{
	public:
		RS_NO_COPY_AND_MOVE(ImGuiRenderer);
		ImGuiRenderer() = default;
		~ImGuiRenderer() = default;

		static ImGuiRenderer* Get();

		void Init();
		void FreeDescriptor();
		void Release();

		void Draw(std::function<void(void)> callback);

		void Render();

		bool WantKeyInput();

		void Resize();

		float GetGuiScale();

	private:
		void InternalResize();
		void ReScale(uint32 width, uint32 height);

	private:
		bool									m_IsReleased = false; // The first call should be treated like it has not been released.
		std::vector<std::function<void(void)>>	s_DrawCalls;
		std::mutex								s_Mutex;
		bool									s_ShouldRescale = false;
		float									s_Scale = 1.f;
		DX12::Dx12DescriptorHandle				m_ImGuiFontDescriptorHandle;
	};
}