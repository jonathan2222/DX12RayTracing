#pragma once

#include "DX12/Final/DXGPUResource.h"

namespace RS::DX12
{
    class DXUploadBuffer : public DXGPUResource
    {
    public:
        virtual ~DXUploadBuffer() { Destroy(); }

        void Create(const std::wstring& name, size_t BufferSize);

        void* Map(void);
        void Unmap(size_t begin = 0, size_t end = -1);

        size_t GetBufferSize() const { return m_BufferSize; }

    protected:

        size_t m_BufferSize;
    };
}