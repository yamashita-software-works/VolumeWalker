//****************************************************************************
//*                                                                          *
//*  volumewalker.cpp                                                        *
//*                                                                          *
//*  PURPOSE:                                                                *
//*    Implements the main procedure and main frame window.                  *
//*                                                                          *
//*  AUTHOR:                                                                 *
//*    YAMASHITA Katsuhiro                                                   *
//*                                                                          *
//*  HISTORY:                                                                *
//+    2022.04.02 SDI frame ver created.                                     *
//*    2022.12.24 MDI frame ver with based on VC++ generated code created.   *
//*    2023.02.24 MDI frame ver with new main frame created.                 *
//*    2024.04.15 Experimentally implemented Volume Contents Console.        *
//*    2024.10.24 Volume Contents Console has been obsoleted.                *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumewalker.h"
#include "mdichild.h"
#include "resource.h"
#include "find.h"
#include "inifile.h"

static HINSTANCE g_hInst = NULL;
static HWND g_hWndMain = NULL;
static HWND g_hWndMDIClient = NULL;
static HWND g_hWndActiveMDIChild = NULL;
static WCHAR *g_pszMainFrameTitle = L"VolumeWalker";
static WCHAR *g_pszWindowClass    = L"VolumeWalkerMDIFrameWindowClass";
static HMENU g_hMainMenu = NULL;
static HMENU g_hMdiMenu = NULL;
static LCID g_lcid = LOCALE_USER_DEFAULT;
static LANGID g_langId = LANG_USER_DEFAULT;
#ifdef _DEBUG
static PCWSTR g_pszIniFileName = L"";  // for debug only
#else
static PCWSTR g_pszIniFileName = L"";
#endif
static BOOL g_bEnableWorkspaceLayout = FALSE;

#define _LAYOUT_FILENAME L"layout.ini"

HINSTANCE _GetResourceInstance()
{
	return g_hInst;
}

HWND _GetMainWnd()
{
	return g_hWndMain;
}

MDICHILDWNDDATA *GetMDIChildWndData(HWND hwndMDIChildFrame)
{
	return (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChildFrame,GWLP_USERDATA);
}

CONSOLE_VIEW_ID *GetMDIConsoleId(HWND hwndMDIChildFrame)
{
	MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChildFrame,GWLP_USERDATA);
	ASSERT(pd != NULL);
	ASSERT(pd->hWndView != NULL);
	CONSOLE_VIEW_ID *pcv = (CONSOLE_VIEW_ID *)GetProp(pd->hWndView,_PROP_CONSOLE_VIEW_ID);
	ASSERT(pcv != NULL);
	return pcv;
}

