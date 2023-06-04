#include "PreCompiled.h"
#include "Console.h"

#include "GUI/LogNotifier.h"

#include <ctype.h>

RS::Console* RS::Console::Get()
{
	static std::unique_ptr<RS::Console> pConsole{ std::make_unique<RS::Console>() };
	return pConsole.get();
}

void RS::Console::Init()
{
	memset(m_InputBuf, 0, ARRAYSIZE(m_InputBuf));

	AddVar("Console.DisplayHeightRatio", m_DisplayHeightRatio, Flag::NONE, "Ratio of the console to the height of the window.");
	AddVar("Console.Opacity", m_Opacity, Flag::NONE, "Opacity of the console.");
	AddVar("Console.SearchPopupMaxDisplayCount", m_SearchPopupMaxDisplayCount, Flag::NONE, "Maximum number of items that fit into the popup display for search matches.");

	AddVar("Console.Info.CurrentCmdHistoryIndexOffset", m_CurrentCmdHistoryIndexOffset, Flag::ReadOnly, "The index offset in the executed commands history list.");

	AddFunction("Console.ListCommands", [this](FuncArgs args)->void
		{
			if (!ValidateFuncArgs(args, { {"-d"} }, false))
				return;

			{
				Line line;
				line.str = Utils::Format("Commands:");
				line.color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
				m_History.push_back(line);
			}

			std::vector<Variable> vars;
			for (auto& e : m_VariablesMap)
				vars.push_back(e.second);
			std::sort(vars.begin(), vars.end(),
				[](const Variable& a, const Variable& b)->bool
				{
					return a.name < b.name;
				}
			);

			bool hasDetail = args.Get("-d").has_value();
			for (Variable& v : vars)
			{
				std::string t = IsTypeVariable(v.type) ? "V" : "F";
				if (hasDetail) t = VarTypeToString(v.type);
				std::string commandType = Utils::Format("[{}]", t);

				Line line;
				line.str = "  " + Utils::FillRight(commandType, ' ', hasDetail ? 14 : 4) + v.name;
				line.color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
				m_History.push_back(line);
			}
		},
		Flag::NONE, "List all commands.\nUse '-d' to display the variable type."
	);

	AddFunction("Console.Clear", [this](FuncArgs args)->void
		{
			if (!ValidateFuncArgs(args, { {"-c"} }, false))
				return;

			m_History.clear();
			if (args.Get("-c"))
			{
				m_CommandHistory.clear();
				m_CurrentCmdHistoryIndexOffset = -1;
			}
		},
		Flag::NONE, "Clear the console.\nUse '-c' to also clear the executed command history."
	);

	AddFunction("Console.Help", [this](FuncArgs args)->void
		{
			if (!ValidateFuncArgs(args, { {FuncArg::TypeFlag::Named} }, ValidateFuncArgsFlag::ArgCountMustMatch | ValidateFuncArgsFlag::TypeMatchOnly))
				return;

			auto arg = args.Get(0);
			if (arg)
			{
				Variable* pVariable = GetVariable(arg.value().name);
				if (pVariable)
				{
					std::vector<std::string> v = Utils::Split(pVariable->documentation, '\n');
					if (v.empty())
						Print(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "\tMissing documentation");
					for (std::string docsLine : v)
						Print(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "\t{}", docsLine);
				}
			}
		},
		{ FuncArg::TypeFlag::Int, FuncArg::TypeFlag::Float, FuncArg::TypeFlag::Function },
		Flag::NONE, "Get the documentation from the passed command.\nExample:\n\tConsole.Help Console.Clear\n\tGives you the documentation of the Clear command."
	);
}

void RS::Console::Release()
{
}

bool RS::Console::RemoveVar(const std::string& name)
{
	std::lock_guard<std::mutex> lock(m_VariablesMutex);
	m_VariablesMap.erase(name);
	return false;
}

