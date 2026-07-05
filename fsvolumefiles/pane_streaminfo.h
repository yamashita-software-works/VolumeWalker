#pragma once
//*****************************************************************************
//
//  pane_streaminfo.h
//
//  PURPOSE: Stream Information Pane.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-01-21 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 

#define STMIF_EXCLUDE_ZERO_STREAM             0x1
#define STMIF_EXCLUDE_LESS_EQ_CLUSTER_SIZE    0x2

typedef struct _STREAM_LIST_ITEM
{
	NT_FILE_STREAM_INFORMATION_EX *pStreamInformation;
	PWSTR pszDisplayName;
	int dataType;
	PCWSTR Title;
	int iGroup;
	int iItem;
	UINT Type;
#if 0
	FS_CLUSTER_INFORMATION *pClusterInfo;
#endif
} STREAM_LIST_ITEM;

class CStreamInformationWindow : public CBaseWindow
{
	HWND m_hWndList;
	HWND m_hWndCtrlFocus;

	HFONT m_hFont;
	HFONT m_hFontHeader;

	PWSTR m_pszFilePath;
	DWORD m_dwFlags;

	STREAM_BUFFER m_stmAttributesStream;
	STREAM_BUFFER m_stmAlternateStream;

	DWORD m_dwClusterSize;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	typedef struct _SORT_PARAM {
		UINT id;
		INT direction; // must 1 or -1, do not use 0
	} SORT_PARAM;

	PWSTR m_pszErrorMessage;

public:
	CStreamInformationWindow()
	{
		m_hWndList = NULL;
		m_hWndCtrlFocus = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_pszErrorMessage = NULL;
		m_pszFilePath = NULL;
		m_dwClusterSize = 0;
		m_dwFlags = 0;
		ZeroMemory(&m_stmAttributesStream,sizeof(m_stmAttributesStream));
		ZeroMemory(&m_stmAlternateStream,sizeof(m_stmAlternateStream));
	}

	~CStreamInformationWindow()
	{
	}

	void FreeStreaBuffer()
	{
		LocalFree(m_stmAttributesStream.StreamInformation);
		m_stmAttributesStream.StreamInformation = NULL;
		m_stmAttributesStream.cStreams = 0;

		LocalFree(m_stmAlternateStream.StreamInformation);
		m_stmAlternateStream.StreamInformation = NULL;
		m_stmAlternateStream.cStreams = 0;
	}

	HRESULT SetFlags(DWORD dwFlags)
	{
		m_dwFlags = (dwFlags & (STMIF_EXCLUDE_ZERO_STREAM|STMIF_EXCLUDE_LESS_EQ_CLUSTER_SIZE));
		return S_OK;
	}

