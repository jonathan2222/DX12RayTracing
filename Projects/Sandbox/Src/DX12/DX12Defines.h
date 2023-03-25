#pragma once

#include "Utils/Logger.h"

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <atlbase.h>

#ifdef RS_CONFIG_DEBUG
#include <dxgidebug.h>
#endif

#include <wrl.h>
#include <combaseapi.h>
using Microsoft::WRL::ComPtr;
#include <shellapi.h>

#include <stdexcept>

#include "Utils/Utils.h"
#include "Core/LaunchArguments.h"

#define DXCall(x)                                                                                       \
    {                                                                                                   \
        HRESULT _internal_hr = x;                                                                       \
        std::string resourceName = #x;                                                                  \
        ThrowIfFailed(_internal_hr, "Failed to call DXCall({})", resourceName.c_str());                 \
        if (LaunchArguments::ContainsAny({LaunchParams::logAllDXCalls, LaunchParams::logDXCalls}))      \
            LOG_DEBUG("DXCall({})", resourceName.c_str());                                              \
    }

#define DXCallVerbose(x)                                                                        \
    {                                                                                           \
        HRESULT _internal_hr = x;                                                               \
        std::string resourceName = #x;                                                          \
        ThrowIfFailed(_internal_hr, "Failed to call DXCallVerbose({})", resourceName.c_str());  \
        if (LaunchArguments::Contains(LaunchParams::logAllDXCalls))                             \
            LOG_DEBUG("DXCallVerbose({})", resourceName.c_str());                               \
    }

