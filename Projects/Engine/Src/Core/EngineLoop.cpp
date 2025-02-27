﻿#include "PreCompiled.h"
#include "EngineLoop.h"

#include "Core/Display.h"
#include "Core/Input.h"

#include "DX12/DirecXRaytracingHelper.h"
//#include "Raytracing.hlsl.h"

#include "DX12/Final/DXCore.h"

#include "Core/Console.h"

// TODO: Remove this when in relase build!
#include "Render/ImGuiRenderer.h"

#include "Graphics/RenderCore.h"
#include "Audio/AudioSystem.h"

using namespace RS;

const wchar_t* EngineLoop::c_hitGroupName = L"MyHitGroup";
const wchar_t* EngineLoop::c_raygenShaderName = L"MyRaygenShader";
const wchar_t* EngineLoop::c_closestHitShaderName = L"MyClosestHitShader";
const wchar_t* EngineLoop::c_missShaderName = L"MyMissShader";

std::shared_ptr<EngineLoop> EngineLoop::Get()
{
    static std::shared_ptr<EngineLoop> s_EngineLoop = std::make_shared<EngineLoop>();
    return s_EngineLoop;
}

void EngineLoop::Init()
{
    Console::Get()->Init();
    RS::Input::Get()->AlwaysListenToKey(RS::Key::MICRO); // For console.

    m_RenderDoc.Init();

    //std::shared_ptr<RS::Display> pDisplay = RS::Display::Get();
    //DX12Core3::Get()->Init(pDisplay->GetHWND(), pDisplay->GetWidth(), pDisplay->GetHeight());
    RS::DX12::DXCore::Init();

    GetRenderCore()->Init();

    AudioSystem::Get()->Init();

    m_DebugWindowsManager.Init();

    InitConsoleCommands();
}

void EngineLoop::Release()
{
    m_DebugWindowsManager.Destory();

    AudioSystem::Get()->Destroy();

    GetRenderCore()->Destory();

    DX12Core3::Get()->Release();

    Console::Get()->Release();
}

void EngineLoop::Run()
{
    std::shared_ptr<RS::Display> pDisplay = RS::Display::Get();

    m_FrameTimer.Init(&m_FrameStats, 0.25f);
    while (!pDisplay->ShouldClose())
    {
        m_FrameTimer.Begin();

        m_RenderDoc.StartFrameCapture();

        pDisplay->PollEvents();
        RS::Input::Get()->PreUpdate();

        // Quit by pressing ESCAPE
        {
            if (RS::Input::Get()->IsKeyPressed(RS::Key::ESCAPE))
                pDisplay->Close();
        }

        // Toggle fullscreen by pressing F11
        {
            if (RS::Input::Get()->IsKeyClicked(RS::Key::F11))
                pDisplay->ToggleFullscreen();
        }

        // Toggle console by pressing §
        {
            if (RS::Input::Get()->IsKeyClicked(RS::Key::MICRO))
            {
                RS:Console* pConsole = RS::Console::Get();
                if (pConsole->IsEnabled())
                    pConsole->Disable();
                else
                    pConsole->Enable();
            }
        }

        m_FrameTimer.FixedTick( [&]()
            {
                FixedTick();
                // TODO: Remove this when in Relase build!
                m_DebugWindowsManager.FixedTick();
            });
        Tick(m_FrameStats);

        // TODO: Remove this when in Relase build!
        RS::ImGuiRenderer::Get()->Draw([&]()
            {
                m_DebugWindowsManager.Render();
            });

        RS::Input::Get()->PostUpdate(m_FrameStats.frame.currentDT);

        m_RenderDoc.EndFrameCapture();

        m_FrameTimer.End();
    }
}

void EngineLoop::FixedTick()
{
    if (additionalFixedTickFunction)
        additionalFixedTickFunction();
}

void EngineLoop::Tick(const RS::FrameStats& frameStats)
{
    m_CurrentFrameNumber++;

    if (additionalTickFunction)
        additionalTickFunction(frameStats);

    AudioSystem::Get()->Update();

    DX12Core3::Get()->Render();
}

uint64 RS::EngineLoop::GetCurrentFrameNumber()
{
    return m_CurrentFrameNumber;
}

