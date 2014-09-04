
#include "stdafx.h"
#include "ETWConsumer.h"
#include "CommonTool.h"

ETWConsumer::OperationType						ETWConsumer::ms_operationType = ETWConsumer::OperationType_RunUpTime;
FILE*											ETWConsumer::ms_pFile = NULL;
DWORD											ETWConsumer::ms_dwThunderProcessId = 0;
USHORT											ETWConsumer::ms_usPointerSize = 0;
ULONGLONG										ETWConsumer::ms_ullThunderCreateTime = 0;
std::vector<ETWConsumer::ImageInfo>				ETWConsumer::ms_vImageInfo;
std::map<ULONGLONG, std::wstring>				ETWConsumer::ms_mFileObjectToName;
std::map<ULONGLONG, ETWConsumer::FileIoInfo>	ETWConsumer::ms_mFileObjectToFileIoInfo;
std::map<ULONGLONG, ETWConsumer::DiskIoInfo>	ETWConsumer::ms_mFileObjectToDiskIoInfo;
std::map<ULONGLONG, ETWConsumer::EtlInfo>		ETWConsumer::ms_mFileObjectToEtlInfo;
std::map<ULONG, ETWConsumer::ThreadInfo>		ETWConsumer::ms_mThreadIdToInfo;

ETWConsumer::ETWConsumer()
{
	
}

ETWConsumer::~ETWConsumer()
{

}


ULONG ETWConsumer::ParseTraceFile(wchar_t* wszEtlFilePath, OperationType operationType, const wchar_t* wszResultFilePath)
{
	TSDEBUG(L"Enter");

	ms_operationType = operationType;

	//打开文件
	char szSaveFilePath[MAX_PATH]; 
	//宽字节编码转换成多字节编码   
	WideCharToMultiByte(CP_ACP, 0, wszResultFilePath, -1, szSaveFilePath, MAX_PATH, NULL, NULL);   
	ms_pFile = fopen(szSaveFilePath, "a+");

	//如果为获取启动时间，则把对应的ETL文件名写到存储结果的文件里
	if (OperationType_RunUpTime == operationType)
	{
		std::wstring wstrEtlMainName = CommonTool::GetMainNameFromPath(wszEtlFilePath);
		std::string strFileName;
		strFileName = CommonTool::WstringToString(wstrEtlMainName);
		fprintf(ms_pFile, "%s ", wstrEtlMainName.c_str());
	}

	EVENT_TRACE_LOGFILE eventTraceLogFile;
	TRACE_LOGFILE_HEADER* pHeader = &eventTraceLogFile.LogfileHeader;
	TRACEHANDLE hTraceHandle = NULL;
	ZeroMemory(&eventTraceLogFile, sizeof(EVENT_TRACE_LOGFILE));
	eventTraceLogFile.LogFileName = wszEtlFilePath;
	eventTraceLogFile.BufferCallback =(PEVENT_TRACE_BUFFER_CALLBACK) (ConsumerBufferCallback);
	eventTraceLogFile.EventCallback = (PEVENT_CALLBACK) (ConsumerEventCallback);

	hTraceHandle = ::OpenTrace(&eventTraceLogFile);
	if((TRACEHANDLE)INVALID_HANDLE_VALUE == hTraceHandle)
	{
		DWORD dwErrCode = ::GetLastError();
		TSDEBUG(L"ParseTraceFile: OpenTrace failed, error code is %u \r\n", dwErrCode);
		return dwErrCode;
	}

	ms_usPointerSize = (USHORT)pHeader->PointerSize;

	if ((ULONGLONG)-1 != hTraceHandle && (ULONGLONG)0x0FFFFFFFF != hTraceHandle) // 不能用INVALID_PROCESSTRACE_HANDLE比较
	{
		ULONG ulRet = ::ProcessTrace(&hTraceHandle, 1, 0, 0);
		if (ulRet != ERROR_SUCCESS)
		{
			TSDEBUG(L"ParseTraceFile: ProcessTrace failed, error code is %u \r\n", ulRet);
			return ulRet;
		}
	}

	ULONG ulRet = ::CloseTrace(hTraceHandle);
	TSDEBUG(L"ParseTraceFile: CloseTrace-lRet = %u \r\n", ulRet);

	MergeEtlInfo();
	OutputEtlInfo();

	fclose(ms_pFile);

	return 0;
}


