//****************************************************************************
//*                                                                          *
//*  volumeinfo.cpp                                                          *
//*                                                                          *
//*  Implements the main procedure and main frame window                     *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2024-04-29 Created.                                            *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumeinfo.h"
#include "resource.h"
#include "find.h"
#if _ENABLE_DARK_MODE_TEST
#include "menubar.h"
#include "darkmode.h"
#pragma comment(lib, "uxtheme.lib")
#define ID_MENUBAR      2
#endif

#define ID_TREE         1
#define TREE_WIDTH      260

#define ITEM_BLANK      1

#define VOLUME_CONSOLE_VOLUME        (VOLUME_CONSOLE_MAX_ID + 1)
#define VOLUME_CONSOLE_PHYSICALDRIVE (VOLUME_CONSOLE_MAX_ID + 2)

static HINSTANCE g_hInst = NULL;
static HWND      g_hWndMain = NULL;
static HWND      g_hWndViewPage = NULL;
static HWND      g_hWndTree = NULL;
static WCHAR*    g_pszWindowTitle = L"VolumeInfo";
static WCHAR*    g_pszWindowClass = L"VolumeInfoWindowClass";
static HMENU     g_hMainMenu = NULL;
static BOOL      g_bPreventChange = FALSE;
static HWND      g_hwndSaveFocus = NULL;
static LCID      g_lcid = LOCALE_USER_DEFAULT;
static LANGID    g_langId = LANG_USER_DEFAULT;
static UINT      g_DefaultVolumeConsole = VOLUME_CONSOLE_VOLUMEINFORMAION;
static UINT      g_DefaultPhysicalDriveConsole = VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION;
#if _ENABLE_DARK_MODE_TEST
static CMenuBar  g_MenuBar;
#endif

VOID UpdateLayout(int cx,int cy,HWND hWndView=FALSE);
VOID OpenConsole(HWND hWnd,UINT ConsoleTypeId,PCWSTR Param=NULL,UINT ParamType=0,BOOL bFocus=FALSE);

HINSTANCE _GetResourceInstance()
{
	return g_hInst;
}

