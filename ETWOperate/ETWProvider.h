/*

	ETW事件提供者，由提供ETW事件的程序包含并调用相应接口。

*/

#pragma once
#include <WTypes.h>
#include <Guiddef.h>
#include <Wmistr.h>
#include <Evntrace.h>

#define MAX_INDICES	3
#define MAX_SIGNATURE_LEN	32

// GUID that identifies the category of events that the provider can log. 
// The GUID is also used in the event MOF class. 
// Remember to change this GUID if you copy and paste this example.
// {D1613217-E0A5-4c77-A4D2-DBAD23B80C71}
static const GUID CategoryGuid = 
{ 0xd1613217, 0xe0a5, 0x4c77, { 0xa4, 0xd2, 0xdb, 0xad, 0x23, 0xb8, 0xc, 0x71 } };

// GUID that identifies the provider that you are registering.
// The GUID is also used in the provider MOF class. 
// Remember to change this GUID if you copy and paste this example.
// {36EF4ADA-9F6B-437a-AE0C-668CD7C291DC}
static const GUID ProviderGuid = 
{ 0x36ef4ada, 0x9f6b, 0x437a, { 0xae, 0xc, 0x66, 0x8c, 0xd7, 0xc2, 0x91, 0xdc } };

// GUID used as the value for EVENT_DATA.ID.
// {74259B1E-FA3C-4f00-91F7-F8DB0E78E020}
static const GUID TempId = 
{ 0x74259b1e, 0xfa3c, 0x4f00, { 0x91, 0xf7, 0xf8, 0xdb, 0xe, 0x78, 0xe0, 0x20 } };


// Identifies the event type within the MyCategoryGuid category 
// of events to be logged. This is the same value as the EventType 
// qualifier that is defined in the event type MOF class for one of 
// the MyCategoryGuid category of events.
#define ETWPROVIDER_EVENT	1


class ETWProvider
{
public:
	// Event passed to TraceEvent
	typedef struct tagCustomEvent
	{
		EVENT_TRACE_HEADER Header;
		MOF_FIELD Data[MAX_MOF_FIELDS];  // Event-specific data
	}CUSTOM_EVENT, * LPCUSTOM_EVENT;
	// Application data to be traced for Version 1 of the MOF class.
	typedef struct tagEventdata
	{
		LONG Cost;
		DWORD Indices[MAX_INDICES];
		WCHAR Signature[MAX_SIGNATURE_LEN];
		BOOL IsComplete;
		GUID ID;
		DWORD Size;
	}EVENT_DATA, * LPVEVENT_DATA;

public:
	//注册ETW日志事件提供者
	static ULONG RegisterProviderGuid();
	//反注册ETW日志事件提供者
	static ULONG UnRegisterProviderGuid();

	//发送事件到跟踪事件会话
	static ULONG TraceEvent();

private:
	// The callback function that receives enable/disable notifications
	// from one or more ETW sessions. Because more than one session
	// can enable the provider, this example ignores requests from other 
	// sessions if it is already enabled.
	static ULONG WINAPI ControlCallback(WMIDPREQUESTCODE RequestCode, PVOID pContext, ULONG *Reserved, PVOID Header);

private:
	static TRACEHANDLE ms_RegistationHandle;
	static TRACEHANDLE ms_TraceHandle;
	static UCHAR ms_EnableLevel;
	static ULONG ms_EnableFalgs;
	static BOOL ms_TraceOn;
};