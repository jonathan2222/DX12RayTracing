#pragma once

#include <RSEngine.h>

struct Entity
{

};

class SandboxApp
{
public:
	SandboxApp();
	virtual ~SandboxApp();

    void FixedTick();
    void Tick(const RS::FrameStats& frameStats);

private:
	void Init();
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
		static const uint32 VertexData = 2;

		// TODO: Same Table
		static const uint32 Textures = 3;
		static const uint32 ConstantBufferViews = 4;
		static const uint32 UnordedAccessViews = 5;
	};

	uint32 m_NumVertices;
};
