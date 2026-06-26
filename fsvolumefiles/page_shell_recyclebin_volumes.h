#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_shell_recyclebin_volumes.h                                          *
//*                                                                           *
//*  Recycle bin usage by each drives/volumes.                                *
//*                                                                           *
//*  Author: YAMASHITA Katsuhiro                                              *
//*                                                                           *
//*  History: 2026-06-15 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "libntwdk.h"
#include "pagewbdbase.h"
#include "common.h"
#include "column.h"
#include "findhandler.h"
#include "shellhelp.h"
#include "fsvolumefilelist.h"

#define _I_INDENT (8+5)

enum {
	ID_GROUP_RECYCLEBIN=1,
	ID_GROUP_VOLUME,
};

typedef struct _RECYCLE_BIN_ITEM
{
	UINT Type;

	INT64 i64Size;
	INT64 i64NumItems;
	PWSTR DeviceName;
	PWSTR VolumeName;
	PWSTR DrivePaths;
	WCHAR szDrive[4];
} RECYCLE_BIN_ITEM, *PRECYCLE_BIN_ITEM;

struct CRecycleBinItem : public RECYCLE_BIN_ITEM
{
	CRecycleBinItem()
	{
		memset(this,0,sizeof(RECYCLE_BIN_ITEM));
	}
	~CRecycleBinItem()
	{
		_SafeMemFree(DeviceName);
		_SafeMemFree(VolumeName);
		_SafeMemFree(DrivePaths);
	}
};

class CRecycleBinVolumesPage :
	public CPageWndBase,
	public CFindHandler<CRecycleBinVolumesPage>
{
protected:
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CRecycleBinVolumesPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CRecycleBinVolumesPage,CRecycleBinItem> *m_comp_proc;

	typedef COMPARE_HANDLER_PROC_DEF<CRecycleBinVolumesPage,CRecycleBinItem> FILELISTPAGE_COMPARE_HANDLER;
	typedef int (CRecycleBinVolumesPage::*COMPHANDLER)(CRecycleBinItem *p1,CRecycleBinItem *p2, const void *p);

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	CColumnList m_columns;

	BOOL m_bUseShellIcon;

	HFONT m_hFont;
	HFONT m_hFontHeader;
	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_SHELL_RECYCLEBIN; }

public:
	HWND GetListView() { return m_hWndList; }

