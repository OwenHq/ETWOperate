
#include "stdafx.h"
#include "ETWConsumer.h"


DWORD ETWConsumer::ms_dwProcessId = 0;
BOOL ETWConsumer::ms_bStopTrace = FALSE;
std::vector<DWORD> ETWConsumer::ms_vThreadId;
IWbemServices* ETWConsumer::g_pServices = NULL;
USHORT ETWConsumer::ms_usPointerSize = 0;

ETWConsumer::ETWConsumer()
{

}

ETWConsumer::~ETWConsumer()
{

}


ULONG ETWConsumer::ParseTraceFile(wchar_t* wszFilePath, DWORD dwProcessId)
{
	wprintf(L"Enter ParseTraceFile \r\n");
	EVENT_TRACE_LOGFILE eventTraceLogFile;
	TRACE_LOGFILE_HEADER* pHeader = &eventTraceLogFile.LogfileHeader;
	TRACEHANDLE hTraceHandle = NULL;
	ZeroMemory(&eventTraceLogFile, sizeof(EVENT_TRACE_LOGFILE));
	eventTraceLogFile.LogFileName = wszFilePath;
	eventTraceLogFile.BufferCallback =(PEVENT_TRACE_BUFFER_CALLBACK) (ConsumerBufferCallback);
	eventTraceLogFile.EventCallback = (PEVENT_CALLBACK) (ConsumerEventCallback);

	hTraceHandle = ::OpenTrace(&eventTraceLogFile);
	if((TRACEHANDLE)INVALID_HANDLE_VALUE == hTraceHandle)
	{
		DWORD dwErrCode = ::GetLastError();
		wprintf(L"ParseTraceFile: OpenTrace failed, error code is %u \r\n", dwErrCode);
		return dwErrCode;
	}

	ms_usPointerSize = (USHORT)pHeader->PointerSize;

	if ((ULONGLONG)-1 != hTraceHandle && (ULONGLONG)0x0FFFFFFFF != hTraceHandle) // 不能用INVALID_PROCESSTRACE_HANDLE比较
	{
		ULONG ulRet = ::ProcessTrace(&hTraceHandle, 1, 0, 0);
		if (ulRet != ERROR_SUCCESS)
		{
			wprintf(L"ParseTraceFile: ProcessTrace failed, error code is %u \r\n", ulRet);
			return ulRet;
		}
	}

	/*if((TRACEHANDLE)INVALID_HANDLE_VALUE != hTraceHandle)
	{
		::CloseTrace(hTraceHandle);
	}*/

	return 0;
}


