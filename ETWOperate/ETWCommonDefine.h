
#pragma once
#define INITGUID
#include <Guiddef.h>
#include <Wmistr.h>
#include <Evntrace.h>

#define CUSTOM_SESSION_NAME L"Trace Session of Han"

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