public:
	CRecycleBinVolumesPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction      = -1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_bUseShellIcon = FALSE;
	}

	virtual ~CRecycleBinVolumesPage()
	{
		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	virtual HRESULT OnInitPage(PVOID ptr,DWORD dwFlags,PVOID)
	{
		SELECT_ITEM *pSelectItem = (SELECT_ITEM *)ptr;

		if( dwFlags & VOLFILES_FLG_USE_SHELL_ICON )
		{
			m_bUseShellIcon = TRUE;
		}

		CStringBuffer columnLayoutString(32768);
		CStringBuffer columnCurrentSort(256);

		if( pSelectItem->Context && pSelectItem->Context->MainApp )
		{
			ILoadViewConfig *pLoadConfig;
			if( pSelectItem->Context->MainApp->QueryInterface( I_LOADCONFIG, (void **)&pLoadConfig ) == S_OK )
			{
				pLoadConfig->ReadValue(GetParent(m_hWnd),L"Columns",columnLayoutString,columnLayoutString.GetBufferSize());
				pLoadConfig->ReadValue(GetParent(m_hWnd),L"CurrentSortColumn",columnCurrentSort,columnCurrentSort.GetBufferSize());
				m_bUseShellIcon = pLoadConfig->ReadValueInt(GetParent(m_hWnd),L"EnableIcon",m_bUseShellIcon);
				pLoadConfig->Release();

				if( columnCurrentSort.IsEmpty() )
					StringCchCopy(columnCurrentSort,columnCurrentSort.GetLength(),L"Name,1");
			}
		}

		InitListView();

		if( !LoadColumns(m_hWndList,columnLayoutString,columnCurrentSort) )
		{
			InsertDefaultColumns(m_hWndList);
		}

		InitGroup();

		ListView_EnableGroupView(m_hWndList,TRUE);

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		UpdateData(pSelectItem);

		return S_OK;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hFontHeader = GetIconFont();
#if 0
		HDC hdc = GetWindowDC(NULL);
		LOGFONT lf = {0};
		SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
		StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Yu Gothic UI");
		m_hFont = CreateFontIndirect( &lf );
		ReleaseDC(NULL,hdc);
#else
		m_hFont = GetGlobalFont(hWnd);
#endif
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SeveConfig();

		if( m_hFont )
		{
			DeleteObject(m_hFont);
			m_hFont = NULL;
		}

		if( m_hFontHeader )
		{
			DeleteObject(m_hFontHeader);
			m_hFontHeader = NULL;
		}

		HIMAGELIST himl = ListView_GetImageList(m_hWndList,LVSIL_SMALL);
		if( himl )
		{
			ImageList_Destroy( himl );
		}

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

		if( pnmhdr->hwndFrom != m_hWndList )
			return 0;

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlvcd->nmcd.lItemlParam;
			return CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT|CDRF_NEWFONT;
		}

		if( pnmlvcd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT|CDDS_SUBITEM) )
		{
			//HWND hwndHeader = ListView_GetHeader(m_hWndList);
			//HDITEM hi;
			//hi.mask = HDI_LPARAM;
			//Header_GetItem(hwndHeader,pnmlvcd->iSubItem,&hi);
			SelectObject(pnmlvcd->nmcd.hdc,m_hFont);
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

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlv->lParam;
		delete pItem;

		return 0;
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( pnmlv->uNewState != pnmlv->uOldState )
		{
			if( pnmlv->uNewState & LVIS_FOCUSED )
			{
				if( ListView_GetItemState(m_hWndList,pnmlv->iItem,LVIS_SELECTED) != 0 )
				{
					CRecycleBinItem *pItem = (CRecycleBinItem *)ListViewEx_GetItemData(pnmhdr->hwndFrom,pnmlv->iItem);
				}
			}
			else
			{
				if( ListView_GetItemState(m_hWndList,pnmlv->iItem,LVIS_FOCUSED|LVIS_SELECTED) == (LVIS_FOCUSED|LVIS_SELECTED) )
				{
					;// Reselet focused item
				}
				else 
				{
					// Clear Select
				}
			}
		}

		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;

		if( pnmia->iItem == -1 )
			return 0;

		CRecycleBinItem *pItem = (CRecycleBinItem *)ListViewEx_GetItemData(m_hWndList,pnmia->iItem);
		if( pItem->i64NumItems > 0 )
		{
			OnRecycleBinFiles();
		}
		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem,TRUE);

		return 0;
	}

	LRESULT OnDisp_VolumeName(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlvdi->item.lParam;

		PWSTR pSel = StrRChrI(pItem->DeviceName,nullptr,L'\\');
		if( pSel )
			pnmlvdi->item.pszText = (++pSel);
		else
			pnmlvdi->item.pszText = pItem->DeviceName;

		return 0;
	}

	LRESULT OnDisp_Drive(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->szDrive;
		return 0;
	}

	LRESULT OnDisp_Guid(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlvdi->item.lParam;
		if( pItem->VolumeName && *pItem->VolumeName && HasPrefix(L"\\\\?\\",pItem->VolumeName) )
		{
			// Skip "\\?\" prefix.
			pnmlvdi->item.pszText = &pItem->VolumeName[4];
			RemoveBackslash(pnmlvdi->item.pszText);
		}
		else
		{
			pnmlvdi->item.pszText = pItem->VolumeName;
		}
		return 0;
	}

	LRESULT OnDisp_TotalSize(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlvdi->item.lParam;
		if( pItem->i64Size )
			_CommaFormatString(pItem->i64Size,pnmlvdi->item.pszText);
		return 0;
	}

	LRESULT OnDisp_TotalCount(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinItem *pItem = (CRecycleBinItem *)pnmlvdi->item.lParam;
		if( pItem->i64NumItems )
			_CommaFormatString(pItem->i64NumItems,pnmlvdi->item.pszText);
		return 0;
	}

	virtual void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CRecycleBinVolumesPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_VolumeName, &CRecycleBinVolumesPage::OnDisp_VolumeName),
			COL_HANDLER_MAP_DEF(COLUMN_Drive,      &CRecycleBinVolumesPage::OnDisp_Drive),
			COL_HANDLER_MAP_DEF(COLUMN_TotalCount, &CRecycleBinVolumesPage::OnDisp_TotalCount),
			COL_HANDLER_MAP_DEF(COLUMN_TotalSize,  &CRecycleBinVolumesPage::OnDisp_TotalSize),
			COL_HANDLER_MAP_DEF(COLUMN_Guid,       &CRecycleBinVolumesPage::OnDisp_Guid),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CRecycleBinVolumesPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CRecycleBinVolumesPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	virtual LRESULT OnGetDispText(int id,NMLVDISPINFO *pdi, CRecycleBinItem *pItem)
	{
		if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
		{
			pdi->item.mask |= LVIF_DI_SETITEM;
			return (this->*m_disp_proc[ id ].pfn)(id,pdi);
		}

		return 0;
	}

	virtual LRESULT OnGetDispImage(int id,NMLVDISPINFO *pdi, CRecycleBinItem *pItem)
	{
		if( m_bUseShellIcon )
		{
			if( *pItem->VolumeName )
			{
				WCHAR szRootDir[] = { pItem->szDrive[0], L':', L'\\', L'\0' };
				pdi->item.iImage = GetShellFileImageListIndex(NULL,szRootDir,FILE_ATTRIBUTE_DIRECTORY);
				if( pdi->item.iImage & 0xff000000 )
				{
					pdi->item.state = INDEXTOOVERLAYMASK(pdi->item.iImage >> 24);
					pdi->item.stateMask = LVIS_OVERLAYMASK;
					pdi->item.mask |= LVIF_STATE;
				}
				pdi->item.iImage = pdi->item.iImage & ~0xFF000000;
			}
			else
			{
				pdi->item.iImage = pItem->i64NumItems > 0 ? 1 : 0;
			}
		}
		else
		{
			pdi->item.iImage = I_IMAGENONE;
		}
		pdi->item.mask |= LVIF_DI_SETITEM;
		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CRecycleBinItem *pItem = (CRecycleBinItem *)pdi->item.lParam;

		int id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( m_disp_proc == NULL )
		{
			InitColumnTable();	
		}

		if( pdi->item.mask & LVIF_IMAGE )
		{
			OnGetDispImage(id,pdi,pItem);
		}

		if( pdi->item.mask & LVIF_TEXT )
		{
			OnGetDispText(id,pdi,pItem);
		}

		return 0;	
	}

	LRESULT OnSaveConfig(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		ISaveViewConfig *psvc = (ISaveViewConfig *)lParam;

		if( psvc == NULL )
		{
			return ERROR_INVALID_PARAMETER;
		}

		// Column Layout String
		{
			PWSTR psz;
			SaveColumns(m_hWndList,&psz);

			psvc->WriteValue( GetParent(m_hWnd), C_KEY_COLUMNLAYOUTSTRING, psz );

			_MemFree(psz);
		}

		// Current Sort Column
		{
			int ColumnId = (int)ListViewEx_GetHeaderItemData( m_hWndList, m_Sort.CurrentSubItem  );

			PCWSTR pColumnName = m_columns.IdToName( ColumnId );

			if( pColumnName )
			{
				WCHAR sz[64];
				StringCchPrintf(sz,ARRAYSIZE(sz),L"%s,%d",pColumnName,m_Sort.Direction==1?0:1);

				psvc->WriteValue( GetParent(m_hWnd), C_KEY_CURRENTSORTCOLUMN, sz );
			}
		}

		return 0;
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

	void InvalidateListColumn(int iColumn,BOOL bImages=FALSE)
	{
		int i,cItems;
	
		cItems =  ListView_GetItemCount(m_hWndList);
	
		for(i = 0; i < cItems; i++)
		{
			ListView_SetItemText(m_hWndList,i,iColumn,LPSTR_TEXTCALLBACK);
	
			if( iColumn == 0)
			{
				LVITEM lvi={0};
				lvi.mask = LVIF_TEXT;
				lvi.iItem = i;
				lvi.pszText = LPSTR_TEXTCALLBACK;
				if( bImages )
				{
					lvi.mask |= LVIF_IMAGE|LVIF_STATE;
					lvi.iImage = I_IMAGECALLBACK;
					lvi.state = 0;
					lvi.stateMask = LVIS_OVERLAYMASK;
				}
				ListView_SetItem(m_hWndList,&lvi);
			}
		}
	
		ListView_RedrawItems(m_hWndList,0,cItems-1);
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
			case PM_SAVECONFIG:
				return OnSaveConfig(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
				return CFindHandler<CRecycleBinVolumesPage>::OnFindItem(hWnd,uMsg,wParam,lParam);;
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	virtual void UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
		{
			int cxList = cx;
			int cyList = cy;

			HDWP hdwp = BeginDeferWindowPos(2);

			DeferWindowPos(hdwp,m_hWndList,NULL,0,0,cxList,cyList,SWP_NOZORDER);

			EndDeferWindowPos(hdwp);
		}
	}

	virtual DWORD DefListViewStyle() const
	{
		return (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_SINGLESEL);
	}

	virtual void InitListView()
	{
		SetupColumnNameSet();

		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              DefListViewStyle(),
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif
		if( m_bUseShellIcon )
		{
			HIMAGELIST himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32,1,1);
			ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);
			ImageList_AddIcon(himl,GetShellStockIcon(SIID_RECYCLER));
			ImageList_AddIcon(himl,GetShellStockIcon(SIID_RECYCLERFULL));
		}
		else
		{
			HIMAGELIST himl = ImageList_Create(1,GetSystemMetrics(SM_CXSMICON),ILC_COLOR32,1,1);
			ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);
		}

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		InitColumnDefinitions();
	}

	typedef struct _GROUP_ITEM
	{
		int idGroup;
		UINT idGroupTitle;
		PCWSTR Text;
	} GROUP_ITEM;

	virtual void InitGroup()
	{
		GROUP_ITEM Group[] = {
			{ ID_GROUP_RECYCLEBIN, 0, L"RecycleBin" },
			{ ID_GROUP_VOLUME,     0, L"Volumes"  },
		};
		int cGroupItem = ARRAYSIZE(Group);

		for(int i = 0; i < cGroupItem; i++)
		{
			InsertGroup(m_hWndList,Group[i].idGroup,Group[i].Text);
		}
	}

	virtual void SetupColumnNameSet()
	{
		static COLUMN_NAME column_name_map[] = {
			{ COLUMN_VolumeName,     L"Volume",          180},
			{ COLUMN_Drive,          L"Drive",            80},
			{ COLUMN_TotalCount,     L"TotalCount",      180},
			{ COLUMN_TotalSize,      L"TotalSize",       180},
			{ COLUMN_Guid,           L"Guid",            340},
		};
		m_columns.SetColumnNameMap( _countof(column_name_map), column_name_map );
	}

	virtual void InitColumnDefinitions()
	{
		static COLUMN def_columns[] = {
			{ COLUMN_VolumeName,     L"Volume",          1, 0, LVCFMT_LEFT },
			{ COLUMN_Drive,          L"Drive",           2, 0, LVCFMT_LEFT },
			{ COLUMN_TotalCount,     L"Total Count",     3, 0, LVCFMT_RIGHT },
			{ COLUMN_TotalSize,      L"Total Size",      4, 0, LVCFMT_RIGHT },
			{ COLUMN_Guid,           L"Guid",            5, 0, LVCFMT_LEFT },
		};

		m_columns.SetDefaultColumns(def_columns,ARRAYSIZE(def_columns));
	}

	void InsertDefaultColumns(HWND hWndList)
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

	virtual BOOL LoadColumns(HWND hWndList,PCWSTR pszColumnLayout,PCWSTR pszCurrentSortColumn)
	{
		COLUMN_TABLE *pcoltbl;

		WCHAR bufDefaultTable[] =
				L"Volume=180;"
				L"Drive=80;"
				L"TotalCount=180;"
				L"TotalSize=180;"
				L"Guid=342\0";

		int cb = sizeof(bufDefaultTable);
		PWSTR ptr;

		if( pszColumnLayout && *pszColumnLayout )
		{
			ptr = ColumnList_ConvertAllocSzToMszColumnString(pszColumnLayout);
			cb  = ColumnList_GetMszColumnStringSizeCb(ptr);
		}
		else
		{
			ptr = ColumnList_ConvertAllocSzToMszColumnString(bufDefaultTable);
			cb  = ColumnList_GetMszColumnStringSizeCb(ptr);
		}

		int iRet = m_columns.LoadUserDefinitionColumnTableFromText(&pcoltbl,ptr,cb);

		ColumnList_FreeString(ptr);

		if( iRet == 0 )
		{
			return FALSE;
		}

		InsertColumns(hWndList,pcoltbl);

		m_columns.FreeUserDefinitionColumnTable(pcoltbl);

		LARGE_INTEGER li;
		PCWSTR pszSortColumn;
		if( pszCurrentSortColumn )
			pszSortColumn = pszCurrentSortColumn;
		else
			pszSortColumn = L"Name,0";

		li = m_columns.GetColumnSortInfoFromText(pszSortColumn);
		if( li.QuadPart != 0 )
		{
			m_Sort.CurrentSubItem = FindSubItemById( li.LowPart );
			m_Sort.Direction = li.HighPart;
		}

		return TRUE;
	}

	BOOL SaveColumns(HWND hWndList,PWSTR *pszRetString=NULL)
	{
		int cColumns = ListViewEx_GetColumnCount(hWndList);
		COLUMN_TABLE *pcoltbl = (COLUMN_TABLE *)_MemAllocZero(sizeof(COLUMN_TABLE) + sizeof(COLUMN) * cColumns);

		pcoltbl->cItems = cColumns;

		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_ORDER;

		ULONG i;
		for(i = 0; i < pcoltbl->cItems; i++)
		{
			ListView_GetColumn(hWndList,i,&lvc);

			pcoltbl->column[i].cx = lvc.cx;
			pcoltbl->column[i].iOrder = lvc.iOrder;
			pcoltbl->column[i].id = (int)ListViewEx_GetHeaderItemData(hWndList,i);
			pcoltbl->column[i].field = 0;
		}

		int ColumnId;
		ColumnId = (int)ListViewEx_GetHeaderItemData( m_hWndList, m_Sort.CurrentSubItem  );

		PWSTR pszColumns = NULL;
		PWSTR pszSortColumn = NULL;
		m_columns.MakeColumnString(pcoltbl,ColumnId,m_Sort.Direction,&pszColumns,&pszSortColumn);

		if( pszRetString )
		{
			*pszRetString = _MemAllocString(pszColumns);
		}

		CoTaskMemFree(pszColumns);
		CoTaskMemFree(pszSortColumn);

		_MemFree(pcoltbl);

		return TRUE;
	}

	virtual HRESULT SeveConfig()
	{
		// NOP
		return S_OK;
	}

	void InsertColumns(HWND hWndList,COLUMN_TABLE *pcoltbl)
	{
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

	void addRecycleBin()
	{
		SHQUERYRBINFO rbi = {0};
		rbi.cbSize = sizeof(SHQUERYRBINFO);
		SHQueryRecycleBin(NULL,&rbi);

		CRecycleBinItem *pItem = new CRecycleBinItem;

		pItem->i64Size = rbi.i64Size;
		pItem->i64NumItems = rbi.i64NumItems;
		pItem->DeviceName = _MemAllocString(L"RecycleBin");
		pItem->VolumeName = _MemAllocString(L"");
		pItem->DrivePaths = _MemAllocString(L"");
		ZeroMemory(pItem->szDrive,sizeof(pItem->szDrive));

		int iItem;
		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE|LVIF_GROUPID|LVIF_INDENT;
		lvi.iItem    = ListView_GetItemCount(m_hWndList);
		lvi.pszText  = LPSTR_TEXTCALLBACK;
		lvi.lParam   = (LPARAM)pItem;
		lvi.iGroupId = ID_GROUP_RECYCLEBIN;
		lvi.iImage  = I_IMAGECALLBACK;
		if( m_bUseShellIcon )
		{
			lvi.iIndent = 1;
		}
		else
		{
			lvi.iIndent = _I_INDENT;
		}
		iItem = ListView_InsertItem(m_hWndList,&lvi);
	}

	void EnumRecycleBin(BOOL bAllVolumes=FALSE)
	{
		DWORD  CharCount            = 0;
		WCHAR  DeviceName[MAX_PATH] = L"";
		DWORD  Error                = ERROR_SUCCESS;
		HANDLE FindHandle           = INVALID_HANDLE_VALUE;
		BOOL   Found                = FALSE;
		size_t Index                = 0;
		BOOL   Success              = FALSE;
		WCHAR  VolumeName[MAX_PATH] = L"";

		FindHandle = FindFirstVolumeW(VolumeName, ARRAYSIZE(VolumeName));

		if( FindHandle == INVALID_HANDLE_VALUE )
		{
			Error = GetLastError();
			return;
		}

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		addRecycleBin();

		for(;;)
		{
	        //  Skip the \\?\ prefix and remove the trailing backslash.
		    Index = wcslen(VolumeName) - 1;

			if (VolumeName[0]     != L'\\' ||
				VolumeName[1]     != L'\\' ||
	            VolumeName[2]     != L'?'  ||
		        VolumeName[3]     != L'\\' ||
			    VolumeName[Index] != L'\\') 
			{
		        Error = ERROR_BAD_PATHNAME;
			    break;
			}

			//  QueryDosDeviceW doesn't allow a trailing backslash,
			//  so temporarily remove it.
			VolumeName[Index] = L'\0';

			CharCount = QueryDosDeviceW(&VolumeName[4], DeviceName, ARRAYSIZE(DeviceName)); 

			VolumeName[Index] = L'\\';

			if ( CharCount == 0 ) 
			{
				Error = GetLastError();
				break;
			}

			WCHAR DrivePaths[260];
			MakeVolumePaths(VolumeName,DrivePaths,260);

			if( bAllVolumes || DrivePaths[0] != L'\0' )
			{
				SHQUERYRBINFO rbi = {0};
				rbi.cbSize = sizeof(SHQUERYRBINFO);
				SHQueryRecycleBin(VolumeName,&rbi);
	
				int iImage;
				SHFILEINFO sfi = {0};
				WCHAR szDrive[4];
				szDrive[0] = DrivePaths[0];
				szDrive[1] = L':';
				szDrive[2] = L'\\';
				szDrive[3] = L'\0';
				if( szDrive[0] != 0 )
				{
					if( SHGetFileInfo(szDrive,0,&sfi,sizeof(sfi),SHGFI_ICON|SHGFI_SMALLICON) != 0 )
					{
						iImage = ImageList_AddIcon(ListView_GetImageList(m_hWndList,LVSIL_SMALL) ,sfi.hIcon);
						DestroyIcon(sfi.hIcon);
					}
					else
					{
						iImage = I_IMAGENONE;
					}
				}
				else
				{
					iImage = I_IMAGENONE;
				}
	
				RECYCLE_BIN_ITEM *pItem = new RECYCLE_BIN_ITEM;
	
				pItem->i64Size = rbi.i64Size;
				pItem->i64NumItems = rbi.i64NumItems;
				pItem->DeviceName = _MemAllocString(DeviceName);
				pItem->VolumeName = _MemAllocString(VolumeName);
				pItem->DrivePaths = _MemAllocString(DrivePaths);
				pItem->szDrive[0] = szDrive[0];
				pItem->szDrive[1] = L':';
				pItem->szDrive[2] = L'\0';
	
				LVITEM lvi = {};
				lvi.mask     = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE|LVIF_GROUPID|LVIF_INDENT;
				lvi.iItem    = ListView_GetItemCount(m_hWndList);
				lvi.pszText  = LPSTR_TEXTCALLBACK;
				lvi.iImage   = iImage;
				lvi.lParam   = (LPARAM)pItem;
				lvi.iGroupId = ID_GROUP_VOLUME;
				lvi.iIndent  = m_bUseShellIcon ? 1 :_I_INDENT;
	
				ListView_InsertItem(m_hWndList,&lvi);
			}

			Success = FindNextVolumeW(FindHandle, VolumeName, ARRAYSIZE(VolumeName));

			if ( !Success ) 
			{
				Error = GetLastError();

				if (Error != ERROR_NO_MORE_FILES) 
				{
					break;
				}

				Error = ERROR_SUCCESS;
				break;
			}
		}

		FindVolumeClose(FindHandle);
		FindHandle = INVALID_HANDLE_VALUE;

		ListViewEx_SetCurSel(m_hWndList,0);

		DoSort(m_Sort.CurrentSubItem,FALSE);

		SetRedraw(m_hWndList,TRUE);
	}

	void MakeVolumePaths(PWCHAR VolumeName,WCHAR *szBuffer,int cchBuffer)
	{
		WCHAR s[4096];
		ZeroMemory(s,sizeof(s));

		DWORD  CharCount = MAX_PATH + 1;
		PWCHAR Names     = NULL;
		PWCHAR NameIdx   = NULL;
		BOOL   Success   = FALSE;

		for (;;) 
		{
			Names = (PWCHAR) new BYTE [CharCount * sizeof(WCHAR)];

			if ( !Names ) 
			{
				return;
			}

	        //  Obtain all of the paths for this volume.
			Success = GetVolumePathNamesForVolumeNameW(
							VolumeName, Names, CharCount, &CharCount);

			if( Success ) 
			{
				break;
			}

			if( GetLastError() != ERROR_MORE_DATA ) 
			{
				break;
			}

	        delete [] Names;
			Names = NULL;
		}

		if ( Success )
		{
			for ( NameIdx = Names; 
				  NameIdx[0] != L'\0'; 
				  NameIdx += wcslen(NameIdx) + 1 ) 
			{
				if( *s )
					StringCchCat(s,ARRAYSIZE(s),L";");
				StringCchCat(s,ARRAYSIZE(s),NameIdx);
				RemoveBackslash(s);
			}
		}

		if( s[0] )
			StringCchCopy(szBuffer,cchBuffer,s);
		else
			*szBuffer = 0;

		if ( Names != NULL ) 
		{
			delete [] Names;
			Names = NULL;
		}

		return;
	}

	virtual HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;
		BOOL bAllVolumes = FALSE;

		if( GetKeyState(VK_SHIFT) < 0 )
			bAllVolumes = TRUE;

		EnumRecycleBin(bAllVolumes);

		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID ptr)
	{
		HRESULT hr;

		SELECT_ITEM *SelectItem = (SELECT_ITEM *)ptr;

		hr = FillItems(SelectItem);

		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Context Menu
	//

	virtual HRESULT MakeContextMenu(HMENU hMenu)
	{
		AppendMenu(hMenu,MF_STRING,ID_EMPTY,L"&Empty");
		AppendMenu(hMenu,MF_STRING,ID_RESTORE,L"&Restore");
		AppendMenu(hMenu,MF_STRING,0,0);
		AppendMenu(hMenu,MF_STRING,ID_RECYCLEBIN_FILES,L"&Files");
		return S_OK;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		HMENU hMenu = CreatePopupMenu();

		SendMessage(GetActiveWindow(),PM_MAKECONTEXTMENU,(WPARAM)hMenu,0);

		CRecycleBinItem *pItem = (CRecycleBinItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		MakeContextMenu(hMenu);

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,0,hMenu,pt,TPM_LEFTALIGN|TPM_TOPALIGN);

		DestroyMenu(hMenu);

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Sort
	//

	int _comp_name(CRecycleBinItem *pItem1,CRecycleBinItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinVolumesPage> *op = (SORT_PARAM<CRecycleBinVolumesPage> *)p;

		if( _wcsnicmp(pItem1->DeviceName,L"\\Device\\HarddiskVolume",22) == 0 && _wcsnicmp(pItem2->DeviceName,L"\\Device\\HarddiskVolume",22) != 0 )
			return -1;
		if( _wcsnicmp(pItem1->DeviceName,L"\\Device\\HarddiskVolume",22) != 0 && _wcsnicmp(pItem2->DeviceName,L"\\Device\\HarddiskVolume",22) == 0 )
			return 1;
		return StrCmpLogicalW(pItem1->DeviceName,pItem2->DeviceName);
	}

	int _comp_drive(CRecycleBinItem *pItem1,CRecycleBinItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinVolumesPage> *op = (SORT_PARAM<CRecycleBinVolumesPage> *)p;
		if( *pItem1->szDrive == 0 && *pItem2->szDrive != 0 )
			return 1 * op->direction;
		else if( *pItem1->szDrive != 0 && *pItem2->szDrive == 0 )
			return -1 * op->direction;
		return wcscmp(pItem1->szDrive,pItem2->szDrive);
	}

	int _comp_total_count(CRecycleBinItem *pItem1,CRecycleBinItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinVolumesPage> *op = (SORT_PARAM<CRecycleBinVolumesPage> *)p;
		if( pItem1->i64NumItems == 0 && pItem2->i64NumItems != 0 )
			return 1 * op->direction;
		else if( pItem1->i64NumItems != 0 && pItem2->i64NumItems == 0 )
			return -1 * op->direction;
		return _COMP(pItem1->i64NumItems,pItem2->i64NumItems);
	}

	int _comp_total_size(CRecycleBinItem *pItem1,CRecycleBinItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinVolumesPage> *op = (SORT_PARAM<CRecycleBinVolumesPage> *)p;
		if( pItem1->i64NumItems == 0 && pItem2->i64NumItems != 0 )
			return 1 * op->direction;
		else if( pItem1->i64NumItems != 0 && pItem2->i64NumItems == 0 )
			return -1 * op->direction;
		return _COMP(pItem1->i64NumItems,pItem2->i64NumItems);
	}

	int _comp_guid(CRecycleBinItem *pItem1,CRecycleBinItem *pItem2, const void *p)
	{
		return StrCmpLogicalW(pItem1->VolumeName,pItem2->VolumeName);
	}

	virtual void init_compare_proc_def_table()
	{
		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CRecycleBinVolumesPage,CRecycleBinItem>[COLUMN_MaxItem];
		ASSERT(m_comp_proc != NULL);

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CRecycleBinVolumesPage,CRecycleBinItem>)*COLUMN_MaxItem);

		static COMPARE_HANDLER_PROC_DEF<CRecycleBinVolumesPage,CRecycleBinItem> _comp_proc[] = 
		{
			{COLUMN_VolumeName,    &CRecycleBinVolumesPage::_comp_name},
			{COLUMN_Drive,         &CRecycleBinVolumesPage::_comp_drive},
			{COLUMN_TotalCount,    &CRecycleBinVolumesPage::_comp_total_count},
			{COLUMN_TotalSize,     &CRecycleBinVolumesPage::_comp_total_size},
			{COLUMN_Guid,          &CRecycleBinVolumesPage::_comp_guid}, 
		};

		for(int i = 0; i < _countof(_comp_proc); i++)
		{
			m_comp_proc[ _comp_proc[i].colid ].colid  = _comp_proc[i].colid;
			m_comp_proc[ _comp_proc[i].colid ].proc   = _comp_proc[i].proc;
		}
	}

	virtual int CompareItem(CRecycleBinItem *pItem1,CRecycleBinItem *pItem2,SORT_PARAM<CRecycleBinVolumesPage> *op)
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
		CRecycleBinItem *pItem1 = (CRecycleBinItem *)lParam1;
		CRecycleBinItem *pItem2 = (CRecycleBinItem *)lParam2;
		SORT_PARAM<CRecycleBinVolumesPage> *op = (SORT_PARAM<CRecycleBinVolumesPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	void DoSort(int iSubItem=-1,BOOL bToggle=FALSE)
	{
		int id = (int)ListViewEx_GetHeaderItemData(m_hWndList,iSubItem);

		if( m_Sort.CurrentSubItem != -1 ) //  previous sort column
			ListViewEx_SetHeaderArrow(m_hWndList,m_Sort.CurrentSubItem,0); // clear previous column header mark

		if( bToggle )
		{
			if( m_Sort.CurrentSubItem != iSubItem )
				m_Sort.Direction = 1; // current column changed: new current column always ascend sort.
			else
				m_Sort.Direction *= -1; // toggle.
		}

		SORT_PARAM<CRecycleBinVolumesPage> op = {0};
		op.pThis           = this;
		op.id              = id;
		op.direction       = m_Sort.Direction; // must 1 or -1, do not use 0
		op.directory_align = 0;                // todo:
		ListView_SortItems(m_hWndList,CompareProc,&op);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Commnad Handler
	//

	virtual HRESULT QueryCmdState(UINT uCmdId,UINT *puState)
	{
		switch( uCmdId )
		{
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*puState = ListView_GetItemCount(m_hWndList) ?  UPDUI_ENABLED : UPDUI_DISABLED;
				break;
			case ID_EDIT_COPY:
				*puState = ListView_GetSelectedCount(m_hWndList) ?  UPDUI_ENABLED : UPDUI_DISABLED;
				break;
			case ID_EMPTY:
			case ID_RESTORE:
				*puState = UPDUI_DISABLED;
				if( ListView_GetSelectedCount(m_hWndList) == 1 )
				{
					RECYCLE_BIN_ITEM *pItem = (RECYCLE_BIN_ITEM *)ListViewEx_GetCurItemData(m_hWndList);
					*puState =  (pItem->i64NumItems != 0) ? UPDUI_ENABLED : UPDUI_DISABLED;
				}
#if 0
				else
				{
					int iItem = -1;
					while( (iItem = ListView_GetNextItem(m_hWndList,iItem,LVNI_SELECTED)) != -1 )
					{
						RECYCLE_BIN_ITEM *pItem = (RECYCLE_BIN_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);
						if( pItem->i64NumItems != 0 )
						{
							*puState =  UPDUI_ENABLED;
							break;
						}
					}
				}
#endif
				break;
			case ID_RECYCLEBIN_FILES:
				*puState = UPDUI_DISABLED;
				if( ListView_GetSelectedCount(m_hWndList) == 1 )
				{
					RECYCLE_BIN_ITEM *pItem = (RECYCLE_BIN_ITEM *)ListViewEx_GetCurItemData(m_hWndList);
					*puState =  (pItem->i64NumItems != 0) ? UPDUI_ENABLED : UPDUI_DISABLED;
				}
				break;
			case ID_VIEW_REFRESH:
				*puState = UPDUI_ENABLED;
				break;
			default:
				return S_FALSE;
		}
		return S_OK;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnEditCopyText();
				break;
			case ID_VIEW_REFRESH:
				OnRefresh();
				break;
			case ID_EMPTY:
				OnEmpty();
				break;
			case ID_RESTORE:
				OnRestore();
				break;
			case ID_RECYCLEBIN_FILES:
				OnRecycleBinFiles();
				break;
			default:
				return S_FALSE;
		}
		return S_OK;
	}

	void OnEditCopy()
	{
	}

	void OnEditCopyText()
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

	void OnRefresh()
	{
		SELECT_ITEM sel = {0};

		FillItems(&sel);
	}

	void UpdateItems()
	{
		int iItem,cItems;

		cItems = ListView_GetItemCount(m_hWndList);

		for(iItem = 0; iItem < cItems; iItem++)
		{
			int iCol = ListViewEx_GetColumnCount(m_hWndList);
			for(int ic = 0; ic < iCol; ic++ )
			{
				ListView_SetItemText(m_hWndList,iItem,ic,LPSTR_TEXTCALLBACK);
			}

			ListView_RedrawItems(m_hWndList,iItem,iItem);
		}
	}

	void OnRecycleBinFiles()
	{
		int iSelItem = ListView_GetNextItem(m_hWndList,-1,LVNI_ALL|LVNI_FOCUSED);
		if( iSelItem == -1 )
			return ;

		RECYCLE_BIN_ITEM *pItem = (RECYCLE_BIN_ITEM *)ListViewEx_GetItemData(m_hWndList,iSelItem);

		if( RecycleBinFilesDialog(m_hWnd,pItem->szDrive,pItem->VolumeName,pItem->DeviceName,0) == S_OK )
		{
			;
		}

		RefreshRecycleBinItem();
	}

	void OnRestore()
	{
		int iSelItem = ListView_GetNextItem(m_hWndList,-1,LVNI_ALL|LVNI_FOCUSED);
		if( iSelItem == -1 )
			return ;

		RECYCLE_BIN_ITEM *pItem = (RECYCLE_BIN_ITEM *)ListViewEx_GetItemData(m_hWndList,iSelItem);

		HRESULT hr;

		hr = RestoreVolumeFiles(pItem->szDrive);

		if( FAILED(hr) )
		{
			;// todo:
		}

		RefreshRecycleBinItem();
	}

	void OnEmpty()
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return;

		RECYCLE_BIN_ITEM *pItem = (RECYCLE_BIN_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

		SHEmptyRecycleBin(GetActiveWindow(),pItem->VolumeName,0);

		RefreshRecycleBinItem();
	}

	void RefreshRecycleBinItem()
	{
		int iItem,cItems;

		cItems = ListView_GetItemCount(m_hWndList);

		for(iItem = 0; iItem < cItems; iItem++)
		{
			RECYCLE_BIN_ITEM *pItem = (RECYCLE_BIN_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

			SHQUERYRBINFO rbi = {0};
			rbi.cbSize = sizeof(SHQUERYRBINFO);
			SHQueryRecycleBin(pItem->VolumeName,&rbi);

			pItem->i64NumItems = rbi.i64NumItems;
			pItem->i64Size = rbi.i64Size;
		}

		UpdateItems();
	}

	HRESULT RestoreVolumeFiles(PCWSTR pszDrive)
	{
		IShellFolder *pFolder = NULL;
		HRESULT hr;

		hr = SHGetDesktopFolder(&pFolder);
		if( hr != S_OK )
		{
			return hr;
		}

	    PIDLIST_ABSOLUTE pidlDesktop = NULL;
		hr = SHGetSpecialFolderLocation(NULL,CSIDL_BITBUCKET,&pidlDesktop);
		if( hr != S_OK )
		{
			pFolder->Release();
			return hr;
		}

		IShellFolder *pBitBucket = NULL;
		hr = pFolder->BindToObject(pidlDesktop,NULL,IID_IShellFolder,(LPVOID *)&pBitBucket);

		if( hr == S_OK )
		{
			IEnumIDList *pEnumIDList = NULL;
			hr = pBitBucket->EnumObjects(NULL,SHCONTF_FOLDERS|SHCONTF_NONFOLDERS,&pEnumIDList);

			if( hr == S_OK )
			{
				if( SUCCEEDED(hr) )
				{
					ULONG ulFetched = 0;
					LPITEMIDLIST pidl;
					WCHAR szManageFilePath[MAX_PATH];

					while( pEnumIDList->Next(1, &pidl, &ulFetched) == S_OK)
					{
						if( _GetShellItemNameW(pBitBucket, pidl, SHGDN_FORPARSING, szManageFilePath, MAX_PATH) )
						{
							if( pszDrive == NULL || *pszDrive == L'\0' || towupper(szManageFilePath[0]) == towupper(pszDrive[0]) )
							{
								hr = _ExecRecycleBinItemCommand(m_hWnd,pidl,"undelete");

								if( hr != S_OK )
								{
									break;
								}
							}
						}

						CoTaskMemFree(pidl);
					}
				}
				pBitBucket->Release();
			}

			pEnumIDList->Release();
		}

		CoTaskMemFree(pidlDesktop);

		pFolder->Release();

		return hr;
	}
};
