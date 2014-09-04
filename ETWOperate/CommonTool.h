
#pragma once
#include <string>

class CommonTool
{
public:
	//string转换为wstring
	static std::wstring StringToWstring(const std::string& str);
	//wstring转换为string
	static std::string WstringToString(const std::wstring& wstr);

	//从路径中获得文件名（不包括后缀）
	static std::wstring GetMainNameFromPath(const std::wstring& wstrPath);
};

