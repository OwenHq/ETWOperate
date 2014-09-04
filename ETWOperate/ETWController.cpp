
#include "stdafx.h"
#include "ETWController.h"
#include <ShlObj.h>
#include <strsafe.h>
#include <evntcons.h>
//Test
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
//Test
#include "ETWConsumer.h"


#define		REGEDIT_THUNDER_SUB_KEY						L"SOFTWARE\\Thunder Network\\ThunderOem\\thunder_backwnd"
#define		REGEDIT_THUNDER_KEY_VALUE_PATH				L"Path"
#define		REGEDIT_THUNDER_KEY_VALUE_VERSION			L"Version"
#define		REGEDIT_THUNDER_KEY_VALUE_INTASLLDIR		L"instdir"

BOOL GetThunderInstallDir(std::wstring& wszInstallDir)
{
	const size_t cdwBufferSize = 512;
	wchar_t szBuffer[cdwBufferSize] = {0};
	DWORD dwBufferSize = cdwBufferSize;
	DWORD dwType = (DWORD)-1;

	if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGEDIT_THUNDER_SUB_KEY, REGEDIT_THUNDER_KEY_VALUE_INTASLLDIR, &dwType, szBuffer, &dwBufferSize))
	{
		if (wcslen(szBuffer) > 0)
		{
			wszInstallDir = szBuffer;
			if (wszInstallDir[wszInstallDir.length()-1] != L'\\')
			{
				wszInstallDir += L"\\program\\thunder.exe";
			}
			return TRUE;
		}

		return FALSE;
	}

	return FALSE;
}


BOOL ETWController::ms_bFinishTrace = FALSE;
HANDLE ETWController::ms_hFinishTraceEvent = NULL;

