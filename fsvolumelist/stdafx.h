#pragma once

#include "targetver.h"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef WIN32_NO_STATUS       // defines STATUS_XXX in ntddk, now using includes DDK.
#include <ntstatus.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS      // no include STATUS_XXX in winnt.h
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <winternl.h>        // WinSDK 7.1
#include <winioctl.h>
#include <commoncontrols.h>
#include <uxtheme.h>
#include <diskguid.h>
#include <setupapi.h>
#include <sddl.h>

#include "builddefs.h"

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

#include "debug.h"
#include "mem.h"
#include "libmisc.h"
#include "common.h"
#include "common_control_helper.h"
#include "common_msg.h"
#include "simplestack.h"
#include "common_resid.h"
#include "appdef_resid.h"
#include "listhelp.h"
#include "appwindowdefs.h"
#include "interface.h"
#include "winfsctl.h"
#include "libntwdk.h"
#include "ntwin32helper.h"
#include "..\fsvolumehelp\volumehelp.h"
#include "..\fsvolumehelp\physicaldriveinformationclass.h"
#include "..\fsvolumehelp\volumedevinfostruct.h"
#include "..\fsvolumehelp\storagedevice.h"

HWND CreateVolumeInformationWindow(HWND hWnd);
HWND CreateDiskLayoutWindow(HWND hWnd);
HWND CreatePhysicalDriveInformationWindow(HWND hWnd);
HWND CreateStorageDeviceWindow(HWND hWndParent);
HWND CreateMountedDeviceWindow(HWND hWndParent);
HWND CreateVolumeListWindow(HWND hWndParent);
HWND CreatePhysicalDriveListWindow(HWND hWndParent);
HWND CreateShadowCopyListWindow(HWND hWndParent);
HWND CreateDosDriveWindow(HWND hWndParent);
HWND CreateFileSystemStatisticsWindow(HWND hWndParent);
HWND CreateSimpleHexDumpWindow(HWND hWndParent);
HWND CreateFilterDriverWindow(HWND hWndParent);

inline VOID _DrawFocusFrame(HWND hWnd,HDC hdc,RECT *prc,BOOL bDrawFocus=FALSE,COLORREF crActiveFrame=RGB(80,110,190))
{
	DrawFocusFrame(hWnd,hdc,prc,bDrawFocus,crActiveFrame);
}

HINSTANCE _GetResourceInstance();
HFONT GetGlobalFont(HWND hWnd);
HFONT GetIconFont();
HICON GetShellStockIcon(SHSTOCKICONID StockIconId);

#define DELAY_OPEN_TIMER (120) // 120ms todo:

#define _COLOR_BKGD_DIRTY_VOLUME    RGB(255,220,0)
#define _COLOR_TEXT_DIRTY_VOLUME    RGB(180,0,0)
#define _COLOR_TEXT_VIRTUALDISK     RGB(0,32,180)