void RS::Console::Render()
{
	if (!m_Enabled)
		return;

	const ImVec2& vp_size = ImGui::GetMainViewport()->Size;

	ImGui::SetNextWindowBgAlpha(m_Opacity);

	const uint32 height = vp_size.y * m_DisplayHeightRatio;

	ImVec2 winPos(0.f, 0.0f);
	ImVec2 winPivot(0.f, 0.0f);
	ImGui::SetNextWindowPos(winPos, ImGuiCond_Always, winPivot);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysVerticalScrollbar;
	ImGui::Begin("_RS_Console", NULL, flags);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

	// Seems to not do what I want. This will resize it depending on the contents of the window.
	ImGui::SetWindowSize(ImVec2(vp_size.x, -(float)height), ImGuiCond_Always);

	// Sets a clipping space, will not allow the window to resize, though negates the effect of the negative height. So need to set it again.
	ImGuiListClipper clipper;
	int maxItemDisplayCount = height / ImGui::GetTextLineHeightWithSpacing();
	clipper.Begin(maxItemDisplayCount, ImGui::GetTextLineHeightWithSpacing());
	while (clipper.Step())
	{
		for (int32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			int32 start = (int32)std::max(0, (int)m_History.size() - maxItemDisplayCount);
			uint32 index = (uint32)(i + start);
			if (index < m_History.size())
			{
				Line& line = m_History[index];
				ImGui::TextColored(line.color, line.str.c_str());
			}
		}
	}

	ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

	// Command-line input
	bool reclaim_focus = true; // Always focus.
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackEdit;
	ImGui::PushItemWidth(vp_size.x);
	if (ImGui::InputText("Input", m_InputBuf, IM_ARRAYSIZE(m_InputBuf), input_text_flags, Console::InputTextCallback, (void*)this))
	{
		ImGuiIO& io = ImGui::GetIO();
		bool usedHistory = io.KeysDown[ImGuiKey_DownArrow] || io.KeysDown[ImGuiKey_UpArrow];
		if (!usedHistory)
		{
			ComputeCurrentSearchableItem(m_InputBuf);

			m_CurrentMatchedVarIndex = UINT32_MAX;
			m_CurrentCmdHistoryIndexOffset = -1;
		}

		Search(m_CurrentSearchableItem);
	}
	ImGui::PopItemWidth();

	// Enter pressed on CMD.
	bool isEnterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter, true) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, true);
	if (ImGui::IsItemFocused() && isEnterPressed)
	{
		// If search popup was used
		if (m_MatchedSearchList.empty() == false)
		{
			// Apply matched var to the text input, if the user have used the history.
			if (m_CurrentMatchedVarIndex < m_MatchedSearchList.size())
			{
				MatchedSearchItem& item = m_MatchedSearchList[m_CurrentMatchedVarIndex];
				memcpy(m_InputBuf + m_CurrentSearchableItemStartPos, item.fullLine.c_str(), std::min((size_t)ARRAYSIZE(m_InputBuf), item.fullLine.size()));
				m_MatchedSearchList.clear();
				m_CurrentMatchedVarIndex = UINT32_MAX;

				ComputeCurrentSearchableItem(m_InputBuf);
			}
			m_CurrentCmdHistoryIndexOffset = -1;
		}
		else
		{
			std::string cmd = m_InputBuf;
			if (cmd.empty() == false)
				ExecuteCommand(cmd);
			strcpy(m_InputBuf, "");
			m_CurrentSearchableItemStartPos = 0;
			m_DisplaySearchResultsForEmptySearchWord = false;
			m_CurrentSearchableItem.clear();
			m_CurrentCmdHistoryIndexOffset = -1;
		}
		reclaim_focus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

	ImGui::End();
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding

	// Pupop search matches window.
	if (m_MatchedSearchList.empty() == false)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
		ImVec2 popupWinPos(0.f, height + ImGui::GetTextLineHeight());
		ImGui::SetNextWindowPos(popupWinPos, ImGuiCond_Always, winPivot);
		if (ImGui::Begin("_SearchPopup", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
		{
			// Seems to not do what I want. This will resize it depending on the contents of the window.
			ImGui::SetWindowSize(ImVec2(vp_size.x, 0.0f), ImGuiCond_Always);

			uint32 frameOffset = 0;
			if (m_CurrentMatchedVarIndex != UINT32_MAX)
			{
				if (m_CurrentMatchedVarIndex >= m_SearchPopupMaxDisplayCount)
					frameOffset = m_CurrentMatchedVarIndex - m_SearchPopupMaxDisplayCount + 1;
			}
			for (int32 i = 0; i < m_SearchPopupMaxDisplayCount; ++i)
			{
				uint32 index = (uint32)(i + frameOffset);
				if (index < m_MatchedSearchList.size())
				{
					MatchedSearchItem& item = m_MatchedSearchList[index];
					std::vector<Line>& parts = item.parts;
					for (int32 j = 0; j < parts.size(); ++j)
					{
						Line line = parts[j];
						line.color.w = index == m_CurrentMatchedVarIndex ? 1.0f : 0.5f;
						ImGui::TextColored(line.color, line.str.c_str());
						if (j != parts.size() - 1)
							ImGui::SameLine(0, 0);
					}
				}
			}

			ImGui::End();
		}
		ImGui::PopStyleVar(2); // ImGuiStyleVar_WindowPadding, ImGuiStyleVar_WindowMinSize
	}
}

void RS::Console::RenderStats()
{
	const ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	ImGui::Begin("_RS_ConsoleStats", NULL, flags);

	ImGui::End();
}

void RS::Console::Print(const char* txt)
{
	Print(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), txt);
}

void RS::Console::Print(const ImVec4& color, const char* txt)
{
	Line line;
	line.str = txt;
	line.str = Utils::ReplaceAll(line.str, "\t", "    "); // Allow tab character to be printed. (If not, ImGui will convert it to a completion call)
	line.color = color;
	m_History.push_back(line);
}

bool RS::Console::AddFunction(const std::string& name, std::function<void(FuncArgs)> func, Flags flags, const std::string& docs)
{
	Variable var;
	var.name = name;
	var.pVar = nullptr;
	var.type = Type::Function;
	var.func = func;
	var.flags = flags;
	var.documentation = docs;
	return AddVarInternal(name, var);
}

