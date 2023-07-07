#pragma once

namespace RS::Utils
{
	/*
	* Functions in this file:
	*	Split			- Slipts the string with a given delimiter into a list of strings.
	*	PaddLeft		- Adds padding to the left of the string.
	*	PaddRight		- Adds padding to the right of the string.
	*	FillLeft		- Fills the left of the string such that the total length of the string is the specified value.
	*	FillRight		- Fills the right of the string such that the total length of the string is the specified value.
	*	ReplaceAll		- Replaces all occurrences of one substring to another.
	*	ToLower			- Converts the string into lowercase.
	*	ToUpper			- Converts the string into uppercase.
	*	ToWString		- Converts the string into a wide string.
	*	ToString		- Converts a wide string into a normal string.
	*	EndsWith		- Checks if the string ends with a certain substring.
	*	StartsWith		- Checks if the string starts with a certain substring.
	*	Format			- Formats the string using fmt syntax.
	*/

	/*
	* Split the string with a specified delimiter.
	* Tip: Use a std::string_view will avoid any copy of the original string.
	*/
	template<typename T>
	inline void Split(std::vector<T>& vector, const T& str, char delimiter)
	{
		RS_ASSERT_NO_MSG(vector.empty());
		if (str.empty())
			return;

		size_t prePos = 0;
		size_t pos = str.find(delimiter);
		while (pos != std::string::npos)
		{
			size_t count = pos - prePos;
			if (count != 0)
				vector.push_back(str.substr(prePos, count));
			prePos = pos + 1;
			pos = str.find(delimiter, prePos);
		}

		if (prePos < str.length())
			vector.push_back(str.substr(prePos));
	}

	/*
	* Split the string with a specified delimiter.
	*/
	inline std::vector<std::string> Split(const std::string& str, char delimiter)
	{
		std::vector<std::string> vector;
		Split<std::string>(vector, str, delimiter);
		return vector;
	}

	inline std::string PaddLeft(const std::string& s, char c, uint32 count)
	{
		std::string padding;
		padding.resize(count);
		std::fill(padding.begin(), padding.end(), c);
		return padding + s;
	}

	/*
		s: Test
		c: '-'
		count: 6
		Res: ------Test
	*/
	inline std::string PaddRight(const std::string& s, char c, uint32 count)
	{
		std::string padding;
		padding.resize(count);
		std::fill(padding.begin(), padding.end(), c);
		return s + padding;
	}

	/*
		s: Test
		c: '-'
		count: 6
		Res: --Test
	*/
	inline std::string FillLeft(const std::string& s, char c, uint32 count)
	{
		if (s.size() >= count)
			return s;
		return PaddLeft(s, c, count - s.size());
	}

	inline std::string FillRight(const std::string& s, char c, uint32 count)
	{
		if (s.size() >= count)
			return s;
		return PaddRight(s, c, count - s.size());
	}

	inline std::string ReplaceAll(const std::string& s, const std::string& from, const std::string& to)
	{
		std::string newString = s;
		if (s.empty() || (from.empty() && to.empty()))
			return newString;

		size_t pos = newString.find(from);
		while (pos != std::string::npos)
		{
			newString.replace(pos, from.size(), to);
			pos = newString.find(from, pos);
		}

		return newString;
	}

	inline std::string ToLower(const std::string& s)
	{
		std::string res = s;
		std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
		return res;
	}

	inline std::wstring ToWString(const std::string& s)
	{
		// s2ws code from: https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
		int len;
		int slength = (int)s.length() + 1;
		len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
		wchar_t* pBuf = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, pBuf, len);
		std::wstring res(pBuf);
		delete[] pBuf;
		return res;
	}

	inline std::string ToString(const std::wstring& s)
	{
		// From https://codereview.stackexchange.com/questions/419/converting-between-stdwstring-and-stdstring
		int len;
		int slength = (int)s.length() + 1;
		len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
		char* buf = new char[len];
		WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
		std::string r(buf);
		delete[] buf;
		return r;
	}

	/*
	* Check if a string ends with a certain string.
	* PS: Can be used at compile time too!
	*/
	inline constexpr bool EndsWith(std::string_view string, std::string_view ending)
	{
		const size_t stringLength = string.size();
		const size_t endingLength = ending.size();

		RS_ASSERT_NO_MSG(endingLength > 0);
		RS_ASSERT_NO_MSG(stringLength > 0);
		if (endingLength > stringLength) return false;
		if (endingLength == stringLength) return string == ending;

		// Using string view to skip allocating memory.
		const std::string_view stringView = string;
		const std::string_view stringEnding = stringView.substr(stringLength - endingLength, endingLength);
		
		return stringEnding == ending;
	}

	/*
	* Check if a string starts with a certain string.
	* PS: Can be used at compile time too!
	*/
	inline constexpr bool StartsWith(std::string_view string, std::string_view starting)
	{
		const size_t stringLength = string.size();
		const size_t startingLength = starting.size();

		RS_ASSERT_NO_MSG(startingLength > 0);
		RS_ASSERT_NO_MSG(stringLength > 0);
		if (startingLength > stringLength) return false;
		if (startingLength == stringLength) return string == starting;

		// Using string view to skip allocating memory.
		const std::string_view stringView = string;
		const std::string_view stringStarting = stringView.substr(0, startingLength);

		return stringStarting == starting;
	}

	// Format strings
	template<typename... Args>
	inline std::string Format(fmt::format_string<Args...> fmt, Args &&...args)
	{
		std::string str;
		fmt::format_to(std::back_inserter(str), fmt, std::forward<Args>(args)...);
		return str;
	}

	inline std::string Format(const char* msg)
	{
		return Format("{}", msg);
	}
}