#pragma once
//***************************************************************************
//*                                                                         *
//*  page_physicaldrivelist.h                                               *
//*                                                                         *
//*  Physical Drive List Page                                               *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-11-02                                                     *
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

struct CPhysicalDriveItem
{
public:
	PWSTR DriveName;
	DWORD DriveNumber;
	PWSTR DriveDevice;
	PWSTR PartitionStyleString;
	PWSTR BufTypeString;
	struct {
		PCHAR VendorId;
		PCHAR ProductId;
		PCHAR SerialNumber;
		PCHAR ProductRevision;
	} DeviceDescriptor;

	CPhysicalDriveInformation *DriveInfo;

	CPhysicalDriveItem()
	{
		DriveName = NULL;
		DriveInfo = NULL;
		DriveDevice = NULL;
		DriveNumber = (DWORD)-1;
		PartitionStyleString = NULL;
		BufTypeString = NULL;
		DeviceDescriptor.VendorId = NULL;
		DeviceDescriptor.ProductId = NULL;
		DeviceDescriptor.SerialNumber = NULL;
		DeviceDescriptor.ProductRevision = NULL;
	}

	~CPhysicalDriveItem()
	{
		_SafeMemFree(DriveName);
		_SafeMemFree(DriveDevice);
		_SafeMemFree(PartitionStyleString);
		_SafeMemFree(BufTypeString);
		if( DriveInfo )
		{
			delete DriveInfo;
			DriveInfo = NULL;
		}
	}
};

class CPhysicalDriveListPage :
	public CPageWndBase,
	public CFindHandler<CPhysicalDriveListPage>
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CPhysicalDriveListPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CPhysicalDriveListPage,CPhysicalDriveItem> *m_comp_proc;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	HFONT m_hFont;

	CColumnList m_columns;

