#pragma once

#include "Core/Console.h"
#include "EditorWindow.h"

namespace RSE
{
	class ConsoleInspector : public EditorWindow
	{
	public:
		ConsoleInspector(const std::string& name, bool enabled) : EditorWindow(name, enabled) {}

		virtual void Render() override;
		virtual bool GetEnableRequirements() const override { return RS::Console::Get() != nullptr; }

	private:
		void RenderVariable(RS::Console::Variable& var);

		void RenderFloatVariable(RS::Console::Variable& var);
		void RenderIntVariable(RS::Console::Variable& var);
		void RenderBoolVariable(RS::Console::Variable& var);

		void SortTable(std::vector<RS::Console::Variable>& refVariables);

	private:
		std::vector<RS::Console::Variable> m_Variables;
		std::vector<RS::Console::Variable> m_Functions;
		std::vector<RS::Console::Variable> m_Unknowns;

		uint64 m_StoredConsoleStateHash = 0;
	};
}