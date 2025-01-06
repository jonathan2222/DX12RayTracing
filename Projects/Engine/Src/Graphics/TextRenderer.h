#pragma once

#include "Types.h"

#include <unordered_map>

#include "DX12/NewCore/Resources.h"

#include "DX12/NewCore/GraphicsPSO.h"
#include "DX12/NewCore/RenderTarget.h"

#include "Graphics/Color.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace RS
{
	class TextRenderer
	{
	public:
		struct Character
		{
			uint TextureID;
			int SizeX;
			int SizeY;
			int BearingX;
			int BearingY;
			uint Advance;
		};

	public:
		static std::shared_ptr<TextRenderer> Get();

		void Init();
		void Release();
		
		bool AddFont(const std::string& fontPath);

		void RenderText(const std::string& txt, uint posX, uint posY, float scale, const glm::vec3& color);

		void Render(std::shared_ptr<RS::CommandList> pCommandList, std::shared_ptr<RenderTarget> pRenderTarget);
	private:
		void InitRenderData();

	private:
		struct RenderItem
		{
			std::string text;
			uint x, y;
			float scale;
			Color color;
		};

	private:
		std::unordered_map<char, Character> m_Characters;
		std::vector<RenderItem> m_RenderItems;

		std::unordered_map<std::string, uint> m_FontNameToTextureListIndex;
		std::vector<std::unordered_map<char, std::shared_ptr<Texture>>> m_TextureMapList;

		FT_Library m_pLibrary = nullptr;

		std::shared_ptr<RS::RootSignature> m_pRootSignature;
		RS::GraphicsPSO m_GraphicsPSO;
		std::shared_ptr<RS::VertexBuffer> m_pVertexBufferResource;
	};
}