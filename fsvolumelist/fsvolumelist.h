#pragma once

#include "volumeconsoleid.h"

EXTERN_C
HRESULT
WINAPI
InitializeVolumeConsole(
	DWORD dwFlags
	);

#ifndef _VOLUME_CONSOLE_CREATE_PARAM
#define _VOLUME_CONSOLE_CREATE_PARAM
typedef struct _VOLUME_CONSOLE_CREATE_PARAM
{
	DWORD dwFlags;
	PWSTR pszInitialDeviceName;
	PAGE_CONTEXT *Context;
} VOLUME_CONSOLE_CREATE_PARAM,*PVOLUME_CONSOLE_CREATE_PARAM;
#endif

EXTERN_C
HWND
WINAPI
CreateVolumeConsoleWindow(
	HWND hwnd,
	UINT ConsoleType,
	PVOLUME_CONSOLE_CREATE_PARAM pParam
	);

EXTERN_C
HICON
WINAPI
GetDeviceClassIcon(
	UINT DeviceType,
	const GUID *DevClassGuid
	);

// DeviceType
enum {
	DEVICE_ICON_VOLUMESNAPSHOT = 1,
};

EXTERN_C
HRESULT
WINAPI
VirtualDiskAttachDialog(
	__in HWND hWnd,
	__in PCWSTR pszImageFileName,
	__in DWORD dwFlags
	);

// Drive Management
EXTERN_C
HRESULT
WINAPI
AssignDriveLetterDialog(
	__in HWND hWnd,
	__in PCWSTR pszNtDeviceName,
	__in PCWSTR pszDrive,
	__in_opt PWSTR pszAssignedDrive,
	__in_opt DWORD cchAssignedDrive,
	__in DWORD dwFlags
	);

EXTERN_C
HRESULT
WINAPI
RemoveDriveLetterDialog(
	__in HWND hWnd,
	__in PCWSTR pszNtDeviceName,
	__in PCWSTR pszDrive,
	__in_opt PWSTR pszRemovedDrive,
	__in_opt DWORD cchRemovedDrive,
	__in DWORD dwFlags
	);

#define RDDF_CHOOSE_DRIVE_UI  (0x1000)