CONSOLE_VIEW_ID *GetConsoleId(HWND hwndViewWindow)
{
	CONSOLE_VIEW_ID *pcv = (CONSOLE_VIEW_ID *)GetProp(hwndViewWindow,_PROP_CONSOLE_VIEW_ID);
	ASSERT(pcv != NULL);
	return pcv;
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

	DWORD cb;

	cb = MAX_PATH;
	SHRegGetValue(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",L"ProductName",SRRF_RT_REG_DWORD,NULL,szText,&cb);

	Edit_AddText(hwndEdit,L"  ");
	Edit_AddText(hwndEdit,szText);
	Edit_AddText(hwndEdit,L"\r\n");


	cb = sizeof(DWORD);
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
				StringCchPrintf(sz,_countof(sz),L"%u.%u.%u.%u (%s) Preview",MEJOR_VERSION,MINOR_VERSION,BUILD_NUMBER,PATCH_NUMBER,
#ifdef _WIN64
					L"x64"
#else
					L"x86"
#endif
						);
				SetDlgItemText(hDlg,IDC_TEXT,sz);
			}

			// OS version:
			{
				Edit_AddText(hwndEdit,L"System Information:\r\n");
				OSVersionText(GetDlgItem(hDlg,IDC_EDIT));
			}

			// miscellaneous/system information:
			{
				Edit_AddText(hwndEdit,L"\r\n\r\n");
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

				StringCchPrintf(szBuf,_countof(szBuf),L"System Boot Time: %s",szBootTime);
				Edit_AddText(hwndEdit,szBuf);

				Edit_AddText(hwndEdit,L"\r\n");

				StringCchPrintf(szBuf,_countof(szBuf),L"Boot Elapsed Time: %s",szBootElapsedTime);
				Edit_AddText(hwndEdit,szBuf);
			}

			// System Install Date
			{
				LARGE_INTEGER t = {0};

				if( GetComputerInformation(9,CIF_REG_WOW64_64KEY,&t.QuadPart,sizeof(t.QuadPart)) != ERROR_SUCCESS )
				{
					DWORD dw;
					GetComputerInformation(6,CIF_REG_WOW64_64KEY,&dw,sizeof(dw));

					SecondsSince1970ToTime(dw,&t);
				}

				WCHAR sz[64];
				_GetDateTimeStringEx(t.QuadPart,sz,64,NULL,NULL,FALSE);

				Edit_AddText(hwndEdit,L"\r\n");
				Edit_AddText(hwndEdit,L"System Install Date: ");
				Edit_AddText(hwndEdit,sz);
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

HRESULT InitLanguage(LPWSTR pszLangIdOrName)
{
	ULONG lang = wcstoul(pszLangIdOrName,NULL,0);
	g_langId = MAKELANGID(lang,SUBLANG_DEFAULT);
	SetThreadUILanguage(g_langId);
	g_lcid = MAKELCID(MAKELANGID(g_langId,SUBLANG_DEFAULT),SORT_DEFAULT);
	SetThreadLocale(g_lcid);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////

/*++

   Consoles

    Single Instance Console:

      - VOLUME_CONSOLE_HOME (Reserved)
      - VOLUME_CONSOLE_VOLUMELIST
      - VOLUME_CONSOLE_PHYSICALDRIVELIST
      - VOLUME_CONSOLE_SHADOWCOPYLIST
      - VOLUME_CONSOLE_STORAGEDEVICE
      - VOLUME_CONSOLE_MOUNTEDDEVICE
      - VOLUME_CONSOLE_DOSDRIVELIST
      - VOLUME_CONSOLE_FILTERDRIVER

    These consoles are allow create instances per volume:

      - VOLUME_CONSOLE_VOLUMEINFORMAION
      - VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION
      - VOLUME_CONSOLE_DISKLAYOUT
      - VOLUME_CONSOLE_DISKPERFORMANCE
      - VOLUME_CONSOLE_FILESYSTEMSTATISTICS
      - VOLUME_CONSOLE_SIMPLEHEXDUMP
      - VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST

--*/
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
	hwnd = GetWindow(g_hWndMDIClient,GW_CHILD);
	if( hwnd )
	{
		WCHAR szTitle[MAX_PATH];
		do
		{
			CONSOLE_VIEW_ID *pcv = GetMDIConsoleId(hwnd);
			if( pcv->wndId == ConsoleType )
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

VOID MakeConsoleGUID(LPGUID pwndGuid,UINT ConsoleId,OPEN_MDI_CHILDFRAME_PARAM *OptionParameter)
{
	GUID Guid = {0};

	switch( ConsoleId )
	{
		case VOLUME_CONSOLE_HOME:
		case VOLUME_CONSOLE_VOLUMELIST:
		case VOLUME_CONSOLE_PHYSICALDRIVELIST:
		case VOLUME_CONSOLE_SHADOWCOPYLIST:
		case VOLUME_CONSOLE_STORAGEDEVICE:
		case VOLUME_CONSOLE_MOUNTEDDEVICE:
		case VOLUME_CONSOLE_DOSDRIVELIST:
		case VOLUME_CONSOLE_FILTERDRIVER:
			Guid.Data1 = (ULONG)ConsoleId;
			break;
		case VOLUME_CONSOLE_VOLUMEINFORMAION:
		case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
		case VOLUME_CONSOLE_DISKLAYOUT:
		case VOLUME_CONSOLE_DISKPERFORMANCE:
		case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
		case VOLUME_CONSOLE_SIMPLEHEXDUMP:
		case VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST:
		{
			Guid.Data1 = (ULONG)ConsoleId;
			GetSystemTimeAsFileTime( (LPFILETIME)Guid.Data4 );
			break;
		}
	}

	*pwndGuid = Guid;
}

BOOL IsVolumeNameRequiredConsole(LPGUID pwndGuid,UINT ConsoleId)
{
	GUID Guid = {0};

	switch( ConsoleId )
	{
		case VOLUME_CONSOLE_VOLUMEINFORMAION:
		case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
		case VOLUME_CONSOLE_DISKLAYOUT:
		case VOLUME_CONSOLE_DISKPERFORMANCE:
		case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
		case VOLUME_CONSOLE_SIMPLEHEXDUMP:
			return TRUE;
	}
	return FALSE;
}

PCWSTR GetConsoleTitle(UINT ConsoleTypeId)
{
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

	return pszTitle;
}

HICON GetConsoleIcon(UINT ConsoleTypeId,PCWSTR pszInitialPath)
{
	HICON hIcon = NULL;
	if( ConsoleTypeId == VOLUME_CONSOLE_SHADOWCOPYLIST )
	{
		hIcon = GetDeviceClassIcon(DEVICE_ICON_VOLUMESNAPSHOT,NULL);
	}
	else if( ConsoleTypeId == VOLUME_CONSOLE_SIMPLEHEXDUMP )
	{
		hIcon = (HICON)LoadImage(_GetResourceInstance(),MAKEINTRESOURCE(IDI_BINDUMP),IMAGE_ICON,16,16,LR_DEFAULTSIZE);
	}
	else if( ConsoleTypeId == VOLUME_CONSOLE_FILTERDRIVER )
	{
		HINSTANCE hmod = LoadLibrary(L"imageres.dll");
		hIcon = (HICON)LoadImage(hmod,MAKEINTRESOURCE(67),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
		FreeLibrary(hmod);
	}
	else if( ConsoleTypeId == VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST )
	{
		hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
	}
	else
	{
		hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
	}
	return hIcon;
}

BOOL IsValidDevicePath( PWSTR Path )
{
	HANDLE Handle;
	WCHAR szDeviceName[MAX_PATH];

	if( wcschr(Path,L'\\') )
		StringCchCopy(szDeviceName,MAX_PATH,Path);
	else
		StringCchPrintf(szDeviceName,MAX_PATH,L"\\??\\%s",Path);

	if( OpenFile_W(&Handle,NULL,szDeviceName,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_NON_DIRECTORY_FILE) == 0 )
	{
		CloseHandle(Handle);
		return TRUE;
	}

#ifdef _DEBUG
	DbgPrint("Fail : %S\n",szDeviceName);
#endif

	return FALSE;
}

void InitConfigFile()
{
	PWSTR pszConfogFileName = _MemAllocStringBuffer( MAX_PATH );
	GetModuleFileName(NULL,pszConfogFileName,MAX_PATH);
	PathRenameExtension(pszConfogFileName,L".ini");
	_SetConfigFileName(pszConfogFileName);
	_SafeMemFree(pszConfogFileName);
}

void InitIniFile()
{
	PWSTR pszIniFileName;
	if( PathFileExists(g_pszIniFileName) )
	{
		pszIniFileName = _MemAllocString(g_pszIniFileName);
	}
	else
	{
		pszIniFileName = _MemAllocStringBuffer( MAX_PATH );
		GetModuleFileName(NULL,pszIniFileName,MAX_PATH);
		PathRemoveFileSpec(pszIniFileName);
		PathCombine(pszIniFileName,pszIniFileName,_LAYOUT_FILENAME);
	}
	_SetIniFileName(pszIniFileName);
	_SafeMemFree(pszIniFileName);
}

VOID SendUIInitLayout(HWND hwndMDIChild,HWND hWndView)
{
	RECT rc;
	GetClientRect(hwndMDIChild,&rc);
	SetWindowPos(hWndView,NULL,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE|SWP_HIDEWINDOW);
	SendMessage(hWndView,WM_CONTROL_MESSAGE,UI_INIT_LAYOUT,(LPARAM)&rc);
}

VOID SendHexDumpInformation(HWND hwndMDIChild,HWND hWndView,OPEN_MDI_CHILDFRAME_PARAM *OpenSelItem)
{
	//
	// Hex dump: Set data source, offset, and home offset.
	//
	SELECT_OFFSET_ITEM sel = {0};
	sel.hdr.mask = SI_MASK_PATH|SI_MASK_NAME|SI_MASK_VIEWTYPE|SI_MASK_START_OFFSET;
	sel.hdr.ViewType   = VOLUME_CONSOLE_SIMPLEHEXDUMP;
	sel.hdr.pszVolume  = OpenSelItem->Path;
	sel.liStartOffset  = OpenSelItem->StartOffset;

	SendMessage(hWndView,WM_NOTIFY_MESSAGE,UI_NOTIFY_VOLUME_SELECTED,(LPARAM)&sel);

	SetWindowText(hwndMDIChild,sel.hdr.pszVolume);
}

VOID SendSelectVolumeOrPhysicalDisk(HWND hwndMDIChild,HWND hWndView,UINT ConsoleTypeId,OPEN_MDI_CHILDFRAME_PARAM *pOpenParam)
{
	SELECT_ITEM sel = {0};
	switch( ConsoleTypeId )
	{
		case VOLUME_CONSOLE_VOLUMEINFORMAION:
		case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
		{
			sel.pszVolume = pOpenParam->Path;
			sel.ViewType = ConsoleTypeId;
			break;
		}
		case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
		case VOLUME_CONSOLE_DISKLAYOUT:
		{
			sel.pszPhysicalDrive = pOpenParam->Path;
			sel.ViewType = ConsoleTypeId;
			break;
		}
	}

	SendMessage(hWndView,WM_NOTIFY_MESSAGE,UI_NOTIFY_VOLUME_SELECTED,(LPARAM)&sel);

	if( pOpenParam && pOpenParam->Path )
	{
		SetWindowText(hwndMDIChild,pOpenParam->Path);
	}
	else
	{
		PCWSTR pszTitle = GetConsoleTitle(ConsoleTypeId);
		SetWindowText(hwndMDIChild,pszTitle);
	}
}

VOID SendFileListPath(HWND hwndMDIChild,UINT ConsoleTypeId,MDICHILDWNDDATA *pd,OPEN_MDI_CHILDFRAME_PARAM *pOpenParam)
{
	ASSERT( pOpenParam != NULL );

	PWSTR pszPath;
	SIZE_T cchPathLength;

	if( pOpenParam == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return;
	}

	if( pOpenParam->Path == NULL )
	{
		WCHAR sz[MAX_PATH];
		GetCurrentDirectory(MAX_PATH,sz);
		pOpenParam->Path = _MemAllocString(sz);
	}

	cchPathLength = wcslen(pOpenParam->Path);

	if( cchPathLength < MAX_PATH && FindRootDirectory_W(pOpenParam->Path,NULL) == 0 )
	{
		pszPath = _MemAllocString(pOpenParam->Path);
	}
	else
	{
		pszPath = _MemAllocStringCat(pOpenParam->Path,L"\\");
	}

	UIS_PAGE pg = {0};
	pg.ConsoleTypeId = ConsoleTypeId;
	pg.ConsoleGuid   = pd->wndGuid;
	pg.pszPath       = pszPath;
	pg.pszFileName   = NULL;
	SendMessage(pd->hWndView,WM_CONTROL_MESSAGE,UI_SELECT_PAGE,(LPARAM)&pg);

	// set MDI child frame title
	WCHAR szVolumeName[128];
	NtPathGetVolumeName(pszPath,szVolumeName,ARRAYSIZE(szVolumeName));
	SetWindowText(hwndMDIChild,szVolumeName);

	_MemFree(pszPath);
}

//----------------------------------------------------------------------------
//
//  OpenMDIChild()
//
//  PURPOSE: Open MDI child window.
//
//----------------------------------------------------------------------------
/*++
  VolumeWalker Window layer

  MainFrame                             -+
   |                                     |
   +- MDI client                         |- exe
       |                                 |
       +- MDI child frame               -+
           |
           +- Window(View host)         -+
               |                         |
               +- View(Page host)        |- dll
                   |                     |
                   +- Page              -+
--*/

_inline void InitViewType(MDICHILDWNDDATA *pd,UINT ConsoleId,GUID *pwndGuid)
{
	ASSERT( pd->hWndView != NULL );

	if( pd->hWndView )
	{
		CONSOLE_VIEW_ID *pcv = new CONSOLE_VIEW_ID;
		pcv->wndId   = ConsoleId;
		pcv->wndGuid = *pwndGuid;
		SetProp(pd->hWndView,_PROP_CONSOLE_VIEW_ID,(HANDLE)pcv);
	}
}

HWND OpenMDIChild(HWND hWnd,UINT ConsoleTypeId,LPGUID pwndGuid,OPEN_MDI_CHILDFRAME_PARAM *pOpenParam,BOOL bMaximize,MDICHILDFRAMEINIT *pmdiInit)
{
	HWND hwndChildFrame = NULL;

	if( ConsoleTypeId == VOLUME_CONSOLE_DISKLAYOUT || 
		ConsoleTypeId == VOLUME_CONSOLE_VOLUMEINFORMAION || 
		ConsoleTypeId == VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION ||
		ConsoleTypeId == VOLUME_CONSOLE_FILESYSTEMSTATISTICS ||
		ConsoleTypeId == VOLUME_CONSOLE_SIMPLEHEXDUMP ||
		ConsoleTypeId == VOLUME_CONSOLE_DISKPERFORMANCE || 
		ConsoleTypeId == VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST )
	{
		hwndChildFrame = FindSameWindowTitle(ConsoleTypeId,(pOpenParam != NULL && pOpenParam->Path != NULL) ? pOpenParam->Path : NULL);
	}
	else
	{
		hwndChildFrame = GetOpenedWindowHandle(ConsoleTypeId);
	}

	if( hwndChildFrame != NULL )
	{
		//
		// Already opened MDI child frame.
		//
		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndChildFrame,GWLP_USERDATA);

		BOOL bMaximized;
		MDIGetActive(g_hWndMDIClient,&bMaximized);

		if( bMaximized )
		{
			SetRedraw(g_hWndMDIClient,FALSE);
		}

		SendMessage(g_hWndMDIClient,WM_MDIACTIVATE,(WPARAM)hwndChildFrame,0);

		if( VOLUME_CONSOLE_SIMPLEHEXDUMP == ConsoleTypeId && pOpenParam )
		{
			SendHexDumpInformation(hwndChildFrame,pd->hWndView,pOpenParam);
		}

		if( bMaximized )
		{
			SetRedraw(g_hWndMDIClient,TRUE);
			RedrawWindow(hwndChildFrame,0,0,RDW_UPDATENOW|RDW_INVALIDATE|RDW_ALLCHILDREN);
		}

		return hwndChildFrame;
	}

	MDICREATEPARAM mcp;
	if( pOpenParam )
		mcp.pszInitialPath = pOpenParam->Path;
	else
		mcp.pszInitialPath = NULL;

	mcp.hIcon = GetConsoleIcon(ConsoleTypeId,mcp.pszInitialPath);

	MDICHILDFRAMEINIT mcinit = { {CW_USEDEFAULT,CW_USEDEFAULT},{CW_USEDEFAULT,CW_USEDEFAULT}, 0 }; // reserved

	BOOL bVisible = IsWindowVisible(hWnd);

	if( bVisible )
		SetRedraw(hWnd,FALSE); // When new MDI child window open maximizing, prevents the mainframe menu flickering.

	SetRedraw(g_hWndMDIClient,FALSE);

	//
	// Open MDI child frame window
	//
	HWND hwndMDIChild = CreateMDIChildFrame(g_hWndMDIClient,NULL,pmdiInit,(LPARAM)&mcp,bMaximize);

	if( bVisible )
	{
		SetRedraw(hWnd,TRUE);  // Reverd redraw state.
		RedrawWindow(hWnd,0,0,RDW_UPDATENOW|RDW_INVALIDATE|RDW_ALLCHILDREN); // Refresh all windows
	}

	if( hwndMDIChild )
	{
		if( bVisible )
			SetWindowPos(hwndMDIChild,0,0,0,0,0,SWP_SHOWWINDOW|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_DRAWFRAME);

		if( g_hMdiMenu == NULL )
			g_hMdiMenu = LoadMenu(_GetResourceInstance(),MAKEINTRESOURCE(IDR_MDICHILDFRAME));

		SendMessage(g_hWndMDIClient,WM_MDISETMENU,(WPARAM)g_hMdiMenu,0);
		DrawMenuBar(hWnd);

		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
		{
			SetWindowHandle(ConsoleTypeId,hwndMDIChild);

			VOLUME_CONSOLE_CREATE_PARAM param = {0};
			param.pszReserved = (PWSTR)mcp.pszInitialPath;

			//
			// Open Console View Window
			//
			switch( ConsoleTypeId )
			{
				case VOLUME_CONSOLE_SIMPLEHEXDUMP:
				{
					pd->hWndView = CreateVolumeConsoleWindow(hwndMDIChild,ConsoleTypeId,&param);

					InitViewType(pd,ConsoleTypeId,pwndGuid);

					SendUIInitLayout(hwndMDIChild,pd->hWndView);

					SendHexDumpInformation(hwndMDIChild,pd->hWndView,pOpenParam);
					break;
				}
				case VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST:
				{
					pd->hWndView = CreateVolumeFileList(hwndMDIChild,ConsoleTypeId,0,0);

					InitViewType(pd,ConsoleTypeId,pwndGuid);

					SendUIInitLayout(hwndMDIChild,pd->hWndView);

					SendFileListPath(hwndMDIChild,ConsoleTypeId,pd,pOpenParam);
					break;
				}
				default:
				{
					pd->hWndView = CreateVolumeConsoleWindow(hwndMDIChild,ConsoleTypeId,&param);

					InitViewType(pd,ConsoleTypeId,pwndGuid);

					SendUIInitLayout(hwndMDIChild,pd->hWndView);

					SendSelectVolumeOrPhysicalDisk(hwndMDIChild,pd->hWndView,ConsoleTypeId,pOpenParam);
					break;
				}
			}

			ASSERT( pd->hWndView != NULL );

			if( pd->hWndView )
			{
				ShowWindow(pd->hWndView,SW_SHOW);
			}

			SendMessage(g_hWndMDIClient,WM_MDIREFRESHMENU,0,0);
			DrawMenuBar(hWnd);
		}
	}

	SetRedraw(g_hWndMDIClient,TRUE);
	RedrawWindow(g_hWndMDIClient,NULL,NULL,RDW_FRAME|RDW_UPDATENOW|RDW_INVALIDATE|RDW_ERASE|RDW_ERASENOW|RDW_ALLCHILDREN);

	return hwndMDIChild;
}

HWND OpenConsole(HWND hWnd,UINT ConsoleTypeId,LPGUID pwndGuid=NULL,OPEN_MDI_CHILDFRAME_PARAM *pOpenParam=NULL,BOOL bMaximize=FALSE)
{
	GUID wndGuid = {0};

	if( pwndGuid == NULL || IsEqualGUID(GUID_NULL,*pwndGuid) )
	{
		// assign new window GUID
		MakeConsoleGUID(&wndGuid,ConsoleTypeId,pOpenParam);
	}

	HWND hwndMDIChildFrame = OpenMDIChild(
								hWnd,ConsoleTypeId,&wndGuid,pOpenParam,
								bMaximize,NULL);

	if( hwndMDIChildFrame )
	{
		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChildFrame,GWLP_USERDATA);
		SetFocus(pd->hWndView);
	}

	return hwndMDIChildFrame;
}

VOID CloseConsole(HWND hwndMDIChildFrame=NULL)
{
	SendMessage(g_hWndMDIClient, WM_MDIDESTROY,
		(WPARAM)(hwndMDIChildFrame ? hwndMDIChildFrame :MDIGetActive(g_hWndMDIClient)),
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
			L"Command Line Syntax\n"
			L"\n"
			L"Options: [/v | /pd | /sd | /md | /sd | /d | [/f]] | [/we]\n"
			L"\n"
			L"/v : Open Volume List.\n"
			L"/pd : Open Physical Drive List.\n"
			L"/sd : Open Storage Device List.\n"
			L"/md : Open Mounted Device List.\n"
			L"/sc : Select Shadow Copy Volume List.\n"
			L"/d : Open MS-DOS Drive List.\n"
			L"\n"
			L"/f : Open maximized in the MDI client area.\n"
			L"\n"
			L"/we : Enable workspace, save/load window layout.";
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
	INT EnableWorkspaceLayout;
	INT Help;
	WCHAR szLangIdOrName[LOCALE_NAME_MAX_LENGTH];

	_ARGPARAM()
	{
		Maximize = FALSE;
		WithoutOpen = FALSE;
		EnableWorkspaceLayout = FALSE;
		Help = FALSE;
		ConsoleBit = 0;
		ZeroMemory(szLangIdOrName,sizeof(szLangIdOrName));
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
				else if( _wcsicmp(pArg,L"sc") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_SHADOWCOPYLIST ) )
						return FALSE;
				}
				else if( _wcsicmp(pArg,L"d") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_DOSDRIVELIST ) )
						return FALSE;
				}
				else if( _wcsicmp(pArg,L"we") == 0 )
				{
					args.EnableWorkspaceLayout = TRUE;
				}
				else if( _wcsicmp(pArg,L"n") == 0 )
				{
					args.WithoutOpen = TRUE; // no mdi child open
				}
				else if( _wcsicmp(pArg,L"f") == 0 )
				{
					args.Maximize = TRUE;    // Open with maximize MDI child frame
				}
				else if( _wcsicmp(pArg,L"langid") == 0 )
				{
					if( (i + 1) < __argc )
					{
						StringCchCopy(args.szLangIdOrName,ARRAYSIZE(args.szLangIdOrName),__wargv[++i]);
					}
					else
					{
						return FALSE;
					}
				}
				else if( _wcsicmp(pArg,L"?") == 0 )
				{
					args.Help = TRUE;        // command line help
				}
				else
				{
					return FALSE;
				}
			}
		}
	}

	if( !args.WithoutOpen && 
		(args.ConsoleTypeId.GetCount() == 0) && 
		(!args.EnableWorkspaceLayout || (args.EnableWorkspaceLayout && !HasValidIniFile())) )
	{
		args.ConsoleTypeId.Add( VOLUME_CONSOLE_VOLUMELIST );
	}

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
	wcex.lpszClassName = g_pszWindowClass;
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
	g_hInst = hInstance;

	ARGPARAM args;
	if( !ParseCommandLine(args) )
	{
		MessageBox(NULL,L"Invalid Parameter.",g_pszMainFrameTitle,MB_OK|MB_ICONSTOP);
		return 0;
	}

	if( args.Help )
	{
		ShowHelp(NULL);
		return 0;
	}

	if( args.szLangIdOrName[0] )
		InitLanguage(args.szLangIdOrName);

	InitializeVolumeConsole(0);

	HWND hWnd;
	hWnd = CreateWindow(g_pszWindowClass, g_pszMainFrameTitle, WS_OVERLAPPEDWINDOW,
				  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	// Load MainFrame menu
	g_hMainMenu = LoadMenu(_GetResourceInstance(),MAKEINTRESOURCE(IDR_MAINFRAME));
	SetMenu(hWnd,g_hMainMenu);

	// Open initial MDI child windows
	if( !args.WithoutOpen )
	{
		if( args.ConsoleTypeId.GetCount() > 0 )
		{ 
			HWND hwndMDIChild;
			int i,c;
			c = args.ConsoleTypeId.GetSize();
			for(i = 0; i < c; i++)
			{
				hwndMDIChild = OpenConsole(hWnd,args.ConsoleTypeId[i],NULL,NULL,args.Maximize ? ((i == (c-1)) ? TRUE : FALSE) : FALSE );

				if( hwndMDIChild && i == 0 )
				{
					//
					// Measures to avoid the phenomenon where the top edge of the window remains black when running on Windows 7 Basic.
					//
					SetWindowPos(hwndMDIChild,0,0,0,0,0,SWP_SHOWWINDOW|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_DRAWFRAME);
				}
			}
		}
		else
		{
			//
			// Load mainframe window configuration
			//
			LoadMainFrameConfig(hWnd,&nCmdShow);

			//
			// Load MDI child windows layout
			//
			HRESULT hr;
			MDICHILDFRAMEINIT_DIRFILES *pFrames = NULL;
			DWORD dwFrames;
	
			hr = LoadLayout(hWnd,g_hWndMDIClient,&pFrames,&dwFrames);
			if( hr == S_OK )
			{
				OPEN_MDI_CHILDFRAME_PARAM op = {0};

				HWND hwndMDIChild = NULL;
				for(int i = ((int)dwFrames-1); i >= 0; i--)
				{
					if( IsVolumeNameRequiredConsole(&pFrames[i].Guid,(UINT)pFrames[i].Guid.Data1) && pFrames[i].Path == NULL )
						continue;

/*					switch( pFrames[i].Guid.Data1 ) // console type id
					{
						case VOLUME_CONSOLE_VOLUMEINFORMAION:
						case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
						case VOLUME_CONSOLE_DISKLAYOUT:
						case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
							if( pFrames[i].Path && !IsValidDevicePath(pFrames[i].Path) )
								continue;
							break;
						case 0x1e: // obsoleted contents browser console.
						case 0x1f: // obsoleted contents browser console.
							continue; // prevent open the blank console.
					} */

					op.Path = pFrames[i].Path;
					op.StartOffset.QuadPart = pFrames[i].liStartOffset.QuadPart;

					hwndMDIChild = OpenMDIChild(hWnd,
							(UINT)pFrames[i].Guid.Data1, // console type id
							&pFrames[i].Guid,
							&op,
							pFrames[i].hdr.show == SW_MAXIMIZE ? TRUE : FALSE,
							&pFrames[i].hdr);

					if( hwndMDIChild )
					{
						ShowWindow(hwndMDIChild,pFrames[i].hdr.show);

						//
						// Measures to avoid the phenomenon where the top edge of the window
						// remains black when running on Windows 7 Basic.
						//
						SetWindowPos(hwndMDIChild,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_DRAWFRAME);
					}
				}
				FreeLayout(pFrames,dwFrames);
			}
		}
	}

	g_bEnableWorkspaceLayout = args.EnableWorkspaceLayout; // todo:

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Save active focus mdi child window.
	g_hWndActiveMDIChild = MDIGetActive(g_hWndMDIClient);
	if( g_hWndActiveMDIChild )
	{
		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(g_hWndActiveMDIChild,GWLP_USERDATA);
		SetFocus( pd->hWndView );
	}

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
	if( g_hMainMenu )
		DestroyMenu(g_hMainMenu);
	if( g_hMdiMenu )
		DestroyMenu(g_hMdiMenu);

	_SetIniFileName(NULL);
}

