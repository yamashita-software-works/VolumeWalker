//***************************************************************************
//*                                                                         *
//*  volumewalker.cpp                                                       *
//*                                                                         *
//*  Implements the main procedure and main frame window                    *
//*                                                                         *
//*  AUTHOR: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  2022.04.02 Create SDI frame ver.                                       *
//*  2022.12.24 Create MDI frame ver.                                       *
//*  2023.02.24 Create MDI based mainframe.                                 *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumewalker.h"
#include "mdichild.h"
#include "resource.h"
#include "find.h"

static HINSTANCE hInst = NULL;
HWND hWndMain = NULL;
HWND hWndMDIClient = NULL;
HWND hWndActiveMDIChild = NULL;
static WCHAR *pszTitle       = L"FSVolumeWalker";
static WCHAR *pszWindowClass = L"FSVolumeWalkerMDIFrameWindowClass";
static HMENU hMainMenu = NULL;
static HMENU hMdiMenu = NULL;

#define _SINGLETON_CONTENTS_BROWSER_WINDOW 1

VOID WINAPI _DoMessage(HWND h)
{
	MSG msg;
	while( PeekMessage(&msg,h,0,0,PM_REMOVE) )
	{
		DispatchMessage(&msg);
	}
}

HINSTANCE _GetResourceInstance()
{
	return hInst;
}

HWND _GetMainWnd()
{
	return hWndMain;
}

//////////////////////////////////////////////////////////////////////////////
#include "..\build.h"

static void Edit_AddText(HWND hwndEdit,PCWSTR psz)
{
	SendMessage(hwndEdit,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);
	SendMessage(hwndEdit,EM_REPLACESEL,0,(LPARAM)psz);
}

