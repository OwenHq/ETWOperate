
#pragma once
#include "ETWCommonDefine.h"
#include <vector>
#include <Wbemcli.h>
#include <comutil.h>
#include <In6addr.h>


// Macro for determining the length of a SID.
#define SeLengthSid( Sid ) \
	(8 + (4 * ((SID*)Sid)->SubAuthorityCount))

typedef struct _propertyList
{
	BSTR Name;     // Property name
	LONG CimType;  // Property data type
	IWbemQualifierSet* pQualifiers;
} PROPERTY_LIST;

typedef LPTSTR (NTAPI *PIPV6ADDRTOSTRING)(
	const IN6_ADDR *Addr,
	LPTSTR S
	);


class ETWConsumer
{
public:
	ETWConsumer();
	~ETWConsumer();

public:
	//解释ETL文件
	ULONG ParseTraceFile(wchar_t* wszFilePath, DWORD dwProcessId);

private:
	//Pointer to the EventCallback function that ETW calls for each event in the buffer.
	//Specify this callback if you are consuming events from a provider that used one of the TraceEvent functions to log events.
	static void WINAPI ConsumerEventCallback(PEVENT_TRACE pEvent);
	//Pointer to the BufferCallback function that receives buffer-related statistics for each buffer ETW flushes. 
	//ETW calls this callback after it delivers all the events in the buffer. This callback is optional.
	static ULONG WINAPI ConsumerBufferCallback(PEVENT_TRACE_LOGFILE pBuffer);

	//获得object类型数据的长度
	static ULONG GetObjectDataLen(PBYTE pEventData, const wchar_t* wszExtension, ULONG ulArraySize, ULONG ulRemainingBytes);

private:
	static DWORD ms_dwProcessId;
	static BOOL ms_bStopTrace;
	static std::vector<DWORD> ms_vThreadId;
	// Points to WMI namespace that contains the ETW MOF classes.
	static IWbemServices* g_pServices;
	// Used to determine the data size of property values that contain the
	// Pointer or SizeT qualifier. The value will be 4 or 8.
	static USHORT ms_usPointerSize;
};