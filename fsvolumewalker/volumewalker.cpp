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

static HINSTANCE hInst = NULL;
HWND hWndMain = NULL;
HWND hWndMDIClient = NULL;
HWND hWndActiveMDIChild = NULL;
static WCHAR *pszTitle       = L"FSVolumeWalker";
static WCHAR *pszWindowClass = L"FSVolumeWalkerMDIFrameWindowClass";
static HMENU hMainMenu = NULL;
static HMENU hMdiMenu = NULL;

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

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
		{
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

				HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT);

				StringCchPrintf(szBuf,_countof(szBuf),L"System Boot Time : %s",szBootTime);
				Edit_AddText(hwndEdit,szBuf);

				Edit_AddText(hwndEdit,L"\r\n");

				StringCchPrintf(szBuf,_countof(szBuf),L"Boot Elapsed Time : %s",szBootElapsedTime);
				Edit_AddText(hwndEdit,szBuf);
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

// A generic programming is using stl::map<>;
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


//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  OpenMDIChild()
//
//  PURPOSE: Open MDI child window.
//
//----------------------------------------------------------------------------
HWND OpenMDIChild(HWND hWnd,UINT ConsoleTypeId,PCWSTR pszPath,BOOL bMaximize=FALSE)
{
	HWND hwndChildFrame = NULL;

	if( ConsoleTypeId == VOLUME_CONSOLE_DISKLAYOUT || 
		ConsoleTypeId == VOLUME_CONSOLE_VOLUMEINFORMAION || 
		ConsoleTypeId == VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION )
	{
		hwndChildFrame = FindSameWindowTitle(ConsoleTypeId,pszPath);
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

		if( bMaximized )
		{
			SetRedraw(hWndMDIClient,TRUE);
			RedrawWindow(hwndChildFrame,0,0,RDW_UPDATENOW|RDW_INVALIDATE|RDW_ALLCHILDREN);
		}

		return hwndChildFrame;
	}

	MDICREATEPARAM mcp;
	if( pszPath )
		mcp.pszInitialPath = pszPath;
	else
		mcp.pszInitialPath = NULL;

	if( ConsoleTypeId == VOLUME_CONSOLE_SHADOWCOPYLIST )
		mcp.hIcon = GetDeviceClassIcon(DEVICE_ICON_VOLUMESNAPSHOT,NULL);
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
			pd->hWndView = CreateVolumeConsoleWindow(hwndMDIChild,ConsoleTypeId,&param);

			RECT rc;
			GetClientRect(hwndMDIChild,&rc);

			SetWindowPos(pd->hWndView,NULL,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE|SWP_HIDEWINDOW);

			SendMessage(pd->hWndView,WM_CONTROL_MESSAGE,UI_INIT_LAYOUT,(LPARAM)&rc);

			SELECT_ITEM sel = {0};
			if( VOLUME_CONSOLE_VOLUMEINFORMAION == ConsoleTypeId || VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION == ConsoleTypeId || VOLUME_CONSOLE_DISKLAYOUT == ConsoleTypeId )
			{
				sel.pszName = (PWSTR)pszPath;
				sel.pszPath = (PWSTR)pszPath;
				sel.ViewType = ConsoleTypeId;
			}

			SendMessage(pd->hWndView,WM_NOTIFY_MESSAGE,UI_NOTIFY_VOLUME_SELECTED,(LPARAM)&sel);

			ShowWindow(pd->hWndView,SW_SHOW);

			if( pszPath )
				SetWindowText(hwndMDIChild,pszPath);
			else
				SetWindowText(hwndMDIChild,pszTitle);

			SendMessage(hWndMDIClient,WM_MDIREFRESHMENU,0,0);
			DrawMenuBar(hWnd);
		}
	}

	return hwndMDIChild;
}

VOID OpenConsole(HWND hWnd,UINT ConsoleTypeId,PCWSTR psz=NULL)
{
	HWND hwndChild = OpenMDIChild(hWnd,ConsoleTypeId,psz);
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
	CSimpleValArray<UINT> ConsoleTypeId;
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

	// Show frame window
//	ShowWindow(hWnd, nCmdShow);
//	UpdateWindow(hWnd);

	// Open initial MDI child windows
	if( !args.WithoutOpen && args.ConsoleTypeId.GetCount() > 0 )
	{ 
		HWND hwndMDIChild;
		int i,c;
		c = args.ConsoleTypeId.GetSize();
		for(i = 0; i < c; i++)
		{
			hwndMDIChild = OpenMDIChild(hWnd,args.ConsoleTypeId[i],NULL,args.Maximize ? ((i == (c-1)) ? TRUE : FALSE) : FALSE );
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
			UINT ConsoleTypeId = (UINT)LOWORD(wParam);
			UINT Async = (UINT)HIWORD(wParam);
			PWSTR psz = (PWSTR)lParam;
			if( psz )
			{
				OpenConsole(hWnd,ConsoleTypeId,psz);
				if( Async )
					CoTaskMemFree(psz);
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
