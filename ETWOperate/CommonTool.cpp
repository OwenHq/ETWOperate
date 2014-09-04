
#include "stdafx.h"
#include "CommonTool.h"


std::wstring CommonTool::StringToWstring(const std::string& str)
{
	TSDEBUG(L"Enter");

	//��ȡ��������С��������ռ䣬��������С���ַ�����   
	int iLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);   
	wchar_t* wszBuffer = new wchar_t[iLen + 1];   

	//���ֽڱ���ת���ɿ��ֽڱ���   
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), wszBuffer, iLen);   
	wszBuffer[iLen] = '\0';             //����ַ�����β   

	std::wstring wstr = wszBuffer;
	TSDEBUG(L"wstr = %s", wstr.c_str());

	delete[] wszBuffer;

	return wstr;
}

std::string CommonTool::WstringToString(const std::wstring& wstr)
{
	TSDEBUG(L"Enter, wstr = %s", wstr.c_str());

	//��ȡ��������С��������ռ䣬��������С�°��ֽڼ����   
	int iLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);   
	char* szBuffer = new char[iLen + 1];  

	//���ֽڱ���ת���ɶ��ֽڱ���   
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), szBuffer, iLen, NULL, NULL);   
	szBuffer[iLen] = '\0'; 

	std::string str = szBuffer;

	delete[] szBuffer;  

	return str; 
}

std::wstring CommonTool::GetMainNameFromPath(const std::wstring& wstrPath)
{
	TSDEBUG(L"Enter, wstrPath = %s", wstrPath.c_str());

	size_t pos;
	size_t pos1 = wstrPath.find_last_of(L"\\");
	size_t pos2 = wstrPath.find_last_of(L"/");
	pos = std::wstring::npos;	
	if (std::wstring::npos != pos1 && std::wstring::npos != pos2)
	{
		pos = pos1;
		if (pos1 < pos2)
		{
			pos = pos2;
		}
	}
	else if (std::wstring::npos != pos1 && std::wstring::npos == pos2)
	{
		pos = pos1;
	}
	else if (std::wstring::npos != pos2 && std::wstring::npos == pos1)
	{
		pos = pos2;
	}

	//ȫ��
	std::wstring wstrName = wstrPath;
	if (std::wstring::npos != pos)
	{
		wstrName = wstrName.substr(pos + 1);
	}

	//ȥ��׺
	std::wstring wstrMainName = wstrName;
	size_t PointPos = wstrMainName.find_last_of(L".");
	if (std::wstring::npos != PointPos)
	{
		wstrMainName = wstrMainName.substr(0, PointPos);
	}
	
	return wstrMainName;
}