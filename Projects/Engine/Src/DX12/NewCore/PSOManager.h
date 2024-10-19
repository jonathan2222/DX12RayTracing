#pragma once

#include "DX12/NewCore/GraphicsPSO.h"

namespace RS
{
	class PSOManager
	{
	public:

		GraphicsPSO* GetGraphicsPSO(uint64 key);

	private:
		std::unordered_map<uint64, GraphicsPSO> m_GraphicsPSOs;
		uint64 m_CurrentKey = 0;
	};
}