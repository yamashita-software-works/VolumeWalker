// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <locale.h>

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <commdlg.h>
#include <winternl.h> // WinSDK 7.1

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

#include "debug.h"
#include "mem.h"
#include "libmisc.h"
#include "menu.h"
#include "libntwdk.h"
#include "ntvolumenames.h"
#include "ntobjecthelp.h"
#include "..\libmisc\libmisc.h"
#include "..\inc\common.h"
#include "..\inc\common_resid.h"
#include "..\inc\appdef_resid.h"
#include "..\inc\simplevalarray.h"
#include "..\fsvolumehelp\volumehelp.h"
#include "..\fsvolumelist\fsvolumelist.h"
#include "..\fsvolumecontents\fsvolumecontents.h"
#include "common_msg.h"


#define SetRedraw(h,f)	SendMessage(h,WM_SETREDRAW,(WPARAM)f,0)

#define GETINSTANCE(hWnd)   (HINSTANCE)GetWindowLongPtr(hWnd,GWLP_HINSTANCE)
#define GETCLASSBRUSH(hWnd) (HBRUSH)GetClassLongPtr(hWnd,GCLP_HBRBACKGROUND)

HINSTANCE _GetResourceInstance();
HWND _GetMainWnd();

inline HICON GetShellStockIcon(SHSTOCKICONID StockIconId)
{
	SHSTOCKICONINFO sii = {0};
	sii.cbSize = sizeof(sii);
	SHGetStockIconInfo(StockIconId,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
	return sii.hIcon;
}

inline HICON SetFrameIcon(HWND hWnd,HICON hIcon)
{
	return (HICON)SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
}
