#pragma once
//***************************************************************************
//*                                                                         *
//*  page_relationview.h                                                    *
//*                                                                         *
//*  Volume and Drive Relation Viewer Page                                  *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2025-03-14 Created.                                           *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//

#define _ENABLE_SHOW_ICONS  0
#define _ENABLE_SHOW_FULLDEVICEPATH  0

enum {
	MPI_ROOT=0,
	MPI_DEVICE,
	MPI_VOLUMEGUID,
	MPI_DRIVE,
	MPI_DIRECTORY,
};

class CMountPointItem
{
	PWSTR m_pszItemText;
public:
	UINT m_ItemType;

	CMountPointItem(UINT Type)
	{
		m_ItemType = Type;
		m_pszItemText = NULL;
	}

	~CMountPointItem()
	{
		_SafeMemFree(m_pszItemText);
	}

	void SetText(PCWSTR psz)
	{
		_SafeMemFree(m_pszItemText);
		m_pszItemText = _MemAllocString(psz);
	}

	PCWSTR GetText() const
	{
		return m_pszItemText;
	}
};

enum {
	PAGE_RELATIONVIEW_MOUNTEDDRIVE=0,
	PAGE_RELATIONVIEW_OBJECTNAMESPACE=1,
};

class CVolumeDriveRelationViewPage :
	public CPageWndBase
{
	HWND m_hWndTree;
	HFONT m_hFont;
#if _ENABLE_SHOW_ICONS
	HIMAGELIST m_himl;
#endif
	BOOL m_bEnableThemeStyle;
public:
	CVolumeDriveRelationViewPage(int nPageType=0)
	{
#if _ENABLE_SHOW_ICONS
		m_himl = NULL;
#endif
		m_bEnableThemeStyle = FALSE;
	}

	virtual ~CVolumeDriveRelationViewPage()
	{
	}

	virtual HRESULT OnInitPage(PVOID,DWORD,PVOID)
	{
		{ // Block:
			HWND hwndTree;
			hwndTree = CreateWindowEx(0,WC_TREEVIEW, 
                              L"",
                              WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE |
							  TVS_HASBUTTONS | TVS_HASLINES | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | TVS_INFOTIP ,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

			SendMessage(hwndTree,WM_SETFONT,(WPARAM)m_hFont,0);

			if( m_bEnableThemeStyle )
				_EnableVisualThemeStyle(hwndTree);

			TreeView_SetExtendedStyle(hwndTree,TVS_EX_DOUBLEBUFFER,TVS_EX_DOUBLEBUFFER);

#if _ENABLE_SHOW_ICONS
			int cyItem = TreeView_GetItemHeight(hwndTree);
			cyItem += 4;
			TreeView_SetItemHeight(hwndTree,cyItem);

			m_himl = ImageList_Create(
							GetSystemMetrics(SM_CXSMICON),
							GetSystemMetrics(SM_CYSMICON),
							ILC_COLOR32 | ILC_MASK, // ensures transparent background.
							8, 0);

			TreeView_SetImageList(hwndTree,m_himl,TVSIL_NORMAL);
#endif

			SHORT cy = (SHORT)TreeView_GetItemHeight(hwndTree);
			cy = max(GetSystemMetrics(SM_CYSMSIZE),(cy+4));
			TreeView_SetItemHeight(hwndTree,cy);

#if _ENABLE_DARK_MODE_TEST
			if( _IsDarkModeEnabled() )
				InitDarkModeListView(hwndTree);
#endif
			m_hWndTree = hwndTree;
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		return S_OK;
	}

	virtual HRESULT OnDestroyPage(PVOID)
	{
		return E_NOTIMPL;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hFont = GetGlobalFont(hWnd);
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
#if _ENABLE_SHOW_ICONS
		if( m_himl )
			ImageList_Destroy(m_himl);
#endif
		if( m_hFont )
			DeleteObject(m_hFont);
		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case TVN_DELETEITEM:
				OnDeleteItem((NMTREEVIEW*)lParam);
				return TRUE;
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMTREEVIEW* pnmtv)
	{
		if( pnmtv->itemOld.lParam )
			delete (CMountPointItem *)pnmtv->itemOld.lParam;
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HTREEITEM hItem = TreeView_GetSelection(m_hWndTree);
		if( hItem )
		{
			BOOL bOnTheItem;
			POINT pt;

			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);

			RECT rc;
			TreeView_GetItemRect(m_hWndTree,hItem,&rc,TRUE);

			if( pt.x == -1 && pt.y == -1 )
			{
				MapWindowPoints(m_hWndTree,NULL,(LPPOINT)&rc,2);
				pt.x = rc.left;
				pt.y = rc.bottom;
				bOnTheItem = TRUE;
			}
			else
			{
				POINT ptClient = pt;
				MapWindowPoints(NULL,m_hWndTree,&ptClient,1);
				bOnTheItem = ( PtInRect(&rc,ptClient) );
			}

			if( bOnTheItem )
			{
				HMENU hMenu;
				hMenu = CreatePopupMenu();

				UINT uFlags = TPM_LEFTALIGN|TPM_TOPALIGN;

				AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
				TrackPopupMenuEx(hMenu,uFlags,pt.x,pt.y,GetActiveWindow(),NULL);

				DestroyMenu(hMenu);
			}
		}
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SETFOCUS:
				SetFocus(m_hWndTree);
				return 0;
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	void UpdateLayout(int cx,int cy)
	{
		if( m_hWndTree )
			SetWindowPos(m_hWndTree,NULL,0,0,cx,cy,SWP_NOMOVE|SWP_NOZORDER);
	}

	HTREEITEM Insert(HTREEITEM hParent,HTREEITEM hInsertAfter,LPCWSTR pszText,PVOID ptr=NULL,int iImage=I_IMAGENONE,BOOL bExpand=FALSE)
	{
		TVINSERTSTRUCT tis = {0};

		tis.hParent = hParent;
		tis.hInsertAfter = hInsertAfter;

		tis.itemex.mask    = TVIF_TEXT|TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tis.itemex.pszText = (PWSTR)pszText;
		tis.itemex.iImage  = iImage;
		tis.itemex.iSelectedImage = iImage;
		tis.itemex.lParam  = (LPARAM)ptr;

		if( bExpand )
		{
			tis.itemex.mask |= TVIF_STATE;
			tis.itemex.state = TVIS_EXPANDED;
			tis.itemex.stateMask = TVIS_EXPANDED;
		}

		return TreeView_InsertItem(m_hWndTree,&tis);
	}

	HTREEITEM InsertRoot()
	{
		PCWSTR pszRootName = L"PC";

		CMountPointItem *pItem = new CMountPointItem(MPI_ROOT);
		pItem->SetText( pszRootName );

#if _ENABLE_SHOW_ICONS
		SHSTOCKICONINFO sii = {0};
		sii.cbSize = sizeof(sii);
		SHGetStockIconInfo(SIID_SERVER,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
		int iImage = ImageList_AddIcon(m_himl,sii.hIcon);
		DestroyIcon(sii.hIcon);
#else
		int iImage = I_IMAGENONE;
#endif
		return Insert(TVI_ROOT,TVI_LAST,pszRootName,pItem,iImage,TRUE);
	}

	HTREEITEM InsertDevice(HTREEITEM hParent,LPCWSTR pszDeviceName)
	{
		CMountPointItem *pItem = new CMountPointItem(MPI_DEVICE);
		pItem->SetText( pszDeviceName );

		WCHAR szDisplayText[MAX_PATH];
#if _ENABLE_SHOW_FULLDEVICEPATH
		StringCchCopy(szDisplayText,MAX_PATH,pszDeviceName);
#else
		GetNamePart(pszDeviceName,szDisplayText,MAX_PATH);
#endif
		int iImage = GetDiskDeviceImageIndex(pszDeviceName);

 		return Insert(hParent,TVI_LAST,szDisplayText,pItem,iImage,TRUE);
	}

	HTREEITEM InsertVolume(HTREEITEM hParent,LPCWSTR pszVolumeName)
	{
		CMountPointItem *pItem = new CMountPointItem(MPI_VOLUMEGUID);
		pItem->SetText( pszVolumeName );

		WCHAR szDisplayText[MAX_PATH];
		StringCchCopy(szDisplayText,MAX_PATH,pszVolumeName);

		RemoveBackslash(szDisplayText);

		WCHAR szNtVolumeName[MAX_PATH];
		StringCchCopy(szNtVolumeName,MAX_PATH,L"\\??\\");
		StringCchCat(szNtVolumeName,MAX_PATH,&szDisplayText[4]);

		int iImage = GetDiskDeviceImageIndex(szNtVolumeName);

		return Insert(hParent,TVI_LAST,&szDisplayText[4],pItem,iImage,1);
	}

	HTREEITEM InsertPath(HTREEITEM hParent,LPCWSTR pszPath)
	{
		CMountPointItem *pItem = new CMountPointItem( PathIsRoot(pszPath) ? MPI_DRIVE : MPI_DIRECTORY );
		pItem->SetText( pszPath );

		WCHAR szDisplayText[MAX_PATH];
		StringCchCopy(szDisplayText,MAX_PATH,pszPath);

#if _ENABLE_SHOW_ICONS
		SHSTOCKICONINFO sii = {0};
		sii.cbSize = sizeof(sii);
		SHGetStockIconInfo(SIID_FOLDER,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
		int iImage = ImageList_AddIcon(m_himl,sii.hIcon);
		DestroyIcon(sii.hIcon);
#else
		int iImage = I_IMAGENONE;
#endif

		return Insert(hParent,TVI_LAST,szDisplayText,pItem,iImage);
	}

	int GetDiskDeviceImageIndex(PCWSTR DeviceName)
	{
#if _ENABLE_SHOW_ICONS
		int iImage;
		HICON hIcon;
		hIcon = GetDiskDeviceIcon(DeviceName);
		if( hIcon )
		{
			iImage = ImageList_AddIcon(m_himl,hIcon);
		}
		else
		{
			iImage = I_IMAGENONE;
		}
		return iImage;
#else
		return I_IMAGENONE;
#endif
	}

#if !(_ENABLE_SHOW_FULLDEVICEPATH)
	HRESULT GetNamePart(PCWSTR pszPath,PWSTR pszNameBuffer,int cchNameBuffer)
	{
		HRESULT hr;
		WCHAR szTempBuffer[MAX_PATH];

		StringCchCopy(szTempBuffer,MAX_PATH,pszPath);
		RemoveBackslash(szTempBuffer);

		WCHAR *pName = wcsrchr(szTempBuffer,L'\\');

		if( pName != NULL )
			pName++;
		else
			pName = szTempBuffer;

		if( 0 )
			hr = StringCchPrintf(pszNameBuffer,cchNameBuffer,L"\\??\\%s",pName);
		else
			hr = StringCchCopy(pszNameBuffer,cchNameBuffer,pName);

		return hr;
	}
#endif

	VOID EnumVolumePaths(HTREEITEM hParent,PWCHAR VolumeName)
	{
		DWORD  CharCount = MAX_PATH + 1;
		PWCHAR Names     = NULL;
		PWCHAR NameIdx   = NULL;
		BOOL   Success   = FALSE;

		for (;;) 
		{
			Names = (PWCHAR) new BYTE [CharCount * sizeof(WCHAR)];

			if ( !Names ) 
			{
				return;
			}

			Success = GetVolumePathNamesForVolumeNameW(
							VolumeName, Names, CharCount, &CharCount
							);

			if ( Success ) 
			{
				break;
			}

			if ( GetLastError() != ERROR_MORE_DATA ) 
			{
				break;
			}

			delete [] Names;
			Names = NULL;
		}

		if ( Success )
		{
			for( NameIdx = Names; NameIdx[0] != L'\0'; NameIdx += wcslen(NameIdx) + 1 ) 
			{
				InsertPath(hParent,NameIdx);
			}
		}

		if ( Names != NULL ) 
		{
			delete [] Names;
			Names = NULL;
		}

		return;
	}

	void EnumDevicesAndVolumes(void)
	{
		DWORD  CharCount            = 0;
		WCHAR  DeviceName[MAX_PATH] = L"";
		DWORD  Error                = ERROR_SUCCESS;
		HANDLE FindHandle           = INVALID_HANDLE_VALUE;
		BOOL   Found                = FALSE;
		size_t Index                = 0;
		BOOL   Success              = FALSE;
		WCHAR  VolumeName[MAX_PATH] = L"";

		FindHandle = FindFirstVolumeW(VolumeName, ARRAYSIZE(VolumeName));

		if (FindHandle == INVALID_HANDLE_VALUE)
		{
			Error = GetLastError();
			return;
		}

		HTREEITEM hRoot;
		hRoot = InsertRoot();

		for (;;)
		{
			Index = wcslen(VolumeName) - 1;

			if (VolumeName[0]     != L'\\' ||
				VolumeName[1]     != L'\\' ||
				VolumeName[2]     != L'?'  ||
				VolumeName[3]     != L'\\' ||
				VolumeName[Index] != L'\\') 
			{
				Error = ERROR_BAD_PATHNAME;
				break;
			}

			//  QueryDosDeviceW doesn't allow a trailing backslash,
			//  so temporarily remove it.
			VolumeName[Index] = L'\0';

			CharCount = QueryDosDeviceW(&VolumeName[4], DeviceName, ARRAYSIZE(DeviceName)); 

			VolumeName[Index] = L'\\';

			if ( CharCount == 0 ) 
			{
				Error = GetLastError();
				break;
			}

			HTREEITEM hItem;
			hItem = InsertDevice(hRoot,DeviceName);

			hItem = InsertVolume(hItem,VolumeName);

			EnumVolumePaths(hItem,VolumeName);

			Success = FindNextVolumeW(FindHandle, VolumeName, ARRAYSIZE(VolumeName));

			if ( !Success ) 
			{
				Error = GetLastError();

				if (Error != ERROR_NO_MORE_FILES) 
				{
					break;
				}

				Error = ERROR_SUCCESS;
				break;
			}
		}

		FindVolumeClose(FindHandle);
		FindHandle = INVALID_HANDLE_VALUE;

		TVSORTCB tsc = {0};
		tsc.hParent = hRoot;
		tsc.lpfnCompare = &CompareProc;
		tsc.lParam  = 0;
		TreeView_SortChildrenCB(m_hWndTree,&tsc,FALSE);

		return;
	}

	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;
		SetRedraw(m_hWndTree,FALSE);

		TreeView_DeleteAllItems(m_hWndTree);
#if _ENABLE_SHOW_ICONS
		ImageList_RemoveAll(m_himl);
#endif

		EnumDevicesAndVolumes();

		SetRedraw(m_hWndTree,TRUE);
		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
	}

	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		LPCWSTR psz1 = (LPCWSTR)((CMountPointItem*)lParam1)->GetText();
		LPCWSTR psz2 = (LPCWSTR)((CMountPointItem*)lParam2)->GetText();
		if( 0 ) {
			if( _wcsnicmp(psz1,L"HarddiskVolume",14) == 0 && _wcsnicmp(psz2,L"HarddiskVolume",14) != 0 )
				return -1;
			if( _wcsnicmp(psz1,L"HarddiskVolume",14) != 0 && _wcsnicmp(psz2,L"HarddiskVolume",14) == 0 )
				return 1;
		} else {
			if( _wcsnicmp(psz1,L"\\Device\\HarddiskVolume",22) == 0 && _wcsnicmp(psz2,L"\\Device\\HarddiskVolume",22) != 0 )
				return -1;
			if( _wcsnicmp(psz1,L"\\Device\\HarddiskVolume",22) != 0 && _wcsnicmp(psz2,L"\\Device\\HarddiskVolume",22) == 0 )
				return 1;
		}
		return StrCmpLogicalW(psz1,psz2);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	//  Command Handling
	//
	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				*State = (TreeView_GetSelection(m_hWndTree) ? UPDUI_ENABLED : UPDUI_DISABLED);
				return S_OK;
			case ID_VIEW_REFRESH:
				*State = UPDUI_ENABLED;
				return S_OK;
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*State = UPDUI_DISABLED; // currentary, not supported.
				return S_OK;
		}
		return S_FALSE;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnCmdEditCopy();
				break;
			case ID_VIEW_REFRESH:
				OnCmdRefresh();
				break;
		}
		return S_OK;
	}

	void OnCmdEditCopy()
	{
		HTREEITEM hItem = TreeView_GetSelection(m_hWndTree);
		if( hItem )
		{
			WCHAR wsz[MAX_PATH];
			TreeView_GetItemText(m_hWnd,hItem,wsz,MAX_PATH);
			SetClipboardText(m_hWndTree,wsz,SCTEXT_UNICODE);

			CHAR asz[MAX_PATH];
		    TVITEMEXA tviex = {0};
		    tviex.mask       = TVIF_TEXT;
			tviex.hItem      = hItem;
			tviex.pszText    = asz;
			tviex.cchTextMax = _countof(asz);
			SendMessage(m_hWndTree,TVM_GETITEMA,0,(LPARAM)&tviex);
			SetClipboardText(m_hWndTree,asz,SCTEXT_ANSI);
		}
	}

	void OnCmdRefresh()
	{
		SELECT_ITEM sel = {0};
		FillItems(&sel);
	}
};