#ifdef RS_CONFIG_DEBUG
    /* Set the name of the DX12 resouce and make a copy of it in nameOut. 
    *  The argument after the name copy is a format string with its arguments after.
    *   Example: DX12_SET_DEBUG_NAME_REF(heap, heapDebugName, "My heap #{}", heapIndex);
    */
    #define DX12_SET_DEBUG_NAME_REF(pResource, nameOut, ...)                         \
        {                                                                           \
            nameOut = Utils::Format(__VA_ARGS__);                                   \
            DXCallVerbose(pResource->SetName(Utils::ToWString(nameOut).c_str()));    \
            if(LaunchArguments::Contains(LaunchParams::logDXNamedObjects))          \
                LOG_DEBUG("Named Resource: {}", nameOut.c_str());                   \
        }
    /* Set the name of the DX12 resource. The argument after is a format string with its arguments after.
    *   Example: DX12_SET_DEBUG_NAME(heap, "My heap #{}", heapIndex);
    */
    #define DX12_SET_DEBUG_NAME(pResource, ...)                               \
        {                                                                    \
            std::string _resource##_name = "Unname resource";                \
            DX12_SET_DEBUG_NAME_REF(pResource, _resource##_name, __VA_ARGS__); \
        }
#else
    #define DX12_SET_DEBUG_NAME(resource, ...)
#endif

#define DX12_RELEASE(x)         \
    {                           \
        if (x) x->Release();    \
        x = nullptr;            \
    }

#define DX12_DEVICE_PTR ID3D12Device8*
#define DX12_FACTORY_PTR IDXGIFactory4*

namespace RS::DX12
{
    constexpr std::string_view GetCommandListTypeAsString(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return "Direct";
        case D3D12_COMMAND_LIST_TYPE_BUNDLE:
            return "Bundle";
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return "Compute";
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return "Copy";
        case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
            return "Video Decode";
        case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
            return "Vide Process";
        case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
            return "Video Encode";
        default:
            LOG_WARNING("Unknown command list type {}", (uint32)type);
            return "Unknown";
        }
    }

    constexpr std::string_view GetDescriptorHeapTypeAsString(D3D12_DESCRIPTOR_HEAP_TYPE type)
    {
        switch (type)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return "CBV_SRV_UAV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return "SAMPLER";
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            return "RTV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            return "DSV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES:
        default:
            LOG_WARNING("Unknown descriptor heap type {}", (uint32)type);
            return "Unknown";
        }
    }

    inline void ReportLiveObjects()
    {
        IDXGIDebug1* pDXGIDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDXGIDebug))))
        {
            pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL | D3D12_RLDO_DETAIL));
            DX12_RELEASE(pDXGIDebug);
        }
    }

    class HrException : public std::runtime_error
    {
        inline std::string HrToString(HRESULT hr)
        {
            char s_str[64] = {};
            sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
            return std::string(s_str);
        }
    public:
        HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
        HRESULT Error() const { return m_hr; }
    private:
        const HRESULT m_hr;
    };

    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw HrException(hr);
        }
    }

    inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg)
    {
        if (FAILED(hr))
        {
            OutputDebugString(msg);
            throw HrException(hr);
        }
    }

    inline void ThrowIfFalse(bool value)
    {
        ThrowIfFailed(value ? S_OK : E_FAIL);
    }

    inline void ThrowIfFalse(bool value, const wchar_t* msg)
    {
        ThrowIfFailed(value ? S_OK : E_FAIL, msg);
    }

    template<typename... Args>
    inline void ThrowIfFailed(HRESULT hr, fmt::format_string<Args...> fmt, Args &&...args)
    {
        if (FAILED(hr))
        {
            LOG_ERROR(fmt, std::forward<Args>(args)...);
            LOG_FLUSH();
        }

        if (FAILED(hr))
            throw HrException(hr);
    }

    inline void ThrowIfFailed(HRESULT hr, const std::string& msg)
    {
        ThrowIfFailed(hr, "{}", msg);
    }


    inline void ThrowIfFalse(bool value, const std::string& msg)
    {
        ThrowIfFailed(value ? S_OK : E_FAIL, msg);
    }

    // Assign a name to the object to aid with debugging.
    #if defined(_DEBUG) || defined(DBG)
    inline void SetName(ID3D12Object* pObject, LPCWSTR name)
    {
        pObject->SetName(name);
    }
    inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
    {
        WCHAR fullName[50];
        if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
        {
            pObject->SetName(fullName);
        }
    }
    #else
    inline void SetName(ID3D12Object*, LPCWSTR)
    {
    }
    inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
    {
    }
    #endif

    // Naming helper for ComPtr<T>.
    // Assigns the name of the variable as the name of the object.
    // The indexed variant will include the index in the name of the object.
    #define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
    #define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

    inline UINT Align(UINT size, UINT alignment)
    {
        return (size + (alignment - 1)) & ~(alignment - 1);
    }

    inline UINT CalculateConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffer size is required to be aligned.
        return Align(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    }

    #ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
    inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target)
    {
        UINT compileFlags = 0;
    #if defined(_DEBUG) || defined(DBG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #endif

        HRESULT hr;

        Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

        if (errors != nullptr)
        {
            OutputDebugStringA((char*)errors->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        return byteCode;
    }
    #endif

    // Resets all elements in a ComPtr array.
    template<class T>
    void ResetComPtrArray(T* comPtrArray)
    {
        for (auto& i : *comPtrArray)
        {
            i.Reset();
        }
    }


    // Resets all elements in a unique_ptr array.
    template<class T>
    void ResetUniquePtrArray(T* uniquePtrArray)
    {
        for (auto& i : *uniquePtrArray)
        {
            i.reset();
        }
    }

    class GpuUploadBuffer
    {
    public:
        ComPtr<ID3D12Resource> GetResource() { return m_resource; }

    protected:
        ComPtr<ID3D12Resource> m_resource;

        GpuUploadBuffer() {}
        ~GpuUploadBuffer()
        {
            if (m_resource.Get())
            {
                m_resource->Unmap(0, nullptr);
            }
        }

        void Allocate(ID3D12Device* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
        {
            auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
            ThrowIfFailed(device->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_resource)));
            m_resource->SetName(resourceName);
        }

        uint8_t* MapCpuWriteOnly()
        {
            uint8_t* mappedData;
            // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
            return mappedData;
        }
    };

    struct D3DBuffer
    {
        ComPtr<ID3D12Resource> resource;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
    };

    // Helper class to create and update a constant buffer with proper constant buffer alignments.
    // Usage: ToDo
    //    ConstantBuffer<...> cb;
    //    cb.Create(...);
    //    cb.staging.var = ...; | cb->var = ... ; 
    //    cb.CopyStagingToGPU(...);
    template <class T>
    class ConstantBuffer : public GpuUploadBuffer
    {
        uint8_t* m_mappedConstantData;
        UINT m_alignedInstanceSize;
        UINT m_numInstances;

    public:
        ConstantBuffer() : m_alignedInstanceSize(0), m_numInstances(0), m_mappedConstantData(nullptr) {}

        void Create(ID3D12Device* device, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
        {
            m_numInstances = numInstances;
            UINT alignedSize = Align(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            UINT bufferSize = numInstances * alignedSize;
            Allocate(device, bufferSize, resourceName);
            m_mappedConstantData = MapCpuWriteOnly();
        }

        void CopyStagingToGpu(UINT instanceIndex = 0)
        {
            memcpy(m_mappedConstantData + instanceIndex * m_alignedInstanceSize, &staging, sizeof(T));
        }

        // Accessors
        T staging;
        T* operator->() { return &staging; }
        UINT NumInstances() { return m_numInstances; }
        D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
        {
            return m_resource->GetGPUVirtualAddress() + instanceIndex * m_alignedInstanceSize;
        }
    };

    // Helper class to create and update a structured buffer.
    // Usage: ToDo
    //    ConstantBuffer<...> cb;
    //    cb.Create(...);
    //    cb.staging.var = ...; | cb->var = ... ; 
    //    cb.CopyStagingToGPU(...);
    template <class T>
    class StructuredBuffer : public GpuUploadBuffer
    {
        T* m_mappedBuffers;
        std::vector<T> m_staging;
        UINT m_numInstances;

    public:
        // Performance tip: Align structures on sizeof(float4) boundary.
        // Ref: https://developer.nvidia.com/content/understanding-structured-buffer-performance
        static_assert(sizeof(T) % 16 == 0, L"Align structure buffers on 16 byte boundary for performance reasons.");

        StructuredBuffer() : m_mappedBuffers(nullptr), m_numInstances(0) {}

        void Create(ID3D12Device* device, UINT numElements, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
        {
            m_staging.resize(numElements);
            UINT bufferSize = numInstances * numElements * sizeof(T);
            Allocate(device, bufferSize, resourceName);
            m_mappedBuffers = reinterpret_cast<T*>(MapCpuWriteOnly());
        }

        void CopyStagingToGpu(UINT instanceIndex = 0)
        {
            memcpy(m_mappedBuffers + instanceIndex, &m_staging[0], InstanceSize());
        }

        // Accessors
        T& operator[](UINT elementIndex) { return m_staging[elementIndex]; }
        size_t NumElementsPerInstance() { return m_staging.size(); }
        UINT NumInstances() { return m_staging.size(); }
        size_t InstanceSize() { return NumElementsPerInstance() * sizeof(T); }
        D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
        {
            return m_resource->GetGPUVirtualAddress() + instanceIndex * InstanceSize();
        }
    };
}