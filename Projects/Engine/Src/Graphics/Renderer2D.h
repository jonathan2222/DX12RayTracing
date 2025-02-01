#pragma once

#include "Graphics/Color.h"
#include "DX12/NewCore/RootSignature.h"
#include "DX12/NewCore/GraphicsPSO.h"
#include "DX12/NewCore/Resources.h"

namespace RS
{
	class Renderer2D
	{
	public:

		void Init();
		void Destory();

		void DrawRect(float x, float y, float width, float height, Color color);
		void DrawCircle(float x, float y, float radius, Color color);

		void Render();

	private:
		void CreateRootSignature();
		void CreateGraphicsPSO();

	private:
		std::shared_ptr<RS::RootSignature> m_pRootSignature;
		RS::GraphicsPSO m_GraphicsPSO;
		std::shared_ptr<RS::VertexBuffer> m_pVertexBufferResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBufferResource;
	};
}
