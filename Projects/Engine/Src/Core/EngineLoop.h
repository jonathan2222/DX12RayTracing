#pragma once

#include "Core/FrameTimer.h"
#include "Core/Display.h"
#include "Core/RenderDoc.h"
#include "DX12/NewCore/DX12Core3.h"
#include "DX12/RaytracingHlslCompat.h"

// TODO: Deactive this on release builds!
#include "Tools/DebugWindowsManager.h"

namespace RS
{
    namespace GlobalRootSignatureParams {
        enum Value {
            OutputViewSlot = 0,
            AccelerationStructureSlot,
            Count
        };
    }

    namespace LocalRootSignatureParams {
        enum Value {
            ViewportConstantSlot = 0,
            Count
        };
    }

	class EngineLoop : public IDeviceNotify, public IDisplaySizeChange
	{
	public:
		EngineLoop() = default;
		~EngineLoop() = default;

		static std::shared_ptr<EngineLoop> Get();

		void Init();
		void Release();

		void Run();

		void FixedTick();

		void Tick(const RS::FrameStats& frameStats);

        static uint64 GetCurrentFrameNumber();

        std::function<void(void)> additionalFixedTickFunction;
        std::function<void(const FrameStats&)> additionalTickFunction;
	private:
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();

		void OnDeviceLost() override;
		void OnDeviceRestored() override;
        void OnSizeChange(uint32 width, uint32 height, bool isFullscreen, bool windowed) override;

        void RecreateD3D();
        void DoRaytracing();
        void CreateRaytracingInterfaces();
        void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
        void CreateRootSignatures();
        void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
        void CreateRaytracingPipelineStateObject();
        void CreateDescriptorHeap();
        void CreateRaytracingOutputResource();
        void BuildGeometry();
        void BuildAccelerationStructures();
        void BuildShaderTables();
        void UpdateForSizeChange(UINT clientWidth, UINT clientHeight);
        void CopyRaytracingOutputToBackbuffer();
        void CalculateFrameStats();
        UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse = UINT_MAX);

        void InitConsoleCommands();

	private:
		FrameStats m_FrameStats = {};
		FrameTimer m_FrameTimer;

        inline static uint64 m_CurrentFrameNumber = 0;

		// ---------------- Raytracing variables ----------------
		static const UINT                   FrameCount = 3;

        // DirectX Raytracing (DXR) attributes
        ComPtr<ID3D12Device5>               m_dxrDevice;
        ComPtr<ID3D12GraphicsCommandList4>  m_dxrCommandList;
        ComPtr<ID3D12StateObject>           m_dxrStateObject;

        // Root signatures
        ComPtr<ID3D12RootSignature>         m_raytracingGlobalRootSignature;
        ComPtr<ID3D12RootSignature>         m_raytracingLocalRootSignature;

        // Descriptors
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;
        UINT                                m_descriptorsAllocated;
        UINT                                m_descriptorSize;

        // Raytracing scene
        RayGenConstantBuffer                m_rayGenCB;

        // Geometry
        typedef UINT16 Index;
        struct Vertex { float v1, v2, v3; };
        ComPtr<ID3D12Resource>              m_indexBuffer;
        ComPtr<ID3D12Resource>              m_vertexBuffer;

        // Acceleration structure
        ComPtr<ID3D12Resource>              m_accelerationStructure;
        ComPtr<ID3D12Resource>              m_bottomLevelAccelerationStructure;
        ComPtr<ID3D12Resource>              m_topLevelAccelerationStructure;

        // Raytracing output
        ComPtr<ID3D12Resource>              m_raytracingOutput;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_raytracingOutputResourceUAVGpuDescriptor;
        UINT                                m_raytracingOutputResourceUAVDescriptorHeapIndex;

        // Shader tables
        static const wchar_t*               c_hitGroupName;
        static const wchar_t*               c_raygenShaderName;
        static const wchar_t*               c_closestHitShaderName;
        static const wchar_t*               c_missShaderName;
        ComPtr<ID3D12Resource>              m_missShaderTable;
        ComPtr<ID3D12Resource>              m_hitGroupShaderTable;
        ComPtr<ID3D12Resource>              m_rayGenShaderTable;

        // Debugging
        RenderDoc m_RenderDoc;
        DebugWindowsManager m_DebugWindowsManager;
	};
}