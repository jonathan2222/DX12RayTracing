#pragma once

#include "DX12/NewCore/Resources.h"
#include <DX12/NewCore/RootSignature.h>

namespace RS
{
	class RenderCore
	{
	public:
		static std::shared_ptr<RenderCore> Get();

		std::shared_ptr<Texture> pTextureBlack;
		std::shared_ptr<Texture> pTextureWhite;

	public:
		void Init();
		void Destory();

		std::shared_ptr<RootSignature> GetMainRootSignature();

	private:

	};

	inline static std::shared_ptr<RenderCore> GetRenderCore()
	{
		return RenderCore::Get();
	}
}