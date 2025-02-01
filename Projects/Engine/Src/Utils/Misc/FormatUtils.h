#pragma once

#include <format>

namespace RS::Utils
{
	// Format strings
	template<typename... Args>
	inline std::string Format(const char* formatStr, Args&&...args)
	{
		return std::vformat(formatStr, std::make_format_args(args...));
	}

	inline std::string Format(const char* msg)
	{
		return Format("{}", msg);
	}
}