bool RS::Console::AddFunction(const std::string& name, Func func, const std::vector<FuncArg::TypeFlags>& searchableTypes, Flags flags, const std::string& docs)
{
	Variable var;
	var.name = name;
	var.pVar = nullptr;
	var.type = Type::Function;
	var.func = func;
	var.flags = flags;
	var.searchableTypes = searchableTypes;
	var.documentation = docs;
	return AddVarInternal(name, var);
}

bool RS::Console::ValidateFuncArgs(FuncArgs args, FuncArgs validArgs, ValidateFuncArgsFlags flags)
{
	const bool argCountMustMatch = flags & ValidateFuncArgsFlag::ArgCountMustMatch;
	const bool typeMatchOnly = flags & ValidateFuncArgsFlag::TypeMatchOnly;

	bool failed = false;

	bool firstPrint = true;
	auto PrintStart = [&]()
	{
		if (firstPrint)
		{
			Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid arguments:");
			firstPrint = false;
			failed = true;
		}
	};

	uint32 nonNamedArgCount = 0;
	for (uint32 i = 0; i < args.size(); ++i)
	{
		const FuncArg& arg = args[i];
		if (arg.name.empty() && arg.type == FuncArg::TypeFlag::NONE)
		{
			PrintStart();
			Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "\tEmpty arguments are not allowed!");
		}

		if (!arg.name.empty())
		{
			auto it = std::find_if(validArgs.begin(), validArgs.end(), [arg, typeMatchOnly](const FuncArg& arg2)->bool
				{
					return (arg.type & arg2.type) != 0 && (typeMatchOnly ? true : arg.name == arg2.name);
				}
			);
			if (it == validArgs.end())
			{
				PrintStart();
				Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "\t{} is not a valid named argument!", arg.name);
			}
		}
		else
			nonNamedArgCount++;
	}

	uint32 validNonNamedArgCount = 0;
	std::for_each(validArgs.begin(), validArgs.end(), [&](const FuncArg& arg)->void
		{
			if (arg.type != FuncArg::TypeFlag::NONE && (arg.type & FuncArg::TypeFlag::Named) != 0 && arg.name.empty())
				validNonNamedArgCount++;
		}
	);

	if ((!argCountMustMatch && nonNamedArgCount > validNonNamedArgCount) ||
		(argCountMustMatch && nonNamedArgCount == validNonNamedArgCount))
	{
		PrintStart();
		if (argCountMustMatch)
			Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "\tUnsupported number of unnamed arguments! Wanted {}, got {}", validNonNamedArgCount, nonNamedArgCount);
		else
			Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "\tToo many unnamed arguments! Wanted {}, got {}", validNonNamedArgCount, nonNamedArgCount);
	}

	if (argCountMustMatch && args.size() != validArgs.size())
	{
		PrintStart();
		Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "\tToo many arguments! Wanted {}, got {}", validArgs.size(), args.size());
	}

	return !failed;
}

bool RS::Console::AddVarInternal(const std::string& name, const Variable& var)
{
	RS_ASSERT_NO_MSG(!name.empty());
	RS_ASSERT_NO_MSG(!var.name.empty());
	RS_ASSERT_NO_MSG((var.type != Type::Function && var.pVar != nullptr) || (var.type == Type::Function && var.func));

	std::lock_guard<std::mutex> lock(m_VariablesMutex);
	if (auto it = std::find_if(m_VariablesMap.begin(), m_VariablesMap.end(),
		[&var](auto& e)->bool {
			return (e.second.name == var.name) && e.second.type == var.type;
		}); it != m_VariablesMap.end())
	{
		LOG_ERROR("Trying to add a var ({}) that already exists!", name.c_str());
		return false;
	}

	m_VariablesMap[name] = var;

	m_SearchableItems.push_back(name);

	std::sort(m_SearchableItems.begin(), m_SearchableItems.end());

	return true;
}

RS::Console::Variable* RS::Console::GetVariable(const std::string& name)
{
	auto it = m_VariablesMap.find(name);
	if (it != m_VariablesMap.end())
		return &it->second;
	return nullptr;
}

