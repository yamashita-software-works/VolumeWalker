#pragma once
//***************************************************************************
//*                                                                         *
//*  page_diskperformance.h                                                 *
//*                                                                         *
//*  Disk performance Page                                                  *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2025-01-04                                                    *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "string_def.h"
#include "ntobjecthelp.h"
#include "findhandler.h"
#include "common_control_helper.h"

inline LONGLONG _getmsec(LONGLONG val) {
	return (val / 10000);   // 100ns to msecond
}

class CDiskPerformancePage :
	public CPageWndBase,
	public CFindHandler<CDiskPerformancePage>
{
	HWND  m_hWndList;
	HFONT m_hFont;
	PWSTR m_pszErrorMessage;

	PWSTR m_pszVolumeDisk;
	WCHAR m_szVolumeDiskName[MAX_PATH];

	DISK_PERFORMANCE m_DiskPerf;

public:
	HWND GetListView() const { return m_hWndList; }

public:
	CDiskPerformancePage()
	{
		m_hWndList = NULL;
		m_hFont = NULL;
		m_pszVolumeDisk = NULL;
		m_pszErrorMessage = NULL;
		ZeroMemory(&m_DiskPerf,sizeof(m_DiskPerf));
		ZeroMemory(m_szVolumeDiskName,sizeof(m_szVolumeDiskName));
	}

	~CDiskPerformancePage()
	{
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | LVS_SINGLESEL, 
                              0,0,0,0,
                              hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		InitList(m_hWndList);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_hFont )
			DeleteObject( m_hFont );

		_SafeMemFree( m_pszVolumeDisk );

		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnSetFocus(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SetFocus(m_hWndList);
		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		HMENU hMenu = CreatePopupMenu();

		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_STOP_DISKPERFORMANCE,L"&Stop Disk Performance");

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case NM_CUSTOMDRAW:
				return OnCustomDraw(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case LVN_ITEMCHANGED:
				return OnItemChanged(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case LVN_ITEMACTIVATE:
				return OnItemActivate(pnmhdr);
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
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

	LRESULT OnCustomDraw(NMHDR *pnmhdr)
	{
		NMLVCUSTOMDRAW *pnmlvcd = (NMLVCUSTOMDRAW *)pnmhdr;

		if( pnmlvcd->nmcd.hdr.hwndFrom != m_hWndList )
			return CDRF_DODEFAULT;

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			SelectObject(pnmlvcd->nmcd.hdc,m_hFont);
			return CDRF_NEWFONT;
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( ((pnmlv->uOldState == 0)|| ((pnmlv->uNewState & (LVIS_SELECTED)) == (LVIS_SELECTED))) 
			 && ((pnmlv->uNewState & (LVIS_SELECTED|LVIS_FOCUSED)) == (LVIS_SELECTED|LVIS_FOCUSED)) )
		{
			;
		}

		return 0;
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

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
				return CFindHandler<CDiskPerformancePage>::OnFindItem(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
		{
			SetWindowPos(m_hWndList,NULL,
					0,
					0,
					cx,
					cy,
					SWP_NOZORDER);
		}
	}

	HRESULT InitList(HWND hWndList)
	{
		_EnableVisualThemeStyle(hWndList);

		m_hFont = GetGlobalFont(m_hWnd);
		
		ListView_SetExtendedListViewStyle(hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

		InitColumns(hWndList);
		InitTitles(hWndList);

		ListView_SetSelectedColumn(hWndList,0);

		return S_OK;
	}

	BOOL InitColumns(HWND hWndList)
	{
		HWND hwndHeader;
		hwndHeader = ListView_GetHeader(hWndList);

		int cxTitleWidth = 200;
		int cxValueWidth = 280;
		int iCol = 0;

		LVCOLUMN lvc = {0};
		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		lvc.fmt     = 0;
		lvc.pszText = L"Name";
		lvc.cx      = cxTitleWidth;
		ListView_InsertColumn(hWndList,iCol++,&lvc);

		lvc.pszText = L"Value";
		lvc.cx      = cxValueWidth;
		lvc.fmt     = LVCFMT_LEFT;
		ListView_InsertColumn(hWndList,iCol++,&lvc);

		return TRUE;
	}

	enum {
		dtBytesRead = 1,
		dtBytesWritten,
		dtReadTime,
		dtWriteTime,
		dtIdleTime,
		dtReadCount,
		dtWriteCount,
		dtQueueDepth,
		dtSplitCount,
		dtQueryTime,
		dtStorageDeviceNumber,
		dtStorageManagerName
	};

	VOID InitTitles(HWND hWndList)
	{
		int iItem = 0;

		struct {
			PWSTR pszTitle;
			int DataId;
		} ItemDef[] = {
			{L"Bytes Read",      dtBytesRead},
			{L"Bytes Written",   dtBytesWritten},
			{L"Read Time",       dtReadTime},
			{L"Write Time",      dtWriteTime},
			{L"Idle Time",       dtIdleTime},
			{L"Read Count",      dtReadCount},
			{L"Write Count",     dtWriteCount},
			{L"Queue Depth",     dtQueueDepth},
			{L"Split Count",     dtSplitCount},
			{L"Query Time",      dtQueryTime},
			{L"Device Manager",  dtStorageManagerName},
		};
		for(int iItem = 0; iItem < _countof(ItemDef); iItem++)
		{
			ListViewEx_InsertStringParam(hWndList,iItem,ItemDef[iItem].pszTitle,ItemDef[iItem].DataId);
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		if( pdi->item.mask & LVIF_TEXT )
		{
			if( pdi->item.lParam == 0 )
				return 0;
		}

		return 0;	
	}

	HRESULT FillItems(SELECT_ITEM *pSelItem)
	{
		HRESULT hr = S_OK;

		if( m_szVolumeDiskName[0] )
			UpdateListData(m_hWndList);

		return hr;
	}

	virtual HRESULT OnInitPage(PVOID ptr,DWORD,PVOID)
	{
		SELECT_ITEM *SelItem = (SELECT_ITEM *)ptr;
		
		HRESULT hr;
		WCHAR szTempBuffer[MAX_PATH];
		WCHAR szDeviceName[MAX_PATH];

		StringCchCopy(szTempBuffer,MAX_PATH,SelItem->pszPath);
		RemoveBackslash(szTempBuffer);

		WCHAR *pName = wcsrchr(szTempBuffer,L'\\');

		if( pName != NULL )
			pName++;
		else
			pName = szTempBuffer;

		StringCchPrintf(szDeviceName,MAX_PATH,L"\\??\\%s",pName);

		DISK_PERFORMANCE DiskPerf = {0};
		int cbDiskPerf = sizeof(DISK_PERFORMANCE);
		hr = QueryDiskPerformance(szDeviceName,&DiskPerf,cbDiskPerf);
		if( hr != S_OK )
		{
			ListView_DeleteAllItems(m_hWndList);
			SetErrorState(hr);
			return hr;
		}

		memcpy(&m_DiskPerf,&DiskPerf,sizeof(DISK_PERFORMANCE));
		StringCchCopy(m_szVolumeDiskName,MAX_PATH,szDeviceName);

		return S_OK;
	}

	void SetErrorState(ULONG Status)
	{
		if( Status == 0 )
		{
			_SafeMemFree(m_pszErrorMessage);
		}
		else
		{
			PWSTR pMessage;
			if( WinGetErrorMessage(Status,&pMessage) > 0 )
			{
				_SafeMemFree(m_pszErrorMessage);
				m_pszErrorMessage = _MemAllocString(pMessage);
				WinFreeErrorMessage(pMessage);
			}
		}
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
	}	

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				*State = ListView_GetSelectedCount(m_hWndList) ? UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_VIEW_REFRESH:
			case ID_STOP_DISKPERFORMANCE:
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*State = UPDUI_ENABLED;
				return S_OK;
		}
		return S_FALSE;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnEditCopy();
				break;
			case ID_VIEW_REFRESH:
				OnViewRefresh();
				break;
			case ID_STOP_DISKPERFORMANCE:
			{
				HRESULT hr = QueryDiskPerformance(this->m_szVolumeDiskName,NULL,0);
				MsgBox(m_hWnd,
					((hr == S_OK) ? L"Succeeded.":L"Faild."),L"Disk Performance",
					(MB_OK|((hr == S_OK)?MB_ICONINFORMATION:MB_ICONEXCLAMATION)));
				break;
			}
		}
		return S_OK;
	}

	void OnEditCopy()
	{
		if( GetKeyState(VK_SHIFT) < 0 )
		{
			SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,1);
		}
		else
		{
			SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
		}
	}

	void OnViewRefresh()
	{
		UpdateData(NULL);
	}

	VOID UpdateListData(HWND hwndLV)
	{
		HRESULT hr;
		DISK_PERFORMANCE DiskPerf = {0};
		int cbDiskPerf = sizeof(DISK_PERFORMANCE);
		hr = QueryDiskPerformance(m_szVolumeDiskName,&DiskPerf,cbDiskPerf);
		if( hr != S_OK )
		{
			ZeroMemory(&m_DiskPerf,sizeof(DISK_PERFORMANCE));
	#if 0
			DisableListData(hwndLV);
	#else
			ListView_DeleteAllItems(m_hWndList);
			SetErrorState(hr);
	#endif
			return;
		}
		else
		{
			SetErrorState(0);
		}

		if( ListView_GetItemCount(m_hWndList) == 0 )
		{
			InitTitles(m_hWndList);
		}

		WCHAR sz[MAX_PATH];

		int cItems = ListView_GetItemCount(hwndLV);

		for(int iItem = 0; iItem < cItems; iItem++)
		{
			int dt = (int)ListViewEx_GetItemData(hwndLV,iItem);
			switch( dt )
			{
				case dtBytesRead:
					_CommaFormatString(DiskPerf.BytesRead.QuadPart,sz);
					break;
				case dtBytesWritten:
					_CommaFormatString(DiskPerf.BytesWritten.QuadPart,sz);
					break;
				case dtReadTime:
					MakeFormatElapsedTime(sz,MAX_PATH,DiskPerf.ReadTime.QuadPart);
					break;
				case dtWriteTime:
					MakeFormatElapsedTime(sz,MAX_PATH,DiskPerf.WriteTime.QuadPart);
					break;
				case dtIdleTime:
					MakeFormatElapsedTime(sz,MAX_PATH,DiskPerf.IdleTime.QuadPart);
					break;
				case dtReadCount:
					_CommaFormatString(DiskPerf.ReadCount,sz);
					break;
				case dtWriteCount:
					_CommaFormatString(DiskPerf.WriteCount,sz);
					break;
				case dtQueueDepth:
					_CommaFormatString(DiskPerf.QueueDepth,sz);
					break;
				case dtSplitCount:
					_CommaFormatString(DiskPerf.SplitCount,sz);
					break;
				case dtQueryTime:
				{
					WCHAR szDate[32],szTime[32];
					WinGetDateString(DiskPerf.QueryTime.QuadPart,szDate,ARRAYSIZE(szDate),NULL,FALSE,0);
					WinGetTimeString(DiskPerf.QueryTime.QuadPart,szTime,ARRAYSIZE(szTime),NULL,FALSE,0);
					StringCchPrintf(sz,ARRAYSIZE(sz),L"%s %s",szDate,szTime);
					break;
				}
				case dtStorageManagerName:
				{
					StringCchPrintf(sz,ARRAYSIZE(sz),L"%s (%u)",DiskPerf.StorageManagerName,DiskPerf.StorageDeviceNumber);
					break;
				}
			}
	
			ListView_SetItemText(hwndLV,iItem,1,sz);
		}

		// save performance data
		memcpy(&m_DiskPerf,&DiskPerf,sizeof(DISK_PERFORMANCE));
	}

	void MakeFormatElapsedTime(PWSTR psz,int cch,LONGLONG val)
	{
		LONGLONG valMS;
		LONGLONG valS;
		LONGLONG tmH;
		LONGLONG tmM;
		LONGLONG tmS;
		GetParsedElapsedTime(val,&valMS,&valS,&tmH,&tmM,&tmS);
	#if 0
		WCHAR sz[100];
		StrFromTimeInterval(sz,ARRAYSIZE(sz),(DWORD)_getmsec(val),13);
		StringCchPrintf(psz,cch,L"%d:%02d:%02d.%d (%s)",tmH,tmM,tmS,(valMS % 1000),sz);
	#else
		StringCchPrintf(psz,cch,L"%d:%02d:%02d.%d",tmH,tmM,tmS,(valMS % 1000));
	#endif
	}

	VOID
	GetParsedElapsedTime(
		LONGLONG val, // unit 100ns
		LONGLONG *pvalMS,
		LONGLONG *pvalS,
		LONGLONG *ptmH,
		LONGLONG *ptmM,
		LONGLONG *ptmS
		)
	{
		LONGLONG valMS,valS,tmS,tmM,tmH;

		valMS = val / 10000;   // 100ns to msecond
		valS  = valMS / 1000;  // msecond to second
		tmS   = valS % 60;
		tmM   = (valS / 60) % 60;
		tmH   = (valS / 60) / 60;

		if( pvalMS ) *pvalMS = valMS;
		if( pvalS ) *pvalS = valS;
		if( ptmH ) *ptmH = tmH;
		if( ptmM ) *ptmM = tmM;
		if( ptmS ) *ptmS = tmS;
	}

	VOID DisableListData(HWND hwndLV)
	{
		int cItems = ListView_GetItemCount(hwndLV);
		for(int iItem = 0; iItem < cItems; iItem++)
		{
			ListView_SetItemText(hwndLV,iItem,1,L"-");
		}
	}
};
