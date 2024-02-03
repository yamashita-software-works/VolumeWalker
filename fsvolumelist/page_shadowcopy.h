#pragma once
//***************************************************************************
//*                                                                         *
//*  page_shadowcopy.h                                                      *
//*                                                                         *
//*  Volume Shadow Copy Page                                                *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-11-09                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "dparray.h"
#include "..\inc\common.h"
#include "column.h"
#include "..\fsvolumeinfo\storagedevice.h"
#include <devguid.h>
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>

struct CVolumeShadowCopyItem
{
public:
	GUID m_SnapshotId;
    GUID m_SnapshotSetId;
    LONG m_lSnapshotsCount;
    PWSTR m_pwszSnapshotDeviceObject;
    PWSTR m_pwszOriginalVolumeName;
    PWSTR m_pwszOriginatingMachine;
    PWSTR m_pwszServiceMachine;
    PWSTR m_pwszExposedName;
    PWSTR m_pwszExposedPath;
    GUID m_ProviderId;
    LONG m_lSnapshotAttributes;
	LONGLONG m_tsCreationTimestamp;
	VSS_SNAPSHOT_STATE m_eStatus;
	PWSTR m_pszSnapshotId;
	PWSTR m_pszSnapshotSetId;
	PWSTR m_pszNtPath;
	PWSTR m_pszOriginaVolumeGuid;

	CVolumeShadowCopyItem()
	{
		memset(this,0,sizeof(CVolumeShadowCopyItem));
	}
	~CVolumeShadowCopyItem()
	{
		_SafeMemFree(m_pwszSnapshotDeviceObject);
		_SafeMemFree(m_pwszOriginalVolumeName);
		_SafeMemFree(m_pwszOriginatingMachine);
		_SafeMemFree(m_pwszServiceMachine);
		_SafeMemFree(m_pwszExposedName);
		_SafeMemFree(m_pwszExposedPath);
		_SafeMemFree(m_pszSnapshotId);
		_SafeMemFree(m_pszSnapshotSetId);
		_SafeMemFree(m_pszNtPath);
		_SafeMemFree(m_pszOriginaVolumeGuid);
	}
};

class CVolumeShadowCopyListPage : public CPageWndBase
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CVolumeShadowCopyListPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CVolumeShadowCopyListPage,CVolumeShadowCopyItem> *m_comp_proc;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	HFONT m_hFont;
	PWSTR m_pszErrorMessage;

	CColumnList m_columns;