public:
	CPhysicalDriveListPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = 0;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_hFont = NULL;
	}

	~CPhysicalDriveListPage()
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
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

		ListViewEx_SetTrickColumnZero(m_hWndList,TRUE);

		if( !LoadColumns(m_hWndList) )
		{
			InitColumns(m_hWndList);
		}

		ListViewEx_SetTrickColumnZero(m_hWndList,FALSE);

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

	LRESULT OnTimer(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		KillTimer(m_hWnd,TE_OPEN_MDI_CHILD_FRAME);

		OpenInformationView(
				ListViewEx_GetCurSel(m_hWndList),
				VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION);

		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_PHYSICALDRIVEINFORMATION,L"Open &Information");
		AppendMenu(hMenu,MF_STRING,ID_DRIVELAYOUT,L"Open Drive &Layout");
		AppendMenu(hMenu,MF_STRING,0,0);
		AppendMenu(hMenu,MF_STRING,ID_HEXDUMP,L"Sector &Dump");
		AppendMenu(hMenu,MF_STRING,0,0);
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		SetMenuDefaultItem(hMenu,ID_PHYSICALDRIVEINFORMATION,FALSE);

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
				return OnNmCustomDraw(pnmhdr);
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
			case HDN_ENDDRAG:
				return OnHeaderEndDrag(pnmhdr);
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

	LRESULT OnNmCustomDraw(NMHDR *pnmhdr)
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
			CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlvcd->nmcd.lItemlParam;

			if( pItem->DriveInfo && pItem->DriveInfo->pDeviceDescriptor )
			{
				if( pItem->DriveInfo->pDeviceDescriptor->BusType == BusTypeVirtual ||
				    pItem->DriveInfo->pDeviceDescriptor->BusType == BusTypeFileBackedVirtual )
				{
					pnmlvcd->clrText = _COLOR_TEXT_VIRTUALDISK;
				}
			} 
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

	LRESULT OnHeaderEndDrag(NMHDR *pnmhdr)
	{
		int cColumns;
		int i;

		int ColumnId;
		ColumnId = (int)ListViewEx_GetHeaderItemData( m_hWndList, m_Sort.CurrentSubItem  );

		HWND hWndHeader = ListView_GetHeader(m_hWndList);

		NMHEADER *pI = (NMHEADER *)pnmhdr;

		// Forwarding message from ListView's header control.
		LVCOLUMN col = {0};
		col.mask = LVCF_ORDER;
		col.iOrder = pI->pitem->iOrder;
		ListView_SetColumn(m_hWndList,pI->iItem,&col);

		cColumns = Header_GetItemCount( hWndHeader );

		LVCOLUMN *aOrder = new LVCOLUMN[cColumns];
		int *aiOrder = new int[cColumns];
		LPARAM *aCol = new LPARAM[cColumns];
		for(i = 0; i < cColumns; i++)
		{
			ZeroMemory(&aOrder[i],sizeof(LVCOLUMN));
			aOrder[i].pszText = _MemAllocStringBuffer( MAX_PATH );
			aOrder[i].cchTextMax = MAX_PATH;
		}

		for(i = 0; i < cColumns; i++)
		{
			aOrder[i].mask = LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_ORDER|LVCF_SUBITEM;
			ListView_GetColumn(m_hWndList,i,&aOrder[i]);
			aiOrder[i] = aOrder[i].iOrder;

			HDITEM hi;
			hi.mask = HDI_LPARAM;
			Header_GetItem( hWndHeader,i,&hi);
			aCol[i] = hi.lParam;
		}

		LVCOLUMN *pTemp = new LVCOLUMN[cColumns];

		for(i = 0; i < cColumns; i++)
		{
			pTemp[ aiOrder[i] ] = aOrder[i];
			pTemp[ aiOrder[i] ].iSubItem = (int)aCol[i];
		}

		SetRedraw(m_hWndList,FALSE);

		for(i = 0; i < cColumns; i++)
		{
			pTemp[i].iOrder = i;
			pTemp[i].mask = LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_ORDER|LVCF_SUBITEM;
			ListView_SetColumn(m_hWndList,i,&pTemp[i]);

			HDITEM hi;
			hi.mask = HDI_LPARAM;
			hi.lParam = pTemp[i].iSubItem;
			Header_SetItem(hWndHeader,i,&hi);
		}

		SetRedraw(m_hWndList,TRUE);

		RedrawWindow(m_hWndList,NULL,NULL,RDW_UPDATENOW|RDW_INVALIDATE);

		for(i = 0; i < cColumns; i++)
		{
			_SafeMemFree( aOrder[i].pszText );
		}

		delete[] aOrder;
		delete[] aiOrder;
		delete[] aCol;
		delete[] pTemp;

		int iSubItem = FindSubItemById( ColumnId );
		m_Sort.CurrentSubItem = iSubItem;

		// To allow the control to automatically place and reorder the item, return FALSE.
		// To prevent the item from being placed, return TRUE.
		return TRUE;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlv->lParam;
		delete pItem;

		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
#if 0
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)ListViewEx_GetItemData(m_hWndList,pnmia->iItem);

		SendMessage(GetActiveWindow(),WM_OPEM_MDI_CHILDFRAME,VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION,(LPARAM)pItem->DriveName);
#else
		// Reason of using SetTimer:
		// (Only case of mouse click open)
		//
		// 1. The LVN_ITEMACTIVATE in occurs after WM_LBUTTONDOWN->WM_LBUTTONDBLCLK
		//    before WM_LBUTTONUP.
		// 2. During LVN_ITEMACTIVATE processing, create new MDI child frame 
		//    and to activate.
		// 3. When LVN_ITEMACTIVATE returns, at that time the active window has changed.
		// 4. It then receives WM_LBUTTONUP. But this message sending to 
		//    the newly created window instead of the previous window.
		// 5. At this moment may case result in unexpected selection operations 
		//    on the new window (e.g. Group header click select).

		SetTimer(m_hWnd,TE_OPEN_MDI_CHILD_FRAME,DELAY_OPEN_TIMER,NULL);
