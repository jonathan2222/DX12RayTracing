#pragma once

#include "DX12/Dx12Device.h"

namespace RS
{
	/*
	
	Desc {
		CPUPtr; // Always set
		GPUPtr; // Only set when binding to pipeline.
	}

	TextureA {
		Desc desc = DescriptorAllocator2::Allocate(RSV_UAV_CBV);
	}
	On release -> DescriptorAllocator2::Free(desc);


	This new descriptor allocator should do similar to the old one, but support bindless descriptors and should not need to bind every descriptor to fill the root signature.

	Might want to do similar to the old method with us only allocating a descriptor when making a view.
	Such that one resource can have multiple views each with their own descriptor. But that is more of a usage problem than an implementation of this class.

	On second thought, might only need to make a new DynamicDescriptorHeap that has support for bindless textures and sparse binds.
	-> Changing the old DynamicDescriptorHeap will only fix the sparse binding problem.
	To fix the bindless texture problem we would need to have the GPU descriptors be allocated when creating the view/resource 
		and then pass an index to the shader such that it knows which resource it should pick from the root signature.
		This will also make us have a large root signature and a really large SRV_UAV_CBV GPU descriptor heap.

		The descriptor heap should have the size of the total amount of GPU resources we will use.

	*/

	// Inspired by the DynamicDescriptorHeap from www.3dgep.com/learning-directx-12
	class GPUDescriptorHeap // <- new Dynamic Descriptor Heap.
	{
	public:
		GPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap = 1024);
		virtual ~GPUDescriptorHeap();

	private:

	};
}