HWND _GetMainWnd()
{
	return g_hWndMain;
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

//----------------------------------------------------------------------------
//
//  NavigationPane
//
//  PURPOSE: Main navigation tree-view
//
//----------------------------------------------------------------------------
namespace NavigationPane
{
	typedef struct _TREE_ITEM
	{
		UINT ItemType;
		UINT ConsoleId;
		PWSTR pszDevicePath;
	} TREE_ITEM;
	
	HTREEITEM InsertItem(UINT ConsoleId,PCWSTR pszText)
	{
		TVINSERTSTRUCT tvis = {0};
	
		TREE_ITEM *pItem = new TREE_ITEM;
		ZeroMemory(pItem,sizeof(TREE_ITEM));
	
		pItem->ConsoleId = ConsoleId;
	
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
	
		tvis.itemex.mask  = TVIF_PARAM|TVIF_TEXT;
		tvis.itemex.pszText = (PWSTR)pszText;
		tvis.itemex.lParam = (LPARAM)pItem;
	
		return TreeView_InsertItem(g_hWndTree,&tvis);
	}
	
	HTREEITEM InsertSubItem(UINT ConsoleId,HTREEITEM hParent,PCWSTR pszText,PCWSTR pszDevicePath)
	{
		TVINSERTSTRUCT tvis = {0};
	
		TREE_ITEM *pItem = new TREE_ITEM;
		ZeroMemory(pItem,sizeof(TREE_ITEM));
	
		pItem->ConsoleId = ConsoleId;
		pItem->pszDevicePath = _MemAllocString(pszDevicePath);
	
		tvis.hParent = hParent;
		tvis.hInsertAfter = TVI_LAST;
	
		tvis.itemex.mask  = TVIF_PARAM|TVIF_TEXT;
		tvis.itemex.pszText = (PWSTR)pszText;
		tvis.itemex.lParam = (LPARAM)pItem;
	
		return TreeView_InsertItem(g_hWndTree,&tvis);
	}
	
	HTREEITEM InsertBlank(HTREEITEM hParent,HTREEITEM hInsert)
	{
		TREE_ITEM *pItem = new TREE_ITEM;
		ZeroMemory(pItem,sizeof(TREE_ITEM));
	
		pItem->ItemType = ITEM_BLANK;
	
		TVINSERTSTRUCT tvi = {0};
		tvi.hInsertAfter = TVI_LAST;
		tvi.hParent =  TVI_ROOT;
		tvi.itemex.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_PARAM|TVIF_INTEGRAL;
		tvi.itemex.pszText = L"";
		tvi.itemex.lParam = (LPARAM)pItem;
		tvi.itemex.iImage = tvi.itemex.iExpandedImage = I_IMAGENONE;
		tvi.itemex.iIntegral = 1;  // blank line height
		tvi.itemex.uStateEx = 0;
		HTREEITEM h = TreeView_InsertItem(g_hWndTree, &tvi);
		TVITEMEX tviex = {0};
		tviex.mask = TVIF_STATEEX;
		tviex.hItem = h;
		tviex.uStateEx = TVIS_EX_DISABLED;
		TreeView_SetItem( g_hWndTree, &tviex );
		return h;
	}
	
	VOID TreeEnumVolumes(HTREEITEM hTreeVol)
	{
		VOLUME_NAME_STRING_ARRAY *VolumeNames;
		EnumVolumeNames( &VolumeNames );
	
		for(ULONG i = 0; i < VolumeNames->Count; i++)
		{
			InsertSubItem(VOLUME_CONSOLE_VOLUME,
					hTreeVol,
					FindFileName_W(VolumeNames->Volume[i].NtVolumeName),
					VolumeNames->Volume[i].NtVolumeName );
		}
	
		FreeVolumeNames( VolumeNames );
	}
	
	VOID TreeEnumPhysicalDrives(HTREEITEM hTreePhysicalDrives)
	{
		PHYSICALDRIVE_NAME_STRING_ARRAY *PhysicalDrives;
	
		EnumPhysicalDriveNames( &PhysicalDrives );
	
		for(ULONG i = 0; i < PhysicalDrives->Count; i++)
		{
			InsertSubItem(VOLUME_CONSOLE_PHYSICALDRIVE,//VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION,
					hTreePhysicalDrives,
					FindFileName_W(PhysicalDrives->Drive[i].PhysicalDriveName),
					PhysicalDrives->Drive[i].PhysicalDriveName );
		}
	
		FreePhysicalDriveNames( PhysicalDrives );
	}
	
	static HTREEITEM hTreeVol = NULL;
	static HTREEITEM hTreePD  = NULL;
	
	VOID InitTreeViewItems()
	{
		hTreeVol = InsertItem( VOLUME_CONSOLE_VOLUMELIST,        L"Volumes" );
		hTreePD  = InsertItem( VOLUME_CONSOLE_PHYSICALDRIVELIST, L"Physical Drives" );
	
		InsertBlank( TVI_ROOT, TVI_LAST );
	
		InsertItem( VOLUME_CONSOLE_DOSDRIVELIST,                 L"MS-DOS Drives" );
		InsertItem( VOLUME_CONSOLE_STORAGEDEVICE,                L"Storage Device" );
		InsertItem( VOLUME_CONSOLE_MOUNTEDDEVICE,                L"Mounted Device" );
		InsertItem( VOLUME_CONSOLE_SHADOWCOPYLIST,               L"Shadow Copy" );
	
		InsertBlank( TVI_ROOT, TVI_LAST );
	
		InsertItem( VOLUME_CONSOLE_FILTERDRIVER,                 L"Filter Driver" );
	
		InsertBlank( TVI_ROOT, TVI_LAST );
	
		TreeEnumVolumes(hTreeVol);
		TreeEnumPhysicalDrives(hTreePD);
	}
	
	VOID DeleteChildItems(HTREEITEM hParent)
	{
		HTREEITEM hItem = TreeView_GetChild(g_hWndTree,hParent);
		HTREEITEM hNextItem;

		while( hItem )
		{
			hNextItem = TreeView_GetNextSibling(g_hWndTree,hItem);

			TreeView_DeleteItem(g_hWndTree,hItem);

			hItem = hNextItem;
		}
	}

	VOID SelectTreeItem(UINT ConsoleTypeId,PCWSTR pszDevicePath)
	{
		HTREEITEM h;
		if( VOLUME_CONSOLE_PHYSICALDRIVE == ConsoleTypeId )
			h = hTreePD;
		else if( VOLUME_CONSOLE_VOLUME == ConsoleTypeId )
			h = hTreeVol;
		else
			h = TVI_ROOT;
	
		h = TreeView_GetChild(g_hWndTree,h);
	
		if( h )
		{
			TVITEM tvi = {0};
			tvi.mask = TVIF_PARAM;
			do
			{
				tvi.hItem = h;
				TreeView_GetItem(g_hWndTree,&tvi);
				TREE_ITEM *pItem = (TREE_ITEM *)tvi.lParam;
				if( pItem )
				{
					if( ConsoleTypeId == pItem->ConsoleId )
					{
						if( pszDevicePath )
						{
							if( _wcsicmp(pItem->pszDevicePath,pszDevicePath) == 0 )
							{
								TreeView_Select(g_hWndTree,h,TVGN_CARET);
								TreeView_Expand(g_hWndTree,TreeView_GetParent(g_hWndTree,h),TVE_EXPAND);
								break;
							}
						}
						else
						{
							TreeView_Select(g_hWndTree,h,TVGN_CARET);
							TreeView_Expand(g_hWndTree,TreeView_GetParent(g_hWndTree,h),TVE_EXPAND);
							break;
						}
					}
				}
			} while( (h = TreeView_GetNextSibling(g_hWndTree,h)) != NULL );
		}
	}

	VOID UpdateTreeViewItems()
	{
		ASSERT(hTreeVol != NULL);
		ASSERT(hTreePD != NULL);

		SetRedraw(g_hWndTree,FALSE);

		HTREEITEM hItemCurSel = TreeView_GetSelection(g_hWndTree);

		TREE_ITEM *pItem = (TREE_ITEM *)TreeView_GetItemData(g_hWndTree,hItemCurSel);

		PWSTR pszSelectedText = NULL;
		UINT ConsoleTypeId;

		ConsoleTypeId = pItem->ConsoleId;
		if( ConsoleTypeId == VOLUME_CONSOLE_VOLUME || ConsoleTypeId == VOLUME_CONSOLE_PHYSICALDRIVE )
		{
			pszSelectedText = _MemAllocString(pItem->pszDevicePath);
			TreeView_Select(g_hWndTree,NULL,TVGN_CARET);
		}

		DeleteChildItems(hTreeVol);
		TreeEnumVolumes(hTreeVol);

		DeleteChildItems(hTreePD);
		TreeEnumPhysicalDrives(hTreePD);

		if( pszSelectedText )
		{
			g_bPreventChange = TRUE;

			SelectTreeItem(ConsoleTypeId,pszSelectedText);

			_SafeMemFree( pszSelectedText );

			g_bPreventChange = FALSE;
		}

		SetRedraw(g_hWndTree,TRUE);
	}

	//
	// message/notification handler
	//
	LRESULT OnSelChanged(HWND hWnd,LPNMTREEVIEW pnmtv)
	{
		if( pnmtv->itemNew.state & TVIS_SELECTED )
		{
			TREE_ITEM *pItem = (TREE_ITEM *)pnmtv->itemNew.lParam;
			if( pItem )
			{
				UINT cid = pItem->ConsoleId;

				if( VOLUME_CONSOLE_VOLUME == cid )
				{
					cid = g_DefaultVolumeConsole;
				}
				else if( VOLUME_CONSOLE_PHYSICALDRIVE == cid )
				{
					cid = g_DefaultPhysicalDriveConsole;
				}

				OpenConsole(hWnd,cid,pItem->pszDevicePath);
			}
		}
		return 0;
	}

	LRESULT OnCustomDraw(LPNMLVCUSTOMDRAW pcd)
	{
		if( pcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		TREE_ITEM *pItem = (TREE_ITEM *)pcd->nmcd.lItemlParam;

		if( pItem->ItemType != ITEM_BLANK )
			return CDRF_DODEFAULT;

		if( pcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			pcd->clrTextBk = TreeView_GetBkColor(g_hWndTree);

			return CDRF_NEWFONT;
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnSetCursor(LPNMMOUSE lpnmm)
	{
		GetCursorPos(&lpnmm->pt);

		TVHITTESTINFO tvht = {0};
		tvht.pt    = lpnmm->pt;
		ScreenToClient(g_hWndTree,&tvht.pt);
		tvht.flags = TVHT_ONITEM|TVHT_ONITEMSTATEICON|TVHT_ONITEMRIGHT;
		tvht.hItem = NULL;
		HTREEITEM hItem = TreeView_HitTest(g_hWndTree,&tvht);
		TREE_ITEM *pItem = (TREE_ITEM *)TreeView_GetItemData(g_hWndTree,hItem);

		if( pItem && pItem->ItemType == ITEM_BLANK )
		{
			SetCursor( LoadCursor(NULL,IDC_ARROW) );
			return 1;
		}
		return 0;
	}

	LRESULT OnSelChanging(NMTREEVIEW *pnmtv)
	{
		TVITEMEX tiex={0};
		tiex.mask = TVIF_STATEEX;
		tiex.hItem = pnmtv->itemNew.hItem;
		TreeView_GetItem(g_hWndTree,&tiex);
		if( tiex.uStateEx & TVIS_EX_DISABLED )
		{
			if( pnmtv->action == 2 )
			{
				BOOL bPageKey = FALSE;

				BYTE keystate[256] = {0};
				GetKeyboardState(keystate);

				SetRedraw(g_hWndTree,FALSE);

				if( (keystate[VK_HOME] & 0x80) || (keystate[VK_PRIOR] & 0x80) || (keystate[VK_NEXT] & 0x80) )
					bPageKey = TRUE;

				HTREEITEM hNextItem = NULL;

				if( bPageKey )
				{
					TreeView_Select(g_hWndTree,(hNextItem = TreeView_GetNextVisible(g_hWndTree,pnmtv->itemNew.hItem)),TVGN_CARET);
				}
				else
				{
					if( GetKeyState(VK_DOWN) < 0 )
						hNextItem = TreeView_GetNextVisible(g_hWndTree,pnmtv->itemNew.hItem);
					else
						hNextItem = TreeView_GetPrevVisible(g_hWndTree,pnmtv->itemNew.hItem);

					if( hNextItem )
						TreeView_Select(g_hWndTree,hNextItem,TVGN_CARET);
				}

				if( hNextItem )
				{
					TreeView_EnsureVisible(g_hWndTree, hNextItem);
				}
				SetRedraw(g_hWndTree,TRUE);
			}
			return TRUE;
		}
		return 0;
	}

	LRESULT OnDeleteItem(LPNMTREEVIEW pnmtv)
	{
		TREE_ITEM *pItem = (TREE_ITEM *)pnmtv->itemOld.lParam;
		_SafeMemFree(pItem->pszDevicePath);
		delete pItem;
		return 0;
	}

	LRESULT OnSetFocus(NMHDR *pnmhdr)
	{
		g_hwndSaveFocus = pnmhdr->hwndFrom;
		return 0;
	}

	LRESULT OnNotify(HWND hWnd,UINT,WPARAM,LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case NM_CUSTOMDRAW:
				return OnCustomDraw((LPNMLVCUSTOMDRAW)pnmhdr);
			case NM_SETFOCUS:
				return OnSetFocus((LPNMHDR)pnmhdr);
			case NM_SETCURSOR:
				return OnSetCursor((LPNMMOUSE)pnmhdr);
			case TVN_DELETEITEM:
				return OnDeleteItem((LPNMTREEVIEW)pnmhdr);
			case TVN_SELCHANGING:
				return OnSelChanging((LPNMTREEVIEW)pnmhdr);
			case TVN_SELCHANGED:
				return OnSelChanged(hWnd,(LPNMTREEVIEW)pnmhdr);
		}
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////////////
// About Box
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

	Edit_AddText(hwndEdit,L"\r\n");
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

	Edit_AddText(hwndEdit,szText);
	Edit_AddText(hwndEdit,L"\r\n");

	cb = _countof(szText);
	szText[0] = 0;
	if( SHRegGetValue(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",L"BuildLabEx",SRRF_RT_REG_SZ,NULL,szText,&cb) == ERROR_SUCCESS)
	{
		if( cb != 0 )
		{
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
	{VOLUME_CONSOLE_HOME,                    0},
	{VOLUME_CONSOLE_VOLUMELIST,              0},
	{VOLUME_CONSOLE_PHYSICALDRIVELIST,       0},
	{VOLUME_CONSOLE_SHADOWCOPYLIST,          0},
	{VOLUME_CONSOLE_STORAGEDEVICE,           0},
	{VOLUME_CONSOLE_MOUNTEDDEVICE,           0},
	{VOLUME_CONSOLE_DOSDRIVELIST,            0},
	{VOLUME_CONSOLE_FILTERDRIVER,            0},
	{VOLUME_CONSOLE_VOLUMEINFORMAION,        0},
	{VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION, 0},
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

VOID SendUIInitLayout(HWND hwndBase,HWND hWndView)
{
	RECT rc;
	GetClientRect(hwndBase,&rc);
	rc.left += TREE_WIDTH;
	rc.right -= TREE_WIDTH;
	SendMessage(hWndView,WM_CONTROL_MESSAGE,UI_INIT_LAYOUT,(LPARAM)&rc);
}

//----------------------------------------------------------------------------
//
//  OpenViewPage()
//
//  PURPOSE: Open view page window.
//
//----------------------------------------------------------------------------
HWND OpenViewPage(HWND hWnd,UINT ConsoleTypeId,PCWSTR pszOpenTarget = NULL,PVOID OffsetParam=NULL)
{
	HWND hWndView = NULL;

	PCWSTR pszInitialPath;
	if( pszOpenTarget )
		pszInitialPath = pszOpenTarget;
	else
		pszInitialPath = NULL;

	VOLUME_CONSOLE_CREATE_PARAM param = {0};
	param.pszReserved = (PWSTR)pszInitialPath;

	hWndView = CreateVolumeConsoleWindow(hWnd,ConsoleTypeId,&param);

	SendUIInitLayout(hWnd,hWndView);

	SELECT_ITEM sel = {0};

	switch( ConsoleTypeId )
	{
		case VOLUME_CONSOLE_VOLUMEINFORMAION:
//		case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
		{
			sel.pszVolume = (PWSTR)pszOpenTarget;
			sel.ViewType = ConsoleTypeId;
			break;
		}
		case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
//		case VOLUME_CONSOLE_DISKLAYOUT:
		{
			sel.pszPhysicalDrive = (PWSTR)pszOpenTarget;
			sel.ViewType = ConsoleTypeId;
			break;
		}
	}

	SendMessage(hWndView,WM_NOTIFY_MESSAGE,UI_NOTIFY_VOLUME_SELECTED,(LPARAM)&sel);

	SetWindowHandle(ConsoleTypeId,hWndView);

	ShowWindow(hWndView,SW_SHOW);

	return hWndView;
}

//----------------------------------------------------------------------------
//
//  OpenConsole()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
VOID OpenConsole(HWND hWnd,UINT ConsoleTypeId,PCWSTR Param,UINT ParamType,BOOL bFocus)
{
	HWND hwndCurView = g_hWndViewPage;
	HWND hwndNewView = GetOpenedWindowHandle(ConsoleTypeId);

	BOOL b = FALSE;
	if( hwndNewView == NULL )
	{
		hwndNewView = OpenViewPage(hWnd,ConsoleTypeId,(PCWSTR)Param);

		SetWindowLongPtr(hwndNewView,GWLP_USERDATA,ConsoleTypeId);
	}
	else
	{
		if( hwndCurView )
		{
			ULONG_PTR cidCur = GetWindowLongPtr(hwndCurView,GWLP_USERDATA);
			ULONG_PTR cidNew = GetWindowLongPtr(hwndNewView,GWLP_USERDATA);

			if( cidNew == ConsoleTypeId )
			{
				SELECT_ITEM sel = {0};
				PWSTR psz = (PWSTR)Param;
				switch( ConsoleTypeId )
				{
					case VOLUME_CONSOLE_VOLUMEINFORMAION:
//					case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
					{
						sel.pszVolume = psz;
						sel.ViewType = ConsoleTypeId;
						break;
					}
					case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
//					case VOLUME_CONSOLE_DISKLAYOUT:
					{
						sel.pszPhysicalDrive = psz;
						sel.ViewType = ConsoleTypeId;
						break;
					}
				}

				if( sel.pszVolume )
				{
					STRING_STRUCT name = {0};
					WCHAR szVolume[MAX_PATH];

					name.Length = 0;
					name.MaximumLength = sizeof(szVolume);
					name.Buffer = szVolume;
					SendMessage(hwndNewView,WM_QUERY_MESSAGE,UI_QUERY_CURRENTITEMNAME,(LPARAM)&name);

					if( _wcsicmp(psz,name.Buffer) == 0 )
					{
						;
					}
					else
					{
						SendMessage(hwndNewView,WM_NOTIFY_MESSAGE,UI_NOTIFY_VOLUME_SELECTED,(LPARAM)&sel);
					}
				}
			}

			if( cidCur == cidNew )
				return;
		}
	}

	if( hwndNewView )
	{
		RECT rc;
		GetClientRect(hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),hwndNewView);
		EnableWindow(hwndNewView,TRUE);
		ShowWindow(hwndNewView,SW_SHOW);
		g_hWndViewPage = hwndNewView;
	}

	if( hwndCurView )
	{
		EnableWindow(hwndCurView,FALSE);
		ShowWindow(hwndCurView,SW_HIDE);
	}

	g_bPreventChange = TRUE;
	switch( ConsoleTypeId )
	{
		case VOLUME_CONSOLE_VOLUMEINFORMAION:
//		case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
			NavigationPane::SelectTreeItem(VOLUME_CONSOLE_VOLUME,Param);
			break;
		case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
			NavigationPane::SelectTreeItem(VOLUME_CONSOLE_PHYSICALDRIVE,Param);
			break;
		default:
			NavigationPane::SelectTreeItem(ConsoleTypeId,Param);
			break;
	}
	g_bPreventChange = FALSE;

	if( bFocus )
		SetFocus(hwndNewView);
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
			L"/v : Select Volume List.\n"
			L"/pd : Select Physical Drive List.\n"
			L"/sd : Select Storage Device List.\n"
			L"/md : Select Mounted Device List.\n"
			L"/sc : Select Shadow Copy Volume List.\n"
			L"/d : Open MS-DOS Drive List.\n";

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
	WCHAR szLangIdOrName[LOCALE_NAME_MAX_LENGTH];

	_ARGPARAM()
	{
		Maximize = FALSE;
		WithoutOpen = FALSE;
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
				else if( _wcsicmp(pArg,L"sc") == 0 )
				{
					if( !args.addOpenConsole( VOLUME_CONSOLE_SHADOWCOPYLIST ) )
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
				{
					args.WithoutOpen = TRUE; // no mdi child open
				}
				else if( _wcsicmp(pArg,L"f") == 0 )
				{
					args.Maximize = TRUE;    // maximize
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

	if( !args.WithoutOpen && args.ConsoleTypeId.GetCount() == 0 )
		args.ConsoleTypeId.Add( VOLUME_CONSOLE_VOLUMELIST );

	return TRUE;
}

//----------------------------------------------------------------------------
//
//  RegisterWindowClass()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
ATOM RegisterWindowClass(HINSTANCE hInstance)
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
		MessageBox(NULL,L"Invalid Parameter.",g_pszWindowTitle,MB_OK|MB_ICONSTOP);
		return 0;
	}

	if( args.szLangIdOrName[0] )
		InitLanguage(args.szLangIdOrName);

	if( args.Help )
	{
		ShowHelp(NULL);
		return 0;
	}

	HWND hWnd;
	hWnd = CreateWindow(g_pszWindowClass, g_pszWindowTitle, WS_OVERLAPPEDWINDOW,
				  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	_CenterWindow(hWnd,GetDesktopWindow()); // todo:

	NavigationPane::InitTreeViewItems();

	NavigationPane::SelectTreeItem(args.ConsoleTypeId[0],NULL);

	SetFocus(g_hWndViewPage);

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
	if( g_hMainMenu )
		DestroyMenu(g_hMainMenu);

	FindText_Uninitialize();
}

//----------------------------------------------------------------------------
//
//  QueryCmdState()
//
//  PURPOSE: Query command status.
//
//----------------------------------------------------------------------------
INT CALLBACK QueryCmdState(UINT CmdId,UINT MenuState,PVOID,LPARAM)
{
	UINT State = MenuState;
	if( g_hWndViewPage && SendMessage(g_hWndViewPage,WM_QUERY_CMDSTATE,MAKEWPARAM(CmdId,0),(LPARAM)&State) )
	{
		return State;
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
	}

	return UPDUI_DISABLED;
}

//----------------------------------------------------------------------------
//
//  OnCreate()
//
//  PURPOSE: WM_CREATE Message Handler
//
//----------------------------------------------------------------------------
VOID OnCreate(HWND hWnd,WPARAM wParam, LPARAM lParam)
{
#if _ENABLE_DARK_MODE_TEST
	if( _IsDarkModeEnabled() )
	{
		AllowDarkModeForWindow(hWnd, true);
		RefreshTitleBarThemeColor(hWnd);

		DWORD dwStyle = WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|
				  TBSTYLE_LIST|TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|
				  CCS_NORESIZE|CCS_NOPARENTALIGN|CCS_NODIVIDER;
		g_MenuBar.Create(hWnd,_GetResourceInstance(),ID_MENUBAR,dwStyle);
		g_MenuBar.LoadMenu(MAKEINTRESOURCE(IDR_MAINFRAME));
		g_MenuBar.SetUnderLineMode(FALSE);

		SetWindowTheme(g_MenuBar.m_hWnd,L"DarkMode",NULL);

		InitializeVolumeConsole(VOLUME_DLL_FLAG_ENABLE_DARK_MODE);
	}
	else
	{
#if _ENABLE_LIGHT_MODE_TOOLBAR_MENU
		DWORD dwStyle = WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|
				  TBSTYLE_LIST|TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|
				  CCS_NORESIZE|CCS_NOPARENTALIGN|CCS_NODIVIDER;
		g_MenuBar.Create(hWnd,_GetResourceInstance(),ID_MENUBAR,dwStyle);
		g_MenuBar.LoadMenu(MAKEINTRESOURCE(IDR_MAINFRAME));
		g_MenuBar.SetUnderLineMode(FALSE);
#else
		g_hMainMenu = LoadMenu(_GetResourceInstance(),MAKEINTRESOURCE(IDR_MAINFRAME));
		SetMenu(hWnd,g_hMainMenu);
#endif
	}
#else
	g_hMainMenu = LoadMenu(_GetResourceInstance(),MAKEINTRESOURCE(IDR_MAINFRAME));
	SetMenu(hWnd,g_hMainMenu);
#endif

	g_hWndTree = CreateWindowEx(0,
                            WC_TREEVIEW,
                            TEXT("Tree View"),
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP |
							TVS_FULLROWSELECT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | TVS_NOHSCROLL | TVS_TRACKSELECT | TVS_LINESATROOT | TVS_HASBUTTONS, 
                            0,0,0,0,
                            hWnd, 
                            (HMENU)ID_TREE, 
                            g_hInst, 
                            NULL); 

	TreeView_SetExtendedStyle(g_hWndTree,TVS_EX_DOUBLEBUFFER|TVS_EX_AUTOHSCROLL,TVS_EX_DOUBLEBUFFER|TVS_EX_AUTOHSCROLL);

#if _ENABLE_DARK_MODE_TEST
	if( _IsDarkModeEnabled() )
	{
		TreeView_SetBkColor(g_hWndTree,RGB(16,16,16));
		TreeView_SetTextColor(g_hWndTree,RGB(224,224,224));
	}
	else
	{
		TreeView_SetBkColor(g_hWndTree,RGB(248,248,250));
	}
#else
	TreeView_SetBkColor(g_hWndTree,RGB(248,248,250));
#endif

	TreeView_SetItemHeight(g_hWndTree, TreeView_GetItemHeight(g_hWndTree) + 8);

#if _ENABLE_DARK_MODE_TEST
	if( _IsDarkModeEnabled() )
	{
		AllowDarkModeForWindow(g_hWndTree, _IsDarkModeEnabled());
		SetWindowTheme(g_hWndTree,L"DarkMode_Explorer",NULL);
	}
	else
	{
		_EnableVisualThemeStyle(g_hWndTree);
	}
#else
	_EnableVisualThemeStyle(g_hWndTree);
#endif		

	FindText_Initialize();
}

VOID UpdateLayout(int cx,int cy,HWND hWndView)
{
	int cyMenuBar = 0;

#if _ENABLE_DARK_MODE_TEST
#if _ENABLE_LIGHT_MODE_TOOLBAR_MENU
	if( IsWindowVisible(g_MenuBar.m_hWnd)
#else
	if( _IsDarkModeEnabled() )
#endif
	{
		cyMenuBar = (LOWORD(g_MenuBar.GetButtonSize()) / 2) + 2 + 2;
		SetWindowPos(g_MenuBar.m_hWnd,NULL,0,0,cx,cyMenuBar,SWP_NOZORDER);
	}
#endif

	SetWindowPos(g_hWndTree,NULL,0,cyMenuBar,TREE_WIDTH,cy-cyMenuBar,SWP_NOZORDER);

	UINT uFlags;
	if( hWndView )
	{
		uFlags = SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE|SWP_HIDEWINDOW;
	}
	else
	{
		uFlags = SWP_NOZORDER;
		hWndView = g_hWndViewPage;
	}

	if( hWndView )
		SetWindowPos(hWndView,NULL,TREE_WIDTH,cyMenuBar,cx-TREE_WIDTH,cy-cyMenuBar,uFlags);
}

//----------------------------------------------------------------------------
//
//  OnSize()
//
//  PURPOSE: WM_SIZE Message Handler
//
//----------------------------------------------------------------------------
VOID OnSize(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
	int cx = GET_X_LPARAM(lParam);
	int cy = GET_Y_LPARAM(lParam);

	UpdateLayout(cx,cy);
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
			OnCreate(hWnd,wParam,lParam);
			break; 
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			OnSize(hWnd,wParam,lParam);
			break;
#if _ENABLE_DARK_MODE_TEST
		case WM_ERASEBKGND:
			if( _IsDarkModeEnabled() )
			{
				RECT rc;
				GetClientRect(hWnd,&rc);
				HBRUSH hbr = CreateSolidBrush(RGB(30,30,30));
				FillRect((HDC)wParam,&rc,hbr);
				DeleteObject(hbr);
				return TRUE;
			}
			break;
#endif
		case WM_NOTIFY:
			if( ((NMHDR*)lParam)->idFrom == ID_TREE )
				return NavigationPane::OnNotify(hWnd,WM_NOTIFY,wParam,lParam);
			break;
		case WM_SETFOCUS:
			if( g_hwndSaveFocus )
				SetFocus(g_hwndSaveFocus);
			break;
		case WM_ACTIVATE:
			if( WA_INACTIVE == wParam )
				g_hwndSaveFocus = GetFocus();
			break;
#if _ENABLE_DARK_MODE_TEST
		case WM_MENUSELECT:
			return g_MenuBar.OnMenuSelect( (HMENU)lParam, LOWORD(wParam), HIWORD(wParam) );
#endif
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
		case WM_COMMAND:
		{
#if _ENABLE_DARK_MODE_TEST
			if( IsWindow(g_MenuBar.m_hWnd) )
				g_MenuBar.SetUnderLineMode(FALSE);
#endif
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
				case ID_EDIT_FIND:
				case ID_EDIT_FIND_NEXT:
				case ID_EDIT_FIND_PREVIOUS:
					if( g_hWndViewPage )
					{
						OnFindText_CommandHandler(hWnd,g_hWndViewPage,
								((wmId == ID_EDIT_FIND) ? Find_Start : ((wmId == ID_EDIT_FIND_NEXT) ? Find_Next : Find_Previous)) );
					}
					break;
				case ID_ABOUT:
					DialogBox(_GetResourceInstance(), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				case ID_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
				{
					// Forward command to the active MDI child window.
					if( g_hWndViewPage )
					{
						if( ID_VIEW_REFRESH == wmId )
						{
							NavigationPane::UpdateTreeViewItems();
						}

						SendMessage(g_hWndViewPage,WM_COMMAND,wParam,lParam);
					}
					break;
				}
			}
			break;
		}
		case WM_OPEN_MDI_CHILDFRAME:
		{
			if( lParam == 0 )
				return 0;
			UINT ConsoleTypeId = (UINT)LOWORD(wParam);
			UINT CallType = (UINT)HIWORD(wParam);
			switch( CallType )
			{
				case 1:
				{
					OPEN_MDI_CHILDFRAME_PARAM *p = (OPEN_MDI_CHILDFRAME_PARAM *)lParam;
					switch( ConsoleTypeId )
					{
						case VOLUME_CONSOLE_VOLUMEINFORMAION:
//						case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
							g_DefaultVolumeConsole = ConsoleTypeId;
							break;
						case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
//						case VOLUME_CONSOLE_DISKLAYOUT:
							g_DefaultPhysicalDriveConsole = ConsoleTypeId;
							break;
					}
					OpenConsole(hWnd,ConsoleTypeId,(PWSTR)p->Path,CallType);
					if( CallType == 1 )
						CoTaskMemFree(p);
					SetFocus(g_hWndViewPage);
					break;
				}
			}
			return 0;
		}
		case PM_MAKECONTEXTMENU:
		{
			if( wParam != NULL )
			{
				HMENU hMenu = (HMENU)lParam;
				UINT ConsoleId = (UINT)wParam;
				switch( ConsoleId )
				{
					case VOLUME_CONSOLE_VOLUMELIST:
					{
						AppendMenu(hMenu,MF_STRING,ID_VOLUMEINFORMATION,L"Volume Information"); 
						AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
						SetMenuDefaultItem(hMenu,ID_VOLUMEINFORMATION,FALSE);
						return (LRESULT)TRUE;
					}
					case VOLUME_CONSOLE_PHYSICALDRIVELIST:
					{
						AppendMenu(hMenu,MF_STRING,ID_PHYSICALDRIVEINFORMATION,L"Drive Information");
						AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
						SetMenuDefaultItem(hMenu,ID_PHYSICALDRIVEINFORMATION,FALSE);
						return (LRESULT)TRUE;
					}
				}
			}
			return (LRESULT)FALSE;
		}
		default:
		{
			if( g_hWndViewPage )
			{
				if( IsFindTextEventMessage(g_hWndViewPage,message,lParam) )
					return 0;
			}
			break;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
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

#if _ENABLE_DARK_MODE_TEST
	if( InitDarkMode() == S_OK )
	{
		EnableDarkMode(TRUE);
	}
#endif

	RegisterWindowClass(hInstance);

	InitializeLibMisc(hInstance,GetUserDefaultUILanguage());

	if( (g_hWndMain = InitInstance (hInstance, nCmdShow)) == NULL )
	{
		return FALSE;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

	MSG msg = {0};
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
			// 1. Receives a notify message LVN_ITEMACTIVATE the MDI child window 
			//    when occurs  WM_LBUTTONDOWN->WM_LBUTTONDBLCLK on the ListVew contorol.
			//    (WM_LBUTTONUP is not notified yet)
			// 2. During LVN_ITEMACTIVATE processing, create new MDI child frame 
			//    and to activate.
			// 3. When LVN_ITEMACTIVATE returns, at that time the active window has changed.
			// 4. It then receives WM_LBUTTONUP. But this message sending to 
			//    the newly created window instead of the previous window.
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
#endif
			//
			// Find dialog message
			//
			if( IsFindTextDialogMessage(&msg) )
				continue;

#if _ENABLE_DARK_MODE_TEST
#if _ENABLE_LIGHT_MODE_TOOLBAR_MENU
			if( IsWindowVisible(g_MenuBar.m_hWnd) && g_MenuBar.TranslateFrameMessage(&msg) )
				continue;
#else
			if( _IsDarkModeEnabled() && g_MenuBar.TranslateFrameMessage(&msg) )
				continue;
#endif
#endif
		    if( !TranslateAccelerator(g_hWndMain, hAccelTable, &msg) )
	        { 
				if( (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) && msg.wParam == VK_TAB )
				{
					if( IsDialogMessage(g_hWndMain,&msg) )
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
