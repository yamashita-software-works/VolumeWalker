#pragma once

#include <commctrl.h>

#include "mem.h"

INT
APIENTRY
_libmisc_initialize(
    void
    );

INT
APIENTRY
_libmisc_uninitialize(
    void
    );

VOID
APIENTRY
_libmisc_set_resource_handle(
    HMODULE hModule
    );

VOID
APIENTRY
_libmisc_set_langage_id(
    DWORD LangId
    );

HMODULE
APIENTRY
_libmisc_get_resource_handle(
    void
    );

DWORD
APIENTRY
_libmisc_get_langage_id(
    void
    );

DWORD
APIENTRY
_GetOSVersion(
    void
    );

DWORD
APIENTRY
_GetOSBuildNumber(
	void
	);

HRESULT
WINAPI
InitializeLibMisc(
    HMODULE hModule,
    DWORD LangId
    );

HRESULT
WINAPI
UninitializeLibMisc(
    VOID
    );

int
WINAPI
_LoadStringResource(
    UINT uStringId,
    PTSTR *pStringPointer
    );

PWSTR
WINAPI
_AllocLoadString(
    LPCWSTR StringId
    );

VOID
WINAPI
_EnableVisualThemeStyle(
    HWND hWnd
    );

SIZE
WINAPI
GetLogicalPixels(
    HWND hWnd
    );

INT
WINAPI
GetLogicalPixelsX(
    HWND hWnd
    );

INT
WINAPI
GetLogicalPixelsY(
    HWND hWnd
    );

#define DPI_SIZE_CY(n)  (int)((96.0/((double)GetLogicalPixelsY(NULL))) * (double)n)
#define DPI_SIZE_CX(n)  (int)((96.0/((double)GetLogicalPixelsX(NULL))) * (double)n)

int
WINAPI
_DPI_Adjust_X(
    int x
    );

int
WINAPI
_DPI_Adjust_Y(
    int y
    );

void
WINAPI
_DPI_Adjust_XY(
    int *px,
    int *py
    );

DWORD
WINAPI
_GetDesktopWorkArea(
    HWND hwnd,
    RECT *prc
    );

BOOL
WINAPI
_SetProcessDPIAware(
    VOID
    );

