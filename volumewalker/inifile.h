#pragma once

#include "mdichild.h"

typedef struct _MDICHILDFRAMEINIT_VOLUMEWALKER
{
	MDICHILDFRAMEINIT hdr;
	UINT ConsoleId;
	GUID Guid;
	PWSTR Path;
	LARGE_INTEGER liStartOffset;
	PWSTR ColumnLayoutString;
	PWSTR CurrentSortColumn;
} MDICHILDFRAMEINIT_VOLUMEWALKER;

HRESULT LoadLayout(HWND hWnd,HWND hWndMDIClient,MDICHILDFRAMEINIT_VOLUMEWALKER **pmdiFrames,DWORD *pdwFrames);
HRESULT FreeLayout(MDICHILDFRAMEINIT_VOLUMEWALKER *mdiFrames,DWORD dwFrames);
HRESULT LoadMainFrameConfig(HWND hWnd,INT *pnCmdShow);
HRESULT SaveMainFrameConfig(HWND hWnd,HWND hWndMDIClient);
BOOL WriteSectionRect(PCWSTR pszSection,PCWSTR pszEntry,const RECT *prc);
BOOL ReadSectionRect(PCWSTR pszSection,PCWSTR pszEntry,RECT *prc);
VOID _SetIniFileName(PCWSTR pszIniFile);
PCWSTR _GetIniFileName();
PCWSTR _GetConfigFileName();
VOID _SetConfigFileName(PCWSTR pszConfigFile);
BOOL HasValidIniFile();

EXTERN_C INT WINAPI GetConfigValue(HWND,UINT,LPCWSTR KeyName,PVOID Value,UINT cbValue);
EXTERN_C INT WINAPI GetConfigValueInt(HWND,UINT,LPCWSTR KeyName,INT Default);
EXTERN_C INT WINAPI SetConfigValue(HWND,UINT,LPCWSTR KeyName,PWSTR Value);

struct CLoadViewConfig : public ILoadViewConfig
{
	virtual VOID Release(void)
	{
		delete this;
	}

	virtual INT ReadValue(HWND hwndView,PCWSTR KeyName,PWSTR Value,UINT ValueLength)
	{
		return GetConfigValue(hwndView,0,KeyName,Value,ValueLength);
	}

	virtual INT ReadValueInt(HWND hwndView,PCWSTR KeyName,INT DefaultValue)
	{
		return GetConfigValueInt(hwndView,0,KeyName,DefaultValue);
	}
};

struct CSaveViewConfig : public ISaveViewConfig
{
	virtual VOID Release(void)
	{
		delete this;
	}

	virtual INT WriteValue(HWND hwndView,PCWSTR KeyName,PWSTR Value)
	{
		return SetConfigValue(hwndView,0,KeyName,Value);
	}

	virtual INT WriteValueInt(HWND hwndView,PCWSTR KeyName,INT Value)
	{
		return 0;//SetConfigValue(hwndView,0,KeyName,Value);
	}
};
