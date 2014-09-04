// ETWOperate.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ETWController.h"
#include "ETWConsumer.h"

//命令行参数
//参数2：
//Controller: 控制器，拉起迅雷，获得ETW日志，分析得到启动速度
//Consumer: 消耗器，分析ETW日志，得到启动速度、IO等信息
//参数3：
//如果参数2为Controller，则参数3代表存储启动速度的TXT文件路径
//如果参数2为Consumer，则参数3代表ETL文件路径
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc > 2 && 0 == wcsicmp(argv[1], L"Controller"))
	{
		ETWController etwController;
		etwController.ETWC_StartTrace(argv[2]);
	}
	else if (argc > 1 && 0 == wcsicmp(argv[1], L"Consumer"))
	{
		ETWConsumer etwConsumer;
		etwConsumer.ParseTraceFile(argv[2], ETWConsumer::OperationType_AnalyzeEtl, argv[3]);
	}

	//system("pause");

	return 0;
}

