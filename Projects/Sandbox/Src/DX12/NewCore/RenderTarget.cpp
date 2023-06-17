#include "PreCompiled.h"
#include "RenderTarget.h"

RS::RenderTarget::RenderTarget()
	: m_pDepthTexture(nullptr)
{
	m_ColorTextures.resize(static_cast<uint32>(AttachmentPoint::NumAttachments) - 1u, nullptr);
}

void RS::RenderTarget::SetAttachment(AttachmentPoint attachmentPoint, std::shared_ptr<Texture> pTexture)
{
	if (attachmentPoint == AttachmentPoint::DepthStencil)
		m_pDepthTexture = pTexture;
	else
		m_ColorTextures[static_cast<uint32>(attachmentPoint)] = pTexture;
}

std::shared_ptr<RS::Texture> RS::RenderTarget::GetAttachment(AttachmentPoint attachmentPoint) const
{
	if (attachmentPoint == AttachmentPoint::DepthStencil)
		return m_pDepthTexture;
	else
		return m_ColorTextures[static_cast<uint32>(attachmentPoint)];
}

void RS::RenderTarget::Reset()
{
	for (auto& pTexture : m_ColorTextures)
	{
		pTexture.reset();
		pTexture = nullptr;
	}

	m_pDepthTexture.reset();
	m_pDepthTexture = nullptr;
}