#ifndef DPI_AWARENESS_CONTEXT_UNAWARE
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((HANDLE)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((HANDLE)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((HANDLE)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((HANDLE)-5)
#endif

BOOL
WINAPI
_SetProcessDpiAwarenessContext(
    HANDLE value /* DPI_AWARENESS_CONTEXT */
    );

HANDLE /* DPI_AWARENESS_CONTEXT */
WINAPI
_GetDpiAwarenessContextForProcess(
    HANDLE hProcess
    );

BOOL
WINAPI
_MonGetMonitorRectFromWindow(
    HWND hWnd,
    RECT *prcResult,
    ULONG Flags,
    BOOLEAN bWorkspace
    );

BOOL
WINAPI
_CenterWindow(
    HWND hwndChild,
    HWND hwndParent
    );

LPTSTR
WINAPI
_CommaFormatString(
    ULONGLONG val,
    LPTSTR pszOut
    );

void
WINAPI
_GetDateTimeString(
    const ULONG64 DateTime,
    LPTSTR pszText,
    int cchTextMax
    );

LPTSTR
WINAPI
_GetDateTimeStringFromFileTime(
    const FILETIME *DateTime,
    LPTSTR pszText,
    int cchTextMax
    );

VOID
WINAPI
_GetDateTimeStringEx(
    ULONG64 DateTime,
    LPTSTR pszText,
    int cchTextMax,
    LPTSTR DateFormat,
    LPTSTR TimeFormat,
    BOOL bDisplayAsUTC
    );

VOID
WINAPI
_GetDateTimeStringEx2(
    ULONG64 DateTime,
    LPTSTR pszText,
    int cchTextMax,
    LPTSTR DateFormat,
    LPTSTR TimeFormat,
    BOOL bDisplayAsUTC,
    BOOL bMilliseconds
    );

VOID
WINAPI
_GetDateTime(
    ULONG64 DateTime,
    LPTSTR pszText,
    int cchTextMax
    );

VOID
WINAPI
_GetDateTimeFromFileTime(
    FILETIME *DateTime,
    LPTSTR pszText,
    int cchTextMax
    );

//
// Clipboard Helper
//
#define SEPCHAR_TAB    0x1 
#define SEPCHAR_COMMA  0x2
#define SEPCHAR_SPACE  0x0

LONG
WINAPI
SetClipboardTextFromListView(
    HWND hwndLV,
    ULONG Flags
    );

#define SCTEXT_ANSI      1
#define SCTEXT_UNICODE   0

LONG
WINAPI
SetClipboardText(
    HWND hwndCilpboardOwner,
    PVOID pszCopyString,
    ULONG CodeType
    );

#define SCTEXT_FORMAT_CSV          0x00000001
#define SCTEXT_FORMAT_TSV          0x00000002
#define SCTEXT_FORMAT_SELECTONLY   0x00004000
#define SCTEXT_FORMAT_DOUBLEQUATE  0x00008000

LONG
WINAPI
SetClipboardTextFromListViewColumn(
    HWND hwndLV,
    UINT uFormat,
    int iColumn
    );

//
// Win32 MessageBox Helper
//
HRESULT
WINAPI
SetMessageBoxCaption(
    PCWSTR Caption
    );

#define _MB_DISABLE_CONTINUE  0x0001
#define _MB_DISABLE_RETRY     0x0002
#define _MB_DISABLE_CANCEL    0x0004
#define _MB_DISABLE_TRYAGAIN  0x0008

VOID
WINAPI
InitMessageBoxLibMisc(
    HINSTANCE hInst,
    PCWSTR pszCaption
    );

int
WINAPI
MsgBoxEx(
    HWND hwndOwner,
    LPCTSTR pszText,
    LPCTSTR pszCaption,
    UINT flags,
    UINT idDisableButton
    );

int
WINAPI
MsgBox(
    HWND hwnd,
    LPCTSTR pszText,
    LPCTSTR pszCaption,
    UINT flags
    );

int
WINAPI
MsgBox(
    HWND hwnd,
    LPCTSTR pszText,
    UINT flags
    );

int
WINAPI
MsgBox(
    HWND hwnd,
    UINT idString,
    UINT flags
    );

int
WINAPI
MsgBox(
    HWND hwnd,
    UINT idString,
    UINT idCaption,
    UINT flags
    );

int
WINAPI
MsgBoxIcon(
    HWND hwndOwner,
    LPCTSTR pszText,
    LPCTSTR pszCaption,
    UINT flags,
    UINT idDisableButton,
    HINSTANCE hInstance,
    LPCWSTR lpszIcon
    );

EXTERN_C
int
WINAPI
_ErrorMessageBox(
    HWND hWnd,
    UINT_PTR idString,
    PCWSTR pszFile,
    ULONG Status,
    ULONG Flags
    );

EXTERN_C
int
WINAPI
_ErrorMessageBoxEx(
    HWND hWnd,
    UINT_PTR idString,
    PCWSTR pszCaption,
    PCWSTR pszFile,
    ULONG Status,
    ULONG Flags
    );

EXTERN_C
int
WINAPI
_ErrorMessageBoxEx2(
    HWND hWnd,
    PCWSTR pszLayout,
    PCWSTR pszCaption,
    PCWSTR pszMessage,
    PCWSTR pszReserved,
    ULONG Status,
    ULONG FormatFlags,
    ULONG Flags
    );

EXTERN_C
INT
CDECL
_ErrorPrintfMessageBox(
    HWND hwndOwner,
    LPCWSTR Title,
    LPCWSTR LayoutString,
    LONG code,
    UINT uType,
    LPCWSTR FormatMessage,
    ...
    );

#if (_WIN32_WINNT >= 0x600)

EXTERN_C
HRESULT
WINAPI
VerificationMessageBox( 
	HWND hwnd,
	HINSTANCE hRes,
	PWSTR pszTitle,
	PWSTR pszMainInstruction,
	PWSTR pszContent,
	PWSTR pszVerificationText,
	TASKDIALOG_BUTTON *buttons,
	int buttonsCount,
	int nDefaultButton,
	int *pSelectedButton,
	int *pVerificationFlag
	);

#endif

//
// System Error Message Helper
//
int
_GetSystemErrorMessage(
    ULONG ErrorCode,
    PWSTR *ppMessage
    );

int
_GetSystemErrorMessageEx(
    ULONG ErrorCode,
    PWSTR *ppMessage,
    DWORD dwLanguageId
    );

void
_FreeSystemErrorMessage(
    PWSTR pMessage
    );

PWSTR
_cdecl
_MakeMessageString(
	UINT idFormatRes,
	LPCWSTR lpszFormat,
	...
	);

PWSTR
FormatNtStatusErrorMessage(
	PCWSTR NtStatusErrorMessage,
	PWSTR Buffer,
	SIZE_T cchBufferLength,
	ULONG Flags
	);

//
// Placeholder Compatibility Mode Funcsion
//
#define PHCM_APPLICATION_DEFAULT 	0
#define PHCM_DISGUISE_PLACEHOLDER 	1
#define PHCM_EXPOSE_PLACEHOLDERS 	2
#define PHCM_MAX 	                2
#define PHCM_ERROR_INVALID_PARAMETER  (-1)
#define PHCM_ERROR_NO_TEB             (-2)

EXTERN_C
CHAR
WINAPI
SetProcessPlaceholderCompatibilityMode(
    CHAR Mode
    );

//
// RECT Helper
//
#define _RECT_WIDTH(rc) (rc.right-rc.left)
#define _RECT_HIGHT(rc) (rc.bottom-rc.top)

//
// Helper macro
//
#ifdef __cplusplus
inline void SetRedraw(HWND h,BOOL f) { SendMessage(h,WM_SETREDRAW,(WPARAM)f,0); }
#else
#define SetRedraw(h,f)	SendMessage(h,WM_SETREDRAW,(WPARAM)f,0)
#endif
#define GETINSTANCE(hWnd)   (HINSTANCE)GetWindowLongPtr(hWnd,GWLP_HINSTANCE)
#define GETCLASSBRUSH(hWnd) (HBRUSH)GetClassLongPtr(hWnd,GCLP_HBRBACKGROUND)


//
// String miscellaneous functions
//
INT
StringFindNumber(
    PCWSTR psz
    );

BOOL
WINAPI
IsStringBackslashEnd(
	PCWSTR pszPath
	);

//
// ListView Helper
//
UINT
ListViewEx_SimpleContextMenuHandler(
    HWND hWnd,
    HWND hWndList,
    HWND hwndRightClicked, // Reserved
    HMENU hMenu,
    POINT point,
    UINT uFlags
    );

int
ListViewEx_SetColumnWidthByHeaderText(
	HWND hwndLV,
	int iColumn,
	DWORD dwFlags
	);

#define LVEXCHTF_ADJUST_WIDTH_BY_COLUMN_ITEM_TEXT (0x1)

int
ListViewEx_SetLastColumnWidth(
	HWND hwndLV,
	DWORD dwFlags
	);

int
ListViewEx_GetColumnIndexFromColumnId(
	HWND hwndLV,
	INT_PTR ColumnId
	);

int
ListViewEx_GetTextColumns(
	HWND hwndLV,
	PWSTR **pColumns,
	INT *pColumnCount,
	SIZE_T *pColumnBufferSize
	);

VOID
DrawListViewColumnMeter(
    HDC hdc,
    HWND hWndList,
    int iItem,
    int iMeterColumn,
    RECT *prcRect,   // Reserved
    HFONT hTextFont, // Optional
    double DiskUsage,
    UINT fMeterStyle
    );

VOID
DrawFocusFrame(
    HWND hWnd,
    HDC hdc,
    RECT *prc,
    BOOL bDrawFocus,
    COLORREF crActiveFrame
    );

typedef struct _DFFSTRUCT
{
	DWORD cbSize; // reserved
} DFFSTRUCT;

#define DFFEXF_ADJUSTGRIDLINE (0x1)

VOID
DrawFocusFrameEx(
	HWND hWnd,
	HDC hdc,
	RECT *prc,
	BOOL bDrawFocus,
	COLORREF crActiveFrame,
	DWORD dwFlags,
	DFFSTRUCT *pdffs
	);

// Make GUID string without brackets
inline VOID GUIDStringRemoveBrackets(WCHAR *pszGuid) { 
    memmove(pszGuid,&pszGuid[1],sizeof(WCHAR)*36);
    pszGuid[36] = L'\0';
}

// Task allocator helper
inline PWSTR _CoTaskMemStrDup(PCWSTR psz) {
    SIZE_T cch = wcslen(psz) + 1;
    PWSTR pszNew = (PWSTR)CoTaskMemAlloc( cch * sizeof(WCHAR) );
    if( pszNew )
        wcscpy_s(pszNew,cch,psz);
    return pszNew;
}

inline PVOID _CoTaskMemAllocZero(SIZE_T cb) {
    PVOID ptr = (PWSTR)CoTaskMemAlloc( cb );
    if( ptr )
        ZeroMemory(ptr,cb);
    return ptr;
}

inline void WINAPI DoMessage(HWND hWnd=NULL) {
    MSG msg;
    while( PeekMessage(&msg,hWnd,0,0,PM_REMOVE) )
    {
        DispatchMessage(&msg);
    }
}

inline void WINAPI DoMessageSwitchThread(HWND hWnd=NULL) {
    MSG msg;
    while( PeekMessage(&msg,hWnd,0,0,PM_REMOVE) )
    {
        SwitchToThread();
        DispatchMessage(&msg);
    }
}

inline void WINAPI DoMessageSleep(HWND hWnd=NULL,DWORD dwMilliseconds=0) {
    DWORD dwResult;
    for(;;)
    {
        dwResult = MsgWaitForMultipleObjects(0,NULL,FALSE,dwMilliseconds,QS_ALLEVENTS);

        if( WAIT_TIMEOUT == dwResult )
        {
            break;			
        }
        else if( (int)dwResult <= (int)WAIT_OBJECT_0 )
        {
            MSG msg;
            while( PeekMessage(&msg,hWnd,0,0,PM_REMOVE) )
            {
                DispatchMessage(&msg);
            }
        }
        else if( WAIT_ABANDONED_0 == dwResult )
        {
            break;			
        }
    }
}

inline BOOL IsWindowVisibleEx(HWND hwnd)
{
	return IsWindow(hwnd) && (GetWindowLong(hwnd,GWL_STYLE) & WS_VISIBLE);
}

inline int GetTextParcent()
{
    //ex)
    //100%
    //125%
    //150%
    HDC hdc;
    hdc = GetWindowDC(NULL);
    int per = (int)(100.0 * ((double)GetDeviceCaps(hdc,LOGPIXELSX) / 96.0));
    ReleaseDC(NULL,hdc);
    return per;
}

//
// Task allocator helper
//
#define _CoTaskSafeMemFree(p) if(p) { CoTaskMemFree(p); p = NULL; }

//
// libwin32ctrl.lib
//
void InitLongPathBox(HMODULE hModule);

//
// Shell Helper
//
BOOL
WINAPI
SHFileIconInit(
    BOOL fRestoreCache
    );

EXTERN_C
LONG
WINAPI
GetPathFromAppPaths(
    PCWSTR pszName,
    PWSTR pszPath,
    DWORD cchPath,
    BOOL bExpand,
    PWSTR pszStartupDir,
    DWORD cchStartupDir,
    BOOL bExpandStartupDir
    );

BOOL
WINAPI
_OpenByExplorerEx(
    HWND hWnd,
    LPCTSTR pszPath,
    LPCTSTR pszCurrentDirectory,
    BOOL bAdmin
    );

EXTERN_C
HRESULT
WINAPI
OpenTerminal(
	HWND hwndOwner,
	PCWSTR pszPath
	);

BOOL
WINAPI
GetBashExePath(
	LPTSTR szBashPath,
	UINT bufSize
	);

BOOL
WINAPI
GetPowershellExePath(
	LPTSTR szPSPath
	);

INT
WINAPI
GetUpDirImageIndex(
	VOID
	);

HIMAGELIST
WINAPI
GetGlobalShareImageList(
	int ImageList
	);

#include <shellapi.h>

int
WINAPI
GetShellFileIconImageListIndexEx(
	PCWSTR pszPath,
	PCWSTR pszFileName,
	DWORD dwFileAttributes,
	DWORD dwFlags,
	HICON *phIcon
	);

int
WINAPI
GetShellFileImageListIndex(
	PCWSTR pszPath,
	PCWSTR pszFileName,
	DWORD dwFileAttributes
	);

HICON
WINAPI
GetShellFileIcon(
	PCWSTR pszPath,
	UINT uFlags
	);

HICON
WINAPI
GetShellStockIcon(
	SHSTOCKICONID StockIconId
	);

enum {
	OpenVolumeLocationWithExplorer=0,
	OpenVolumeLocationWithCommandPrompt=1,
	OpenVolumeLocationWithPowerShell=2,
	OpenVolumeLocationWithTerminal=3,
	OpenVolumeLocationWithBash=4,
	OpenVolumeLocationWithAdmin=0x8000,
};

HRESULT
WINAPI
OpenVolumeLocationByShell(
	HWND hWnd,
	UINT Open,
	PCWSTR pszDosDrive,
	PCWSTR pszVolumeGuid
	);

#define WINUSER_ICON_APPLICATION 100
#define WINUSER_ICON_EXCLAMATION 101
#define WINUSER_ICON_QUESTION    102
#define WINUSER_ICON_STOP        103
#define WINUSER_ICON_INFORMATION 104
#define WINUSER_ICON_WINLOGO     105 // same as Application
#define WINUSER_ICON_SHIELD      106

#define WINUSER_ICON_SMALL       16
#define WINUSER_ICON_LARGE       32

HICON
WINAPI
_GetUser32Icon(
    int idIconType,
    int cSize
    );

HBITMAP _CreateBitmapARGB(int nWidth, int nHeight);
HBITMAP _IconToBitmap(HICON hIcon);
HBITMAP _IconToBitmap16(HICON hIcon); /* Backwards Compatible function */
HBITMAP _IconToBitmap16(HICON hIcon); /* Backwards Compatible function */

BOOL
WINAPI
_SetMenuIcon(
    HMENU hMenu,
    UINT idMenu,
    HICON hIcon
    );

