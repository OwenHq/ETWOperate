
#pragma once
#define INITGUID
#include <WTypes.h>
#include <Guiddef.h>
#include <Wmistr.h>
#include <Evntrace.h>

#define CUSTOM_SESSION_NAME L"Trace Session of Han"

// Identifies the event type within the MyCategoryGuid category 
// of events to be logged. This is the same value as the EventType 
// qualifier that is defined in the event type MOF class for one of 
// the CategoryGuid category of events.
#define ETWPROVIDER_PROCESS_ENTERWMAIN	1
#define ETWPROVIDER_PROCESS_SHOWVIEW	2

//The following values define the possible class GUIDs for kernel events that an NT Kernel Logger session can trace. 
//http://msdn.microsoft.com/en-us/library/windows/desktop/aa364085(v=vs.85).aspx

DEFINE_GUID ( /* 3d6fa8d4-fe05-11d0-9dda-00c04fd7ba7c */
			 DiskIoGuid,
			 0x3d6fa8d4,
			 0xfe05,
			 0x11d0,
			 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
			 );

DEFINE_GUID ( /* ce1dbfb4-137e-4da6-87b0-3f59aa102cbc */
			 PerfInfoGuid,
			 0xce1dbfb4,
			 0x137e,
			 0x4da6,
			 0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc
			 );

DEFINE_GUID ( /* 2cb15d1d-5fc1-11d2-abe1-00a0c911f518 */
			 ImageLoadGuid,
			 0x2cb15d1d,
			 0x5fc1,
			 0x11d2,
			 0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18
			 );

DEFINE_GUID ( /* 3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c */
			 ProcessGuid,
			 0x3d6fa8d0,
			 0xfe05,
			 0x11d0,
			 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
			 );

DEFINE_GUID ( /* 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c */
			 ThreadGuid,
			 0x3d6fa8d1,
			 0xfe05,
			 0x11d0,
			 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
			 );

DEFINE_GUID ( /* 90cbdc39-4a3e-11d1-84f4-0000f80464e3 */
			 FileIoGuid,
			 0x90cbdc39,
			 0x4a3e,
			 0x11d1,
			 0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
			 );

DEFINE_GUID ( /* def2fe46-7bd6-4b80-bd94-f57fe20d0ce3 */
			 StackWalkGuid,
			 0xdef2fe46,
			 0x7bd6,
			 0x4b80,
			 0xbd, 0x94, 0xf5, 0x7f, 0xe2, 0x0d, 0x0c, 0xe3
			 );

DEFINE_GUID ( /* 3d6fa8d3-fe05-11d0-9dda-00c04fd7ba7c */
			 PageFaultGuid,
			 0x3d6fa8d3,
			 0xfe05,
			 0x11d0,
			 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
			 );


// GUID that identifies the category of events that the provider can log. 
// The GUID is also used in the event MOF class. 
// Remember to change this GUID if you copy and paste this example.
// {56C6CB14-3AC4-47f5-9AD1-9EF7E728EB0F}
static const GUID CategoryGuid = 
{ 0x56c6cb14, 0x3ac4, 0x47f5, { 0x9a, 0xd1, 0x9e, 0xf7, 0xe7, 0x28, 0xeb, 0xf } };

// GUID that identifies the provider that you are registering.
// The GUID is also used in the provider MOF class. 
// Remember to change this GUID if you copy and paste this example.
// {C5F097E2-6D10-4820-81BE-04A15B6F8DBC}
static const GUID ProviderGuid = 
{ 0xc5f097e2, 0x6d10, 0x4820, { 0x81, 0xbe, 0x4, 0xa1, 0x5b, 0x6f, 0x8d, 0xbc } };

// GUID used as the value for EVENT_DATA.ID.
// {4FBAE9A1-A8FD-4687-A0B3-F813E210E613}
static const GUID TempId = 
{ 0x4fbae9a1, 0xa8fd, 0x4687, { 0xa0, 0xb3, 0xf8, 0x13, 0xe2, 0x10, 0xe6, 0x13 } };