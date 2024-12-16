#pragma once

//
// include for win32 build
//
#ifndef _NTIFS_
#ifndef _WINTERNL_

typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#endif
#endif

#include "ntpathcomponent.h"
#include "ntnativehelp.h"
#include "ntvolumehelp.h"

//
// WDK definitions for Win32 build compatible
//

typedef struct _FS_FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FS_FILE_BOTH_DIR_INFORMATION, *PFS_FILE_BOTH_DIR_INFORMATION;

typedef struct _FS_FILE_ID_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FS_FILE_ID_BOTH_DIR_INFORMATION, *PFS_FILE_ID_BOTH_DIR_INFORMATION;

typedef struct _FS_FILE_ID_GLOBAL_TX_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    LARGE_INTEGER FileId;
    GUID LockingTransactionId;
    ULONG TxInfoFlags;
    WCHAR FileName[1];
} FS_FILE_ID_GLOBAL_TX_DIR_INFORMATION, *PFS_FILE_ID_GLOBAL_TX_DIR_INFORMATION;

#ifndef FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_WRITELOCKED
#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_WRITELOCKED         0x00000001
#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_VISIBLE_TO_TX       0x00000002
#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_VISIBLE_OUTSIDE_TX  0x00000004
#endif

#define USN_VERSION_2 2
#define USN_VERSION_3 3

#ifndef FILE_ATTRIBUTE_INTEGRITY_STREAM
#define FILE_ATTRIBUTE_INTEGRITY_STREAM      0x00008000
#endif

#ifndef FILE_ATTRIBUTE_NO_SCRUB_DATA
#define FILE_ATTRIBUTE_NO_SCRUB_DATA         0x00020000
#endif

#ifndef FILE_ATTRIBUTE_EA
#define FILE_ATTRIBUTE_EA                    0x00040000
#endif

#ifndef FILE_ATTRIBUTE_PINNED
#define FILE_ATTRIBUTE_PINNED                0x00080000
#endif

#ifndef FILE_ATTRIBUTE_UNPINNED
#define FILE_ATTRIBUTE_UNPINNED              0x00100000
#endif

#ifndef FILE_ATTRIBUTE_RECALL_ON_OPEN
#define FILE_ATTRIBUTE_RECALL_ON_OPEN        0x00040000
#endif

#ifndef FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS 0x00400000
#endif

#ifndef FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL
#define FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL   0x20000000  
#endif

EXTERN_C
ULONG
__cdecl
DbgPrint (
    __in_z __drv_formatString(printf) PCSTR Format,
    ...
    );

#ifndef _DBGPRINT
#ifdef _DEBUG
#define _DBGPRINT DbgPrint
#else
#define _DBGPRINT __noop
#endif
#endif

//  OBJECT_INFORMATION_CLASS compatible
#define ObjectNameInformation      ((OBJECT_INFORMATION_CLASS)1)
#define ObjectAllTypesInformation  ((OBJECT_INFORMATION_CLASS)3)
#define ObjectHandleInformation    ((OBJECT_INFORMATION_CLASS)4)

typedef struct _NT_FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} NT_FILE_BASIC_INFORMATION, *PNT_FILE_BASIC_INFORMATION;

typedef struct _NT_FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} NT_FILE_STANDARD_INFORMATION, *PNT_FILE_STANDARD_INFORMATION;

EXTERN_C
NTSTATUS
NTAPI
SetFileBasicInformation(
	HANDLE hFile,
	NT_FILE_BASIC_INFORMATION *pfbi
	);

EXTERN_C
NTSTATUS
NTAPI
QueryAttributesFile(
	HANDLE hRoot,
	PCWSTR pszFileName,
	NT_FILE_BASIC_INFORMATION *FileBasicInfo
	);

EXTERN_C
NTSTATUS
NTAPI
GetFileSizeByHandle(
	HANDLE hFile,
	LARGE_INTEGER *pSize,
	LARGE_INTEGER *pAllocationSize
	);

EXTERN_C
NTSTATUS
NTAPI
GetFileId(
	HANDLE hFile,
	LARGE_INTEGER *pFildId
	);

#ifndef _WINDOWS_
// win32 compatible SYSTEMTIME structure
typedef struct _SYSTEMTIME {
  USHORT wYear;
  USHORT wMonth;
  USHORT wDayOfWeek;
  USHORT wDay;
  USHORT wHour;
  USHORT wMinute;
  USHORT wSecond;
  USHORT wMilliseconds;
}SYSTEMTIME, *PSYSTEMTIME;
#endif

EXTERN_C
VOID
NTAPI
LocalSystemTimeToTimeInteger(
	SYSTEMTIME *pst,
	LARGE_INTEGER *pliTime
	);

EXTERN_C
VOID
NTAPI
TimeIntegerToLocalSystemTime(
	LARGE_INTEGER *pnSysTime,
	SYSTEMTIME *ps
	);

EXTERN_C
VOID
NTAPI
SystemTimeToTimeInteger(
	SYSTEMTIME *pst,
	LARGE_INTEGER *pliTime
	);

EXTERN_C
VOID
NTAPI
TimeIntegerToSystemTime(
	LARGE_INTEGER *pnSysTime,
	SYSTEMTIME *pst
	);

EXTERN_C
VOID
NTAPI
SecondsSince1970ToTime(
    IN ULONG ElapsedSeconds,
    OUT PLARGE_INTEGER Time
	);
