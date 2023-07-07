#pragma once

#include <shared_mutex>

#include "RenderGroup.h"

namespace RS
{
	/*
	* // Use a group to record commands. This does not call the actual graphics api commands but instead put it into a format that is easy to convert from.
	* // The group will set the states, like raster state, IA state, blend state, rt, depth target, etc.
	* // If none is set, a default will be used. (As for vertex and index buffers, they should be able to be omitted if one uses bindless buffers for the data instead)
	* auto group = RS_Renderer::CreateGroup();
	* group.BeginCommands();
	* 
	* // State functions
	* group.SetVertexBuffer(some_vertex_bufffer);
	* group.SetIndexBuffer(some_index_buffer);
	* group.SetBlendState(some_blend_state);
	* group.SetRenderTarget(some_render_target);
	* 
	* // Resource bindings
	* group.BindConstantBuffer(0, some_constant_buffer);
	* group.BindTexture(0, some_texture);
	* 
	* // Draw/Dispatch commands
	* group.SetTopology(RS_TRIANGLES);
	* group.Draw(3);
	* 
	* group.EndCommands();
	* 
	* // Put this on a stack for rendering. The actual submit occurs on a render thread.
	* RS_Renderer::Submit(group);
	* 
	* auto group2 = RS_Renderer::CreateGroup();
	* group2.BeginCommands();
	* group2.BindConstantBuffer(0, some_constant_buffer);
	* group2.BindBuffer(0, some_buffer);
	* group2.BindRWBuffer(0, some_read_write_buffer);
	* group2.SetTopology(RS_LINES);
	* group2.EndCommands();
	* 
	* // Takes multiple groups and creates a valid group. The states can be overwritten by other groups.
	* // Like in this case group2 is the last in the list so it will take priority. e.g the contant buffer is bound by both groups on slot 0. So the lates will be used (group2).
	* auto compiledGroups = RS_Renderer::MergeGroups({group, group2});
	* RS_Renderer::Submit(compiledGroups);
	* 
	* // The group system is meant to be used with many groups for one effect.
	* // One group might be for view data, one for the initial state of the vertex data, and one for material data, etc.
	* // A group does not in it self have an order inside it. So calling BindBuffer after Draw does not break it.
	* // It is self containd so the state will not be changed until it is submitted. The next submitted group will not use states from a previous submit.
	* 
	* // Internally the renderer might inster some dependencies and remove/add some commands to increase performance and remove redundancies.
	*/

	class Renderer
	{
	public:

	public:
		Renderer() = default;
		~Renderer() = default;

		void Init();
		void Release();

		// Move these to an InternalRenderer which will have the render thread?
		void PreRender();
		void PostRender();
		
		// Uses a sort index to know the order of submit for one frame (used in the render thread?).
		void Submit(uint64 sortIndex, const Group& group);

		// Copies the group list to another list for the render thread to process. This prepares it for the next frame.
		void EndFrame();

	private:
		void RenderThread();

	private:
		std::shared_mutex		m_GroupDataMutex;
		std::vector<GroupData>	m_UnprocessedGroups;
		std::vector<GroupData>	m_Groups;
	};
}