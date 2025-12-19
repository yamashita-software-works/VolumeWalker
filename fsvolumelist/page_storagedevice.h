#pragma once
//***************************************************************************
//*                                                                         *
//*  page_storagedevice.h                                                   *
//*                                                                         *
//*  Storage Device Page                                                    *
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
#include "xmlwrite.h"

struct CStorageDeviceItem
{
public:
	PWSTR FriendlyName;
	PWSTR VolumeNameStrings;
	PWSTR InstanceId;
	FILETIME InstallDate;
	FILETIME FirstInstallDate;
	FILETIME LastArrivalDate;
	FILETIME LastRemovalDate;

	CStorageDeviceItem()
	{
		FriendlyName = NULL;
		VolumeNameStrings = NULL;
		InstanceId = NULL;
	}
	~CStorageDeviceItem()
	{
		_SafeMemFree(FriendlyName);
		_SafeMemFree(VolumeNameStrings);
		_SafeMemFree(InstanceId);
	}
};

//
// Group Id
//
enum {
	ID_GROUP_DISK=1,
	ID_GROUP_CDROM,
	ID_GROUP_FLOPPYDISK,
	ID_GROUP_VOLUME,
	ID_GROUP_VOLUMESNAPSHOT,
};

class CStorageDevicePage : 
	public CPageWndBase,
	public CFindHandler<CStorageDevicePage>
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CStorageDevicePage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CStorageDevicePage,CStorageDeviceItem> *m_comp_proc;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	HFONT m_hFont;
#if _ENABLE_GROUP_ICON
	SP_CLASSIMAGELIST_DATA m_scd;
#endif

	CColumnList m_columns;

public:
	HWND GetListView() const { return m_hWndList; }

