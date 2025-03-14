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
#include <commctrl.h>
#include <uxtheme.h>
#include <winternl.h> // WinSDK 7.1

#pragma warning(disable : 4995)
#include <atlbase.h>
#include <atlcoll.h>

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

#include "debug.h"
#include "mem.h"
#include "libmisc.h"
#include "menu.h"
#include "multisz.h"
#include "simplevalarray.h"
#include "libntwdk.h"
#include "libmisc.h"
#include "common_msg.h"
#include "common_control_helper.h"
#include "common.h"
#include "volumeconsoledef.h"
#include "common_resid.h"
#include "common_resid_appdef.h"
#include "ntvolumenames.h"
#include "..\fsvolumehelp\volumehelp.h"
#include "..\fsvolumelist\fsvolumelist.h"
#undef HMODULE // for switch WDK header to Win32

#include "..\build_switch.inc"

#define SetRedraw(h,f)	SendMessage(h,WM_SETREDRAW,(WPARAM)f,0)

#define GETINSTANCE(hWnd)   (HINSTANCE)GetWindowLongPtr(hWnd,GWLP_HINSTANCE)
#define GETCLASSBRUSH(hWnd) (HBRUSH)GetClassLongPtr(hWnd,GCLP_HBRBACKGROUND)

HINSTANCE _GetResourceInstance();
HWND _GetMainWnd();

inline HICON SetFrameIcon(HWND hWnd,HICON hIcon)
{
	return (HICON)SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
}

#if !(_ENABLE_DARK_MODE_TEST)
inline BOOL _IsDarkModeEnabled() { return FALSE; }
#endif
