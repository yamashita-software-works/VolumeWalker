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

#include "builddefs.h"

#include "common_control_helper.h"

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

#include "debug.h"
#include "mem.h"
#include "libmisc.h"
#include "..\libntwdk\libntwdk.h"
#include "..\libntwdk\ntwin32helper.h"
#include "..\inc\common.h"
#include "..\inc\common_control_helper.h"
#include "..\inc\simplestack.h"
#include "..\fsvolumeinfo\volumeinfo.h"
#include "..\inc\common_resid.h"
#include "..\inc\appdef_resid.h"

#include "appwindowdefs.h"

#define  _ASSERT ASSERT

HINSTANCE _GetResourceInstance();

enum {
	TitleNone = 0,                // 0
	TitleName,                    // 1
	TitleAttributes,              // 2
	TitleLastWriteTime,           // 3
	TitleCreationTime,            // 4
	TitleLastAccessTime,          // 5
	TitleChangeTime,              // 6
	TitleLastWriteTimeDirEntry,   // 7
	TitleCreationTimeDirEntry,    // 8
	TitleLastAccessTimeDirEntry,  // 9
	TitleChangeTimeDirEntry,      // 10
	TitleEndOfFile,               // 11
	TitleAllocationSize,          // 12
	TitleEndOfFileDirEntry,       // 13
	TitleAllocationSizeDirEntry,  // 14
	TitleNumberOfHardLink,        // 15
	TitleDirectory,               // 16
	TitleDeletePending,           // 17
	TitleShortName,               // 18
	TitleExtension,               // 19
	TitleEAData,                  // 20
	TitleObjectId,                // 21
	TitleBirthVolumeId,           // 22
	TitleBirthObjectId,           // 23
	TitleDomainId,                // 24
	TitleFileId,                  // 25
	TitleLocation,                // 26
	TitleWofItem,                 // 27
	TitleCount,
	TitleTableSize = TitleCount,
};

HIMAGELIST GetGlobalShareImageList();
INT GetUpDirImageIndex();

#include "interface.h"

#include "appwindowdefs.h"
HWND CreateVolumeInformationWindow(HWND hWnd);
HWND CreateDiskLayoutWindow(HWND hWnd);
HWND CreatePhysicalDriveInformationWindow(HWND hWnd);
HWND CreateStorageDeviceWindow(HWND hWndParent);
HWND CreateMountedDeviceWindow(HWND hWndParent);
HWND CreateVolumeListWindow(HWND hWndParent);
HWND CreatePhysicalDriveListWindow(HWND hWndParent);
HWND CreateShadowCopyListWindow(HWND hWndParent);
HWND CreateDosDriveWindow(HWND hWndParent);

//
// GetDisp Column Hanlder
//
template< class T > struct COLUMN_HANDLER_PROC
{
	LRESULT (T::*pfn)(UINT,NMLVDISPINFO*);
};

template< class T > struct COLUMN_HANDLER_DEF
{
	int colid;
	LRESULT (T::*pfn)(UINT,NMLVDISPINFO*);
};

#define COL_HANDLER_MAP_DEF(colid,pfn) { colid,pfn }

//
// Compare Column Handler
//
template< class T, class C > struct COMPARE_HANDLER_PROC
{
	int (T::*proc)(C *p1,C *p2, const void *p);
};

template< class T, class C > struct COMPARE_HANDLER_PROC_DEF
{
	int colid;
	int (T::*proc)(C *p1,C *p2, const void *p);
};

#define _COMP(n1,n2)  (n1 < n2 ? -1 : n1 > n2 ? 1 : 0)

// compare helper
_inline bool _compare_pointer_nullstring(PCWSTR psz1,PCWSTR psz2,int direction,int& result)
{
	if( psz1 != NULL && psz2 == NULL )
		return result = (-1 * direction), true;
	else if( psz1 == NULL && psz2 != NULL )
		return result = (1  * direction), true;
	else if( psz1 == NULL && psz2 == NULL )
		return result = 0, true;

	if( *psz1 != L'\0' && *psz2 == L'\0' )
		return result = (-1 * direction), true;
	else if( *psz1 == L'\0' && *psz2 != L'\0' )
		return result = (1  * direction), true;

	return false;
}

_inline int _compare_pointer_string(PCWSTR psz1,PCWSTR psz2,int direction)
{
	int result;
	if( _compare_pointer_nullstring(psz1,psz2,direction,result) )
		return result;
	return StrCmp(psz1,psz2);
}

_inline int _compare_pointer_string_logical(PCWSTR psz1,PCWSTR psz2,int direction)
{
	int result;
	if( _compare_pointer_nullstring(psz1,psz2,direction,result) )
		return result;
	return StrCmpLogicalW(psz1,psz2);
}

_inline bool _compare_pointer_ansi_nullstring(PCSTR psz1,PCSTR psz2,int direction,int& result)
{
	if( psz1 != NULL && psz2 == NULL )
		return result = (-1 * direction), true;
	else if( psz1 == NULL && psz2 != NULL )
		return result = (1  * direction), true;
	else if( psz1 == NULL && psz2 == NULL )
		return result = 0, true;

	if( *psz1 != '\0' && *psz2 == '\0' )
		return result = (-1 * direction), true;
	else if( *psz1 == '\0' && *psz2 != '\0' )
		return result = (1  * direction), true;

	return false;
}

_inline int _compare_pointer_ansi_string(PCSTR psz1,PCSTR psz2,int direction)
{
	int result;
	if( _compare_pointer_ansi_nullstring(psz1,psz2,direction,result) )
		return result;
	return StrCmpA(psz1,psz2);
}

template <class T>struct SORT_PARAM
{
	T* pThis;
	UINT id;
	int direction;
	int directory_align;
};

inline VOID _DrawFocusFrame(HWND hWnd,HDC hdc,RECT *prc,BOOL bDrawFocus=FALSE,COLORREF crActiveFrame=RGB(80,110,190))
{
	DrawFocusFrame(hWnd,hdc,prc,bDrawFocus,crActiveFrame);
}

HFONT GetGlobalFont(HWND hWnd,BOOL bCreate);
HICON GetShellStockIcon(SHSTOCKICONID StockIconId);

#define DELAY_OPEN_TIMER (120) // 120ms todo:
