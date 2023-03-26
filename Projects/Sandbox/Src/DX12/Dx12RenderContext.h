#pragma once

#include "Dx12Core2.h"
#include "Dx12Resources.h"
#include "Dx12Surface.h"

namespace RS::DX12
{
	class Dx12RenderContext
	{
	public:
		static Dx12RenderContext* Get();

		void BeginFrame();
		void EndFrame();
		void Present();

		void ClearRTV(const float clearColor[4]);

	private:
		Dx12FrameCommandList	m_FrameCommandList;
		Dx12Surface				m_Surface;
	};
}