int RS::Console::HandleInputText(ImGuiInputTextCallbackData* data)
{
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion: // Pressing Tab
		{
			if (m_MatchedSearchList.size() < 1)
				return 0;

			std::string str = PerformSearchCompletion();
			if (str.empty() == false)
			{
				data->DeleteChars(m_CurrentSearchableItemStartPos, data->BufTextLen - m_CurrentSearchableItemStartPos);
				data->InsertChars(m_CurrentSearchableItemStartPos, str.c_str());
				ComputeCurrentSearchableItem(data->Buf);
			}
			m_CurrentCmdHistoryIndexOffset = -1;
		}
		break;
	case ImGuiInputTextFlags_CallbackHistory: // Up/Down arrows
		{
			if (m_MatchedSearchList.size() < 1)
			{
				if (data->EventKey == ImGuiKey_DownArrow)
					m_CurrentCmdHistoryIndexOffset = std::max(m_CurrentCmdHistoryIndexOffset - 1, 0);
				else
					m_CurrentCmdHistoryIndexOffset = std::min(m_CurrentCmdHistoryIndexOffset + 1, (int32)m_CommandHistory.size() - 1);

				// Note: This does not change the searchable item!
				if (m_CurrentCmdHistoryIndexOffset < m_CommandHistory.size())
				{
					std::string& cmd = m_CommandHistory[m_CommandHistory.size() - m_CurrentCmdHistoryIndexOffset - 1];
					data->DeleteChars(m_CurrentSearchableItemStartPos, data->BufTextLen - m_CurrentSearchableItemStartPos);
					data->InsertChars(m_CurrentSearchableItemStartPos, cmd.c_str());
				}
			}
			else
			{
				if (data->EventKey == ImGuiKey_DownArrow)
					m_CurrentMatchedVarIndex = std::min(m_CurrentMatchedVarIndex + 1, (uint32)m_MatchedSearchList.size() - 1);
				else if (data->EventKey == ImGuiKey_UpArrow)
				{
					if (m_CurrentMatchedVarIndex == UINT32_MAX)
						m_CurrentMatchedVarIndex = (uint32)m_MatchedSearchList.size() - 1;
					else
						m_CurrentMatchedVarIndex = m_CurrentMatchedVarIndex > 0 ? m_CurrentMatchedVarIndex - 1 : 0;
				}

				// Note: This does not change the searchable item!
				RS_ASSERT_NO_MSG(m_CurrentMatchedVarIndex < m_MatchedSearchList.size());
				MatchedSearchItem& item = m_MatchedSearchList[m_CurrentMatchedVarIndex];
				data->DeleteChars(m_CurrentSearchableItemStartPos, data->BufTextLen - m_CurrentSearchableItemStartPos);
				data->InsertChars(m_CurrentSearchableItemStartPos, item.fullLine.c_str());
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

void RS::Console::ExecuteCommand(const std::string& cmdLine)
{
	std::vector<std::string> tokens = Utils::Split(cmdLine, ' ');
	if (tokens.empty())
	{
		RS_NOTIFY_ERROR("Trying to execute an empty command line!");
		return;
	}

	const uint32 numArguments = (uint32)tokens.size() - 1;

	// Find command.
	std::string cmd = tokens[0];
	auto it = m_VariablesMap.find(cmd);
	if (it == m_VariablesMap.end())
	{
		RS_NOTIFY_WARNING("Command '{}' not found!", cmd);
		Print(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Command '{}' not found!", cmd);
		m_CommandHistory.push_back(cmdLine);
		return;
	}

	RS_NOTIFY_SUCCESS("Executing: {}", cmd);

	Variable& var = it->second;
	if (var.type == Type::Function)
	{
		Print(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), cmdLine.c_str());

		FuncArgsInternal args;
		for (uint32 i = 1; i < tokens.size(); ++i)
		{
			FuncArg arg;
			bool isNamed = (tokens[i][0] == '-' && tokens[i].size() > 1 && std::isalpha(tokens[i][1])) ||
				(tokens[i][0] != '-' && std::isalpha(tokens[i][0]));
			if (isNamed)
			{
				arg.name = tokens[i];
				i++;
				if (i >= tokens.size())
				{
					arg.type |= FuncArg::TypeFlag::Named;
					auto it = m_VariablesMap.find(arg.name);
					if (it != m_VariablesMap.end())
						arg.type |= TypeToFuncArgType(it->second.type);
					args.push_back(arg);
					break;
				}
			}
			isNamed = (tokens[i][0] == '-' && tokens[i].size() > 1 && std::isalpha(tokens[i][1])) ||
				(tokens[i][0] != '-' && std::isalpha(tokens[i][0]));
			if (isNamed)
			{
				i--;
				arg.type |= FuncArg::TypeFlag::Named;
				auto it = m_VariablesMap.find(arg.name);
				if (it != m_VariablesMap.end())
					arg.type |= TypeToFuncArgType(it->second.type);
				args.push_back(arg);
				continue;
			}

			std::string value = tokens[i];
			Type type = StringToVarType(value);
			arg.type = FuncArg::TypeFlag::NONE;
			if (IsTypeInt(type) || IsTypeUInt(type))
			{
				type = Type::Int32;
				arg.type = FuncArg::TypeFlag::Int;
			}
			else if (IsTypeFloat(type))
			{
				type = Type::Float;
				arg.type = FuncArg::TypeFlag::Float;
			}
			if (!arg.name.empty())
			{
				arg.type |= FuncArg::TypeFlag::Named;
				auto it = m_VariablesMap.find(arg.name);
				if (it != m_VariablesMap.end())
					arg.type |= TypeToFuncArgType(it->second.type);
			}
			StringToVarValue(type, value, (void*)&arg.value);
			args.push_back(arg);
		}
		var.func(args);
		m_CommandHistory.push_back(cmdLine);
		return;
	}
	else
	{
		if (numArguments == 0)
		{ // Reading

			std::string line = Utils::Format("{} == {}", cmd, VarValueToString(var.type, var.pVar));
			Print(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), line.c_str());
			m_CommandHistory.push_back(cmdLine);
			return;
		}
		else if (numArguments == 1 || (numArguments == 2 && tokens[1] == "="))
		{ // Writing
			std::string value = tokens[numArguments];

			Type type = StringToVarType(value);
			if ((IsTypeInt(type) && IsTypeUInt(var.type)) ||
				(IsTypeUInt(type) && IsTypeInt(var.type)))
			{
				int typeBC = GetByteCountFromType(type);
				int vtypeBC = GetByteCountFromType(var.type);
				if (typeBC <= vtypeBC)
					type = var.type; // Change signed/unsigned.
				else
				{
					Print(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), cmdLine.c_str());
					std::string wantType = VarTypeToString(var.type);
					std::string gotType = VarTypeToString(type);
					RS_NOTIFY_ERROR("The given type is too large for the stored type. Wants {} but got {}", wantType, gotType);
					Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "The given type is too large for the stored type. Wants {} but got {}", wantType, gotType);
					m_CommandHistory.push_back(cmdLine);
					return;
				}
			}

			if (IsOfSameType(type, var.type))
			{
				StringToVarValue(var.type, value, var.pVar);
				std::string line = Utils::Format("{} = {}", cmd, value);
				Print(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), line.c_str());
				m_CommandHistory.push_back(cmdLine);
				return;
			}
			else
			{
				Print(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), cmdLine.c_str());
				std::string wantType = VarTypeToString(var.type);
				std::string gotType = VarTypeToString(type);
				RS_NOTIFY_ERROR("Type does not match! Variable wants {} but got {}", wantType, gotType);
				Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Type does not match! Variable wants {} but got {}", wantType, gotType);
				m_CommandHistory.push_back(cmdLine);
				return;
			}
		}
		else
		{
			Print(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), cmdLine.c_str());
			RS_NOTIFY_ERROR("Writing {} arguments to a variable is not supported!", numArguments);
			Print(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Writing {} arguments to a variable is not supported!", numArguments);
			m_CommandHistory.push_back(cmdLine);
			return;
		}
	}
}

