
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
	//��������
	typedef enum tagOperationType
	{
		OperationType_RunUpTime = 0,//��ȡ����ʱ��
		OperationType_AnalyzeEtl,//����ETL�ļ�
	}OperationType;
	//�洢�߳���Ϣ
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
	//�洢Image��Ϣ
	typedef struct tagImageInfo
	{
		std::wstring wstrFileName;
		ULONG ulTimeDateStamp;
		ULONGLONG ullImageSize;
	}ImageInfo;
	//�洢FileIo��Ϣ
	typedef struct tagFileIoInfo
	{
		ULONG ulFileIoReadCount;
		ULONG ulFileIoWriteCount;
		ULONGLONG ullFileIoSize;//Size of data read to or written from disk, in bytes.
		ULONGLONG ullFileKey;
	}FileIoInfo;
	//�洢DiskIo��Ϣ
	typedef struct tagDiskIoInfo
	{
		ULONG ulDiskIoReadCount;
		ULONG ulDiskIoWriteCount;
		ULONGLONG ullTransferSize;//Size of data read to or written from disk, in bytes.
		ULONGLONG ullHighResResponseTime;//The time between I/O initiation and completion as measured by the partition manager (in the KeQueryPerformanceCounter tick units).
	}DiskIoInfo;
	//�洢����ETL�ļ��õ�����Ϣ
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
	//����ETL�ļ�
	ULONG ParseTraceFile(wchar_t* wszEtlFilePath, OperationType operationType, const wchar_t* wszResultFilePath);

private:
	//Pointer to the EventCallback function that ETW calls for each event in the buffer.
	//Specify this callback if you are consuming events from a provider that used one of the TraceEvent functions to log events.
	static void WINAPI ConsumerEventCallback(PEVENT_TRACE pEvent);
	//Pointer to the BufferCallback function that receives buffer-related statistics for each buffer ETW flushes. 
	//ETW calls this callback after it delivers all the events in the buffer. This callback is optional.
	static ULONG WINAPI ConsumerBufferCallback(PEVENT_TRACE_LOGFILE pBuffer);

	//���object�������ݵĳ���
	static ULONG GetObjectDataLen(PBYTE pEventData, const wchar_t* wszExtension, ULONG ulArraySize, ULONG ulRemainingBytes);

	//�ϲ��������ݵ�ms_mFileObjectToEtlInfo
	ULONG MergeEtlInfo();
	//��ӡms_mFileObjectToEtlInfo�е�����
	ULONG OutputEtlInfo();

private:	
	static OperationType						ms_operationType;
	static FILE*								ms_pFile;					//�洢����ʱ����ļ�����
	static DWORD								ms_dwThunderProcessId;
	static USHORT								ms_usPointerSize;			// Used to determine the data size of property values that contain the Pointer or SizeT qualifier. The value will be 4 or 8.
	static ULONGLONG							ms_ullThunderCreateTime;	//Thunder����ʱ�䣬��λΪ100����

	static std::vector<ImageInfo>				ms_vImageInfo;				//�洢ImageInfo
	static std::map<ULONGLONG, std::wstring>	ms_mFileObjectToName;		//�洢FileIo�¼���FileObject-FileName
	static std::map<ULONGLONG, FileIoInfo>		ms_mFileObjectToFileIoInfo;	//�洢FileIo�¼���FileKey-FileIoInfo
	static std::map<ULONGLONG, DiskIoInfo>		ms_mFileObjectToDiskIoInfo;	//�洢DiskIo�¼���FileObject-DiskIoInfo
	static std::map<ULONGLONG, EtlInfo>			ms_mFileObjectToEtlInfo;	//�洢�ļ������ETL��Ϣ
	static std::map<ULONG, ThreadInfo>			ms_mThreadIdToInfo;			//�洢�߳�ID���߳���Ϣ
};