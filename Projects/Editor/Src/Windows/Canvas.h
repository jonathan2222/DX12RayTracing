#pragma once

#include "DX12/NewCore/RootSignature.h"
#include "DX12/NewCore/Resources.h"
#include "DX12/NewCore/RenderTarget.h"

#include "EditorWindow.h"

namespace RSE
{
	class Canvas : public EditorWindow
	{
	public:
		Canvas(const std::string& name, bool enabled) : EditorWindow(name, enabled) {}

		virtual void Init() override;
		virtual void Release() override;
		virtual void Render() override;

	private:
		void RenderDX12();

		void CreatePipelineState();
		void CreateRootSignature();

	private:
		std::shared_ptr<RS::RootSignature> m_pRootSignature;
		ID3D12PipelineState* m_pPipelineState = nullptr;
		std::shared_ptr<RS::VertexBuffer> m_pVertexBufferResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBufferResource;
		std::shared_ptr<RS::Texture> m_NullTexture;
		std::shared_ptr<RS::Texture> m_NormalTexture;

		std::shared_ptr<RS::RenderTarget> m_RenderTarget;

		struct RootParameter
		{
			static const uint32 PixelData = 0;
			static const uint32 PixelData2 = 1;

			// TODO: Same Table
			static const uint32 Textures = 2;
			static const uint32 ConstantBufferViews = 3;
			static const uint32 UnordedAccessViews = 4;
		};
	};
}