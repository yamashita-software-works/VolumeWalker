#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_shell_recyclebin_files.h                                            *
//*                                                                           *
//*  Recycle Bin files view.                                                  *
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
#include <stgprop.h>

#define PM_RECYCLEBIN_CHANGED  (WM_APP+1210)

#ifndef _ENABLE_ROW_NAME_INDEPENDENT_FONT
#define _ENABLE_ROW_NAME_INDEPENDENT_FONT  0
#endif

typedef struct _RECYCLEBIN_FILE_ITEM
{
	PWSTR Name;
	PWSTR OriginalPath;
	PWSTR ManagementPath;
	LONGLONG WriteTime;
	LONGLONG CreateTime; // as DeleteTime
	LONGLONG Size;
	DWORD FileAttributes;
	LPITEMIDLIST pidl;
} RECYCLE_BIN_FILE_ITEM, *PRECYCLE_FILE_ITEM;

struct CRecycleBinFileItem : public RECYCLE_BIN_FILE_ITEM
{
	CRecycleBinFileItem()
	{
		memset(this,0,sizeof(RECYCLE_BIN_FILE_ITEM));
	}
};

class CRecycleBinFilesPage :
	public CPageWndBase,
	public CFindHandler<CRecycleBinFilesPage>
{
	BOOL m_bEnableIconImage;
	int m_imgDir;
	int m_imgFile;
protected:
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CRecycleBinFilesPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CRecycleBinFilesPage,CRecycleBinFileItem> *m_comp_proc;

	typedef COMPARE_HANDLER_PROC_DEF<CRecycleBinFilesPage,CRecycleBinFileItem> FILELISTPAGE_COMPARE_HANDLER;
	typedef int (CRecycleBinFilesPage::*COMPHANDLER)(CRecycleBinFileItem *p1,CRecycleBinFileItem *p2, const void *p);

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	CColumnList m_columns;

	BOOL m_bUseShellIcon;

	HFONT m_hFont;
	HFONT m_hFontHeader;
	HFONT m_hFontName;
	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_SHELL_RECYCLEBIN_FILES; }

public:
	HWND GetListView() { return m_hWndList; }

