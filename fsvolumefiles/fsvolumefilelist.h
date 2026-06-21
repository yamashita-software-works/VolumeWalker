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

EXTERN_C
HRESULT
WINAPI
ClusterInformationDialog(
	__in HWND hWnd,
	__in PCWSTR pszFileName
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

#define CSDF_MAKEFULLPATH           (0x00000001)
#define CSDF_FILENAMEWITHSTREAMNAME (0x00000002)

#define S_SSD_NO_STREAM                MAKE_HRESULT(SEVERITY_SUCCESS,1,1)
#define S_SSD_DEFAULT_STREAM_ONLY      MAKE_HRESULT(SEVERITY_SUCCESS,1,2)
#define S_SSD_NAMED_STREAM_NOT_SUPPORTED MAKE_HRESULT(SEVERITY_SUCCESS,1,3)

EXTERN_C
HRESULT
WINAPI
QuickBinaryDumpDialog(
	__in HWND hWnd,
	__in_opt PCWSTR pszStreamName,
	__in_opt PBYTE pb,
	__in_opt ULONG cb,
	__in_opt DWORD dwFlags
	);
