#pragma once

#include <RSEngine.h>

#include "Camera.h"

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

		static const uint32 GlobalRTextures = 0; // R Textures that are set used by the engine. (SRV)
		static const uint32 GlobalRWTextures = 1; // RW Textures that are set used by the engine. (UAV)
		static const uint32 LocalRTextures = 2; // R Textures that can be changed per frame per shader. This uses bindless data. (SRV)
		static const uint32 LocalRWTextures = 3; // RW Textures that can be changed per frame per shader. This uses bindless data. (UAV)
		//static const uint32 
	};

	uint32 m_NumVertices;

	RS::Camera m_Camera;
};
