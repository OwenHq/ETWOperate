
#include "stdafx.h"
#include "ETWController.h"
#include <ShlObj.h>
#include <strsafe.h>
#include <evntcons.h>


BOOL ETWController::ms_bFinishTrace = FALSE;

ETWController::ETWController()
: m_pKernelEventTraceProperties(NULL)
, m_hKernelTraceHandle(NULL)
, m_pCustomEventTraceProperties(NULL)
, m_hCustomTraceHandle(NULL)
{
	SYSTEMTIME sysTime;
	::GetLocalTime( &sysTime );
	wchar_t wszSubDirPath[MAX_PATH] = {0};
	wsprintf(wszSubDirPath, L"%04d-%02d-%02d-%02d-%02d-%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	m_wstrLogDirPath = LOG_DIRPATH;
	m_wstrLogDirPath = m_wstrLogDirPath + wszSubDirPath;
	::SHCreateDirectoryEx(NULL, m_wstrLogDirPath.c_str(), NULL);
}

ETWController::~ETWController()
{

}


ULONG ETWController::ETWC_StartTrace()
{
	ULONG status = ERROR_SUCCESS;
	TRACEHANDLE hTraceHandle = 0;
	EVENT_TRACE_PROPERTIES *pEventTraceProperties = NULL;
	ULONG BufferSize = 0;

	std::wstring wstrSystemLogFilePath = m_wstrLogDirPath + L"\\" + SYSTEM_LOGFILENAME;
	std::wstring wstrCustomLogFilePath = m_wstrLogDirPath + L"\\" + CUSTOM_LOGFILENAME;

	//跟踪系统ETW日志

	// Allocate memory for the session properties. The memory must
	// be large enough to include the log file name and session name,
	// which get appended to the end of the session properties structure.
	BufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAME) + (1 + wstrSystemLogFilePath.length()) * sizeof(wchar_t) ;
	pEventTraceProperties = (EVENT_TRACE_PROPERTIES *)malloc(BufferSize);
	if(NULL == pEventTraceProperties)
	{
		wprintf(L"Unalbe to allocate %d bytes for properties structure \r\n", BufferSize);
		ETWC_StopTrace();
		return FALSE;
	}

	// Set the session properties. You only append the log file name
	// to the properties structure; the StartTrace function appends
	// the session name for you.
	::ZeroMemory(pEventTraceProperties, BufferSize);
	pEventTraceProperties->Wnode.BufferSize = BufferSize;
	pEventTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	pEventTraceProperties->Wnode.ClientContext = 1;	//QPC clock resolusion
	pEventTraceProperties->Wnode.Guid = SystemTraceControlGuid;
	pEventTraceProperties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
	pEventTraceProperties->MaximumFileSize = 128;
	pEventTraceProperties->FlushTimer = 1;
	pEventTraceProperties->EnableFlags = EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_THREAD | EVENT_TRACE_FLAG_IMAGE_LOAD | EVENT_TRACE_FLAG_DISK_IO | EVENT_TRACE_FLAG_DISK_FILE_IO
		| EVENT_TRACE_FLAG_SYSTEMCALL | EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS | EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS | EVENT_TRACE_FLAG_PROCESS_COUNTERS | EVENT_TRACE_FLAG_CSWITCH 
		| EVENT_TRACE_FLAG_DPC| EVENT_TRACE_FLAG_PROFILE | EVENT_TRACE_FLAG_FILE_IO | EVENT_TRACE_FLAG_FILE_IO_INIT
		| EVENT_TRACE_FLAG_FORWARD_WMI | EVENT_TRACE_FLAG_SPLIT_IO | EVENT_TRACE_FLAG_DISK_IO_INIT | EVENT_TRACE_FLAG_FILE_IO_INIT;
	pEventTraceProperties->MinimumBuffers = 128;
	pEventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	pEventTraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAME);
	StringCbCopy((LPWSTR)((char*)pEventTraceProperties + pEventTraceProperties->LogFileNameOffset), (wstrSystemLogFilePath.length()+1) * sizeof(wchar_t), wstrSystemLogFilePath.c_str());
	m_pKernelEventTraceProperties = pEventTraceProperties;

	// Create the trace session.
	status = StartTrace(&hTraceHandle, KERNEL_LOGGER_NAME, pEventTraceProperties);
	m_hKernelTraceHandle = hTraceHandle;
	if(ERROR_SUCCESS != status)
	{
		wprintf(L"Start Trace failed with %lu \r\n", status);
		ETWC_StopTrace();
		return FALSE;
	}

	//The TraceSetInformation function enables or disables event tracing session settings for the specified information class.
	//If the InformationClass parameter is set to TraceStackTracingInfo, calling this function enables stack tracing of the specified kernel events. 
	//Subsequent calls to this function overwrites the previous list of kernel events for which stack tracing is enabled. To disable stack tracing, call this function with InformationClass set to TraceStackTracingInfo and InformationLength set to 0.
	CLASSIC_EVENT_ID eventId[10] = {0};
	eventId[0].EventGuid = PerfInfoGuid;
	eventId[0].Type = 46;
	status = TraceSetInformation(m_hKernelTraceHandle, TraceStackTracingInfo, eventId, sizeof(eventId));
	if(ERROR_SUCCESS != status)
	{
		wprintf(L"TraceSetInformation failed with %lu \r\n", status);
	}

	//跟踪自定义ETW日志

	BufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(CUSTOM_SESSION_NAME) + (1 + wstrCustomLogFilePath.length()) * sizeof(wchar_t) ;
	pEventTraceProperties = (EVENT_TRACE_PROPERTIES *)malloc(BufferSize);
	ZeroMemory(pEventTraceProperties, BufferSize);
	pEventTraceProperties->Wnode.BufferSize = BufferSize;
	pEventTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	pEventTraceProperties->Wnode.ClientContext = 1;	//QPC clock resolusion
	pEventTraceProperties->Wnode.Guid = SesionGuid;
	pEventTraceProperties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL | EVENT_TRACE_REAL_TIME_MODE;
	pEventTraceProperties->MaximumFileSize = 128;
	pEventTraceProperties->FlushTimer = 1;
	pEventTraceProperties->EnableFlags = 0;
	pEventTraceProperties->MinimumBuffers = 128;
	pEventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	pEventTraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(CUSTOM_SESSION_NAME);
	StringCbCopy((LPWSTR)((char*)pEventTraceProperties + pEventTraceProperties->LogFileNameOffset), (1+wstrCustomLogFilePath.length()) * sizeof(wchar_t) , wstrCustomLogFilePath.c_str());
	m_pCustomEventTraceProperties = pEventTraceProperties;

	// Create the trace session.
	status = StartTrace(&hTraceHandle, CUSTOM_SESSION_NAME, pEventTraceProperties);
	m_hCustomTraceHandle = hTraceHandle;
	status = EnableTrace(
		TRUE,
		0,
		TRACE_LEVEL_INFORMATION,
		(LPGUID)&(ProviderGuid),
		hTraceHandle);

	if(ERROR_SUCCESS != status)
	{
		wprintf(L"Enable Provider Failed with %lu \r\n", status);
		ETWC_StopTrace();
		return FALSE;
	}

	//Run your process

	ETWC_ParseTrace();

	return TRUE;
}

