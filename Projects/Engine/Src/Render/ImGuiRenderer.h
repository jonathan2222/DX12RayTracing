#pragma once

#include "DX12/Final/DXCore.h"
#include "DX12/Final/DXRootSignature.h"
#include "DX12/Final/DXPipelineState.h"
#include "DX12/Final/DXTexture.h"
#include "DX12/Final/DXGPUBuffers.h"
#include "DX12/Final/DXColorBuffer.h"
#include "DX12/Final/DXCommandContext.h"
//#define ImTextureID RS::Texture*
#include <imgui.h>
#include <mutex>
#include <functional>

//#include "DX12/Dx12Device.h"
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

		void Render(DX12::DXColorBuffer& colorBuffer, DX12::DXGraphicsContext* pContext);

		bool WantKeyInput();

		void Resize();

		float GetGuiScale();

		ImTextureID GetImTextureID(std::shared_ptr<DX12::DXTexture> pTexture);

		std::function<void(uint32, uint32)> additionalResizeFunction;
	private:
		struct ImplDX12BackendRendererData
		{
			DXGI_FORMAT rtvFormat;
			DX12::DXRootSignature rootSignature; // Should be able to hold varying number of textures.
			DX12::DXGraphicsPSO graphicsPSO;
			//ID3D12PipelineState* pPipelineState = nullptr;
			std::shared_ptr<DX12::DXTexture> pFontTexture;

			ImplDX12BackendRendererData() : graphicsPSO(L"ImGUI Graphics PSO") { }
		};

		struct ImplDX12RenderBuffers
		{
			DX12::DXByteAddressBuffer geometry;
			uint allocatedVertexCount = 0u;
			uint allocatedIndexCount = 0u;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
			D3D12_INDEX_BUFFER_VIEW indexBufferView;
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

		static void ImplDX12SetupRenderState(ImDrawData* draw_data, DX12::DXGraphicsContext& context, ImplDX12RenderBuffers* fr);
		void ImplDX12RenderDrawData(ImDrawData* draw_data, DX12::DXGraphicsContext& context);
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

		void TrackTextureResource(std::shared_ptr<DX12::DXTexture> pTexture);
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

		std::unordered_map<DX12::DXTexture*, std::shared_ptr<DX12::DXTexture>>	m_TrackedTextureResources;
	};
}