void EngineLoop::CreateDeviceDependentResources()
{
    // Initialize raytracing pipeline.

    // Create raytracing interfaces: raytracing device and commandlist.
    CreateRaytracingInterfaces();

    // Create root signatures for the shaders.
    CreateRootSignatures();

    // Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
    CreateRaytracingPipelineStateObject();

    // Create a heap for descriptors.
    CreateDescriptorHeap();

    // Build geometry to be used in the sample.
    BuildGeometry();

    // Build raytracing acceleration structures from the generated geometry.
    BuildAccelerationStructures();

    // Build shader tables, which define shaders and their local root arguments.
    BuildShaderTables();

    // Create an output 2D texture to store the raytracing result to.
    CreateRaytracingOutputResource();
}

void EngineLoop::CreateWindowSizeDependentResources()
{
    CreateRaytracingOutputResource();

    // For simplicity, we will rebuild the shader tables.
    BuildShaderTables();
}

void EngineLoop::ReleaseWindowSizeDependentResources()
{
    m_rayGenShaderTable.Reset();
    m_missShaderTable.Reset();
    m_hitGroupShaderTable.Reset();
    m_raytracingOutput.Reset();
}

void EngineLoop::ReleaseDeviceDependentResources()
{
    m_raytracingGlobalRootSignature.Reset();
    m_raytracingLocalRootSignature.Reset();

    m_dxrDevice.Reset();
    m_dxrCommandList.Reset();
    m_dxrStateObject.Reset();

    m_descriptorHeap.Reset();
    m_descriptorsAllocated = 0;
    m_raytracingOutputResourceUAVDescriptorHeapIndex = UINT_MAX;
    m_indexBuffer.Reset();
    m_vertexBuffer.Reset();

    m_accelerationStructure.Reset();
    m_bottomLevelAccelerationStructure.Reset();
    m_topLevelAccelerationStructure.Reset();
}

void EngineLoop::OnDeviceLost()
{
    ReleaseWindowSizeDependentResources();
    ReleaseDeviceDependentResources();
}

void EngineLoop::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

void RS::EngineLoop::OnSizeChange(uint32 width, uint32 height, bool isFullscreen, bool windowed)
{
    if (!DX12Core3::Get()->WindowSizeChanged(width, height, isFullscreen, windowed, false))
        return;
}

void RS::EngineLoop::RecreateD3D()
{
    // Give GPU a chance to finish its execution in progress.
    try
    {
        DX12Core3::Get()->WaitForGPU();
    }
    catch (DX12::HrException&)
    {
        // Do nothing, currently attached adapter is unresponsive.
    }
    //DX12Core3::Get()->HandleDeviceLost();
}

void RS::EngineLoop::DoRaytracing()
{
    RS_ASSERT(false, "Not implemented!");
    ID3D12GraphicsCommandList* commandList = nullptr;// DX12Core3::Get()->GetCommandList();

    auto pDisplay = Display::Get();

    auto DispatchRays = [&](auto* commandList, auto* stateObject, D3D12_DISPATCH_RAYS_DESC* dispatchDesc)
    {
        // Since each shader table has only one shader record, the stride is same as the size.
        dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
        dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
        dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
        dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
        dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
        dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
        dispatchDesc->RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
        dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
        dispatchDesc->Width = pDisplay->GetWidth();
        dispatchDesc->Height = pDisplay->GetHeight();
        dispatchDesc->Depth = 1;
        commandList->SetPipelineState1(stateObject);
        commandList->DispatchRays(dispatchDesc);
    };

    commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());

    // Bind the heaps, acceleration structure and dispatch rays.    
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
    commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_raytracingOutputResourceUAVGpuDescriptor);
    commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
    DispatchRays(m_dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc);
}

void RS::EngineLoop::CreateRaytracingInterfaces()
{
    auto device = DX12Core3::Get()->GetD3D12Device();
    //auto commandList = DX12Core3::Get()->GetCommandList();
    //
    //DX12::ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)), "Couldn't get DirectX Raytracing interface for the device.");
    //DX12::ThrowIfFailed(commandList->QueryInterface(IID_PPV_ARGS(&m_dxrCommandList)), "Couldn't get DirectX Raytracing interface for the command list.");
}

void RS::EngineLoop::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    auto device = DX12Core3::Get()->GetD3D12Device();
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    DX12::ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    DX12::ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

void RS::EngineLoop::CreateRootSignatures()
{
    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
        UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
        rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);
        rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &m_raytracingGlobalRootSignature);
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
        CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
        rootParameters[LocalRootSignatureParams::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);
        CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
    }
}

// Local root signature and shader association
// This is a root signature that enables a shader to have unique arguments that come from shader tables.
void RS::EngineLoop::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
    // Hit group and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

    // Local root signature to be used in a ray gen shader.
    {
        auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.Get());
        // Shader association
        auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(c_raygenShaderName);
    }
}

