#pragma once

#include "DX12/Final/DXGPUBuffers.h"

namespace RS::DX12
{
	class DXReadbackBuffer : public DXGPUBuffer
	{
    public:
        virtual ~DXReadbackBuffer() { Destroy(); }

        void Create(const std::wstring& name, uint32 NumElements, uint32 ElementSize);

        void* Map(void);
        void Unmap(void);

    protected:

        void CreateDerivedViews(void) {}
	};
}