static void OSVersionText(HWND hwndEdit)
{
	WCHAR szText[MAX_PATH];

	OSVERSIONINFOEX osi = {0};
	osi.dwOSVersionInfoSize = sizeof(osi);
	GetVersionEx((LPOSVERSIONINFO)&osi);

	DWORD cb = sizeof(DWORD);
	DWORD UBR = 0;
	SHRegGetValue(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",L"UBR",SRRF_RT_REG_DWORD,NULL,&UBR,&cb);

	WCHAR szDisplayVersion[MAX_PATH];
	cb = _countof(szDisplayVersion);
	szDisplayVersion[0] = 0;
	SHRegGetValue(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",L"DisplayVersion",SRRF_RT_REG_SZ,NULL,szDisplayVersion,&cb);

	WCHAR szCSD[MAX_PATH];
	StringCchCopy(szCSD,MAX_PATH,osi.szCSDVersion);

	cb = _countof(szText);
	szText[0] = 0;
	if( SHRegGetValue(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",L"CSDBuildNumber",SRRF_RT_REG_SZ,NULL,szText,&cb) == 0 )
	{
		if( cb != 0 )
		{
			StringCchCat(szCSD,MAX_PATH,L" Build ");
			StringCchCat(szCSD,MAX_PATH,szText);
		}
	}

	if( UBR != 0 )
	{
		StringCchPrintf(szText,MAX_PATH,L"%u.%u.%u.%u %s",
			osi.dwMajorVersion,osi.dwMinorVersion,osi.dwBuildNumber,UBR,szCSD);
	}
	else
	{
		StringCchPrintf(szText,MAX_PATH,L"%u.%u.%u %s",
			osi.dwMajorVersion,osi.dwMinorVersion,osi.dwBuildNumber,szCSD);
	}

	if( szDisplayVersion[0] != L'\0' )
	{
		StringCchCat(szText,MAX_PATH,L" (");
		StringCchCat(szText,MAX_PATH,szDisplayVersion);
		StringCchCat(szText,MAX_PATH,L")");
	}

	Edit_AddText(hwndEdit,L"  ");
	Edit_AddText(hwndEdit,szText);
	Edit_AddText(hwndEdit,L"\r\n");

	cb = _countof(szText);
	szText[0] = 0;
	if( SHRegGetValue(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",L"BuildLabEx",SRRF_RT_REG_SZ,NULL,szText,&cb) == ERROR_SUCCESS)
	{
		if( cb != 0 )
		{
			Edit_AddText(hwndEdit,L"  ");
			Edit_AddText(hwndEdit,szText);
		}
	}
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
		{
			HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT);

			// version:
			{
				WCHAR sz[32];
				StringCchPrintf(sz,_countof(sz),L"%u.%u.%u.%u Preview",MEJOR_VERSION,MINOR_VERSION,BUILD_NUMBER,PATCH_NUMBER);
				SetDlgItemText(hDlg,IDC_TEXT,sz);
			}

			// miscellaneous/system information:
			{
				LARGE_INTEGER liBootTime;
				LARGE_INTEGER liElapsedTime;
				GetSystemBootTime( &liBootTime, NULL, &liElapsedTime );

				_CenterWindow(hDlg,GetActiveWindow());

				WCHAR szBuf[64];
				WCHAR szBootTime[32];
				WCHAR szBootElapsedTime[32];
				_GetDateTimeString(liBootTime.QuadPart,szBootTime,ARRAYSIZE(szBootTime));
				StrFromTimeInterval(szBootElapsedTime,_countof(szBootElapsedTime),(DWORD)((liElapsedTime.QuadPart)/10000),3);
				StrTrim(szBootElapsedTime,L" ");

				StringCchPrintf(szBuf,_countof(szBuf),L"System Boot Time : %s",szBootTime);
				Edit_AddText(hwndEdit,szBuf);

				Edit_AddText(hwndEdit,L"\r\n");

				StringCchPrintf(szBuf,_countof(szBuf),L"Boot Elapsed Time : %s",szBootElapsedTime);
				Edit_AddText(hwndEdit,szBuf);
			}

			// OS version:
			{
				Edit_AddText(hwndEdit,L"\r\n\r\n");
				Edit_AddText(hwndEdit,L"Windows Versison:\r\n");
				OSVersionText(GetDlgItem(hDlg,IDC_EDIT));
			}

			return (INT_PTR)TRUE;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

//////////////////////////////////////////////////////////////////////////////

// stl::map<> is the preferred using.
typedef struct {
	UINT ConsoleId;
	HWND hWnd;
} MDICHILDFRAMETABLE;

static MDICHILDFRAMETABLE table[]= {
	{VOLUME_CONSOLE_HOME,              0},
	{VOLUME_CONSOLE_VOLUMELIST,        0},
	{VOLUME_CONSOLE_PHYSICALDRIVELIST, 0},
	{VOLUME_CONSOLE_SHADOWCOPYLIST,    0},
	{VOLUME_CONSOLE_STORAGEDEVICE,     0},
	{VOLUME_CONSOLE_MOUNTEDDEVICE,     0},
	{VOLUME_CONSOLE_DOSDRIVELIST,      0},
	{VOLUME_CONSOLE_FILTERDRIVER,      0},
#if _SINGLETON_CONTENTS_BROWSER_WINDOW
	{VOLUME_CONSOLE_CONTENT_FILES,     0}, // todo: currently, singleton console.
	{VOUUME_CONSOLE_CHANGE_JOURNAL,    0}, // todo: currently, singleton console.
#endif
};

int ClearWindowHandle(UINT_PTR ConsoleTypeId,HWND hwnd)
{
	int i;
	for(i = 0; i < ARRAYSIZE(table); i++)
	{
		if( table[i].ConsoleId == ConsoleTypeId )
		{
			if( table[i].hWnd == hwnd )
			{
				table[i].hWnd = NULL;
				return i;
			}
		}
	}
	return -1;
}

int SetWindowHandle(UINT_PTR ConsoleTypeId,HWND hwnd)
{
	int i;
	for(i = 0; i < ARRAYSIZE(table); i++)
	{
		if( table[i].ConsoleId == ConsoleTypeId )
		{
			table[i].hWnd = hwnd;
			return i;
		}
	}
	return -1;
}

HWND GetOpenedWindowHandle(UINT_PTR ConsoleTypeId)
{
	int i;
	for(i = 0; i < ARRAYSIZE(table); i++)
	{
		if( table[i].ConsoleId == ConsoleTypeId )
		{
			return table[i].hWnd;
		}
	}
	return NULL;
}

HWND FindSameWindowTitle(UINT ConsoleType,PCWSTR pszName)
{
	HWND hwnd;
	hwnd = GetWindow(hWndMDIClient,GW_CHILD);
	if( hwnd )
	{
		WCHAR szTitle[MAX_PATH];
		do
		{
			MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
			if( pd->wndId == ConsoleType )
			{
				GetWindowText(hwnd,szTitle,MAX_PATH);
				if( _wcsicmp(szTitle,pszName) == 0 )
				{
					return hwnd;
				}
			}

		} while( hwnd = GetWindow(hwnd,GW_HWNDNEXT) );
	}

	return NULL;
}

VOID SendUIInitLayout(HWND hwndMDIChild,HWND hWndView)
{
	RECT rc;
	GetClientRect(hwndMDIChild,&rc);
	SetWindowPos(hWndView,NULL,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE|SWP_HIDEWINDOW);
	SendMessage(hWndView,WM_CONTROL_MESSAGE,UI_INIT_LAYOUT,(LPARAM)&rc);
}

VOID SetContentsBrowserPath(HWND hwndMDIChild,HWND hWndView,PCWSTR pszPath,BOOL bInitLayout)
{
	if( bInitLayout ) 
		SendMessage(hWndView,WM_CONTROL_MESSAGE,UI_INIT_VIEW,(LPARAM)0);

	WCHAR szVolumeRootOrPath[MAX_PATH];
	StringCchPrintf(szVolumeRootOrPath,MAX_PATH,L"%s\\",pszPath);
	SendMessage(hWndView,WM_CONTROL_MESSAGE,UI_SET_DIRECTORY,(LPARAM)szVolumeRootOrPath);

	if( pszPath )
		SetWindowText(hwndMDIChild,pszPath);
	else
		SetWindowText(hwndMDIChild,pszTitle);
}

VOID SendHexDumpInformation(HWND hwndMDIChild,SELECT_OFFSET_ITEM *OpenSelItem,PCWSTR pszOpenTarget)
{
	MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
	{
		//
		// Hex dump: Set data source, offset, and home offset.
		//
		SELECT_OFFSET_ITEM sel = {0};
		sel.hdr.mask = SI_MASK_PATH|SI_MASK_NAME|SI_MASK_VIEWTYPE|SI_MASK_START_OFFSET;
		sel.hdr.ViewType = VOLUME_CONSOLE_SIMPLEHEXDUMP;

		if( OpenSelItem )
		{
			sel.hdr.pszStorage = OpenSelItem->hdr.pszStorage;
			sel.liStartOffset  = OpenSelItem->liStartOffset;
		}
		else
		{
			sel.hdr.pszStorage = (PWSTR)pszOpenTarget;
			sel.liStartOffset.QuadPart = 0;
		}

		SendMessage(pd->hWndView,WM_NOTIFY_MESSAGE,UI_NOTIFY_VOLUME_SELECTED,(LPARAM)&sel);

		SetWindowText(hwndMDIChild,sel.hdr.pszStorage);
	}
}

//----------------------------------------------------------------------------
//
//  OpenMDIChild()
//
//  PURPOSE: Open MDI child window.
//
//----------------------------------------------------------------------------
HWND OpenMDIChild(HWND hWnd,UINT ConsoleTypeId,PCWSTR pszOpenTarget,PVOID pOpenSelItem,BOOL bMaximize)
{
	HWND hwndChildFrame = NULL;

	if( ConsoleTypeId == VOLUME_CONSOLE_DISKLAYOUT || 
		ConsoleTypeId == VOLUME_CONSOLE_VOLUMEINFORMAION || 
		ConsoleTypeId == VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION ||
		ConsoleTypeId == VOLUME_CONSOLE_FILESYSTEMSTATISTICS ||
#if _SINGLETON_CONTENTS_BROWSER_WINDOW
		ConsoleTypeId == VOLUME_CONSOLE_SIMPLEHEXDUMP )
#else
		ConsoleTypeId == VOLUME_CONSOLE_SIMPLEHEXDUMP ||
		ConsoleTypeId == VOLUME_CONSOLE_CONTENT_FILES ||
		ConsoleTypeId == VOUUME_CONSOLE_CHANGE_JOURNAL )
#endif

	{
		hwndChildFrame = FindSameWindowTitle(ConsoleTypeId,
							(pOpenSelItem != NULL) ? ((SELECT_ITEM*)pOpenSelItem)->pszVolume : pszOpenTarget);
	}
	else
	{
		hwndChildFrame = GetOpenedWindowHandle(ConsoleTypeId);
	}

	if( hwndChildFrame != NULL )
	{
		BOOL bMaximized;
		MDIGetActive(hWndMDIClient,&bMaximized);

		if( bMaximized )
		{
			SetRedraw(hWndMDIClient,FALSE);
		}

		SendMessage(hWndMDIClient,WM_MDIACTIVATE,(WPARAM)hwndChildFrame,0);

		if( VOLUME_CONSOLE_CONTENT_FILES == ConsoleTypeId || VOUUME_CONSOLE_CHANGE_JOURNAL == ConsoleTypeId )
		{
			MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndChildFrame,GWLP_USERDATA);
			{
				SetContentsBrowserPath(hwndChildFrame,pd->hWndView,pszOpenTarget,FALSE);
			}
		}
		else if( VOLUME_CONSOLE_SIMPLEHEXDUMP == ConsoleTypeId && pOpenSelItem )
		{
			SendHexDumpInformation(hwndChildFrame,(SELECT_OFFSET_ITEM *)pOpenSelItem,pszOpenTarget);
		}

		if( bMaximized )
		{
			SetRedraw(hWndMDIClient,TRUE);
			RedrawWindow(hwndChildFrame,0,0,RDW_UPDATENOW|RDW_INVALIDATE|RDW_ALLCHILDREN);
		}

		return hwndChildFrame;
	}

	MDICREATEPARAM mcp;
	if( pszOpenTarget )
		mcp.pszInitialPath = pszOpenTarget;
	else
		mcp.pszInitialPath = NULL;

	if( ConsoleTypeId == VOLUME_CONSOLE_SHADOWCOPYLIST )
		mcp.hIcon = GetDeviceClassIcon(DEVICE_ICON_VOLUMESNAPSHOT,NULL);
	else if( ConsoleTypeId == VOLUME_CONSOLE_FILTERDRIVER )
	{
		HINSTANCE hmod = LoadLibrary(L"imageres.dll");
		mcp.hIcon = (HICON)LoadImage(hmod,MAKEINTRESOURCE(67),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
		FreeLibrary(hmod);
	}
#if 0
	else if( ConsoleTypeId == VOUUME_CONSOLE_CHANGE_JOURNAL )
	{
		HINSTANCE hmod = LoadLibrary(L"shell32.dll");
		mcp.hIcon = (HICON)LoadImage(hmod,MAKEINTRESOURCE(152),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
		FreeLibrary(hmod);
	}
#endif
	else if( ConsoleTypeId == VOLUME_CONSOLE_SIMPLEHEXDUMP )
		mcp.hIcon = (HICON)LoadImage(_GetResourceInstance(),MAKEINTRESOURCE(IDI_BINDUMP),IMAGE_ICON,16,16,LR_DEFAULTSIZE);
	else
		mcp.hIcon = GetShellStockIcon(SIID_DRIVEFIXED);

	static struct {
		UINT ConsoleId;
		PCWSTR Title;
	} title[] = {
		{VOLUME_CONSOLE_VOLUMELIST,         L"Volumes"},
		{VOLUME_CONSOLE_PHYSICALDRIVELIST,  L"Physical Drives"},
		{VOLUME_CONSOLE_SHADOWCOPYLIST,     L"Shadow Copy Volumes"},
		{VOLUME_CONSOLE_STORAGEDEVICE,      L"Storage Devices"},
		{VOLUME_CONSOLE_MOUNTEDDEVICE,      L"Mounted Devices"},
		{VOLUME_CONSOLE_DOSDRIVELIST,       L"Dos Drives"},
		{VOLUME_CONSOLE_FILTERDRIVER,       L"Minifilter Driver"},
		{VOLUME_CONSOLE_CONTENT_FILES,      L"Volume Contents Browser"},
		{VOUUME_CONSOLE_CHANGE_JOURNAL,     L"Volume Change Journal Browser"},
	};

	PCWSTR pszTitle = L"";

	for(int i = 0; i < _countof(title); i++)
	{
		if( title[i].ConsoleId == ConsoleTypeId )
		{
			pszTitle = title[i].Title;
			break;
		}
	}

	MDICHILDFRAMEINIT mcinit = { {CW_USEDEFAULT,CW_USEDEFAULT},{CW_USEDEFAULT,CW_USEDEFAULT}, 0 }; // reserved

	BOOL bVisible = IsWindowVisible(hWnd);

	if( bVisible )
		SetRedraw(hWnd,FALSE); // When new MDI child window open maximizing, prevents the mainframe menu flickering.

	//
	// Open MDI child frame window
	//
	HWND hwndMDIChild = CreateMDIChildFrame(hWndMDIClient,NULL,&mcinit,(LPARAM)&mcp,bMaximize);

	if( bVisible )
	{
		SetRedraw(hWnd,TRUE);  // Reverd redraw state.
		RedrawWindow(hWnd,0,0,RDW_UPDATENOW|RDW_INVALIDATE|RDW_ALLCHILDREN); // Refresh all windows
	}

	if( hwndMDIChild )
	{
		if( bVisible )
			SetWindowPos(hwndMDIChild,0,0,0,0,0,SWP_SHOWWINDOW|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_DRAWFRAME);

		if( hMdiMenu == NULL )
			hMdiMenu = LoadMenu(_GetResourceInstance(),MAKEINTRESOURCE(IDR_MDICHILDFRAME));

		SendMessage(hWndMDIClient,WM_MDISETMENU,(WPARAM)hMdiMenu,0);
		DrawMenuBar(hWnd);

		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
		{
			SetWindowHandle(ConsoleTypeId,hwndMDIChild);
			pd->wndId = ConsoleTypeId;

			VOLUME_CONSOLE_CREATE_PARAM param = {0};
			param.pszReserved = (PWSTR)mcp.pszInitialPath;

			if( VOLUME_CONSOLE_SIMPLEHEXDUMP == ConsoleTypeId )
			{
				pd->hWndView = CreateVolumeConsoleWindow(hwndMDIChild,ConsoleTypeId,&param);

				SendUIInitLayout(hwndMDIChild,pd->hWndView);

				SendHexDumpInformation(hwndMDIChild,(SELECT_OFFSET_ITEM *)pOpenSelItem,pszOpenTarget);
			}
			else if( VOLUME_CONSOLE_CONTENT_FILES == ConsoleTypeId || VOUUME_CONSOLE_CHANGE_JOURNAL == ConsoleTypeId )
			{
				pd->hWndView = CreateVolumeContentsBrowserWindow(hwndMDIChild,ConsoleTypeId);

				SendUIInitLayout(hwndMDIChild,pd->hWndView);

				SetContentsBrowserPath(hwndMDIChild,pd->hWndView,pszOpenTarget,TRUE);
			}
			else
			{
				pd->hWndView = CreateVolumeConsoleWindow(hwndMDIChild,ConsoleTypeId,&param);

				SendUIInitLayout(hwndMDIChild,pd->hWndView);

				SELECT_ITEM sel = {0};
				switch( ConsoleTypeId )
				{
					case VOLUME_CONSOLE_VOLUMEINFORMAION:
					case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
					{
						sel.pszVolume = (PWSTR)pszOpenTarget;
						sel.ViewType = ConsoleTypeId;
						break;
					}
					case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
					case VOLUME_CONSOLE_DISKLAYOUT:
					{
						sel.pszPhysicalDrive = (PWSTR)pszOpenTarget;
						sel.ViewType = ConsoleTypeId;
						break;
					}
				}

				SendMessage(pd->hWndView,WM_NOTIFY_MESSAGE,UI_NOTIFY_VOLUME_SELECTED,(LPARAM)&sel);

				if( pszOpenTarget )
					SetWindowText(hwndMDIChild,pszOpenTarget);
				else
					SetWindowText(hwndMDIChild,pszTitle);
			}

			ShowWindow(pd->hWndView,SW_SHOW);

			SendMessage(hWndMDIClient,WM_MDIREFRESHMENU,0,0);
			DrawMenuBar(hWnd);
		}
	}

	return hwndMDIChild;
}

VOID OpenConsole(HWND hWnd,UINT ConsoleTypeId,PCWSTR Param=NULL,UINT ParamType=0)
{
	HWND hwndChild = OpenMDIChild(hWnd,ConsoleTypeId,
				(PCWSTR)((ParamType != 2) ? Param : NULL),
				(PVOID)((ParamType == 2) ? Param : NULL),
				FALSE);
	if( hwndChild )
	{
		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndChild,GWLP_USERDATA);
		SetFocus(pd->hWndView);
	}
}

VOID CloseConsole(HWND hwndMDIChildFrame=NULL)
{
	SendMessage(hWndMDIClient, WM_MDIDESTROY,
		(WPARAM)(hwndMDIChildFrame ? hwndMDIChildFrame :MDIGetActive(hWndMDIClient)),
		0L); 
}

//----------------------------------------------------------------------------
//
//  ShowHelp()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
VOID ShowHelp(HWND hWnd)
{
	PCWSTR pszHelpText = 
			L"Command Line Option:\n"
			L"/v : Open Volume List.\n"
			L"/pd : Open Physical Drive List.\n"
			L"/sd : Open Storage Device List.\n"
			L"/md : Open Mounted Device List.\n"
			L"/d : Open MS-DOS Drive List.\n"
			L"\n"
			L"/f : Open maximized in the MDI client area.";

	MessageBox(hWnd,pszHelpText,L"Command Line Help",MB_OK|MB_ICONINFORMATION);
}

//----------------------------------------------------------------------------
//
//  ParseCommandLine()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
typedef struct _ARGPARAM {
	CValArray<UINT> ConsoleTypeId;
	INT Maximize;
	INT WithoutOpen;
	INT Help;

	_ARGPARAM()
	{
		Maximize = FALSE;
		WithoutOpen = FALSE;
		Help = FALSE;
		ConsoleBit = 0;
	}

	ULONG ConsoleBit;
	BOOL addOpenConsole(UINT Id)
	{
		if( (ConsoleBit & (0x1 << Id)) == 0 )
		{
			ConsoleTypeId.Add( Id );
			ConsoleBit |= (0x1 << Id);
			return TRUE;
		}
		return FALSE;
	}

} ARGPARAM;

BOOL ParseCommandLine(ARGPARAM &args)
{
	UINT Maximize = 0;

	extern int __argc;
	extern wchar_t **__wargv;

	if( __argc > 1 )
	{
		//
		// This option may change in the future.
		//
		for(int i = 1; i < __argc; i++)
		{
			PWSTR pArg = __wargv[i];

			if( *pArg == L'-' || *pArg == L'/' )
			{
				pArg++;
				if( _wcsicmp(pArg,L"v") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_VOLUMELIST ) )
						return FALSE;
				}
				else if( _wcsicmp(pArg,L"pd") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_PHYSICALDRIVELIST ) )
						return FALSE;
				}
				else if( _wcsicmp(pArg,L"sd") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_STORAGEDEVICE ) )
						return FALSE;
				}
				else if( _wcsicmp(pArg,L"md") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_MOUNTEDDEVICE ) )
						return FALSE;
				}
				else if( _wcsicmp(pArg,L"d") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_DOSDRIVELIST ) )
						return FALSE;
				}
				else if( _wcsicmp(pArg,L"n") == 0 )
					args.WithoutOpen = TRUE; // no mdi child open
				else if( _wcsicmp(pArg,L"f") == 0 )
					args.Maximize = TRUE;    // maximize
				else if( _wcsicmp(pArg,L"?") == 0 )
					args.Help = TRUE;        // command line help
				else
					return FALSE;
			}
		}
	}

	if( !args.WithoutOpen && args.ConsoleTypeId.GetCount() == 0 )
		args.ConsoleTypeId.Add( VOLUME_CONSOLE_VOLUMELIST );

	return TRUE;
}

