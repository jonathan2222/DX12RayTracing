#pragma once

#include "DX12/NewCore/DX12Core3.h"

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
	};
}