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

#define CVFLF_USE_SHELL_ICON           0x1000

EXTERN_C
HRESULT
WINAPI
FileClusterInformationDialog(
	__in HWND hWnd,
	__in PCWSTR pszFileName,
	__in_opt UINT Reserved,
	__in_opt PVOID Ptr
	);

EXTERN_C
HRESULT
WINAPI
FileSelectStreamDialog(
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
