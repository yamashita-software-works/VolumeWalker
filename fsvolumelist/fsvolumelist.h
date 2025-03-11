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
	PWSTR pszReserved;
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