public:
	CRecycleBinFilesPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction      = -1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_hFontName = NULL;

		m_bEnableIconImage = TRUE;
		m_bUseShellIcon = TRUE;
	}

	virtual ~CRecycleBinFilesPage()
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

		InitListView();

		CStringBuffer columnLayoutString(32768);
		CStringBuffer columnCurrentSort(256);

		if( pSelectItem->Context && pSelectItem->Context->MainApp )
		{
			ILoadViewConfig *pLoadConfig;
			if( pSelectItem->Context->MainApp->QueryInterface( I_LOADCONFIG, (void **)&pLoadConfig ) == S_OK )
			{
				pLoadConfig->ReadValue(GetParent(m_hWnd),L"Columns",columnLayoutString,columnLayoutString.GetBufferSize());
				pLoadConfig->ReadValue(GetParent(m_hWnd),L"CurrentSortColumn",columnCurrentSort,columnCurrentSort.GetBufferSize());
				m_bEnableIconImage = pLoadConfig->ReadValueInt(GetParent(m_hWnd),L"EnableIcon",m_bEnableIconImage);
				pLoadConfig->Release();

				if( columnCurrentSort.IsEmpty() )
					StringCchCopy(columnCurrentSort,columnCurrentSort.GetLength(),L"Name,1");
			}
		}
		
		if( !LoadColumns(m_hWndList,columnLayoutString,columnCurrentSort) )
		{
			InsertDefaultColumns(m_hWndList);
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		UpdateData(pSelectItem);

		ListView_SetColumnWidth(m_hWndList,FindSubItemById( COLUMN_Name ),LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(m_hWndList,FindSubItemById( COLUMN_RecycleBin_OriginalLocation ),LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(m_hWndList,FindSubItemById( COLUMN_RecycleBin_ManagementFile ),LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(m_hWndList,FindSubItemById( COLUMN_Size ),LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(m_hWndList,FindSubItemById( COLUMN_RecycleBin_DateDeleted ),LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(m_hWndList,FindSubItemById( COLUMN_RecycleBin_DateModified ),LVSCW_AUTOSIZE);

		return S_OK;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hFont = GetGlobalFont(hWnd);
		m_hFontHeader = GetIconFont();
#if _ENABLE_ROW_NAME_INDEPENDENT_FONT
		HDC hdc = GetWindowDC(NULL);
		LOGFONT lf = {0};
		SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
		StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Yu Gothic UI");
		m_hFontName = CreateFontIndirect( &lf );
		ReleaseDC(NULL,hdc);
#else
		m_hFontName = GetGlobalFont(hWnd);
#endif
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_hFont )
		{
			DeleteObject(m_hFont);
			m_hFont = NULL;
		}

		if( m_hFontName )
		{
			DeleteObject(m_hFontName);
			m_hFontName = NULL;
		}

		if( m_hFontHeader )
		{
			DeleteObject(m_hFontHeader);
			m_hFontHeader = NULL;
		}

		if( m_bUseShellIcon )
		{
			; // do not destroy the image list because using share image list.
		}
		else
		{
			HIMAGELIST himl = ListView_GetImageList(m_hWndList,LVSIL_SMALL);
			if( himl )
			{
				ImageList_Destroy( himl);
			}
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
			case HDN_ENDDRAG:
				return OnHeaderEndDrag(pnmhdr); // return prevent order change
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

		if( IsXpThemeEnabled() )
			SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);

		if( pnmhdr->hwndFrom != m_hWndList )
			return 0;

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlvcd->nmcd.lItemlParam;
#if _ENABLE_DARK_MODE_TEST
			if( _IsDarkModeEnabled() )
			{
				pnmlvcd->clrText = RGB(128,0,80);
			}
			else
#endif
			{
				pnmlvcd->clrText = RGB(0,0,0);
			}

			return CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT|CDRF_NEWFONT;
		}

		if( pnmlvcd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT|CDDS_SUBITEM) )
		{
			HWND hwndHeader = ListView_GetHeader(m_hWndList);
			HDITEM hi;
			hi.mask = HDI_LPARAM;
			Header_GetItem(hwndHeader,pnmlvcd->iSubItem,&hi);

			switch( (int)(hi.lParam) )
			{
				case COLUMN_Name:
				case COLUMN_RecycleBin_OriginalLocation:
				case COLUMN_RecycleBin_ManagementFile:
					SelectObject(pnmlvcd->nmcd.hdc,m_hFontName);
					break;
				default:
					SelectObject(pnmlvcd->nmcd.hdc,m_hFont);
					break;
			}
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

					COLORREF cr;
					BOOL bMultiSel = ( ListView_GetSelectedCount(m_hWndList) > 1 );

#if _ENABLE_DARK_MODE_TEST
					if( _IsDarkModeEnabled() )
					{
						cr = RGB(255,255,0);
					}	
					else
#endif
					{
						if( bMultiSel )
							cr = RGB(255,196,37);
						else
							cr = RGB(80,110,190);
					}

					DrawFocusFrameEx(m_hWndList,pnmlvcd->nmcd.hdc,&rcFocus,FALSE,cr,0,NULL);

					if( bMultiSel )
					{
						rcFocus.top++;
						rcFocus.bottom--;
						DrawFocusFrameEx(m_hWndList,pnmlvcd->nmcd.hdc,&rcFocus,FALSE,cr,0,NULL);
					}
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
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlv->lParam;
		_SafeMemFree(pItem->Name);
		_SafeMemFree(pItem->OriginalPath);
		_SafeMemFree(pItem->ManagementPath);
		ILFree(pItem->pidl);
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
					CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)ListViewEx_GetItemData(pnmhdr->hwndFrom,pnmlv->iItem);
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
					;// Clear Select
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

		/* Reserved */

		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem,TRUE);

		return 0;
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

		SORT_PARAM<CRecycleBinFilesPage> op = {0};
		op.pThis           = this;
		op.id              = id;
		op.direction       = m_Sort.Direction; // must 1 or -1, do not use 0
		op.directory_align = 0;                // todo:
		ListView_SortItems(m_hWndList,CompareProc,&op);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	LRESULT OnDisp_Name(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->Name;
		return 0;
	}

	LRESULT OnDisp_ManagementFile(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->ManagementPath;
		return 0;
	}

	LRESULT OnDisp_OriginalLocation(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->OriginalPath;
		return 0;
	}

	LRESULT OnDisp_DateDeleted(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlvdi->item.lParam;
		_GetDateTimeStringEx2(pItem->CreateTime,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,NULL,NULL,FALSE,FALSE);
		return 0;
	}

	LRESULT OnDisp_DateModified(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlvdi->item.lParam;
		_GetDateTimeStringEx2(pItem->WriteTime,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,NULL,NULL,FALSE,FALSE);
		return 0;
	}

	LRESULT OnDisp_Size(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pnmlvdi->item.lParam;
		_CommaFormatString(pItem->Size,pnmlvdi->item.pszText);
		return 0;
	}

	virtual void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CRecycleBinFilesPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_Name,                        &CRecycleBinFilesPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_RecycleBin_OriginalLocation, &CRecycleBinFilesPage::OnDisp_OriginalLocation),
			COL_HANDLER_MAP_DEF(COLUMN_RecycleBin_DateDeleted,      &CRecycleBinFilesPage::OnDisp_DateDeleted),
			COL_HANDLER_MAP_DEF(COLUMN_RecycleBin_DateModified,     &CRecycleBinFilesPage::OnDisp_DateModified),
			COL_HANDLER_MAP_DEF(COLUMN_Size,                        &CRecycleBinFilesPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_RecycleBin_ManagementFile,   &CRecycleBinFilesPage::OnDisp_ManagementFile),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CRecycleBinFilesPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CRecycleBinFilesPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	virtual LRESULT OnGetDispText(int id,NMLVDISPINFO *pdi, CRecycleBinFileItem *pItem)
	{
		if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
		{
			return (this->*m_disp_proc[ id ].pfn)(id,pdi);
		}
		return 0;
	}

	virtual LRESULT OnGetDispImage(int id,NMLVDISPINFO *pdi, CRecycleBinFileItem *pItem)
	{
		if( m_bUseShellIcon )
		{
			pdi->item.iImage = GetShellFileImageListIndex(NULL,pItem->OriginalPath,pItem->FileAttributes);

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
			if( pItem->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				pdi->item.iImage = m_imgDir;
			}
			else
			{
				pdi->item.iImage = m_imgFile;
			}
		}

		pdi->item.mask |= LVIF_DI_SETITEM;

		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)pdi->item.lParam;

		int col_id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( m_disp_proc == NULL )
		{
			InitColumnTable();	
		}

		if( pdi->item.mask & LVIF_IMAGE )
		{
			OnGetDispImage(col_id,pdi,pItem);
		}

		if( pdi->item.mask & LVIF_TEXT )
		{
			OnGetDispText(col_id,pdi,pItem);
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

	LRESULT OnHeaderEndDrag(NMHDR *pnmhdr)
	{
		int i;

		SetRedraw(m_hWndList,FALSE);
	
		NMHEADER *pHeader = (NMHEADER *)pnmhdr;
	
		int curSortColumnId;
		curSortColumnId = (int)ListViewEx_GetHeaderItemData( m_hWndList, m_Sort.CurrentSubItem  );
	
		HWND hWndHeader = ListView_GetHeader(m_hWndList);
	
		// Forwarding message from ListView's header control.
		LVCOLUMN col = {0};
		col.mask   = LVCF_ORDER;
		col.iOrder = pHeader->pitem->iOrder;
		ListView_SetColumn(m_hWndList,pHeader->iItem,&col);
	
		LVCOLUMN *aOrder = nullptr;
		int *aiOrder     = nullptr;
		LPARAM *aCol     = nullptr;
		LVCOLUMN *pTemp  = nullptr;

		int cColumns = 0;

		__try
		{
			cColumns = Header_GetItemCount( hWndHeader );

			aOrder  = new LVCOLUMN[cColumns];
			aiOrder = new int[cColumns];
			aCol    = new LPARAM[cColumns];
			pTemp   = new LVCOLUMN[cColumns];

			if( aOrder == nullptr|| aiOrder == nullptr || aCol == nullptr || pTemp == nullptr )
			{
				__leave;
			}

			for(i = 0; i < cColumns; i++)
			{
				ZeroMemory(&aOrder[i],sizeof(LVCOLUMN));
				aOrder[i].pszText    = _MemAllocStringBuffer( MAX_PATH );
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

			for(i = 0; i < cColumns; i++)
			{
				pTemp[ aiOrder[i] ] = aOrder[i];
				pTemp[ aiOrder[i] ].iSubItem = (int)aCol[i];
			}

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

			// redraw all list-view items
			SetRedraw(m_hWndList,TRUE);

			int cItems = ListView_GetItemCount(m_hWndList);
			for(i = 0; i < cItems; i++)
			{
				InvalidateListItem(i);
			}

			int iSubItem = FindSubItemById( curSortColumnId );
			m_Sort.CurrentSubItem = iSubItem;
		}
		__finally
		{
			// free memory	
			if( aOrder )
			{
				for(i = 0; i < cColumns; i++)
				{
					_SafeMemFree( aOrder[i].pszText );
				}
				delete[] aOrder;
			}
			if( aiOrder )
				delete[] aiOrder;
			if( aCol )
				delete[] aCol;
			if( pTemp )
				delete[] pTemp;
		}
		// To allow the control to automatically place and reorder the item, return FALSE.
		// To prevent the item from being placed, return TRUE.
		return TRUE;
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

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SETFOCUS:
				SetFocus(m_hWndList);
				if( IsXpThemeEnabled() )
					SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
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
				return CFindHandler<CRecycleBinFilesPage>::OnFindItem(hWnd,uMsg,wParam,lParam);;
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

	virtual void InitListView()
	{
		SetupColumnNameSet();

		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP |
							  LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS),
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
		if( m_bEnableIconImage )
		{
			HIMAGELIST himl;
			if( m_bUseShellIcon )
			{
				himl = GetGlobalShareImageList(0);
			}
			else
			{
				int cxIcon,cyIcon;
				cxIcon = GetSystemMetrics(SM_CXSMICON);
				cyIcon = GetSystemMetrics(SM_CYSMICON);
				himl = ImageList_Create(cxIcon,cyIcon,ILC_COLOR32|ILC_MASK,8, 0);
				m_imgDir  = _ImageList_LoadIcon(himl,IDI_LIST_FOLDER,cxIcon,cyIcon);
				m_imgFile = _ImageList_LoadIcon(himl,IDI_LIST_FILE,cxIcon,cyIcon);
			}
			ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);
		}

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		InitColumnDefinitions();
	}

	virtual void SetupColumnNameSet()
	{
		static COLUMN_NAME column_name_map[] = {
			{ COLUMN_Name,                        L"Name",                0},
			{ COLUMN_RecycleBin_OriginalLocation, L"OriginalLocation",    0},
			{ COLUMN_RecycleBin_DateDeleted,      L"DateDeleted",         0},
			{ COLUMN_RecycleBin_DateModified,     L"DateModified",        0},
		    { COLUMN_RecycleBin_ManagementFile,   L"ManagementFile",      0},
			{ COLUMN_Size,                        L"Size",                0},
		};
		m_columns.SetColumnNameMap( _countof(column_name_map), column_name_map );
	}

	virtual void InitColumnDefinitions()
	{
		static COLUMN def_columns[] = {
			{ COLUMN_Name,                        L"File Name",          1, 200, LVCFMT_LEFT },
			{ COLUMN_RecycleBin_OriginalLocation, L"Original Location",  2, 320, LVCFMT_LEFT },
			{ COLUMN_RecycleBin_DateDeleted,      L"Date Deleted",       3, 120, LVCFMT_LEFT },
			{ COLUMN_RecycleBin_DateModified,     L"Date Modified",      4, 120, LVCFMT_LEFT },
		    { COLUMN_Size,                        L"Size",               5,  80, LVCFMT_RIGHT },
		    { COLUMN_RecycleBin_ManagementFile,   L"Management File",    6, 640, LVCFMT_LEFT },
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
				L"Name=140;"
				L"Size=80;"
				L"ManagementFile=380;"
				L"OriginalLocation=240;"
				L"DateDeleted=120;"
				L"DateModified=120";

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
		PCWSTR pszSortColumn = NULL;
		if( pszCurrentSortColumn && *pszCurrentSortColumn )
			pszSortColumn = pszCurrentSortColumn;
//		else
//			pszSortColumn = L"Name,0";

		if( pszSortColumn && *pszSortColumn )
		{
			li = m_columns.GetColumnSortInfoFromText(pszSortColumn);
			if( li.QuadPart != 0 )
			{
				m_Sort.CurrentSubItem = FindSubItemById( li.LowPart );
				m_Sort.Direction = li.HighPart;
			}
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

	int Insert(HWND hWndList,int iGroupId,int iItem,CFileItemEx *pFI,int iIndent=0,int iImage=I_IMAGECALLBACK)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);

		CRecycleBinFileItem *pItem = new CRecycleBinFileItem;

		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM;
		lvi.iItem    = iItem;
		lvi.iImage   = iImage;
		lvi.iIndent  = iIndent;
		lvi.lParam   = (LPARAM)pItem;
		lvi.pszText  = LPSTR_TEXTCALLBACK;

		if( iGroupId != -1 )
		{
			lvi.mask |= LVIF_GROUPID;
			lvi.iGroupId = iGroupId;
		}

		return ListView_InsertItem(hWndList,&lvi);
	}

	//
	// Enumerate files
	//
	virtual HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		EnumRecycleBinFiles( pSel->pszPath, pSel->pszVolume );

		DoSort(m_Sort.CurrentSubItem,FALSE);

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
		AppendMenu(hMenu,MF_STRING,ID_DELETE,L"&Delete");
		AppendMenu(hMenu,MF_STRING,ID_RESTORE,L"&Restore");
		return S_OK;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		HMENU hMenu = CreatePopupMenu();

		SendMessage(GetActiveWindow(),PM_MAKECONTEXTMENU,(WPARAM)hMenu,0);

		CRecycleBinFileItem *pItem = (CRecycleBinFileItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		MakeContextMenu(hMenu);

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,0,hMenu,pt,TPM_LEFTALIGN|TPM_TOPALIGN);

		DestroyMenu(hMenu);

		return 0;
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

	//////////////////////////////////////////////////////////////////////////
	//
	// Sort
	//

	int _comp_name(CRecycleBinFileItem *pItem1,CRecycleBinFileItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinFilesPage> *op = (SORT_PARAM<CRecycleBinFilesPage> *)p;
		return StrCmpLogicalW(pItem1->Name,pItem2->Name);
	}

	int _comp_original_location(CRecycleBinFileItem *pItem1,CRecycleBinFileItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinFilesPage> *op = (SORT_PARAM<CRecycleBinFilesPage> *)p;
		return StrCmpLogicalW(pItem1->OriginalPath,pItem2->OriginalPath);
	}

	int _comp_date_deleted(CRecycleBinFileItem *pItem1,CRecycleBinFileItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinFilesPage> *op = (SORT_PARAM<CRecycleBinFilesPage> *)p;
		return _COMP(pItem1->CreateTime,pItem2->CreateTime);
	}

	int _comp_date_modified(CRecycleBinFileItem *pItem1,CRecycleBinFileItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinFilesPage> *op = (SORT_PARAM<CRecycleBinFilesPage> *)p;
		return _COMP(pItem1->WriteTime,pItem2->WriteTime);
	}

	int _comp_date_size(CRecycleBinFileItem *pItem1,CRecycleBinFileItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinFilesPage> *op = (SORT_PARAM<CRecycleBinFilesPage> *)p;
		return _COMP(pItem1->Size,pItem2->Size);
	}

	int _comp_managementfile(CRecycleBinFileItem *pItem1,CRecycleBinFileItem *pItem2, const void *p)
	{
		SORT_PARAM<CRecycleBinFilesPage> *op = (SORT_PARAM<CRecycleBinFilesPage> *)p;
		return StrCmpLogicalW(pItem1->ManagementPath,pItem2->ManagementPath);
	}

	virtual void init_compare_proc_def_table()
	{
		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CRecycleBinFilesPage,CRecycleBinFileItem>[COLUMN_MaxItem];
		ASSERT(m_comp_proc != NULL);

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CRecycleBinFilesPage,CRecycleBinFileItem>)*COLUMN_MaxItem);

		static COMPARE_HANDLER_PROC_DEF<CRecycleBinFilesPage,CRecycleBinFileItem> _comp_proc[] = 
		{
			{COLUMN_Name,                        &CRecycleBinFilesPage::_comp_name},
			{COLUMN_RecycleBin_OriginalLocation, &CRecycleBinFilesPage::_comp_original_location},
			{COLUMN_RecycleBin_DateDeleted,      &CRecycleBinFilesPage::_comp_date_deleted},
			{COLUMN_RecycleBin_DateModified,     &CRecycleBinFilesPage::_comp_date_modified},
			{COLUMN_RecycleBin_ManagementFile,   &CRecycleBinFilesPage::_comp_managementfile}, 
			{COLUMN_Size,                        &CRecycleBinFilesPage::_comp_date_size},
		};

		int i;
		for(i = 0; i < _countof(_comp_proc); i++)
		{
			m_comp_proc[ _comp_proc[i].colid ].colid  = _comp_proc[i].colid;
			m_comp_proc[ _comp_proc[i].colid ].proc   = _comp_proc[i].proc;
		}
	}

	virtual int CompareItem(CRecycleBinFileItem *pItem1,CRecycleBinFileItem *pItem2,SORT_PARAM<CRecycleBinFilesPage> *op)
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
		CRecycleBinFileItem *pItem1 = (CRecycleBinFileItem *)lParam1;
		CRecycleBinFileItem *pItem2 = (CRecycleBinFileItem *)lParam2;
		SORT_PARAM<CRecycleBinFilesPage> *op = (SORT_PARAM<CRecycleBinFilesPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Commnad Handler
	//

	virtual HRESULT QueryCmdState(UINT uCmdId,UINT *puState)
	{
		switch( uCmdId )
		{
			case ID_EMPTY:
			case ID_RESTORE:
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*puState = ListView_GetItemCount(m_hWndList) ?  UPDUI_ENABLED : UPDUI_DISABLED;
				break;
			case ID_EDIT_COPY:
				*puState = ListView_GetSelectedCount(m_hWndList) ?  UPDUI_ENABLED : UPDUI_DISABLED;
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
			case ID_RESTORE:
				OnRestore();
				break;
			case ID_DELETE:
				OnDelete();
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

	void OnRestore()
	{
		if( RestoreRecycleBinItem() == S_OK )
		{
			SendMessage(GetParent(m_hWnd),PM_RECYCLEBIN_CHANGED,0,0);
		}
	}

	void OnDelete()
	{
		if( DeleteRecycleBinItem() == S_OK )
		{
			SendMessage(GetParent(m_hWnd),PM_RECYCLEBIN_CHANGED,0,0);
		}
	}

	HRESULT RestoreRecycleBinItem()
	{
		HRESULT hr = E_FAIL;

		int iSelItem = ListView_GetNextItem(m_hWndList,-1,LVNI_SELECTED);
		if( iSelItem == -1 )
			return S_FALSE;

		int iItem;
		iItem = ListView_GetNextItem(m_hWndList,-1,LVNI_SELECTED);
		if( iItem != -1 )
		{
			do
			{
				RECYCLE_BIN_FILE_ITEM *pItem = (RECYCLE_BIN_FILE_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

				hr = _ExecRecycleBinItemCommand(m_hWnd,pItem->pidl,"undelete");

				if( hr == S_OK )
				{
					ListView_DeleteItem(m_hWndList,iItem);
					iItem--;

					iSelItem = iItem;
				}
				else
				{
					break;
				}

				iItem = ListView_GetNextItem(m_hWndList,iItem,LVNI_SELECTED);
			}
			while(iItem != -1);
		}
		else
		{
			hr = S_FALSE;
		}

		ListView_SetItemState(m_hWndList,iSelItem,LVNI_FOCUSED,LVNI_FOCUSED);
		SetFocus(m_hWndList);

		return hr;
	}

	HRESULT DeleteRecycleBinItem()
	{
		//if( _GetOSVersion() < 0x600 )
		//{
		//	int iSelItem = ListView_GetNextItem(m_hWndList,-1,LVNI_SELECTED);
		//	if( iSelItem == -1 )
		//		return 0;
		//
		//	int iItem;
		//	iItem = ListView_GetNextItem(m_hWndList,-1,LVNI_SELECTED);
		//	if( iItem != -1 )
		//	{
		//		HRESULT hr;
		//		do
		//		{
		//			RECYCLE_BIN_FILE_ITEM *pItem = (RECYCLE_BIN_FILE_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);
		//
		//			hr = _ExecRecycleBinItemCommand(m_hWnd,pItem->pidl,"delete");
		//
		//			if( hr == S_OK )
		//			{
		//				ListView_DeleteItem(m_hWndList,iItem);
		//				iItem--;
		//			}
		//
		//			iItem = ListView_GetNextItem(m_hWndList,iItem,LVNI_SELECTED);
		//		}
		//		while(iItem != -1);
		//	}
		//
		//	ListView_SetItemState(m_hWndList,iSelItem,LVNI_FOCUSED,LVNI_FOCUSED);
		//	SetFocus(m_hWndList);
		//
		//	return 0;
		//}

		int iSelItem = ListView_GetNextItem(m_hWndList,-1,LVNI_ALL|LVNI_FOCUSED);
		if( iSelItem == -1 )
			return S_FALSE;

		HRESULT hr;
		IFileOperation*  pFO = NULL;
		IShellFolder*    pBitBucketFolder = NULL;
	    IShellItemArray* pShItemArray = NULL;
		HWND hwndActive = NULL;

		try
		{
			int iItem;
			iItem = ListView_GetNextItem(m_hWndList,-1,LVNI_ALL|LVNI_SELECTED);
			if( iItem != -1 )
			{
				CValArray<LPITEMIDLIST> idls;

				do
				{
					RECYCLE_BIN_FILE_ITEM *pItem = (RECYCLE_BIN_FILE_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

					idls.Add( pItem->pidl );

					iItem = ListView_GetNextItem(m_hWndList,iItem,LVNI_SELECTED);
				}
				while(iItem != -1);

				_GetRecycleBinFolder( &pBitBucketFolder );

#if 1 // vista API
				hr = SHCreateShellItemArray(NULL,pBitBucketFolder,
							idls.GetSize(),
							(PCUITEMID_CHILD_ARRAY)idls.GetBuffer(),
							&pShItemArray);
#else	
				if( pfnSHCreateShellItemArray != NULL )
				{
					hr = pfnSHCreateShellItemArray(NULL,pBitBucketFolder,
								idls.GetSize(),
								(PCUITEMID_CHILD_ARRAY)idls.GetData(),
								&pShItemArray);
				}
				else
				{
					hr = E_NOTIMPL;
				}
#endif	
				if( FAILED(hr) )
					throw hr;

				hr = CoCreateInstance(CLSID_FileOperation,NULL,CLSCTX_INPROC_SERVER,
							__uuidof(IFileOperation),(PVOID *)&pFO);

				if( FAILED(hr) )
					throw hr;

				pFO->SetOwnerWindow( m_hWnd );

				hwndActive = GetActiveWindow();
				EnableWindow(hwndActive,FALSE);

				hr = pFO->DeleteItems(pShItemArray);

				if( SUCCEEDED(hr) )
				{
					hr = pFO->PerformOperations();
				}
				else
				{
					throw hr;
				}

				// Update list view
				//
				iItem = ListView_GetNextItem(m_hWndList,-1,LVNI_ALL|LVNI_SELECTED);
				if( iItem != - 1 )
				{
					do
					{
						RECYCLE_BIN_FILE_ITEM *pItem = (RECYCLE_BIN_FILE_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

						if( !PathFileExists( pItem->ManagementPath ) )
						{
							ListView_DeleteItem(m_hWndList,iItem);
							iItem--;
						}

						iItem = ListView_GetNextItem(m_hWndList,iItem,LVNI_SELECTED);
					}
					while(iItem != -1);
				}

				ListView_SetItemState(m_hWndList,iSelItem,LVNI_FOCUSED,LVNI_FOCUSED);
				SetFocus(m_hWndList);

				EnableWindow(hwndActive,TRUE);

				hr = S_OK;
			}
		}
		catch( HRESULT _hr )
		{
			if( hwndActive )
				EnableWindow(hwndActive,TRUE);
			hr = _hr;
		}

		if( pFO )
			pFO->Release();

		if( pBitBucketFolder )
			pBitBucketFolder->Release();
	    
		if( pShItemArray )
			pShItemArray->Release();

		return hr;
	}

	HRESULT EnumRecycleBinFiles(PCWSTR pszDrivePath,PCWSTR /*pszVolumeName*/)
	{
		IShellFolder *pFolder;
		HRESULT hr;

		hr = SHGetDesktopFolder(&pFolder);
		if( hr != S_OK )
		{
			return hr;
		}

	    PIDLIST_ABSOLUTE pidlDesktop = NULL;
		SHGetSpecialFolderLocation(NULL,CSIDL_BITBUCKET,&pidlDesktop);

		IShellFolder *pBitBucket;
		pFolder->BindToObject(pidlDesktop,NULL,IID_IShellFolder,(LPVOID *)&pBitBucket);

		IEnumIDList *pEnumIDList;
		pBitBucket->EnumObjects(NULL,SHCONTF_FOLDERS|SHCONTF_NONFOLDERS,&pEnumIDList);

		ULONG ulFetched = 0;
		LPITEMIDLIST pidl;
		WCHAR szBuffer[MAX_PATH];
		RECYCLE_BIN_FILE_ITEM *pItem;

		while( pEnumIDList->Next(1, &pidl, &ulFetched) == S_OK)
		{
			ULONG ulAttrs = SFGAO_DISPLAYATTRMASK;
			hr = pBitBucket->GetAttributesOf(1,(const struct _ITEMIDLIST **)&pidl, &ulAttrs);

			if( _GetShellItemNameW(pBitBucket, pidl, SHGDN_FORPARSING, szBuffer, MAX_PATH) )
			{
				if( pszDrivePath == NULL || *pszDrivePath == L'\0' || towupper(szBuffer[0]) == towupper(pszDrivePath[0]) )
				{
					pItem = new RECYCLE_BIN_FILE_ITEM;
					if( pItem == NULL )
					{
						hr = E_OUTOFMEMORY;
						break;
					}

					ZeroMemory(pItem,sizeof(RECYCLE_BIN_FILE_ITEM));

					pItem->ManagementPath = _MemAllocString( szBuffer );

					// File extensions depend on your shell settings. They cannot be retrieved
					// if [Hide extensions for known file types] is enabled.
					if( _GetShellItemNameW(pBitBucket, pidl, SHGDN_NORMAL, szBuffer, MAX_PATH) )
						pItem->OriginalPath = _MemAllocString( szBuffer );

					if( _GetShellItemNameW(pBitBucket, pidl, SHGDN_INFOLDER, szBuffer, MAX_PATH) )
						pItem->Name = _MemAllocString( szBuffer );

					if( pItem->OriginalPath == NULL || 
						pItem->Name == NULL ||
						pItem->ManagementPath == NULL )
					{
						_SafeMemFree( pItem->OriginalPath );
						_SafeMemFree( pItem->Name );
						_SafeMemFree( pItem->ManagementPath );
						hr = E_OUTOFMEMORY;
						break;
					}

					IShellFolder2 *pFolder2;
					if( pBitBucket->QueryInterface(IID_IShellFolder2, (void**)&pFolder2) == S_OK )
					{
						SHCOLUMNID sci = {0};
						sci.fmtid = FMTID_Storage;

						VARIANT val;

						sci.pid = PID_STG_ATTRIBUTES;
						hr = pFolder2->GetDetailsEx(pidl,&sci,&val);
						if( SUCCEEDED(hr) )
							pItem->FileAttributes = val.ulVal;

						sci.pid = PID_STG_WRITETIME;
						hr = pFolder2->GetDetailsEx(pidl,&sci,&val);
						if( SUCCEEDED(hr) )
							pItem->WriteTime = _ConvertVariantDateTime( val );

						sci.pid = PID_STG_CREATETIME;
						hr = pFolder2->GetDetailsEx(pidl,&sci,&val);
						if( SUCCEEDED(hr) )
							pItem->CreateTime = _ConvertVariantDateTime( val );

						sci.pid = PID_STG_SIZE;
						hr = pFolder2->GetDetailsEx(pidl,&sci,&val);
						if( SUCCEEDED(hr) )
							pItem->Size = val.llVal;

						pFolder2->Release();
					}

					pItem->pidl = pidl;

					if( InsertFileItem(pItem) == -1 )
						break;
				}
			}
		}

		CoTaskMemFree(pidlDesktop);

		pEnumIDList->Release();
		pBitBucket->Release();
		pFolder->Release();

		return hr;
	}

	int InsertFileItem(RECYCLE_BIN_FILE_ITEM *pItem)
	{
		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = ListView_GetItemCount(m_hWndList); // todo:
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.iImage  = I_IMAGECALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		int n = ListView_InsertItem(m_hWndList,&lvi);
		return n;
	}

	int GetFileIcon(PCWSTR pszFilePath)
	{
		SHFILEINFO sfi = {0};
		int iImage = I_IMAGENONE;

		if( SHGetFileInfo(pszFilePath,0,&sfi,sizeof(sfi),SHGFI_ICON|SHGFI_SMALLICON) != 0 )
		{
			iImage = ImageList_AddIcon(ListView_GetImageList(m_hWndList,LVSIL_SMALL) ,sfi.hIcon);
			DestroyIcon(sfi.hIcon);
		}
		return iImage;
	}
};
