#pragma once
//***************************************************************************
//*                                                                         *
//*  page_volumelist.h                                                      *
//*                                                                         *
//*  Volume List Page                                                       *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-11-01                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "dparray.h"
#include "common.h"
#include "column.h"
#include "misc.h"
#include "findhandler.h"
#include <devguid.h>

#define _ENABLE_GROUP_VIEW  0 // Reserved

#define TE_OPEN_MDI_CHILD_FRAME  (1)

struct CVolumeItem
{
public:
	VOLUME_DEVICE_INFORMATION *VolInfoBuffer;
	PWSTR VolumeName;
	PWSTR VolumeDevice;
	PWSTR GuidName;
	PWSTR Drive;
	PWSTR DrivePaths;
	double DiskUsage;
	LONGLONG TotalSize;
	LONGLONG AvailableSize;
	LONGLONG Usage;

	CVolumeItem()
	{
		VolInfoBuffer = NULL;
		VolumeName = NULL;
		VolumeDevice = NULL;
		GuidName = NULL;
		VolInfoBuffer = NULL;
		Drive = NULL;
		DrivePaths = NULL;
		DiskUsage = 0.0;
		TotalSize = 0;
		AvailableSize = 0;
		Usage = 0;
	}
	~CVolumeItem()
	{
		_SafeMemFree(VolumeName);
		_SafeMemFree(VolumeDevice);
		_SafeMemFree(GuidName);
		_SafeMemFree(Drive);
		_SafeMemFree(DrivePaths);
		DestroyVolumeInformationBuffer(VolInfoBuffer);
	}
};

//////////////////////////////////////////////////////////////////////////////

