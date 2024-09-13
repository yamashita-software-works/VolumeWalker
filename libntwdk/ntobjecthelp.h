#pragma once 

#define MAX_OBJECT_NAME 260
#define MAX_OBJECT_PATH 1024 

#ifndef NTAPI
#define NTAPI __stdcall
#endif

EXTERN_C
ULONG
NTAPI
OpenObjectDirectory(
	PCWSTR DirectoryName,
	HANDLE *pHandle
	);

EXTERN_C
ULONG
NTAPI
CloseObjectDirectory(
	HANDLE Handle
	);

EXTERN_C
ULONG 
NTAPI
QueryObjectDirectory(
    HANDLE Handle,
    PULONG pulIndex,
    PWSTR ObjectName,
    ULONG ObjectNameLength,
    PWSTR TypeName,
    ULONG TypeNameLength
    );

EXTERN_C
ULONG 
NTAPI
QueryObjectDirectory_U(
    HANDLE Handle,
    PULONG pulIndex,
    UNICODE_STRING *ObjectName,
    UNICODE_STRING *TypeName,
    BOOLEAN AllocateString
    );

EXTERN_C
ULONG
NTAPI
QuerySymbolicLinkObject(
    HANDLE hRootDirectory,
    PCWSTR pszSymbolicName,
	UNICODE_STRING *pusSymbolicName OPTIONAL,
    PWSTR ObjectName,
    int cchObjectName
    );

EXTERN_C
ULONG
NTAPI
QuerySymbolicLinkObject_U(
    HANDLE hObjectDirectory,
    PCWSTR pszSymbolicLinkName,
    UNICODE_STRING *ObjectName,
    BOOLEAN AllocateNameString
    );

EXTERN_C
ULONG
NTAPI
QuerySymbolicLinkObjectName(
    PCWSTR SymbolicLinkPath,
    PCWSTR SymbolicLinkName,
    PWSTR ObjectNameBuffer,      // name buffer
    LONG ObjectNameBufferLength, // buffer length in character
    PULONG ReturnedLength        // returns length of character including terminate null 
    );

EXTERN_C
NTSTATUS
NTAPI
LookupVolumeGuidName(
    PCWSTR NtDevicePath,
    PWSTR VolumeSymbolicLink,
    ULONG cchVolumeSymbolicLink
    );

EXTERN_C
NTSTATUS
NTAPI
EnumDosDeviceTargetNames(
	HANDLE *phspa,
    PCWSTR NtDevicePath,
	ULONG Flags
    );

#define GLOBAL_DOS_DEVICE  0x0
#define LOCAL_DOS_DEVICE   0x1

#define EDDTNF_GLOBAL (GLOBAL_DOS_DEVICE)
#define EDDTNF_LOCAL  (LOCAL_DOS_DEVICE)

EXTERN_C
INT
NTAPI
GetDosDeviceTargetNamesCount(
	HANDLE hspa
	);	

typedef struct _DOSDEVICEITEMHINT
{
	ULONG  Reserved;
} DOSDEVICEITEMHINT,*PDOSDEVICEITEMHINT;

EXTERN_C
PCWSTR
NTAPI
GetDosDeviceTargetNamesItem(
	HANDLE hspa,
	int iIndex,	
	PDOSDEVICEITEMHINT ItemHint
	);	

EXTERN_C
NTSTATUS
NTAPI
FreeDosDeviceTargetNames(
	HANDLE hspa
    );

EXTERN_C
NTSTATUS
NTAPI
GetNtDeviceNameAssignedDosDrive(
    PCWSTR **DriveLetters
    );
