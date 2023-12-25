#pragma once

#define MDICHILD_CLASSNAME  L"MDIChild";
#define CBWNDEXTRA   256

typedef struct _MDICREATEPARAM
{
	PCWSTR pszInitialPath;
	HICON hIcon;
} MDICREATEPARAM;

typedef struct _MDICHILDWNDDATA
{
	HWND hWndView;
	HWND hWndFocus;
	UINT_PTR wndId;
} MDICHILDWNDDATA;

typedef struct _MDICHILDFRAMEINIT
{
	POINT pt;
	SIZE size;
	INT show;
} MDICHILDFRAMEINIT;

ATOM RegisterMDIChildFrameClass(HINSTANCE hInstance);
HWND CreateMDIClient(HWND hWnd);
HWND CreateMDIChildFrame(HWND hWndMDIClient,PCWSTR pszTitle,MDICHILDFRAMEINIT *pmdiInit,LPARAM lParam,BOOL bMaximize);

__inline HWND MDIGetActive(HWND hwndMDIClient,LPBOOL pbMaximized=NULL) { return (HWND)SendMessage(hwndMDIClient,WM_MDIGETACTIVE,0,(LPARAM)pbMaximized); }