void WINAPI ETWConsumer::ConsumerEventCallback(PEVENT_TRACE pEvent)
{
	PBYTE pEventData = NULL;  
	PBYTE pEndOfEventData = NULL;
	ULONGLONG TimeStamp = 0;
	ULONGLONG Nanoseconds = 0;
	SYSTEMTIME st;
	SYSTEMTIME stLocal;
	FILETIME ft;


	// Skips the event if it is the event trace header. Log files contain this event
	// but real-time sessions do not. The event contains the same information as 
	// the EVENT_TRACE_LOGFILE.LogfileHeader member that you can access when you open 
	// the trace. 

	if (::IsEqualGUID(pEvent->Header.Guid, EventTraceGuid) && pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO)
	{
		return; // Skip this event.
	}
	else if(::IsEqualGUID(pEvent->Header.Guid, ProcessGuid))
	{
		// 进程事件	

		// If the event contains event-specific data find the MOF class that
		// contains the format of the event data.
		if ((pEvent->Header.Class.Type == EVENT_TRACE_TYPE_START) && (pEvent->MofLength > 0))
		{
			pEventData = (PBYTE)(pEvent->MofData);
			pEndOfEventData = ((PBYTE)(pEvent->MofData) + pEvent->MofLength);

			ULONG ulOffset = ms_usPointerSize + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(LONG) + ms_usPointerSize;//UniqueProcessKey(Pointer)+ProcessId(uint32)+ParentId(uint32)+SessionId(uint32)+ExitStatus(sint32)+DirectoryTableBase(Pointer)
			ULONG ulObjectDataLen = GetObjectDataLen(pEventData + ulOffset, L"Sid", 1, (USHORT)(pEndOfEventData - pEventData));
			ulOffset = ulOffset + ulObjectDataLen;//+UserSID(object)

			char szImageFileName[MAX_PATH];
			strcpy(szImageFileName, (char*)(pEventData+ulOffset));
			if (0 == stricmp(szImageFileName, "Thunder.exe")) //Thunder进程启动
			{
				// Print the time stamp for when the event occurred.
				ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
				ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
				FileTimeToSystemTime(&ft, &st);
				SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
				TimeStamp = pEvent->Header.TimeStamp.QuadPart;
				Nanoseconds = (TimeStamp % 10000000) * 100;
				wprintf(L"ProcessGuid: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
			}
		}
	}
	else if (::IsEqualGUID(pEvent->Header.Guid, CategoryGuid))
	{
		// Thunder自定义事件

		if (pEvent->Header.Class.Type == ETWPROVIDER_PROCESS_SHOWVIEW) //迅雷主界面出现
		{
			// Print the time stamp for when the event occurred.
			ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
			ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
			FileTimeToSystemTime(&ft, &st);
			SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
			TimeStamp = pEvent->Header.TimeStamp.QuadPart;
			Nanoseconds = (TimeStamp % 10000000) * 100;
			wprintf(L"CategoryGuid: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
		}
	}
}

ULONG WINAPI ETWConsumer::ConsumerBufferCallback(PEVENT_TRACE_LOGFILE pBuffer)
{
	//To continue processing events, return TRUE. Otherwise, return FALSE. 
	//Returning FALSE will terminate the ProcessTrace function.
	return TRUE;
}

ULONG ETWConsumer::GetObjectDataLen(PBYTE pEventData, const wchar_t* wszExtension, ULONG ulArraySize, ULONG ulRemainingBytes)
{
	// An object data type has to include the Extension qualifier.

	ULONG ulObjectDataLen = 0;

	if (_wcsicmp(L"SizeT", wszExtension) == 0)
	{
		wprintf(L"GetObjectDataLen: wszExtension = SizeT \r\n");
		// You do not need to know the data type of the property, you just 
		// retrieve either 4 bytes or 8 bytes depending on the pointer's size.

		for (ULONG i = 0; i < ulArraySize; i++)
		{			
			ulObjectDataLen += ms_usPointerSize;
		}

		return ulObjectDataLen;
	}
	if (_wcsicmp(L"Port", wszExtension) == 0)
	{
		wprintf(L"GetObjectDataLen: wszExtension = Port \r\n");
		USHORT temp = 0;

		for (ULONG i = 0; i < ulArraySize; i++)
		{
			ulObjectDataLen += sizeof(USHORT);
		}

		return ulObjectDataLen;
	}
	else if (_wcsicmp(L"IPAddr", wszExtension) == 0 || _wcsicmp(L"IPAddrV4", wszExtension) == 0)
	{
		wprintf(L"GetObjectDataLen: wszExtension = IPAddr | IPAddrV4 \r\n");
		ULONG temp = 0;

		for (ULONG i = 0; i < ulArraySize; i++)
		{
			ulObjectDataLen += sizeof(ULONG);
		}

		return ulObjectDataLen;
	}
	else if (_wcsicmp(L"IPAddrV6", wszExtension) == 0)
	{
		wprintf(L"GetObjectDataLen: wszExtension = IPAddrV6 \r\n");
		for (ULONG i = 0; i < ulArraySize; i++)
		{
			ulObjectDataLen += sizeof(IN6_ADDR);
		}

		return ulObjectDataLen;
	}
	else if (_wcsicmp(L"Guid", wszExtension) == 0)
	{
		wprintf(L"GetObjectDataLen: wszExtension = Guid \r\n");
		for (ULONG i = 0; i < ulArraySize; i++)
		{
			ulObjectDataLen += sizeof(GUID);
		}

		return ulObjectDataLen;
	}
	else if (_wcsicmp(L"Sid", wszExtension) == 0)
	{
		wprintf(L"GetObjectDataLen: wszExtension = Sid \r\n");
		// Get the user's security identifier and print the 
		// user's name and domain.

		SID* psid;
		ULONG temp = 0;
		USHORT CopyLength = 0;
		BYTE buffer[SECURITY_MAX_SID_SIZE];

		for (ULONG i = 0; i < ulArraySize; i++)
		{
			CopyMemory(&temp, pEventData, sizeof(ULONG));

			if (temp > 0)
			{
				// A property with the Sid extension is actually a 
				// TOKEN_USER structure followed by the SID. The size
				// of the TOKEN_USER structure differs depending on 
				// whether the events were generated on a 32-bit or 
				// 64-bit architecture. Also the structure is aligned
				// on an 8-byte boundary, so its size is 8 bytes on a
				// 32-bit computer and 16 bytes on a 64-bit computer.
				// Doubling the pointer size handles both cases.

				USHORT BytesToSid = ms_usPointerSize * 2;

				pEventData += BytesToSid;
				ulObjectDataLen += BytesToSid;

				if (ulRemainingBytes - BytesToSid > SECURITY_MAX_SID_SIZE)
				{
					CopyLength = SECURITY_MAX_SID_SIZE;
				}
				else
				{
					CopyLength = ulRemainingBytes - BytesToSid;
				}

				::CopyMemory(&buffer, pEventData, CopyLength);
				psid = (SID*)&buffer;				

				ulObjectDataLen += SeLengthSid(psid);
			}
			else  // There is no SID
			{
				wprintf(L"GetObjectDataLen: temp <= 0 \r\n");
				ulObjectDataLen += sizeof(ULONG);
			}
		}

		return ulObjectDataLen;
	}
	else
	{
		wprintf(L"Extension, %s, not supported.\n", wszExtension);
		return 0;
	}
}

