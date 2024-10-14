#pragma once

#include "targetver.h"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef WIN32_NO_STATUS        // defines STATUS_XXX in ntddk, now using includes DDK.
#include <ntstatus.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS       // no include STATUS_XXX in winnt.h
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <winternl.h> // WinSDK 7.1
#include <winioctl.h>
#include <commoncontrols.h>
#include <uxtheme.h>
#include <commdlg.h>

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

#include "libntwdk.h"
#include "debug.h"
#include "mem.h"
#include "libmisc.h"
#include "dparray.h"
#include "simplestack.h"
#include "common_control_helper.h"
#include "simplevalarray.h"
#include "common.h"
#include "common_resid.h"
#include "appdef_resid.h"
#include "common_msg.h"
#include "listhelp.h"
#include "volumeconsoleid.h"
#include "stringbuffer.h"

#define  _ASSERT ASSERT

HINSTANCE _GetResourceInstance();

#include "fsfilelib.h"
#include "fileitem.h"

HICON SetWindowIcon(HWND hWnd,SHSTOCKICONID ssii);
VOID DrawFocusFrame(HWND hWnd,HDC hdc,RECT *prc,BOOL bDrawFocus=FALSE,COLORREF crBorder=RGB(80,110,190));
PWSTR GetIniFilePath();
HFONT GetGlobalFont(HWND hWnd);
HFONT GetIconFont();

enum {
    COLUMN_None=0,
    COLUMN_Name,            /* 1 */
    COLUMN_LastWriteTime,   /* 2 */
    COLUMN_CreationTime,    /* 3 */
    COLUMN_LastAccessTime,  /* 4 */
    COLUMN_ChangeTime,      /* 5 */
    COLUMN_EndOfFile,       /* 6 */
    COLUMN_AllocationSize,  /* 7 */
    COLUMN_FileAttributes,  /* 8 */
    COLUMN_FileNameLength,  /* 9 */
    COLUMN_EaSize,          /* 10 */
    COLUMN_FileIndex,       /* 11 */
    COLUMN_FileId,          /* 12 */
    COLUMN_ShortName,       /* 13 */
    COLUMN_Extension,       /* 14 */
    COLUMN_Path,            /* 15 */
    COLUMN_Lcn,             /* 16 */
    COLUMN_Date,            /* 17 */
    COLUMN_Reason,          /* 18 */
    COLUMN_Usn,             /* 19 */
    COLUMN_Frn,             /* 20 */
    COLUMN_ParentFrn,       /* 21 */
    COLUMN_PhysicalDriveNumber, /* 22 */
    COLUMN_PhysicalDriveOffset, /* 23 */
    COLUMN_MaxItem,
    COLUMN_MaxCount=COLUMN_MaxItem,
};
