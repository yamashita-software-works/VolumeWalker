#pragma once
//***************************************************************************
//*                                                                         *
//*  page_filterdriver.h                                                    *
//*                                                                         *
//*  Minifilter Driver List Page                                            *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-04-09                                                     *
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
#include "flthelp.h"

#define _ENABLE_GROUP_ICON    0

#define _VOLUME_NAME_LENGTH 256

#define _I_INDENT (1*8+6)

typedef struct _INSTANCE_AGGREGATE_STANDARD_INFORMATION_WIN8 {
  ULONG NextEntryOffset;
  ULONG Flags;
  union {
    struct {
      ULONG               Flags;
      ULONG               FrameID;
      FLT_FILESYSTEM_TYPE VolumeFileSystemType;
      USHORT              InstanceNameLength;
      USHORT              InstanceNameBufferOffset;
      USHORT              AltitudeLength;
      USHORT              AltitudeBufferOffset;
      USHORT              VolumeNameLength;
      USHORT              VolumeNameBufferOffset;
      USHORT              FilterNameLength;
      USHORT              FilterNameBufferOffset;
      ULONG               SupportedFeatures;
    } MiniFilter;
    struct {
      ULONG  Flags;
      USHORT AltitudeLength;
      USHORT AltitudeBufferOffset;
      USHORT VolumeNameLength;
      USHORT VolumeNameBufferOffset;
      USHORT FilterNameLength;
      USHORT FilterNameBufferOffset;
      ULONG  SupportedFeatures;
    } LegacyFilter;
  } Type;
} INSTANCE_AGGREGATE_STANDARD_INFORMATION_WIN8, *PINSTANCE_AGGREGATE_STANDARD_INFORMATION_WIN8;

//
// FLT_MINIFILTERDIVER_INFORMATION
//
typedef struct _FLT_MINIFILTERDIVER_INFORMATION
{
	PWSTR FilterName;
	PWSTR InstanceName;
	PWSTR Altitude;
	PWSTR VolumeName;
	PCWSTR DosDrivePath;
	ULONG Type;
	ULONG Flags;
	ULONG FrameID;
	FLT_FILESYSTEM_TYPE VolumeFileSystemType;
} FLT_MINIFILTERDIVER_INFORMATION;

#if 0
struct CFltMiniFilterItem : public FLT_MINIFILTERDIVER_INFORMATION
{
public:
	CFltMiniFilterItem()
	{
	}
	~CFltMiniFilterItem()
	{
	}
};
#else
typedef FLT_MINIFILTERDIVER_INFORMATION CFltMiniFilterItem;
#endif

class CFilterDriverPage :
	public CPageWndBase,
	public CFindHandler<CFilterDriverPage>
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CFilterDriverPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CFilterDriverPage,CFltMiniFilterItem> *m_comp_proc;

	typedef struct _SORT_COLUMN_DIRECTION {
		int CurrentSubItem;
		int Direction;
	} SORT_COLUMN_DIRECTION;

	SORT_COLUMN_DIRECTION m_Sort;

	HFONT m_hFont;
#if _ENABLE_GROUP_ICON
	SP_CLASSIMAGELIST_DATA m_scd;
#endif

	CColumnList m_columns;

	PWSTR m_pszErrorMessage;

public:
	HWND GetListView() const { return m_hWndList; }