void RS::Console::Search(const std::string& searchLine)
{
	m_MatchedSearchList.clear();
	if (searchLine.empty() && !m_DisplaySearchResultsForEmptySearchWord) return;

	// Each part is searched by matching sequential chars and then combined.
	std::vector<std::string> searchStrings = Utils::Split(searchLine, '.');
	if (searchStrings.empty() && !m_DisplaySearchResultsForEmptySearchWord) return;
	if (!searchLine.empty() && searchLine.back() == '.') searchStrings.push_back("");

	std::vector<MatchedSearchItem> intermediateMatchedList;

	for (std::string item : m_SearchableItems)
	{
		// If it is the exact same as in the sarchable list, then we do need to search.
		if (Utils::ToLower(item) == Utils::ToLower(searchLine))
			return;

		Search(item, searchStrings, intermediateMatchedList);
	}

	// Sort intermediate list
	std::stable_sort(intermediateMatchedList.begin(), intermediateMatchedList.end(),
		[](const MatchedSearchItem& a, const MatchedSearchItem& b) -> bool
		{
			return a.startIndex < b.startIndex;
		}
	);
	std::sort(intermediateMatchedList.begin(), intermediateMatchedList.end(),
		[](const MatchedSearchItem& a, const MatchedSearchItem& b) -> bool
		{
			return a.matchedCount > b.matchedCount;
		}
	);

	// Filter searchable types
	if (m_ValidSearchableArgTypes.empty() == false)
	{
		auto interIt = std::remove_if(intermediateMatchedList.begin(), intermediateMatchedList.end(),
			[&](const MatchedSearchItem& e)->bool
			{
				auto it = m_VariablesMap.find(e.fullLine);
				if (it != m_VariablesMap.end())
				{
					bool valid = false;
					for (FuncArg::TypeFlags type : m_ValidSearchableArgTypes)
					{
						FuncArg::TypeFlags varType = TypeToFuncArgType(it->second.type);
						if (type & varType)
							valid = true;
					}
					return valid == false;
				}
				return false;
			}
		);
		if (interIt != intermediateMatchedList.end())
			intermediateMatchedList.erase(interIt, intermediateMatchedList.end());
	}

	// Copy to the matched list.
	m_MatchedSearchList = intermediateMatchedList;
}

int RS::Console::GetByteCountFromType(Type type)
{
	int byteCount = 0;
	if (IsTypeInt(type))
		byteCount = 1 << ((int)type - (int)Type::Int8);
	if (IsTypeUInt(type))
		byteCount = 1 << ((int)type - (int)Type::UInt8);
	if (IsTypeFloat(type))
		byteCount = 1 << ((int)type - (int)Type::Float);
	return byteCount;
}

