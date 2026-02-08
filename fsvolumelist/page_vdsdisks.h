#pragma once
//****************************************************************************
//*                                                                          *
//*  page_vdsmanager.h                                                       *
//*                                                                          *
//*  Information Manager for VDS                                             *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2025-09-24                                                     *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "dparray.h"
#include "common.h"
#include "column.h"
#include "misc.h"
#include "findhandler.h"
#include "vdshelp.h"

#define IDW_LIST         100
#define IDW_DISKLIST     101
#define IDW_VOLUMELIST   102
#define IDW_EXTENTLIST   103

#define ID_EDIT_COPY_VALUE 40300

#define ID_PANE_VOLUME     1
#define ID_PANE_EXTENT     2

//////////////////////////////////////////////////////////////////////////////

class CVDSDisksPage : 
	public CPageWndBase,
	public CFindHandler<CVDSDisksPage>
{
	CVDSDataManager *m_pVDSData;

	HWND m_hWndList;
	HWND m_hwndVolumeLV;
	HWND m_hwndExtentLV;
	HWND m_hwndPropLV;
	HWND m_hwndCurVolExtLV;
	HWND m_hwndFocusCtrl;

	CSimpleToolbar m_Toolbar;

	COLUMN_HANDLER_DEF<CVDSDisksPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CVDSDisksPage,CVDSDiskItem>* m_comp_proc;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	HFONT m_hFont;
	HFONT m_hFontHeader;

	CColumnList m_columns;

public:
	CVDSDisksPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = 0;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_pVDSData = NULL;
		m_hwndFocusCtrl = NULL;
		m_hwndCurVolExtLV = NULL;
	}

	~CVDSDisksPage()
	{
		if( m_disp_proc )
			delete[] m_disp_proc;

		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	HWND GetListView() const {
		return m_hwndFocusCtrl;
	}

	virtual HRESULT OnInitPage(PVOID ptr,DWORD,PVOID)
	{
		SELECT_ITEM *SelectItem = (SELECT_ITEM *)ptr;

		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)IDW_LIST,
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
			{ COLUMN_VDS_Name,           L"Name",            1, 0, LVCFMT_LEFT },
			{ COLUMN_VDS_FriendlyName,   L"Friendly Name",   2, 0, LVCFMT_LEFT },
			{ COLUMN_VDS_Size,           L"Size",            3, 0, LVCFMT_RIGHT },
			{ COLUMN_VDS_Status,         L"Status",          4, 0, LVCFMT_LEFT },
			{ COLUMN_VDS_PartitionStyle, L"Partition Style", 5, 0, LVCFMT_LEFT },
			{ COLUMN_VDS_BusType,        L"Bus Type",        6, 0, LVCFMT_RIGHT },
		};

		m_columns.SetDefaultColumns(def_columns,ARRAYSIZE(def_columns));

		static COLUMN_NAME column_name_map[] = {
			{ COLUMN_VDS_Name,           L"Name",               0 },
			{ COLUMN_VDS_FriendlyName,   L"FriendlyName",       0 }, 
			{ COLUMN_VDS_PartitionStyle, L"PartitionStyle",     0 }, 
			{ COLUMN_VDS_Status,         L"Status",             0 }, 
			{ COLUMN_VDS_Size,           L"Size",               0 }, 
			{ COLUMN_VDS_BusType,        L"BusType",            0 }, 
		};

		m_columns.SetColumnNameMap(_countof(column_name_map),column_name_map);

		if( !LoadColumns(m_hWndList,nullptr,0) )
		{
			InitColumns(m_hWndList);
		}

		ListViewEx_SetTrickColumnZero(m_hWndList,FALSE);

		Init_DiskList();
		Init_VolumeList();
		Init_ExtentList();

	    TBBUTTON tbButtons[] = 
		{
			{ MAKELONG(0, 0), ID_PANE_VOLUME, TBSTATE_ENABLED, BTNS_CHECKGROUP|BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Volume" },
	        { MAKELONG(0, 0), ID_PANE_EXTENT, TBSTATE_ENABLED, BTNS_CHECKGROUP|BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Extent" },
		};

		DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE |
			TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT;

		m_Toolbar.CreateSimpleToolbar(m_hWnd,tbButtons,ARRAYSIZE(tbButtons),dwStyle);

		ChangeRightPane( ID_PANE_VOLUME );
		SendMessage(m_Toolbar.m_hWnd,TB_CHECKBUTTON,ID_PANE_VOLUME,TRUE);

		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		return S_OK;
	}

	virtual HRESULT OnDestroyPage(PVOID)
	{
		return S_OK;
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
		DetachHookProc(m_hwndPropLV);
		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnCommand(HWND,UINT,WPARAM wParam,LPARAM)
	{
		switch( LOWORD(wParam) )
		{
			case ID_PANE_VOLUME:
				ChangeRightPane(ID_PANE_VOLUME);
				break;
			case ID_PANE_EXTENT:
				ChangeRightPane(ID_PANE_EXTENT);
				break;
		}
		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		HWND hwndLV = NULL;
		if( pt.x == -1 && pt.y == -1 )
		{
			hwndLV = GetFocus();
			if( m_hWndList != hwndLV && m_hwndPropLV != hwndLV && m_hwndVolumeLV != hwndLV && m_hwndExtentLV != hwndLV)
				return 0;
		}
		else
		{
			RECT rc;
			GetWindowRect(m_hWndList,&rc);
			if( PtInRect(&rc,pt) )
			{
				hwndLV = m_hWndList;
			}
			else
			{
				GetWindowRect(m_hwndPropLV,&rc);
				if( PtInRect(&rc,pt) )
				{
					hwndLV = m_hwndPropLV;
				}
				else
				{
					if( m_hwndCurVolExtLV == m_hwndVolumeLV )
					{
						GetWindowRect(m_hwndVolumeLV,&rc);
						if( PtInRect(&rc,pt) )
						{
							hwndLV = m_hwndVolumeLV;
						}
					}
					else if( m_hwndCurVolExtLV == m_hwndExtentLV )
					{
						GetWindowRect(m_hwndExtentLV,&rc);
						if( PtInRect(&rc,pt) )
						{
							hwndLV = m_hwndExtentLV;
						}
					}
				}
			}
		}

		if( hwndLV == NULL )
			return 0;

		int iItem = ListViewEx_GetCurSel(hwndLV);
		if( iItem == -1 )
			return 0;

		CVDSDiskItem *pItem = NULL;

		if( hwndLV == m_hWndList )
		{
			pItem = (CVDSDiskItem *)ListViewEx_GetItemData(hwndLV,iItem);
		}

		HMENU hMenu = CreatePopupMenu();
		{
			if( hwndLV == m_hwndPropLV || hwndLV == m_hwndVolumeLV || hwndLV == m_hwndExtentLV )
				AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY_VALUE,L"&Copy Value");
			AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"Copy &Line");
			SetMenuDefaultItem(hMenu,ID_VOLUMEINFORMATION,FALSE);
		}

		ListViewEx_SimpleContextMenuHandler(NULL,hwndLV,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	LRESULT OnQueryMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
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
		m_hwndFocusCtrl = pnmhdr->hwndFrom;

		if( IsXpThemeEnabled() )
		{
			SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
			SendMessage(m_hwndPropLV,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
			SendMessage(m_hwndVolumeLV,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
			SendMessage(m_hwndExtentLV,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
		}
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom   = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	LRESULT OnCustomDraw(NMHDR *pnmhdr)
	{
		LRESULT lResult = 0;

		NMLVCUSTOMDRAW* pcd = (NMLVCUSTOMDRAW* )pnmhdr;

		if( pcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			CVDSItem *pParam = (CVDSItem *)pcd->nmcd.lItemlParam;
			return CDRF_NOTIFYPOSTPAINT;
		}

		if( pcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT )
		{
			CVDSDiskItem *pParam = (CVDSDiskItem *)pcd->nmcd.lItemlParam;
			if( IsXpThemeEnabled() )
			{
				UINT State = ListView_GetItemState(pnmhdr->hwndFrom,(int)pcd->nmcd.dwItemSpec,LVIS_FOCUSED);
				if( State & LVIS_FOCUSED )
				{
					VDSDrawFocusFrame(pnmhdr->hwndFrom,pcd->nmcd.hdc,&pcd->nmcd.rc);
				}
			}
		}

		return CDRF_DODEFAULT;
	}

	VOID VDSDrawFocusFrame(HWND hWnd,HDC hdc,RECT *prc,BOOL bDrawFocus=FALSE)
	{
		const COLORREF crActiveFrame = RGB(80,110,190);
		const COLORREF crDarkModeActiveFrame = RGB(96,205,255);
		DrawFocusFrameEx(hWnd,hdc,prc,bDrawFocus,
			_IsDarkModeEnabled() ? crDarkModeActiveFrame : crActiveFrame,DFFEXF_ADJUSTGRIDLINE,nullptr);
	}

	HFONT GetFont()
	{
		if( m_hFont )
			return m_hFont;
		return (HFONT)SendMessage(m_hWndList,WM_GETFONT,0,0);
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		if( pnmia->iItem != -1 )
		{
			CVDSDiskItem *pItem = (CVDSDiskItem *)ListViewEx_GetItemData(m_hWndList,pnmia->iItem);
		}
		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CVDSItem *pItem = (CVDSItem *)pnmlv->lParam;

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
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = &pDiskItem->Prop.pwszName[4];
		return 0;
	}

	LRESULT OnDisp_DiskAddress(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pDiskItem->Prop.pwszDiskAddress;
		return 0;
	}

	LRESULT OnDisp_FriendlyName(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pDiskItem->Prop.pwszFriendlyName;
		return 0;
	}

	LRESULT OnDisp_AdaptorName(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pDiskItem->Prop.pwszAdaptorName;
		return 0;
	}

	LRESULT OnDisp_DevicePath(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pDiskItem->Prop.pwszDevicePath;
		return 0;
	}

	LRESULT OnDisp_PartitionStyle(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = (LPWSTR)VDS_GetPartitionStyleText(pDiskItem->Prop.PartitionStyle);
		return 0;
	}

	LRESULT OnDisp_Status(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = (LPWSTR)VDS_GetStatusText(pDiskItem->Prop.status);

		return 0;
	}

	LRESULT OnDisp_Size(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;

		if( pDiskItem->Prop.status == VDS_DS_ONLINE )
		{
			if( 1 )
			{
				StrFormatByteSizeW(pDiskItem->Prop.ullSize,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
			}
			else
			{
				_CommaFormatString(pDiskItem->Prop.ullSize,pnmlvdi->item.pszText);
			}
		}

		return 0;
	}

	LRESULT OnDisp_BusType(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CVDSDiskItem *pDiskItem = (CVDSDiskItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = (LPWSTR)VDS_GetBusTypeText(pDiskItem->Prop.BusType);
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CVDSDisksPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_VDS_Name,           &CVDSDisksPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_DiskAddress,    &CVDSDisksPage::OnDisp_DiskAddress),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_FriendlyName,   &CVDSDisksPage::OnDisp_FriendlyName),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_AdaptorName,    &CVDSDisksPage::OnDisp_AdaptorName),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_DevicePath,     &CVDSDisksPage::OnDisp_DevicePath),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_PartitionStyle, &CVDSDisksPage::OnDisp_PartitionStyle),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_Status,         &CVDSDisksPage::OnDisp_Status),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_Size,           &CVDSDisksPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_VDS_BusType,        &CVDSDisksPage::OnDisp_BusType),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CVDSDisksPage>[COLUMN_VDS_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CVDSDisksPage>) * COLUMN_VDS_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CVDSItem *pItem = (CVDSItem *)pdi->item.lParam;

		int id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( m_disp_proc == NULL )
		{
			InitColumnTable();	
		}

		if( pdi->item.mask & LVIF_TEXT )
		{
			if( (id < COLUMN_VDS_MaxItem) && m_disp_proc[ id ].pfn )
			{
				return (this->*m_disp_proc[ id ].pfn)(id,pdi);
			}
		}
		return 0;	
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( pnmlv->hdr.idFrom == IDW_LIST )
		{
			if( ((pnmlv->uOldState == 0)|| ((pnmlv->uNewState & (LVIS_SELECTED)) == (LVIS_SELECTED))) 
				&& ((pnmlv->uNewState & (LVIS_SELECTED|LVIS_FOCUSED)) == (LVIS_SELECTED|LVIS_FOCUSED)) )
			{
				CVDSDiskItem *pItem = (CVDSDiskItem *)ListViewEx_GetItemData(m_hWndList,pnmlv->iItem);
	
				SetDiskInfo(pItem);

				LookupVolumes(pItem);
	
				FillExtents(pItem);
			}
		}
		return 0;
	}

	void FillBack(HDC hdc)
	{
		COLORREF cr;
		if( _IsDarkModeEnabled() )
			cr = RGB(76,76,76);
		else
			cr = RGB(243,243,243);
		HBRUSH hbr = CreateSolidBrush( cr );
		RECT rc;
		GetClientRect(m_hWnd,&rc);
		FillRect(hdc,&rc,hbr);
		DeleteObject(hbr);
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SETFOCUS:
			{
				if( m_hwndFocusCtrl )
					SetFocus(m_hwndFocusCtrl);
				else
					SetFocus(m_hWndList);
				return 0;
			}
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd,&ps);
				FillBack(hdc);
				EndPaint(hWnd,&ps);
				return 0;
			}
			case WM_ERASEBKGND:
			{
				FillBack((HDC)wParam);
				return 1;
			}
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hWnd,uMsg,wParam,lParam);
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
		int cySplit = cy/2;
		int cxSplit = _DPI_Adjust_X(410);
		if( m_hWndList )
		{
			SetWindowPos(m_hWndList,NULL,0,0,cx-cxSplit,cySplit,SWP_NOZORDER|SWP_NOCOPYBITS);
		}

		if( m_hwndPropLV )
		{
			SetWindowPos(m_hwndPropLV,NULL,0,cySplit+1,cx-cxSplit,cy-cySplit-1,SWP_NOZORDER|SWP_NOCOPYBITS);

			{
				RECT rc;
				GetClientRect(m_hwndPropLV,&rc);
				int cxValue = ListView_GetColumnWidth(m_hwndPropLV,0);
				ListView_SetColumnWidth(m_hwndPropLV,1,_RECT_WIDTH(rc)-cxValue);
			}
		}

		if( m_hwndCurVolExtLV && m_hwndVolumeLV && m_hwndExtentLV )
		{
			int cytb = (int)HIWORD(SendMessage(m_Toolbar.m_hWnd,TB_GETBUTTONSIZE,0,0));

			SetWindowPos(m_hwndVolumeLV,NULL,cx-cxSplit+1,cytb,cxSplit-1,cy-cytb,SWP_NOZORDER|SWP_NOCOPYBITS);

			{
				RECT rc;
				GetClientRect(m_hwndVolumeLV,&rc);
				int cxValue = ListView_GetColumnWidth(m_hwndVolumeLV,0);
				ListView_SetColumnWidth(m_hwndVolumeLV,1,_RECT_WIDTH(rc)-cxValue);
			}

			SetWindowPos(m_hwndExtentLV,NULL,cx-cxSplit+1,cytb,cxSplit-1,cy-cytb,SWP_NOZORDER|SWP_NOCOPYBITS);

			SetWindowPos(m_Toolbar.m_hWnd,NULL,cx-cxSplit+1,0,cxSplit-1,cytb,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
	}

#define _HSB_HOOKPROC_PROPNAME L"HSBHookProc"

	static LRESULT HideHScrollBarProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
	{
		if( msg == WM_SIZE || msg == WM_WINDOWPOSCHANGING || msg == WM_WINDOWPOSCHANGED )
		{
			LRESULT l = CallWindowProc((WNDPROC)GetProp(hwnd,_HSB_HOOKPROC_PROPNAME), hwnd, msg, wparam, lparam);
			if( (GetWindowLong(hwnd,GWL_STYLE) & WS_VSCROLL) )
			{
				RECT rc;
				int cyAllItems = 0;

				// Header hight
				HWND hwndHD = ListView_GetHeader(hwnd);
				GetClientRect(hwndHD,&rc);
				cyAllItems += _RECT_HIGHT(rc);

				// Items hight
				int cItems = ListView_GetItemCount(hwnd);
				ListView_GetItemRect(hwnd,0,&rc,LVIR_BOUNDS);
				cyAllItems += (_RECT_HIGHT(rc) * cItems);

				// Client area rect
				GetClientRect(hwnd,&rc);

				if( cyAllItems < _RECT_HIGHT(rc) )
				{
					SCROLLINFO si = {0};
					si.cbSize = sizeof(si);
					si.fMask = SIF_POS|SIF_RANGE|SIF_TRACKPOS;
					SetScrollInfo(hwnd,SB_VERT,&si,FALSE);
				}
			}
			if( GetWindowLong(hwnd,GWL_STYLE) & WS_HSCROLL )
			{
				SCROLLINFO si = {0};
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS|SIF_RANGE|SIF_TRACKPOS;
				SetScrollInfo(hwnd,SB_HORZ,&si,FALSE);
			}
			return l;
		}
	    return CallWindowProc((WNDPROC)GetProp(hwnd,_HSB_HOOKPROC_PROPNAME), hwnd, msg, wparam, lparam);
	}

	VOID AttachHookProc(HWND hwnd)
	{
		SetProp(hwnd,_HSB_HOOKPROC_PROPNAME,(HANDLE)GetWindowLongPtr( hwnd, GWLP_WNDPROC  ));
		((WNDPROC)SetWindowLongPtr((hwnd), GWLP_WNDPROC, (LPARAM)(WNDPROC)(HideHScrollBarProc)));
	}

	VOID DetachHookProc(HWND hwnd)
	{
		((WNDPROC)SetWindowLongPtr((hwnd), GWLP_WNDPROC, (LPARAM)(WNDPROC)(GetProp(hwnd,_HSB_HOOKPROC_PROPNAME))));
		RemoveProp(hwnd,_HSB_HOOKPROC_PROPNAME);
	}

	void Init_DiskList()
	{
		m_hwndPropLV = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)IDW_DISKLIST,
                              GetModuleHandle(NULL), 
                              NULL); 

		SendMessage(m_hwndPropLV,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hwndPropLV),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		DWORD dwExLvStyle = LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP;
