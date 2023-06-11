#pragma once

#include "DX12/NewCore/Resources.h"

namespace RS
{
	class RenderTarget
	{
	public:

		std::vector<std::shared_ptr<Texture>> GetTextures() { m_Textures; }

	private:
		friend class SwapChain;

		std::vector<std::shared_ptr<Texture>> m_Textures;
	};
}