// ETWOperate.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ETWController.h"
#include "ETWConsumer.h"


int _tmain(int argc, _TCHAR* argv[])
{
	ETWController etwController;
	etwController.ETWC_StartTrace();

	/*ETWConsumer etwConsumer;
	etwConsumer.ParseTraceFile(L"C:\\ETWOperate\\2014-05-18-11-23-51\\SystemLogFile.etl", 0);*/

	system("pause");

	return 0;
}