#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			dwExLvStyle &= ~LVS_EX_GRIDLINES;
#endif
		ListView_SetExtendedListViewStyle(m_hwndPropLV,dwExLvStyle);
		ListView_SetSelectedColumn(m_hwndPropLV,0);

		AttachHookProc(m_hwndPropLV);

		_EnableVisualThemeStyle(m_hwndPropLV);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hwndPropLV);
#endif
		LVCOLUMN lvc = {0};

		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
		lvc.fmt     = LVCFMT_LEFT|LVCFMT_FIXED_RATIO;
		lvc.cx      = DPI_SIZE_CX(140);
		lvc.pszText = L"Item";
		lvc.iOrder  = 0;
		ListView_InsertColumn(m_hwndPropLV,lvc.iOrder,&lvc);

		lvc.pszText = L"Value";
		lvc.fmt     = LVCFMT_LEFT;
		lvc.iOrder  = 1;
		ListView_InsertColumn(m_hwndPropLV,lvc.iOrder,&lvc);
	}

	void Init_VolumeList()
	{
		m_hwndVolumeLV = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)IDW_VOLUMELIST,
                              GetModuleHandle(NULL), 
                              NULL); 

		SendMessage(m_hwndVolumeLV,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hwndVolumeLV),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		DWORD dwExLvStyle = LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP;