#endif
		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem);

		return 0;
	}

	void DoSort(int iSubItem=-1,int iDirection=-1)
	{
		int id = (int)ListViewEx_GetHeaderItemData(m_hWndList,iSubItem);

		if( m_Sort.CurrentSubItem != -1 )
			ListViewEx_SetHeaderArrow(m_hWndList,m_Sort.CurrentSubItem,0);

		if( iDirection != 0 )
		{
			if( m_Sort.CurrentSubItem != iSubItem )
				m_Sort.Direction = 1;
			else
				m_Sort.Direction *= iDirection;
		}

		SortItems(id,NULL);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	LRESULT OnDisp_Name(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->DriveName;
		return 0;
	}

	LRESULT OnDisp_Size(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlvdi->item.lParam;
		if( pItem->DriveInfo && pItem->DriveInfo->pGeometry )
		{
			StrFormatByteSizeEx(pItem->DriveInfo->pGeometry->DiskSize.QuadPart,
							SFBS_FLAGS_ROUND_TO_NEAREST_DISPLAYED_DIGIT,
							pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
		}
		return 0;
	}

	LRESULT OnDisp_VendorId(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlvdi->item.lParam;
		if( pItem->DriveInfo && pItem->DriveInfo->pDeviceDescriptor )
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%S",
				pItem->DeviceDescriptor.VendorId);
		}
		return 0;
	}

	LRESULT OnDisp_ProductId(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlvdi->item.lParam;
		if( pItem->DriveInfo && pItem->DriveInfo->pDeviceDescriptor )
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%S",
				pItem->DeviceDescriptor.ProductId);
		}
		return 0;
	}

	LRESULT OnDisp_BusType(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlvdi->item.lParam;
		if( pItem->DriveInfo && pItem->DriveInfo->pDeviceDescriptor )
		{
			StringCchCopy(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,
				pItem->BufTypeString);
		}
		return 0;
	}

	LRESULT OnDisp_PartitionStyle(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pnmlvdi->item.lParam;
		if( pItem->DriveInfo && pItem->DriveInfo->pDriveLayout )
		{
			StringCchCopy(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,
				pItem->PartitionStyleString);
		}
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CPhysicalDriveListPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_Name,           &CPhysicalDriveListPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_Size,           &CPhysicalDriveListPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_VendorId,       &CPhysicalDriveListPage::OnDisp_VendorId),
			COL_HANDLER_MAP_DEF(COLUMN_ProductId,      &CPhysicalDriveListPage::OnDisp_ProductId),
			COL_HANDLER_MAP_DEF(COLUMN_PartitionStyle, &CPhysicalDriveListPage::OnDisp_PartitionStyle),
			COL_HANDLER_MAP_DEF(COLUMN_BusType,        &CPhysicalDriveListPage::OnDisp_BusType),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CPhysicalDriveListPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CPhysicalDriveListPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)pdi->item.lParam;

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
			;
		}

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
			case WM_TIMER:
				return OnTimer(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
				return OnFindItem(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
		{
			SetWindowPos(m_hWndList,NULL,0,0,cx,cy,SWP_NOMOVE|SWP_NOZORDER);
		}
	}

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		static COLUMN def_columns[] = {
			{ COLUMN_Name,           L"Name",           1, 200,                      LVCFMT_LEFT },
			{ COLUMN_Size,           L"Size",           2, LVSCW_AUTOSIZE,           LVCFMT_RIGHT },
			{ COLUMN_ProductId,      L"Product Id",     3, LVSCW_AUTOSIZE,           LVCFMT_LEFT },
			{ COLUMN_VendorId,       L"Vender Id",      4, LVSCW_AUTOSIZE_USEHEADER, LVCFMT_LEFT },
			{ COLUMN_PartitionStyle, L"PartitionStyle", 5, LVSCW_AUTOSIZE_USEHEADER, LVCFMT_LEFT },
			{ COLUMN_BusType,        L"Bus Type",       6, LVSCW_AUTOSIZE,           LVCFMT_LEFT },
		};

		m_columns.SetDefaultColumns(def_columns,ARRAYSIZE(def_columns));

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

	int Insert(int iItem,PHYSICALDRIVE_NAME_STRING& name,CPhysicalDriveInformation *info,PCWSTR pszDriveName,DWORD dwDriveNumber)
	{
		CPhysicalDriveItem *pItem = new CPhysicalDriveItem;

		pItem->DriveName = _MemAllocString(name.PhysicalDriveName);
		pItem->DriveDevice = _MemAllocString(name.DevicePath);
		pItem->DriveNumber = dwDriveNumber;

		pItem->DeviceDescriptor.VendorId = GetDeviceDescriptorString(diVendorId,info->pDeviceDescriptor);
		pItem->DeviceDescriptor.ProductId = GetDeviceDescriptorString(diProductId,info->pDeviceDescriptor);
		pItem->DeviceDescriptor.SerialNumber =  GetDeviceDescriptorString(diSerialNumber,info->pDeviceDescriptor);
		pItem->DeviceDescriptor.ProductRevision = GetDeviceDescriptorString(diProductRevision,info->pDeviceDescriptor);

		WCHAR szBuf[MAX_PATH];
		GetStorageBusTypeDescString(info->pDeviceDescriptor->BusType,szBuf,MAX_PATH);
		pItem->BufTypeString = _MemAllocString(szBuf);
		pItem->PartitionStyleString = _MemAllocString(GetPartitionStyleText(info->pDriveLayout->PartitionStyle));

		pItem->DriveInfo = info;

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		iItem   = ListView_InsertItem(m_hWndList,&lvi);

		return iItem;
	}

	//
	// Enumerate Physical Drive Items
	//
	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		LoadFltLibDll(NULL);

		SetRedraw(m_hWndList,FALSE);

		//
		// Reset and clear data.
		//
		ListView_DeleteAllItems(m_hWndList);

		//
		// Enumerate Physical Drives
		//
		PHYSICALDRIVE_NAME_STRING_ARRAY *pDisks = NULL;
		EnumPhysicalDriveNames(&pDisks);

		for(ULONG i = 0; i < pDisks->Count; i++)
		{
			PCWSTR pszPhysicalDisk = pDisks->Drive[i].DevicePath;

			int cch = ((sizeof(L"PhysicalDrive")/sizeof(WCHAR))-1);
			DWORD dwDriveNumber = _wtoi( &pszPhysicalDisk[cch] );

			CPhysicalDriveInformation *pdi = new CPhysicalDriveInformation;

			if( pdi->OpenDisk(pszPhysicalDisk,dwDriveNumber) == S_OK )
			{
				pdi->GetGeometry();
				pdi->GetDriveLayout();
				pdi->GetDeviceIdDescriptor();
				pdi->GetDetectSectorSize();

				Insert(i,pDisks->Drive[i],pdi,pszPhysicalDisk,dwDriveNumber);
			}
			else
			{
				delete pdi;
			}
		}

		FreePhysicalDriveNames(pDisks);

		//
		// Adjust column width
		//
		int cColumns = ListViewEx_GetColumnCount(m_hWndList);

		for(int i = 0; i < cColumns; i++)
		{
			int colid = (int)ListViewEx_GetHeaderItemData(m_hWndList,i);

			const COLUMN *col = m_columns.GetDefaultColumnItemFromId(colid);
			if( col )
			{
				ListView_SetColumnWidth(m_hWndList,i,col->cx);
			}
		}

		//
		// Sort in new list.
		//
		DoSort(m_Sort.CurrentSubItem,0);

		UnloadFltLibDll(NULL);

		SetRedraw(m_hWndList,TRUE);

		return S_OK;
	}

	PCHAR GetDeviceDescriptorString(int iItemType,PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor)
	{
		CHAR __based(DeviceDescriptor) *pstr = 0;

		CHAR szBuffer[256];
		PSTR pszText = szBuffer;

		if( iItemType == diVendorId )
		{
			PCHAR ven = pstr + DeviceDescriptor->VendorIdOffset;
			if( DeviceDescriptor->VendorIdOffset != 0 )
			{
				return ven;
			}
			return "";
		}

		if( iItemType == diProductId )
		{
			PCHAR pro = pstr + DeviceDescriptor->ProductIdOffset;
			if( DeviceDescriptor->ProductIdOffset != 0 )
			{
				return pro;
			}
			return "";
		}

		if( iItemType == diProductRevision )
		{
			PCHAR rev = pstr + DeviceDescriptor->ProductRevisionOffset;
			if( DeviceDescriptor->ProductRevisionOffset != 0 )
			{
				return rev;
			}
			return "";
		}

		if( iItemType == diSerialNumber )
		{
			PCHAR num = pstr + DeviceDescriptor->SerialNumberOffset;
			if( DeviceDescriptor->SerialNumberOffset != 0 )
			{
				return num;
			}
			return "";
		}

		return "";
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
	}

	int FindSubItemById(int ColumnId)
	{
		int i,c;
		c = ListViewEx_GetColumnCount(m_hWndList);
		for(i = 0; i < c; i++)
		{
			if( ListViewEx_GetHeaderItemData(m_hWndList,i) == ColumnId )
			{
				return i;
			}
		}
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Sort
	//

	int comp_name(CPhysicalDriveItem *pItem1,CPhysicalDriveItem *pItem2, const void *op)
	{
		return StrCmpLogicalW(pItem1->DriveName,pItem2->DriveName);
	}

	int comp_venderid(CPhysicalDriveItem *pItem1,CPhysicalDriveItem *pItem2, const void *op)
	{
		return _compare_pointer_ansi_string(
					pItem1->DeviceDescriptor.VendorId,
					pItem2->DeviceDescriptor.VendorId,
					((SORT_PARAM<CPhysicalDriveListPage>*)op)->direction);
	}

	int comp_productid(CPhysicalDriveItem *pItem1,CPhysicalDriveItem *pItem2, const void *op)
	{
		return _compare_pointer_ansi_string(
					pItem1->DeviceDescriptor.ProductId,
					pItem2->DeviceDescriptor.ProductId,
					((SORT_PARAM<CPhysicalDriveListPage>*)op)->direction);
	}

	int comp_size(CPhysicalDriveItem *pItem1,CPhysicalDriveItem *pItem2, const void *op)
	{
		return _COMP(pItem1->DriveInfo->pGeometry->DiskSize.QuadPart,
					 pItem2->DriveInfo->pGeometry->DiskSize.QuadPart);
	}

	int comp_partition_style(CPhysicalDriveItem *pItem1,CPhysicalDriveItem *pItem2, const void *op)
	{
		return _compare_pointer_string(
					pItem1->PartitionStyleString,
					pItem2->PartitionStyleString,
					((SORT_PARAM<CPhysicalDriveListPage>*)op)->direction);
	}

	int comp_bustype(CPhysicalDriveItem *pItem1,CPhysicalDriveItem *pItem2, const void *op)
	{
		return _compare_pointer_string(
					pItem1->BufTypeString,
					pItem2->BufTypeString,
					((SORT_PARAM<CPhysicalDriveListPage>*)op)->direction);
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CPhysicalDriveListPage,CPhysicalDriveItem> comp_proc[] = 
		{
			{ COLUMN_Name,           &CPhysicalDriveListPage::comp_name },
			{ COLUMN_Size,           &CPhysicalDriveListPage::comp_size },
			{ COLUMN_ProductId,      &CPhysicalDriveListPage::comp_productid },
			{ COLUMN_VendorId,       &CPhysicalDriveListPage::comp_venderid },
			{ COLUMN_PartitionStyle, &CPhysicalDriveListPage::comp_partition_style },
			{ COLUMN_BusType,        &CPhysicalDriveListPage::comp_bustype },
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CPhysicalDriveListPage,CPhysicalDriveItem> [COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CPhysicalDriveListPage,CPhysicalDriveItem>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc  = comp_proc[i].proc;
		}
	}

	int CompareItem(CPhysicalDriveItem *pItem1,CPhysicalDriveItem *pItem2,SORT_PARAM<CPhysicalDriveListPage> *op)
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
		CPhysicalDriveItem *pItem1 = (CPhysicalDriveItem *)lParam1;
		CPhysicalDriveItem *pItem2 = (CPhysicalDriveItem *)lParam2;
		SORT_PARAM<CPhysicalDriveListPage> *op = (SORT_PARAM<CPhysicalDriveListPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,CPhysicalDriveItem *)
	{
		SORT_PARAM<CPhysicalDriveListPage> op = {0};
		op.pThis = this;
		op.id = id;
		op.direction = m_Sort.Direction; // 1 or -1 do not use 0
		ListView_SortItems(m_hWndList,CompareProc,&op);
	}

	//
	// Commaand Handling
	//

	VOID OpenInformationView(int iItem,UINT ConsoleId)
	{
		if( iItem != -1 )
		{
			CPhysicalDriveItem *pItem = (CPhysicalDriveItem *)ListViewEx_GetItemData(m_hWndList,iItem);
			if( pItem && pItem->DriveName )
				SendMessage(GetActiveWindow(),WM_OPEM_MDI_CHILDFRAME,ConsoleId,(LPARAM)pItem->DriveName);
		}
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_PHYSICALDRIVEINFORMATION:
			case ID_DRIVELAYOUT:
			case ID_HEXDUMP:
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
			case ID_PHYSICALDRIVEINFORMATION:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION );
				break;
			case ID_DRIVELAYOUT:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_DISKLAYOUT );
				break;
			case ID_HEXDUMP:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_SIMPLEHEXDUMP );
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