	HRESULT SetFile(PCWSTR pszFilePath,DWORD dwFileAttributes)
	{
		FILEITEMEX fiex = {};
		fiex.FileAttributes = dwFileAttributes;
		OnSetPath(pszFilePath,&fiex);
		return S_OK;
	}

protected:
	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		InitializeListView();
		return 0;
	}

	LRESULT OnDestroy(HWND,UINT,WPARAM,LPARAM)
	{
		if( m_hFont )
			DeleteObject(m_hFont);
		if( m_hFontHeader )
			DeleteObject(m_hFont);
		FreeStreaBuffer();
		_SafeMemFree(m_pszErrorMessage);
		_SafeMemFree( m_pszFilePath );
		return 0;
	}

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( m_hWndCtrlFocus )
			SetFocus(m_hWndCtrlFocus);
		return 0;
	}

	LRESULT OnEraseBkGnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hWnd,&rc);
		FillRect(hdc,&rc,GetSysColorBrush(COLOR_3DFACE));
		return (LRESULT)1;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		STREAM_LIST_ITEM *pItem = (STREAM_LIST_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

		if( pItem )
		{
			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");

			HMENU hSubMenu = CreatePopupMenu();
			{
				PWSTR *pColumns;
				int cColumns;
				ListViewEx_GetTextColumns(m_hWndList,&pColumns,&cColumns,NULL);

				for(int i = 0; i < cColumns; i++)
				{
					AppendMenu(hSubMenu,MF_STRING,ID_EDIT_COPY_COUMN_FIRST+i,pColumns[i]);
				}
				CoTaskMemFree(pColumns);
			}
			AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hSubMenu,L"Copy Co&lumn Text");

			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ListViewEx_SimpleContextMenuHandler(GetParent(hWnd),m_hWndList,(HWND)wParam,hMenu,pt,0);

			DestroyMenu(hMenu);
		}

		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;
		switch( pnmhdr->code )
		{
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case LVN_ITEMCHANGED:
				return OnItemChanged(pnmhdr);
			case LVN_COLUMNCLICK:
				return OnColumnClick(pnmhdr);
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		m_hWndCtrlFocus = pnmhdr->hwndFrom;

		// Message forward to parent window
		return ForwardNotificationToParent(m_hWnd,pnmhdr);
	}

	LRESULT OnGetEmptyMarkup(NMHDR *pnmhdr)
	{
		NMLVEMPTYMARKUP *pnmlvem = (NMLVEMPTYMARKUP *)pnmhdr;
		if( m_pszErrorMessage )
		{
			pnmlvem->dwFlags = EMF_CENTERED;
			StringCchCopy(pnmlvem->szMarkup,ARRAYSIZE(pnmlvem->szMarkup),m_pszErrorMessage);
			return TRUE;
		}
		return FALSE;
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW nmlv = *(NMLISTVIEW *)pnmhdr;
		nmlv.hdr.hwndFrom = m_hWnd;
		nmlv.hdr.idFrom = GetWindowLong(m_hWnd,GWL_ID);
		return SendMessage(GetParent(m_hWnd),WM_NOTIFY,(WPARAM)nmlv.hdr.idFrom,(LPARAM)&nmlv);
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW* pnmlv = (NMLISTVIEW*)pnmhdr;

		NT_FILE_STREAM_INFORMATION_EX *pSI = (NT_FILE_STREAM_INFORMATION_EX *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem,TRUE);

		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR* pnmhdr)
	{
		NMLVDISPINFO* pnmlvdi = (NMLVDISPINFO*)pnmhdr;

		if( pnmlvdi->item.mask & LVIF_IMAGE )
		{
			; // reserved
		}
		if( pnmlvdi->item.mask & LVIF_TEXT )
		{
			STREAM_LIST_ITEM *pItem = (STREAM_LIST_ITEM *)pnmlvdi->item.lParam;
	
			*pnmlvdi->item.pszText = L'\0';

			switch( pnmlvdi->item.iSubItem )
			{
				case 0:
					StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",pItem->pszDisplayName);
					break;
				case 1:
					_CommaFormatString(pItem->pStreamInformation->StreamSize.QuadPart,pnmlvdi->item.pszText);
					break;
				case 2:
					_CommaFormatString(pItem->pStreamInformation->StreamAllocationSize.QuadPart,pnmlvdi->item.pszText);
					break;
#if 0
				case 3:
					if( pItem->pClusterInfo && (pItem->pClusterInfo->ExtentCount > 0) )
					{
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->pClusterInfo->Extents[0].Lcn.QuadPart);
					}
					else
					{
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"-");
					}
					break;
#endif
			}
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW* pnmlv = (NMLISTVIEW*)pnmhdr;

		STREAM_LIST_ITEM *pItem = (STREAM_LIST_ITEM*)pnmlv->lParam;

		_SafeMemFree(pItem->pszDisplayName);
#if 0
		if( pItem->pClusterInfo )
			FreeClusterInformation( pItem->pClusterInfo );
