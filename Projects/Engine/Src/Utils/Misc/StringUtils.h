#pragma once

#include <format>
#include <filesystem>

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

	/*
	* Example: 
	* size_t pos;
	* std::string playerName = FindTag("Player <Loner>: How are you doing?", "<", ">", pos);
	* 
	* This will set playerName to "Loner" and pos to 7
	* If not found pos will be std::string::npos
	*/
	inline std::string FindTag(const std::string& s, const std::string& start, const std::string& end, size_t& foundAt, size_t offset = 0)
	{
		std::string result;
		foundAt = s.find_first_of(start, offset);
		if (foundAt == std::string::npos) return result;
		size_t startP = foundAt + start.size();
		size_t endP = s.find_first_of(start, startP);
		if (endP == std::string::npos)
		{
			foundAt = std::string::npos;
			return result;
		}

		return s.substr(startP, endP - startP);
	}

	inline std::string ToLower(const std::string& s)
	{
		std::string res = s;
		std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
		return res;
	}

	inline std::string ToUpper(const std::string& s)
	{
		std::string res = s;
		std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::toupper(c); });
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

		if (endingLength > stringLength) return false;
		if (endingLength == stringLength) return string == ending;
		if (endingLength == 0 || stringLength == 0) return false;

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

		if (startingLength > stringLength) return false;
		if (startingLength == stringLength) return string == starting;
		if (startingLength == 0 || stringLength == 0) return false;

		// Using string view to skip allocating memory.
		const std::string_view stringView = string;
		const std::string_view stringStarting = stringView.substr(0, startingLength);

		return stringStarting == starting;
	}

	// Trim from start (in place)
	inline void TrimLeft(std::string& s) {
		// From: https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring?page=1&tab=scoredesc#tab-top
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
	}

	// Trim from end (in place)
	inline void TrimRight(std::string& s) {
		// From: https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring?page=1&tab=scoredesc#tab-top
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
	}

	inline void Trim(std::string& str)
	{
		TrimLeft(str);
		TrimRight(str);
	}

	inline std::string TrimLeftC(const std::string& str)
	{
		std::string result = str;
		TrimLeft(result);
		return result;
	}

	inline std::string TrimRightC(const std::string& str)
	{
		std::string result = str;
		TrimRight(result);
		return result;
	}

	inline std::string TrimC(const std::string& str)
	{
		std::string result = str;
		TrimLeft(result);
		TrimRight(result);
		return result;
	}

	// Format strings
	//template<typename... Args>
	//inline std::string Format(const char* formatStr, Args&&...args)
	//{
	//	return std::vformat(formatStr, std::make_format_args(std::forward<Args>(args)...));
	//}

	template<typename... Args>
	inline std::string Format(const char* formatStr, Args&&...args)
	{
		return std::vformat(formatStr, std::make_format_args(args...));
	}

	inline std::string Format(const char* msg)
	{
		return Format("{}", msg);
	}

	inline std::string GetNameFromPath(const std::string& path)
	{
		std::filesystem::path fsPath(path);
		return fsPath.stem().string();
	}

	inline std::string GetExtension(const std::string& path)
	{
		std::filesystem::path fsPath(path);
		return fsPath.extension().string();
	}
}