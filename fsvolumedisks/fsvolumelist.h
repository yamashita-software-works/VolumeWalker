#pragma once

#include "volumeconsoleid.h"

EXTERN_C
HRESULT
WINAPI
InitializeVolumeConsole(
	DWORD dwFlags
	);

#define VOLUME_DLL_FLAG_ENABLE_DARK_MODE 0x1

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

EXTERN_C
HRESULT
WINAPI
CreateMountPointDialog(
	__in HWND hWnd,
	__in PCWSTR pszVolumeName,
	__in PCWSTR pszPointPointFolderPath,
	__in DWORD dwFlags
	);

#define MPDF_APPENDPREFIX_NT (0x1)

//////////////////////////////////////////////////////////////////////////////
//
// Create Disk Image File
//

typedef enum {
	CDIFCB_Progress=0,
	CDIFCB_Start,
	CDIFCB_End,
	CDIFCB_Confirm=98,
	CDIFCB_Error=99,
} CALLBACKREASON;

typedef BOOL (CALLBACK *CREATEDISKIMAGEFILECALLBACK)(
	CALLBACKREASON CallReason,
	LARGE_INTEGER ProcessedSize,
	LARGE_INTEGER DiskSize,
	DWORD ErrorCode,
	ULONG_PTR Context
	);

HRESULT 
WINAPI
CreateDiskImageFile(
	PCWSTR pszPhysicalDrive, // "\\.\PhysicalDrive0"
	PCWSTR pszImageFilePath, // "C:\foo\bar\disk_image.img"
	ULONG reserved,
	PVOID reserved_ptr,
	CREATEDISKIMAGEFILECALLBACK reserved_callback,
	ULONG_PTR reserved_context
	);

EXTERN_C
HRESULT
WINAPI
CreateDiskImageFileDialog(
	__in HWND hWnd,
	__in PWSTR Reserved1,
	__in PWSTR Reserved2,
	__in PVOID ReservedPtr,
	__inout_opt HANDLE *phHandle // Reserved
	);

EXTERN_C
HRESULT
WINAPI
WriteBackDiskImageFileDialog(
	__in HWND hWnd,
	__in PWSTR Reserved1,
	__in PWSTR Reserved2,
	__in PVOID ReservedPtr,
	__inout_opt	 HANDLE *phHandle // Reserved
	);
