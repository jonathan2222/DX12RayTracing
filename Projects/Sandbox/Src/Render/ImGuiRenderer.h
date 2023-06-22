#pragma once

#include "DX12/NewCore/DX12Core3.h"
#include "DX12/NewCore/RootSignature.h"
#include "DX12/NewCore/Resources.h"
//#define ImTextureID RS::Texture*
#include <imgui.h>
#include <mutex>
#include <functional>

#include "DX12/Dx12Device.h"
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

		ImTextureID GetImTextureID(std::shared_ptr<Texture> pTexture);

	private:
		struct ImplDX12BackendRendererData
		{
			DXGI_FORMAT rtvFormat;
			std::shared_ptr<RootSignature> pRootSignature; // Should be able to hold varying number of textures.
			ID3D12PipelineState* pPipelineState = nullptr;
			std::shared_ptr<Texture> pFontTexture;

			ImplDX12BackendRendererData() { memset((void*)this, 0, sizeof(*this)); }
		};

		struct ImplDX12RenderBuffers
		{
			std::shared_ptr<VertexBuffer> pVertexBuffer;
			std::shared_ptr<IndexBuffer> pIndexBuffer;
		};

		struct ImplDX12ViewportData
		{
			DXGI_FORMAT rtvFormat;

			// Use these if we would like multiple viewports.
			//std::shared_ptr<CommandQueue> pCommandQueue;
			//std::shared_ptr<SwapChain> pSwapChain;

			ImplDX12RenderBuffers renderBuffers;
		};

		struct VERTEX_CONSTANT_BUFFER_DX12
		{
			float   mvp[4][4];
		};

	private:
		void InternalInit();
		void InternalResize();
		void ReScale(uint32 width, uint32 height);

		// New renderer
		bool ImplDX12Init(DXGI_FORMAT rtvFormat);
		void ImplDX12Shutdown();
		void ImplDX12CreateRootSignature(ImplDX12BackendRendererData* br);
		void ImplDX12CreatePipelineState(ImplDX12BackendRendererData* br);

		void ImplDX12NewFrame();

		static void ImplDX12SetupRenderState(ImDrawData* draw_data, std::shared_ptr<CommandList> pCommandList, ImplDX12RenderBuffers* fr);
		void ImplDX12RenderDrawData(ImDrawData* draw_data, std::shared_ptr<CommandList> pCommandList);
		static void ImplDX12CreateFontsTexture();
		bool ImplDX12CreateDeviceObjects();
		void ImplDX12InvalidateDeviceObjects();

		static ImplDX12BackendRendererData* ImplDX12GetBackendData();
		static void ImplDX12InitPlatformInterface();
		static void ImplDX12ShutdownPlatformInterface();

		static void ImplDX12SwapBuffers(ImGuiViewport* viewport, void*);
		static void ImplDX12RenderWindow(ImGuiViewport* viewport, void*);
		static void ImplDX12SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
		static void ImplDX12DestroyWindow(ImGuiViewport* viewport);
		static void WaitForPendingOperations(ImplDX12ViewportData* vd);
		static void ImplDX12CreateWindow(ImGuiViewport* viewport);

		void TrackTextureResource(std::shared_ptr<Texture> pTexture);
		void FreeAllTextureResources();

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

		std::unordered_map<Texture*, std::shared_ptr<Texture>>	m_TrackedTextureResources;
	};
}