ETWController::ETWController()
: m_pKernelEventTraceProperties(NULL)
, m_hKernelTraceHandle(NULL)
, m_pCustomEventTraceProperties(NULL)
, m_hCustomTraceHandle(NULL)
{
	ms_hFinishTraceEvent = ::CreateEvent(NULL, TRUE, FALSE, L"ETWController_FinishTraceEvent");

	SYSTEMTIME sysTime;
	::GetLocalTime( &sysTime );
	wchar_t wszSubDirPath[MAX_PATH] = {0};
	wsprintf(wszSubDirPath, L"%04d-%02d-%02d-%02d-%02d-%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	//结果存放目录
	m_wstrLogDirPath = LOG_DIRPATH;
	m_wstrLogDirPath = m_wstrLogDirPath + wszSubDirPath;
	::SHCreateDirectoryEx(NULL, m_wstrLogDirPath.c_str(), NULL);
	//几个文件名
	m_wstrCustomEtlFileName = L"";
	m_wstrCustomEtlFileName += wszSubDirPath;
	m_wstrCustomEtlFileName += CUSTOM_LOGFILENAME;
	m_wstrSystemEtlFileName = L"";
	m_wstrSystemEtlFileName += wszSubDirPath;
	m_wstrSystemEtlFileName += SYSTEM_LOGFILENAME;
	m_wstrMergeEtlFileName = L"";
	m_wstrMergeEtlFileName += wszSubDirPath;
	m_wstrMergeEtlFileName += MERGE_LOGFILENAME;
}

ETWController::~ETWController()
{

}


ULONG ETWController::ETWC_StartTrace(const wchar_t* wszResultFilePath)
{
	wprintf(L"Enter ETWC_StartTrace \r\n");
	ULONG status = ERROR_SUCCESS;
	TRACEHANDLE hTraceHandle = 0;
	EVENT_TRACE_PROPERTIES *pEventTraceProperties = NULL;
	ULONG BufferSize = 0;

	//	提升应用程序权限

	HANDLE token;
	//GetCurrentProcess()函数返回本进程的伪句柄
	if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
	{
		return FALSE;
	}
	LUID luid;

	if(!::LookupPrivilegeValue(NULL, SE_SYSTEM_PROFILE_NAME, &luid))
	{
		return FALSE;
	}

	TOKEN_PRIVILEGES pToken;
	pToken.PrivilegeCount = 1;
	pToken.Privileges[0].Luid = luid;
	pToken.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if(!::AdjustTokenPrivileges(token, FALSE, &pToken, NULL, NULL, NULL))
	{
		return FALSE;
	}

	m_wstrResultFilePath = wszResultFilePath;
	std::wstring wstrSystemLogFilePath = m_wstrLogDirPath + L"\\" + m_wstrSystemEtlFileName;
	std::wstring wstrCustomLogFilePath = m_wstrLogDirPath + L"\\" + m_wstrCustomEtlFileName;

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
	pEventTraceProperties->MaximumFileSize = 150;
	pEventTraceProperties->FlushTimer = 1;
	pEventTraceProperties->EnableFlags = EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_THREAD | EVENT_TRACE_FLAG_IMAGE_LOAD | EVENT_TRACE_FLAG_DISK_IO | EVENT_TRACE_FLAG_DISK_FILE_IO
		| EVENT_TRACE_FLAG_SYSTEMCALL | EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS | EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS | EVENT_TRACE_FLAG_PROCESS_COUNTERS | EVENT_TRACE_FLAG_CSWITCH 
		| EVENT_TRACE_FLAG_DPC| EVENT_TRACE_FLAG_PROFILE | EVENT_TRACE_FLAG_FILE_IO | EVENT_TRACE_FLAG_FILE_IO_INIT
		| EVENT_TRACE_FLAG_FORWARD_WMI | EVENT_TRACE_FLAG_SPLIT_IO | EVENT_TRACE_FLAG_DISK_IO_INIT | EVENT_TRACE_FLAG_FILE_IO_INIT;
	pEventTraceProperties->MinimumBuffers = 150;
	pEventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	pEventTraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAME);
	::StringCbCopy((LPWSTR)((char*)pEventTraceProperties + pEventTraceProperties->LogFileNameOffset), (wstrSystemLogFilePath.length()+1) * sizeof(wchar_t), wstrSystemLogFilePath.c_str());
	m_pKernelEventTraceProperties = pEventTraceProperties;

	// Create the trace session.
	status = ::StartTrace(&hTraceHandle, KERNEL_LOGGER_NAME, pEventTraceProperties);
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
	status = ::TraceSetInformation(m_hKernelTraceHandle, TraceStackTracingInfo, eventId, sizeof(eventId));
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
	pEventTraceProperties->MaximumFileSize = 150;
	pEventTraceProperties->FlushTimer = 1;
	pEventTraceProperties->EnableFlags = 0;
	pEventTraceProperties->MinimumBuffers = 150;
	pEventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	pEventTraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(CUSTOM_SESSION_NAME);
	StringCbCopy((LPWSTR)((char*)pEventTraceProperties + pEventTraceProperties->LogFileNameOffset), (1+wstrCustomLogFilePath.length()) * sizeof(wchar_t) , wstrCustomLogFilePath.c_str());
	m_pCustomEventTraceProperties = pEventTraceProperties;

	// Create the trace session.
	status = ::StartTrace(&hTraceHandle, CUSTOM_SESSION_NAME, pEventTraceProperties);
	m_hCustomTraceHandle = hTraceHandle;
	status = ::EnableTrace(
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
	std::wstring programPath;
	GetThunderInstallDir(programPath);
	programPath = L"\"" + programPath + L"\"";
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = {0};
	BOOL isCreateSuccesss = ::CreateProcessW(NULL ,(LPWSTR)programPath.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if(!isCreateSuccesss)
	{
		ETWC_StopTrace();
		return FALSE;
	}

	ETWC_ParseTrace();

	return TRUE;
}

ULONG ETWController::ETWC_StopTrace()
{
	wprintf(L"Enter ETWC_StopTrace \r\n");
	if(m_hKernelTraceHandle)
	{
		ULONG status = ::ControlTrace(m_hKernelTraceHandle, NULL, m_pKernelEventTraceProperties, EVENT_TRACE_CONTROL_STOP);
		if(ERROR_SUCCESS != status )
		{
			wprintf(L"Stop Failed with %lu \r\n", status);
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
	wprintf(L"Enter ETWC_ParseTrace \r\n");
	// Identify the log file from which you want to consume events
	// and the callbacks used to process the events and buffers.
	EVENT_TRACE_LOGFILE eventTraceLogFile;
	::ZeroMemory(&eventTraceLogFile, sizeof(EVENT_TRACE_LOGFILE));
	eventTraceLogFile.LoggerName        = (LPWSTR)CUSTOM_SESSION_NAME;
	eventTraceLogFile.BufferCallback    = (PEVENT_TRACE_BUFFER_CALLBACK)CustomBufferCallback;
	eventTraceLogFile.EventCallback     = (PEVENT_CALLBACK)CustomEventCallback;
	eventTraceLogFile.ProcessTraceMode  = PROCESS_TRACE_MODE_REAL_TIME;

	ULONG ulRet = ERROR_SUCCESS;
	TRACEHANDLE hParseTraceHandle = ::OpenTrace(&eventTraceLogFile);
	if ((ULONGLONG)-1 != hParseTraceHandle && (ULONGLONG)0x0FFFFFFFF != hParseTraceHandle) // 不能用INVALID_PROCESSTRACE_HANDLE比较
	{
		ulRet = ::ProcessTrace(&hParseTraceHandle, 1, 0, 0);
		if (ulRet != ERROR_SUCCESS)
		{
			wprintf(L"ETWC_ParseTrace: ProcessTrace failed, error code is %u \r\n", ulRet);
			return ulRet;
		}
	}

	DWORD dwRet = ::WaitForSingleObject(ms_hFinishTraceEvent, INFINITE);
	wprintf(L"ETWC_ParseTrace: WaitForSingleObject-dwRet = %u \r\n", dwRet);
	
	ulRet = ::CloseTrace(hParseTraceHandle);
	wprintf(L"ETWC_ParseTrace: CloseTrace-lRet = %u \r\n", ulRet);
	ETWC_StopTrace();
	ETWC_MergeTraceFile();

	//测试
	ETWConsumer etwConsumer;
	std::wstring wstrMergeLogFilePath = m_wstrLogDirPath + L"\\" + m_wstrMergeEtlFileName;
	wchar_t* wszMergeLogFilePath = new wchar_t[wstrMergeLogFilePath.length() + 1];
	wcscpy(wszMergeLogFilePath, wstrMergeLogFilePath.c_str());
	etwConsumer.ParseTraceFile(wszMergeLogFilePath, ETWConsumer::OperationType_RunUpTime, m_wstrResultFilePath.c_str());
	delete [] wszMergeLogFilePath;
	wszMergeLogFilePath = NULL;

	return 0;
}

ULONG ETWController::ETWC_MergeTraceFile()
{
	wprintf(L"Enter ETWC_MergeTraceFile \r\n");
	std::wstring wstrSystemLogFilePath = m_wstrLogDirPath + L"\\" + m_wstrSystemEtlFileName;
	std::wstring wstrCustomLogFilePath = m_wstrLogDirPath + L"\\" + m_wstrCustomEtlFileName;
	std::wstring wstrMergeLogFilePath = m_wstrLogDirPath + L"\\" + m_wstrMergeEtlFileName;

	std::string strMergeCommand;
	char szCustomLogFilePath[MAX_PATH] = {0};
	char szSystemLogFilePath[MAX_PATH] = {0};
	char szMergeLogFilePath[MAX_PATH] = {0};
	::WideCharToMultiByte(CP_ACP,0, wstrSystemLogFilePath.c_str(), -1, szSystemLogFilePath, MAX_PATH, NULL, FALSE);
	::WideCharToMultiByte(CP_ACP,0, wstrCustomLogFilePath.c_str(), -1, szCustomLogFilePath, MAX_PATH, NULL, FALSE);
	::WideCharToMultiByte(CP_ACP,0, wstrMergeLogFilePath.c_str(), -1, szMergeLogFilePath, MAX_PATH, NULL, FALSE);

	strMergeCommand = "xperf -merge ";
	strMergeCommand = strMergeCommand + szSystemLogFilePath + " " + szCustomLogFilePath + " " + szMergeLogFilePath;
	system(strMergeCommand.c_str());

	return 0;
}

void WINAPI ETWController::CustomEventCallback(PEVENT_TRACE pEventTrace)
{
	if(::IsEqualGUID(pEventTrace->Header.Guid, CategoryGuid))
	{
		if( ETWPROVIDER_PROCESS_SHOWVIEW == pEventTrace->Header.Class.Type )
		{
			wprintf(L"CustomEventCallback is called, and pEventTrace->Header.Class.Type = 2 \r\n");
			ms_bFinishTrace = TRUE;

			::SetEvent(ms_hFinishTraceEvent);
		}
	}
}

ULONG WINAPI ETWController::CustomBufferCallback(PEVENT_TRACE_LOGFILE pBuffer)
{
	//To continue processing events, return TRUE. Otherwise, return FALSE. 
	//Returning FALSE will terminate the ProcessTrace function.
	return !ms_bFinishTrace;
}