#endif
		delete pItem;

		return 0;
	}

	LRESULT OnCommand(HWND,UINT,WPARAM wParam,LPARAM)
	{
		UINT CmdId = LOWORD(wParam);

		switch( CmdId )
		{
			case ID_EDIT_COPY:
				if( GetKeyState(VK_SHIFT) < 0 )
				{
					SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,1);
				}
				else
				{
					SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
				}
				break;
			default:
				if( ID_EDIT_COPY_COUMN_FIRST <= CmdId && CmdId <= ID_EDIT_COPY_COUMN_LAST )
					SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,(CmdId-ID_EDIT_COPY_COUMN_FIRST));
				break;
		}

		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return OnEraseBkGnd(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
		    case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case PPM_SETPATH:
				return OnSetPath((PCWSTR)lParam,(FILEITEMEX *)wParam);
			case PPM_CHDIR:
				return OnChDir((PCWSTR)lParam,(FILEITEMEX *)wParam);
			case PPM_SELECTSTREAM:
				return OnSelItem(wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		STREAM_LIST_ITEM *pItem1 = (STREAM_LIST_ITEM *)lParam1;
		STREAM_LIST_ITEM *pItem2 = (STREAM_LIST_ITEM *)lParam2;

		SORT_PARAM *psp = (SORT_PARAM *)lParamSort;

		int iRet = 0;

		switch( psp->id )
		{
			case 0:
				iRet = wcscmp(pItem1->pStreamInformation->StreamName,pItem2->pStreamInformation->StreamName);
				break;
			case 1:
				iRet = _COMP(pItem1->pStreamInformation->StreamSize.QuadPart,pItem2->pStreamInformation->StreamSize.QuadPart);
				break;
			case 2:
				iRet = _COMP(pItem1->pStreamInformation->StreamAllocationSize.QuadPart,pItem2->pStreamInformation->StreamAllocationSize.QuadPart);
				break;
#if 0
			case 3:
			{
				#define	_VALID_ITEM(pItem)	(pItem->pClusterInfo && (pItem->pClusterInfo->ExtentCount > 0))
				if( !_VALID_ITEM(pItem1) && _VALID_ITEM(pItem2) )
					return 1;
				else if( _VALID_ITEM(pItem1) && !_VALID_ITEM(pItem2) )
					return -1;
				else if( !_VALID_ITEM(pItem1) && !_VALID_ITEM(pItem2) )
					return 0;
				else
				{
					iRet = _COMP(pItem1->pClusterInfo->Extents[0].Lcn.QuadPart,pItem2->pClusterInfo->Extents[0].Lcn.QuadPart);
				}
			}
#endif
		}

		iRet *= psp->direction; 

		return iRet;
	}

	void DoSort(int iSubItem=-1,BOOL bToggle=FALSE)
	{
		if( m_Sort.CurrentSubItem != -1 )  //  previous sort column
			ListViewEx_SetHeaderArrow(m_hWndList,m_Sort.CurrentSubItem,0); // clear previous column header mark

		if( bToggle )
		{
			if( m_Sort.CurrentSubItem != iSubItem )
				m_Sort.Direction = 1;     // current column changed: new current column always ascend sort.
			else
				m_Sort.Direction *= -1;   // toggle.
		}

		SORT_PARAM op = {0};
		op.id              = iSubItem;
		op.direction       = m_Sort.Direction; // must 1 or -1, do not use 0
		ListView_SortItems(m_hWndList,&CStreamInformationWindow::CompareProc,&op);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	VOID UpdateLayout(int cx=-1,int cy=-1,BOOL absSplitPos=FALSE)
	{
		if( cx == -1 && cy == -1 )
		{
			RECT rc;
			GetClientRect(m_hWnd,&rc);
			cx = _RECT_WIDTH(rc);
			cy = _RECT_HIGHT(rc);
		}

		HDWP hdwp = BeginDeferWindowPos(1);

		DeferWindowPos(hdwp,m_hWndList,NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOCOPYBITS);

		EndDeferWindowPos(hdwp);
	}

	void InitializeListView()
	{
		m_hWndList = CreateWindowEx(0,
							  WC_LISTVIEW, 
                              L"", 
							  WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_TABSTOP|LVS_REPORT|LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		m_hFont = GetGlobalFont(m_hWnd);
		m_hFontHeader = GetIconFont();

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_INFOTIP|LVS_EX_LABELTIP);
	
		_EnableVisualThemeStyle(m_hWndList);
	
		RECT rc;
		GetClientRect(m_hWndList,&rc);
		int cx = _RECT_WIDTH(rc);

		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;

		struct {
			PWSTR Text;
			int fmt;
		} Columns[] = {
			{L"Name",            LVCFMT_LEFT},
			{L"Size",            LVCFMT_RIGHT},
			{L"Allocation Size", LVCFMT_RIGHT},
		};

		{
			lvc.fmt = LVCFMT_LEFT;
			lvc.pszText = L"";
			lvc.cx = 0;
			lvc.iSubItem = 0;
			lvc.iOrder = 0;
			ListView_InsertColumn(m_hWndList,0,&lvc);
		}

		for(int i = 0; i < ARRAYSIZE(Columns); i++)
		{
			lvc.fmt = Columns[i].fmt;
			lvc.pszText = Columns[i].Text;
			lvc.cx = (int)((double)cx * 0.3);
			lvc.iSubItem = i+1;
			ListView_InsertColumn(m_hWndList,i+1,&lvc);
		}

		// If a column is added to a list-view control with index 0 (the leftmost column),
		// it is always LVCFMT_LEFT. Setting other flags on column 0 does not override that alignment. 
		// Therefore if you keep inserting columns with index 0, the text in all columns are left-aligned. 
		// If you want the first column to be right-aligned or centered you can make a dummy column, 
		// then insert one or more columns with index 1 or higher and specify the alignment you require.
		// Finally delete the dummy column
		ListView_DeleteColumn(m_hWndList,0);

		ListView_SetColumnWidth(m_hWndList,0,DPI_SIZE_CX(340));
		ListView_SetColumnWidth(m_hWndList,1,DPI_SIZE_CX(140));
		ListView_SetColumnWidth(m_hWndList,2,DPI_SIZE_CX(140));

		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction = -1;

		setErrorMessage( L"No stream data." );
	}

	int InsertItem(HWND hWndList,int iItem,int iType,NT_FILE_STREAM_INFORMATION_EX *pSI,PVOID)
	{
		STREAM_LIST_ITEM *pItem = new STREAM_LIST_ITEM;
		if( pItem == NULL )
			return -1;

		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);
	
		ZeroMemory(pItem,sizeof(STREAM_LIST_ITEM));

		pItem->pStreamInformation = pSI;

		WCHAR szName[MAX_PATH];
		StringCchCopy(szName,MAX_PATH,pSI->StreamName);

		//if( wcscmp(szName,L"::$DATA") == 0 )
		//{
		//	pItem->pszDisplayName = _MemAllocString( L"$DATA" );
		//}
		if( szName[0] == L':' && szName[1] == L':' )
		{
			// default (no name) stream
			pItem->pszDisplayName = _MemAllocString( &szName[2] );
		}
		else
		{
			// named stream
			pItem->pszDisplayName = &szName[1];
			WCHAR *pSep = wcschr(pItem->pszDisplayName,L':');
			if( pSep != NULL )
			{
				*pSep = L'\0';
				pItem->pszDisplayName = _MemAllocString(pItem->pszDisplayName);
			}
			else
			{
				pItem->pszDisplayName = _MemAllocString(pItem->pStreamInformation->StreamName);
			}
		}

#if 0
		PWSTR pszFullyQualifiedStreamName = _MemAllocStringCat(m_pszFilePath,pSI->StreamName);
		if( pszFullyQualifiedStreamName )
		{
			FS_CLUSTER_INFORMATION *pFCI = CreateClusterInformationBuffer(pszFullyQualifiedStreamName);
			if( pFCI )
			{
				pItem->pClusterInfo = pFCI;
			}
			_SafeMemFree(pszFullyQualifiedStreamName);
		}
#endif

		LVITEM lvi = {0};
		lvi.mask    = LVIF_PARAM|LVIF_TEXT|LVIF_IMAGE;
		lvi.pszText = (PWSTR)LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		lvi.iItem   = iItem;

		iItem = ListView_InsertItem(hWndList,&lvi);
	
		return iItem;
	}

	//VOID FillListItems()
	//{
	//	ULONG index;
	//	for(index = 0; index < m_StreamCount; index++)
	//	{
	//		InsertItem(m_hWndList,index,0,&m_pStreamInformation[index],0);
	//	}
	//
	//	DoSort(m_Sort.CurrentSubItem,FALSE);
	//}

	void makeErrorMessage( PCWSTR psz )
	{
		_SafeMemFree( m_pszErrorMessage );
		m_pszErrorMessage = _MemAllocString(psz);
	}

	void setErrorMessage( PCWSTR psz )
	{
		makeErrorMessage( psz );
	}

	void setErrorMessage( HRESULT hr )
	{
		WCHAR sz[MAX_PATH];
		StringCchPrintf(sz,MAX_PATH,L"Error Code: 0x%08X",hr);
	}

	LRESULT OnSelItem(WPARAM,LPARAM)
	{
		if( ListView_GetItemCount(m_hWndList) > 0 )
		{
			ListViewEx_SetCurSel(m_hWndList,0);
			return 0;
		}
		return (LRESULT)-1;
	}

	LRESULT OnChDir(PCWSTR pszFilePath,FILEITEMEX *pFIEx)
	{
		return 0;
	}

	LRESULT OnSetPath(PCWSTR pszFilePath,FILEITEMEX *pFIEx)
	{
		LRESULT lResult = 0;

		if( pszFilePath == NULL && pFIEx == NULL )
		{
			// Clear list
			SetRedraw(m_hWndList,FALSE);
			ListView_DeleteAllItems(m_hWndList);
			SetRedraw(m_hWndList,TRUE);
			ListViewEx_ResetEmptyText(m_hWndList);
			return 0;
		}

		// 
		{
			HANDLE hFile;
			NTSTATUS Status;
			m_dwClusterSize = 4096;
			Status = OpenFileEx_W(&hFile,pszFilePath,GENERIC_READ|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);
			if( Status == STATUS_SUCCESS )
			{
				NT_VOLUME_SIZE_INFORMATION Size;
				if( NtDosGetVolumeSizeInformation(hFile,&Size,sizeof(Size)) == STATUS_SUCCESS )
					m_dwClusterSize = Size.ClusterSize;
				CloseHandle(hFile);
			}
		}

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		_SafeMemFree(m_pszFilePath);
		m_pszFilePath = _MemAllocString( pszFilePath );

		if( m_pszFilePath == NULL )
		{
			SetRedraw(m_hWndList,FALSE);
			return 0;
		}

		FreeStreaBuffer();

		HRESULT hr;
		NT_FILE_STREAM_INFORMATION_EX *pStmNames = NULL;
		int cStmNames = 0;

		//
		// Attribute Stream
		//
		hr = NtDosGetAttributeTypeCodeStreams( pszFilePath,
					(pFIEx->FileAttributes & FILE_ATTRIBUTE_DIRECTORY),
					&pStmNames, &cStmNames );

		if( hr == S_OK )
		{
			m_stmAttributesStream.cStreams = cStmNames;
			m_stmAttributesStream.StreamInformation = pStmNames; // free memory using LocalFree later.

			for(int index = 0, iItem = 0; index < cStmNames; index++)
			{
				if( (m_dwFlags & STMIF_EXCLUDE_ZERO_STREAM) &&
					(m_stmAttributesStream.StreamInformation[index].StreamAllocationSize.QuadPart == 0 && 
					 m_stmAttributesStream.StreamInformation[index].StreamSize.QuadPart == 0) )
				{
					continue;
				}

				if( (m_dwFlags & STMIF_EXCLUDE_LESS_EQ_CLUSTER_SIZE) && 
					(m_stmAttributesStream.StreamInformation[index].StreamAllocationSize.QuadPart <= m_dwClusterSize) )
				{
					continue;
				}

				InsertItem(m_hWndList,iItem++,0,&m_stmAttributesStream.StreamInformation[index],0);
			}
		}

		//
		// Alternate Stream
		//
		hr = NtDosGetAlternateStreams( pszFilePath, &pStmNames, &cStmNames );

		if( hr == S_OK )
		{
			m_stmAlternateStream.cStreams = cStmNames;
			m_stmAlternateStream.StreamInformation = pStmNames; // free memory using LocalFree later.

            int cBaseIndex = ListView_GetItemCount(m_hWndList);
			for(int index = 0,iItem = 0; index < cStmNames; index++)
			{
				if( wcscmp(m_stmAlternateStream.StreamInformation[index].StreamName,L"::$DATA") != 0 ) {
					InsertItem(m_hWndList,cBaseIndex+iItem,0,&m_stmAlternateStream.StreamInformation[index],0);
					iItem++;
				}
			}
		}

		DoSort(m_Sort.CurrentSubItem,FALSE);

	    if( ListView_GetItemCount(m_hWndList) == 0 )
		{
			//
			// Update and Reset empty text
			//
			if( ERROR_NO_MORE_ITEMS == HRESULT_CODE(hr) )
			{
				setErrorMessage( L"No stream data." );
			}
			else
			{
				setErrorMessage( hr );
			}
			ListViewEx_ResetEmptyText(m_hWndList);
		}

		SetRedraw(m_hWndList,TRUE);

		return lResult;
	}
};