// Create a raytracing pipeline state object (RTPSO).
// An RTPSO represents a full set of shaders reachable by a DispatchRays() call,
// with all configuration options resolved, such as local signatures and other state.
void RS::EngineLoop::CreateRaytracingPipelineStateObject()
{
    // Create 7 subobjects that combine into a RTPSO:
    // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
    // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
    // This simple sample utilizes default shader association except for local root signature subobject
    // which has an explicit association specified purely for demonstration purposes.
    // 1 - DXIL library
    // 1 - Triangle hit group
    // 1 - Shader config
    // 2 - Local root signature and association
    // 1 - Global root signature
    // 1 - Pipeline config
    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
    auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    //D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
    //lib->SetDXILLibrary(&libdxil);
    // Define which shader exports to surface from the library.
    // If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
    // In this sample, this could be omitted for convenience since the sample uses all shaders in the library. 
    {
        lib->DefineExport(c_raygenShaderName);
        lib->DefineExport(c_closestHitShaderName);
        lib->DefineExport(c_missShaderName);
    }

    // Triangle hit group
    // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
    // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
    auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup->SetClosestHitShaderImport(c_closestHitShaderName);
    hitGroup->SetHitGroupExport(c_hitGroupName);
    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    // Shader config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    UINT payloadSize = 4 * sizeof(float);   // float4 color
    UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
    shaderConfig->Config(payloadSize, attributeSize);

    // Local root signature and shader association
    CreateLocalRootSignatureSubobjects(&raytracingPipeline);
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(m_raytracingGlobalRootSignature.Get());

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    // PERFOMANCE TIP: Set max recursion depth as low as needed 
    // as drivers may apply optimization strategies for low recursion depths. 
    UINT maxRecursionDepth = 1; // ~ primary rays only. 
    pipelineConfig->Config(maxRecursionDepth);

#if RS_CONFIG_DEBUG
    DX12::PrintStateObjectDesc(raytracingPipeline);
#endif

    // Create the state object.
    DX12::ThrowIfFailed(m_dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");
}

void RS::EngineLoop::CreateDescriptorHeap()
{
    auto device = DX12Core3::Get()->GetD3D12Device();

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    // Allocate a heap for a single descriptor:
    // 1 - raytracing output texture UAV
    descriptorHeapDesc.NumDescriptors = 1;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descriptorHeapDesc.NodeMask = 0;
    device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
    DX12::NAME_D3D12_OBJECT(m_descriptorHeap);

    m_descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RS::EngineLoop::CreateRaytracingOutputResource()
{
    auto device = DX12Core3::Get()->GetD3D12Device();
    auto backbufferFormat = DX12Core3::Get()->GetSwapChain()->GetFormat();

    auto pDisplay = Display::Get();

    // Create the output resource. The dimensions and format should match the swap-chain.
    auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, pDisplay->GetWidth(), pDisplay->GetHeight(), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DX12::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_raytracingOutput)));
    DX12::NAME_D3D12_OBJECT(m_raytracingOutput);

    D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
    m_raytracingOutputResourceUAVDescriptorHeapIndex = AllocateDescriptor(&uavDescriptorHandle, m_raytracingOutputResourceUAVDescriptorHeapIndex);
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
    m_raytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), m_raytracingOutputResourceUAVDescriptorHeapIndex, m_descriptorSize);
}

void RS::EngineLoop::BuildGeometry()
{
    auto device = DX12Core3::Get()->GetD3D12Device();
    Index indices[] =
    {
        0, 1, 2
    };

    float depthValue = 1.0;
    float offset = 1.0f;
    Vertex vertices[] =
    {
        // The sample raytraces in screen space coordinates.
        // Since DirectX screen space coordinates are right handed (i.e. Y axis points down).
        // Define the vertices in counter clockwise order ~ clockwise in left handed.
        { 0, -offset, depthValue },
        { -offset, offset, depthValue },
        { offset, offset, depthValue }
    };

    DX12::AllocateUploadBuffer(device, vertices, sizeof(vertices), &m_vertexBuffer, L"VertexBuffer");
    DX12::AllocateUploadBuffer(device, indices, sizeof(indices), &m_indexBuffer, L"IndexBuffer");
}

