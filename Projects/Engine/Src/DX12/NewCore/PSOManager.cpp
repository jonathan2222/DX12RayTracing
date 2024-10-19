#include "PreCompiled.h"
#include "PSOManager.h"

RS::GraphicsPSO* RS::PSOManager::GetGraphicsPSO(uint64 key)
{
    auto it = m_GraphicsPSOs.find(key);
    if (it == m_GraphicsPSOs.end())
        return nullptr;
    return &it->second;
}
