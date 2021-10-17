#pragma once

namespace RS
{
	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		void Init();
		void Release();

		void PreRender();
		void PostRender();

	private:

	};
}