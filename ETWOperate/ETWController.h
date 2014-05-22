
#pragma once
#include <string>
#include "ETWCommonDefine.h"

#define LOG_DIRPATH	L"c:\\ETWOperate\\"
#define CUSTOM_LOGFILENAME	L"CustomLogFile.etl"
#define SYSTEM_LOGFILENAME	L"SystemLogFile.etl"
#define MERGE_LOGFILENAME	L"MergeLogFile.etl"

// GUID that identifies your trace session.
// Remember to create your own session GUID.
// {AD1EE5C2-9D8C-43bc-A655-0C6DE26B772B}
static const GUID SesionGuid = 
{ 0xad1ee5c2, 0x9d8c, 0x43bc, { 0xa6, 0x55, 0xc, 0x6d, 0xe2, 0x6b, 0x77, 0x2b } };

class ETWController
{
public:
	ETWController();
	~ETWController();

public:
	//开始跟踪
	ULONG ETWC_StartTrace();
	//停止跟踪
	ULONG ETWC_StopTrace();

private:
	//实时解释日志
	ULONG ETWC_ParseTrace();
	//合并日志
	ULONG ETWC_MergeTraceFile();

	//Pointer to the EventCallback function that ETW calls for each event in the buffer.
	//Specify this callback if you are consuming events from a provider that used one of the TraceEvent functions to log events.
	static void WINAPI CustomEventCallback(PEVENT_TRACE pEvent);
	//Pointer to the BufferCallback function that receives buffer-related statistics for each buffer ETW flushes. 
	//ETW calls this callback after it delivers all the events in the buffer. This callback is optional.
	static ULONG WINAPI CustomBufferCallback(PEVENT_TRACE_LOGFILE pBuffer);

private:
	std::wstring	m_wstrLogDirPath;

	//Session相关
	EVENT_TRACE_PROPERTIES *m_pKernelEventTraceProperties;
	TRACEHANDLE m_hKernelTraceHandle;
	EVENT_TRACE_PROPERTIES *m_pCustomEventTraceProperties;
	TRACEHANDLE m_hCustomTraceHandle;

	//标识获取结束
	static BOOL ms_bFinishTrace;
	static HANDLE ms_hFinishTraceEvent;
};