#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			dwExLvStyle &= ~LVS_EX_GRIDLINES;
#endif
		ListView_SetExtendedListViewStyle(m_hwndVolumeLV,dwExLvStyle);
		_EnableVisualThemeStyle(m_hwndVolumeLV);
		ListView_SetSelectedColumn(m_hwndVolumeLV,0);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hwndVolumeLV);
#endif
		LVCOLUMN lvc = {0};

		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
		lvc.fmt     = LVCFMT_LEFT|LVCFMT_FIXED_RATIO;
		lvc.cx      = DPI_SIZE_CX(100);
		lvc.pszText = L"Item";
		lvc.iOrder  = 0;
		ListView_InsertColumn(m_hwndVolumeLV,lvc.iOrder,&lvc);

		lvc.pszText = L"Value";
		lvc.iOrder  = 1;
		ListView_InsertColumn(m_hwndVolumeLV,lvc.iOrder,&lvc);
	}

	void Init_ExtentList()
	{
		m_hwndExtentLV = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)IDW_EXTENTLIST,
                              GetModuleHandle(NULL), 
                              NULL); 

		SendMessage(m_hwndExtentLV,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hwndExtentLV),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		DWORD dwExLvStyle = LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP;
#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			dwExLvStyle &= ~LVS_EX_GRIDLINES;
#endif
		ListView_SetExtendedListViewStyle(m_hwndExtentLV,dwExLvStyle);
		_EnableVisualThemeStyle(m_hwndExtentLV);
		ListView_SetSelectedColumn(m_hwndExtentLV,0);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hwndExtentLV);
