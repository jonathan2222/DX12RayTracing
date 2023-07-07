#include "PreCompiled.h"
#include "Renderer.h"

using namespace RS;

void Renderer::Init()
{
}

void Renderer::Release()
{
}

void Renderer::PreRender()
{
}

void Renderer::PostRender()
{
}

void RS::Renderer::Submit(uint64 sortIndex, const Group& group)
{
	std::unique_lock lock(m_GroupDataMutex);
	m_UnprocessedGroups.push_back(group);
}

void RS::Renderer::EndFrame()
{
	std::unique_lock lock(m_GroupDataMutex);
	RS_ASSERT(m_Groups.empty(), "Should not be any groups left!");
	//m_Groups = m_UnprocessedGroups;
}

void RS::Renderer::RenderThread()
{
	// C
}
