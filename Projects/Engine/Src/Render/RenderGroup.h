#pragma once

#include "GUI/LogNotifier.h"

namespace RS
{
	enum class Topology : uint8
	{
		None,
		Triagles,
		TriangeList,
		Lines,
		LineList,
		Points
	};

	struct RenderResource
	{
		//Dx12Resource*
	};

	struct DummyStruct {};

	struct GraphicsGroupData
	{
		// TODO: Create a Rect struct!
		std::vector<DummyStruct> m_Viewports;
		std::vector<DummyStruct> m_ScissorRects;
		Topology m_Topology = Topology::None;

		// TODO: RenderResource here should be encapsulated in a Buffer, or Vertex/Index Buffer classes.
		std::vector<RenderResource*> m_VertexBuffers;
		RenderResource* m_IndexBuffer = nullptr;

		// TODO: RenderResource here should be encapsulated in a RenderTarget and/or DepthTarget classes.
		std::vector<RenderResource*> m_RenderTargets;
		RenderResource* m_pDepthTarget = nullptr;

		// TODO: RenderResource here should be encapsulated in Buffer(view?) and Texture(view?) classes.
		// They are split because of the different bindings in the shader.
		std::vector<RenderResource*> m_Buffers;
		std::vector<RenderResource*> m_RWBuffers;
		std::vector<RenderResource*> m_ConstantBuffers;
		std::vector<RenderResource*> m_Textures;
		std::vector<RenderResource*> m_RWTextures;

		uint32 m_Count = 0;
		uint32 m_Offset = 0;
	};

	struct ComputeGroupData
	{
		// Filled by the Dispatch call in the Group class.
		// TODO: RenderResource here should be encapsulated in Buffer(view?) and Texture(view?) classes.
		std::vector<RenderResource*> m_Buffers;
		std::vector<RenderResource*> m_RWBuffers;
		std::vector<RenderResource*> m_ConstantBuffers;
		std::vector<RenderResource*> m_Textures;
		std::vector<RenderResource*> m_RWTextures;

		// Filled by the Dispatch call in the Group class.
		uint32 m_ThreadCountX = 0;
		uint32 m_ThreadCountY = 0;
		uint32 m_ThreadCountZ = 0;
		uint32 m_GroupCountX = 0;
		uint32 m_GroupCountY = 0;
		uint32 m_GroupCountZ = 0;
	};

	struct GroupData
	{
		//union
		//{
			GraphicsGroupData graphicsData;
			ComputeGroupData computeData;
		//};
	};


	class Renderer;
	// Internally this might want to use a RenderContext type of architecture? At least when the renderframe processes them.
	// TODO: Make one group for Graphics commands and one for Compute commands (this is to allow for a raytracing group later).
	class Group : public GroupData
	{
	public:
		void SetVertexBuffer() { RS_NOTIFY_CRITICAL("SetVertexBuffer is not implemented!"); }
		void SetIndexBuffer() { RS_NOTIFY_CRITICAL("SetIndexBuffer is not implemented!"); }
		void SetRenderTargets() { RS_NOTIFY_CRITICAL("SetRenderTargets is not implemented!"); }
		void SetDepthTarget() { RS_NOTIFY_CRITICAL("SetDepthTarget is not implemented!"); }
		void SetBlendState() { RS_NOTIFY_CRITICAL("SetBlendState is not implemented!"); }
		void SetTopology(Topology topology) { RS_NOTIFY_CRITICAL("SetTopology is not implemented!"); }
		void SetViewports() { RS_NOTIFY_CRITICAL("SetViewports is not implemented!"); }
		void SetScissorRects() { RS_NOTIFY_CRITICAL("SetScissorRects is not implemented!"); }

		// TODO: Might want to infere some of the other commands from this? Or at least bindings? Or should that maybe be done outside the renderer?
		void SetShader() { RS_NOTIFY_CRITICAL("SetShader is not implemented!"); }

		void ClearBuffer() { RS_NOTIFY_CRITICAL("ClearBuffer is not implemented!"); }
		void ClearTexture() { RS_NOTIFY_CRITICAL("ClearTexture is not implemented!"); }

		void BindConstantBuffer() { RS_NOTIFY_CRITICAL("BindConstantBuffer is not implemented!"); }
		void BindBuffer() { RS_NOTIFY_CRITICAL("BindBuffer is not implemented!"); }
		void BindRWBuffer() { RS_NOTIFY_CRITICAL("BindRWBuffer is not implemented!"); }
		void BindTexture() { RS_NOTIFY_CRITICAL("BindTexture is not implemented!"); }
		void BindRWTexture() { RS_NOTIFY_CRITICAL("BindRWTexture is not implemented!"); }

		void Draw(uint32 offset, uint32 count) { RS_NOTIFY_CRITICAL("Draw is not implemented!"); }
		void DrawInstanced() { RS_NOTIFY_CRITICAL("DrawInstanced is not implemented!"); }
		void DrawIndirect() { RS_NOTIFY_CRITICAL("DrawIndirect is not implemented!"); }
		void DrawInstancedIndirect() { RS_NOTIFY_CRITICAL("DrawInstancedIndirect is not implemented!"); }
		void Dispatch(uint32 x, uint32 y, uint32 z) { RS_NOTIFY_CRITICAL("Dispatch is not implemented!"); }

	private:
		// The renderer takes the data and order/optimize the calls both internally and with the other groups.
		friend class Renderer;
		// Some state data to be parsed/ordered by the render thread.

		// Should this be used?
		uint64 m_StateHash = 0; // Should be different depending on the calls and its data.
	};
}