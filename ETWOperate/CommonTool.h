
#pragma once
#include <string>

class CommonTool
{
public:
	//stringת��Ϊwstring
	static std::wstring StringToWstring(const std::string& str);
	//wstringת��Ϊstring
	static std::string WstringToString(const std::wstring& wstr);

	//��·���л���ļ�������������׺��
	static std::wstring GetMainNameFromPath(const std::wstring& wstrPath);
};

