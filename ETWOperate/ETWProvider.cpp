
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

ULONG ETWProvider::TraceEvent()
{
	return 0;
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