void WINAPI ETWConsumer::ConsumerEventCallback(PEVENT_TRACE pEvent)
{
	PBYTE pEventData = NULL;  
	PBYTE pEndOfEventData = NULL;
	static BOOL s_bThunderMainThread = TRUE;

	// Skips the event if it is the event trace header. Log files contain this event
	// but real-time sessions do not. The event contains the same information as 
	// the EVENT_TRACE_LOGFILE.LogfileHeader member that you can access when you open 
	// the trace. 

	if (::IsEqualGUID(pEvent->Header.Guid, EventTraceGuid) && pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO)
	{
		return; // Skip this event.
	}

	pEventData = (PBYTE)(pEvent->MofData);
	pEndOfEventData = ((PBYTE)(pEvent->MofData) + pEvent->MofLength);
	
	//TSDEBUG(L"ms_dwThunderProcessId = %u, pEvent->Header.ProcessId = %u", ms_dwThunderProcessId, pEvent->Header.ProcessId);
	if(::IsEqualGUID(pEvent->Header.Guid, ProcessGuid))
	{
		// 进程事件	
		TSDEBUG(L"ProcessGuid");

		// If the event contains event-specific data find the MOF class that
		// contains the format of the event data.
		if ((pEvent->Header.Class.Type == EVENT_TRACE_TYPE_START) && (pEvent->MofLength > 0))
		{
			ULONG ulImageFileNameOffset = ms_usPointerSize + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(LONG) + ms_usPointerSize;//UniqueProcessKey(Pointer)+ProcessId(uint32)+ParentId(uint32)+SessionId(uint32)+ExitStatus(sint32)+DirectoryTableBase(Pointer)
			ULONG ulObjectDataLen = GetObjectDataLen(pEventData + ulImageFileNameOffset, L"Sid", 1, (USHORT)(pEndOfEventData - pEventData));
			ulImageFileNameOffset = ulImageFileNameOffset + ulObjectDataLen;//+UserSID(object)

			char szImageFileName[MAX_PATH];
			strcpy(szImageFileName, (char*)(pEventData + ulImageFileNameOffset));
			if (0 == stricmp(szImageFileName, "Thunder.exe")) //Thunder进程启动
			{
				// Print the time stamp for when the event occurred.

				if (OperationType_AnalyzeEtl == ms_operationType)
				{
					SYSTEMTIME st;
					SYSTEMTIME stLocal;
					FILETIME ft;
					ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
					ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
					::FileTimeToSystemTime(&ft, &st);
					::SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
					ULONGLONG TimeStamp = 0;
					ULONGLONG Nanoseconds = 0;
					TimeStamp = pEvent->Header.TimeStamp.QuadPart;
					Nanoseconds = (TimeStamp % 10000000) * 100;
					//wprintf(L"ProcessGuid: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
					fprintf(ms_pFile, "ETWPROVIDER_PROCESS_STARTTHUNDERPROCESS: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
				}
				
				//存储进程ID
				::CopyMemory(&ms_dwThunderProcessId, (pEventData + ms_usPointerSize), sizeof(ULONG));
				//wprintf(L"ProcessGuid: ms_dwThunderProcessId = %u \n", ms_dwThunderProcessId);
				TSDEBUG(L"ProcessGuid: ms_dwThunderProcessId = %u", ms_dwThunderProcessId);

				//存储Thunder创建时间
				ms_ullThunderCreateTime = pEvent->Header.TimeStamp.QuadPart;
			}
		}
	}
	else if (::IsEqualGUID(pEvent->Header.Guid, ThreadGuid))
	{
		//线程事件，线程ID在整个系统中是唯一的
		TSDEBUG(L"ThreadGuid");

		if (EVENT_TRACE_TYPE_START == pEvent->Header.Class.Type)
		{
			DWORD dwProcessId = 0;
			::CopyMemory(&dwProcessId, pEventData, sizeof(ULONG));

			if (dwProcessId == ms_dwThunderProcessId)
			{
				DWORD dwThreadId = 0;
				::CopyMemory(&dwThreadId, pEventData + sizeof(ULONG), sizeof(ULONG));
				TSDEBUG(L"ThreadGuid: dwThreadId = %u", dwThreadId);

				std::map<ULONG, ThreadInfo>::iterator itThread = ms_mThreadIdToInfo.find(dwThreadId);
				if (ms_mThreadIdToInfo.end() == itThread)
				{
					ThreadInfo threadInfo;
					threadInfo.ullFileIoSize = 0;
					threadInfo.ulFileIoReadCount = 0;
					threadInfo.ulFileIoWriteCount = 0;
					threadInfo.ullDiskIoSize = 0;
					threadInfo.ulDiskIoReadCount = 0;
					threadInfo.ulDiskIoWriteCount = 0;
					ms_mThreadIdToInfo.insert(std::pair<ULONG, ThreadInfo>(dwThreadId, threadInfo));
				}

				if (s_bThunderMainThread)
				{
					s_bThunderMainThread = FALSE;

					// Print the time stamp for when the event occurred.
					ULONGLONG TimeStamp = 0;
					TimeStamp = pEvent->Header.TimeStamp.QuadPart;

					if (OperationType_RunUpTime == ms_operationType)
					{
						//记录Thunder进程进入WinMain的时间
						ULONGLONG ullMainThreadTime = (TimeStamp - ms_ullThunderCreateTime) / 10000;
						fprintf(ms_pFile, "%I64u ", ullMainThreadTime);
					}
					else if (OperationType_AnalyzeEtl == ms_operationType)
					{
						SYSTEMTIME st;
						SYSTEMTIME stLocal;
						FILETIME ft;
						ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
						ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
						FileTimeToSystemTime(&ft, &st);
						SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
						ULONGLONG Nanoseconds = 0;
						Nanoseconds = (TimeStamp % 10000000) * 100;
						//wprintf(L"ETWPROVIDER_PROCESS_ENTERMAINTHREAD: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
						fprintf(ms_pFile, "ETWPROVIDER_PROCESS_ENTERMAINTHREAD: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
					}
				}
			}
		}
	}
	else if (::IsEqualGUID(pEvent->Header.Guid, CategoryGuid))
	{
		// Thunder自定义事件
		TSDEBUG(L"CategoryGuid");

		if (pEvent->Header.Class.Type == ETWPROVIDER_PROCESS_ENTERWMAIN)
		{
			// Print the time stamp for when the event occurred.
			ULONGLONG TimeStamp = 0;
			TimeStamp = pEvent->Header.TimeStamp.QuadPart;

			if (OperationType_RunUpTime == ms_operationType)
			{
				//记录进程Thunder进程WinMain的时间
				ULONGLONG ullWinMainTime = (TimeStamp - ms_ullThunderCreateTime) / 10000;
				fprintf(ms_pFile, "%I64u ", ullWinMainTime);
			}
			else if (OperationType_AnalyzeEtl == ms_operationType)
			{
				SYSTEMTIME st;
				SYSTEMTIME stLocal;
				FILETIME ft;
				ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
				ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
				FileTimeToSystemTime(&ft, &st);
				SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
				ULONGLONG Nanoseconds = 0;
				Nanoseconds = (TimeStamp % 10000000) * 100;
				//wprintf(L"ETWPROVIDER_PROCESS_ENTERWMAIN: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
				fprintf(ms_pFile, "ETWPROVIDER_PROCESS_ENTERWMAIN: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
			}
		}
		else if (pEvent->Header.Class.Type == ETWPROVIDER_PROCESS_SHOWVIEW) //迅雷主界面出现
		{
			// Print the time stamp for when the event occurred.
			ULONGLONG TimeStamp = 0;
			TimeStamp = pEvent->Header.TimeStamp.QuadPart;

			//记录Thunder进程界面出现的时间
			if (OperationType_RunUpTime == ms_operationType)
			{
				ULONGLONG ullShowViewTime = (TimeStamp - ms_ullThunderCreateTime) / 10000;
				fprintf(ms_pFile, "%I64u\r\n", ullShowViewTime);
			}
			else if (OperationType_AnalyzeEtl == ms_operationType)
			{
				SYSTEMTIME st;
				SYSTEMTIME stLocal;
				FILETIME ft;
				ULONGLONG Nanoseconds = 0;
				ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
				ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
				FileTimeToSystemTime(&ft, &st);
				SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
				Nanoseconds = (TimeStamp % 10000000) * 100;
				//wprintf(L"ETWPROVIDER_PROCESS_SHOWVIEW: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
				fprintf(ms_pFile, "ETWPROVIDER_PROCESS_SHOWVIEW: %02d/%02d/%02d %02d:%02d:%02d.%I64u\n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, Nanoseconds);
			}
		}
	}
	else if (OperationType_AnalyzeEtl == ms_operationType && ::IsEqualGUID(pEvent->Header.Guid, ImageLoadGuid))
	{
		//镜像加载事件

		//ProcessId
		DWORD dwProcessId = 0;
		::CopyMemory(&dwProcessId, pEventData + 2 * ms_usPointerSize, sizeof(ULONG));

		if (OperationType_AnalyzeEtl == ms_operationType && EVENT_TRACE_TYPE_LOAD == pEvent->Header.Class.Type && ms_dwThunderProcessId != 0 && dwProcessId == ms_dwThunderProcessId)
		{
			//ImageSize
			ULONG ulImageSize = 0;
			::CopyMemory(&ulImageSize, pEventData + ms_usPointerSize, ms_usPointerSize);

			//FileName
			wchar_t wszFileName[MAX_PATH];
			ULONG ulFileNameOffset = ms_usPointerSize + ms_usPointerSize + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + ms_usPointerSize + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG);//ImageBase(pointer)+ImageSize(pointer)+ProcessId(uint32)+ImageCheckSum(uint32)+TimeDateStamp(uint32)+Reserved0(uint32)+DefaultBase(pointer)+Reserved1(uint32)+Reserved2(uint32)+Reserved3(uint32)+Reserved4(uint32)
			wcscpy(wszFileName, (wchar_t*)(pEventData + ulFileNameOffset));

			//TimeDateStamp
			ULONG ulTimeDateStamp = 0;
			ULONG ulTimeDataStampOffset = ms_usPointerSize + ms_usPointerSize + sizeof(ULONG) + sizeof(ULONG);//ImageBase(pointer)+ImageSize(pointer)+ProcessId(uint32)+ImageCheckSum(uint32)
			::CopyMemory(&ulTimeDateStamp, pEventData + ulTimeDataStampOffset, sizeof(ULONG));

			//存储
			ImageInfo imageInfo;
			imageInfo.wstrFileName = wszFileName;
			imageInfo.ulTimeDateStamp = ulTimeDateStamp;
			imageInfo.ullImageSize = ulImageSize;
			ms_vImageInfo.push_back(imageInfo);
/*
			char szType[32];
			if (EVENT_TRACE_TYPE_LOAD == pEvent->Header.Class.Type)
			{
				strcpy(szType, "EVENT_TRACE_TYPE_LOAD");
			}
			else if (EVENT_TRACE_TYPE_END == pEvent->Header.Class.Type)
			{
				strcpy(szType, "EVENT_TRACE_TYPE_END");
			}
			else if (EVENT_TRACE_TYPE_DC_START == pEvent->Header.Class.Type)
			{
				strcpy(szType, "EVENT_TRACE_TYPE_DC_START");
			}
			else if (EVENT_TRACE_TYPE_DC_END == pEvent->Header.Class.Type)
			{
				strcpy(szType, "EVENT_TRACE_TYPE_DC_END");
			}
			char szFileName[MAX_PATH];
			//宽字节编码转换成多字节编码   
			WideCharToMultiByte(CP_ACP, 0, wszFileName, -1, szFileName, MAX_PATH, NULL, NULL);  
			fprintf(ms_pFile, "%s：%s\r\n", szType, szFileName);*/
		}
	}
	else if (OperationType_AnalyzeEtl == ms_operationType && ::IsEqualGUID(pEvent->Header.Guid, FileIoGuid))
	{
		//触发FileIo事件

		UCHAR ucType = pEvent->Header.Class.Type;
		//TSDEBUG(L"FileIoGuid: ucType = %d", ucType);
		if (0 == ucType || 32 == ucType || 35 == ucType || 36 == ucType)
		{
			//FileObject: Match the value of this pointer to the FileObject pointer value in a DiskIo_TypeGroup1 event to determine the type of I/O operation.
			ULONGLONG ullFileObject = 0;
			::CopyMemory(&ullFileObject, pEventData, ms_usPointerSize);

			//FileName
			wchar_t wszFileName[MAX_PATH];
			wcscpy(wszFileName, (wchar_t*)(pEventData + ms_usPointerSize));
			
			//存储FileObject-FileName
			std::map<ULONGLONG, std::wstring>::iterator itFileObject = ms_mFileObjectToName.find(ullFileObject);
			if (ms_mFileObjectToName.end() == itFileObject)
			{
				//TSDEBUG(L"FileIoGuid: wszFileName = %s", wszFileName);
				//存储文件对象和文件名的对应关系
				ms_mFileObjectToName[ullFileObject] = wszFileName;
			}
		}
		else if (67 == ucType || 68 == ucType)
		{
			//TTID: Thread identifier of the thread that is performing the operation.
			ULONG ulThreadId = 0;
			ULONG ulThreadIdOffset = sizeof(ULONGLONG) + ms_usPointerSize;//Offset(uint64)+IrpPtr(pointer)
			::CopyMemory(&ulThreadId, pEventData + ulThreadIdOffset, ms_usPointerSize);

			//判断是否为Thunder的线程
			std::map<ULONG, ThreadInfo>::iterator itThunderThread = ms_mThreadIdToInfo.find(ulThreadId);
			if (ms_mThreadIdToInfo.end() != itThunderThread)
			{
				//FileObject: Identifier that can be used for correlating operations to the same opened file object instance between file create and close events.
				ULONGLONG ullFileObject = 0;
				ULONG ulFileObjectOffset = ulThreadIdOffset + ms_usPointerSize;//ThreadkIdOffset + TTID(pointer)
				::CopyMemory(&ullFileObject, pEventData + ulFileObjectOffset, ms_usPointerSize);

				//FileKey: To determine the file name, match the value of this property to the FileObject property of a FileIo_Name event.
				ULONGLONG ullFileKey = 0;
				ULONG ulFileKeyOffset = ulFileObjectOffset + ms_usPointerSize;//FileObjectOffset + FileObject(pointer)
				::CopyMemory(&ullFileKey, pEventData + ulFileKeyOffset, ms_usPointerSize);

				//IoSize
				ULONG ulIoSize = 0;
				ULONG ulIoSizeOffset = ulFileKeyOffset + ms_usPointerSize;//FileKeyOffset + FileKey(pointer)
				::CopyMemory(&ulIoSize, pEventData + ulIoSizeOffset, sizeof(ULONG));

				/*//FileName
				std::wstring wstrFileName = L"";
				std::map<ULONGLONG, std::wstring>::iterator itFileName = ms_mFileObjectToName.find(ullFileKey);
				if (ms_mFileObjectToName.end() != itFileName)
				{
					wstrFileName = itFileName->second;
				}*/

				//存储FileKey-FileIoInfo
				std::map<ULONGLONG, FileIoInfo>::iterator itFileObject = ms_mFileObjectToFileIoInfo.find(ullFileObject);
				if (ms_mFileObjectToFileIoInfo.end() == itFileObject)
				{
					FileIoInfo fileIoInfo;
					fileIoInfo.ullFileIoSize = 0;
					fileIoInfo.ulFileIoReadCount = 0;
					fileIoInfo.ulFileIoWriteCount = 0;
					std::pair<std::map<ULONGLONG, FileIoInfo>::iterator, bool> pairRet = ms_mFileObjectToFileIoInfo.insert(std::pair<ULONGLONG, FileIoInfo>(ullFileObject, fileIoInfo));
					itFileObject = pairRet.first;
				}				
				itFileObject->second.ullFileKey = ullFileKey;
				itFileObject->second.ullFileIoSize += ulIoSize;
				//线程信息
				itThunderThread->second.ullFileIoSize += ulIoSize;
				if (67 == ucType)
				{
					itFileObject->second.ulFileIoReadCount ++;
					itThunderThread->second.ulFileIoReadCount ++;
				}
				else if (68 == ucType)
				{
					itFileObject->second.ulFileIoWriteCount ++;
					itThunderThread->second.ulFileIoWriteCount ++;
				}

				//TSDEBUG(L"FileIoGuid: ullFileObject = %I64u, ullFileKey = %I64u, ulIoSize = %u, ullFileIoSize = %I64u, ulFileIoReadCount = %u, ulFileIoWriteCount = %u, wstrFileName = %s", ullFileObject, ullFileKey, ulIoSize, itFileKey->second.ullFileIoSize, itFileKey->second.ulFileIoReadCount, itFileKey->second.ulFileIoWriteCount, wstrFileName.c_str());
			}
		}
	}
	else if (OperationType_AnalyzeEtl == ms_operationType && ::IsEqualGUID(pEvent->Header.Guid, DiskIoGuid))
	{
		//触发DiskIo事件

		UCHAR ucType = pEvent->Header.Class.Type;
		//TSDEBUG(L"DiskIoGuid: ucType = %d", ucType);
		if (EVENT_TRACE_TYPE_IO_READ == ucType || EVENT_TRACE_TYPE_IO_WRITE == ucType)
		{
			//FileObject: Match the value of this pointer to the FileObject pointer value in a FileIo_Name event to determine the file involved in the I/O operation.
			ULONGLONG ullFileObject = 0;
			ULONG ulFileObjectOffset = sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(LONGLONG);//DiskNumber(uint32)+IrpFlags(uint32)+TransferSize(uint32)+Reserved(uint32)+ByteOffset(sint64)
			::CopyMemory(&ullFileObject, pEventData + ulFileObjectOffset, ms_usPointerSize);

			//TransferSize
			ULONG ulTransferSize = 0;
			ULONG ulTransferSizeOffset = sizeof(ULONG) + sizeof(ULONG);//DiskNumber(uint32)+IrpFlags(uint32)
			::CopyMemory(&ulTransferSize, pEventData + ulTransferSizeOffset, sizeof(ULONG));

			//HighResResponseTime
			ULONGLONG ullHighResResponseTime = 0;
			ULONG ulHighResResponseTimeOffset = sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) + sizeof(LONGLONG) + ms_usPointerSize + sizeof(ULONG);//DiskNumber(uint32)+IrpFlags(uint32)+TransferSize(uint32)+Reserved(uint32)+ByteOffset(sint64)+FileObject(Pointer)+Irp(uint32)
			::CopyMemory(&ullHighResResponseTime, pEventData + ulHighResResponseTimeOffset, sizeof(ULONGLONG));

			//存储FileObject-DiskIoInfo
			std::map<ULONGLONG, DiskIoInfo>::iterator itFileObject = ms_mFileObjectToDiskIoInfo.find(ullFileObject);
			if (ms_mFileObjectToDiskIoInfo.end() == itFileObject)//没有找到对象
			{
				DiskIoInfo diskIoInfo;
				diskIoInfo.ulDiskIoReadCount = 0;
				diskIoInfo.ulDiskIoWriteCount = 0;
				diskIoInfo.ullTransferSize = 0;
				diskIoInfo.ullHighResResponseTime = 0;
				std::pair<std::map<ULONGLONG, DiskIoInfo>::iterator, bool> pairRet = ms_mFileObjectToDiskIoInfo.insert(std::pair<ULONGLONG, DiskIoInfo>(ullFileObject, diskIoInfo));
				itFileObject = pairRet.first;
			}
			itFileObject->second.ullTransferSize += ulTransferSize;
			itFileObject->second.ullHighResResponseTime += ullHighResResponseTime;
			if (EVENT_TRACE_TYPE_IO_READ == ucType)
			{
				itFileObject->second.ulDiskIoReadCount ++;
			}
			else if (EVENT_TRACE_TYPE_IO_WRITE == ucType)
			{
				itFileObject->second.ulDiskIoWriteCount ++;
			}

			//TSDEBUG(L"DiskIoGuid: ullFileObject = %I64u, ullTransferSize = %I64u, ulDiskIoReadCount = %u, ulDiskIoWriteCount = %u, ullHighResResponseTime = %I64u", ullFileObject, itFileObject->second.ullTransferSize, itFileObject->second.ulDiskIoReadCount, itFileObject->second.ulDiskIoWriteCount, itFileObject->second.ullHighResResponseTime);
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
			::CopyMemory(&temp, pEventData, sizeof(ULONG));

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

ULONG ETWConsumer::MergeEtlInfo()
{
	ms_mFileObjectToEtlInfo.clear();

	std::map<ULONGLONG, FileIoInfo>::iterator itFileObject = ms_mFileObjectToFileIoInfo.begin();
	for (; itFileObject != ms_mFileObjectToFileIoInfo.end(); ++ itFileObject)
	{
		EtlInfo etlInfo;

		//FileIoInfo
		etlInfo.ulFileIoReadCount = itFileObject->second.ulFileIoReadCount;
		etlInfo.ulFileIoWriteCount = itFileObject->second.ulFileIoWriteCount;
		etlInfo.ullFileIoSize = itFileObject->second.ullFileIoSize;

		//文件名
		etlInfo.wstrFileName = L"";
		std::map<ULONGLONG, std::wstring>::iterator itFileName = ms_mFileObjectToName.find(itFileObject->second.ullFileKey);
		if (ms_mFileObjectToName.end() != itFileName)
		{
			etlInfo.wstrFileName = itFileName->second;
		}

		//DiskIoInfo
		etlInfo.ulDiskIoReadCount = 0;
		etlInfo.ulDiskIoWriteCount = 0;
		etlInfo.ullTransferSize = 0;
		etlInfo.ullHighResResponseTime = 0;
		std::map<ULONGLONG, DiskIoInfo>::iterator itDiskIoInfo = ms_mFileObjectToDiskIoInfo.find(itFileObject->second.ullFileKey);
		if (ms_mFileObjectToDiskIoInfo.end() != itDiskIoInfo)
		{
			etlInfo.ulDiskIoReadCount = itDiskIoInfo->second.ulDiskIoReadCount;
			etlInfo.ulDiskIoWriteCount = itDiskIoInfo->second.ulDiskIoWriteCount;
			etlInfo.ullTransferSize = itDiskIoInfo->second.ullTransferSize;
			etlInfo.ullHighResResponseTime = itDiskIoInfo->second.ullHighResResponseTime;
		}

		//填充ms_mFileObjectToEtlInfo信息
		std::pair<std::map<ULONGLONG, EtlInfo>::iterator, bool> pairRet = ms_mFileObjectToEtlInfo.insert(std::pair<ULONGLONG, EtlInfo>(itFileObject->first, etlInfo));
	}

	return 0;
}

ULONG ETWConsumer::OutputEtlInfo()
{
	std::string strFileName;

	//分割线
	fprintf(ms_pFile, "\n----------------------------------------------------------------Image信息----------------------------------------------------------------\n");

	//Image相关
	for (std::vector<ImageInfo>::iterator itImageInfo = ms_vImageInfo.begin(); itImageInfo != ms_vImageInfo.end(); itImageInfo++)
	{
		strFileName.c_str();
		strFileName = CommonTool::WstringToString(itImageInfo->wstrFileName);
		fprintf(ms_pFile, "ImageSize:%I64u ; TimeDateStamp:%u ; ImageName:%s\n", itImageInfo->ullImageSize, itImageInfo->ulTimeDateStamp, strFileName.c_str());
	}

	//分割线
	fprintf(ms_pFile, "\n----------------------------------------------------------------IO信息----------------------------------------------------------------\n");

	//IO相关
	for (std::map<ULONGLONG, EtlInfo>::iterator itEtlInfo = ms_mFileObjectToEtlInfo.begin(); itEtlInfo != ms_mFileObjectToEtlInfo.end(); itEtlInfo++)
	{
		strFileName.clear();
		strFileName = CommonTool::WstringToString(itEtlInfo->second.wstrFileName);
		fprintf(ms_pFile, "FileIoReadCount:%u ; FileIoWriteCount:%u ; FileIoSize:%I64u ; DiskIoReadCount:%u ; DiskIoWriteCount:%u ; TransferSize:%I64u ; HighResResponseTime:%I64u ; FileName:%s\n", itEtlInfo->second.ulFileIoReadCount, itEtlInfo->second.ulFileIoWriteCount, itEtlInfo->second.ullFileIoSize, itEtlInfo->second.ulDiskIoReadCount, itEtlInfo->second.ulDiskIoWriteCount, itEtlInfo->second.ullTransferSize, itEtlInfo->second.ullHighResResponseTime, strFileName.c_str());
	}

	return 0;
}

