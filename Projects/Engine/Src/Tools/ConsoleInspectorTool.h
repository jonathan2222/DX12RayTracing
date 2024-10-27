#pragma once

#include "Core/Console.h"
#include "Tools/DebugWindow.h"

namespace RS
{
	class ConsoleInspectorTool
	{
	public:

		static void ImGuiRender(std::vector<RS::Console::Variable>& variables,
			std::vector<RS::Console::Variable>& functions,
			std::vector<RS::Console::Variable>& unknowns);

	private:
		static void RenderVariable(Console::Variable& var);

		static void RenderFloatVariable(Console::Variable& var);
		static void RenderIntVariable(Console::Variable& var);
		static void RenderBoolVariable(Console::Variable& var);

		static void SortTable(std::vector<Console::Variable>& refVariables);
	};

	class DebugConsoleInspectorWindow : public DebugWindow
	{
	public:
		DebugConsoleInspectorWindow(const std::string& name) : DebugWindow(name) {}

		void Render() override;

		bool GetEnableRequirements() const override { return RS::Console::Get() != nullptr; }

	private:
		std::vector<RS::Console::Variable> m_Variables;
		std::vector<RS::Console::Variable> m_Functions;
		std::vector<RS::Console::Variable> m_Unknowns;

		uint64 m_StoredConsoleStateHash = 0;
	};
}
