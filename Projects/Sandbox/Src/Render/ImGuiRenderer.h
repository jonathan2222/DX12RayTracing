#pragma once

#include <imgui.h>
#include <mutex>
#include <functional>

#include "DX12/Dx12Resources.h"
#include "GUI/IconsFontAwesome6.h"
#include "GUI/ImGuiNotify.h"

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
		void InternalInit();
		void InternalResize();
		void ReScale(uint32 width, uint32 height);

	private:
		bool									m_IsInitialized = false;
		bool									m_IsReleased = true;
		std::vector<std::function<void(void)>>	m_DrawCalls;
		std::mutex								m_Mutex;
		bool									m_ShouldRescale = false;
		float									m_Scale = 1.f;
		uint32									m_OldWidth;
		uint32									m_OldHeight;

		ImGuiConfigFlags						m_ImGuiConfigFlags = 0;

		ID3D12DescriptorHeap*					m_SRVHeap = nullptr;
	};
}