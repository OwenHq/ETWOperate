// ETWOperate.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "ETWController.h"
#include "ETWConsumer.h"

//�����в���
//����2��
//Controller: ������������Ѹ�ף����ETW��־�������õ������ٶ�
//Consumer: ������������ETW��־���õ������ٶȡ�IO����Ϣ
//����3��
//�������2ΪController�������3����洢�����ٶȵ�TXT�ļ�·��
//�������2ΪConsumer�������3����ETL�ļ�·��
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