class CVolumeListPage : 
	public CPageWndBase,
	public CFindHandler<CVolumeListPage>
{
	HWND m_hWndList;

#if _ENABLE_GROUP_VIEW
	enum {
		ID_GROUP_DISK=1,
		ID_GROUP_CDROM,
		ID_GROUP_FLOPPY,
		ID_GROUP_VIRTUAL,
		ID_GROUP_SHADOWCOPY
	};
#endif

	COLUMN_HANDLER_DEF<CVolumeListPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CVolumeListPage,CVolumeItem>* m_comp_proc;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	HFONT m_hFont;
	HFONT m_hFontHeader;
	UINT  m_MeterStyle;

	CColumnList m_columns;

public:
	CVolumeListPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = 0;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_MeterStyle = 0;
	}

	~CVolumeListPage()
	{
		if( m_disp_proc )
			delete[] m_disp_proc;

		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	HWND GetListView() const { return m_hWndList; }

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

		ListViewEx_SetTrickColumnZero(m_hWndList,TRUE);

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		_EnableVisualThemeStyle(m_hWndList);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP|LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		static COLUMN def_columns[] = {
			{ COLUMN_Name,         L"Name",           1, 160, LVCFMT_LEFT },
			{ COLUMN_Drive,        L"Drive",          2, 100, LVCFMT_LEFT },
			{ COLUMN_UsageRate,    L"Usage %",        3, 100, LVCFMT_CENTER },
			{ COLUMN_Size,         L"Size",           4, 100, LVCFMT_RIGHT },
			{ COLUMN_Usage,        L"Usage",          5, 100, LVCFMT_RIGHT },
			{ COLUMN_Free,         L"Free",           6, 100, LVCFMT_RIGHT },
			{ COLUMN_VolumeLabel,  L"Label",          7, 100, LVCFMT_LEFT },
			{ COLUMN_Format,       L"Format",         8,  80, LVCFMT_LEFT },
			{ COLUMN_Guid,         L"Guid",           9, 340, LVCFMT_LEFT },
			{ COLUMN_CreationTime, L"Creation Time", 10, 160, LVCFMT_LEFT },
		};

		m_columns.SetDefaultColumns(def_columns,ARRAYSIZE(def_columns));

		if( !LoadColumns(m_hWndList) )
		{
			InitColumns(m_hWndList);
		}

		ListViewEx_SetTrickColumnZero(m_hWndList,FALSE);

#if _ENABLE_GROUP_VIEW
		if( 1 )
		{
			ListView_EnableGroupView(m_hWndList,TRUE);
			InitGroup();
		}
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
		m_hFontHeader = GetIconFont();
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_hFont )
			DeleteObject(m_hFont);
		if( m_hFontHeader )
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

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		CVolumeItem *pItem = (CVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();

		if( SendMessage(GetActiveWindow(),PM_MAKECONTEXTMENU,MAKEWPARAM(VOLUME_CONSOLE_VOLUMELIST,0),(LPARAM)hMenu) == 0 )
		{
			HMENU hSubMenu;

			AppendMenu(hMenu,MF_STRING,ID_VOLUMEINFORMATION,L"Open &Information");
			AppendMenu(hMenu,MF_STRING,ID_FILESYSTEMSTATISTICS,L"Open File System &Statistics");
			AppendMenu(hMenu,MF_STRING,0,0);
			AppendMenu(hMenu,MF_STRING,ID_DISKPERFORMANCE,L"Disk &Performance");
			AppendMenu(hMenu,MF_STRING,ID_HEXDUMP,L"Cluster &Dump");
			AppendMenu(hMenu,MF_STRING,ID_FILE_SIMPLEFILELIST,L"Display &Files in Volume");
			AppendMenu(hMenu,MF_STRING,0,0);
			hSubMenu = CreatePopupMenu();
			{
				AppendMenu(hSubMenu,MF_STRING,ID_OPEN_LOCATION_EXPLORER,   L"&Explorer");
				AppendMenu(hSubMenu,MF_STRING,ID_OPEN_LOCATION_TERMINAL,   L"&Terminal");
				AppendMenu(hSubMenu,MF_STRING,ID_OPEN_LOCATION_POWERSHELL, L"&PowerShell");
				AppendMenu(hSubMenu,MF_STRING,ID_OPEN_LOCATION_CMDPROMPT,  L"&Command Prompt");
				AppendMenu(hSubMenu,MF_STRING,ID_OPEN_LOCATION_BASH,       L"&Bash");
			}
			AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hSubMenu,L"Open &Location");

			AppendMenu(hMenu,MF_STRING,0,0);
			AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
			SetMenuDefaultItem(hMenu,ID_VOLUMEINFORMATION,FALSE);
		}

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	LRESULT OnQueryMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_QUERY_SELECTEDITEM:
			{
				int iItem = ListViewEx_GetCurSel(m_hWndList);
				if( iItem != -1 )
				{
					CVolumeItem *pItem = (CVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem);
					if( pItem && pItem->VolumeDevice )
					{
						if( lParam != 0 )
						{
							UIS_SELECTEDITEM *ptr = (UIS_SELECTEDITEM*)lParam;
							if( ptr->Path && ptr->cchPath )
							{	
								if( StringCchCopy(ptr->Path,ptr->cchPath,pItem->VolumeDevice) == S_OK )
									return (LRESULT)TRUE;
							}
						}
					}
				}
				break;
			}
		}
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
			case HDN_ENDDRAG:
				OnHeaderEndDrag(pnmhdr);
				return TRUE; // prevent order change
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
		LRESULT lResult = 0;

		if( pnmhdr->hwndFrom != m_hWndList )
			return 0;

		NMLVCUSTOMDRAW* pcd = (NMLVCUSTOMDRAW* )pnmhdr;

		if( pcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			CVolumeItem *pParam = (CVolumeItem *)pcd->nmcd.lItemlParam;

			LRESULT RetFlag = 0;

			if( pParam->VolInfoBuffer->VirtualDiskVolume )
			{
#if _ENABLE_DARK_MODE_TEST
				pcd->clrText = _IsDarkModeEnabled() ? RGB(153,217,234) : _COLOR_TEXT_VIRTUALDISK;
#else
				pcd->clrText = _COLOR_TEXT_VIRTUALDISK;
#endif
				RetFlag = CDRF_NEWFONT;
			} 

			if( pParam->VolInfoBuffer->DirtyBit != (CHAR)-1 )
			{
				if( pParam->VolInfoBuffer->DirtyBit != 0 )
				{
					pcd->clrTextBk = _COLOR_BKGD_DIRTY_VOLUME;

					if( CDIS_SELECTED & pcd->nmcd.uItemState )
						pcd->clrText = _COLOR_TEXT_DIRTY_VOLUME;

					RetFlag = CDRF_NEWFONT;
				}
			}

			return CDRF_NOTIFYPOSTPAINT|RetFlag;
		}

		if( pcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT )
		{
			CVolumeItem *pParam = (CVolumeItem *)pcd->nmcd.lItemlParam;
			if( pParam->VolInfoBuffer->TotalAllocationUnits.QuadPart != 0 )
			{
				HWND hWndHeader = ListView_GetHeader(m_hWndList);
				int i,cColumns = Header_GetItemCount( hWndHeader );
				int iUsageRateColumn = -1;

				for(i = 0; i < cColumns; i++)
				{
					int id = (int)ListViewEx_GetHeaderItemData(pcd->nmcd.hdr.hwndFrom,i);
					if( id == COLUMN_UsageRate )
					{
						iUsageRateColumn = i;
						break;
					}
				}

				if( iUsageRateColumn != -1 )
				{
					DrawListViewColumnMeter(pcd->nmcd.hdc,m_hWndList,(int)pcd->nmcd.dwItemSpec,iUsageRateColumn,NULL,GetFont(),pParam->DiskUsage,
						_IsDarkModeEnabled() ? 2 : 0);
				}
			}

			if( IsXpThemeEnabled() )
			{
				UINT State = ListView_GetItemState(m_hWndList,(int)pcd->nmcd.dwItemSpec,LVIS_FOCUSED);
				if( State & LVIS_FOCUSED )
				{
					_DrawFocusFrame(m_hWndList,pcd->nmcd.hdc,&pcd->nmcd.rc);
				}
			}
		}

		return CDRF_DODEFAULT;
	}

	HFONT GetFont()
	{
		if( m_hFont )
			return m_hFont;
		return (HFONT)SendMessage(m_hWndList,WM_GETFONT,0,0);
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
		CVolumeItem *pItem = (CVolumeItem *)pnmlv->lParam;
		delete pItem;
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
#if 1
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		if( pnmia->iItem != -1 )
		{
			CVolumeItem *pItem = (CVolumeItem *)ListViewEx_GetItemData(m_hWndList,pnmia->iItem);

			UIS_ITEM_ACTIVATED uia = {0};
			uia.ConsoleTypeId = VOLUME_CONSOLE_VOLUMELIST;
			uia.Path = pItem->VolumeDevice;
			if( SendMessage(GetActiveWindow(),WM_NOTIFY_MESSAGE,MAKEWPARAM(UI_NOTIFY_ITEM_ACTIVATED,0),(LPARAM)&uia) != 0 )
			{
				return 0;
			}

			OpenInformationView((int)pnmia->iItem,VOLUME_CONSOLE_VOLUMEINFORMAION);
		}
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

		CVolumeItem *pItem = (CVolumeItem *)pnmlv->lParam;

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
		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->VolumeName;
		return 0;
	}

	LRESULT OnDisp_Size(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;

		if( !pItem->VolInfoBuffer->State.SizeInformation )
			return 0;

		LONGLONG cb;

		if( id == COLUMN_Size )
			cb = pItem->TotalSize;
		else if( id == COLUMN_Free )
			cb = pItem->AvailableSize;
		else if( id == COLUMN_Usage )
			cb = pItem->Usage;

		if( 1 )
		{
			StrFormatByteSizeW(cb,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
		}
		else
		{
			_CommaFormatString(cb,pnmlvdi->item.pszText);
		}

		return 0;
	}

	LRESULT OnDisp_Format(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,pItem->VolInfoBuffer->FileSystemName);
		return 0;
	}

	LRESULT OnDisp_Guid(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;
		if( pItem->GuidName )
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,pItem->GuidName);
		return 0;
	}

	LRESULT OnDisp_Drive(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,pItem->Drive);
		return 0;
	}

	LRESULT OnDisp_UsageRate(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		*pnmlvdi->item.pszText = L'\0';

		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;

		if( pItem->VolInfoBuffer->TotalAllocationUnits.QuadPart != 0 )
		{
			if( (pItem->DiskUsage * 100.0) == 0.0 )
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0%%");
			else if( (pItem->DiskUsage * 100.0) < 1.0 )
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%.g%%",(pItem->DiskUsage * 100.0));
			else
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%d%%",(int)(pItem->DiskUsage * 100.0));
		}
		return 0;
	}

	LRESULT OnDisp_VolumeLabel(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		*pnmlvdi->item.pszText = L'\0';

		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;
		if( pItem->VolInfoBuffer->VolumeLabel )
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",pItem->VolInfoBuffer->VolumeLabel);

		return 0;
	}

	LRESULT OnDisp_CreationTime(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		*pnmlvdi->item.pszText = L'\0';

		CVolumeItem *pItem = (CVolumeItem *)pnmlvdi->item.lParam;
		if( pItem->VolInfoBuffer->VolumeCreationTime.QuadPart > 0 )
		{
			_GetDateTimeStringEx(
					pItem->VolInfoBuffer->VolumeCreationTime.QuadPart,
					pnmlvdi->item.pszText,
					pnmlvdi->item.cchTextMax,
					NULL,NULL,FALSE);
		}
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CVolumeListPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_Name,           &CVolumeListPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_Size,           &CVolumeListPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_Usage,          &CVolumeListPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_Free,           &CVolumeListPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_Format,         &CVolumeListPage::OnDisp_Format),
			COL_HANDLER_MAP_DEF(COLUMN_Guid,           &CVolumeListPage::OnDisp_Guid),
			COL_HANDLER_MAP_DEF(COLUMN_UsageRate,      &CVolumeListPage::OnDisp_UsageRate),
			COL_HANDLER_MAP_DEF(COLUMN_Drive,          &CVolumeListPage::OnDisp_Drive),
			COL_HANDLER_MAP_DEF(COLUMN_VolumeLabel,    &CVolumeListPage::OnDisp_VolumeLabel),
			COL_HANDLER_MAP_DEF(COLUMN_CreationTime,   &CVolumeListPage::OnDisp_CreationTime),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CVolumeListPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CVolumeListPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CVolumeItem *pItem = (CVolumeItem *)pdi->item.lParam;

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
			; // Reserved.
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
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_MESSAGE:
				return OnQueryMessage(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
				return OnFindItem(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
		{
			SetWindowPos(m_hWndList,NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE);
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

#if _ENABLE_GROUP_VIEW
	typedef struct _GROUP_ITEM
	{
		int idGroup;
		UINT idGroupTitle;
		PCWSTR Text;
	} GROUP_ITEM;

	void InitGroup()
	{
		GROUP_ITEM Group[] = {
			{ ID_GROUP_DISK,   0, L"Disk Drive" },
			{ ID_GROUP_CDROM,  0, L"CD-ROM"  },
			{ ID_GROUP_FLOPPY, 0, L"Floppy Disk"  },
			{ ID_GROUP_VIRTUAL, 0, L"Virtual"  },
			{ ID_GROUP_SHADOWCOPY, 0, L"Shadow Copy"  },
		};
		int cGroupItem = ARRAYSIZE(Group);

		for(int i = 0; i < cGroupItem; i++)
		{
			InsertGroup(m_hWndList,Group[i].idGroup,Group[i].Text);
		}
	}
#endif

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		int i,c;
		c = m_columns.GetDefaultColumnCount();
		for(i = 0; i < c; i++)
		{
			const COLUMN *pcol = m_columns.GetDefaultColumnItem(i);
			lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
			lvc.fmt     = pcol->fmt;
			lvc.cx      = pcol->cx >= 0 ? _DPI_Adjust_X(pcol->cx) : pcol->cx;
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

		// If already inserted 'trick column', add offset to an order value.
		int iTrickOrder = ListViewEx_GetColumnCount(hWndList);

		ULONG i;
		for(i = 0; i < pcoltbl->cItems; i++)
		{
			const COLUMN *pcol = &pcoltbl->column[i];
			lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
			lvc.fmt     = pcol->fmt;
			lvc.cx      = pcol->cx;
			lvc.pszText = pcol->Name;
			lvc.iOrder  = pcol->iOrder + iTrickOrder;
			int index = ListView_InsertColumn(hWndList,lvc.iOrder,&lvc);

			ListViewEx_SetHeaderItemData( hWndList, index, pcol->id );
		}

		return TRUE;
	}

	HRESULT GetData(PCWSTR pszVolumeName,VOLUME_DEVICE_INFORMATION **VolInfoBufferPtr)
	{
		ULONG OpenFlags = 0;
		if( IsUserAnAdmin() )
			OpenFlags = OPEN_VOLUME_READ_DATA;

		HRESULT hr;
		hr = CreateVolumeInformationBuffer(pszVolumeName,0,OpenFlags,(void **)VolInfoBufferPtr);

		return hr;
	}

	__forceinline LONGLONG _calcSize(LONGLONG TotalAllocationUnits,LONGLONG SectorsPerAllocationUnit,LONGLONG  BytesPerSector)
	{
		return TotalAllocationUnits * SectorsPerAllocationUnit * BytesPerSector;
	}

	int Insert(int iItem,const VOLUME_NAME_STRING& name,int GroupId=0)
	{
		VOLUME_DEVICE_INFORMATION *VolInfoBufferPtr;
		GetData(name.NtVolumeName,&VolInfoBufferPtr);

		const DWORD cchDriveName = 32768;
		WCHAR *DriveName = new WCHAR[cchDriveName];
		WCHAR GuidVolume[260];

		ZeroMemory(DriveName,sizeof(WCHAR) * cchDriveName);

		StringCchPrintf(GuidVolume,ARRAYSIZE(GuidVolume),L"\\\\?\\%s\\",name.VolumeGuidString);
		GetVolumeDrivePathsString(GuidVolume,DriveName,cchDriveName);

		CVolumeItem *pItem = new CVolumeItem;

		pItem->VolumeName = _MemAllocString( wcsrchr(name.NtVolumeName,L'\\') + 1 );
		pItem->VolumeDevice = _MemAllocString( name.NtVolumeName );
		if( name.VolumeGuidString )
			pItem->GuidName = _MemAllocString( name.VolumeGuidString );
		pItem->VolInfoBuffer = VolInfoBufferPtr;

		pItem->DrivePaths = _MemAllocString( DriveName );

		if( IsCharAlpha(DriveName[0]) && DriveName[1] == L':' && (DriveName[2] == L'\0'||DriveName[2] == L';') )
		{
			if( DriveName[2] == L';' )
				DriveName[2] = L'\0';
			pItem->Drive = _MemAllocString( DriveName );
		}
		if( pItem->Drive == NULL )
			pItem->Drive = _MemAllocString( L"" );

		if( pItem->VolInfoBuffer->State.SizeInformation )
		{
			LONGLONG Total;
			Total = _calcSize( pItem->VolInfoBuffer->TotalAllocationUnits.QuadPart,
						pItem->VolInfoBuffer->SectorsPerAllocationUnit,
						pItem->VolInfoBuffer->BytesPerSector );

			LONGLONG Available;
			Available = _calcSize( pItem->VolInfoBuffer->AvailableAllocationUnits.QuadPart,
						pItem->VolInfoBuffer->SectorsPerAllocationUnit,
						pItem->VolInfoBuffer->BytesPerSector);

			LONGLONG Usage;
			Usage = Total - Available;

			pItem->TotalSize = Total;
			pItem->AvailableSize = Available;
			pItem->Usage = Usage;

			if( Total != 0 )
			{
				double pct = (double)Usage / (double)Total;
				pItem->DiskUsage = pct;
			}
		}

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;

		if( GroupId != 0 )
		{
			lvi.mask |= LVIF_GROUPID;
			lvi.iGroupId = GroupId;
		}

		iItem   = ListView_InsertItem(m_hWndList,&lvi);

		delete[] DriveName;

		return iItem;
	}

#if _ENABLE_INSERT_SHADOWCOPY_ITEMS
	int InsertShadowCopy(int iItem,PCWSTR pszVolumeName,int GroupId=0)
	{
		VOLUME_DEVICE_INFORMATION *VolInfoBufferPtr;
		GetData(pszVolumeName,&VolInfoBufferPtr);

		const DWORD cchDriveName = 32768;
		WCHAR *DriveName = new WCHAR[cchDriveName];
		WCHAR GuidVolume[260];

		ZeroMemory(DriveName,sizeof(WCHAR) * cchDriveName);

		StringCchPrintf(GuidVolume,ARRAYSIZE(GuidVolume),L"\\\\?\\%s\\",pszVolumeName);
		GetVolumeDrivePathsString(GuidVolume,DriveName,cchDriveName);

		CVolumeItem *pItem = new CVolumeItem;

		pItem->VolumeName = _MemAllocString( wcsrchr(pszVolumeName,L'\\') + 1 );
		pItem->VolumeDevice = _MemAllocString( pszVolumeName );
		pItem->GuidName = NULL;
		pItem->VolInfoBuffer = VolInfoBufferPtr;

		pItem->DrivePaths = _MemAllocString( DriveName );

		if( IsCharAlpha(DriveName[0]) && DriveName[1] == L':' && (DriveName[2] == L'\0'||DriveName[2] == L';') )
		{
			if( DriveName[2] == L';' )
				DriveName[2] = L'\0';
			pItem->Drive = _MemAllocString( DriveName );
		}
		if( pItem->Drive == NULL )
			pItem->Drive = _MemAllocString( L"" );

		if( pItem->VolInfoBuffer->State.SizeInformation )
		{
			LONGLONG Total;
			Total = _calcSize( pItem->VolInfoBuffer->TotalAllocationUnits.QuadPart,
						pItem->VolInfoBuffer->SectorsPerAllocationUnit,
						pItem->VolInfoBuffer->BytesPerSector );

			LONGLONG Available;
			Available = _calcSize( pItem->VolInfoBuffer->AvailableAllocationUnits.QuadPart,
						pItem->VolInfoBuffer->SectorsPerAllocationUnit,
						pItem->VolInfoBuffer->BytesPerSector);

			LONGLONG Usage;
			Usage = Total - Available;

			pItem->TotalSize = Total;
			pItem->AvailableSize = Available;
			pItem->Usage = Usage;

			if( Total != 0 )
			{
				double pct = (double)Usage / (double)Total;
				pItem->DiskUsage = pct;
			}
		}

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;

		if( GroupId != 0 )
		{
			lvi.mask |= LVIF_GROUPID;
			lvi.iGroupId = GroupId;
		}

		iItem   = ListView_InsertItem(m_hWndList,&lvi);

		delete[] DriveName;

		return iItem;
	}
#endif

	//
	// Enumerate volume items
	//
	HRESULT FillItems(SELECT_ITEM *)
	{
		CWaitCursor wait;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		//
		// Enumerate volumes
		//
		VOLUME_NAME_STRING_ARRAY *pVolNames;
		EnumVolumeNames(&pVolNames);

		for(ULONG i = 0; i < pVolNames->Count; i++)
		{
			Insert(i,pVolNames->Volume[i],0);
		}

		FreeVolumeNames(pVolNames);

#if _ENABLE_INSERT_SHADOWCOPY_ITEMS
		HANDLE hHardwareProduct;
		if( GetKnownHardwareProducts(&hHardwareProduct,&GUID_DEVCLASS_VOLUMESNAPSHOT,0) )
		{
			ULONG i,cItems = GetKnownHardwareProductsCount(hHardwareProduct);

			const FS_HARDWARE_PRODUCT *pProductInfo;
			for(i = 0; i < cItems; i++)
			{
				if( GetKnownHardwareProductPointer(hHardwareProduct,i,&pProductInfo) )
				{
					if( (*pProductInfo).PysicalDeviceObjectName )
						InsertShadowCopy(i,(*pProductInfo).PysicalDeviceObjectName,0);
				}
			}
			FreeKnownHardwareProducts(hHardwareProduct);
		}
#endif

		//
		// Sort in new list.
		//
		DoSort(m_Sort.CurrentSubItem,0);

		//
		// Default selection (if exists).
		//
		SetRedraw(m_hWndList,TRUE);

		RedrawWindow(m_hWndList,NULL,NULL,RDW_UPDATENOW|RDW_INVALIDATE);

		return S_OK;
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

	int comp_name(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		if( _wcsnicmp(pItem1->VolumeName,L"HarddiskVolume",14) == 0 && _wcsnicmp(pItem2->VolumeName,L"HarddiskVolume",14) != 0 )
			return -1;
		if( _wcsnicmp(pItem1->VolumeName,L"HarddiskVolume",14) != 0 && _wcsnicmp(pItem2->VolumeName,L"HarddiskVolume",14) == 0 )
			return 1;

		return StrCmpLogicalW(pItem1->VolumeName,pItem2->VolumeName);
	}

	int comp_drive(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)p;
		return _compare_pointer_string(pItem1->Drive,pItem2->Drive,op->direction);
	}

	int comp_format(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)p;
		return _compare_pointer_string(pItem1->VolInfoBuffer->FileSystemName,pItem2->VolInfoBuffer->FileSystemName,op->direction);
	}

	int comp_volumelabel(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)p;
		return _compare_pointer_string_logical(pItem1->VolInfoBuffer->VolumeLabel,pItem2->VolInfoBuffer->VolumeLabel,op->direction);
	}

	int comp_guid(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)p;
		return _compare_pointer_string(pItem1->GuidName,pItem2->GuidName,op->direction);
	}

	int comp_size(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)p;

		LONGLONG cb1,cb2;

		switch( op->id )
		{
			case COLUMN_Size:
				cb1 = pItem1->TotalSize;
				cb2 = pItem2->TotalSize;
				break;
			case COLUMN_Free:
				cb1 = pItem1->AvailableSize;
				cb2 = pItem2->AvailableSize;
				break;
			case COLUMN_Usage:
				cb1 = pItem1->Usage;
				cb2 = pItem2->Usage;
				break;
		}

		if( pItem1->VolInfoBuffer->State.SizeInformation != 0
			&& pItem2->VolInfoBuffer->State.SizeInformation == 0 )
			return (-1 * op->direction);
		else if( pItem1->VolInfoBuffer->State.SizeInformation == 0
			&& pItem2->VolInfoBuffer->State.SizeInformation != 0 )
			return (1  * op->direction);

		return _COMP(cb1,cb2);
	}

	int comp_usagerate(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)p;

		if( pItem1->DiskUsage != 0 && pItem2->DiskUsage == 0 )
			return (-1 * op->direction);
		else if( pItem1->DiskUsage == 0 && pItem2->DiskUsage != 0 )
			return (1  * op->direction);

		return _COMP(pItem1->DiskUsage,pItem2->DiskUsage);
	}

	int comp_creationtime(CVolumeItem *pItem1,CVolumeItem *pItem2, const void *p)
	{
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)p;
		return _COMP(pItem1->VolInfoBuffer->VolumeCreationTime.QuadPart,
					 pItem2->VolInfoBuffer->VolumeCreationTime.QuadPart);
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CVolumeListPage,CVolumeItem> comp_proc[] = 
		{
			{COLUMN_Name,        &CVolumeListPage::comp_name},
			{COLUMN_Drive,       &CVolumeListPage::comp_drive},
			{COLUMN_Size,        &CVolumeListPage::comp_size},
			{COLUMN_Free,        &CVolumeListPage::comp_size},
			{COLUMN_Usage,       &CVolumeListPage::comp_size},
			{COLUMN_UsageRate,   &CVolumeListPage::comp_usagerate},
			{COLUMN_VolumeLabel, &CVolumeListPage::comp_volumelabel},
			{COLUMN_Format,      &CVolumeListPage::comp_format},
			{COLUMN_Guid,        &CVolumeListPage::comp_guid},
			{COLUMN_CreationTime,&CVolumeListPage::comp_creationtime},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CVolumeListPage,CVolumeItem>[COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CVolumeListPage,CVolumeItem>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid  = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc   = comp_proc[i].proc;
		}
	}

	int CompareItem(CVolumeItem *pItem1,CVolumeItem *pItem2,SORT_PARAM<CVolumeListPage> *op)
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
		CVolumeItem *pItem1 = (CVolumeItem *)lParam1;
		CVolumeItem *pItem2 = (CVolumeItem *)lParam2;
		SORT_PARAM<CVolumeListPage> *op = (SORT_PARAM<CVolumeListPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,CVolumeItem *)
	{
		SORT_PARAM<CVolumeListPage> op = {0};
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
			CVolumeItem *pItem = (CVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem);
			if( pItem && pItem->VolumeDevice )
			{
				OpenConsole_SendMessage(ConsoleId,pItem->VolumeDevice);
			}
		}
	}

	VOID OpenLocation(int iItem,int Open)
	{
		if( iItem != -1 )
		{
			CVolumeItem *pItem = (CVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem);

			if( GetKeyState(VK_CONTROL) < 0 )
				Open |= 0x8000;

			OpenVolumeLocationByShell(GetActiveWindow(),Open,pItem->Drive,pItem->GuidName);
		}
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_VOLUMEINFORMATION:
			case ID_FILESYSTEMSTATISTICS:
			case ID_DISKPERFORMANCE:
			case ID_HEXDUMP:
			case ID_EDIT_COPY:
			case ID_OPEN_LOCATION_EXPLORER:
			case ID_OPEN_LOCATION_CMDPROMPT:
			case ID_OPEN_LOCATION_POWERSHELL:
			case ID_OPEN_LOCATION_TERMINAL:
			case ID_OPEN_LOCATION_BASH:
			case ID_FILE_SIMPLEFILELIST:
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
			case ID_VOLUMEINFORMATION:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_VOLUMEINFORMAION );
				break;
			case ID_FILESYSTEMSTATISTICS:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_FILESYSTEMSTATISTICS );
				break;
			case ID_FILE_SIMPLEFILELIST:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST );
				break;
			case ID_HEXDUMP:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_SIMPLEHEXDUMP );
				break;
			case ID_DISKPERFORMANCE:
				OpenInformationView( ListViewEx_GetCurSel(m_hWndList), VOLUME_CONSOLE_DISKPERFORMANCE );
				break;
			case ID_OPEN_LOCATION_EXPLORER:
				OpenLocation( ListViewEx_GetCurSel(m_hWndList), 0 );
				break;
			case ID_OPEN_LOCATION_CMDPROMPT:
				OpenLocation( ListViewEx_GetCurSel(m_hWndList), 1 );
				break;
			case ID_OPEN_LOCATION_POWERSHELL:
				OpenLocation( ListViewEx_GetCurSel(m_hWndList), 2 );
				break;
			case ID_OPEN_LOCATION_TERMINAL:
				OpenLocation( ListViewEx_GetCurSel(m_hWndList), 3 );
				break;
			case ID_OPEN_LOCATION_BASH:
				OpenLocation( ListViewEx_GetCurSel(m_hWndList), 4 );
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