ULONG ETWController::ETWC_StopTrace()
{
	if(m_hKernelTraceHandle)
	{
		ULONG status = ::ControlTrace(m_hKernelTraceHandle, NULL, m_pKernelEventTraceProperties, EVENT_TRACE_CONTROL_STOP);
		if(ERROR_SUCCESS != status )
		{
			wprintf(L"Stop Failed with %lu \n", status);
		}
	}

	if(m_hCustomTraceHandle)
	{
		ULONG status = ::ControlTrace(m_hCustomTraceHandle, NULL, m_pCustomEventTraceProperties, EVENT_TRACE_CONTROL_STOP);
		if(ERROR_SUCCESS != status)
		{
			wprintf(L"Stop Private Session Failed with %lu \r\n", status);
		}
	}

	if(m_pKernelEventTraceProperties)
	{
		free(m_pKernelEventTraceProperties);
		m_pKernelEventTraceProperties = NULL;
	}

	if(m_pCustomEventTraceProperties)
	{
		free(m_pCustomEventTraceProperties);
		m_pCustomEventTraceProperties = NULL;
	}

	return 0;
}


ULONG ETWController::ETWC_ParseTrace()
{
	// Identify the log file from which you want to consume events
	// and the callbacks used to process the events and buffers.
	EVENT_TRACE_LOGFILE logFile;
	ZeroMemory(&logFile, sizeof(EVENT_TRACE_LOGFILE));
	logFile.LoggerName        = (LPWSTR)CUSTOM_SESSION_NAME;
	logFile.BufferCallback    = (PEVENT_TRACE_BUFFER_CALLBACK)CustomBufferCallback;
	logFile.EventCallback     = (PEVENT_CALLBACK)CustomEventCallback;
	logFile.ProcessTraceMode  = PROCESS_TRACE_MODE_REAL_TIME;

	while(!ms_bFinishTrace)
	{
		ULONG ulStatus = ERROR_SUCCESS;
		TRACEHANDLE hTrace = OpenTrace(&logFile);

		if ((ULONGLONG)-1 != hTrace && (ULONGLONG)0x0FFFFFFFF != hTrace) // 不能用INVALID_PROCESSTRACE_HANDLE比较
		{
			while ((ulStatus = ProcessTrace(&hTrace, 1, 0, 0)) == ERROR_SUCCESS)
			{
				if(ms_bFinishTrace)
					break;
			}

			CloseTrace(hTrace);
		}
		Sleep(200);
	}
	ETWC_StopTrace();
	ETWC_MergeTraceFile();

	return 0;
}

