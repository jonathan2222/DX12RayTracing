#pragma once

#include "DX12/NewCore/Resources.h"

namespace RS
{
	enum class AttachmentPoint : uint32
	{
		Color0 = 0,
		Color1,
		Color2,
		Color3,
		Color4,
		Color5,
		Color6,
		Color7,
		DepthStencil,
		NumAttachments
	};

	class RenderTarget
	{
	public:
		RenderTarget();

		void SetAttachment(AttachmentPoint attachmentPoint, std::shared_ptr<Texture> pTexture);

		std::shared_ptr<Texture> GetAttachment(AttachmentPoint attachmentPoint) const;

		const std::vector<std::shared_ptr<Texture>>& GetColorTextures() const { return m_ColorTextures; }
		std::shared_ptr<Texture> GetDepthTexture() const { return m_pDepthTexture; }

		void Reset();

	private:
		friend class SwapChain;

		std::vector<std::shared_ptr<Texture>> m_ColorTextures;
		std::shared_ptr<Texture> m_pDepthTexture;
	};
}