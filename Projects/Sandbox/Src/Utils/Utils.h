#pragma once

namespace RS
{
	class Utils
	{
	public:
		RS_STATIC_CLASS(Utils);

		static std::wstring Str2WStr(const std::string& s)
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
	};
}