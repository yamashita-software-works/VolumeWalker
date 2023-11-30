#pragma once

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef BOOL
#define BOOL INT
#endif

#ifndef HMODULE
#define HMODULE HANDLE
#endif

EXTERN_C
BOOL
WINAPI
GetVolumeNameSet(
	PCWSTR pszInputName,
	PWSTR  pszDevicePath,ULONG cchDevicePath,
	PWSTR  pszVolumeName,ULONG cchVolumeName,
	PWSTR  pszDosDrive,ULONG cchDosDrive
	);

EXTERN_C
ULONG
WINAPI
LookupVolumeNameByNtDeviceName(
	__in PCWSTR DevicePath,
	__inout PWSTR *pRootPos,
	__out PWSTR VolumeSymbolicLink,
	__in ULONG cchVolumeSymbolicLink
	);

typedef struct _VOLUME_NAME_STRING
{
	WCHAR VolumeGuidString[45];   // "Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
	const WCHAR *NtVolumeName;    // "\Device\HarddiskVolumeX","\Device\CdRomX" etc
} VOLUME_NAME_STRING;

typedef struct _VOLUME_NAME_STRING_ARRAY
{
	ULONG Count;
	VOLUME_NAME_STRING Volume[1];
} VOLUME_NAME_STRING_ARRAY;

EXTERN_C
ULONG
WINAPI
EnumHarddiskVolumes(
	__inout VOLUME_NAME_STRING_ARRAY **VolumeNames // point to pointer to array of VOLUME_NAME_STRING
	);

EXTERN_C
ULONG
WINAPI
EnumVolumeNames(
	__inout VOLUME_NAME_STRING_ARRAY **VolumeNames // point to pointer to array of VOLUME_NAME_STRING
	);

EXTERN_C
ULONG
WINAPI
FreeVolumeNames(
	__inout VOLUME_NAME_STRING_ARRAY *VolumeNames
	);

typedef struct PHYSICALDRIVE_NAME_STRING
{
	const WCHAR *PhysicalDriveName;  // "PhysicalDrive0"
	const WCHAR *DevicePath;         // "\Device\Harddisk0\DR0"
} PHYSICALDRIVE_NAME_STRING;

typedef struct _PHYSICALDRIVE_NAME_STRING_ARRAY
{
	ULONG Count;
	PHYSICALDRIVE_NAME_STRING Drive[1];
} PHYSICALDRIVE_NAME_STRING_ARRAY;

EXTERN_C
ULONG
WINAPI
EnumPhysicalDriveNames(
	__inout PHYSICALDRIVE_NAME_STRING_ARRAY **PhysicalDriveNames // point to pointer to array of PHYSICALDRIVE_NAME_STRING_
	);

EXTERN_C
ULONG
WINAPI
FreePhysicalDriveNames(
	__inout PHYSICALDRIVE_NAME_STRING_ARRAY *PhysicalDriveNames
	);

//
// FltLib.dll
//
EXTERN_C
HRESULT
WINAPI
LoadFltLibDll(
	IN HMODULE *phModule OPTIONAL
	);

EXTERN_C
HRESULT
WINAPI
UnloadFltLibDll(
	IN HMODULE hModule OPTIONAL
	);

EXTERN_C
HRESULT
WINAPI
GetVolumeDosName(
    IN LPCWSTR  lpVolumeName,
    IN OUT LPWSTR  lpDosName,
    IN DWORD  dwDosNameBufferSize
    );

//
// miscellaneous
//
EXTERN_C
INT
WINAPI
SortVolumeNameCompareProc(
	PCWSTR pszVolName1,
	PCWSTR pszVolName2
	);

