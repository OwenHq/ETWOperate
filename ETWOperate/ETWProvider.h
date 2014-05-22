/*

	ETW�¼��ṩ�ߣ����ṩETW�¼��ĳ��������������Ӧ�ӿڡ�

*/

#pragma once
#include "ETWCommonDefine.h"

#define MAX_INDICES	3
#define MAX_SIGNATURE_LEN	32
#define EVENT_DATA_FIELDS_CNT	16


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
	//ע��ETW��־�¼��ṩ��
	static ULONG RegisterProviderGuid();
	//��ע��ETW��־�¼��ṩ��
	static ULONG UnRegisterProviderGuid();

	//�����¼��������¼��Ự
	static ULONG TraceCustomEvent(UCHAR EventType);

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