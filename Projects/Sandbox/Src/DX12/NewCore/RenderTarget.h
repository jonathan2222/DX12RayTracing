#pragma once

#include "DX12/NewCore/Resources.h"

namespace RS
{
	class RenderTarget
	{
	public:

		std::vector<std::shared_ptr<Texture>> GetColorTextures() const { m_ColorTextures; }
		std::shared_ptr<Texture> GetDepthTextures() const { m_DepthTexture; }

	private:
		friend class SwapChain;

		std::vector<std::shared_ptr<Texture>> m_ColorTextures;
		std::shared_ptr<Texture> m_DepthTexture;
	};
}