public:
	CStorageDevicePage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = 0;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
	}

	~CStorageDevicePage()
	{
		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	virtual HRESULT OnInitPage(PVOID,DWORD,PVOID)
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

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

#if _ENABLE_GROUP_ICON
		m_scd.cbSize = sizeof(m_scd);
		SetupDiGetClassImageList(&m_scd);
		ListView_SetImageList(m_hWndList,m_scd.ImageList,LVSIL_GROUPHEADER);
#endif

		if( !LoadColumns(m_hWndList) )
		{
			InitColumns(m_hWndList);
		}

		InitGroup();
		ListView_EnableGroupView(m_hWndList,TRUE);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif

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
#if _ENABLE_GROUP_ICON
		SetupDiDestroyClassImageList(&m_scd);
#endif
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
			case LVN_KEYDOWN:
				return OnKeyDown(pnmhdr);
			case LVN_ITEMACTIVATE:
				return OnItemActivate(pnmhdr);
			case LVN_COLUMNCLICK:
				return OnColumnClick(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CStorageDeviceItem *pItem = (CStorageDeviceItem *)pnmlv->lParam;
		delete pItem;

		return 0;
	}

	LRESULT OnKeyDown(NMHDR *pnmhdr)
	{
		NMLVKEYDOWN *pnmkd = (NMLVKEYDOWN *)pnmhdr;

		if( pnmkd->wVKey == VK_SPACE || pnmkd->wVKey == VK_RETURN)
		{
			int iGroup = (int)ListView_GetFocusedGroup(pnmkd->hdr.hwndFrom);
			if( iGroup != -1 )
			{
				LVGROUP lvg = {0};
				lvg.cbSize = sizeof(lvg);
				lvg.mask = LVGF_GROUPID|LVGF_STATE;
				ListView_GetGroupInfoByIndex(pnmkd->hdr.hwndFrom,iGroup,&lvg);

				lvg.state = ListView_GetGroupState(pnmkd->hdr.hwndFrom,lvg.iGroupId,LVGS_COLLAPSED|LVGS_COLLAPSIBLE);

				lvg.mask = LVGF_STATE;
				lvg.state ^= LVGS_COLLAPSED;
				lvg.stateMask = LVGS_COLLAPSED;
				ListView_SetGroupInfo(pnmkd->hdr.hwndFrom,lvg.iGroupId,&lvg);
			}
		}
		
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

		CStorageDeviceItem *pItem = (CStorageDeviceItem *)pnmlv->lParam;

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

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		if( IsXpThemeEnabled() )
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

		if( IsXpThemeEnabled() )
		{
			int iGroup = (int)ListView_GetFocusedGroup(m_hWndList);
			if( iGroup == -1 )
				SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
			else
				SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);
		}

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
			if( IsXpThemeEnabled() )
			{
				UINT State = ListView_GetItemState(m_hWndList,(int)pnmlvcd->nmcd.dwItemSpec,LVIS_FOCUSED);
				if( State & LVIS_FOCUSED )
				{
					_DrawFocusFrame(m_hWndList,pnmlvcd->nmcd.hdc,&pnmlvcd->nmcd.rc);
				}
			}
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnDisp_Name(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CStorageDeviceItem *pItem = (CStorageDeviceItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->FriendlyName;
		return 0;
	}

	LRESULT OnDisp_DateTime(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CStorageDeviceItem *pItem = (CStorageDeviceItem *)pnmlvdi->item.lParam;

		LARGE_INTEGER liDate = {0};

		switch( id )
		{
			case COLUMN_InstallDate:
				liDate.HighPart = pItem->InstallDate.dwHighDateTime;
				liDate.LowPart  = pItem->InstallDate.dwLowDateTime;
				break;
			case COLUMN_FirstInstallDate:
				liDate.HighPart = pItem->FirstInstallDate.dwHighDateTime;
				liDate.LowPart  = pItem->FirstInstallDate.dwLowDateTime;
				break;
			case COLUMN_LastArrivalDate:
				liDate.HighPart = pItem->LastArrivalDate.dwHighDateTime;
				liDate.LowPart  = pItem->LastArrivalDate.dwLowDateTime;
				break;
			case COLUMN_LastRemovalDate:
				liDate.HighPart = pItem->LastRemovalDate.dwHighDateTime;
				liDate.LowPart  = pItem->LastRemovalDate.dwLowDateTime;
				break;
		}

		if( liDate.HighPart != 0 && liDate.LowPart != 0 )
			_GetDateTimeStringEx2(liDate.QuadPart,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,NULL,NULL,0,1);
		else
			pnmlvdi->item.pszText = L"-";
		return 0;
	}

	LRESULT OnDisp_DeviceId(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CStorageDeviceItem *pItem = (CStorageDeviceItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->InstanceId;
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CStorageDevicePage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_Name,            &CStorageDevicePage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_InstallDate,     &CStorageDevicePage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_FirstInstallDate,&CStorageDevicePage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_LastArrivalDate, &CStorageDevicePage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_LastRemovalDate, &CStorageDevicePage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_DeviceId,        &CStorageDevicePage::OnDisp_DeviceId),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CStorageDevicePage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CStorageDevicePage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CStorageDeviceItem *pItem = (CStorageDeviceItem *)pdi->item.lParam;

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

		CStorageDeviceItem *pItem = (CStorageDeviceItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		AppendMenu(hMenu,MF_STRING,0,0);
		AppendMenu(hMenu,MF_STRING,ID_FILE_EXPORT,L"&Export All Items");

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
				return CFindHandler<CStorageDevicePage>::OnFindItem(hWnd,uMsg,wParam,lParam);
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

	int InsertGroup(HWND hWndList,int iGroupId,LPCWSTR pszHeaderText,int iImage=I_IMAGENONE,BOOL fCollapsed=FALSE,LPCWSTR pszSubTitle=NULL)
	{
		LVGROUP group = {0};

		group.cbSize      = sizeof(LVGROUP);
		group.mask        = LVGF_GROUPID|LVGF_TITLEIMAGE|LVGF_HEADER|LVGF_STATE;
		group.iTitleImage = iImage;
		group.pszHeader   = (LPWSTR)pszHeaderText;
		group.uAlign      = LVGA_HEADER_LEFT;
		group.iGroupId    = iGroupId;
		group.state       = LVGS_COLLAPSIBLE | (fCollapsed ? LVGS_COLLAPSED : 0);

		if( pszSubTitle )
		{
			group.mask |= LVGF_SUBTITLE;
			group.pszSubtitle = (LPWSTR)pszSubTitle;
		}

		return (int)ListView_InsertGroup(hWndList,-1,(PLVGROUP)&group);
	}

	typedef struct _GROUP_ITEM
	{
		int idGroup;
		const GUID *Guid;
		PCWSTR Text;
	} GROUP_ITEM;

	void InitGroup()
	{
		GROUP_ITEM Group[] = {
			{ ID_GROUP_DISK,      &GUID_DEVCLASS_DISKDRIVE,  L"Disk Drive" },
			{ ID_GROUP_CDROM,     &GUID_DEVCLASS_CDROM,      L"CD-ROM"  },
			{ ID_GROUP_FLOPPYDISK,&GUID_DEVCLASS_FLOPPYDISK, L"Floppy Disk" },
			{ ID_GROUP_VOLUME,    &GUID_DEVCLASS_VOLUME,     L"Volume" },
			{ ID_GROUP_VOLUMESNAPSHOT,&GUID_DEVCLASS_VOLUMESNAPSHOT, L"Volume Snapshot" },
		};
		int cGroupItem = ARRAYSIZE(Group);

		for(int i = 0; i < cGroupItem; i++)
		{
			int iImage = I_IMAGENONE;
#if _ENABLE_GROUP_ICON
			SetupDiGetClassImageIndex(&m_scd,Group[i].Guid,&iImage);
#endif
			InsertGroup(m_hWndList,Group[i].idGroup,Group[i].Text,iImage);
		}
	}

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		static COLUMN columns_filelist[] = {
			{ COLUMN_Name,             L"Name",                  0, 280, LVCFMT_LEFT },
			{ COLUMN_InstallDate,      L"Install Date",          0, 100, LVCFMT_LEFT },
#if 0
			{ COLUMN_FirstInstallDate, L"First Install Date",    0, 100, LVCFMT_LEFT },
#endif
			{ COLUMN_LastArrivalDate,  L"Arrival Date",          0, 100, LVCFMT_LEFT },
			{ COLUMN_LastRemovalDate,  L"Removal Date",          0, 100, LVCFMT_LEFT },
			{ COLUMN_DeviceId,         L"Device Instance Id",    0, 100, LVCFMT_LEFT },
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
			lvc.iOrder  = i;
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

	int Insert(int iItem,const FS_HARDWARE_PRODUCT *pProductInfo,int GroupId)
	{
		CStorageDeviceItem *pItem = new CStorageDeviceItem;

		if( pProductInfo->FriendlyName )
			pItem->FriendlyName = _MemAllocString( pProductInfo->FriendlyName );
		else
			if( pProductInfo->PysicalDeviceObjectName )
				pItem->FriendlyName = _MemAllocString( pProductInfo->PysicalDeviceObjectName );
			else if( pProductInfo->DeviceDesc )
				pItem->FriendlyName = _MemAllocString( pProductInfo->DeviceDesc );
			else if( pProductInfo->HardwareIds )
				pItem->FriendlyName = _MemAllocString( pProductInfo->HardwareIds );
			else
				pItem->FriendlyName = _MemAllocString( L"" );

		pItem->InstanceId = _MemAllocString( pProductInfo->InstanceId );
		pItem->InstallDate = pProductInfo->InstallDate;
		pItem->FirstInstallDate = pProductInfo->FirstInstallDate;
		pItem->LastArrivalDate = pProductInfo->LastArrivalDate;
		pItem->LastRemovalDate = pProductInfo->LastRemovalDate;

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_GROUPID;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		lvi.iGroupId = GroupId;
		iItem   = ListView_InsertItem(m_hWndList,&lvi);

		return iItem;
	}

	void EnumItems(HANDLE Handle,int GroupId)
	{
		ULONG i,c = GetKnownHardwareProductsCount(Handle);

		const FS_HARDWARE_PRODUCT *pProductInfo;
		for(i = 0; i < c; i++)
		{
			if( GetKnownHardwareProductPointer(Handle,i,&pProductInfo) )
			{
				Insert(i,pProductInfo,GroupId);
			}
		}
	}

	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		HANDLE hHardwareProduct;
		if( GetKnownHardwareProducts(&hHardwareProduct,&GUID_DEVCLASS_DISKDRIVE,0) )
		{
			EnumItems(hHardwareProduct,ID_GROUP_DISK);

			FreeKnownHardwareProducts(hHardwareProduct);
		}

		if( GetKnownHardwareProducts(&hHardwareProduct,&GUID_DEVCLASS_CDROM,0) )
		{
			EnumItems(hHardwareProduct,ID_GROUP_CDROM);

			FreeKnownHardwareProducts(hHardwareProduct);
		}

		if( GetKnownHardwareProducts(&hHardwareProduct,&GUID_DEVCLASS_VOLUME,0) )
		{
			EnumItems(hHardwareProduct,ID_GROUP_VOLUME);

			FreeKnownHardwareProducts(hHardwareProduct);
		}

		if( GetKnownHardwareProducts(&hHardwareProduct,&GUID_DEVCLASS_VOLUMESNAPSHOT,0) )
		{
			EnumItems(hHardwareProduct,ID_GROUP_VOLUMESNAPSHOT);

			FreeKnownHardwareProducts(hHardwareProduct);
		}

		if( GetKnownHardwareProducts(&hHardwareProduct,&GUID_DEVCLASS_FLOPPYDISK,0) )
		{
			EnumItems(hHardwareProduct,ID_GROUP_FLOPPYDISK);

			FreeKnownHardwareProducts(hHardwareProduct);
		}

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

	int comp_name(CStorageDeviceItem *pItem1,CStorageDeviceItem *pItem2, const void *p)
	{
		return StrCmpLogicalW(pItem1->FriendlyName,pItem2->FriendlyName);
	}

	int comp_deviceid(CStorageDeviceItem *pItem1,CStorageDeviceItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->InstanceId,pItem2->InstanceId);
	}

	int comp_installdate(CStorageDeviceItem *pItem1,CStorageDeviceItem *pItem2, const void *p)
	{
		return CompareFileTime(&pItem1->InstallDate,&pItem2->InstallDate);
	}

	int comp_firstinstalldate(CStorageDeviceItem *pItem1,CStorageDeviceItem *pItem2, const void *p)
	{
		return CompareFileTime(&pItem1->FirstInstallDate,&pItem2->FirstInstallDate);
	}

	int comp_lastarrivaldate(CStorageDeviceItem *pItem1,CStorageDeviceItem *pItem2, const void *p)
	{
		return CompareFileTime(&pItem1->LastArrivalDate,&pItem2->LastArrivalDate);
	}

	int comp_lastremovaldate(CStorageDeviceItem *pItem1,CStorageDeviceItem *pItem2, const void *p)
	{
		return CompareFileTime(&pItem1->LastRemovalDate,&pItem2->LastRemovalDate);
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CStorageDevicePage,CStorageDeviceItem> comp_proc[] = 
		{
			{0,NULL},
			{COLUMN_Name,             &CStorageDevicePage::comp_name},
			{COLUMN_DeviceId,         &CStorageDevicePage::comp_deviceid},
			{COLUMN_InstallDate,      &CStorageDevicePage::comp_installdate},
			{COLUMN_FirstInstallDate, &CStorageDevicePage::comp_firstinstalldate},
			{COLUMN_LastArrivalDate,  &CStorageDevicePage::comp_lastarrivaldate},
			{COLUMN_LastRemovalDate,  &CStorageDevicePage::comp_lastremovaldate},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CStorageDevicePage,CStorageDeviceItem>[COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CStorageDevicePage,CStorageDeviceItem>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc  = comp_proc[i].proc;
		}
	}

	int CompareItem(CStorageDeviceItem *pItem1,CStorageDeviceItem *pItem2,SORT_PARAM<CStorageDevicePage> *op)
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
		CStorageDeviceItem *pItem1 = (CStorageDeviceItem *)lParam1;
		CStorageDeviceItem *pItem2 = (CStorageDeviceItem *)lParam2;
		SORT_PARAM<CStorageDevicePage> *op = (SORT_PARAM<CStorageDevicePage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,CStorageDeviceItem *)
	{
		SORT_PARAM<CStorageDevicePage> op = {0};
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
			case ID_FILE_EXPORT:
				*State = ListView_GetItemCount(m_hWndList) ? UPDUI_ENABLED : UPDUI_DISABLED;
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
			case ID_FILE_EXPORT:
				OnCmdExport();
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

	HRESULT GetSaveFilename(HWND hwnd,PWSTR *ppwszFilePath)
	{
		IFileDialog *pfd;
    
		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, 
								  NULL, 
								  CLSCTX_INPROC_SERVER, 
								  __uuidof(IFileDialog),(void**)&pfd);

	    if (SUCCEEDED(hr))
		{
			DWORD dwOptions;
			pfd->GetOptions(&dwOptions);
			dwOptions |= FOS_OVERWRITEPROMPT;
			pfd->SetOptions(dwOptions|FOS_FORCEFILESYSTEM);

			{
				COMDLG_FILTERSPEC *fileTypes = new COMDLG_FILTERSPEC[ 2 ];

				fileTypes[0].pszName = L"XML File";
				fileTypes[0].pszSpec = L"*.xml";

				fileTypes[1].pszName = L"All Files";
				fileTypes[1].pszSpec = L"*.*";

				pfd->SetFileTypes(2,fileTypes);

				pfd->SetFileTypeIndex(0);

				delete[] fileTypes;
			} 

			WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
			DWORD cch = ARRAYSIZE(szComputerName);
			GetComputerName(szComputerName,&cch);
			SYSTEMTIME st;
			GetLocalTime(&st);
			WCHAR szFileName[MAX_PATH];
			StringCchPrintf(szFileName,MAX_PATH,L"StorageDevices-%s-%04d%02d%02d%02d%02d%02d",szComputerName,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
			pfd->SetTitle( L"Export" );
			pfd->SetFileName( szFileName );
			pfd->SetDefaultExtension( L".xml" );

			// Show the dialog
			hr = pfd->Show(hwnd);
        
			if (SUCCEEDED(hr))
			{
				// Obtain the result of the user's interaction with the dialog.
	            IShellItem *psiResult;
		        hr = pfd->GetResult(&psiResult);
            
			    if (SUCCEEDED(hr))
				{
					LPWSTR name;
					hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH,&name);

					if( SUCCEEDED(hr) )
					{
						*ppwszFilePath = name;
					}

					psiResult->Release();
				}

			}

			pfd->Release();
		}

		return hr;
	}

	HRESULT WriteDateElemnt( IWriteFile *pWriter, PCWSTR ElementName, UINT DateTypeId, CStorageDeviceItem *pItem )
	{
		HRESULT hr;

		WCHAR sz[260];

		hr = pWriter->StartElement( ElementName );

		NMLVDISPINFO nmlvdi = {0};
		LARGE_INTEGER li = {0};
		switch( DateTypeId )
		{
			case COLUMN_InstallDate:
				li.HighPart = pItem->InstallDate.dwHighDateTime;
				li.LowPart  = pItem->InstallDate.dwLowDateTime;
				break;
			case COLUMN_FirstInstallDate:
				li.HighPart = pItem->FirstInstallDate.dwHighDateTime;
				li.LowPart  = pItem->FirstInstallDate.dwLowDateTime;
				break;
			case COLUMN_LastArrivalDate:
				li.HighPart = pItem->LastArrivalDate.dwHighDateTime;
				li.LowPart  = pItem->LastArrivalDate.dwLowDateTime;
				break;
			case COLUMN_LastRemovalDate:
				li.HighPart = pItem->LastRemovalDate.dwHighDateTime;
				li.LowPart  = pItem->LastRemovalDate.dwLowDateTime;
				break;
		}

		StringCchPrintf(sz,ARRAYSIZE(sz),L"0x%I64x",li.QuadPart);
		hr = pWriter->AttributeString( L"Hex", sz );

		nmlvdi.item.lParam = (LPARAM)pItem;
		nmlvdi.item.pszText = sz;
		nmlvdi.item.cchTextMax = ARRAYSIZE(sz);
		OnDisp_DateTime(DateTypeId,&nmlvdi);

		hr = pWriter->WriteString( sz );
		hr = pWriter->EndElement();

		return hr;
	}

	HRESULT WriteSanpshotInfo( IWriteFile *pWriter )
	{
		HRESULT hr;
		DWORD cchBuffer;
		WCHAR szBuffer[MAX_PATH];

		cchBuffer = MAX_PATH;
		GetComputerName(szBuffer,&cchBuffer);

		hr = pWriter->StartElement( L"ComputerName" );
		if( hr == S_OK )
		{
			hr = pWriter->WriteString( szBuffer );
			pWriter->EndElement();
		}
		if( hr != S_OK )
			return hr;

		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		_GetDateTimeStringFromFileTime(&ft,szBuffer,MAX_PATH);

		hr = pWriter->StartElement( L"SnapshotTime" );
		if( hr == S_OK )
		{
			hr = pWriter->WriteString( szBuffer );
			pWriter->EndElement();
		}

		return hr;
	}

	void OnCmdExport()
	{
		HRESULT hr;

		PWSTR pszFilePath = NULL;
		hr = GetSaveFilename( GetActiveWindow(), &pszFilePath );
		if( hr != S_OK )
			return ;

		IWriteFile *pWriter = NULL;

		__try
		{
			hr = CreateWriter( &pWriter );
			if( FAILED(hr) )
				__leave;

			hr = pWriter->Create( pszFilePath );
			if( FAILED(hr) )
				__leave;

			hr = pWriter->StartElement( L"VolumeWalker" );
			if( FAILED(hr) )
				__leave;

			hr = WriteSanpshotInfo( pWriter );
			if( FAILED(hr) )
				__leave;

			hr = pWriter->StartElement( L"StorageDevices" );
			if( FAILED(hr) )
				__leave;

			int i,cGroups = (int)ListView_GetGroupCount(m_hWndList);

			WCHAR sz[MAX_PATH];

			LVGROUP lvg = {0};
			lvg.cbSize    = sizeof(lvg);
			lvg.mask      = LVGF_HEADER|LVGF_GROUPID|LVGF_ITEMS;
			lvg.pszHeader = sz;
			lvg.cchHeader = ARRAYSIZE(sz);

			for(i = 0; i < cGroups; i++)
			{
				ListView_GetGroupInfoByIndex(m_hWndList,i,&lvg);

				LVITEMINDEX lvii;
				lvii.iGroup = i; // index
				lvii.iItem  = lvg.iFirstItem;

				pWriter->StartElement( L"DeviceClass" );
				pWriter->AttributeString( L"Name", lvg.pszHeader );

				if( lvg.cItems > 0 )
				{
					do
					{
						lvii.iGroup = i; // Group index
						CStorageDeviceItem *pItem = (CStorageDeviceItem *)ListViewEx_GetItemData(m_hWndList,lvii.iItem);

						pWriter->StartElement( L"Device" );

						{
							pWriter->StartElement( L"FriendlyName" );
							pWriter->WriteString( pItem->FriendlyName );
							pWriter->EndElement();

							WriteDateElemnt( pWriter, L"InstallDate",      COLUMN_InstallDate, pItem );
							WriteDateElemnt( pWriter, L"FirstInstallDate", COLUMN_FirstInstallDate, pItem );
							WriteDateElemnt( pWriter, L"LastArrivalDate",  COLUMN_LastArrivalDate, pItem );
							WriteDateElemnt( pWriter, L"LastRemovalDate",  COLUMN_LastRemovalDate, pItem );

							pWriter->StartElement( L"InstanceId" );
							pWriter->WriteString( pItem->InstanceId );
							pWriter->EndElement();
						}

						pWriter->EndElement();
					}
					while( ListView_GetNextItemIndex(m_hWndList,&lvii,LVNI_VISIBLEORDER|LVNI_SAMEGROUPONLY) );
				}
				else
				{
					; // no items
				}

				pWriter->EndElement();
			}
		}
		__finally
		{
			hr = pWriter->EndElement();

			if( pszFilePath )
				CoTaskMemFree(pszFilePath);

			pWriter->Commit();
			pWriter->Close();
	
			pWriter->Release();
		}
	}
};