std::string RS::Console::PerformSearchCompletion()
{
	// Restriction: Completion can only be done from start of the searchable.

	if (m_MatchedSearchList.size() < 1)
		return std::string(); // No matched searches, cannot complete.

	int32 maxMatchedCount = 0;
	std::vector<MatchedSearchItem> validMatches;
	for (uint32 i = 0; i < m_MatchedSearchList.size(); ++i)
	{
		MatchedSearchItem& item = m_MatchedSearchList[i];
		if (item.startIndex == 0)
		{
			maxMatchedCount = std::max(maxMatchedCount, item.matchedCount);
			validMatches.push_back(item);
		}
	}

	std::remove_if(validMatches.begin(), validMatches.end(), [maxMatchedCount](const MatchedSearchItem& e)->bool { return e.matchedCount == maxMatchedCount; });

	// Get max match.
	for (int32 matchedCount = maxMatchedCount; matchedCount < 512; ++matchedCount)
	{
		// Test with one higher.
		bool foundCompleteTarget = false;
		std::string str;
		int32 currentCount = matchedCount + 1;
		for (uint32 i = 0; i < validMatches.size(); ++i)
		{
			MatchedSearchItem& item = validMatches[i];
			if (item.fullLine.size() < currentCount)
			{
				foundCompleteTarget = true;
				currentCount--;
				str = item.fullLine.substr(0, currentCount); // To be safe, we save it again.
				break;
			}
			else if (str.empty()) str = item.fullLine.substr(0, currentCount);
			else
			{
				if (str != item.fullLine.substr(0, currentCount))
				{
					foundCompleteTarget = true;
					currentCount--;
					str = item.fullLine.substr(0, currentCount); // To be safe, we save it again.
					break;
				}
			}
		}

		if (foundCompleteTarget)
		{
			m_CurrentMatchedVarIndex = UINT32_MAX;
			m_MatchedSearchList.clear();
			return str;
		}
	}

	return std::string();
}

std::string RS::Console::VarTypeToString(Type type)
{
	// TODO: Make a static array instead.
	switch (type)
	{
	case RS::Console::Type::Int8:		return "Int8";
	case RS::Console::Type::Int16:		return "Int16";
	case RS::Console::Type::Int32:		return "Int32";
	case RS::Console::Type::Int64:		return "Int64";
	case RS::Console::Type::UInt8:		return "UInt8";
	case RS::Console::Type::UInt16:		return "UInt16";
	case RS::Console::Type::UInt32:		return "UInt32";
	case RS::Console::Type::UInt64:		return "UInt64";
	case RS::Console::Type::Float:		return "Float";
	case RS::Console::Type::Double:		return "Double";
	case RS::Console::Type::Function:	return "Function";
	case RS::Console::Type::Unknown:
	default:
		return "Unknown value";
	}
}

std::string RS::Console::VarValueToString(Type type, void* value)
{
	switch (type)
	{
	case RS::Console::Type::Int8:		return std::to_string(*(int8*)value);
	case RS::Console::Type::Int16:		return std::to_string(*(int16*)value);
	case RS::Console::Type::Int32:		return std::to_string(*(int32*)value);
	case RS::Console::Type::Int64:		return std::to_string(*(int64*)value);
	case RS::Console::Type::UInt8:		return std::to_string(*(uint8*)value);
	case RS::Console::Type::UInt16:		return std::to_string(*(uint16*)value);
	case RS::Console::Type::UInt32:		return std::to_string(*(uint32*)value);
	case RS::Console::Type::UInt64:		return std::to_string(*(uint64*)value);
	case RS::Console::Type::Float:		return std::to_string(*(float*)value);
	case RS::Console::Type::Double:		return std::to_string(*(double*)value);
	case RS::Console::Type::Function:	return "void (func*)(void)";
	case RS::Console::Type::Unknown:
	default:
		return "Unknown value";
	}
}

RS::Console::Type RS::Console::StringToVarValue(Type type, const std::string& str, void* refValue, int _internal)
{
	try
	{
		switch (type)
		{
		case RS::Console::Type::Int8:		*(int8*)refValue = (int8)std::stoi(str.c_str()); break;
		case RS::Console::Type::Int16:		*(int16*)refValue = (int16)std::stoi(str.c_str()); break;
		case RS::Console::Type::Int32:		*(int32*)refValue = (int32)std::stoi(str.c_str()); break;
		case RS::Console::Type::Int64:		*(int64*)refValue = (int64)std::stoll(str.c_str()); break;
		case RS::Console::Type::UInt8:		*(uint8*)refValue = (uint8)std::stoul(str.c_str()); break;
		case RS::Console::Type::UInt16:		*(uint16*)refValue = (uint16)std::stoul(str.c_str()); break;
		case RS::Console::Type::UInt32:		*(uint32*)refValue = (uint32)std::stoul(str.c_str()); break;
		case RS::Console::Type::UInt64:		*(uint64*)refValue = (uint64)std::stoull(str.c_str()); break;
		case RS::Console::Type::Float:		*(float*)refValue = (float)std::stof(str.c_str()); break;
		case RS::Console::Type::Double:		*(double*)refValue = (double)std::stod(str.c_str()); break;
		case RS::Console::Type::Function:
		case RS::Console::Type::Unknown:
		default:
			refValue = nullptr;
			break;
		}
	}
	catch (std::invalid_argument const& ex)
	{
		// Try to change from signed to unsigned
		if (IsTypeInt(type))
			return StringToVarValue((Type)((int32)type + (int32)TypeInfo::NumInts), str, refValue);

		if (IsTypeUInt(type))
			return StringToVarValue(Type::Float, str, refValue, 1);
		if (IsTypeFloat(type) && _internal != 1)
			return StringToVarValue(Type::Int8, str, refValue);

		refValue = nullptr;
		return Type::Unknown;
	}
	catch (std::out_of_range const& ex)
	{
		// Promote it to a higher value.
		if ((IsTypeInt(type) && type < Type::Int64) ||
			(IsTypeUInt(type) && type < Type::UInt64) || 
			(IsTypeFloat(type) && type < Type::Double))
		{
			return StringToVarValue((Type)((uint32)type+1), str, refValue);
		}
		// Was not able to promote, cannot identify the type.
		return Type::Unknown;
	}

	return type;
}