public:
	CVolumeShadowCopyListPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = 0;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_pszErrorMessage = NULL;
	}

	~CVolumeShadowCopyListPage()
	{
		_SafeMemFree(m_pszErrorMessage);

		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	virtual HRESULT OnInitPage(PVOID)
	{
		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

		if( !LoadColumns(m_hWndList) )
		{
			InitColumns(m_hWndList);
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		return E_NOTIMPL;
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
			case NM_CUSTOMDRAW:
				return OnCustomDraw(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case LVN_ITEMCHANGED:
				return OnItemChanged(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case LVN_ITEMACTIVATE:
				return OnItemActivate(pnmhdr);
			case LVN_COLUMNCLICK:
				return OnColumnClick(pnmhdr);
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlv->lParam;
		delete pItem;

		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem);

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

	void DoSort(int iSubItem=-1,int iDirection=-1)
	{
		HWND hwndLV = m_hWndList;
	
		int id = (int)ListViewEx_GetHeaderItemData(hwndLV,iSubItem);

		if( m_Sort.CurrentSubItem != -1 )
			ListViewEx_SetHeaderArrow(hwndLV,m_Sort.CurrentSubItem,0);

		if( iDirection != 0 )
		{
			if( m_Sort.CurrentSubItem != iSubItem )
				m_Sort.Direction = 1;
			else
				m_Sort.Direction *= iDirection;
		}

		SortItems(id,NULL);

		ListViewEx_SetHeaderArrow(hwndLV,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
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
			return CDRF_NEWFONT|CDRF_NOTIFYPOSTPAINT;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT )
		{
			UINT State = ListView_GetItemState(m_hWndList,(int)pnmlvcd->nmcd.dwItemSpec,LVIS_FOCUSED);
			if( State & LVIS_FOCUSED )
			{
				_DrawFocusFrame(m_hWndList,pnmlvcd->nmcd.hdc,&pnmlvcd->nmcd.rc);
			}
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnDisp_Name(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->m_pwszSnapshotDeviceObject;
		return 0;
	}

	LRESULT OnDisp_CreationTime(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlvdi->item.lParam;
		_GetDateTimeStringEx(pItem->m_tsCreationTimestamp,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,NULL,NULL,FALSE);
		return 0;
	}

	LRESULT OnDisp_OriginalDevice(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->m_pszNtPath;
		return 0;
	}

	LRESULT OnDisp_OriginalVolume(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlvdi->item.lParam;
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"\\??\\%s",pItem->m_pszOriginaVolumeGuid);
		return 0;
	}

	LRESULT OnDisp_SnapshotId(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->m_pszSnapshotId;
		return 0;
	}

	LRESULT OnDisp_SnapshotSetId(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->m_pszSnapshotSetId;
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CVolumeShadowCopyListPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_Name,           &CVolumeShadowCopyListPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_CreationTime,   &CVolumeShadowCopyListPage::OnDisp_CreationTime),
			COL_HANDLER_MAP_DEF(COLUMN_OriginalDevice, &CVolumeShadowCopyListPage::OnDisp_OriginalDevice),
			COL_HANDLER_MAP_DEF(COLUMN_OriginalVolume, &CVolumeShadowCopyListPage::OnDisp_OriginalVolume),
			COL_HANDLER_MAP_DEF(COLUMN_SnapshotId,     &CVolumeShadowCopyListPage::OnDisp_SnapshotId),
			COL_HANDLER_MAP_DEF(COLUMN_SnapshotSetId,  &CVolumeShadowCopyListPage::OnDisp_SnapshotSetId),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CVolumeShadowCopyListPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CVolumeShadowCopyListPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)pdi->item.lParam;

		int id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( m_disp_proc == NULL )
		{
			InitColumnTable();	
		}

		if( pdi->item.mask & LVIF_TEXT )
		{
			if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
			{
				pdi->item.mask |= LVIF_DI_SETITEM;
				return (this->*m_disp_proc[ id ].pfn)(id,pdi);
			}
		}

		return 0;	
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( ((pnmlv->uOldState == 0)|| ((pnmlv->uNewState & (LVIS_SELECTED)) == (LVIS_SELECTED))) 
			 && ((pnmlv->uNewState & (LVIS_SELECTED|LVIS_FOCUSED)) == (LVIS_SELECTED|LVIS_FOCUSED)) )
		{
			; // Reserved
		}

		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		CVolumeShadowCopyItem *pItem = (CVolumeShadowCopyItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SETFOCUS:
				SetFocus(m_hWndList);
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
		if( m_hWndList )
		{
			SetWindowPos(m_hWndList,NULL,0,0,cx,cy,SWP_NOMOVE|SWP_NOZORDER);
		}
	}

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		static COLUMN columns_filelist[] = {
			{ COLUMN_Name,           L"Snapshot Device",   0, 120, LVCFMT_LEFT },
			{ COLUMN_OriginalDevice, L"Original Device",   1, 120, LVCFMT_LEFT },
			{ COLUMN_CreationTime,   L"Creation Time",     2, 120, LVCFMT_LEFT },
			{ COLUMN_OriginalVolume, L"Original Volume",   3, 120, LVCFMT_LEFT },
			{ COLUMN_SnapshotId,     L"Snapshot Id",       4, 120, LVCFMT_LEFT },
			{ COLUMN_SnapshotSetId,  L"Snapshot Set Id",   5, 120, LVCFMT_LEFT },
		};

		m_columns.SetDefaultColumns(columns_filelist,ARRAYSIZE(columns_filelist));

		int i,c;
		c = m_columns.GetDefaultColumnCount();
		for(i = 0; i < c; i++)
		{
			const COLUMN *pcol = m_columns.GetDefaultColumnItem(i);
			lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
			lvc.fmt     = pcol->fmt;
			lvc.cx      = pcol->cx;
			lvc.pszText = pcol->Name;
			lvc.iOrder  = pcol->iOrder;
			int index = ListView_InsertColumn(hWndList,lvc.iOrder,&lvc);

			ListViewEx_SetHeaderItemData( hWndList, index, pcol->id );
		}
	}

	BOOL LoadColumns(HWND hWndList)
	{
		COLUMN_TABLE *pcoltbl;
		if( m_columns.LoadUserDefinitionColumnTable(&pcoltbl,L"ColumnLayout") == 0)
			return FALSE;

		LVCOLUMN lvc = {0};

		ULONG i;
		for(i = 0; i < pcoltbl->cItems; i++)
		{
			const COLUMN *pcol = &pcoltbl->column[i];
			lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
			lvc.fmt     = pcol->fmt;
			lvc.cx      = pcol->cx;
			lvc.pszText = pcol->Name;
			lvc.iOrder  = pcol->iOrder;
			int index = ListView_InsertColumn(hWndList,lvc.iOrder,&lvc);

			ListViewEx_SetHeaderItemData( hWndList, index, pcol->id );
		}

		m_columns.FreeUserDefinitionColumnTable(pcoltbl);

		return TRUE;
	}

	int Insert(int iItem,CVolumeShadowCopyItem *pItem)
	{
		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		iItem   = ListView_InsertItem(m_hWndList,&lvi);
		return iItem;
	}

	HRESULT EnumVolumeShadowCopyItems()
	{
		HRESULT hr;
		IVssBackupComponents *pvbc = NULL;
	
		hr = CreateVssBackupComponents(&pvbc);
	
		if( SUCCEEDED(hr) )	
		{
			pvbc->InitializeForBackup();
			pvbc->SetContext(VSS_CTX_ALL);
	
			IVssEnumObject *pIEnumSnapshots = NULL;
			hr = pvbc->Query(GUID_NULL,VSS_OBJECT_NONE,VSS_OBJECT_SNAPSHOT,&pIEnumSnapshots);
	
			if( hr == S_OK )
			{
				int iItem = 0;
	
				VSS_OBJECT_PROP Prop;
				VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
				WCHAR GuidString[64];
				WCHAR szNtPath[MAX_PATH];
				WCHAR buf[MAX_PATH];
	
				while(true)
				{
					ULONG ulFetched;
					hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
	
			        if (ulFetched == 0)
				        break;
	
					if( Prop.Type == VSS_OBJECT_SNAPSHOT )
					{
						CVolumeShadowCopyItem *pItem = new CVolumeShadowCopyItem;
	
						pItem->m_eStatus = Snap.m_eStatus;
						pItem->m_SnapshotId = Snap.m_SnapshotId;
						pItem->m_SnapshotSetId = Snap.m_SnapshotSetId;
						pItem->m_tsCreationTimestamp = Snap.m_tsCreationTimestamp;
						pItem->m_lSnapshotAttributes = Snap.m_lSnapshotAttributes;
						pItem->m_lSnapshotsCount = Snap.m_lSnapshotsCount;
	
						StringFromGUID2(Snap.m_SnapshotId,GuidString,_countof(GuidString));
						pItem->m_pszSnapshotId = _MemAllocString(GuidString);
	
						StringFromGUID2(Snap.m_SnapshotSetId,GuidString,_countof(GuidString));
						pItem->m_pszSnapshotSetId = _MemAllocString(GuidString);
	
						pItem->m_pwszOriginalVolumeName = _MemAllocString( Snap.m_pwszOriginalVolumeName );
						if( HasPrefix(L"\\\\?\\",pItem->m_pwszOriginalVolumeName) )
						{
							RemovePrefix(L"\\\\?\\",pItem->m_pwszOriginalVolumeName,buf,ARRAYSIZE(buf));
							RemoveBackslash(buf);
							if( QueryDosDevice(buf,szNtPath,MAX_PATH) > 0 )
								pItem->m_pszNtPath = _MemAllocString(szNtPath);
							else
								pItem->m_pszNtPath = _MemAllocString(buf);
							pItem->m_pszOriginaVolumeGuid = _MemAllocString(buf);
						}
	
						if( HasPrefix(L"\\\\?\\GLOBALROOT\\",Snap.m_pwszSnapshotDeviceObject) )
						{
							pItem->m_pwszSnapshotDeviceObject = _MemAllocString( &Snap.m_pwszSnapshotDeviceObject[14] );
						}
						else
						{
							pItem->m_pwszSnapshotDeviceObject = _MemAllocString( Snap.m_pwszSnapshotDeviceObject );
						}
	
						Insert(iItem++,pItem);
					}
	
					VssFreeSnapshotProperties(&Prop.Obj.Snap);
				}
	
				pIEnumSnapshots->Release();

				hr = S_OK;
			}
	
			pvbc->Release();
		}
		return hr;
	}

	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		HRESULT hr;
		CWaitCursor wait;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		hr = EnumVolumeShadowCopyItems();

		if( hr == S_OK )
		{
			DoSort(m_Sort.CurrentSubItem,0);

			ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE);
		}
		else
		{
			PWSTR pMessage;
			if( WinGetErrorMessage(hr,&pMessage) > 0 )
			{
				_SafeMemFree(m_pszErrorMessage);
				m_pszErrorMessage = _MemAllocString(pMessage);
				WinFreeErrorMessage(pMessage);
			}
		}

		SetRedraw(m_hWndList,TRUE);

		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Sort
	//

	int comp_name(CVolumeShadowCopyItem *pItem1,CVolumeShadowCopyItem *pItem2, const void *p)
	{
		return StrCmpLogicalW(pItem1->m_pwszSnapshotDeviceObject,pItem2->m_pwszSnapshotDeviceObject);
	}

	int comp_originalvolume(CVolumeShadowCopyItem *pItem1,CVolumeShadowCopyItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->m_pszOriginaVolumeGuid,pItem2->m_pszOriginaVolumeGuid);
	}

	int comp_originaldevice(CVolumeShadowCopyItem *pItem1,CVolumeShadowCopyItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->m_pszNtPath,pItem2->m_pszNtPath);
	}

	int comp_creationtime(CVolumeShadowCopyItem *pItem1,CVolumeShadowCopyItem *pItem2, const void *p)
	{
		return _COMP(pItem1->m_tsCreationTimestamp,pItem2->m_tsCreationTimestamp);
	}

	int comp_snapshotid(CVolumeShadowCopyItem *pItem1,CVolumeShadowCopyItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->m_pszSnapshotId,pItem2->m_pszSnapshotId);
	}

	int comp_snapshotsetid(CVolumeShadowCopyItem *pItem1,CVolumeShadowCopyItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->m_pszSnapshotSetId,pItem2->m_pszSnapshotSetId);
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CVolumeShadowCopyListPage,CVolumeShadowCopyItem> comp_proc[] = 
		{
			{COLUMN_Name,           &CVolumeShadowCopyListPage::comp_name},
			{COLUMN_OriginalVolume, &CVolumeShadowCopyListPage::comp_originalvolume},
			{COLUMN_OriginalDevice, &CVolumeShadowCopyListPage::comp_originaldevice},
			{COLUMN_SnapshotId,     &CVolumeShadowCopyListPage::comp_snapshotid},
			{COLUMN_SnapshotSetId,  &CVolumeShadowCopyListPage::comp_snapshotsetid},
			{COLUMN_CreationTime,   &CVolumeShadowCopyListPage::comp_creationtime},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CVolumeShadowCopyListPage,CVolumeShadowCopyItem>[COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CVolumeShadowCopyListPage,CVolumeShadowCopyItem>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc  = comp_proc[i].proc;
		}
	}

	int CompareItem(CVolumeShadowCopyItem *pItem1,CVolumeShadowCopyItem *pItem2,SORT_PARAM<CVolumeShadowCopyListPage> *op)
	{
		if( m_comp_proc == NULL )
		{
			init_compare_proc_def_table();
		}

		int iResult = 0;

		if( iResult == 0 && m_comp_proc[op->id].proc != NULL )
		{
			iResult = (this->*m_comp_proc[op->id].proc)(pItem1,pItem2,op);
		}

		iResult *= op->direction;

		return iResult;
	}

	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		CVolumeShadowCopyItem *pItem1 = (CVolumeShadowCopyItem *)lParam1;
		CVolumeShadowCopyItem *pItem2 = (CVolumeShadowCopyItem *)lParam2;
		SORT_PARAM<CVolumeShadowCopyListPage> *op = (SORT_PARAM<CVolumeShadowCopyListPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,CVolumeShadowCopyItem *)
	{
		SORT_PARAM<CVolumeShadowCopyListPage> op = {0};
		op.pThis = this;
		op.id = id;
		op.direction = m_Sort.Direction; // 1 or -1 do not use 0
		ListView_SortItems(m_hWndList,CompareProc,&op);
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				*State = ListView_GetSelectedCount(m_hWndList) ? UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_VIEW_REFRESH:
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
		if( GetKeyState(VK_SHIFT) < 0 )
		{
			SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,1);
		}
		else
		{
			SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
		}
	}

	void OnCmdRefresh()
	{
		SELECT_ITEM sel = {0};
		FillItems(&sel);
	}
};