//----------------------------------------------------------------------------
//
//  RegisterMDIFrameClass()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
ATOM RegisterMDIFrameClass(HINSTANCE hInstance)
{
	extern LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

	WNDCLASSEX wcex = {0};

	wcex.cbSize        = sizeof(wcex);
	wcex.style         = 0;
	wcex.lpfnWndProc   = WndProc;
	wcex.cbClsExtra	   = 0;
	wcex.cbWndExtra	   = 0;
	wcex.hInstance	   = hInstance;
	wcex.hIcon		   = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcex.hCursor	   = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName  = NULL;
	wcex.lpszClassName = pszWindowClass;
	wcex.hIconSm	   = (HICON)LoadImage(wcex.hInstance, MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,16,16,LR_DEFAULTSIZE);

	return RegisterClassEx(&wcex);
}

//----------------------------------------------------------------------------
//
//  InitInstance()
//
//  PURPOSE: Initialize this instance.
//
//----------------------------------------------------------------------------
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	ARGPARAM args;
	if( !ParseCommandLine(args) )
	{
		MessageBox(NULL,L"Invalid Parameter.",pszTitle,MB_OK|MB_ICONSTOP);
		return 0;
	}

	if( args.Help )
	{
		ShowHelp(NULL);
		return 0;
	}

	HWND hWnd;

	hInst = hInstance;

	hWnd = CreateWindow(pszWindowClass, pszTitle, WS_OVERLAPPEDWINDOW,
				  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	// Load MainFrame menu
	hMainMenu = LoadMenu(_GetResourceInstance(),MAKEINTRESOURCE(IDR_MAINFRAME));
	SetMenu(hWnd,hMainMenu);

	// Open initial MDI child windows
	if( !args.WithoutOpen && args.ConsoleTypeId.GetCount() > 0 )
	{ 
		HWND hwndMDIChild;
		int i,c;
		c = args.ConsoleTypeId.GetSize();
		for(i = 0; i < c; i++)
		{
			hwndMDIChild = OpenMDIChild(hWnd,args.ConsoleTypeId[i],NULL,NULL,args.Maximize ? ((i == (c-1)) ? TRUE : FALSE) : FALSE );
			if( hwndMDIChild && i == 0 )
			{
				//
				// Measures to avoid the phenomenon where the top edge of the window remains black when running on Windows 7 Basic.
				//
				SetWindowPos(hwndMDIChild,0,0,0,0,0,SWP_SHOWWINDOW|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_DRAWFRAME);
			}
		}
	}

	// Save active focus mdi child window.
	hWndActiveMDIChild = MDIGetActive(hWndMDIClient);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

//----------------------------------------------------------------------------
//
//  ExitInstance()
//
//  PURPOSE: Exit this instance.
//
//----------------------------------------------------------------------------
VOID ExitInstance()
{
	if( hMainMenu )
		DestroyMenu(hMainMenu);
	if( hMdiMenu )
		DestroyMenu(hMdiMenu);
}

//----------------------------------------------------------------------------
//
//  QueryCmdState()
//
//  PURPOSE: Query command status.
//
//----------------------------------------------------------------------------
INT CALLBACK QueryCmdState(UINT CmdId,PVOID,LPARAM)
{
	HWND hwndMDIChild = MDIGetActive(hWndMDIClient);
	if( hwndMDIChild )
	{
		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
		UINT State = 0;
		if( SendMessage(pd->hWndView,WM_QUERY_CMDSTATE,MAKEWPARAM(CmdId,0),(LPARAM)&State) )
		{
			return State;
		}
	}

	switch( CmdId )
	{
		case ID_STORAGEDEVICE:
		case ID_MOUNTEDDEVICE:
		case ID_VOLUMELIST:
		case ID_PHYSICALDRIVELIST:
		case ID_VOLUMESHADOWCOPY:
		case ID_MSDOSDRIVES:
		case ID_FILTERDRIVER:
		case ID_ABOUT:
		case ID_EXIT:
			return UPDUI_ENABLED;
		case ID_WINDOW_CASCADE:
		case ID_WINDOW_TILE_HORZ:
		case ID_WINDOW_TILE_VERT:
		case ID_FILE_CLOSE:
			if( hwndMDIChild != NULL )
				return UPDUI_ENABLED;
			break;
	}

	return UPDUI_DISABLED;
}

//----------------------------------------------------------------------------
//
//  WndProc()
//
//  PURPOSE: Main frame window procedure.
//
//----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE: 
		{
			hWndMDIClient = CreateMDIClient(hWnd);
			FindText_Initialize();
			break; 
		} 
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_SIZE:
		{
			// Resizes the MDI client window to fit in the new frame window's client area. 
			// If the frame window procedure sizes the MDI client window to a different size, 
			// it should not pass the message to the DefWindowProc function.
			int cx = GET_X_LPARAM(lParam);
			int cy = GET_Y_LPARAM(lParam);
			SetWindowPos(hWndMDIClient,NULL,0,0,cx,cy,SWP_NOZORDER);
			break;
		}
		case WM_COMMAND:
		{
			int wmId, wmEvent;
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			switch (wmId)
			{
				case ID_VOLUMELIST:
					OpenConsole(hWnd,VOLUME_CONSOLE_VOLUMELIST);
					break;
				case ID_PHYSICALDRIVELIST:
					OpenConsole(hWnd,VOLUME_CONSOLE_PHYSICALDRIVELIST);
					break;
				case ID_STORAGEDEVICE:
					OpenConsole(hWnd,VOLUME_CONSOLE_STORAGEDEVICE);
					break;
				case ID_MOUNTEDDEVICE:
					OpenConsole(hWnd,VOLUME_CONSOLE_MOUNTEDDEVICE);
					break;
				case ID_VOLUMESHADOWCOPY:
					OpenConsole(hWnd,VOLUME_CONSOLE_SHADOWCOPYLIST);
					break;
				case ID_MSDOSDRIVES:
					OpenConsole(hWnd,VOLUME_CONSOLE_DOSDRIVELIST);
					break;
				case ID_FILTERDRIVER:
					OpenConsole(hWnd,VOLUME_CONSOLE_FILTERDRIVER);
					break;
				case ID_FILE_CLOSE:
					CloseConsole();
					break;
				case ID_WINDOW_CASCADE:
					SendMessage(hWndMDIClient,WM_MDICASCADE,MDITILE_SKIPDISABLED|MDITILE_ZORDER,0);
					break;
				case ID_WINDOW_TILE_HORZ:
					SendMessage(hWndMDIClient,WM_MDITILE,MDITILE_SKIPDISABLED|MDITILE_HORIZONTAL,0);
					break;
				case ID_WINDOW_TILE_VERT:
					SendMessage(hWndMDIClient,WM_MDITILE,MDITILE_SKIPDISABLED|MDITILE_VERTICAL,0);
					break;
				case ID_EDIT_FIND:
				case ID_EDIT_FIND_NEXT:
				case ID_EDIT_FIND_PREVIOUS:
				{
					HWND hwndMDIChild = MDIGetActive(hWndMDIClient);
					if( hwndMDIChild )
					{
						MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
						OnFindText_CommandHandler(hWnd,pd->hWndView,wmId);
					}
					break;
				}
				case ID_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				case ID_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
				{
					// Forward command to the active MDI child window.
					HWND hwndMDIChild = MDIGetActive(hWndMDIClient);
					if( hwndMDIChild )
					{
						MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
						SendMessage(pd->hWndView,WM_COMMAND,wParam,lParam);
					}
					break;
				}
			}
			break;
		}
		case WM_MDIACTIVATE:
		{
			hWndActiveMDIChild = (HWND)lParam;

			if( hWndActiveMDIChild == NULL )
			{
				SendMessage(hWndMDIClient,WM_MDISETMENU,(WPARAM)hMainMenu,0);
				DrawMenuBar(hWnd);
			}
			else
			{
				SendMessage(hWndMDIClient,WM_MDISETMENU,(WPARAM)hMdiMenu,0);
				DrawMenuBar(hWnd);
			}
			break;
		}
		case WM_INITMENUPOPUP:
		{
			HMENU hMenu = (HMENU)wParam;
			INT RelativePosition = (INT)LOWORD(lParam);
			INT WindowMenu = HIWORD(lParam);

			// ignores system menu
			if( GetMenuPosFromID(hMenu,SC_SIZE) != -1 )
				break;

			UpdateUI_MenuItem(hMenu,&QueryCmdState,0);

			break;
		}
		case WM_OPEM_MDI_CHILDFRAME:
		{
			if( lParam == 0 )
				return 0;
			UINT ConsoleTypeId = (UINT)LOWORD(wParam);
			UINT CallType = (UINT)HIWORD(wParam);
			PWSTR psz = (PWSTR)lParam;
			switch( CallType )
			{
				case 0:
					OpenConsole(hWnd,ConsoleTypeId,psz,CallType);
					break;
				case 1:
					OpenConsole(hWnd,ConsoleTypeId,psz,CallType);
					CoTaskMemFree(psz);
					break;
				case 2:
					OpenConsole(hWnd,ConsoleTypeId,(PCWSTR)lParam,CallType);
					break;
			}
			return 0;
		}
		case WM_MDI_CHILDFRAME_CLOSE:
		{
			HWND hwndChildFrame = (HWND)wParam;
			if( hwndChildFrame )
			{
				MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndChildFrame,GWLP_USERDATA);
				ClearWindowHandle(pd->wndId,hwndChildFrame);
			}
			return 0;
		}
		default:
		{
			HWND hwndChildFrame = MDIGetActive(hWndMDIClient);
			if( hwndChildFrame )
			{
				MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndChildFrame,GWLP_USERDATA);
				if( IsFindTextEventMessage(pd->hWndView,message,lParam) )
					return 0;
			}
			break;
		}
	}

	return DefFrameProc(hWnd, hWndMDIClient, message, wParam, lParam);
}