public:
	CFilterDriverPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_pszErrorMessage = NULL;
	}

	~CFilterDriverPage()
	{
		_SafeMemFree(m_pszErrorMessage);

		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	virtual HRESULT OnInitPage(PVOID)
	{
#if _ENABLE_GROUP_ICON
		m_scd.cbSize = sizeof(m_scd);
		SetupDiGetClassImageList(&m_scd);
#endif

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
#if _ENABLE_GROUP_ICON
			ListView_SetImageList(hwndList,m_scd.ImageList,LVSIL_GROUPHEADER);
#endif
			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

			HIMAGELIST himl = ImageList_Create(1,16,ILC_COLOR32,1,1);
			ListView_SetImageList(hwndList,himl,LVSIL_SMALL);

			if( !LoadColumns(hwndList) )
			{
				InitColumns(hwndList);
			}

			ListView_EnableGroupView(hwndList,TRUE);

			SetProp(hwndList,L"SortItem",&m_Sort);

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
		DeleteFilters();

		if( m_hFont )
			DeleteObject(m_hFont);

#if _ENABLE_GROUP_ICON
		SetupDiDestroyClassImageList(&m_scd);
#endif

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
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
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

		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlv->lParam;

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

		int iGroup = (int)ListView_GetFocusedGroup(m_hWndList);
		if( iGroup == -1 )
			SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
		else
			SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);

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
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->FilterName;

		return 0;
	}

	LRESULT OnDisp_InstanceName(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->InstanceName;
		return 0;
	}

	LRESULT OnDisp_Altitude(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->Altitude;
		return 0;
	}

	LRESULT OnDisp_FrameId(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%d",pItem->FrameID);
		return 0;
	}

	LRESULT OnDisp_FileSystemType(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%S",
			GetFilterFileSystemTypeString(pItem->VolumeFileSystemType));
		return 0;
	}

	LRESULT OnDisp_Flags(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%08X",pItem->Flags);
		return 0;
	}

	LRESULT OnDisp_VolumeName(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->VolumeName;
		return 0;
	}

	LRESULT OnDisp_DosDrive(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pnmlvdi->item.lParam;
		if( pItem->DosDrivePath )
			pnmlvdi->item.pszText = (PWSTR)pItem->DosDrivePath;
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CFilterDriverPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_FilterName,          &CFilterDriverPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_FilterInstanceName,  &CFilterDriverPage::OnDisp_InstanceName),
			COL_HANDLER_MAP_DEF(COLUMN_FilterAltitude,      &CFilterDriverPage::OnDisp_Altitude),
			COL_HANDLER_MAP_DEF(COLUMN_FilterFrameId,       &CFilterDriverPage::OnDisp_FrameId),
			COL_HANDLER_MAP_DEF(COLUMN_FileSystemType,      &CFilterDriverPage::OnDisp_FileSystemType),
			COL_HANDLER_MAP_DEF(COLUMN_Flags,               &CFilterDriverPage::OnDisp_Flags),
			COL_HANDLER_MAP_DEF(COLUMN_FilterVolumeName,    &CFilterDriverPage::OnDisp_VolumeName),
			COL_HANDLER_MAP_DEF(COLUMN_Drive,               &CFilterDriverPage::OnDisp_DosDrive),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CFilterDriverPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CFilterDriverPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)pdi->item.lParam;

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

		CFltMiniFilterItem *pItem = (CFltMiniFilterItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		AppendMenu(hMenu,MF_STRING,0,0);
		AppendMenu(hMenu,MF_STRING,ID_VIEW_TOGGLE_GROUPVIEWMODE,L"&Group View Mode");
		AppendMenu(hMenu,MF_STRING,ID_OPEN_URL,L"Open the Website for Filter Altitude Definition");

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
				return CFindHandler<CFilterDriverPage>::OnFindItem(hWnd,uMsg,wParam,lParam);
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
		PWSTR pszVolumeName1 = m_volumes[Group1_ID]->VolumeName;
		PWSTR pszVolumeName2 = m_volumes[Group2_ID]->VolumeName;
		PWSTR pszName1;
		PWSTR pszName2;

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
	}

	static INT CALLBACK LVGroupCompare(INT Group1_ID,INT Group2_ID,VOID *pvData)
	{
		return ((CFilterDriverPage *)pvData)->GroupCompare(Group1_ID,Group2_ID);
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
			{ COLUMN_FilterName,         L"Filter Name",    0, 0, LVCFMT_LEFT },
			{ COLUMN_FilterInstanceName, L"Instance Name",  1, 0, LVCFMT_LEFT },
			{ COLUMN_FilterAltitude,     L"Altitude",       2, 0, LVCFMT_LEFT },
			{ COLUMN_FilterFrameId,      L"Frame ID",       3, 0, LVCFMT_LEFT },
			{ COLUMN_FileSystemType,     L"FileSystem",     4, 0, LVCFMT_LEFT },
			{ COLUMN_FilterVolumeName,   L"Volume",         5, 0, LVCFMT_LEFT },
			{ COLUMN_Drive,              L"DosDrive/Path",  6, 0, LVCFMT_LEFT },
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

		DeleteFilters();

		EnumFilters();

		int iItem = 0;
		int iGroupId = 0;
		WCHAR sz[32768];

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iImage  = I_IMAGENONE;

		for(int i = 0; i < m_volumes.GetCount(); i++)
		{
			if( m_volumes[i]->DosDrivePath )
			{
				if( m_volumes[i]->DosDrivePath[1] == L':' && m_volumes[i]->DosDrivePath[2] == L'\0' )
				{
					SHFILEINFO sfi = {0};
					int iImage = I_IMAGENONE;
					WCHAR szDriveRoot[MAX_PATH];
					UINT fOverlay = 0;
					DWORD dwFileAttributes = 0;
					StringCchPrintf(szDriveRoot,MAX_PATH,L"%s\\",m_volumes[i]->DosDrivePath);
					fOverlay |= (SHGSI_LINKOVERLAY|SHGFI_OVERLAYINDEX);
					SHGetFileInfo(szDriveRoot,dwFileAttributes,&sfi,sizeof(sfi),
							SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_TYPENAME|SHGFI_DISPLAYNAME|fOverlay);
					iImage = sfi.iIcon;
					if( sfi.hIcon != NULL )
						DestroyIcon(sfi.hIcon);
					StringCchPrintf(sz,ARRAYSIZE(sz),L"%s",sfi.szDisplayName);
				}
				else
				{
					StringCchPrintf(sz,ARRAYSIZE(sz),L"%s",m_volumes[i]->DosDrivePath);
				}
			}
			else
			{
				StringCchPrintf(sz,ARRAYSIZE(sz),L"%s",L"");
			}

			InsertGroup(m_hWndList,iGroupId,m_volumes[i]->VolumeName,I_IMAGENONE,FALSE,*sz ? sz : NULL);

			BOOL bGroupView = ListView_IsGroupViewEnabled(m_hWndList);

			lvi.mask     = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_GROUPID|LVIF_INDENT;
			lvi.iImage   = I_IMAGENONE;
			lvi.pszText  = LPSTR_TEXTCALLBACK;
			lvi.iGroupId = iGroupId;
			lvi.iIndent  = bGroupView ? _I_INDENT : 0;

			for(int d = 0; d < m_volumes[i]->Filters.GetCount(); d++)
			{
				lvi.iItem   = iItem++;
				lvi.lParam  = (LPARAM)m_volumes[i]->Filters[d];
				ListView_InsertItem(m_hWndList,&lvi);
			}

			iGroupId++;
		}

		SORT_COLUMN_DIRECTION *Sort = (SORT_COLUMN_DIRECTION*)GetProp(m_hWndList,L"SortItem");
		DoSort(m_hWndList,Sort->CurrentSubItem,0);

		ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE_USEHEADER);

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

	int comp_filtername(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->FilterName,pItem2->FilterName);
	}

	int comp_filterinstancename(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2, const void *p)
	{
		return StrCmp(pItem1->InstanceName,pItem2->InstanceName);
	}

	int comp_frameid(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2, const void *p)
	{
		return _COMP(pItem1->FrameID,pItem2->FrameID);
	}

	int comp_filesystemtype(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2, const void *p)
	{
		return StrCmpIA(GetFilterFileSystemTypeString(pItem1->VolumeFileSystemType),
				GetFilterFileSystemTypeString(pItem2->VolumeFileSystemType));
	}

	int comp_volume(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2, const void *p)
	{
		return StrCmpLogicalW(pItem1->VolumeName,pItem2->VolumeName);
	}

	int comp_altitude(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2, const void *p)
	{
		if( pItem1->Type == FLTFL_IASI_IS_MINIFILTER && pItem2->Type == FLTFL_IASI_IS_LEGACYFILTER )
			return 1;
		else if( pItem1->Type == FLTFL_IASI_IS_LEGACYFILTER && pItem2->Type == FLTFL_IASI_IS_MINIFILTER )
			return -1;
		else
		{
			if( pItem1->Type == FLTFL_IASI_IS_MINIFILTER && pItem2->Type == FLTFL_IASI_IS_MINIFILTER )
			{
				int n1,n2,d1,d2;
				n1 = n2 = d1 = d2 = 0;
				swscanf(pItem1->Altitude,L"%u.%u",&n1,&d1);
				swscanf(pItem2->Altitude,L"%u.%u",&n2,&d2);

				if( n1 != n2 )
					return _COMP(n1,n2);
				else
					return _COMP(d1,d2);
			}
		}
		return 0;
	}

	int comp_dosdrivepath(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2, const void *p)
	{
		if( pItem1->DosDrivePath != NULL && pItem2->DosDrivePath == NULL )
			return -1;
		else if( pItem1->DosDrivePath == NULL && pItem2->DosDrivePath != NULL )
			return 1;
		else if( pItem1->DosDrivePath != NULL && pItem2->DosDrivePath != NULL )
			return StrCmpI(pItem1->DosDrivePath,pItem2->DosDrivePath);
		return 0;
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CFilterDriverPage,CFltMiniFilterItem> comp_proc[] = 
		{
			{0,NULL},
			{COLUMN_FilterName,         &CFilterDriverPage::comp_filtername},
			{COLUMN_FilterInstanceName, &CFilterDriverPage::comp_filterinstancename},
			{COLUMN_FilterAltitude,     &CFilterDriverPage::comp_altitude},
			{COLUMN_FilterFrameId,      &CFilterDriverPage::comp_frameid},
			{COLUMN_FileSystemType,     &CFilterDriverPage::comp_filesystemtype},
			{COLUMN_FilterVolumeName,   &CFilterDriverPage::comp_volume},
			{COLUMN_Drive,              &CFilterDriverPage::comp_dosdrivepath},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CFilterDriverPage,CFltMiniFilterItem>[COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CFilterDriverPage,CFltMiniFilterItem>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc  = comp_proc[i].proc;
		}
	}

	int CompareItem(CFltMiniFilterItem *pItem1,CFltMiniFilterItem *pItem2,SORT_PARAM<CFilterDriverPage> *op)
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
		CFltMiniFilterItem *pItem1 = (CFltMiniFilterItem *)lParam1;
		CFltMiniFilterItem *pItem2 = (CFltMiniFilterItem *)lParam2;
		SORT_PARAM<CFilterDriverPage> *op = (SORT_PARAM<CFilterDriverPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void SortItems(UINT id,SORT_COLUMN_DIRECTION *pSort)
	{
		SORT_PARAM<CFilterDriverPage> op = {0};
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
			case ID_VIEW_TOGGLE_GROUPVIEWMODE:
				OnCmdToggleGroupViewMode();
				break;
			case ID_OPEN_URL:
				OnCmdOpenURL();
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
		}

		SetRedraw(m_hWndList,TRUE);
	}

	void OnCmdOpenURL()
	{
		ShellExecute(m_hWnd,L"open",
			L"https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/allocated-altitudes",
			NULL,NULL,SW_SHOW);
	}

	void OnCmdRefresh()
	{
		SELECT_ITEM sel = {0};
		FillItems(&sel);
	}

	//
	// Minifilter User-Mode Application Functions
	//

	void get_string(PWSTR *String,PVOID pBuffer,USHORT offset,USHORT len)
	{
		UNICODE_STRING us;
		us.Length = len;
		us.MaximumLength = us.Length;
		us.Buffer = (PWSTR)((PBYTE)pBuffer + offset);
		*String = AllocateSzFromUnicodeString(&us);
	}

	HRESULT EnumVolumeInstance(PCWSTR pszVolumeName,PCWSTR pszDosDrivePath,CValArray<FLT_MINIFILTERDIVER_INFORMATION *>*pDriverList)
	{
		HRESULT hr;
		HANDLE hVolumeInstanceFind;
		INSTANCE_INFORMATION_CLASS InfoClass;
		PVOID pBuffer;
	
		InfoClass = InstanceAggregateStandardInformation;
	
		hr = FindFirstVolumeInstance(pszVolumeName,InfoClass,
					(PVOID *)&pBuffer,&hVolumeInstanceFind);
	
		if( hr == S_OK )
		{
			PWSTR psz;
			do
			{
				INSTANCE_AGGREGATE_STANDARD_INFORMATION_WIN8 *piasi 
					= (INSTANCE_AGGREGATE_STANDARD_INFORMATION_WIN8 *)pBuffer;
	
				FLT_MINIFILTERDIVER_INFORMATION *pItem = new FLT_MINIFILTERDIVER_INFORMATION;
	
				ZeroMemory(pItem,sizeof(FLT_MINIFILTERDIVER_INFORMATION));
	
				pItem->Type = piasi->Flags;
	
				if( piasi->Flags == FLTFL_IASI_IS_MINIFILTER )
				{
					get_string(&psz,piasi,
						piasi->Type.MiniFilter.InstanceNameBufferOffset,
						piasi->Type.MiniFilter.InstanceNameLength);
					pItem->InstanceName = psz;
	
					get_string(&psz,piasi,
						piasi->Type.MiniFilter.FilterNameBufferOffset,
						piasi->Type.MiniFilter.FilterNameLength);
					pItem->FilterName = psz;
	
					get_string(&psz,piasi,
						piasi->Type.MiniFilter.AltitudeBufferOffset,
						piasi->Type.MiniFilter.AltitudeLength);
					pItem->Altitude = psz;
	
					get_string(&psz,piasi,
						piasi->Type.MiniFilter.VolumeNameBufferOffset,
						piasi->Type.MiniFilter.VolumeNameLength);
					pItem->VolumeName = psz;
	
					pItem->VolumeFileSystemType = piasi->Type.MiniFilter.VolumeFileSystemType;
					pItem->FrameID = piasi->Type.MiniFilter.FrameID;
					pItem->Flags = piasi->Type.MiniFilter.Flags;
				}
				else if( piasi->Flags == FLTFL_IASI_IS_LEGACYFILTER )
				{
					get_string(&psz,piasi,
						piasi->Type.LegacyFilter.FilterNameBufferOffset,
						piasi->Type.LegacyFilter.FilterNameLength);
					pItem->FilterName = psz;
	
					get_string(&psz,piasi,
						piasi->Type.LegacyFilter.AltitudeBufferOffset,
						piasi->Type.LegacyFilter.AltitudeLength);
					pItem->Altitude = psz;
	
					get_string(&psz,piasi,
						piasi->Type.LegacyFilter.VolumeNameBufferOffset,
						piasi->Type.LegacyFilter.VolumeNameLength);
					pItem->VolumeName = psz;
				}
	
				pItem->DosDrivePath = pszDosDrivePath; // Refers to another structure member.
	
				LocalFree(pBuffer);
	
				pDriverList->Add( pItem );
	
				hr = FindNextVolumeInstance(hVolumeInstanceFind,
								InfoClass,(PVOID *)&pBuffer);
			}
			while( hr == S_OK );
		}

		return hr;
	}

	typedef struct  _FLT_VOLUME_INFORMATION
	{
		PWSTR VolumeName;
		PWSTR DosDrivePath;
		CValArray<FLT_MINIFILTERDIVER_INFORMATION *> Filters;
	} FLT_VOLUME_INFORMATION;
	
	CValArray<FLT_VOLUME_INFORMATION *> m_volumes;
	
	void DeleteFilters()
	{
		int cVolumes = m_volumes.GetCount();
	
		for(int i = 0; i < cVolumes; i++)
		{
			int cFilters = m_volumes[i]->Filters.GetCount();
			for(int j = 0; j < cFilters; j++)
			{
				FLT_MINIFILTERDIVER_INFORMATION *d = m_volumes[i]->Filters[j];
				FreeMemory( d->FilterName );
				FreeMemory( d->InstanceName );
				FreeMemory( d->Altitude );
				FreeMemory( d->VolumeName );
				delete d;
			}
			_SafeMemFree( m_volumes[i]->VolumeName );
			_SafeMemFree( m_volumes[i]->DosDrivePath );
			delete m_volumes[i];
		}
	
		m_volumes.RemoveAll();
	}
	
	int EnumFilters()
	{
		HRESULT hr;
		HANDLE hFilterFind;
		DWORD BytesReturned;
	
		DWORD dwBufferSize = sizeof(FILTER_VOLUME_STANDARD_INFORMATION)
							+ (sizeof(WCHAR) * _VOLUME_NAME_LENGTH);
		FILTER_VOLUME_STANDARD_INFORMATION *lpBuffer =
							(FILTER_VOLUME_STANDARD_INFORMATION *)_MemAlloc(dwBufferSize);
		if( lpBuffer == NULL )
			return -1;
	
		hr = FilterVolumeFindFirst(
					FilterVolumeStandardInformation,
					lpBuffer,
					dwBufferSize,
					&BytesReturned,
					&hFilterFind
					); 
	
		if( hr == S_OK )
		{
			do
			{
				WCHAR szVolumeName[_VOLUME_NAME_LENGTH+1];
				WCHAR szDosDrive[MAX_PATH];
	
				memcpy(szVolumeName,lpBuffer->FilterVolumeName,lpBuffer->FilterVolumeNameLength);
				szVolumeName[ lpBuffer->FilterVolumeNameLength/sizeof(WCHAR) ] = UNICODE_NULL;
	
				if( FilterGetDosName(szVolumeName,szDosDrive,MAX_PATH) != S_OK )
				{
					szDosDrive[0] = 0;
				}
	
				FLT_VOLUME_INFORMATION *pVol = new FLT_VOLUME_INFORMATION;
	
				pVol->VolumeName = _MemAllocString(szVolumeName);
				if( szDosDrive[0] != L'\0' )
					pVol->DosDrivePath = _MemAllocString(szDosDrive); 
				else
					pVol->DosDrivePath = NULL;
	
				EnumVolumeInstance(szVolumeName,pVol->DosDrivePath,&pVol->Filters);
	
				m_volumes.Add( pVol );
	
				hr = FilterVolumeFindNext(
							hFilterFind,
							FilterVolumeStandardInformation,
							lpBuffer,
							dwBufferSize,
							&BytesReturned
							); 
			}
			while( hr == S_OK );
		}
		else
		{
			SetErrorState(hr);
		}
	
		_MemFree(lpBuffer);
	
		return 0;
	}

	void SetErrorState(ULONG Status)
	{
		PWSTR pMessage;
		if( WinGetErrorMessage(Status,&pMessage) > 0 )
		{
			_SafeMemFree(m_pszErrorMessage);
			m_pszErrorMessage = _MemAllocString(pMessage);
			WinFreeErrorMessage(pMessage);
		}
	}
};
