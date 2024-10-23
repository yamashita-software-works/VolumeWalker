#pragma once

#include "mdichild.h"

typedef struct _MDICHILDFRAMEINIT_DIRFILES
{
	MDICHILDFRAMEINIT hdr;
	GUID Guid;
	PWSTR Path;
	LARGE_INTEGER liStartOffset;
} MDICHILDFRAMEINIT_DIRFILES; 

HRESULT LoadLayout(HWND hWnd,HWND hWndMDIClient,MDICHILDFRAMEINIT_DIRFILES **pmdiFrames,DWORD *pdwFrames);
HRESULT FreeLayout(MDICHILDFRAMEINIT_DIRFILES *mdiFrames,DWORD dwFrames);
HRESULT LoadMainFrameConfig(HWND hWnd,INT *pnCmdShow);
HRESULT SaveMainFrameConfig(HWND hWnd,HWND hWndMDIClient);
#if 0
BOOL WriteSectionRect(PCWSTR pszSection,PCWSTR pszEntry,const RECT *prc);
BOOL ReadSectionRect(PCWSTR pszSection,PCWSTR pszEntry,RECT *prc);
#endif
VOID _SetIniFileName(PCWSTR pszIniFile);
PCWSTR _GetIniFileName();

PCWSTR _GetConfigFileName();
VOID _SetConfigFileName(PCWSTR pszConfigFile);

BOOL HasValidIniFile();
