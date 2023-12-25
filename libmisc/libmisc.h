#pragma once

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

BOOL
WINAPI
_OpenByExplorerEx(
	HWND hWnd,
	LPCTSTR pszPath,
	LPCTSTR pszCurrentDirectory,
	BOOL bAdmin
	);

BOOL
WINAPI
SHFileIconInit(
	BOOL fRestoreCache
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
