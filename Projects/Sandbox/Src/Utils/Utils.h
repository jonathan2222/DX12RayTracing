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

		// Bit manipulations

		template<typename T>
		static T AlignUp(T x, T alignment)
		{
			// Align 1010 1101b with 4 (100b)
			// mask 4 or greater => 4-1 = 11b
			// invert mask => 1111 1100b
			// If and with x we get the aligned value rounded down.
			// 1010 1101
			// 1111 1100 &
			// 1010 1100 (lost 1 bit)
			// Adding one less of the alignment (3=11b) we get the aligned value rounded up.
			// Added 3:
			// 1010 1101
			// 0000 0011 +
			// 1011 0000
			// New aligned value:
			// 1011 0000
			// 1111 1100 &
			// 1011 0000
			return (x + (alignment - 1)) & ~(alignment - 1);
		}

		template<typename T>
		static T AlignDown(T x, T alignment)
		{
			return x & ~(alignment - 1);
		}

		// Check if power of 2 and treat 0 as false. (with other words, check if only one bit is set)
		template<typename T>
		static bool IsPowerOfTwo(T x)
		{
			return x && !(x & (x - 1));
		}

		template<typename T>
		static T IndexOfPow2Bit(T x)
		{
			if (!IsPowerOfTwo<T>(x))
				return (T)-1;

			T i = 1, pos = 0;
			while (!(i & x))
			{
				i = i << 1;
				++pos;
			}
			return pos;
		}
	};
}