RS::Console::Type RS::Console::StringToVarType(const std::string& str)
{
	Type initialType = Type::Int32;
	if (str.find('.') != std::string::npos)
		initialType = Type::Float;
	uint64 temp = 0;
	return StringToVarValue(initialType, str, (void*)&temp);
}

void RS::Console::ComputeCurrentSearchableItem(const char* searchableLine)
{
	std::string buff(searchableLine);
	m_CurrentSearchableItem = searchableLine;
	size_t pos = m_CurrentSearchableItem.rfind(' ');
	if (pos == std::string::npos)
	{
		m_CurrentSearchableItemStartPos = 0;
		m_DisplaySearchResultsForEmptySearchWord = false;
	}
	else
	{
		m_CurrentSearchableItemStartPos = pos + 1;
	}

	m_CurrentSearchableItem = m_CurrentSearchableItem.substr(m_CurrentSearchableItemStartPos, m_CurrentSearchableItem.size() - m_CurrentSearchableItemStartPos);

	if (pos > 1)
	{
		size_t pos2 = m_CurrentSearchableItem.rfind(' ', pos == std::string::npos ? pos : pos - 2);
		if (pos2 == std::string::npos) pos2 = 0;
		SetValidSearchableArgTypesFromCMD(buff.substr(pos2, pos - pos2));
	}
	else
		m_ValidSearchableArgTypes.clear();
}

void RS::Console::SetValidSearchableArgTypesFromCMD(const std::string& cmd)
{
	auto it = m_VariablesMap.find(cmd);
	if (it != m_VariablesMap.end())
	{
		m_ValidSearchableArgTypes = it->second.searchableTypes;
		m_DisplaySearchResultsForEmptySearchWord = m_ValidSearchableArgTypes.empty() == false;
	}
}

