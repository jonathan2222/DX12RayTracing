#pragma once

#include <RSEngine.h>

#include "Camera2D.h"
#include "DX12/NewCore/GraphicsPSO.h"

#include "Entity.h"

#include "Maths/GLMDefines.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include "Audio/Sound.h"

class Game1App
{
public:
	Game1App();
	virtual ~Game1App();

    void FixedTick();
    void Tick(const RS::FrameStats& frameStats);

private:
	void Init();
	void CreatePipelineStateEntities();
	void CreatePipelineStatePlayer();
	void CreateRootSignature();

	void UpdateEntities(const RS::FrameStats& frameStats);

	void DrawEntites(const RS::FrameStats& frameStats, std::shared_ptr<RS::CommandList> commandList);
	void DrawPlayer(std::shared_ptr<RS::CommandList> pCommandList);

	void SpawnEnemy(Entity::Type type, float initialSpeed, std::shared_ptr<RS::CommandList> pCommandList);
	void SpawnBit(const glm::vec2& pos, const glm::vec2& initialSpeed, std::shared_ptr<RS::CommandList> pCommandList);

	void ResizeEntitiesInstanceData(uint newCount, std::shared_ptr<RS::CommandList> pCommandList, bool updateData);
	void UpdateEntitiesInstanceData(std::shared_ptr<RS::CommandList> pCommandList);

	void FindOverlappingEnemiesWithPlayer();

private:
	std::shared_ptr<RS::RootSignature> m_pRootSignature;
	RS::GraphicsPSO m_EntitiesPSO;
	RS::GraphicsPSO m_PlayerPSO;
	std::shared_ptr<RS::VertexBuffer> m_pVertexBufferResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBufferResource;
	std::shared_ptr<RS::Texture> m_NullTexture;
	std::shared_ptr<RS::Texture> m_NormalTexture;

	std::shared_ptr<RS::RenderTarget> m_RenderTarget;
	struct RootParameter
	{
		static const uint32 CBVs = 0;
		static const uint32 SRVs = 1;
	};

	uint32 m_NumVertices;

	RS::Camera2D m_Camera;

	// Instande data
	struct InstanceData
	{
		glm::mat4 transform;
		glm::vec3 color;
		uint type;
		float scale = 1.f;
		uint padding0;
		uint padding1;
		uint padding2;
	};
	std::shared_ptr<RS::Buffer> m_InstanceBuffer;
	std::vector<InstanceData> m_InstanceData;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_InstanceDataSRVDesc;

	// Game data
	glm::vec2 m_WorldSize;
	std::vector<Entity> m_Entities;
	std::vector<uint> m_EntitiesThatOverlapPlayer;
	uint m_ActiveEntities = 0;

	glm::vec2 m_PlayerPosition;
	glm::vec2 m_PlayerSize = glm::vec2(3.f, 3.f);

	uint m_PlayerMaxHealth = 100;
	int m_PlayerCurrentHealth = 100;

	// Sounds
	RS::Sound* pButtonOnSound = nullptr;
	RS::Sound* pBackgroundSound = nullptr;
};
