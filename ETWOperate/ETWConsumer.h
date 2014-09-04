
#pragma once
#include "ETWCommonDefine.h"
#include <Wbemcli.h>
#include <comutil.h>
#include <In6addr.h>
#include <map>
#include <vector>


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
	//操作类型
	typedef enum tagOperationType
	{
		OperationType_RunUpTime = 0,//获取启动时间
		OperationType_AnalyzeEtl,//分析ETL文件
	}OperationType;
	//存储线程信息
	typedef struct tagThreadInfo
	{
		//FileIo
		ULONG ulFileIoReadCount;
		ULONG ulFileIoWriteCount;
		ULONGLONG ullFileIoSize;
		//DiskIo
		ULONG ulDiskIoReadCount;
		ULONG ulDiskIoWriteCount;
		ULONGLONG ullDiskIoSize;
	}ThreadInfo;
	//存储Image信息
	typedef struct tagImageInfo
	{
		std::wstring wstrFileName;
		ULONG ulTimeDateStamp;
		ULONGLONG ullImageSize;
	}ImageInfo;
	//存储FileIo信息
	typedef struct tagFileIoInfo
	{
		ULONG ulFileIoReadCount;
		ULONG ulFileIoWriteCount;
		ULONGLONG ullFileIoSize;//Size of data read to or written from disk, in bytes.
		ULONGLONG ullFileKey;
	}FileIoInfo;
	//存储DiskIo信息
	typedef struct tagDiskIoInfo
	{
		ULONG ulDiskIoReadCount;
		ULONG ulDiskIoWriteCount;
		ULONGLONG ullTransferSize;//Size of data read to or written from disk, in bytes.
		ULONGLONG ullHighResResponseTime;//The time between I/O initiation and completion as measured by the partition manager (in the KeQueryPerformanceCounter tick units).
	}DiskIoInfo;
	//存储分析ETL文件得到的信息
	typedef struct tagEtlInfo
	{
		std::wstring wstrFileName;//Full path to the file, not including the drive letter.

		//FileIo
		ULONG ulFileIoReadCount;
		ULONG ulFileIoWriteCount;
		ULONGLONG ullFileIoSize;

		//DiskIo
		ULONG ulDiskIoReadCount;
		ULONG ulDiskIoWriteCount;
		ULONGLONG ullTransferSize;//Size of data read to or written from disk, in bytes.
		ULONGLONG ullHighResResponseTime;//The time between I/O initiation and completion as measured by the partition manager (in the KeQueryPerformanceCounter tick units).
	}EtlInfo;
public:
	ETWConsumer();
	~ETWConsumer();

public:
	//解释ETL文件
	ULONG ParseTraceFile(wchar_t* wszEtlFilePath, OperationType operationType, const wchar_t* wszResultFilePath);

private:
	//Pointer to the EventCallback function that ETW calls for each event in the buffer.
	//Specify this callback if you are consuming events from a provider that used one of the TraceEvent functions to log events.
	static void WINAPI ConsumerEventCallback(PEVENT_TRACE pEvent);
	//Pointer to the BufferCallback function that receives buffer-related statistics for each buffer ETW flushes. 
	//ETW calls this callback after it delivers all the events in the buffer. This callback is optional.
	static ULONG WINAPI ConsumerBufferCallback(PEVENT_TRACE_LOGFILE pBuffer);

	//获得object类型数据的长度
	static ULONG GetObjectDataLen(PBYTE pEventData, const wchar_t* wszExtension, ULONG ulArraySize, ULONG ulRemainingBytes);

	//合并所有数据到ms_mFileObjectToEtlInfo
	ULONG MergeEtlInfo();
	//打印ms_mFileObjectToEtlInfo中的数据
	ULONG OutputEtlInfo();

private:	
	static OperationType						ms_operationType;
	static FILE*								ms_pFile;					//存储启动时间的文件对象
	static DWORD								ms_dwThunderProcessId;
	static USHORT								ms_usPointerSize;			// Used to determine the data size of property values that contain the Pointer or SizeT qualifier. The value will be 4 or 8.
	static ULONGLONG							ms_ullThunderCreateTime;	//Thunder启动时间，单位为100纳秒

	static std::vector<ImageInfo>				ms_vImageInfo;				//存储ImageInfo
	static std::map<ULONGLONG, std::wstring>	ms_mFileObjectToName;		//存储FileIo事件的FileObject-FileName
	static std::map<ULONGLONG, FileIoInfo>		ms_mFileObjectToFileIoInfo;	//存储FileIo事件的FileKey-FileIoInfo
	static std::map<ULONGLONG, DiskIoInfo>		ms_mFileObjectToDiskIoInfo;	//存储DiskIo事件的FileObject-DiskIoInfo
	static std::map<ULONGLONG, EtlInfo>			ms_mFileObjectToEtlInfo;	//存储文件对象和ETL信息
	static std::map<ULONG, ThreadInfo>			ms_mThreadIdToInfo;			//存储线程ID和线程信息
};