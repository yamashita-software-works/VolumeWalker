#pragma once

EXTERN_C
HWND
WINAPI
CreateVolumeFileList(
	HWND hwnd,
	UINT ConsoleId,
	DWORD dwOptionFlags,
	LPARAM lParam
	);

#define VOLFILES_FLG_USE_SHELL_ICON    0x1000
#define VOLFILES_FLG_USE_PROXY_OPEN    0x8000

typedef struct _CHOOSE_CLUSTER_LOCATION
{
	ULONG Flags;
	LARGE_INTEGER Vcn;
	LARGE_INTEGER Lcn;
	LARGE_INTEGER Count;
	LARGE_INTEGER Offset;
	LARGE_INTEGER PhysicalDriveOffset;
	WCHAR szVolumeName[32];
	WCHAR szPhysicalDrive[32];
} CHOOSE_CLUSTER_LOCATION, *PCHOOSE_CLUSTER_LOCATION;

EXTERN_C
HRESULT
WINAPI
ChooseClusterLocationDialog(
	__in HWND hWnd,
	__in PCWSTR pszFileName,
	__in_opt UINT Reserved,
	__in_opt CHOOSE_CLUSTER_LOCATION *ClusterInfo
	);

EXTERN_C
HRESULT
WINAPI
ChooseStreamDialog(
	__in HWND hWnd,
	__in PCWSTR pszFilePath,
	__inout PWSTR *pszFileStreamName,
	__inout_opt LPDWORD cchFileStreamName,
	__in_opt DWORD dwFlags
	);

#define FSSDF_MAKEFULLPATH           (0x00000001)
#define FSSDF_FILENAMEWITHSTREAMNAME (0x00000002)

#define S_SSD_NO_STREAM                MAKE_HRESULT(SEVERITY_SUCCESS,1,1)
#define S_SSD_DEFAULT_STREAM_ONLY      MAKE_HRESULT(SEVERITY_SUCCESS,1,2)