//----------------------------------------------------------------------------
//
//  WinMain()
//
//  PURPOSE: Main procedure.
//
//----------------------------------------------------------------------------
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	_wsetlocale(LC_ALL,L"");

	_MemInit();

	RegisterMDIFrameClass(hInstance);
	RegisterMDIChildFrameClass(hInstance);

	InitializeLibMisc(hInstance,GetUserDefaultUILanguage());

	if( (hWndMain = InitInstance (hInstance, nCmdShow)) == NULL )
	{
		return FALSE;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

	MSG msg;
	INT ret;
	MSG msgDelay = {0};

	while ((ret = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0)
	{
		if( ret == -1 )
	    {
		    break; // handle the error and possibly exit
	    }
		else 
	    { 

#if 1
		// This code is follows reason:
		//
		// 1. Receives a notify message LVN_ITEMACTIVATE to the MDI child window 
		//    when occurs  WM_LBUTTONDOWN->WM_LBUTTONDBLCLK on the ListVew contorol.
		//    (WM_LBUTTONUP is not notified yet.)
		// 2. During LVN_ITEMACTIVATE processing, create new MDI child frame 
		//    and to activate.
		// 3. When LVN_ITEMACTIVATE returns, at that time the active window has changed.
		// 4. It then receives WM_LBUTTONUP. But this message sending to 
		//    the newly created window instead of the previous window.
		// 5. At this moment may case result in unexpected selection operations 
		//    on the new window (e.g. Group header click select).

			if( (WM_MOUSEFIRST <= msg.message && msg.message <= WM_MOUSELAST) && (msg.message != WM_MOUSEMOVE) )
			{
				if( msg.message == WM_LBUTTONUP && msgDelay.hwnd != NULL )
				{
					SendMessage(GetActiveWindow(),WM_OPEM_MDI_CHILDFRAME,msgDelay.wParam,msgDelay.lParam);
					ZeroMemory(&msgDelay,sizeof(msgDelay));
				}
			}
			else if( msg.message == WM_OPEM_MDI_CHILDFRAME ) // post message mode
			{
				msgDelay.hwnd = msg.hwnd;
				msgDelay.lParam = msg.lParam;
				msgDelay.wParam = msg.wParam;
				continue;
			}
#endif

			if( hWndActiveMDIChild )
			{
				// Forward message to active view on MDI child window.
				MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hWndActiveMDIChild,GWLP_USERDATA);

				if( pd && IsWindow(pd->hWndView) && SendMessage(pd->hWndView,WM_PRETRANSLATEMESSAGE,0,(LPARAM)&msg) != 0 )
					continue;

				if( IsFindTextDialogMessage(&msg) )
					continue;
			}

			if( (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) && (msg.wParam == VK_RETURN) )
			{
				// prevent ring beep when press enter key on tree-view.
				DispatchMessage(&msg);
				continue;
			}

		    if (!TranslateMDISysAccel(hWndMDIClient, &msg) && 
			    !TranslateAccelerator(hWndMain, hAccelTable, &msg))
	        { 
				if( hWndActiveMDIChild )
				{
					if( IsDialogMessage(hWndActiveMDIChild,&msg) )
						continue;
				}
		        TranslateMessage(&msg); 
			    DispatchMessage(&msg); 
			} 
		} 
	}

	ExitInstance();

	_MemEnd();

	return (int) msg.wParam;
}
