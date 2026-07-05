#pragma once
//*****************************************************************************
//
//  pane_clusterinfo.h
//
//  PURPOSE: Cluster Information Pane.
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
#include "ntvolumehelp.h"

#define _LPWSTR_EMPTY_MESSAGE L"No clusters"

#define _CLR_WORNING_TEXT_COLOR RGB(210,0,0)

typedef struct _FCI_LIST_ITEM
{
	FILEITEMEX *pFIEx;
	UINT Type;
	FS_CLUSTER_INFORMATION *pClusterInfo;
	struct {
		FS_CLUSTER_LOCATION *Location;
		FS_VOLUME_PHYSICAL_OFFSET *PhysicalOffset;
	} Cluster;
} FCI_LIST_ITEM;

inline int _IsCompValidType(FCI_LIST_ITEM *pItem1,FCI_LIST_ITEM *pItem2) {
	if( pItem1->Type == 0 && pItem2->Type != 0 )
		return -1;
	if( pItem1->Type != 0 && pItem2->Type == 0 )
		return 1;
	if( pItem1->Type != 0 && pItem2->Type != 0 )
		return 0;
	return 0;
}

class CClusterInformationWindow : public CBaseWindow
{
	HWND m_hWndList;
	HWND m_hWndCtrlFocus;

	HFONT m_hFont;
	HFONT m_hFontHeader;

	DWORD m_dwFileSystemFlags;
	DWORD m_dwFileSystemType;

	FS_CLUSTER_INFORMATION *m_pci;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	typedef struct _SORT_PARAM {
		UINT id;
		INT direction; // must 1 or -1, do not use 0
	} SORT_PARAM;

	PWSTR m_pszErrorMessage;

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

	void FreeClusterData()
	{
		if( m_pci ) {
			FreeClusterInformation(m_pci);
			m_pci = NULL;
		}
	}

public:
	CClusterInformationWindow()
	{
		m_hWndList = NULL;
		m_hWndCtrlFocus = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_pci = NULL;
		m_dwFileSystemFlags = 0;
		m_dwFileSystemType = 0;
		m_pszErrorMessage = NULL;
	}

	~CClusterInformationWindow()
	{
		FreeClusterData();
		_SafeMemFree(m_pszErrorMessage);
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		InitializeListView();
		return 0;
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
		lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH|LVCF_ORDER|LVCF_SUBITEM;

		struct {
			PWSTR Text;
			int iSubItem;
			int fmt;
		} Columns[] = {
			{L"Offset",            0,LVCFMT_LEFT},
			{L"VCN",               1,LVCFMT_RIGHT},
			{L"LCN Start",         2,LVCFMT_RIGHT},
			{L"LCN End",           3,LVCFMT_RIGHT},
			{L"Cluster Count",     4,LVCFMT_RIGHT},
			{L"Logical Offset",    5,LVCFMT_RIGHT},
			{L"Physical Drive",    6,LVCFMT_LEFT},
			{L"Physical Offset",   7,LVCFMT_RIGHT},
			{L"Extent Size",       8,LVCFMT_RIGHT},
		};

		// Insert dummy column
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
			lvc.iSubItem = Columns[i].iSubItem;
			lvc.iOrder = i;
			ListView_InsertColumn(m_hWndList,i+1,&lvc);
		}

		// If a column is added to a list-view control with index 0 (the leftmost column),
		// it is always LVCFMT_LEFT. Setting other flags on column 0 does not override that alignment. 
		// Therefore if you keep inserting columns with index 0, the text in all columns are left-aligned. 
		// If you want the first column to be right-aligned or centered you can make a dummy column, 
		// then insert one or more columns with index 1 or higher and specify the alignment you require.
		// Finally delete the dummy column
		ListView_DeleteColumn(m_hWndList,0);

		int cColumns = ListViewEx_GetColumnCount(m_hWndList);
		for(int i = 0; i < cColumns; i++)
			ListView_SetColumnWidth(m_hWndList,i,LVSCW_AUTOSIZE_USEHEADER);