void RS::EngineLoop::BuildAccelerationStructures()
{
    RS_ASSERT(false, "Not implemented!");
    auto device = DX12Core3::Get()->GetD3D12Device();
    ID3D12GraphicsCommandList* commandList = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12CommandAllocator* commandAllocator = nullptr;

    // Reset the command list for the acceleration structure construction.
    commandList->Reset(commandAllocator, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer->GetDesc().Width) / sizeof(Index);
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer->GetDesc().Width) / sizeof(Vertex);
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    DX12::ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;
    m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    DX12::ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    ComPtr<ID3D12Resource> scratchResource;
    DX12::AllocateUAVBuffer(device, std::max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesnt need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    {
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        DX12::AllocateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
        DX12::AllocateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
    }

    // Create an instance desc for the bottom-level acceleration structure.
    ComPtr<ID3D12Resource> instanceDescs;
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    DX12::AllocateUploadBuffer(device, &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

    // Bottom Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    {
        bottomLevelBuildDesc.Inputs = bottomLevelInputs;
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    }

    // Top Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    {
        topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
        topLevelBuildDesc.Inputs = topLevelInputs;
        topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
    }

    auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
    {
        raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get());
        commandList->ResourceBarrier(1, &barrier);
        raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    };

    // Build acceleration structure.
    BuildAccelerationStructure(m_dxrCommandList.Get());

    // Kick off acceleration structure construction.
    //DX12::Dx12Core::Get()->ExecuteCommandList();

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    //DX12::Dx12Core::Get()->WaitForGpu();
}

void RS::EngineLoop::BuildShaderTables()
{
    auto device = DX12Core3::Get()->GetD3D12Device();

    void* rayGenShaderIdentifier;
    void* missShaderIdentifier;
    void* hitGroupShaderIdentifier;

    auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
    {
        rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_raygenShaderName);
        missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_missShaderName);
        hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_hitGroupName);
    };

    // Get shader identifiers.
    UINT shaderIdentifierSize;
    {
        ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        DX12::ThrowIfFailed(m_dxrStateObject.As(&stateObjectProperties));
        GetShaderIdentifiers(stateObjectProperties.Get());
        shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    }

    // Ray gen shader table
    {
        struct RootArguments {
            RayGenConstantBuffer cb;
        } rootArguments;
        rootArguments.cb = m_rayGenCB;

        UINT numShaderRecords = 1;
        UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
        DX12::ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
        rayGenShaderTable.push_back(DX12::ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
        m_rayGenShaderTable = rayGenShaderTable.GetResource();
    }

    // Miss shader table
    {
        UINT numShaderRecords = 1;
        UINT shaderRecordSize = shaderIdentifierSize;
        DX12::ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
        missShaderTable.push_back(DX12::ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
        m_missShaderTable = missShaderTable.GetResource();
    }

    // Hit group shader table
    {
        UINT numShaderRecords = 1;
        UINT shaderRecordSize = shaderIdentifierSize;
        DX12::ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
        hitGroupShaderTable.push_back(DX12::ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
        m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
    }
}

void RS::EngineLoop::UpdateForSizeChange(UINT clientWidth, UINT clientHeight)
{
    auto pDisplay = Display::Get();
    float border = 0.1f;
    if (pDisplay->GetWidth() <= pDisplay->GetHeight())
    {
        m_rayGenCB.stencil =
        {
            -1 + border, -1 + border * pDisplay->GetAspectRatio(),
            1.0f - border, 1 - border * pDisplay->GetAspectRatio()
        };
    }
    else
    {
        m_rayGenCB.stencil =
        {
            -1 + border / pDisplay->GetAspectRatio(), -1 + border,
             1 - border / pDisplay->GetAspectRatio(), 1.0f - border
        };

    }
}

void RS::EngineLoop::CopyRaytracingOutputToBackbuffer()
{
    RS_ASSERT(false, "Not implemented!");
    ID3D12GraphicsCommandList* commandList = nullptr;
    ID3D12Resource* renderTarget = nullptr;

    D3D12_RESOURCE_BARRIER preCopyBarriers[2];
    preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
    preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

    commandList->CopyResource(renderTarget, m_raytracingOutput.Get());

    D3D12_RESOURCE_BARRIER postCopyBarriers[2];
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
}

void RS::EngineLoop::CalculateFrameStats()
{
}

UINT RS::EngineLoop::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse)
{
    auto descriptorHeapCpuBase = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    if (descriptorIndexToUse >= m_descriptorHeap->GetDesc().NumDescriptors)
    {
        descriptorIndexToUse = m_descriptorsAllocated++;
    }
    *cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_descriptorSize);
    return descriptorIndexToUse;
}

void RS::EngineLoop::InitConsoleCommands()
{
    Console::Get()->AddFunction("Engine.Quit", [this](Console::FuncArgs args)->bool
        {
            Display::Get()->Close();
            return true;
        },
        Console::Flag::NONE, "Quit the application."
    );
}
