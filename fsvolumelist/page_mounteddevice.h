#pragma once
//***************************************************************************
//*                                                                         *
//*  page_mounteddevice.h                                                   *
//*                                                                         *
//*  Mounted Device Page                                                    *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-10-27                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "dparray.h"
#include "common.h"
#include "column.h"
#include "findhandler.h"
#include <devguid.h>

struct CMountedDeviceItem
{
public:
	PWSTR VolumeName;
	PWSTR VolumeIdentifier;

	CMountedDeviceItem()
	{
		VolumeName = NULL;
		VolumeIdentifier = NULL;
	}
	~CMountedDeviceItem()
	{
		_SafeMemFree(VolumeName);
		_SafeMemFree(VolumeIdentifier);
	}
};

//////////////////////////////////////////////////////////////////////////////

class CMountedDevicePage : 
	public CPageWndBase,
	public CFindHandler<CMountedDevicePage>
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CMountedDevicePage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CMountedDevicePage,CMountedDeviceItem> *m_comp_proc;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	HFONT m_hFont;

	CColumnList m_columns;

public:
	CMountedDevicePage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = 0;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_hFont = NULL;
	}

	~CMountedDevicePage()
	{
		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	HWND GetListView() const { return m_hWndList; }

	virtual HRESULT OnInitPage(PVOID)
	{
		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

		if( !LoadColumns(m_hWndList) )
		{
			InitColumns(m_hWndList);
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif

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
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
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


	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CMountedDeviceItem *pItem = (CMountedDeviceItem *)pnmlv->lParam;
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

		CMountedDeviceItem *pItem = (CMountedDeviceItem *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem);

		return 0;
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

	LRESULT OnDisp_Name(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CMountedDeviceItem *pItem = (CMountedDeviceItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->VolumeName;
		return 0;
	}

	LRESULT OnDisp_Identifier(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CMountedDeviceItem *pItem = (CMountedDeviceItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->VolumeIdentifier;
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CMountedDevicePage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_None,          NULL),
			COL_HANDLER_MAP_DEF(COLUMN_Name,          &CMountedDevicePage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_Identifier,    &CMountedDevicePage::OnDisp_Identifier),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CMountedDevicePage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CMountedDevicePage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CMountedDeviceItem *pItem = (CMountedDeviceItem *)pdi->item.lParam;

		int id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( m_disp_proc == NULL )
		{
			InitColumnTable();	
		}

		if( pdi->item.mask & LVIF_TEXT )
		{
			if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
			{
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

		CMountedDeviceItem *pItem = (CMountedDeviceItem *)ListViewEx_GetItemData(m_hWndList,iItem);

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
			case PM_FINDITEM:
				return CFindHandler<CMountedDevicePage>::OnFindItem(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	void UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
		{
			SetWindowPos(m_hWndList,NULL,0,0,cx,cy,SWP_NOZORDER);
		}
	}

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		static COLUMN columns_filelist[] = {
			{ COLUMN_Name,           L"Name",                  0, 240, LVCFMT_LEFT },
			{ COLUMN_Identifier,     L"Volume Identifier",     1, 280, LVCFMT_LEFT },
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

		return TRUE;
	}

	int _Insert(int iItem,const FS_VOL_MOUNTED_DEVICE *MountedDevice,int GroupId)
	{
		WCHAR sz[1024];
		WCHAR szNum[100];

		switch( MountedDevice->DataType )
		{
			case FS_VMDT_NT4INFO:
			{
				StringCchPrintf(sz,MAX_PATH, L"0x%08X 0x%016I64X (%s)", 
					MountedDevice->VolMgrLocationInfo.Signiture,MountedDevice->VolMgrLocationInfo.StartOffset.QuadPart,
					_CommaFormatString(MountedDevice->VolMgrLocationInfo.StartOffset.QuadPart,szNum));
				break;
			}
			case FS_VMDT_WCHAR:
			{
				StringCchPrintf(sz,MAX_PATH,L"%s",MountedDevice->VolMgrSymbolicLinkName);
				break;
			}
			case FS_VMDT_GPT_PARTITION:
			{
				if( StringFromGUID(&MountedDevice->PartitionGuid,sz,MAX_PATH) != 0 )
				{
					szNum[0] = L'-';
					szNum[1] = L'\0';
					StringCchPrintf(sz,MAX_PATH, L"%s",szNum);
				}
				break;
			}
			default:
				ASSERT(FALSE);
				return -1;
		}

		CMountedDeviceItem *pItem = new CMountedDeviceItem;

		pItem->VolumeName = _MemAllocString( MountedDevice->VolumeName );
		pItem->VolumeIdentifier = _MemAllocString( sz );

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		lvi.iGroupId = GroupId;
		iItem   = ListView_InsertItem(m_hWndList,&lvi);

		return iItem;
	}

	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		FS_VOL_MOUNTED_DEVICE_LIST *pMountedDevices = NULL;

		GetMountedDeviceList( &pMountedDevices );

		ULONG i;

		for(i = 0; i < pMountedDevices->Count; i++)
		{
			_Insert(i,&pMountedDevices->Device[i],ID_GROUP_DISK);
		}

		FreeMountedDeviceList( pMountedDevices );

		DoSort(m_Sort.CurrentSubItem,0);

		ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE);

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

	int comp_name(CMountedDeviceItem *pItem1,CMountedDeviceItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->VolumeName,pItem2->VolumeName);
	}

	int comp_identifier(CMountedDeviceItem *pItem1,CMountedDeviceItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->VolumeIdentifier,pItem2->VolumeIdentifier);
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CMountedDevicePage,CMountedDeviceItem> comp_proc[] = 
		{
			{0,NULL},
			{COLUMN_Name,           &CMountedDevicePage::comp_name},
			{COLUMN_Identifier,     &CMountedDevicePage::comp_identifier},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CMountedDevicePage,CMountedDeviceItem>[COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CMountedDevicePage,CMountedDeviceItem>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc  = comp_proc[i].proc;
		}
	}

	int CompareItem(CMountedDeviceItem *pItem1,CMountedDeviceItem *pItem2,SORT_PARAM<CMountedDevicePage> *op)
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
		CMountedDeviceItem *pItem1 = (CMountedDeviceItem *)lParam1;
		CMountedDeviceItem *pItem2 = (CMountedDeviceItem *)lParam2;
		SORT_PARAM<CMountedDevicePage> *op = (SORT_PARAM<CMountedDevicePage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,CMountedDeviceItem *)
	{
		SORT_PARAM<CMountedDevicePage> op;
		op.pThis = this;
		op.id = id;
		op.direction = m_Sort.Direction; // 1 or -1 do not use 0
		ListView_SortItems(m_hWndList,CompareProc,&op);
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

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				*State = ListView_GetSelectedCount(m_hWndList) ? UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_VIEW_REFRESH:
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*State = UPDUI_ENABLED;
				return S_OK;
		}
		return S_FALSE;
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