void RS::Console::Search(const std::string& item, const std::vector<std::string>& searchStrings, std::vector<MatchedSearchItem>& refIntermediateMatchedList)
{
	MatchedSearchItem matchedLine;
	std::optional<int32> combinedMatchedCount;
	std::optional<int32> combinedStartIndex;
	std::vector<int32> matchedParts;
	int lastLinePartMatchIndex = -1;
	for (int32 swIndex = 0; swIndex < searchStrings.size(); ++swIndex)
	{
		const std::string& searchWord = searchStrings[swIndex];

		// the searchWord need to mach exact to the line part if it is not the last search word in the list.
		std::string currentSearchString = Utils::ToLower(std::string(searchWord));
		std::string searchableLine = Utils::ToLower(item);

		if (searchStrings.size() > 1) // multiple search words
		{
			std::vector<std::string> searchableLinePartList = Utils::Split(searchableLine, '.');

			if (swIndex != searchStrings.size() - 1)
			{
				// Find first part match.
				if (lastLinePartMatchIndex == -1)
				{
					auto it = std::find(searchableLinePartList.begin(), searchableLinePartList.end(), currentSearchString);
					if (it != searchableLinePartList.end())
					{
						lastLinePartMatchIndex = it - searchableLinePartList.begin();

						combinedMatchedCount = (int32)searchableLinePartList[lastLinePartMatchIndex].length();
						int32 startPos = 0;
						for (int32 linePartIndex = 0; linePartIndex < lastLinePartMatchIndex; ++linePartIndex)
							startPos += (linePartIndex == 0 ? 0 : 1) + searchableLinePartList[linePartIndex].length();
						combinedStartIndex = startPos;
					}
				}
				// All other parts need to match (in order) except the last search word.
				else
				{
					if (lastLinePartMatchIndex + 1 < searchableLinePartList.size())
					{
						int32 pos = combinedStartIndex.value();
						if (searchWord == searchableLinePartList[lastLinePartMatchIndex + 1])
						{
							lastLinePartMatchIndex++;
							int32 count = (int32)searchableLinePartList[lastLinePartMatchIndex].length();
							combinedMatchedCount = combinedMatchedCount.value() + count + 1;
						}
						else
						{
							// Did not match consecutive parts, abort search.
							return;
						}
					}
				}

				if (lastLinePartMatchIndex == -1)
				{
					// No first full match, abort search.
					return;
				}
			}
			else // Last part
			{
				// Add the unmatched parts.
				if (combinedStartIndex.value() > 0)
				{
					Line line;
					line.str = item.substr(0, combinedStartIndex.value());
					line.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					matchedLine.parts.push_back(line);
				}

				Line line;
				line.str = item.substr(combinedStartIndex.value(), combinedMatchedCount.value() + 1);
				line.color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
				matchedLine.parts.push_back(line);

				int baseIndex = combinedStartIndex.value() + combinedMatchedCount.value() + 1;
				int matchedCount = -1;

				if (lastLinePartMatchIndex + 1 < searchableLinePartList.size() && !searchWord.empty())
				{
					bool found = false;
					lastLinePartMatchIndex++;
					int startIndex = -1;
					if (SearchSinglePart(currentSearchString, searchableLinePartList[lastLinePartMatchIndex], matchedCount, startIndex))
					{
						if (startIndex == 0 && matchedCount == currentSearchString.size())
						{
							Line line;
							line.str = item.substr(baseIndex, matchedCount + 1);
							line.color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
							matchedLine.parts.push_back(line);

							baseIndex += matchedCount + 1;

							combinedMatchedCount = combinedMatchedCount.value() + matchedCount;
							found = true;
						}
					}

					if (!found)
					{
						// Did not find a match in the last line part, abort search.
						return;
					}
				}

				// Add last unmatched part
				if (matchedCount != searchableLinePartList[lastLinePartMatchIndex].length())
				{
					Line line;
					line.str = item.substr(baseIndex);
					line.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					matchedLine.parts.push_back(line);
				}
			}
		}
		else // Single search word
		{
			int preIndex = 0;
			int matchedCount = -1;
			int startIndex = -1;
			if (SearchSinglePart(currentSearchString, searchableLine, matchedCount, startIndex))
			{
				if (matchedCount != currentSearchString.size())
					return;

				if (combinedMatchedCount.has_value())
				{
					combinedMatchedCount = combinedMatchedCount.value() + matchedCount;
					combinedStartIndex = std::min(combinedStartIndex.value(), startIndex);
				}
				else
				{
					combinedMatchedCount = matchedCount;
					combinedStartIndex = startIndex;
				}

				// Add unmatched part
				if (startIndex != 0 && startIndex != preIndex)
				{
					Line line;
					line.str = item.substr(preIndex, startIndex);
					line.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					matchedLine.parts.push_back(line);
				}

				// Add matched part
				Line line;
				line.str = item.substr(startIndex, matchedCount);
				line.color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
				matchedLine.parts.push_back(line);

				preIndex = startIndex + matchedCount;
			}

			// Add last unmatched part
			if (preIndex != searchableLine.length())
			{
				Line line;
				line.str = item.substr(preIndex);
				line.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				matchedLine.parts.push_back(line);
			}
		}
	}

	// Add all items when no search string is present.
	if (searchStrings.empty() && m_DisplaySearchResultsForEmptySearchWord)
	{
		Line line;
		line.str = item;
		line.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		matchedLine.parts.push_back(line);
		combinedMatchedCount = 0;
		combinedStartIndex = 0;
	}

	if (combinedMatchedCount.has_value() && (combinedMatchedCount.value() > 0 || m_DisplaySearchResultsForEmptySearchWord))
	{
		matchedLine.fullLine = item;
		matchedLine.matchedCount = combinedMatchedCount.value();
		matchedLine.startIndex = combinedStartIndex.value();
		refIntermediateMatchedList.push_back(matchedLine);
	}
}

bool RS::Console::SearchSinglePart(const std::string& searchWord, const std::string& searchableLine, int& outMatchedCount, int& outStartIndex)
{
	outMatchedCount = -1;
	outStartIndex = -1;
	int currentMatchedCount = 0;
	int currentStartIndex = 0;

	int swIndex = 0;
	for (int i = 0; i < searchableLine.length(); ++i)
	{
		char c = searchableLine[i];
		if (swIndex < searchWord.size() && c == searchWord[swIndex])
		{
			if (swIndex == 0)
			{
				currentStartIndex = i;
				currentMatchedCount = 0;
			}
			currentMatchedCount++;
			swIndex++;
		}
		else
		{
			swIndex = 0;

			if (currentMatchedCount > outMatchedCount)
			{
				outMatchedCount = currentMatchedCount;
				outStartIndex = currentStartIndex;
			}
		}
	}

	if (currentMatchedCount > outMatchedCount)
	{
		outMatchedCount = currentMatchedCount;
		outStartIndex = currentStartIndex;
	}

	if (outMatchedCount == -1)
	{
		outMatchedCount = currentMatchedCount;
		outStartIndex = currentStartIndex;
	}

	return outMatchedCount > 0;
}
