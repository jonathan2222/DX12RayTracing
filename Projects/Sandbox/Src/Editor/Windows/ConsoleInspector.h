#pragma once

#include "Core/Console.h"

namespace RSE
{
	class ConsoleInspector
	{
	public:
		bool m_Enabled = false;

		void Render();

	private:
		void RenderVariable(RS::Console::Variable& var);

		void RenderFloatVariable(RS::Console::Variable& var);
		void RenderIntVariable(RS::Console::Variable& var);

	};
}