ULONG ETWController::ETWC_MergeTraceFile()
{
	std::wstring wstrSystemLogFilePath = m_wstrLogDirPath + L"\\" + SYSTEM_LOGFILENAME;
	std::wstring wstrCustomLogFilePath = m_wstrLogDirPath + L"\\" + CUSTOM_LOGFILENAME;
	std::wstring wstrMergeLogFilePath = m_wstrLogDirPath + L"\\" + MERGE_LOGFILENAME;

	std::string strMergeCommand;
	char szCustomLogFilePath[MAX_PATH] = {0};
	char szSystemLogFilePath[MAX_PATH] = {0};
	char szMergeLogFilePath[MAX_PATH] = {0};
	WideCharToMultiByte(CP_ACP,0, wstrSystemLogFilePath.c_str(), -1, szSystemLogFilePath, MAX_PATH, NULL, FALSE);
	WideCharToMultiByte(CP_ACP,0, wstrCustomLogFilePath.c_str(), -1, szCustomLogFilePath, MAX_PATH, NULL, FALSE);
	WideCharToMultiByte(CP_ACP,0, wstrMergeLogFilePath.c_str(), -1, szMergeLogFilePath, MAX_PATH, NULL, FALSE);

	strMergeCommand = "xperf -merge ";
	strMergeCommand = strMergeCommand + szSystemLogFilePath + " " + szCustomLogFilePath + " " + szMergeLogFilePath;
	system(strMergeCommand.c_str());

	return 0;
}

void WINAPI ETWController::CustomEventCallback(PEVENT_TRACE pEvent)
{
	if(IsEqualGUID(pEvent->Header.Guid, CategoryGuid))
	{
		/*if( THUNDER_PROVIDER_TYPE_END == pEvent->Header.Class.Type )
		{
			sm_IsFinishTrace = TRUE;
		}*/
	}
}

ULONG WINAPI ETWController::CustomBufferCallback(PEVENT_TRACE_LOGFILE pBuffer)
{
	//To continue processing events, return TRUE. Otherwise, return FALSE. 
	//Returning FALSE will terminate the ProcessTrace function.
	return !ms_bFinishTrace;
}