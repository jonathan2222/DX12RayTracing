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

		void SortTable(std::vector<RS::Console::Variable>& refVariables);

	private:
		std::vector<RS::Console::Variable> m_Variables;
		std::vector<RS::Console::Variable> m_Functions;
		std::vector<RS::Console::Variable> m_Unknowns;

		uint64 m_StoredConsoleStateHash = 0;
	};
}