		setErrorMessage( _LPWSTR_EMPTY_MESSAGE );
	}

	int InsertItem(HWND hWndList,PVOID,int iItem,int iType,FS_CLUSTER_LOCATION *pcl,FS_VOLUME_PHYSICAL_OFFSET *pvpo)
	{
		FCI_LIST_ITEM *pItem = new FCI_LIST_ITEM;
		if( pItem == NULL )
			return -1;

		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);
	
		ZeroMemory(pItem,sizeof(FCI_LIST_ITEM));
		pItem->Type                   = iType;
		pItem->Cluster.Location       = (FS_CLUSTER_LOCATION *)pcl;
		pItem->Cluster.PhysicalOffset = (FS_VOLUME_PHYSICAL_OFFSET *)pvpo;
		pItem->pClusterInfo           = m_pci;
//++ hack
		if( m_dwFileSystemType & (0x00000001|0x00008000|0x00000200) )
			pItem->pClusterInfo->ClusterHeapBase.ValidFileAreaOffset = true;
//-- hack
		LVITEM lvi = {0};
		lvi.mask    = LVIF_PARAM|LVIF_TEXT|LVIF_IMAGE;
		lvi.pszText = (PWSTR)LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		lvi.iItem   = iItem;

		iItem = ListView_InsertItem(hWndList,&lvi);
	
		return iItem;
	}

	VOID FillListItems()
	{
		FS_CLUSTER_INFORMATION *pci = m_pci;

		ULONG ext;
		for(ext = 0; ext < pci->ExtentCount; ext++)
		{
			InsertItem(m_hWndList,0,ext,0,&pci->Extents[ext],NULL);

			if( pci->Extents[ext].PhysicalOffsets && pci->Extents[ext].PhysicalOffsets->NumberOfPhysicalOffsets > 1 )
			{
				for( UINT index = 1; index < pci->Extents[ext].PhysicalOffsets->NumberOfPhysicalOffsets; index++ )
				{
					InsertItem(m_hWndList,0,-1,1,
							&pci->Extents[ext],
							&pci->Extents[ext].PhysicalOffsets->PhysicalOffset[index]);
				}
			}
		}

		DoSort(m_Sort.CurrentSubItem,FALSE);
	}

	LRESULT OnSetPath(PCWSTR pszCurPath,FILEITEMEX *)
	{
		if( pszCurPath == NULL )
		{
			SetRedraw(m_hWndList,FALSE);
			ListView_DeleteAllItems(m_hWndList);
			FreeClusterData();
			SetRedraw(m_hWndList,TRUE);
			ListViewEx_ResetEmptyText(m_hWndList);
			return 0;
		}

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		FreeClusterData();

		PCWSTR pszFilePath = pszCurPath;

		HANDLE hFile;
		NTSTATUS Status;
		if( (Status = OpenFileEx_W(&hFile,pszFilePath,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN_FOR_BACKUP_INTENT)) == 0 )
		{
			NtDosGetVolumeInformationFlags(hFile,&m_dwFileSystemFlags,&m_dwFileSystemType);
			CloseHandle(hFile);
		}

		ASSERT(m_pci == NULL);

		m_pci = (FS_CLUSTER_INFORMATION *)CreateClusterInformationBuffer(pszFilePath);
		if( m_pci == NULL )
		{
			DWORD dwError = GetLastError();
			setErrorMessage( dwError );
		}
		else
		{
			FillListItems();
		}

		if( ListView_GetItemCount(m_hWndList) > 0 ) 
		{ 
			ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE_USEHEADER);
			ListViewEx_SetLastColumnWidth(m_hWndList,0);
		}

		SetRedraw(m_hWndList,TRUE);

		ListViewEx_ResetEmptyText(m_hWndList);

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
		    case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case PPM_SETPATH:
				return OnSetPath((PCWSTR)lParam,(FILEITEMEX *)wParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	LRESULT OnDestroy(HWND,UINT,WPARAM,LPARAM)
	{
		if( m_hFont )
			DeleteObject(m_hFont);
		if( m_hFontHeader )
			DeleteObject(m_hFont);
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

		NT_FILE_STREAM_INFORMATION_EX *pItem = (NT_FILE_STREAM_INFORMATION_EX *)ListViewEx_GetItemData(m_hWndList,iItem);

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

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;
		switch( pnmhdr->code )
		{
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case NM_CUSTOMDRAW:
				return OnCustomDraw(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo((NMLVDISPINFO*)pnmhdr);
			case LVN_DELETEITEM:
				OnDeleteItem((NMLISTVIEW*)lParam);
				return TRUE;
			case LVN_COLUMNCLICK:
				return OnColumnClick((NMHDR*)lParam);
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

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		NT_FILE_STREAM_INFORMATION_EX *pSI = (NT_FILE_STREAM_INFORMATION_EX *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem,TRUE);

		return 0;
	}

	LRESULT OnCustomDraw(NMHDR *pnmhdr)
	{
		NMLVCUSTOMDRAW *pnmlvcd = (NMLVCUSTOMDRAW *)pnmhdr;

		if( pnmhdr->hwndFrom != m_hWndList )
			return 0;

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			return CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT|CDRF_NEWFONT;
		}

		if( pnmlvcd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT|CDDS_SUBITEM) )
		{
			HWND hwndHeader = ListView_GetHeader(m_hWndList);
			HDITEM hi;
			hi.mask = HDI_LPARAM;
			Header_GetItem(hwndHeader,pnmlvcd->iSubItem,&hi);

			FCI_LIST_ITEM *pItem = (FCI_LIST_ITEM *)pnmlvcd->nmcd.lItemlParam;

			switch( (int)(hi.lParam) )
			{
				case 5: // Logical Offset
				case 7: // Physical Offset
					if( pItem->pClusterInfo->ClusterHeapBase.ValidFileAreaOffset == 0 )
						pnmlvcd->clrText = _CLR_WORNING_TEXT_COLOR;
					else
						pnmlvcd->clrText = ListView_GetTextColor(m_hWndList);
					break;
				default:
					pnmlvcd->clrText = ListView_GetTextColor(m_hWndList);
					break;
			}
			return CDRF_NOTIFYPOSTPAINT;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)
		{
			if( IsXpThemeEnabled() )
			{
				if( pnmlvcd->nmcd.uItemState & CDIS_FOCUS )
				{
					RECT rcFocus;
					ListView_GetItemRect(m_hWndList,pnmlvcd->nmcd.dwItemSpec,&rcFocus,LVIR_SELECTBOUNDS);
					DrawFocusFrame(m_hWndList,pnmlvcd->nmcd.hdc,&rcFocus);
				}
			}
		}
		return CDRF_DODEFAULT;
	}

	LRESULT OnDeleteItem(NMLISTVIEW* pnmlv)
	{
		FCI_LIST_ITEM *p = (FCI_LIST_ITEM*)pnmlv->lParam;
		delete p;
		return 0;
	}

	LRESULT OnGetDispInfo(NMLVDISPINFO* pnmlvdi)
	{
		if( pnmlvdi->item.mask & LVIF_IMAGE )
		{
			; // reserved
		}
		if( pnmlvdi->item.mask & LVIF_TEXT )
		{
			FCI_LIST_ITEM *pItem = (FCI_LIST_ITEM *)pnmlvdi->item.lParam;
	
			*pnmlvdi->item.pszText = L'\0';

			StringCchCopy(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax, L"-");

			LVCOLUMN col = {0};
			col.mask = LVCF_SUBITEM;
			ListView_GetColumn(m_hWndList,pnmlvdi->item.iSubItem,&col);

			switch( col.iSubItem )
			{
				case 0: // Cluster Offset
				{
					LARGE_INTEGER li;
					li.QuadPart = pItem->Cluster.Location->Vcn.QuadPart * m_pci->BytesPerCluster;
					StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%08X`%08X",li.HighPart,li.LowPart);
					break;
				}
				case 1: // VCN
					if( pItem->Type == 0 )
					{
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->Vcn.QuadPart);
					}
					break;
				case 2: // LCN Start
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->Lcn.QuadPart != -1 )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->Lcn.QuadPart);
					}
					break;
				case 3: // LCN End
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->Lcn.QuadPart != -1 )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->Lcn.QuadPart
								+ pItem->Cluster.Location->Count.QuadPart - 1);
					}
					break;
				case 4: // Cluster Count
					if( pItem->Type == 0 )
					{
						_CommaFormatString(pItem->Cluster.Location->Count.QuadPart,pnmlvdi->item.pszText);
					}
					break;
				case 5: // Logical Offset
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->Lcn.QuadPart != -1 )
						{
							if( !pItem->pClusterInfo->ClusterHeapBase.ValidFileAreaOffset )
							{
								StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",
									pItem->pClusterInfo->BytesPerCluster * pItem->Cluster.Location->Lcn.QuadPart);
							}
							else
							{
								StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",
									(pItem->pClusterInfo->BytesPerCluster * pItem->Cluster.Location->Lcn.QuadPart)
										+(pItem->pClusterInfo->ClusterHeapBase.FileAreaOffset.QuadPart * pItem->pClusterInfo->BytesPerSector));
							}
						}
					}
					break;
				case 6: // Physical Drive
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->PhysicalOffsets )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"PhysicalDrive%d",pItem->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].DiskNumber);
					}
					else
					{
						if( pItem->Cluster.PhysicalOffset )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"PhysicalDrive%d",pItem->Cluster.PhysicalOffset->DiskNumber);
					}
					break;
				case 7: // Physical Offset
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->PhysicalOffsets )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].Offset);
					}
					else
					{
						if( pItem->Cluster.PhysicalOffset )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.PhysicalOffset->Offset);
					}
					break;
				case 8: // Extent Size
					StrFormatByteSizeW((pItem->pClusterInfo->BytesPerCluster * pItem->Cluster.Location->Count.QuadPart),pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
					break;
			}
		}
		return 0;
	}

	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		FCI_LIST_ITEM *pItem1 = (FCI_LIST_ITEM *)lParam1;
		FCI_LIST_ITEM *pItem2 = (FCI_LIST_ITEM *)lParam2;

		SORT_PARAM *psp = (SORT_PARAM *)lParamSort;

		int iRet = 0;

		switch( psp->id )
		{
			case 0: // Cluster Offset
			{
				iRet = _COMP(pItem1->Cluster.Location->Vcn.QuadPart,pItem2->Cluster.Location->Vcn.QuadPart);
				break;
			}
			case 1: // VCN
			{
				if( (iRet = _IsCompValidType(pItem1,pItem2)) == 0 )
					iRet = _COMP(pItem1->Cluster.Location->Vcn.QuadPart,pItem2->Cluster.Location->Vcn.QuadPart);
				break;
			}
			case 2: // LCN Start
			{
				if( (iRet = _IsCompValidType(pItem1,pItem2)) == 0 )
				{
					if( pItem1->Cluster.Location->Lcn.QuadPart == -1 && pItem2->Cluster.Location->Lcn.QuadPart != -1 )
						return 1;
					else if( pItem1->Cluster.Location->Lcn.QuadPart != -1 && pItem2->Cluster.Location->Lcn.QuadPart == -1 )
						return -1;
					else if( pItem1->Cluster.Location->Lcn.QuadPart == -1 && pItem2->Cluster.Location->Lcn.QuadPart == -1 )
						iRet = 0;
					else
						iRet = _COMP(pItem1->Cluster.Location->Lcn.QuadPart,pItem2->Cluster.Location->Lcn.QuadPart);
				}
				break;
			}
			case 3: // LCN End
			{
				if( (iRet = _IsCompValidType(pItem1,pItem2)) == 0 )
				{
					if( pItem1->Cluster.Location->Lcn.QuadPart == -1 && pItem2->Cluster.Location->Lcn.QuadPart != -1 )
						return 1;
					else if( pItem1->Cluster.Location->Lcn.QuadPart != -1 && pItem2->Cluster.Location->Lcn.QuadPart == -1 )
						return -1;
					else if( pItem1->Cluster.Location->Lcn.QuadPart == -1 && pItem2->Cluster.Location->Lcn.QuadPart == -1 )
						iRet = 0;
					else
					{
						iRet = _COMP((pItem1->Cluster.Location->Lcn.QuadPart + pItem1->Cluster.Location->Count.QuadPart - 1),
									(pItem2->Cluster.Location->Lcn.QuadPart + pItem2->Cluster.Location->Count.QuadPart - 1));
					}
				}
				break;
			}
			case 4: // Cluster Count
			{
				if( (iRet = _IsCompValidType(pItem1,pItem2)) == 0 )
				{
					iRet = _COMP(pItem1->Cluster.Location->Count.QuadPart,pItem2->Cluster.Location->Count.QuadPart);
				}
				break;
			}
			case 5: // Logical Offset
			{
				if( (iRet = _IsCompValidType(pItem1,pItem2)) == 0 )
				{
					if( pItem1->Cluster.Location->Lcn.QuadPart == -1 && pItem2->Cluster.Location->Lcn.QuadPart != -1 )
						return 1;
					else if( pItem1->Cluster.Location->Lcn.QuadPart != -1 && pItem2->Cluster.Location->Lcn.QuadPart == -1 )
						return -1;
					else if( pItem1->Cluster.Location->Lcn.QuadPart == -1 && pItem2->Cluster.Location->Lcn.QuadPart == -1 )
						iRet = 0;
					else
					{
						iRet = _COMP(
							((pItem1->pClusterInfo->BytesPerCluster * pItem1->Cluster.Location->Lcn.QuadPart)+(pItem1->pClusterInfo->ClusterHeapBase.FileAreaOffset.QuadPart * pItem1->pClusterInfo->BytesPerSector)),
							((pItem2->pClusterInfo->BytesPerCluster * pItem2->Cluster.Location->Lcn.QuadPart)+(pItem2->pClusterInfo->ClusterHeapBase.FileAreaOffset.QuadPart * pItem2->pClusterInfo->BytesPerSector))
							);
					}
					break;
				}
			}
			case 6: // Physical Drive
			{
				LONG64 n1,n2;

				if( pItem1->Type == 0 )
				{
					if( pItem1->Cluster.Location->PhysicalOffsets )
						n1 = pItem1->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].DiskNumber;
					else
						n1 = -1;
				}
				else
				{
					if( pItem1->Cluster.PhysicalOffset )
						n1 = pItem1->Cluster.PhysicalOffset->DiskNumber;
					else
						n1 = -1;
				}

				if( pItem2->Type == 0 )
				{
					if( pItem2->Cluster.Location->PhysicalOffsets )
						n2 = pItem2->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].DiskNumber;
					else
						n2 = -1;
				}
				else
				{
					if( pItem2->Cluster.PhysicalOffset )
						n2 = pItem2->Cluster.PhysicalOffset->DiskNumber;
					else
						n2 = -1;
				}

				if( n1 == -1 && n2 != -1 )
					return 1;
				else if( n1 != -1 && n2 == -1 )
					return -1;
				else if( n1 == -1 && n2 == -1 )
					return 0;
				else
					iRet = _COMP(n1,n2);
				break;
			}
			case 7: // Physical Offset
			{
				LONG64 n1,n2;

				if( pItem1->Type == 0 )
				{
					if( pItem1->Cluster.Location->PhysicalOffsets )
						n1 = pItem1->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].Offset;
					else
						n1 = -1;
				}
				else
				{
					if( pItem1->Cluster.PhysicalOffset )
						n1 = pItem1->Cluster.PhysicalOffset->Offset;
					else
						n1 = -1;
				}

				if( pItem2->Type == 0 )
				{
					if( pItem2->Cluster.Location->PhysicalOffsets )
						n2 = pItem2->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].Offset;
					else
						n2 = -1;
				}
				else
				{
					if( pItem2->Cluster.PhysicalOffset )
						n2 = pItem2->Cluster.PhysicalOffset->Offset;
					else
						n2 = -1;
				}

				if( n1 == -1 && n2 != -1 )
					return 1;
				else if( n1 != -1 && n2 == -1 )
					return -1;
				else if( n1 == -1 && n2 == -1 )
					return 0;
				else
					iRet = _COMP(n1,n2);
				break;
			}
			case 8: // Extent Size
			{
				iRet = _COMP(
							(pItem1->pClusterInfo->BytesPerCluster * pItem1->Cluster.Location->Count.QuadPart),
							(pItem2->pClusterInfo->BytesPerCluster * pItem2->Cluster.Location->Count.QuadPart)
							);
				break;
			}
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
		ListView_SortItems(m_hWndList,&CClusterInformationWindow::CompareProc,&op);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}
};
