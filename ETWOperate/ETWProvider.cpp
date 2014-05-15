
#include "stdafx.h"
#include "ETWProvider.h"


TRACEHANDLE ETWProvider::ms_RegistationHandle = 0;
TRACEHANDLE ETWProvider::ms_TraceHandle = 0;
UCHAR ETWProvider::ms_EnableLevel = 0;
ULONG ETWProvider::ms_EnableFalgs = 0;
BOOL ETWProvider::ms_TraceOn = FALSE;

ULONG ETWProvider::RegisterProviderGuid()
{
	TRACE_GUID_REGISTRATION EventClassGuids[] = {(LPGUID)&CategoryGuid, NULL}; 
	ms_RegistationHandle = 0;

	return ::RegisterTraceGuids(ControlCallback, NULL,(LPGUID)&ProviderGuid, sizeof(EventClassGuids) / sizeof(TRACE_GUID_REGISTRATION), EventClassGuids, NULL, NULL, &ms_RegistationHandle);
}

ULONG ETWProvider::UnRegisterProviderGuid()
{
	if (ms_RegistationHandle)
	{
		return ::UnregisterTraceGuids(ms_RegistationHandle);
	}

	return ERROR_INVALID_HANDLE;
}

ULONG ETWProvider::TraceCustomEvent(UCHAR EventType)
{
	EVENT_DATA EventData;
	EventData.Cost = 32;
	EventData.ID = TempId;
	EventData.Indices[0] = 4;
	EventData.Indices[1] = 5;
	EventData.Indices[2] = 6;
	EventData.IsComplete = TRUE;
	wcscpy_s(EventData.Signature, MAX_SIGNATURE_LEN, L"Signature");
	EventData.Size = 1024;
	if(ms_TraceOn && (4 == ms_EnableLevel || TRACE_LEVEL_ERROR <= ms_EnableFalgs ))
	{
		CUSTOM_EVENT CustomEvent;

		// initial the event data structure;

		ZeroMemory(&CustomEvent, sizeof(CUSTOM_EVENT));
		CustomEvent.Header.Size = sizeof(EVENT_TRACE_HEADER) + sizeof(MOF_FIELD) * EVENT_DATA_FIELDS_CNT;
		CustomEvent.Header.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
		CustomEvent.Header.Guid = CategoryGuid;
		CustomEvent.Header.Class.Type = EventType;
		CustomEvent.Header.Class.Version = 1;
		CustomEvent.Header.Class.Level = ms_EnableLevel;

		// Load the event data
		CustomEvent.Data[0].DataPtr = (ULONG64) &(EventData.Cost);
		CustomEvent.Data[0].Length = sizeof(EventData.Cost);
		CustomEvent.Data[1].DataPtr = (ULONG64) &(EventData.Indices);
		CustomEvent.Data[1].Length = sizeof(EventData.Indices);
		CustomEvent.Data[2].DataPtr = (ULONG64) &(EventData.Signature);
		CustomEvent.Data[2].Length = (ULONG) ((wcslen(EventData.Signature) + 1) *sizeof(WCHAR));
		CustomEvent.Data[3].DataPtr = (ULONG64) &(EventData.IsComplete);
		CustomEvent.Data[3].Length = sizeof(EventData.IsComplete);
		CustomEvent.Data[4].DataPtr = (ULONG64) &(EventData.ID);
		CustomEvent.Data[4].Length = sizeof(EventData.ID);
		CustomEvent.Data[5].DataPtr = (ULONG64) &(EventData.Size);
		CustomEvent.Data[5].Length = sizeof(EventData.Size);

		ULONG status = ::TraceEvent(ms_TraceHandle, &(CustomEvent.Header));
		if(ERROR_SUCCESS != status)
		{
			//wprintf(L"Trace Failed");
			ms_TraceOn = FALSE;
		}

		return status;
	}
	return FALSE;
}


ULONG WINAPI ETWProvider::ControlCallback(WMIDPREQUESTCODE RequestCode, PVOID pContext, ULONG *Reserved, PVOID Header)
{
	UNREFERENCED_PARAMETER(pContext);
	UNREFERENCED_PARAMETER(Reserved);

	ULONG status = ERROR_SUCCESS;
	TRACEHANDLE TempSessionHandle = 0; 

	switch (RequestCode)
	{
	case WMI_ENABLE_EVENTS:  // Enable the provider.
		{
			::SetLastError(0);

			// If the provider is already enabled to a provider, ignore 
			// the request. Get the session handle of the enabling session.
			// You need the session handle to call the TraceEvent function.
			// The session could be enabling the provider or it could be
			// updating the level and enable flags.

			TempSessionHandle = ::GetTraceLoggerHandle(Header);
			if (INVALID_HANDLE_VALUE == (HANDLE)TempSessionHandle)
			{
				//wprintf(L"GetTraceLoggerHandle failed. Error code is %lu.\n", status = GetLastError());
				break;
			}

			if (0 == ms_TraceHandle)
			{
				ms_TraceHandle = TempSessionHandle;
			}
			else if (ms_TraceHandle != TempSessionHandle)
			{
				break;
			}

			// Get the severity level of the events that the
			// session wants you to log.

			ms_EnableLevel = ::GetTraceEnableLevel(ms_TraceHandle); 
			if (0 == ms_EnableLevel)
			{
				// If zero, determine whether the session passed zero
				// or an error occurred.

				if (ERROR_SUCCESS == (status = ::GetLastError()))
				{
					// Decide what a zero enable level means to your provider.
					// For this example, it means log all events.
					; 
				}
				else
				{
					//wprintf(L"GetTraceEnableLevel failed with, %lu.\n", status);
					break;
				} 
			}

			// Get the enable flags that indicate the events that the
			// session wants you to log. The provider determines the
			// flags values. How it articulates the flag values and 
			// meanings to perspective sessions is up to it.

			ms_EnableFalgs = ::GetTraceEnableFlags(ms_TraceHandle);
			if (0 == ms_EnableFalgs)
			{
				// If zero, determine whether the session passed zero
				// or an error occurred.

				if (ERROR_SUCCESS == (status = ::GetLastError()))
				{
					// Decide what a zero enable flags value means to your provider.
					; 
				}
				else
				{
					//wprintf(L"GetTraceEnableFlags failed with, %lu.\n", status);
					break;
				}
			}

			ms_TraceOn = TRUE;
			break;
		}

	case WMI_DISABLE_EVENTS:  // Disable the provider.
		{
			// Disable the provider only if the request is coming from the
			// session that enabled the provider.

			TempSessionHandle = ::GetTraceLoggerHandle(Header);
			if (INVALID_HANDLE_VALUE == (HANDLE)TempSessionHandle)
			{
				//wprintf(L"GetTraceLoggerHandle failed. Error code is %lu.\n", status = ::GetLastError());
				break;
			}

			if (ms_TraceHandle == TempSessionHandle)
			{
				ms_TraceOn = FALSE;
				ms_TraceHandle = 0;
			}
			break;
		}

	default:
		{
			status = ERROR_INVALID_PARAMETER;
			break;
		}
	}

	return status;
}