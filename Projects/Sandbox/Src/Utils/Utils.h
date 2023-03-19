#pragma once

namespace RS
{
	class Utils
	{
	public:
		RS_STATIC_CLASS(Utils);

		static std::string ToLower(const std::string& s)
		{
			std::string res = s;
			std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
			return res;
		}

		static std::wstring ToWString(const std::string& s)
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

		static std::string ToString(const std::wstring& s)
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
		static constexpr bool ContainsLastStr(std::string_view view, std::string_view ending)
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
	};
}