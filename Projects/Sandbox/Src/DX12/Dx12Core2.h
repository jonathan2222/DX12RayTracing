#pragma once

#include "Dx12Device.h"
#include "Dx12CommandList.h"

namespace RS::DX12
{
	class Dx12Core2
	{
	public:
		RS_NO_COPY_AND_MOVE(Dx12Core2);
		Dx12Core2() = default;
		~Dx12Core2() = default;

		static Dx12Core2* Get();

		void Init();
		void Release();

		void Render();

	private:
		Dx12Device				m_Device;
		Dx12FrameCommandList	m_FrameCommandList;
	};
}