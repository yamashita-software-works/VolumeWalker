#pragma once
//***************************************************************************
//*                                                                         *
//*  page_volumemountpoint.h                                                *
//*                                                                         *
//*  Volume Mount Point List Page                                           *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2025-02-18                                                     *
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
#include "..\fsfilelib\fsfilelib.h"

#define _VOLUME_NAME_LENGTH 256

#define _I_INDENT (1*8+6)

class MountPointItem 
{
private:
	PWSTR pszMountPointFileVolume;
	PWSTR pszMountPointFilePath;
	PWSTR pszMountPointFileName;
	PWSTR pszTargetVolumeName;
	PWSTR pszTargetDeviceName;
	PWSTR pszTargetDrive;

public:
	MountPointItem()
	{
		pszMountPointFileVolume = NULL;
		pszMountPointFilePath = NULL;
		pszMountPointFileName = NULL;
		pszTargetVolumeName = NULL;
		pszTargetDeviceName = NULL;
		pszTargetDrive = NULL;
	}

	~MountPointItem()
	{
		_SafeMemFree(pszMountPointFileVolume);
		_SafeMemFree(pszMountPointFilePath);
		_SafeMemFree(pszMountPointFileName);
		_SafeMemFree(pszTargetVolumeName);
		_SafeMemFree(pszTargetDeviceName);
		_SafeMemFree(pszTargetDrive);
	}

	void SetMountPointFilePath(PCWSTR MountPointFilePath)
	{
		pszMountPointFilePath = _MemAllocString(MountPointFilePath);

		WCHAR *pch = wcsrchr(pszMountPointFilePath,L'\\');

		pszMountPointFileName = _MemAllocString( pch ? pch+1 :pszMountPointFilePath );
	}

	void SetMountPointFileVolume(PCWSTR MountPointFileVolume)
	{
		pszMountPointFileVolume = _MemAllocString(MountPointFileVolume);
	}

	void SetTargetVolumeName(PCWSTR TargetVolumeName)
	{
		pszTargetVolumeName = _MemAllocString(TargetVolumeName);
	}

	void SetTargetDeviceName(PCWSTR TargetDeviceName)
	{
		pszTargetDeviceName = _MemAllocString(TargetDeviceName);
	}

	void SetTargetDrive(PCWSTR TargetDrive)
	{
		pszTargetDrive = _MemAllocString(TargetDrive);
	}

	PCWSTR GetMountPointFileName() const
	{
		return pszMountPointFileName;
	}

	PCWSTR GetMountPointFilePath() const
	{
		return pszMountPointFilePath;
	}

	PCWSTR GetMountPointFileVolume() const
	{
		return pszMountPointFileVolume;
	}

	PCWSTR GetTargetVolumeName() const
	{
		return pszTargetVolumeName;
	}

	PCWSTR GetTargetDeviceName() const
	{
		return pszTargetDeviceName;
	}

	PCWSTR GetTargetDrive() const
	{
		return pszTargetDrive;
	}
};

class CVolumeMountPointPage :
	public CPageWndBase,
	public CFindHandler<CVolumeMountPointPage>
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CVolumeMountPointPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CVolumeMountPointPage,MountPointItem> *m_comp_proc;

	typedef struct _SORT_COLUMN_DIRECTION {
		int CurrentSubItem;
		int Direction;
	} SORT_COLUMN_DIRECTION;

	SORT_COLUMN_DIRECTION m_Sort;

	HFONT m_hFont;

	CColumnList m_columns;

	PWSTR m_pszErrorMessage;

	HIMAGELIST m_himl;
	int m_iImageFixedDisk;
	int m_iImageFolder;

public:
	HWND GetListView() const { return m_hWndList; }