//----------------------------------------------------------------------------
//
//  QueryCmdState()
//
//  PURPOSE: Query command status.
//
//----------------------------------------------------------------------------
INT CALLBACK QueryCmdState(UINT CmdId,UINT MenuState,PVOID,LPARAM /*Param*/)
{
	HWND hwndMDIChild = MDIGetActive(g_hWndMDIClient);
	if( hwndMDIChild )
	{
		MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
		UINT State = MenuState;
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
			g_hWndMDIClient = CreateMDIClient(hWnd);
			FindText_Initialize();
			break; 
		} 
		case WM_DESTROY:
		{
			//
			// Save configuration main frame and each MDI MDI child/view/page window.
			//
			if( g_bEnableWorkspaceLayout )
			{
				SaveMainFrameConfig(hWnd,g_hWndMDIClient);
			}
			FindText_Uninitialize();
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
			SetWindowPos(g_hWndMDIClient,NULL,0,0,cx,cy,SWP_NOZORDER);
			break;
		}
		case WM_COMMAND:
		{
			int wmId, wmEvent;
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			if( (QueryCmdState(wmId,UPDUI_DISABLED,0,0) & UPDUI_DISABLED) != 0 )
				break;

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
					SendMessage(g_hWndMDIClient,WM_MDICASCADE,MDITILE_SKIPDISABLED|MDITILE_ZORDER,0);
					break;
				case ID_WINDOW_TILE_HORZ:
					SendMessage(g_hWndMDIClient,WM_MDITILE,MDITILE_SKIPDISABLED|MDITILE_HORIZONTAL,0);
					break;
				case ID_WINDOW_TILE_VERT:
					SendMessage(g_hWndMDIClient,WM_MDITILE,MDITILE_SKIPDISABLED|MDITILE_VERTICAL,0);
					break;
				case ID_EDIT_FIND:
				case ID_EDIT_FIND_NEXT:
				case ID_EDIT_FIND_PREVIOUS:
				{
					HWND hwndMDIChild = MDIGetActive(g_hWndMDIClient);
					if( hwndMDIChild )
					{
						MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndMDIChild,GWLP_USERDATA);
						OnFindText_CommandHandler(hWnd,pd->hWndView,
								(wmId == ID_EDIT_FIND) ? Find_Start : ((wmId == ID_EDIT_FIND_NEXT) ? Find_Next : Find_Previous) );
					}
					break;
				}
				case ID_ABOUT:
					DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				case ID_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
				{
					// Forward command to the active MDI child window.
					HWND hwndMDIChild = MDIGetActive(g_hWndMDIClient);
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
			g_hWndActiveMDIChild = (HWND)lParam;

			if( g_hWndActiveMDIChild == NULL )
			{
				SendMessage(g_hWndMDIClient,WM_MDISETMENU,(WPARAM)g_hMainMenu,0);
				DrawMenuBar(hWnd);
			}
			else
			{
				SendMessage(g_hWndMDIClient,WM_MDISETMENU,(WPARAM)g_hMdiMenu,0);
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
		case WM_OPEN_MDI_CHILDFRAME:
		{
			//
			// Open new child frame window
			//
			if( lParam == 0 )
				return 0;
			UINT ConsoleTypeId = (UINT)LOWORD(wParam);
			UINT CallType = (UINT)HIWORD(wParam);
			OPEN_MDI_CHILDFRAME_PARAM *opp = (OPEN_MDI_CHILDFRAME_PARAM*)lParam;
			switch( CallType )
			{
				case 0:
					OpenConsole(hWnd,ConsoleTypeId,NULL,opp);
					break;
				case 1:
					OpenConsole(hWnd,ConsoleTypeId,NULL,opp);
					CoTaskMemFree(opp);
					break;
				case 2:
					OpenConsole(hWnd,ConsoleTypeId,NULL,opp);
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
				CONSOLE_VIEW_ID *pcv = (CONSOLE_VIEW_ID *)GetProp(pd->hWndView,_PROP_CONSOLE_VIEW_ID);

				if( pcv )
					ClearWindowHandle(pcv->wndId,hwndChildFrame);

				RemoveProp(pd->hWndView,_PROP_CONSOLE_VIEW_ID);

				delete pcv;
			}
			return 0;
		}
		case WM_MDI_CHILDFRAME_NCDESTROY:
		{
			break;
		}
		case PM_MAKECONTEXTMENU:
		{
			// Not used feature in this application. must returns zero.
			return 0;
		}
		case PM_UPDATETITLE:
		{
			HWND hwndViewWindow = (HWND)wParam;
			PWSTR pszPath = (PWSTR)lParam;

			if( pszPath )
			{
				CONSOLE_VIEW_ID *pcv;
				if( hwndViewWindow == NULL )
				{
					MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(MDIGetActive(g_hWndMDIClient),GWLP_USERDATA);
					if( pd )
						hwndViewWindow = pd->hWndView;
				}

				pcv = GetConsoleId(hwndViewWindow);
				if( pcv == NULL )
					return 0;
			}
			return 0;
		}
		default:
		{
			HWND hwndChildFrame = MDIGetActive(g_hWndMDIClient);
			if( hwndChildFrame )
			{
				MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwndChildFrame,GWLP_USERDATA);
				if( IsFindTextEventMessage(pd->hWndView,message,lParam) )
					return 0;
			}
			break;
		}
	}

	return DefFrameProc(hWnd, g_hWndMDIClient, message, wParam, lParam);
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

	InitIniFile();

	InitializeLibMisc(hInstance,GetUserDefaultUILanguage());

	if( _GetOSVersion() >= 0xA00 )
	{
		SetProcessPlaceholderCompatibilityMode(PHCM_EXPOSE_PLACEHOLDERS);
	}

	if( (g_hWndMain = InitInstance (hInstance, nCmdShow)) == NULL )
	{
		return FALSE;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

	MSG msg;
	INT ret;
	MSG msgDblClk = {0};

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
			// 1. Receives a notify message LVN_ITEMACTIVATE the MDI child frame window 
			//    when occurs  WM_LBUTTONDOWN->WM_LBUTTONDBLCLK on the ListVew contorol.
			//    (WM_LBUTTONUP is not notified yet)
			// 2. During LVN_ITEMACTIVATE processing, create new MDI child frame window
			//    and to activate.
			// 3. When LVN_ITEMACTIVATE returns, at that time the active window has changed.
			// 4. It then receives WM_LBUTTONUP. But this message sending to 
			//    the newly created MDI child frame window instead of the previous it.
			// 5. At this moment may case result in unexpected selection operations 
			//    on the new window (e.g. Group header click select).
			if( msg.message == WM_LBUTTONDBLCLK  )
			{
				msgDblClk.hwnd = msg.hwnd;
			}
			else if( msg.message == WM_LBUTTONUP && msgDblClk.hwnd )
			{
				if( msgDblClk.hwnd != msg.hwnd )
				{
#ifdef _DEBUG
					Beep(1500,100);
#endif
					msgDblClk.hwnd = NULL;
					continue;
				}
			}
			else if( msg.message == WM_LBUTTONDOWN )
			{
				msgDblClk.hwnd = NULL;
			}
#endif
			if( g_hWndActiveMDIChild )
			{
				// Forward message to active view on MDI child window.
				MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(g_hWndActiveMDIChild,GWLP_USERDATA);

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

		    if (!TranslateMDISysAccel(g_hWndMDIClient, &msg) && 
			    !TranslateAccelerator(g_hWndMain, hAccelTable, &msg))
	        {
				if( g_hWndActiveMDIChild )
				{
					if( IsDialogMessage(g_hWndActiveMDIChild,&msg) )
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
