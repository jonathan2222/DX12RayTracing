#pragma once

namespace RS::Utils
{
	inline std::vector<std::string> Split(const std::string& str, char c)
	{
		std::vector<std::string> v;
		if (str.empty())
			return v;

		size_t prePos = 0;
		size_t pos = str.find(c);
		while (pos != std::string::npos)
		{
			size_t count = pos - prePos;
			if (count != 0)
				v.push_back(str.substr(prePos, count));
			prePos = pos + 1;
			pos = str.find(c, prePos);
		}

		if (prePos < str.length())
			v.push_back(str.substr(prePos));

		return v;
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

	// Compile time function to check if a string has a certain ending.
	inline constexpr bool ContainsLastStr(std::string_view view, std::string_view ending)
	{
		if (view.length() >= ending.length()) {
			return (0 == view.compare(view.length() - ending.length(), ending.length(), ending));
		}
		else {
			return false;
		}
	}

	template<typename... Args>
	static std::string Format(fmt::format_string<Args...> fmt, Args &&...args)
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