public:
	CVolumeMountPointPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_pszErrorMessage = NULL;
		m_himl = NULL;
		m_iImageFixedDisk = I_IMAGENONE;
		m_iImageFolder = I_IMAGENONE;
	}

	~CVolumeMountPointPage()
	{
		_SafeMemFree(m_pszErrorMessage);

		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	virtual HRESULT OnInitPage(PVOID,DWORD,PVOID)
	{
		{ // Block: Create List
			HWND hwndList;

			hwndList = CreateWindowEx(0,WC_LISTVIEW, 
                              L"",
                              WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

			_EnableVisualThemeStyle(hwndList);

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

			HIMAGELIST himl = ImageList_Create(1,16,ILC_COLOR32,1,1);
			ListView_SetImageList(hwndList,himl,LVSIL_SMALL);

			if( !LoadColumns(hwndList) )
			{
				InitColumns(hwndList);
			}

			ListView_EnableGroupView(hwndList,TRUE);

			SetProp(hwndList,L"SortItem",&m_Sort);

#if _ENABLE_DARK_MODE_TEST
			if( _IsDarkModeEnabled() )
				InitDarkModeListView(hwndList);
#endif
			m_himl = ImageList_Create(1,16,ILC_COLOR32,1,1);
			ListView_SetImageList(hwndList,himl,LVSIL_SMALL);

			// Save current list-view handle
			m_hWndList = hwndList;
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		ShowWindow(m_hWndList,SW_SHOW);
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
		if( m_hFont )
			DeleteObject(m_hFont);

		RemoveProp(m_hWndList,L"SortItem");

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
			case LVN_KEYDOWN:
				return OnKeyDown(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		delete (MountPointItem *)pnmlv->lParam;
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
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

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		MountPointItem *pItem = (MountPointItem *)pnmlv->lParam;

		DoSort(pnmlv->hdr.hwndFrom,pnmlv->iSubItem);

		return 0;
	}

	void DoSort(HWND hwndLV,int iSubItem=-1,int iDirection=-1)
	{
		int id = (int)ListViewEx_GetHeaderItemData(hwndLV,iSubItem);

		SORT_COLUMN_DIRECTION& Sort = *(SORT_COLUMN_DIRECTION*)GetProp(hwndLV,L"SortItem");

		if( Sort.CurrentSubItem != -1 )
			ListViewEx_SetHeaderArrow(hwndLV,Sort.CurrentSubItem,0);

		if( iDirection != 0 )
		{
			if( Sort.CurrentSubItem != iSubItem )
				Sort.Direction = 1;
			else
				Sort.Direction *= iDirection;
		}

		SortItems(id,&Sort);

		ListViewEx_SetHeaderArrow(hwndLV,iSubItem,Sort.Direction);

		Sort.CurrentSubItem = iSubItem;
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

		if( IsXpThemeEnabled() )
		{
			int iGroup = (int)ListView_GetFocusedGroup(m_hWndList);
			if( iGroup == -1 )
				SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
			else
				SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);
		}

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
		MountPointItem *pItem = (MountPointItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = (PWSTR)pItem->GetMountPointFileName();
		return 0;
	}

	LRESULT OnDisp_Volume(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		MountPointItem *pItem = (MountPointItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = (PWSTR)pItem->GetTargetVolumeName();
		pnmlvdi->item.pszText = &pnmlvdi->item.pszText[4];
		return 0;
	}

	LRESULT OnDisp_Device(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		MountPointItem *pItem = (MountPointItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = (PWSTR)pItem->GetTargetDeviceName();
		if( *pnmlvdi->item.pszText ) {
			pnmlvdi->item.pszText = wcsrchr(pnmlvdi->item.pszText,L'\\');
			if( pnmlvdi->item.pszText == NULL )
				pnmlvdi->item.pszText = (PWSTR)pItem->GetTargetDeviceName();
			else
				pnmlvdi->item.pszText++;
		}
		return 0;
	}

	LRESULT OnDisp_Drive(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		MountPointItem *pItem = (MountPointItem *)pnmlvdi->item.lParam;
		if( pItem->GetTargetDrive() )
			pnmlvdi->item.pszText = (PWSTR)pItem->GetTargetDrive();
		return 0;
	}

	LRESULT OnDisp_Path(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		MountPointItem *pItem = (MountPointItem *)pnmlvdi->item.lParam;

		if( ListView_IsGroupViewEnabled(m_hWndList) )
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"\\%s",pItem->GetMountPointFilePath());
		}
		else
		{
			WCHAR szDrive[100];
			PCWSTR pszVolume = pItem->GetMountPointFileVolume();
			if( GetVolumeDosName(pszVolume,szDrive,ARRAYSIZE(szDrive)) == S_OK )
			{
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s\\%s",szDrive,pItem->GetMountPointFilePath());
			}
			else
			{
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s\\%s",pszVolume,pItem->GetMountPointFilePath());
			}
		}

		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CVolumeMountPointPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_Name,     &CVolumeMountPointPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_Volume,   &CVolumeMountPointPage::OnDisp_Volume),
			COL_HANDLER_MAP_DEF(COLUMN_Device,   &CVolumeMountPointPage::OnDisp_Device),
			COL_HANDLER_MAP_DEF(COLUMN_Drive,    &CVolumeMountPointPage::OnDisp_Drive),
			COL_HANDLER_MAP_DEF(COLUMN_Path,     &CVolumeMountPointPage::OnDisp_Path),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CVolumeMountPointPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CVolumeMountPointPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		MountPointItem *pItem = (MountPointItem *)pdi->item.lParam;

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

		MountPointItem *pItem = (MountPointItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		AppendMenu(hMenu,MF_STRING,0,0);
		AppendMenu(hMenu,MF_STRING,ID_VIEW_TOGGLE_GROUPVIEWMODE,L"&Group View Mode");

		if( ListView_IsGroupViewEnabled(m_hWndList) )
		{
			CheckMenuItem(hMenu,ID_VIEW_TOGGLE_GROUPVIEWMODE,MF_CHECKED|MF_BYCOMMAND);
		}

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
				return CFindHandler<CVolumeMountPointPage>::OnFindItem(hWnd,uMsg,wParam,lParam);
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

	int GroupCompare(INT Group1_ID,INT Group2_ID)
	{
		LVGROUP lvg = {0};
		lvg.cbSize = sizeof(lvg);

		WCHAR sz1[MAX_PATH];
		WCHAR sz2[MAX_PATH];

		ZeroMemory(sz1,sizeof(sz1));
		ZeroMemory(sz2,sizeof(sz2));

		lvg.mask = LVGF_HEADER|LVGF_GROUPID;
		lvg.iGroupId  = Group1_ID;
		lvg.pszHeader = sz1;
		lvg.cchHeader = MAX_PATH;
		ListView_GetGroupInfo(m_hWndList,Group1_ID,&lvg);

		lvg.iGroupId  = Group2_ID;
		lvg.pszHeader = sz2;
		ListView_GetGroupInfo(m_hWndList,Group2_ID,&lvg);

		PWSTR pszVolumeName1 = sz1;
		PWSTR pszVolumeName2 = sz2;
		PWSTR pszName1;
		PWSTR pszName2;

		if( *pszVolumeName1 == L'\0' && *pszVolumeName2 != L'\0' )
			return 1;

		if( *pszVolumeName1 != L'\0' && *pszVolumeName2 == L'\0' )
			return -1;

		pszName1 = StrRChr(pszVolumeName1,NULL,L'\\');
		if( pszName1 == NULL )
			pszName1 = pszVolumeName1;
		else
			pszName1++;

		pszName2 = StrRChr(pszVolumeName2,NULL,L'\\');
		if( pszName2 == NULL )
			pszName2 = pszVolumeName2;
		else
			pszName2++;

		if( StrCmpNI(pszName1,L"HarddiskVolume",14) == 0 && StrCmpNI(pszName2,L"HarddiskVolume",14) != 0 )
			return -1;
		else if( StrCmpNI(pszName1,L"HarddiskVolume",14) != 0 && StrCmpNI(pszName2,L"HarddiskVolume",14) == 0 )
			return 1;
		else
			return StrCmpLogicalW(pszName1,pszName2);

		return 0;
	}

	static INT CALLBACK LVGroupCompare(INT Group1_ID,INT Group2_ID,VOID *pvData)
	{
		return ((CVolumeMountPointPage *)pvData)->GroupCompare(Group1_ID,Group2_ID);
	}

	int InsertGroup(HWND hWndList,int iGroupId,LPCWSTR pszHeaderText,int iImage=I_IMAGENONE,BOOL fCollapsed=FALSE,LPCWSTR pszSubTitle=NULL)
	{
		LVGROUP group = {0};

		WCHAR szDrive[8];
		WCHAR sz[MAX_PATH];

		if( GetVolumeDosName(pszHeaderText,szDrive,ARRAYSIZE(szDrive)) == S_OK )
		{
			StringCchPrintf(sz,MAX_PATH,L"%s (%s)",pszHeaderText,szDrive);
		}
		else
		{
			StringCchPrintf(sz,MAX_PATH,L"%s",pszHeaderText);
		}

		group.cbSize      = sizeof(LVGROUP);
		group.mask        = LVGF_GROUPID|LVGF_TITLEIMAGE|LVGF_HEADER|LVGF_STATE;
		group.iTitleImage = iImage;
		group.pszHeader   = sz;
		group.uAlign      = LVGA_HEADER_LEFT;
		group.iGroupId    = iGroupId;
		group.state       = LVGS_COLLAPSIBLE | (fCollapsed ? LVGS_COLLAPSED : 0);

		if( pszSubTitle )
		{
			group.mask |= LVGF_SUBTITLE;
			group.pszSubtitle = (LPWSTR)pszSubTitle;
		}

		LVINSERTGROUPSORTED lviSort = {0};
		lviSort.lvGroup = group;
		lviSort.pfnGroupCompare = &LVGroupCompare;
		lviSort.pvData = this;
		return (int)ListView_InsertGroupSorted(hWndList,&lviSort);
	}

	typedef struct _GROUP_ITEM
	{
		int idGroup;
		const GUID *Guid;
		PCWSTR Text;
	} GROUP_ITEM;

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		static COLUMN columns_filelist[] = {
			{ COLUMN_Name,      L"Mount Point Name",        0, 0, LVCFMT_LEFT },
			{ COLUMN_Volume,    L"Mounted Volume",          1, 0, LVCFMT_LEFT },
			{ COLUMN_Device,    L"Mounted Device",          2, 0, LVCFMT_LEFT },
			{ COLUMN_Drive,     L"Mounted Drive",           3, 0, LVCFMT_LEFT },
			{ COLUMN_Path,      L"Mount Point Location",    4, 0, LVCFMT_LEFT },
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

	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);
		ListView_RemoveAllGroups(m_hWndList);

		LoadFltLibDll(NULL);

		VOLUME_NAME_STRING_ARRAY *VolumeNames = NULL;
		EnumVolumeNames( &VolumeNames );

		for(ULONG index = 0; index < VolumeNames->Count; index++)
		{
			EnumVolumeMountPoint(VolumeNames->Volume[index].NtVolumeName,index+1);
		}

		FreeVolumeNames( VolumeNames );

		UnloadFltLibDll(0);


		SORT_COLUMN_DIRECTION *Sort = (SORT_COLUMN_DIRECTION*)GetProp(m_hWndList,L"SortItem");
		DoSort(m_hWndList,Sort->CurrentSubItem,0);

		ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE_USEHEADER);

		if( ListView_GetItemCount(m_hWndList) == 0 )
		{
			PCWSTR pMessage = L"Mount point not found or could not open volume(s).\nTo get full-information, open in administrators mode.";
			SetEmptyMessage(pMessage);
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

	int comp_name(MountPointItem *pItem1,MountPointItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->GetMountPointFileName(),pItem2->GetMountPointFileName());
	}

	int comp_volume(MountPointItem *pItem1,MountPointItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->GetTargetVolumeName(),pItem2->GetTargetVolumeName());
	}

	int comp_device(MountPointItem *pItem1,MountPointItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->GetTargetDeviceName(),pItem2->GetTargetDeviceName());
	}

	int comp_drive(MountPointItem *pItem1,MountPointItem *pItem2, const void *p)
	{
		if( *pItem1->GetTargetDrive() != NULL && *pItem2->GetTargetDrive() == NULL )
			return -1;
		else if( *pItem1->GetTargetDrive() == NULL && *pItem2->GetTargetDrive() != NULL )
			return 1;
		else if( *pItem1->GetTargetDrive() != NULL && *pItem2->GetTargetDrive() != NULL )
			return StrCmpI(pItem1->GetTargetDrive(),pItem2->GetTargetDrive());
		return 0;
	}

	int comp_path(MountPointItem *pItem1,MountPointItem *pItem2, const void *p)
	{
		return 0;//_COMP(pItem1->FrameID,pItem2->FrameID);
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CVolumeMountPointPage,MountPointItem> comp_proc[] = 
		{
			{0,NULL},
			{COLUMN_Name,    &CVolumeMountPointPage::comp_name},
			{COLUMN_Volume,  &CVolumeMountPointPage::comp_volume},
			{COLUMN_Device,  &CVolumeMountPointPage::comp_device},
			{COLUMN_Drive,   &CVolumeMountPointPage::comp_drive},
			{COLUMN_Path,    &CVolumeMountPointPage::comp_path},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CVolumeMountPointPage,MountPointItem>[COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CVolumeMountPointPage,MountPointItem>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc  = comp_proc[i].proc;
		}
	}

	int CompareItem(MountPointItem *pItem1,MountPointItem *pItem2,SORT_PARAM<CVolumeMountPointPage> *op)
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
		MountPointItem *pItem1 = (MountPointItem *)lParam1;
		MountPointItem *pItem2 = (MountPointItem *)lParam2;
		SORT_PARAM<CVolumeMountPointPage> *op = (SORT_PARAM<CVolumeMountPointPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,SORT_COLUMN_DIRECTION *pSort)
	{
		SORT_PARAM<CVolumeMountPointPage> op = {0};
		op.pThis = this;
		op.id = id;
		op.direction = pSort->Direction; // 1 or -1 do not use 0
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
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*State = ListView_GetItemCount(m_hWndList) ?  UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_VIEW_TOGGLE_GROUPVIEWMODE:
				*State = UPDUI_ENABLED;
				if( ListView_IsGroupViewEnabled(m_hWndList) )
					*State |= UPDUI_CHECKED;
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
			case ID_VIEW_TOGGLE_GROUPVIEWMODE:
				OnCmdToggleGroupViewMode();
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

	void OnCmdToggleGroupViewMode()
	{
		SetRedraw(m_hWndList,FALSE);

		BOOL bGroupView = ListView_IsGroupViewEnabled(m_hWndList);

		ListView_EnableGroupView(m_hWndList,bGroupView ? FALSE : TRUE);

		int cItems = ListView_GetItemCount(m_hWndList);

		LVITEM lvi = {0};
		lvi.mask = LVIF_INDENT;
		for(int i = 0; i < cItems; i++)
		{
			lvi.iItem = i;
			lvi.iIndent = bGroupView ? 0 : _I_INDENT;
			ListView_SetItem(m_hWndList,&lvi);

			ListView_SetItemText(m_hWndList,i,0,LPSTR_TEXTCALLBACK);
			ListView_SetItemText(m_hWndList,i,4,LPSTR_TEXTCALLBACK);
		}

		SetRedraw(m_hWndList,TRUE);
	}

	void OnCmdRefresh()
	{
		SELECT_ITEM sel = {0};
		FillItems(&sel);
	}

	void SetErrorState(ULONG Status)
	{
		PWSTR pMessage;
		if( WinGetErrorMessage(Status,&pMessage) > 0 )
		{
			SetEmptyMessage(pMessage);
			WinFreeErrorMessage(pMessage);
		}
	}

	void SetEmptyMessage(PCWSTR psz)
	{
		_SafeMemFree(m_pszErrorMessage);
		m_pszErrorMessage = _MemAllocString(psz);
	}

	int InsertItem(HWND hWndList,int iGroupId,int iItem,int iImage, MountPointItem *pItem)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);

		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM|LVIF_GROUPID;
		lvi.iItem    = iItem;
		lvi.iImage   = iImage;
		lvi.iIndent  = _I_INDENT;
		lvi.lParam   = (LPARAM)pItem;
		lvi.iGroupId = iGroupId;
		lvi.pszText  = (PWSTR)LPSTR_TEXTCALLBACK;

		iItem = ListView_InsertItem(hWndList,&lvi);

		ListView_SetItemText(hWndList,iItem,1,LPSTR_TEXTCALLBACK);
		ListView_SetItemText(hWndList,iItem,2,LPSTR_TEXTCALLBACK);
		ListView_SetItemText(hWndList,iItem,3,LPSTR_TEXTCALLBACK);
		ListView_SetItemText(hWndList,iItem,4,LPSTR_TEXTCALLBACK);

		return iItem;
	}

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

	VOID EnumVolumeMountPoint(PCWSTR pszVolumeName,int iGroupId)
	{
		WCHAR szVolumeName[MAX_PATH];
		WCHAR szWin32VolumeRoot[MAX_PATH];
		WCHAR szVolumeMountPoint[MAX_PATH]; // todo: 32767

		GetNamePart(pszVolumeName,szVolumeName,MAX_PATH);

		StringCchPrintf(szWin32VolumeRoot,MAX_PATH,L"\\\\.\\%s\\",szVolumeName);

		HANDLE h = FindFirstVolumeMountPoint( szWin32VolumeRoot, szVolumeMountPoint, MAX_PATH );

		if( h != INVALID_HANDLE_VALUE )
		{
			InsertGroup(m_hWndList,iGroupId,pszVolumeName,m_iImageFixedDisk);

			do
			{
				RemoveBackslash(szVolumeMountPoint);

				WCHAR szFullPath[32767];
				WCHAR *buf =(WCHAR*)_MemAlloc( 32767 );
				StringCchPrintf(szFullPath,32767,L"%s\\%s",pszVolumeName,szVolumeMountPoint);
				GetReparsePointInformation(NULL,szFullPath,FsReparsePointTargetPath,buf,32767);

				// Volume GUID
				RemoveBackslash(buf);

				// NtDevice/Dos
				WCHAR ntdevice[256];
				WCHAR dosdrive[256];
				ZeroMemory(ntdevice,sizeof(ntdevice));
				ZeroMemory(dosdrive,sizeof(dosdrive));

				if( QueryDosDevice(&buf[4],ntdevice,ARRAYSIZE(ntdevice)) )
				{
					NtPathToDosPath(ntdevice,dosdrive,ARRAYSIZE(dosdrive));
				}

				MountPointItem *pItem = new MountPointItem;

				pItem->SetMountPointFileVolume( pszVolumeName );
				pItem->SetMountPointFilePath( szVolumeMountPoint );
				pItem->SetTargetVolumeName( buf );
				pItem->SetTargetDeviceName( ntdevice );
				pItem->SetTargetDrive( dosdrive );

				InsertItem(m_hWndList,iGroupId,-1,m_iImageFolder,pItem);

				_SafeMemFree(buf);

			} while( FindNextVolumeMountPoint(h,szVolumeMountPoint,MAX_PATH) );
	
			FindVolumeMountPointClose(h);
		}
	}
};