#endif

		LVCOLUMN lvc = {0};

		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
		lvc.fmt     = LVCFMT_LEFT|LVCFMT_FIXED_RATIO;
		lvc.cx      = DPI_SIZE_CX(100);
		lvc.pszText = L"Item";
		lvc.iOrder  = 0;
		ListView_InsertColumn(m_hwndExtentLV,lvc.iOrder,&lvc);

		lvc.pszText = L"Value";
		lvc.iOrder  = 1;
		ListView_InsertColumn(m_hwndExtentLV,lvc.iOrder,&lvc);
	}

	void ChangeRightPane(int nPane)
	{
		HWND hwndPrev = m_hwndCurVolExtLV;
		HWND hwndNew = NULL;

		switch( nPane )
		{
			case ID_PANE_VOLUME:
				hwndNew = m_hwndVolumeLV;
				break;
			case ID_PANE_EXTENT:
				hwndNew = m_hwndExtentLV;
				break;
			default:
				return;
		}

		EnableWindow(hwndNew,TRUE);
		ShowWindow(hwndNew,SW_SHOW);

		if( hwndPrev )
		{
			ShowWindow(hwndPrev,SW_HIDE);
			EnableWindow(hwndPrev,FALSE);
		}

		m_hwndCurVolExtLV = hwndNew;
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

	BOOL LoadColumns(HWND hWndList,PWSTR psz,ULONG cb)
	{
		COLUMN_TABLE *pcoltbl;
		if( m_columns.LoadUserDefinitionColumnTableFromText(&pcoltbl,psz,cb) == 0)
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

	int InsertDisk(int iItem,CVDSDiskItem *pDisk)
	{
		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pDisk;

		iItem   = ListView_InsertItem(m_hWndList,&lvi);

		return iItem;
	}

	//
	// Enumerate volume items
	//
	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		if( pSel == NULL )
			return E_FAIL;

		CVDSDataManager *pVDB = (CVDSDataManager *)pSel->pszPhysicalDrive;

		if( pVDB == NULL )
		{
			ASSERT(FALSE);
			return E_FAIL;
		}

		SetRedraw(m_hWndList,FALSE);

		BOOL bFirstUpdate = (ListView_GetItemCount(m_hWndList) == 0);

		ListView_DeleteAllItems(m_hWndList);

		int i,c;
		c = pVDB->m_pVdsPacks.GetCount();

		for(i = 0; i < c; i++)
		{
			CVDSPackItem *pPackItem = pVDB->m_pVdsPacks.Get(i);

			int cDisks = pPackItem->Disks.GetCount();
			for(int d = 0; d < cDisks; d++)
			{
				CVDSDiskItem *pDiskItem = pPackItem->Disks.Get(d);
				InsertDisk(d,pDiskItem);
			}
		}

		if( bFirstUpdate )
		{
			int iCols = ListViewEx_GetColumnCount(m_hWndList);
			for(int i = 0; i < iCols; i++)
			{
				ListView_SetColumnWidth(m_hWndList,i,LVSCW_AUTOSIZE_USEHEADER);
			}
			ListViewEx_SetLastColumnWidth(m_hWndList,LVEXCHTF_ADJUST_WIDTH_BY_COLUMN_ITEM_TEXT);
		}

		//
		// Sort in new list.
		//
		DoSort(m_Sort.CurrentSubItem,0);

		//
		// Default selection (if exists).
		//
		SetRedraw(m_hWndList,TRUE);

		RedrawWindow(m_hWndList,NULL,NULL,RDW_UPDATENOW|RDW_INVALIDATE);

		ListViewEx_SetCurSel(m_hWndList,0);

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
	int comp_name(CVDSDiskItem *pItem1,CVDSDiskItem *pItem2, const void *p)
	{
#define __Name  L"\\\\?\\PhysicalDrive" // preference name
		const wchar_t *_NAME = __Name;
		const size_t _cchNAME = ARRAYSIZE(__Name) - 1;
		if( _wcsnicmp(pItem1->Prop.pwszName,_NAME,_cchNAME) == 0 && _wcsnicmp(pItem2->Prop.pwszName,_NAME,_cchNAME) != 0 )
			return -1;
		if( _wcsnicmp(pItem1->Prop.pwszName,_NAME,_cchNAME) != 0 && _wcsnicmp(pItem2->Prop.pwszName,_NAME,_cchNAME) == 0 )
			return 1;
		return StrCmpLogicalW(pItem1->Prop.pwszName,pItem2->Prop.pwszName);
	}

	int comp_friendlyname(CVDSDiskItem *pItem1,CVDSDiskItem *pItem2, const void *p)
	{
		SORT_PARAM<CVDSDisksPage> *op = (SORT_PARAM<CVDSDisksPage> *)p;
		return StrCmp(pItem1->Prop.pwszFriendlyName,pItem2->Prop.pwszFriendlyName);
	}

	int comp_size(CVDSDiskItem *pItem1,CVDSDiskItem *pItem2, const void *p)
	{
		SORT_PARAM<CVDSDisksPage> *op = (SORT_PARAM<CVDSDisksPage> *)p;

		if( pItem1->Prop.status == VDS_DS_ONLINE && pItem2->Prop.status != VDS_DS_ONLINE )
			return (op->direction == 1) ? -1 : 1;
		if( pItem1->Prop.status != VDS_DS_ONLINE && pItem2->Prop.status == VDS_DS_ONLINE )
			return (op->direction == 1) ? 1 : -1;

		return _COMP(pItem1->Prop.ullSize,pItem2->Prop.ullSize);
	}

	int comp_status_text(CVDSDiskItem *pItem1,CVDSDiskItem *pItem2, const void *p)
	{
		SORT_PARAM<CVDSDisksPage> *op = (SORT_PARAM<CVDSDisksPage> *)p;

		PCWSTR pszText1 = VDS_GetStatusText( pItem1->Prop.status );
		PCWSTR pszText2 = VDS_GetStatusText( pItem2->Prop.status );

		return StrCmp(pszText1,pszText2);
	}

	int comp_partitionstyle_text(CVDSDiskItem *pItem1,CVDSDiskItem *pItem2, const void *p)
	{
		SORT_PARAM<CVDSDisksPage> *op = (SORT_PARAM<CVDSDisksPage> *)p;

		PCWSTR pszText1 = VDS_GetPartitionStyleText( pItem1->Prop.PartitionStyle );
		PCWSTR pszText2 = VDS_GetPartitionStyleText( pItem2->Prop.PartitionStyle );

		return StrCmp(pszText1,pszText2);
	}

	int comp_bustype_text(CVDSDiskItem *pItem1,CVDSDiskItem *pItem2, const void *p)
	{
		SORT_PARAM<CVDSDisksPage> *op = (SORT_PARAM<CVDSDisksPage> *)p;

		PCWSTR pszText1 = VDS_GetBusTypeText( pItem1->Prop.BusType );
		PCWSTR pszText2 = VDS_GetBusTypeText( pItem2->Prop.BusType );

		return StrCmp(pszText1,pszText2);
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CVDSDisksPage,CVDSDiskItem> comp_proc[] = 
		{
			{COLUMN_VDS_Name,           &CVDSDisksPage::comp_name},
			{COLUMN_VDS_FriendlyName,   &CVDSDisksPage::comp_friendlyname},
			{COLUMN_VDS_Size,           &CVDSDisksPage::comp_size},
			{COLUMN_VDS_Status,         &CVDSDisksPage::comp_status_text},
			{COLUMN_VDS_PartitionStyle, &CVDSDisksPage::comp_partitionstyle_text},
			{COLUMN_VDS_BusType,        &CVDSDisksPage::comp_bustype_text},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CVDSDisksPage,CVDSDiskItem>[COLUMN_VDS_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CVDSDisksPage,CVDSDiskItem>)*COLUMN_VDS_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid  = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc   = comp_proc[i].proc;
		}
	}

	int CompareItem(CVDSDiskItem *pItem1,CVDSDiskItem *pItem2,SORT_PARAM<CVDSDisksPage> *op)
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
		CVDSDiskItem *pItem1 = (CVDSDiskItem *)lParam1;
		CVDSDiskItem *pItem2 = (CVDSDiskItem *)lParam2;
		SORT_PARAM<CVDSDisksPage> *op = (SORT_PARAM<CVDSDisksPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,CVDSItem *)
	{
		SORT_PARAM<CVDSDisksPage> op = {0};
		op.pThis = this;
		op.id = id;
		op.direction = m_Sort.Direction; // 1 or -1 do not use 0
		ListView_SortItems(m_hWndList,CompareProc,&op);
	}

	//
	// Commaand Handling
	//

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		HWND hwndLV = GetFocus();
		switch( CmdId )
		{
			case ID_EDIT_COPY:
			case ID_EDIT_COPY_VALUE:
				*State = ListView_GetSelectedCount(hwndLV) ? UPDUI_ENABLED : UPDUI_DISABLED;
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
				OnCmdEditCopy(0);
				break;
			case ID_EDIT_COPY_VALUE:
				OnCmdEditCopy(1);
				break;
		}
		return S_OK;
	}

	void OnCmdEditCopy(int nCopyColumn)
	{
		HWND hwndLV = GetFocus();
		if( nCopyColumn == 1 )
		{
			SetClipboardTextFromListViewColumn(hwndLV,SCTEXT_FORMAT_SELECTONLY,1);
		}
		else
		{
			SetClipboardTextFromListView(hwndLV,SEPCHAR_TAB); // SEPCHAR_COMMA);
		}
	}

	void OnCmdRefresh()
	{
		SELECT_ITEM sel = {0};
		FillItems(&sel);
	}

	CVDSItem *GetCurItem(int *piItem=NULL)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( piItem )
			*piItem = iItem;
		if( iItem == -1 )
			return NULL;
		return (CVDSItem *)ListViewEx_GetItemData(m_hWndList,iItem);
	}

	void InvalidateListItem(int iItem)
	{
		int i,cColumns;
		cColumns = ListViewEx_GetColumnCount(m_hWndList);
		for(i = 0; i < cColumns; i++)
			ListView_SetItemText(m_hWndList,iItem,i,LPSTR_TEXTCALLBACK);
	
		LVITEM lvi={0};
		lvi.mask      = LVIF_TEXT|LVIF_IMAGE|LVIF_STATE;
		lvi.iItem     = iItem;
		lvi.iImage    = I_IMAGECALLBACK;
		lvi.state     = 0;
		lvi.stateMask = LVIS_OVERLAYMASK;
		lvi.pszText   = LPSTR_TEXTCALLBACK;
	
		ListView_SetItem(m_hWndList,&lvi);
	
		ListView_RedrawItems(m_hWndList,iItem,iItem);
	}

	int InsertVolumeItemText(int iItem,PWSTR pszName,PWSTR pszValue)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(m_hwndVolumeLV);

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = pszName;
		lvi.lParam  = (LPARAM)0;

		iItem = ListView_InsertItem(m_hwndVolumeLV,&lvi);

		ListView_SetItemText(m_hwndVolumeLV,iItem,1,pszValue);

		return iItem;
	}

	int InsertExtentItemText(int iItem,PWSTR pszName,PWSTR pszValue)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(m_hwndExtentLV);

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = pszName;
		lvi.lParam  = (LPARAM)0;

		iItem = ListView_InsertItem(m_hwndExtentLV,&lvi);

		ListView_SetItemText(m_hwndExtentLV,iItem,1,pszValue);

		return iItem;
	}

	static int CALLBACK ExtentSort(void *p1, void *p2, LPARAM lParam)
	{
		VDS_DISK_EXTENT *pExtentItem1 = (VDS_DISK_EXTENT *)p1;
		VDS_DISK_EXTENT *pExtentItem2 = (VDS_DISK_EXTENT *)p2;

		return _COMP( pExtentItem1->ullOffset, pExtentItem2->ullOffset );
	}

	CVDSVolumeItem *LookupVolumeItem(HDPA hdpaVolume,const GUID& Guid)
	{
		int i,c;

		c = DPA_GetPtrCount(hdpaVolume);

		for(i = 0; i < c; i++)
		{
			CVDSVolumeItem *pVolume = (CVDSVolumeItem *)DPA_GetPtr(hdpaVolume,i);
			if( IsEqualGUID(pVolume->Prop.id,Guid) )
			{
				return pVolume;
			}
		}
		return NULL;
	}

	void LookupVolumes(CVDSDiskItem *pItem)
	{
		WCHAR szBuf[MAX_PATH];

		SetRedraw(m_hwndVolumeLV,FALSE);

		ListView_DeleteAllItems(m_hwndVolumeLV);

		int i,c;
		c = pItem->PackItem->Volumes.GetCount();

		HDPA hdpaVolume = DPA_Create( 256 );
		HDPA hdpaExtent = DPA_Create( 256 );

		for(i = 0; i < c; i++)
		{
			CVDSVolumeItem *pVolume = pItem->PackItem->Volumes.Get(i);

			int cPlex = pVolume->PlexArray.GetCount();
			for( int plex = 0; plex < cPlex; plex++ )
			{
				CVDSVolumeItem::PLEX_EXTENT *pPlexItem = pVolume->PlexArray.GetItemPtr(plex);

				for( long ext = 0; ext < pPlexItem->cExtents; ext++  )
				{
					VDS_DISK_EXTENT &extent = pPlexItem->pExtents[ext];

					if( IsEqualGUID(pItem->Prop.id,extent.diskId) )
					{
						DPA_AppendPtr(hdpaVolume,pVolume);
						break;
					}
				}

				for( long ext = 0; ext < pPlexItem->cExtents; ext++  )
				{
					VDS_DISK_EXTENT &extent = pPlexItem->pExtents[ext];

					if( IsEqualGUID(pItem->Prop.id,extent.diskId) )
					{
						DPA_AppendPtr(hdpaExtent,&pPlexItem->pExtents[ext]);
					}
				}
			}
		}

		DPA_Sort(hdpaExtent,&CVDSDisksPage::ExtentSort,0);

		c = DPA_GetPtrCount(hdpaExtent);

		for(i = 0; i < c; i++)
		{
			VDS_DISK_EXTENT *extent = (VDS_DISK_EXTENT *)DPA_GetPtr(hdpaExtent,i);

			CVDSVolumeItem *pVolume = LookupVolumeItem(hdpaVolume,extent->volumeId);
			if( pVolume )
			{
				InsertVolumeItemText(-1,L"Volume",&pVolume->Prop.pwszName[14]);

				StringCchPrintf(szBuf,MAX_PATH,L"%s",VDS_FileSystemTypeText(pVolume->Prop.RecommendedFileSystemType));
				InsertVolumeItemText(-1,L"File System",szBuf);

				StringCchPrintf(szBuf,MAX_PATH,L"%s",VDS_GetVolumeTypeText(pVolume->Prop.type));
				InsertVolumeItemText(-1,L"Type",szBuf);

				StringCchPrintf(szBuf,MAX_PATH,L"0x%I64X",extent->ullOffset);
				InsertVolumeItemText(-1,L"Offset",szBuf);

				WCHAR szSize[100];
				StrFormatByteSizeW(extent->ullSize,szSize,ARRAYSIZE(szSize));
				StringCchPrintf(szBuf,MAX_PATH,L"0x%I64X (%s)",extent->ullSize,szSize);
				InsertVolumeItemText(-1,L"Size",szBuf);

				StringCchPrintf(szBuf,MAX_PATH,L"%d",extent->type);
				InsertVolumeItemText(-1,L"VDS Volume Id",CGuidToString(extent->volumeId));
				InsertVolumeItemText(-1,L"VDS Plex Id",CGuidToString(extent->plexId));
			}

			InsertVolumeItemText(-1,L"",L"");
		}

		DPA_Destroy(hdpaVolume);
		DPA_Destroy(hdpaExtent);

		ListView_SetColumnWidth(m_hwndVolumeLV,0,LVSCW_AUTOSIZE);

		SetRedraw(m_hwndVolumeLV,TRUE);
	}

	void FillExtents(CVDSDiskItem *pItem)
	{
		WCHAR szBuf[MAX_PATH];

		SetRedraw(m_hwndExtentLV,FALSE);

		ListView_DeleteAllItems(m_hwndExtentLV);

		LONG i,c;
		c = pItem->ExtentsCount;

		for(i = 0; i < c; i++)
		{
			VDS_DISK_EXTENT *extent = &pItem->Extents[i];
			{
				StringCchPrintf(szBuf,MAX_PATH,L"%s",VDS_GetDiskExtentTypeDescription(extent->type,2));
				InsertExtentItemText(-1,L"Type",szBuf);

				StringCchPrintf(szBuf,MAX_PATH,L"0x%I64X",extent->ullOffset);
				InsertExtentItemText(-1,L"Offset",szBuf);

				WCHAR szSize[100];
				StrFormatByteSizeW(extent->ullSize,szSize,ARRAYSIZE(szSize));
				StringCchPrintf(szBuf,MAX_PATH,L"0x%I64X (%s)",extent->ullSize,szSize);
				InsertExtentItemText(-1,L"Size",szBuf);

				StringCchPrintf(szBuf,MAX_PATH,L"%u",extent->memberIdx);
				InsertExtentItemText(-1,L"Member Index",szBuf);

				InsertExtentItemText(-1,L"VDS Volume Id",CGuidToString(extent->volumeId));
				InsertExtentItemText(-1,L"VDS Plex Id",CGuidToString(extent->plexId));
				InsertExtentItemText(-1,L"VDS Disk Id",CGuidToString(extent->diskId));
			}

			InsertExtentItemText(-1,L"",L"");
		}

		ListView_SetColumnWidth(m_hwndExtentLV,0,LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(m_hwndExtentLV,1,LVSCW_AUTOSIZE);

		SetRedraw(m_hwndExtentLV,TRUE);
	}

	int InsertPropItemText(PWSTR pszName,PWSTR pszValue,int iItem=-1)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(m_hwndPropLV);

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = pszName;
		lvi.lParam  = (LPARAM)0;

		iItem = ListView_InsertItem(m_hwndPropLV,&lvi);

		ListView_SetItemText(m_hwndPropLV,iItem,1,pszValue);

		return iItem;
	}

	void SetDiskInfo(CVDSDiskItem *pItem)
	{
		WCHAR szBuf[MAX_PATH];

		SetRedraw(m_hwndPropLV,FALSE);

		DWORD dwPrevStyle = GetWindowLong(m_hwndPropLV,GWL_STYLE);

		ListView_DeleteAllItems(m_hwndPropLV);

		InsertPropItemText(L"Name",pItem->Prop.pwszName);
		InsertPropItemText(L"Friendly Name",pItem->Prop.pwszFriendlyName);
		InsertPropItemText(L"Disk Address",pItem->Prop.pwszDiskAddress);
		InsertPropItemText(L"Adaptor Name",pItem->Prop.pwszAdaptorName);
		InsertPropItemText(L"Device Path",pItem->Prop.pwszDevicePath);

		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%s",VDS_GetBusTypeText(pItem->Prop.BusType));
		InsertPropItemText(L"BusType",szBuf);

		if( VDS_DS_ONLINE == pItem->Prop.status )
		{
			_CommaFormatString(pItem->Prop.ullSize,szBuf);
			InsertPropItemText(L"Size",szBuf);

			_CommaFormatString(pItem->Prop.ulBytesPerSector,szBuf);
			InsertPropItemText(L"Bytes/Sector",szBuf);

			_CommaFormatString(pItem->Prop.ulSectorsPerTrack,szBuf);
			InsertPropItemText(L"Sectors/Track",szBuf);

			_CommaFormatString(pItem->Prop.ulTracksPerCylinder,szBuf);
			InsertPropItemText(L"Tracks/Cylinder",szBuf);

			WCHAR szFlagBuf[MAX_PATH];
			makeDiskFlagString(pItem->Prop.ulFlags,szFlagBuf,ARRAYSIZE(szFlagBuf));

			StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"0x%08X (%s)",pItem->Prop.ulFlags,szFlagBuf);
			InsertPropItemText(L"Flags",szBuf);

			if( VDS_PST_GPT == pItem->Prop.PartitionStyle )
			{
				InsertPropItemText(L"Disk Guid",CGuidToString(pItem->Prop.DiskGuid));
			}
			else
			{
				StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"0x%08X",pItem->Prop.dwSignature);
				InsertPropItemText(L"Signature",szBuf);
			}
		}

		InsertPropItemText(L"VDS Disk Id",CGuidToString(pItem->Prop.id));

		if( pItem->PackItem )
		{
			InsertPropItemText(L"VDS Pack Id",CGuidToString(pItem->PackItem->Prop.id));
			if( pItem->PackItem->Prop.pwszName )
				InsertPropItemText(L"VDS Pack Name",pItem->PackItem->Prop.pwszName);
		}

		SetRedraw(m_hwndPropLV,TRUE);

		DWORD dwNewStyle = GetWindowLong(m_hwndPropLV,GWL_STYLE);
		dwPrevStyle &= WS_VSCROLL;
		dwNewStyle  &= WS_VSCROLL;

		if( dwNewStyle != dwPrevStyle )
		{
			RECT rc;
			GetClientRect(m_hwndPropLV,&rc);

			int cxName = ListView_GetColumnWidth(m_hwndPropLV,0);
			int sc = 0;
			int cxClient = _RECT_WIDTH(rc);
			ListView_SetColumnWidth(m_hwndPropLV,1,cxClient - cxName - sc);
		}
	}

	void makeDiskFlagString(ULONG flags,LPWSTR pszBuffer,int cchBuffer)
	{
		ULONG mask = 0x1;

		*pszBuffer = L'\0';

		for(int i = 0; i < 32; i++,mask <<= 1)
		{
			if( flags & mask )
			{
				PCWSTR pszFlag;
				pszFlag = VDS_GetDiskFlagText((VDS_DISK_FLAG)mask,NULL,0,NULL,0);

				if( *pszBuffer )
					StringCchCat(pszBuffer,cchBuffer,L",");

				StringCchCat(pszBuffer,cchBuffer,pszFlag);
			}
		}
	}
};
