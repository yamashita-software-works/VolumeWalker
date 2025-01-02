#pragma once
//***************************************************************************
//*                                                                         *
//*  page_volumefilelist.h                                                  *
//*                                                                         *
//*  NT simple file list page for VolumeWalker                              *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  History: 2022-04-04 Base code created                                  *
//*           2024-12-04 Ported to VolumeWalker                             *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//

//++ CAUTION: Do not modify, these features are not supported on this products.
#define _ENABLE_FILE_TOOLS             0
#define _ENABLE_EXTENDED_TOOLS         0 
#define _ENABLE_DRAG_AND_DROP_SUPPORT  0
//--

#include "libntwdk.h"
#include "pagewbdbase.h"
#include "common.h"
#include "column.h"
#include "dparray.h"
#include "history.h"
#include "findhandler.h"
#include "gotodialog.h"
#include "ntvolumenames.h"
#include "ntnotifychangedirectory.h"
#include "filesheaderbar.h"
#if _ENABLE_FILE_TOOLS
#include "fopfilelist.h"
#include "..\fsfiletools\fsfiletools.h"
#include "..\fsvolumehelp\storagedevice.h"
#endif

#define PM_UPDATE_FILELIST (PM_PRIVATEBASE + 1)

#define _IS_CURDIR_NAME( fname ) (fname[0] == L'.' && fname[1] == L'\0')
#define _IS_PARENT_DIR_NAME( fname ) (fname[0] == L'.' && fname[1] == L'.' && fname[2] == L'\0')
#define _PARENT_DIRECTORY(path) (path[0]==L'.'&&path[1]==L'.'&&path[2]==L'\0')

#define _FLG_COSTRY_DATA             (0x1)
#define _FLG_NTFS_SPECIALFILE        (0x800)

#define _FILE_ACTION_RENAME          (0x10)

#define _MAX_LEADING_READ 128

enum {
	ID_GROUP_DIRECTORY=1,
	ID_GROUP_FILE,
};

typedef struct _FILEINFOITEM
{
	UINT Type;
	CFileItem *pFI;
} FILEINFOITEM, *PFILEINFOITEM;

struct CFileInfoItem : public FILEINFOITEM
{
	CFileInfoItem()
	{
		memset(this,0,sizeof(FILEINFOITEM));
	}
};

class CFileListPage :
	public CPageWndBase,
	public CFindHandler<CFileListPage>
{
protected:
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CFileListPage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF<CFileListPage,CFileInfoItem> *m_comp_proc;

	typedef COMPARE_HANDLER_PROC_DEF<CFileListPage,CFileInfoItem> FILELISTPAGE_COMPARE_HANDLER;
	typedef int (CFileListPage::*COMPHANDLER)(CFileInfoItem *p1,CFileInfoItem *p2, const void *p);

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	// CurrentDirectory
	PWSTR m_pszCurDir;
	LARGE_INTEGER m_liCurrentDirectory;

	CColumnList m_columns;

	HFONT m_hFont;
	HFONT m_hFontHeader;

	BOOL m_bNtfsSpecialDirectory;
	BOOL m_bFillCostlyData;
	BOOL m_bEnumShadowCopyVolumes;
	BOOL m_bEnableIconImage;
	BOOL m_bExecFileVerification;

	UINT m_columnShowStyleFlags[COLUMN_MaxCount];

	HANDLE m_hWatchHandle;

	CHistoryManager m_history;

	HRESULT m_LastErrorCode;

	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST; }

	CHeaderBar *m_pHeaderBar;

public:
	PWSTR GetPath() const { return m_pszCurDir; }
	PWSTR GetCurPath() const { return m_pszCurDir; }
	HWND GetListView() const { return m_hWndList; }

	LARGE_INTEGER GetFileId() const
	{
		LARGE_INTEGER li;
		FS_NTFS_SPECIAL_FILE_LIST ntfsFile;
		GetNtfsSpecialFiles(GetPath(),m_liCurrentDirectory.QuadPart, FALSE, &ntfsFile);
		li = ntfsFile.pItemList[0]->ParentDirectory;
		FreeNtfsSpecialFiles(&ntfsFile);
		return li;
	}

	BOOL IsNtfsSpecialDirectory() const { return m_bNtfsSpecialDirectory; }

	CFileListPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = 0; // default: 0 column
		m_Sort.Direction      = 1; // default: ascend order
		m_pszCurDir = NULL;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_liCurrentDirectory.QuadPart = 0;
		m_bNtfsSpecialDirectory = FALSE;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_hWatchHandle = NULL;
		m_LastErrorCode = 0;
		m_bFillCostlyData = TRUE;
		m_pHeaderBar = NULL;
		m_bEnumShadowCopyVolumes = TRUE;
		m_bEnableIconImage = FALSE;
		m_bExecFileVerification = TRUE;
		ZeroMemory(m_columnShowStyleFlags,sizeof(m_columnShowStyleFlags));
	}

	virtual ~CFileListPage()
	{
		_SafeMemFree(m_pszCurDir);

		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;
	}

	virtual HRESULT OnInitPage(PVOID pParam,DWORD,PVOID)
	{
		InitListView();

		if( !LoadColumns(m_hWndList,NULL) )
		{
			InsertDefaultColumns(m_hWndList);
		}

		InitGroup();

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		StartDirectoryWatch(&m_hWatchHandle,&CFileListPage::NotifyCallback,this);

		return S_OK;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hFont = GetGlobalFont(hWnd);
#if 0
		m_hFontHeader = GetIconFont();
#else
		HDC hdc = GetWindowDC(NULL);
		LOGFONT lf = {0};
		lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		lf.lfCharSet = DEFAULT_CHARSET;
		StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Yu Gothic UI");
		m_hFontHeader = CreateFontIndirect( &lf );
		ReleaseDC(NULL,hdc);
#endif

		m_history.Initialize();

		CreateHeaderBar(hWnd);

		return 0;
	}

	BOOL IsHeaderBarVisible()
	{
		return m_pHeaderBar && IsWindow(m_pHeaderBar->GetHwnd()) && (GetWindowLong(m_pHeaderBar->GetHwnd(),GWL_STYLE) & WS_VISIBLE);
	}

	virtual HWND CreateHeaderBar(HWND hWnd)
	{
		HWND hwnd;

		m_pHeaderBar = new CHeaderBar;

		hwnd = m_pHeaderBar->Create(hWnd,0,L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

		COLORREF crHighlightBack = RGB(247,247,247);
		COLORREF crBack = GetSysColor(COLOR_3DFACE);

		HEADERBARCOLOR ibc;

		if( _IsDarkModeEnabled() ) 
		{
			COLORREF crBack = RGB(98,98,98);
			COLORREF crText = RGB(214,214,214);
			COLORREF crHighlightText = RGB(255,255,255);
			ibc.crHighlightText     = 
			ibc.crHighlightBack     = crBack;
			ibc.crNormalBack        = crBack;
			ibc.crNormalText        = crText;
			ibc.crBoxBack           = crBack;
			ibc.crBoxBorder         = crBack;
			ibc.crBoxInactiveBack   = crBack;
			ibc.crBoxInactiveBorder = crBack;
		}
		else
		{
			COLORREF crBack = RGB(250,250,250);
			COLORREF crText = RGB(108,108,108);
			COLORREF crHighlightText = RGB(0,0,0);
			ibc.crHighlightText     = crHighlightText;
			ibc.crHighlightBack     = crBack;
			ibc.crNormalBack        = crBack;
			ibc.crNormalText        = crText;
			ibc.crBoxBack           = crBack;
			ibc.crBoxBorder         = crBack;
			ibc.crBoxInactiveBack   = crBack;
			ibc.crBoxInactiveBorder = crBack;
		}

		SendMessage(m_pHeaderBar->GetHwnd(),HBM_COLOR,2,(LPARAM)&ibc);

		SendMessage(m_pHeaderBar->GetHwnd(),HBM_COLOR,1,0);

		return hwnd;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_hWatchHandle )
		{
			StopDirectoryWatch(m_hWatchHandle);
			m_hWatchHandle = NULL;
		}

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

		if( m_pHeaderBar )
		{
			DestroyWindow( m_pHeaderBar->GetHwnd() );
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
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case HDN_ENDDRAG:
				OnHeaderEndDrag(pnmhdr);
				return TRUE; // prevent order change
			case LVN_COLUMNDROPDOWN:
				return OnColumnDropDown(pnmhdr);
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

		if( pnmhdr->hwndFrom != m_hWndList )
			return 0;

		switch( pnmlvcd->nmcd.dwDrawStage )
		{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;
			case CDDS_ITEMPREPAINT:
			{
				CFileInfoItem *pItem = (CFileInfoItem *)pnmlvcd->nmcd.lItemlParam;

				if( (pItem->pFI->ItemTypeFlag & _FLG_COSTRY_DATA) == 0 )
				{
					GetCostlyFileInformationData(pItem->pFI);
				}

				if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					pnmlvcd->clrText = RGB(32,32,32);

				if( pItem->pFI->ItemTypeFlag & _FLG_NTFS_SPECIALFILE )
					pnmlvcd->clrText = RGB(128,128,128);

				if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_COMPRESSED )
					pnmlvcd->clrText = RGB(0,0,180);
				else if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED )
					pnmlvcd->clrText = RGB(0,174,54);
				else if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
					pnmlvcd->clrText = RGB(100,0,80);
				else if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_VIRTUAL )
					pnmlvcd->clrText = RGB(0,200,48);
				else if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE )
					pnmlvcd->clrText = RGB(185,122,87);
				else if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_SYSTEM )
					pnmlvcd->clrText = RGB(108,108,114);

				if( pItem->pFI->Wof )
					pnmlvcd->clrText = RGB(128,0,80);

				if( 0 ) 
				{
					if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					{
						pnmlvcd->clrTextBk = RGB(232,248,255);

						if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
							pnmlvcd->clrTextBk = RGB(248,243,253);
						else if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED )
							pnmlvcd->clrTextBk = RGB(219,255,219);
						else if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_COMPRESSED )
							pnmlvcd->clrTextBk = RGB(200,231,255);
					}
				}
				return CDRF_NOTIFYPOSTPAINT;
			}
			case CDDS_ITEMPOSTPAINT:
			{
				if( pnmlvcd->nmcd.uItemState & CDIS_FOCUS )
				{
					DrawFocusFrame(m_hWndList,pnmlvcd->nmcd.hdc,&pnmlvcd->nmcd.rc);
				}
				return CDRF_DODEFAULT;
			}
		}
		return 0;
	}

	LRESULT OnHeaderEndDrag(NMHDR *pnmhdr)
	{
		int cColumns;
		int i;

		SetRedraw(m_hWndList,FALSE);
	
		int ColumnId;
		ColumnId = (int)ListViewEx_GetHeaderItemData( m_hWndList, m_Sort.CurrentSubItem  );
	
		HWND hWndHeader = ListView_GetHeader(m_hWndList);
	
		NMHEADER *pI = (NMHEADER *)pnmhdr;
	
		// Forwarding message from ListView's header control.
		LVCOLUMN col = {0};
		col.mask   = LVCF_ORDER;
		col.iOrder = pI->pitem->iOrder;
		ListView_SetColumn(m_hWndList,pI->iItem,&col);
	
		cColumns = Header_GetItemCount( hWndHeader );
	
		LVCOLUMN *aOrder = new LVCOLUMN[cColumns];
		int *aiOrder     = new int[cColumns];
		LPARAM *aCol     = new LPARAM[cColumns];
		LVCOLUMN *pTemp  = new LVCOLUMN[cColumns];

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

		// free memory	
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

		CFileInfoItem *pItem = (CFileInfoItem *)pnmlv->lParam;
		delete pItem->pFI;
		delete pItem;
		return 0;
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( ((pnmlv->uOldState == 0)|| ((pnmlv->uNewState & (LVIS_SELECTED)) == (LVIS_SELECTED))) 
			 && ((pnmlv->uNewState & (LVIS_SELECTED|LVIS_FOCUSED)) == (LVIS_SELECTED|LVIS_FOCUSED)) )
		{
			CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(pnmhdr->hwndFrom,pnmlv->iItem);

			if( wcscmp(pItem->pFI->hdr.FileName,L"..") == 0 )
				return 0;

			PWSTR pszFullPath;
			pszFullPath = CombinePath(m_pszCurDir,pItem->pFI->hdr.FileName);

			SELECT_ITEM sel = {0};
			sel.ViewType  = GetConsoleId();
			sel.pszPath   = pszFullPath;
			sel.pszCurDir = DuplicateString(m_pszCurDir);
			sel.pszName   = DuplicateString(pItem->pFI->hdr.FileName);

			SendMessage(GetParent(m_hWnd),WM_NOTIFY_MESSAGE,UI_NOTIFY_ITEM_SELECTED,(LPARAM)&sel);

			FreeMemory(sel.pszCurDir);
			FreeMemory(sel.pszName);
			FreeMemory(pszFullPath);
		}

		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;

		if( pnmia->iItem == -1 )
			return 0;

		OpenItem( pnmia->iItem );

		return 0;
	}

	LRESULT OnGetEmptyMarkup(NMHDR *pnmhdr)
	{
		NMLVEMPTYMARKUP *pnmMarkup = (NMLVEMPTYMARKUP*)pnmhdr;

		pnmMarkup->dwFlags = EMF_CENTERED;

		PWSTR SystemErrorMessage = NULL;
		_GetSystemErrorMessageEx(this->m_LastErrorCode,&SystemErrorMessage,0);

		StringCchPrintf(pnmMarkup->szMarkup,ARRAYSIZE(pnmMarkup->szMarkup),L"%s 0x%08X",SystemErrorMessage,m_LastErrorCode);

		_FreeSystemErrorMessage(SystemErrorMessage);

		return TRUE;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CFileInfoItem *pItem = (CFileInfoItem *)pnmlv->lParam;

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

		SORT_PARAM<CFileListPage> op = {0};
		op.pThis           = this;
		op.id              = id;
		op.direction       = m_Sort.Direction; // must 1 or -1, do not use 0
		op.directory_align = 0;                // todo:
		ListView_SortItems(m_hWndList,CompareProc,&op);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	LRESULT OnColumnDropDown(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CFileInfoItem *pItem = (CFileInfoItem *)pnmlv->lParam;

		HWND hwndHeader = ListView_GetHeader(pnmlv->hdr.hwndFrom);
		RECT rc;
		Header_GetItemRect(hwndHeader,pnmlv->iSubItem,&rc);
		MapWindowPoints(hwndHeader,NULL,(LPPOINT)&rc,2);

		HDITEM hi;
		hi.mask = HDI_LPARAM;
		Header_GetItem(hwndHeader,pnmlv->iSubItem,&hi);
		int column_id = (int)hi.lParam;
		int check_pos = -1;

		HMENU hMenu = CreatePopupMenu();	
	
		UINT f = m_columnShowStyleFlags[ column_id ];

		switch( column_id )
		{
			case COLUMN_FileAttributes:
				AppendMenu(hMenu,MF_STRING,1,L"Displays by Attribute Character");
				AppendMenu(hMenu,MF_STRING,2,L"Displays by Hex");
				AppendMenu(hMenu,MF_STRING,3,L"Displays by Bit");
				CheckMenuRadioItem(hMenu,1,3,(f+1),MF_BYCOMMAND);
				break;
			case COLUMN_EndOfFile:
			case COLUMN_AllocationSize:
				AppendMenu(hMenu,MF_STRING,1,L"Displays by Decimal");
				AppendMenu(hMenu,MF_STRING,2,L"Displays by Unit");
				AppendMenu(hMenu,MF_STRING,3,L"Displays by Hex");
				CheckMenuRadioItem(hMenu,1,3,(f+1),MF_BYCOMMAND);
				break;
			case COLUMN_LastWriteTime:
			case COLUMN_CreationTime:
			case COLUMN_LastAccessTime:
			case COLUMN_ChangeTime:
				AppendMenu(hMenu,MF_STRING,1,L"Default");
				AppendMenu(hMenu,MF_STRING,2,L"Displays by UTC");
				AppendMenu(hMenu,MF_STRING,3,L"Displays by Hex");
				CheckMenuRadioItem(hMenu,1,3,(f+1),MF_BYCOMMAND);
				break;
		}

		UINT cmd;
		cmd = TrackPopupMenu(hMenu,TPM_RETURNCMD|TPM_RIGHTALIGN|TPM_TOPALIGN,rc.right,rc.bottom,0,m_hWnd,NULL);

		int iColumn;
		switch( column_id )
		{
			case COLUMN_FileAttributes:
				switch( cmd )
				{
					case 1:
						m_columnShowStyleFlags[ column_id ] = 0x0;
						break;
					case 2:
						m_columnShowStyleFlags[ column_id ] = 0x1;
						break;
					case 3:
						m_columnShowStyleFlags[ column_id ] = 0x2;
						break;
				}
				iColumn = FindSubItemById( column_id );
				InvalidateListColumn(iColumn);
				break;
			case COLUMN_EndOfFile:
			case COLUMN_AllocationSize:
				switch( cmd )
				{
					case 1:
						m_columnShowStyleFlags[ column_id ] = 0x0;
						break;
					case 2:
						m_columnShowStyleFlags[ column_id ] = 0x1;
						break;
					case 3:
						m_columnShowStyleFlags[ column_id ] = 0x2;
						break;
				}
				iColumn = FindSubItemById( column_id );
				InvalidateListColumn(iColumn);
				break;
			case COLUMN_LastWriteTime:
			case COLUMN_CreationTime:
			case COLUMN_LastAccessTime:
			case COLUMN_ChangeTime:
				switch( cmd )
				{
					case 1:
						m_columnShowStyleFlags[ column_id ] = 0x0;
						break;
					case 2:
						m_columnShowStyleFlags[ column_id ] = 0x1;
						break;
					case 3:
						m_columnShowStyleFlags[ column_id ] = 0x2;
						break;
				}
				iColumn = FindSubItemById( column_id );
				InvalidateListColumn(iColumn);
				break;
		}

		return 0;
	}

	LRESULT OnDisp_Name(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->pFI->hdr.FileName;
		return 0;
	}

	LRESULT OnDisp_FileId(UINT,NMLVDISPINFO *pnmlvdi)
	{
		// not support 128bit FRN
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		if( pItem->pFI->FileId.QuadPart != 0 )
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%016I64X",pItem->pFI->FileId.QuadPart);
		return 0;
	}

	LRESULT OnDisp_EaSize(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		if( pItem->pFI->EaSize )
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%08X",pItem->pFI->EaSize);
		return 0;
	}

	LRESULT OnDisp_Extension(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		if( !(pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			pnmlvdi->item.pszText = PathFindExtension(pItem->pFI->hdr.FileName);
		else
			pnmlvdi->item.pszText = L"<dir>";
		return 0;
	}

	LRESULT OnDisp_ShortName(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->pFI->ShortName;
		return 0;
	}

	LRESULT OnDisp_Path(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		pnmlvdi->item.pszText = pItem->pFI->hdr.Path;
		return 0;
	}

	LRESULT OnDisp_Lcn(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		if( pItem->pFI->FirstLCN.QuadPart != 0 && pItem->pFI->FirstLCN.QuadPart != -1)
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->pFI->FirstLCN);
		else
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"-");
		return 0;
	}

	LRESULT OnDisp_DateTime(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		LONGLONG dt = 0;
		switch( id )
		{
			case COLUMN_CreationTime:
				dt = pItem->pFI->CreationTime.QuadPart;
				break;
			case COLUMN_LastWriteTime:
				dt = pItem->pFI->LastWriteTime.QuadPart;
				break;
			case COLUMN_LastAccessTime:
				dt = pItem->pFI->LastAccessTime.QuadPart;
				break;
			case COLUMN_ChangeTime:
				dt = pItem->pFI->ChangeTime.QuadPart;
				break;
		}

		UINT fmt = m_columnShowStyleFlags[ id ];

		switch( fmt )
		{
			case 0:
				if( dt > 0 )
					_GetDateTimeStringEx2(dt,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,NULL,NULL,FALSE,1);
				else
					pnmlvdi->item.pszText = L"-";
				break;
			case 1:
				if( dt > 0 )
					_GetDateTimeStringEx2(dt,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,NULL,NULL,TRUE,1);
				else
					pnmlvdi->item.pszText = L"-";
				break;
			case 2:	
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%016I64X",dt);
				break;
		}

		return 0;
	}

	LRESULT OnDisp_Size(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;

		LONGLONG cb;

		if( id == COLUMN_EndOfFile )
			cb = pItem->pFI->EndOfFile.QuadPart;
		else
			cb = pItem->pFI->AllocationSize.QuadPart;

		UINT f = m_columnShowStyleFlags[ id ];
		switch( f )
		{
			case 0:
				_CommaFormatString(cb,pnmlvdi->item.pszText);
				break;
			case 1:
				StrFormatByteSizeW(cb,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
				break;
			case 2:
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",cb);
				break;
		}
		return 0;
	}

	LRESULT OnDisp_Attributes(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		UINT f = m_columnShowStyleFlags[ id ];
		if( f == 0 )
			GetAttributeString(pItem->pFI->FileAttributes,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
		else if( f == 1 )
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%08X",pItem->pFI->FileAttributes);
		else
		{
//			int cch;
//			WCHAR sz[32+1];
//			cch = IntegerToStringCchW(pItem->pFI->FileAttributes,2,sz,_countof(sz));
//			memcpy(pnmlvdi->item.pszText,L"................................", 33 * sizeof(WCHAR));
//			memcpy(&pnmlvdi->item.pszText[ 32 - cch ],sz,cch * sizeof(WCHAR));
			IntegerToStringCchW(pItem->pFI->FileAttributes,2,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
		}

		return 0;
	}

	LRESULT OnDisp_PhysicalDriveNumber(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		if( pItem->pFI->PhysicalDriveNumber != (ULONG)-1 )
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"PhysicalDrive%u",pItem->pFI->PhysicalDriveNumber);
		else
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"-");
		return 0;
	}

	LRESULT OnDisp_PhysicalDriveOffset(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFileInfoItem *pItem = (CFileInfoItem *)pnmlvdi->item.lParam;
		if( pItem->pFI->PhysicalDriveOffset.QuadPart != 0 && pItem->pFI->PhysicalDriveOffset.QuadPart != -1 )
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->pFI->PhysicalDriveOffset.QuadPart);
		else
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"-");
		return 0;
	}

	virtual void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CFileListPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_None,                NULL),
			COL_HANDLER_MAP_DEF(COLUMN_Name,                &CFileListPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_LastWriteTime,       &CFileListPage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_CreationTime,        &CFileListPage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_LastAccessTime,      &CFileListPage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_ChangeTime,          &CFileListPage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_EndOfFile,           &CFileListPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_AllocationSize,      &CFileListPage::OnDisp_Size),
			COL_HANDLER_MAP_DEF(COLUMN_FileAttributes,      &CFileListPage::OnDisp_Attributes),
			COL_HANDLER_MAP_DEF(COLUMN_EaSize,              &CFileListPage::OnDisp_EaSize),
			COL_HANDLER_MAP_DEF(COLUMN_FileId,              &CFileListPage::OnDisp_FileId),
			COL_HANDLER_MAP_DEF(COLUMN_ShortName,           &CFileListPage::OnDisp_ShortName),
			COL_HANDLER_MAP_DEF(COLUMN_Extension,           &CFileListPage::OnDisp_Extension),
			COL_HANDLER_MAP_DEF(COLUMN_Path,                &CFileListPage::OnDisp_Path),
			COL_HANDLER_MAP_DEF(COLUMN_Lcn,                 &CFileListPage::OnDisp_Lcn),
			COL_HANDLER_MAP_DEF(COLUMN_PhysicalDriveNumber, &CFileListPage::OnDisp_PhysicalDriveNumber),
			COL_HANDLER_MAP_DEF(COLUMN_PhysicalDriveOffset, &CFileListPage::OnDisp_PhysicalDriveOffset),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CFileListPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CFileListPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	virtual LRESULT OnGetDispText(int id,NMLVDISPINFO *pdi, CFileInfoItem *pItem)
	{
		if( !(pItem->pFI->ItemTypeFlag & _FLG_COSTRY_DATA) )
		{
			GetCostlyFileInformationData( pItem->pFI );

			pdi->item.mask |= LVIF_DI_SETITEM;
		}

		if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
		{
			return (this->*m_disp_proc[ id ].pfn)(id,pdi);
		}

		return 0;
	}

	virtual LRESULT OnGetDispImage(int id,NMLVDISPINFO *pdi, CFileInfoItem *pItem)
	{
		pdi->item.iImage = GetShellFileImageListIndex(NULL,pItem->pFI->hdr.FileName,pItem->pFI->FileAttributes);

		if( pdi->item.iImage & 0xff000000 )
		{
			pdi->item.state = INDEXTOOVERLAYMASK(pdi->item.iImage >> 24);
			pdi->item.stateMask = LVIS_OVERLAYMASK;
			pdi->item.mask |= LVIF_STATE;
		}

		pdi->item.iImage = pdi->item.iImage & ~0xFF000000;

		pdi->item.mask |= LVIF_DI_SETITEM;

		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CFileInfoItem *pItem = (CFileInfoItem *)pdi->item.lParam;

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

	LRESULT OnGetCurPath(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( wParam == 0 && lParam == 0 )
			return (LRESULT)this->m_pszCurDir;

		if( wParam != 0 )
		{
			StringCchCopy((PWSTR)lParam,(SIZE_T)wParam,GetPath());
			return (LRESULT)IsNtfsSpecialDirectory();
		}
		else
		{
			*((LARGE_INTEGER *)lParam) = GetFileId();
		}
		return 0;
	}

	LRESULT OnGetSelectedFilePath(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( lParam == 0 )
			return 0;
		return GetSelectedFileInfo((FS_SELECTED_FILE *)lParam) == S_OK ? TRUE : FALSE;
	}

#if 0
	LRESULT OnGetSelectedFileList(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( lParam == 0 )
			return 0;
		return MakeSelectedFileList((FS_SELECTED_FILELIST *)lParam) == S_OK ? TRUE : FALSE;
	}

	LRESULT OnGetSelectedFileAttribute(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,iItem);
		return pItem->pFI->FileAttributes;
	}

	LRESULT OnQuerySelectedItemState(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		UINT mask = (UINT)wParam;
		UINT state = 0;
		int cSelItems = ListView_GetSelectedCount(m_hWndList);
		if( cSelItems > 0 )
		{
			if( mask & QSIS_SELECTED )
				state |= QSIS_SELECTED;
			if( (mask & QSIS_MULTIITEMSELECTED) && cSelItems > 1 )
				state |= QSIS_MULTIITEMSELECTED;
		}

		if( mask & QSIS_EMPTYLIST )
			state |= ((ListView_GetItemCount(m_hWndList) == 0) ? QSIS_EMPTYLIST : 0);

		return (LRESULT)state;
	}
#endif

	LRESULT OnChangePath(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HEADERBARITEM *phbi = (HEADERBARITEM *)lParam;
		SIZE_T cch = 32768;
		WCHAR *sz = new WCHAR[cch];
		if( sz != NULL )
		{
			if( HDIF_DRIVE_ITEM & phbi->Flags )
				StringCchPrintf(sz,cch,L"\\??\\%s\\",phbi->Drive);
			else
				StringCchPrintf(sz,cch,L"%s\\",phbi->NtDeviceName);

			SELECT_ITEM sel = {0};
			sel.pszPath = sz;
			UpdateData( &sel );

			m_pHeaderBar->SetPath(sz);

			delete[] sz;
		}
		return 0;
	}

	LRESULT OnMakeContextMenu(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HMENU hMenu = (HMENU)lParam;
		UINT id = LOWORD(wParam);
		if( ID_MENU == id )
		{
			AppendMenu(hMenu,MF_STRING,ID_UP_DIR,                   L"&Go Up One Directory\tAlt+UpArrow");
			AppendMenu(hMenu,MF_STRING,ID_GOTO,                     L"&GoTo Directory\tCtrl+G");
			AppendMenu(hMenu,MF_STRING,0,NULL);
			AppendMenu(hMenu,MF_STRING,ID_OPEN_LOCATION_EXPLORER,   L"Open Explorer");
			AppendMenu(hMenu,MF_STRING,ID_OPEN_LOCATION_CMDPROMPT,  L"Open Command Prompt");
			AppendMenu(hMenu,MF_STRING,ID_OPEN_LOCATION_POWERSHELL, L"Open PowerShell");
			AppendMenu(hMenu,MF_STRING,ID_OPEN_LOCATION_TERMINAL,   L"Open Terminal");
			AppendMenu(hMenu,MF_STRING,ID_OPEN_LOCATION_BASH,       L"Open Bash");
		}
		else
		{
			AppendMenu(hMenu,MF_STRING,ID_GOTO,                     L"&GoTo Directory");
			SetMenuDefaultItem(hMenu,ID_GOTO,FALSE);
		}
		return 0;
	}

	LRESULT OnAsyncUpdateList(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ASSERT(lParam != 0);

		if( lParam == 0 )
			return 0;

		UINT Action = LOWORD(wParam);

		switch( Action )
		{
			case FILE_ACTION_REMOVED:
			case FILE_ACTION_RENAMED_OLD_NAME:
			{
				int cItems = ListView_GetItemCount(m_hWndList);
				int iItemDelete = -1;
				iItemDelete = FindFileName((PCWSTR)lParam);
				if( iItemDelete != -1 )
				{
					SetRedraw(m_hWndList,FALSE);
					ListView_DeleteItem(m_hWndList,iItemDelete);
					SetRedraw(m_hWndList,TRUE);
					InvalidateListItem(iItemDelete);
				}
				_MemFree((PVOID)lParam);
				return 0;	
			}
			case FILE_ACTION_ADDED:
			case FILE_ACTION_RENAMED_NEW_NAME:
			{
				CFileItem *pFI = (CFileItem *)lParam;
				if( FindFileName(pFI->hdr.FileName) == -1 )
				{
					Insert(m_hWndList,pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ? ID_GROUP_DIRECTORY : ID_GROUP_FILE,-1,pFI);
				}
				else
				{
					delete pFI;
				}
				return 0;
			}
			case FILE_ACTION_MODIFIED:
			{
				CFileItem *pFI = (CFileItem *)lParam;
				int iItem;
				iItem = FindFileName(pFI->hdr.FileName);
				if( iItem != -1 )
				{
					CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,iItem);

					pItem->pFI->AllocationSize = pFI->AllocationSize;
					pItem->pFI->ChangeTime     = pFI->ChangeTime;
					pItem->pFI->CreationTime   = pFI->CreationTime;
					pItem->pFI->EndOfFile      = pFI->EndOfFile;
					pItem->pFI->LastAccessTime = pFI->LastAccessTime;
					pItem->pFI->LastWriteTime  = pFI->LastWriteTime;
					pItem->pFI->FileAttributes = pFI->FileAttributes;

					pItem->pFI->ItemTypeFlag &= ~_FLG_COSTRY_DATA;

					InvalidateListItem(iItem);
				}
				delete pFI;
				return 0;
			}
			case _FILE_ACTION_RENAME:
			{
				PWSTR pmszFileNamePair = (PWSTR)lParam;
				int iItem;
				PWSTR pszOldName = pmszFileNamePair;
				PWSTR pszNewName = &pmszFileNamePair[ wcslen(pmszFileNamePair) + 1 ];

				iItem = FindFileName( pszOldName );
				if( iItem != -1 )
				{
					CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,iItem);
					pItem->pFI->SetFileName( pszNewName );
					InvalidateListItem(iItem);
				}
				else
				{
					AppendFileItem( pszNewName );
				}
				_MemFree(pmszFileNamePair);
				return 0;
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
			case WM_PRETRANSLATEMESSAGE:
				return FALSE;
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
			case WM_NCACTIVATE:
				if( m_pHeaderBar )
					SendMessage(m_pHeaderBar->GetHwnd(),HBM_COLOR,wParam?1:0,0);
				return 0;
			case WM_QUERY_MESSAGE:
			{
				if( LOWORD(wParam) == QMT_GETVOLUMEPATH && lParam != 0 ) 
				{
					if( m_pszCurDir )
					{
						QM_PARAM *pParam = (QM_PARAM *)lParam;
						StringCchCopy(pParam->VolumePath,pParam->dwLength,m_pszCurDir);
						return (LRESULT)wcslen(m_pszCurDir);
					}
				}
				return 0;
			}
			case PM_FINDITEM:
				return CFindHandler<CFileListPage>::OnFindItem(hWnd,uMsg,wParam,lParam);;
			case PM_GETWORKINGDIRECTORY:
				return OnGetCurPath(hWnd,uMsg,wParam,lParam);
			case PM_GETSELECTEDFILE:
				return OnGetSelectedFilePath(hWnd,uMsg,wParam,lParam);
			case PM_UPDATE_FILELIST:
				return OnAsyncUpdateList(hWnd,uMsg,wParam,lParam);
#if 0
			case PM_GETSELECTEDFILELIST:
				return OnGetSelectedFileList(hWnd,uMsg,wParam,lParam);
			case PM_GETSELECTEDFILEATTRIBUTE:
				return OnGetSelectedFileAttribute(hWnd,uMsg,wParam,lParam);
			case PM_QUERYSELECTEDITEMSTATE:
				return OnQuerySelectedItemState(hWnd,uMsg,wParam,lParam);
#endif
			case PM_CHANGEPATH:
				return OnChangePath(hWnd,uMsg,wParam,lParam);
			case PM_MAKECONTEXTMENU:
				return OnMakeContextMenu(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	virtual VOID UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
		{
			int cxList = cx;
			int cyList = cy;
			int cyHeaderBar = 0;

			HDWP hdwp = BeginDeferWindowPos(2);

			if( IsHeaderBarVisible() )
			{
				cyHeaderBar = m_pHeaderBar->GetHeight();
				DeferWindowPos(hdwp,m_pHeaderBar->GetHwnd(),NULL,0,0,cx,cyHeaderBar,SWP_NOZORDER|SWP_NOMOVE);
			}

			DeferWindowPos(hdwp,m_hWndList,NULL,0,cyHeaderBar,cxList,cyList-cyHeaderBar,SWP_NOZORDER);

			EndDeferWindowPos(hdwp);
		}
	}

	virtual DWORD DefListViewStyle() const
	{
		return (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS);
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
		if( m_bEnableIconImage )
		{
			HIMAGELIST himl = GetGlobalShareImageList(0);
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
			{ ID_GROUP_DIRECTORY, 0, L"Directory" },
			{ ID_GROUP_FILE,      0, L"File"  },
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
			{ COLUMN_Name,                L"Name",                  0},
			{ COLUMN_Extension,           L"Extension",             0},
			{ COLUMN_FileAttributes,      L"Attributes",            0},
			{ COLUMN_EndOfFile,           L"EndOfFile",             0},
			{ COLUMN_AllocationSize,      L"AllocationSize",        0},
			{ COLUMN_LastWriteTime,       L"LastWriteTime",         0},
			{ COLUMN_CreationTime,        L"CreationTime",          0},
			{ COLUMN_LastAccessTime,      L"LastAccessTime",        0},
			{ COLUMN_ChangeTime,          L"ChangeTime",            0},
			{ COLUMN_EaSize,              L"EaSize",                0},
			{ COLUMN_FileId,              L"FileId",                0},
			{ COLUMN_FileIndex,           L"FileIndex",             0},
			{ COLUMN_ShortName,           L"ShortName",             0},
			{ COLUMN_Extension,           L"Extension",             0},
			{ COLUMN_Path,                L"Path",                  0},
			{ COLUMN_Lcn,                 L"Lcn",                   0},
			{ COLUMN_PhysicalDriveNumber, L"PhysicalDrive",         0}, 
			{ COLUMN_PhysicalDriveOffset, L"PhysicalOffset",        0},
		};
		m_columns.SetColumnNameMap( _countof(column_name_map), column_name_map );
	}

	virtual void InitColumnDefinitions()
	{
		static COLUMN def_columns[] = {
			{ COLUMN_Name,                L"Name",                  1, 280, LVCFMT_LEFT },
			{ COLUMN_Extension,           L"Extension",             2,  80, LVCFMT_LEFT },
			{ COLUMN_FileId,              L"FRN",                   3, 156, LVCFMT_LEFT },
			{ COLUMN_FileAttributes,      L"Attributes",            4, 100, LVCFMT_RIGHT|LVCFMT_SPLITBUTTON },
			{ COLUMN_Lcn,                 L"LCN",                   5, 120, LVCFMT_RIGHT },
			{ COLUMN_PhysicalDriveOffset, L"Physical Offset",       6, 120, LVCFMT_RIGHT },
			{ COLUMN_PhysicalDriveNumber, L"Physical Drive",        7, 120, LVCFMT_LEFT },
			{ COLUMN_EndOfFile,           L"Size",                  8, 116, LVCFMT_RIGHT|LVCFMT_SPLITBUTTON },
			{ COLUMN_AllocationSize,      L"Allocation Size",       9, 116, LVCFMT_RIGHT|LVCFMT_SPLITBUTTON },
			{ COLUMN_LastWriteTime,       L"Date",                 10, 180, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_CreationTime,        L"Creation Time",        11, 180, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_LastAccessTime,      L"Last Access Time",     12, 180, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_ChangeTime,          L"Change Time",          13, 180, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_EaSize,              L"ExAttr",               14, 100, LVCFMT_LEFT },
			{ COLUMN_ShortName,           L"Short Name",           15, 120, LVCFMT_LEFT },
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

	virtual BOOL LoadColumns(HWND hWndList,PCWSTR pszSectionName)
	{
#if 0
		COLUMN_TABLE *pcoltbl;

		WCHAR buf[] = 
				L"Name=280\0"
				L"Extension=80\0"
				L"Attributes=100\0"
				L"EndOfFile=116\0"
				L"AllocationSize=116\0"
				L"LastWriteTime=180\0"
				L"CreationTime=180\0"
				L"LastAccessTime=180\0"
				L"ChangeTime=180\0"
				L"EaSize=100\0"
				L"FileId=156\0"
				L"ShortName=120\0";

		int cb = sizeof(buf);

		if( m_columns.LoadUserDefinitionColumnTableFromText(&pcoltbl,buf,cb) == 0)
			return FALSE;

		InsertColumns(hWndList,pcoltbl);

		m_columns.FreeUserDefinitionColumnTable(pcoltbl);

		LARGE_INTEGER li;
		PWSTR pszSortColumn = L"Name,1";
		li = m_columns.GetColumnSortInfoFromText(pszSortColumn);
		if( li.QuadPart != 0 )
		{
			m_Sort.CurrentSubItem = FindSubItemById( li.LowPart );
			m_Sort.Direction = li.HighPart;
		}

		return TRUE;
#else
		return FALSE;
#endif
	}

	BOOL SaveColumns(HWND hWndList)
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
			pcoltbl->column[i].field = m_columnShowStyleFlags[ pcoltbl->column[i].id ];
		}

		int ColumnId;
		ColumnId = (int)ListViewEx_GetHeaderItemData( m_hWndList, m_Sort.CurrentSubItem  );

#if 0
		m_columns.SaveColumnTable(pcoltbl,SectionName,IniFileName,ColumnId,m_Sort.Direction);
#else
		PWSTR pszColumns = NULL;
		PWSTR pszSortColumn = NULL;
		m_columns.MakeColumnString(pcoltbl,ColumnId,m_Sort.Direction,&pszColumns,&pszSortColumn);
		CoTaskMemFree(pszColumns);
		CoTaskMemFree(pszSortColumn);
#endif

		_MemFree(pcoltbl);

		return TRUE;
	}

	virtual HRESULT SeveConfig()
	{
		SaveColumns(m_hWndList); // todo:
		return S_OK;
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

	int Insert(HWND hWndList,int iGroupId,int iItem,CFileItem *pFI,int iIndent=0,int iImage=I_IMAGECALLBACK)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);

		CFileInfoItem *pItem = new CFileInfoItem;

		pItem->Type  = 0;
		pItem->pFI   = pFI;

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
	// NT filesystem enum callback
	//
	static HRESULT CALLBACK EnumCallbackProc(ULONG /*CallType*/,PVOID FileInfo,PFSDIRENUMCALLBACKINFO /*DirEnumInfo*/,PVOID Context)
	{
		FS_FILE_ID_BOTH_DIR_INFORMATION *pfibdi = (FS_FILE_ID_BOTH_DIR_INFORMATION *)FileInfo;

		if( *pfibdi->FileName == L'.' && pfibdi->FileNameLength == sizeof(WCHAR) )
		{
			return S_OK; // skip myself
		}

		WCHAR filename[MAX_PATH];
		memcpy_s(filename,sizeof(filename),pfibdi->FileName,pfibdi->FileNameLength);
		filename[pfibdi->FileNameLength/sizeof(WCHAR)] = UNICODE_NULL;

		CFileItem *pFI = new CFileItem(NULL,filename);

		pFI->FileAttributes = pfibdi->FileAttributes;
		pFI->LastWriteTime  = pfibdi->LastWriteTime;
		pFI->CreationTime   = pfibdi->CreationTime;
		pFI->LastAccessTime = pfibdi->LastAccessTime;
		pFI->ChangeTime     = pfibdi->ChangeTime;
		pFI->EndOfFile      = pfibdi->EndOfFile;
		pFI->AllocationSize = pfibdi->AllocationSize;
		pFI->EaSize         = pfibdi->EaSize;
	    pFI->FileId         = pfibdi->FileId;
		pFI->FileNameLength = pfibdi->FileNameLength;
		pFI->FileIndex      = pfibdi->FileIndex;

		memcpy(pFI->ShortName,pfibdi->ShortName,pfibdi->ShortNameLength);
		pFI->ShortName[pfibdi->ShortNameLength/sizeof(WCHAR)] = UNICODE_NULL;

		((PtrArray<CFileItem*> *)Context)->Add( pFI );	

		return S_OK;
	}

	HRESULT AddNtfsItem(FS_NTFS_SPECIAL_FILE_ITEM *pntfs,PtrArray<CFileItem*> *pa)
	{
		WCHAR filename[MAX_PATH];
		memcpy_s(filename,sizeof(filename),pntfs->Name,pntfs->NameLength);
		filename[pntfs->NameLength/sizeof(WCHAR)] = UNICODE_NULL;

		CFileItem *pFI = new CFileItem(NULL,filename);

		pFI->FileAttributes = pntfs->FileAttributes;
		pFI->LastWriteTime  = pntfs->LastModificationTime;
		pFI->CreationTime   = pntfs->CreationTime;
		pFI->LastAccessTime = pntfs->LastAccessTime;
		pFI->ChangeTime     = pntfs->LastModificationTime;
		pFI->EndOfFile      = pntfs->FileSize;
		pFI->AllocationSize = pntfs->AllocatedLength;
		pFI->EaSize         = pntfs->PackedEaSize;
	    pFI->FileId         = pntfs->FileId;
		pFI->FileNameLength = pntfs->NameLength;
		pFI->FileIndex      = 0;
		pFI->ParentFileId   = pntfs->ParentDirectory;

		memcpy(pFI->ShortName,pntfs->ShortName,pntfs->ShortNameLength);
		pFI->ShortName[pntfs->ShortNameLength/sizeof(WCHAR)] = UNICODE_NULL;

		pFI->ItemTypeFlag |= _FLG_NTFS_SPECIALFILE;

		pa->Add( pFI );	

		return S_OK;
	}

	void InitCostryFileItemInformation(CFileItem *pFI)
	{
	    pFI->NumberOfLinks = -1;
		pFI->DeletePending = false;
		pFI->Directory = false;
		pFI->Wof = false;
		pFI->FirstLCN.QuadPart = 0;
		pFI->PhysicalDriveOffset.QuadPart = -1;
		pFI->PhysicalDriveNumber = (ULONG)-1;
	}

	void UpdateFileItemInformation(HANDLE hVolume,HANDLE hFile,CFileItem *pFI)
	{
		pFI->ItemTypeFlag |= _FLG_COSTRY_DATA;

		if( pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			FILE_STANDARD_INFO fsi = {0};
			if( GetFileInformationByHandleEx(hFile,FileStandardInfo,&fsi,sizeof(fsi)) )
			{
				if( (pFI->EndOfFile.QuadPart != fsi.EndOfFile.QuadPart) || 
					(pFI->AllocationSize.QuadPart != fsi.AllocationSize.QuadPart) )
				{
					pFI->EndOfFile      = fsi.EndOfFile;
					pFI->AllocationSize = fsi.AllocationSize;
				    pFI->NumberOfLinks  = fsi.NumberOfLinks;
					pFI->DeletePending  = fsi.DeletePending;
					pFI->Directory      = fsi.Directory;
				}
			}
		}

		PWSTR RootDirectory=NULL;
		if( hVolume != NULL )
			SplitRootPath_W(m_pszCurDir,&RootDirectory,NULL,NULL,NULL);
		else
			RootDirectory = DuplicateString(m_pszCurDir);

		if( pFI->AllocationSize.QuadPart != 0 )
		{
			if( m_bFillCostlyData )
			{
				if( 1 ) // bQuick
				{
					FS_CLUSTER_INFORMATION_BASIC_EX clusterex = {0};
					if( ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationBasicWithPhysicalLocation,&clusterex,sizeof(clusterex)) == 0 )
					{
						pFI->FirstLCN = clusterex.FirstLcn;
						pFI->PhysicalDriveOffset = clusterex.PhysicalLocation;
						pFI->PhysicalDriveNumber = clusterex.DiskNumber;
					}
				}
				else if( 0 ) // bFull
				{
					FS_CLUSTER_INFORMATION *pci;
					if( ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationAll,&pci,sizeof(pci)) == 0 )
					{
						pFI->FirstLCN = pci->Extents[0].Lcn;
						pFI->PhysicalDriveOffset.QuadPart = pci->Extents[0].PhysicalOffsets[0].PhysicalOffset->Offset;
						pFI->PhysicalDriveNumber = pci->Extents[0].PhysicalOffsets[0].PhysicalOffset->DiskNumber;
						FreeClusterInformation(pci);
					}
				}
				else // bBasic
				{
					FS_CLUSTER_INFORMATION_BASIC cluster = {0};
					if( ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationBasic,&cluster,sizeof(cluster)) == 0 )
					{
						pFI->FirstLCN = cluster.FirstLcn;
					}
				}
			}
		}
		else
		{
			if( (pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
			{
				pFI->Wof = (GetWofInformation(hFile,NULL,NULL) == S_OK);
			}
		}
		FreeMemory(RootDirectory);
	}

	//
	// Get costly file information.
	//
	virtual void GetCostlyFileInformationData(CFileItem *pFI)
	{
		NTSTATUS Status;

		PWSTR RootDirectory=NULL,RootRelativePath=NULL;
		ULONG cchRootDirectory=0,cchRootRelativePath=0;
		SplitRootPath_W(m_pszCurDir,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);

		InitCostryFileItemInformation(pFI);

		HANDLE hRootDirectory = NULL;
		if( OpenRootDirectory(RootDirectory,0,&hRootDirectory) != STATUS_SUCCESS )
		{
			//
			// The Root directory open failed. If so try open directory using
			// full path string without splitting the volume and root relative path.
			//
			FreeMemory(RootRelativePath);
			RootRelativePath = DuplicateString(m_pszCurDir); // duplicate full-path string
			hRootDirectory = NULL;
		}

		HANDLE hCurDir;
		if( (Status = OpenFile_W(&hCurDir,hRootDirectory,RootRelativePath,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE)) == 0 )
		{
			if( !_IS_CURDIR_NAME( pFI->hdr.FileName ) && !_IS_PARENT_DIR_NAME( pFI->hdr.FileName ) )
			{
				HANDLE hFile;
				ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
				ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;
				if( OpenFile_W(&hFile,hCurDir,pFI->hdr.FileName,DesiredAccess,FILE_SHARE_READ|FILE_SHARE_WRITE,Option) == 0 )
				{
					UpdateFileItemInformation(hRootDirectory,hFile,pFI);

					CloseHandle(hFile);
				}
			}
			CloseHandle(hCurDir);
		}

		FreeMemory(RootRelativePath);
		FreeMemory(RootDirectory);

		if( hRootDirectory )
			CloseHandle(hRootDirectory);

		// We've already tried to get the data.
		pFI->ItemTypeFlag |= _FLG_COSTRY_DATA;
	}

	virtual void GetCostlyFilesInformationOnPtrArray(PtrArray<CFileItem*> *pa)
	{
		NTSTATUS Status;

		PWSTR RootDirectory=NULL,RootRelativePath=NULL;
		ULONG cchRootDirectory=0,cchRootRelativePath=0;
		SplitRootPath_W(m_pszCurDir,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);

		HANDLE hRootDirectory = NULL;
		if( OpenRootDirectory(RootDirectory,0,&hRootDirectory) != STATUS_SUCCESS )
		{
			//
			// The Root directory open failed. If so try open directory using
			// full path string without splitting the volume and root relative path.
			//
			FreeMemory(RootRelativePath);
			RootRelativePath = DuplicateString(m_pszCurDir); // duplicate full-path string
			hRootDirectory = NULL;
		}

		HANDLE hCurDir;
		if( (Status = OpenFile_W(&hCurDir,hRootDirectory,RootRelativePath,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE)) == 0 )
		{
			int i,cFiles = pa->GetCount();

			cFiles = min(cFiles,_MAX_LEADING_READ);

			for(i = 0; i < cFiles; i++)
			{
				CFileItem *pFI = pa->Get(i);

				InitCostryFileItemInformation(pFI);

				if( !_IS_CURDIR_NAME( pFI->hdr.FileName ) && !_IS_PARENT_DIR_NAME( pFI->hdr.FileName ) )
				{
					HANDLE hFile;

					ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
					ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;

					if( OpenFile_W(&hFile,hCurDir,pFI->hdr.FileName,DesiredAccess,FILE_SHARE_READ|FILE_SHARE_WRITE,Option) == 0 )
					{
						UpdateFileItemInformation(hRootDirectory,hFile,pFI);

						CloseHandle(hFile);
					}
				}
				else
				{
					pFI->ItemTypeFlag |= _FLG_COSTRY_DATA;
				}
			}
			CloseHandle(hCurDir);
		}

		FreeMemory(RootRelativePath);
		FreeMemory(RootDirectory);

		if( hRootDirectory )
			CloseHandle(hRootDirectory);
	}

	void GetRootDirectoryInformation(CFileItem* pFI)
	{
		NTSTATUS Status;
		HANDLE hRootDirectory = NULL;
		PWSTR RootDir;

		RootDir = CombinePath(pFI->hdr.Path,L"\\");

		if( (Status = OpenFile_W(&hRootDirectory,hRootDirectory,RootDir,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE)) == 0 )
		{
			if( ::GetFileId(hRootDirectory,&pFI->FileId) == 0 )
			{

			}

			FILE_BASIC_INFO fbi = {0};
			if( GetFileInformationByHandleEx(hRootDirectory,FileBasicInfo,&fbi,sizeof(fbi)) )
			{
				pFI->CreationTime   = fbi.CreationTime;
				pFI->LastWriteTime  = fbi.LastWriteTime;
			    pFI->LastAccessTime = fbi.LastAccessTime;
				pFI->ChangeTime     = fbi.ChangeTime;
				pFI->FileAttributes = fbi.FileAttributes;
			}

			FILE_STANDARD_INFO fsi = {0};
			if( GetFileInformationByHandleEx(hRootDirectory,FileStandardInfo,&fsi,sizeof(fsi)) )
			{
				if( (pFI->EndOfFile.QuadPart != fsi.EndOfFile.QuadPart) || 
					(pFI->AllocationSize.QuadPart != fsi.AllocationSize.QuadPart) )
				{
					pFI->EndOfFile      = fsi.EndOfFile;
					pFI->AllocationSize = fsi.AllocationSize;
				    pFI->NumberOfLinks  = fsi.NumberOfLinks;
					pFI->DeletePending  = fsi.DeletePending;
					pFI->Directory      = fsi.Directory;
				}
			}

			if( pFI->AllocationSize.QuadPart != 0 )
			{
				FS_CLUSTER_INFORMATION_BASIC_EX clusterex = {0};
				if( ReadFileClusterInformaion(NULL,hRootDirectory,RootDir,ClusterInformationBasicWithPhysicalLocation,&clusterex,sizeof(clusterex)) == 0 )
				{
					pFI->FirstLCN = clusterex.FirstLcn;
					pFI->PhysicalDriveOffset = clusterex.PhysicalLocation;
					pFI->PhysicalDriveNumber = clusterex.DiskNumber;
				}
			}

			CloseHandle(hRootDirectory);

			pFI->FlagsEx = FIF_EX_ROOTDIRECTORY;
			pFI->ItemTypeFlag = _FLG_COSTRY_DATA; // todo: trick: avoid re-collect information.
		}
		else
		{
			pFI->FlagsEx = FIF_EX_INVALIDITEM|FIF_EX_ROOTDIRECTORY;
			pFI->ItemTypeFlag = _FLG_COSTRY_DATA; // todo: trick: avoid re-collect information.
		}

		FreeMemory(RootDir);
	}

	HRESULT EnumRootDirectories(PtrArray<CFileItem*> *pa)
	{
		HRESULT hr = S_OK;
		WCHAR szName[MAX_PATH];
		WCHAR szDrive[8];

		LoadFltLibDll(NULL);

		VOLUME_NAME_STRING_ARRAY *VolumeNames;
		EnumVolumeNames( &VolumeNames );

		for(ULONG i = 0; i < VolumeNames->Count; i++)
		{
			if( GetVolumeDosName(VolumeNames->Volume[i].NtVolumeName,szDrive,ARRAYSIZE(szDrive)) != S_OK )
			{
				szDrive[0] = 0;
			}

			PWSTR pName = L"";
			if( HasPrefix(L"\\Device\\",VolumeNames->Volume[i].NtVolumeName) )
			{
				pName = (PWSTR)&VolumeNames->Volume[i].NtVolumeName[8];
			}

			if( szDrive[0] )
			{
				StringCchPrintf(szName,MAX_PATH,L"%s (%s)",pName,szDrive);
			}
			else
			{
				StringCchPrintf(szName,MAX_PATH,L"%s",pName);
			}

			CFileItem *pFI = new CFileItem(VolumeNames->Volume[i].NtVolumeName,pName);

			if( pFI )
			{
				ZeroMemory(pFI->ShortName,sizeof(pFI->ShortName));

				GetRootDirectoryInformation(pFI);

				pa->Add( pFI );	
			}
		}

		FreeVolumeNames( VolumeNames );

		if( m_bEnumShadowCopyVolumes )
		{
			int cchBuffer = 65536;
			WCHAR *pszBuffer = (WCHAR *)AllocStringBuffer( cchBuffer );
			if( pszBuffer != NULL )
			{
				QueryDosDeviceW(NULL,pszBuffer,cchBuffer);

				WCHAR *pName = pszBuffer;

				while( *pName )
				{
					if( wcsnicmp(pName,L"HarddiskVolumeShadowCopy",24) == 0 )
					{
						WCHAR szShadowVolumeName[MAX_PATH];
						StringCchPrintf(szShadowVolumeName,MAX_PATH,L"\\Device\\%s",pName);

						if( GetVolumeDosName(szShadowVolumeName,szDrive,ARRAYSIZE(szDrive)) != S_OK )
						{
							szDrive[0] = 0;
						}

						CFileItem *pFI = new CFileItem(szShadowVolumeName,pName);
						if( pFI )
						{
							ZeroMemory(pFI->ShortName,sizeof(pFI->ShortName));

							GetRootDirectoryInformation(pFI);

							pa->Add( pFI );	
						}
					}
					pName += (wcslen(pName) + 1);
				}
				FreeMemory(pszBuffer);
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}

		UnloadFltLibDll(0);

		return hr;
	}

	//
	// Enumerate files
	//
	virtual HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		PWSTR pszPath = pSel->pszPath;
		PWSTR pszFileName = pSel->pszName;
		FILE_ID_DESCRIPTOR fid = pSel->FileId;

		DWORD hr;
		PtrArray<CFileItem*> *pa = new PtrArray<CFileItem*>;
		if( pa == NULL )
		{
			return E_OUTOFMEMORY;
		}

		SetRedraw(m_hWndList,FALSE);

		if( (pSel->Flags & SI_FLAG_ROOT_DIRECTORY) == 0 )
		{
			pa->Create( 4096 );

			//
			// Enum directort/file items by NT native API.
			//

			m_bNtfsSpecialDirectory = FALSE;

			if( (pSel->mask & SI_MASK_FILEID) == 0 )
			{
				WCHAR *szNtPath = new WCHAR[ _NT_PATH_FULL_LENGTH  ];
				if( szNtPath )
				{
					StringCchCopy(szNtPath,_NT_PATH_FULL_LENGTH ,pszPath);
	
					hr = EnumDirectoryFiles_W(szNtPath,NULL,0,&EnumCallbackProc,(PVOID)pa);
	
					// always save curdir
					_SafeMemFree(m_pszCurDir);
					m_pszCurDir = _MemAllocString(szNtPath);
	
					delete szNtPath;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
	
				if( hr != S_OK )
				{
					delete pa;
					SetRedraw(m_hWndList,TRUE);
					MessageBeep(MB_ICONSTOP);
	
					ListView_DeleteAllItems(m_hWndList);
	
					m_LastErrorCode = hr;
					ListViewEx_ResetEmptyText(m_hWndList);
	
					return hr;
				}
	
				if( IsRootDirectory_W(m_pszCurDir) )
				{
					FS_NTFS_SPECIAL_FILE_LIST ntfsFiles;
					if( GetNtfsSpecialFiles(m_pszCurDir,0,TRUE,&ntfsFiles) == 0 )
					{
						for( ULONG i = 0; i < ntfsFiles.cItemListCount; i++ )
						{
							FS_NTFS_SPECIAL_FILE_ITEM* pItem = ntfsFiles.pItemList[i];
							AddNtfsItem(pItem,pa);
						}
	
						FreeNtfsSpecialFiles(&ntfsFiles);
					}
				}
			}
			else
			{
				FS_NTFS_SPECIAL_FILE_LIST ntfsFiles;
				if( GetNtfsSpecialFiles(m_pszCurDir,fid.FileId.QuadPart,TRUE,&ntfsFiles) == 0 )
				{
					for( ULONG i = 0; i < ntfsFiles.cItemListCount; i++ )
					{
						FS_NTFS_SPECIAL_FILE_ITEM* pItem = ntfsFiles.pItemList[i];
						AddNtfsItem(pItem,pa);
					}
					FreeNtfsSpecialFiles(&ntfsFiles);
	
					_SafeMemFree(m_pszCurDir);
					m_pszCurDir = _MemAllocString(pszPath);
	
					m_liCurrentDirectory = fid.FileId;
	
					m_bNtfsSpecialDirectory = TRUE;
		
					hr = S_OK;
				}
				else
				{
					delete pa;
					SetRedraw(m_hWndList,TRUE);
					MessageBeep(MB_ICONSTOP);
	
					ListView_DeleteAllItems(m_hWndList);
	
					m_LastErrorCode = hr = E_FAIL;
					ListViewEx_ResetEmptyText(m_hWndList);
	
					return E_FAIL;
				}
			}
	
			//
			// Get information enumerated files.
			//
			GetCostlyFilesInformationOnPtrArray(pa);
		}
		else
		{
			//
			// Make Root Directories List
			//
			_SafeMemFree(m_pszCurDir);
			if( pSel->pszPath )
				m_pszCurDir = _MemAllocString(pSel->pszPath);
			else
				m_pszCurDir = _MemAllocString(L"");

			pa->Create( 256 );
			EnumRootDirectories(pa);
		}

		//
		// Append item to history list.
		//
		if( m_pszCurDir && *m_pszCurDir != 0 && (pSel->Flags & SI_FLAG_NOT_ADD_TO_HISTORY) == 0 )
			m_history.Push( m_pszCurDir );

		//
		// Delete all items on the list-view control.
		//
		ListView_DeleteAllItems(m_hWndList);

		//
		// Insert to list-view window
		//
		CFileItem **pFI = pa->GetPtrPtr();
		int cFiles = pa->GetCount();
		for(int i = 0; i < cFiles; i++)
		{
			if( (pFI[i]->FlagsEx & FIF_EX_INVALIDITEM) == 0 )
			{
				Insert(m_hWndList,
						pFI[i]->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ? ID_GROUP_DIRECTORY : ID_GROUP_FILE,
						i,
						pFI[i]);
			}
			else
			{
				delete pFI[i]; // remove unrecognized item
				pa->Set(i,NULL);
			}
		}

		//
		// Sort Items
		//
		DoSort(m_Sort.CurrentSubItem,FALSE);

		//
		// Delete array
		//
		delete pa;

		//
		// Select item, if name available.
		//
		int iItem = -1;
		if( pszFileName )
			iItem = FindFileName(pszFileName);

		SetRedraw(m_hWndList,TRUE);

		if( iItem != -1 )
		{
			ListView_SetItemState(m_hWndList,iItem,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
			ListView_EnsureVisible(m_hWndList,iItem,FALSE);
		}

		SetWatchDirectory(m_hWatchHandle,pSel->pszPath);

		m_LastErrorCode = S_OK;

		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pSelItem)
	{
		HRESULT hr;

		SELECT_ITEM *pPath = (SELECT_ITEM *)pSelItem;

		if( m_pHeaderBar )
		{
			if( pPath->pszPath == NULL )
				return S_FALSE;

			m_pHeaderBar->SetPath(pPath->pszPath);
			m_pHeaderBar->EnableButton(ID_UP_DIR, NtPathIsRootDirectory(pPath->pszPath) ? FALSE : TRUE );
		}

		hr = FillItems(pPath);

		return hr;
	}

	int OpenItem(int iItem)
	{
		if( iItem == -1 )
			return -1;

		CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		if( pItem == NULL )
			return -1;

		if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if( wcscmp(pItem->pFI->hdr.FileName,L".") == 0 )
				return 0;

			PWSTR pszGoToDir;

			if( pItem->pFI->FlagsEx & FIF_EX_ROOTDIRECTORY )
			{
				pszGoToDir = CombinePath(pItem->pFI->hdr.Path,L"\\");
			}
			else
			{
				if( wcscmp(pItem->pFI->hdr.FileName,L"..") == 0 )
					pszGoToDir = DuplicateString(m_pszCurDir);
				else
					pszGoToDir = CombinePath(m_pszCurDir,pItem->pFI->hdr.FileName);
			}

			SELECT_ITEM sel = {0};
			sel.mask      = SI_MASK_PATH|SI_MASK_NAME|SI_MASK_CURDIR;
			sel.ViewType  = GetConsoleId();
			sel.pszPath   = pszGoToDir;
			sel.pszCurDir = DuplicateString(m_pszCurDir);

			if( wcscmp(pItem->pFI->hdr.FileName,L"..") == 0 )
				sel.pszName   = DuplicateString(pItem->pFI->hdr.FileName);

			sel.FileId.dwSize = sizeof(FILE_ID_DESCRIPTOR);
			sel.FileId.Type = FileIdType;
			sel.FileId.FileId.QuadPart = pItem->pFI->FileId.QuadPart;

			if( pItem->pFI->ItemTypeFlag & _FLG_NTFS_SPECIALFILE )
			{
				sel.Flags = _FLG_NTFS_SPECIALFILE;
				sel.mask |= SI_MASK_FILEID;
			}

			SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);

			FreeMemory(sel.pszCurDir);
			FreeMemory(sel.pszName);
			FreeMemory(pszGoToDir);
		}
		else
		{
			if( m_bExecFileVerification && PathIsExe( pItem->pFI->hdr.FileName ) )
			{
				WCHAR szInstruction[MAX_PATH + 64];
				WCHAR szMessage[MAX_PATH + 64];
				StringCchPrintf(szInstruction,ARRAYSIZE(szInstruction),L"Confirm Execution - %s",pItem->pFI->hdr.FileName);
				StringCchPrintf(szMessage,ARRAYSIZE(szMessage),L"Do you want to execute '%s'?",pItem->pFI->hdr.FileName);

				TASKDIALOG_BUTTON buttons[] =
				{
					{ IDOK, L"Exec"  },
					{ IDCANCEL, L"Cancel" }
				};

				int SelectedButton = 0;
				int VerificationFlag = 0;
				VerificationMessageBox( GetParent(m_hWnd), _GetResourceInstance(),
						L"Confirm Execution",
						szInstruction,
						szMessage,
						L"Does not confirm next time",
						buttons,ARRAYSIZE(buttons),
						0,
						&SelectedButton,
						&VerificationFlag);

				if( SelectedButton != IDOK )
					return 0;

				if( VerificationFlag )
					m_bExecFileVerification = FALSE; // does not confirm next time
			}

			PWSTR pszFullPath;
			pszFullPath = CombinePath(m_pszCurDir,pItem->pFI->hdr.FileName);

			SELECT_ITEM sel = {0};
			sel.ViewType  = VOLUME_CONSOLE_HOME;
			sel.pszPath   = pszFullPath;
			sel.pszCurDir = DuplicateString(m_pszCurDir);
			sel.pszName   = DuplicateString(pItem->pFI->hdr.FileName);

			{
				SELECT_ITEM* pFile = &sel;
				WCHAR szDosPath[MAX_PATH];
				ZeroMemory(szDosPath,sizeof(szDosPath));

				if( PathIsPrefixDosDeviceDrive(pFile->pszPath) )
				{
					StringCchCopy(szDosPath,MAX_PATH,&pFile->pszPath[4]);
				}
				else if( HasPrefix(L"\\??\\",pFile->pszPath) )
				{
					StringCchCopy(szDosPath,MAX_PATH,L"\\\\?\\");
					StringCchCat(szDosPath,MAX_PATH,&pFile->pszPath[4]);
				}
				else if( NtPathToDosPath(pFile->pszPath,szDosPath,MAX_PATH) )
				{
					;
				}
				else
				{
					StringCchCopy(szDosPath,MAX_PATH,pFile->pszPath);
				}
		
				if( szDosPath[0] )
				{
					_OpenByExplorerEx(m_hWnd,szDosPath,NULL,FALSE);
				}
				else
				{
					MessageBeep(MB_ICONSTOP);
				}
			}

			FreeMemory(sel.pszCurDir);
			FreeMemory(sel.pszName);
			FreeMemory(pszFullPath);
		}

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Context Menu
	//

	virtual HRESULT MakeContextMenu(HMENU hMenu)
	{
		AppendMenu(hMenu,MF_STRING,ID_OPEN,L"&Open");
		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		SetMenuDefaultItem(hMenu,ID_OPEN,FALSE);
		return S_OK;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		HMENU hMenu = CreatePopupMenu();

		SendMessage(GetActiveWindow(),PM_MAKECONTEXTMENU,(WPARAM)hMenu,0);

		CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		MakeContextMenu(hMenu);

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,0,hMenu,pt,TPM_LEFTALIGN|TPM_TOPALIGN);

		DestroyMenu(hMenu);

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Miscellaneous Functions
	//
#if 0
	HRESULT MakeSelectedFileList(FS_SELECTED_FILELIST *pFileList)
	{
#if _ENABLE_FILE_TOOLS
		int cSelItems = ListView_GetSelectedCount(m_hWndList);

		if( cSelItems == 0 )
			return S_FALSE;

		CFileOperationList *pFiles = new CFileOperationList(32,16);

		CFileInfoItem *pItem;
		PWSTR pNtPath;
		int iItem = -1;
		while( (iItem = ListView_GetNextItem(m_hWndList,iItem,LVNI_SELECTED)) != -1 )
		{
			pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,iItem);

			ASSERT(pItem != NULL);

			if( pItem->pFI->hdr.Path != NULL )
				pNtPath = DuplicateString(pItem->pFI->hdr.Path);
			else
				pNtPath = CombinePath(GetCurPath(),pItem->pFI->hdr.FileName);

			pFiles->Add( pNtPath, pItem->pFI->FileAttributes );

			FreeMemory(pNtPath);
		}

		FS_FILELISTBUFFER *FileListBuffer;
		pFiles->CreateBuffer(&FileListBuffer,FOPFL_SET_NAME_FIELD_OFFSET);
		delete pFiles;

		pFileList->FileListBuffer = FileListBuffer;

		return S_OK;
#else
		return E_NOTIMPL;
#endif
	}
#endif

	int FindFileName(PCWSTR pszName)
	{
		int i,cItems;
		cItems = ListView_GetItemCount(m_hWndList);
		for(i = 0; i < cItems; i++)
		{
			CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,i);

			if( _wcsicmp(pItem->pFI->hdr.FileName,pszName) == 0 )
			{
				return i;
			}
		}
		return -1;
	}

	HRESULT GetSelectedFileInfo(FS_SELECTED_FILE *pPath)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
		{
			return S_FALSE;
		}

		CFileInfoItem *pItem = (CFileInfoItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		if( wcscmp(pItem->pFI->hdr.FileName,L".") == 0 )
			return S_FALSE;

		if( wcscmp(pItem->pFI->hdr.FileName,L"..") == 0 )
			return S_FALSE;

		PWSTR pszFullPath = CombinePath(m_pszCurDir,pItem->pFI->hdr.FileName);

		pPath->FileAttributes = pItem->pFI->FileAttributes;

		pPath->pszPath = (PWSTR)LocalAlloc(LPTR, (wcslen(pszFullPath)+1) * sizeof(WCHAR));
		wcscpy(pPath->pszPath,pszFullPath);

		FreeMemory(pszFullPath);

		return S_OK;
	}

	int SelectFileName(PCWSTR pszName)
	{
		int iItem = FindFileName(pszName);
		if( iItem != -1 )
		{
			ListView_SetItemState(m_hWndList,iItem,LVNI_SELECTED|LVNI_FOCUSED,LVNI_SELECTED|LVNI_FOCUSED);
		}
		return iItem;
	}

	int AppendFileItem( PCWSTR pszFileName )
	{
		CFileItem *pFI = CreateFileItem( pszFileName );
		if( pFI )
		{
			return Insert(m_hWndList,pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ? ID_GROUP_DIRECTORY : ID_GROUP_FILE,-1,pFI);
		}
		return -1;
	}

	CFileItem *CreateFileItem( PCWSTR pszFileName )
	{
		CFileItem *pFI = NULL;

		PWSTR pszFullPath;
		pszFullPath = CombinePath(m_pszCurDir,pszFileName);
		if( pszFullPath == NULL )
			return NULL;

		HANDLE hDirectory;

		if( OpenFileEx_W(&hDirectory,m_pszCurDir,
					GENERIC_READ|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,
					FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT) == STATUS_SUCCESS )
		{
			UNICODE_STRING usFileName;

			RtlInitUnicodeString(&usFileName,pszFileName);

			FS_FILE_DIRECTORY_INFORMATION fdi;
			if( GetDirectoryFileInformation_U(hDirectory,&usFileName,&fdi,NULL) == STATUS_SUCCESS )
			{
				pFI = new CFileItem(NULL,pszFileName);

				pFI->FileAttributes = fdi.FileAttributes;
				pFI->LastWriteTime  = fdi.LastWriteTime;
				pFI->CreationTime   = fdi.CreationTime;
				pFI->LastAccessTime = fdi.LastAccessTime;
				pFI->ChangeTime     = fdi.ChangeTime;
				pFI->EndOfFile      = fdi.EndOfFile;
				pFI->AllocationSize = fdi.AllocationSize;
				pFI->EaSize         = fdi.EaSize;
				pFI->FileId         = fdi.FileId;
				memcpy(pFI->ShortName,fdi.ShortName,fdi.ShortNameLength);
				pFI->ShortName[fdi.ShortNameLength/sizeof(WCHAR)] = UNICODE_NULL;
			}

			CloseHandle(hDirectory);
		}

		FreeMemory(pszFullPath);

		return pFI;
	}

	VOID InsertColumns(HWND hWndList,COLUMN_TABLE *pcoltbl)
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

	int _comp_name(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;

		if( wcscmp(pItem1->pFI->hdr.FileName,L"..") == 0 && wcscmp(pItem2->pFI->hdr.FileName,L"..") != 0 )
			return -1 * op->direction;
		else if( wcscmp(pItem1->pFI->hdr.FileName,L"..") != 0 && wcscmp(pItem2->pFI->hdr.FileName,L"..") == 0 )
			return 1 * op->direction;

		return StrCmpLogicalW(pItem1->pFI->hdr.FileName,pItem2->pFI->hdr.FileName);
	}

	int _comp_size(CFileInfoItem *p1,CFileInfoItem *p2, const void *p)
	{
		CFileInfoItem *pItem1 = (CFileInfoItem *)p1;
		CFileInfoItem *pItem2 = (CFileInfoItem *)p2;
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;

		LONGLONG cb1,cb2;

		switch( op->id )
		{
			case COLUMN_EndOfFile:
				cb1 = pItem1->pFI->EndOfFile.QuadPart;
				cb2 = pItem2->pFI->EndOfFile.QuadPart;
				break;
			case COLUMN_AllocationSize:
				cb1 = pItem1->pFI->AllocationSize.QuadPart;
				cb2 = pItem2->pFI->AllocationSize.QuadPart;
				break;
		}

		return _COMP(cb1,cb2);
	}

	int _comp_datetime(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;

		LONGLONG cb1,cb2;

		switch( op->id )
		{
			case COLUMN_LastWriteTime:
				cb1 = pItem1->pFI->LastWriteTime.QuadPart;
				cb2 = pItem2->pFI->LastWriteTime.QuadPart;
				break;
			case COLUMN_CreationTime:
				cb1 = pItem1->pFI->CreationTime.QuadPart;
				cb2 = pItem2->pFI->CreationTime.QuadPart;
				break;
			case COLUMN_LastAccessTime:
				cb1 = pItem1->pFI->LastAccessTime.QuadPart;
				cb2 = pItem2->pFI->LastAccessTime.QuadPart;
				break;
			case COLUMN_ChangeTime:
				cb1 = pItem1->pFI->ChangeTime.QuadPart;
				cb2 = pItem2->pFI->ChangeTime.QuadPart;
				break;
		}

		return _COMP(cb1,cb2);
	}

	int _comp_fileattributes(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		UINT f = m_columnShowStyleFlags[ COLUMN_FileAttributes ];

		if( f != 0 )
		{
			// hex mode
			return _COMP(pItem1->pFI->FileAttributes,pItem2->pFI->FileAttributes);
		}
		else
		{
			// attr character
			WCHAR sz1[32+1],sz2[32+1];
			GetAttributeString(pItem1->pFI->FileAttributes,sz1,ARRAYSIZE(sz1));
			GetAttributeString(pItem2->pFI->FileAttributes,sz2,ARRAYSIZE(sz2));
			return wcscmp(sz1,sz2);
		}
	}

	int _comp_ea(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		return _COMP(pItem1->pFI->EaSize,pItem2->pFI->EaSize);
	}

	int _comp_extension(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		if( _IS_DIRECTORY(pItem1->pFI->FileAttributes) && !_IS_DIRECTORY(pItem2->pFI->FileAttributes) )
			return -1;
		if( !_IS_DIRECTORY(pItem1->pFI->FileAttributes) && _IS_DIRECTORY(pItem2->pFI->FileAttributes) )
			return 1;

		PCWSTR pExt1 = PathFindExtension(pItem1->pFI->hdr.FileName);
		PCWSTR pExt2 = PathFindExtension(pItem2->pFI->hdr.FileName);
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;

		return _compare_pointer_string_logical(pExt1,pExt2,op->direction);
	}

	int _comp_shortname(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		PCWSTR pShortName1 = pItem1->pFI->ShortName;
		PCWSTR pShortName2 = pItem2->pFI->ShortName;
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;

		return _compare_pointer_string_logical(pShortName1,pShortName2,op->direction);
	}

	int _comp_fileid(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		// refer to only 64bit id.
		if( GetKeyState(VK_SHIFT) < 0 )
		{
			LONGLONG id1 = pItem1->pFI->FileId.QuadPart;
			LONGLONG id2 = pItem2->pFI->FileId.QuadPart;

			id1 &= 0xFFFFFFFFFFFF;
			id2 &= 0xFFFFFFFFFFFF;
	
			return _COMP(id1,id2);
		}
		return _COMP(pItem1->pFI->FileId.QuadPart,pItem2->pFI->FileId.QuadPart);
	}

	int _comp_path(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		return StrCmpLogicalW(pItem1->pFI->hdr.Path,pItem2->pFI->hdr.Path);
	}

	int _comp_lcn(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;
		if( pItem1->pFI->FirstLCN.QuadPart == 0 && pItem2->pFI->FirstLCN.QuadPart != 0 )
			return 1 * op->direction;
		if( pItem1->pFI->FirstLCN.QuadPart != 0 && pItem2->pFI->FirstLCN.QuadPart == 0 )
			return -1 * op->direction;
		return _COMP(pItem1->pFI->FirstLCN.QuadPart,pItem2->pFI->FirstLCN.QuadPart);
	}

	int _comp_physicaldrivenumber(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;
		if( pItem1->pFI->PhysicalDriveNumber == -1 && pItem2->pFI->PhysicalDriveNumber != -1 )
			return 1 * op->direction;
		if( pItem1->pFI->PhysicalDriveNumber != -1 && pItem2->pFI->PhysicalDriveNumber == -1 )
			return -1 * op->direction;
		return _COMP(pItem1->pFI->PhysicalDriveNumber,pItem2->pFI->PhysicalDriveNumber);
	}

	int _comp_physicaldriveoffset(CFileInfoItem *pItem1,CFileInfoItem *pItem2, const void *p)
	{
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;
		if( pItem1->pFI->PhysicalDriveOffset.QuadPart == -1 && pItem2->pFI->PhysicalDriveOffset.QuadPart != -1 )
			return 1 * op->direction;
		if( pItem1->pFI->PhysicalDriveOffset.QuadPart != -1 && pItem2->pFI->PhysicalDriveOffset.QuadPart == -1 )
			return -1 * op->direction;
		return _COMP(pItem1->pFI->PhysicalDriveOffset.QuadPart,pItem2->pFI->PhysicalDriveOffset.QuadPart);
	}

	virtual void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF<CFileListPage,CFileInfoItem> _comp_proc[] = 
		{
			{COLUMN_Name,                &CFileListPage::_comp_name},
			{COLUMN_LastWriteTime,       &CFileListPage::_comp_datetime},
			{COLUMN_CreationTime,        &CFileListPage::_comp_datetime},
			{COLUMN_LastAccessTime,      &CFileListPage::_comp_datetime},
			{COLUMN_ChangeTime,          &CFileListPage::_comp_datetime},
			{COLUMN_EndOfFile,           &CFileListPage::_comp_size},
			{COLUMN_AllocationSize,      &CFileListPage::_comp_size},
			{COLUMN_FileAttributes,      &CFileListPage::_comp_fileattributes},
			{COLUMN_EaSize,              &CFileListPage::_comp_ea},
			{COLUMN_ShortName,           &CFileListPage::_comp_shortname},
			{COLUMN_Extension,           &CFileListPage::_comp_extension},
			{COLUMN_FileId,              &CFileListPage::_comp_fileid},
			{COLUMN_Lcn,                 &CFileListPage::_comp_lcn},
			{COLUMN_PhysicalDriveNumber, &CFileListPage::_comp_physicaldrivenumber},
			{COLUMN_PhysicalDriveOffset, &CFileListPage::_comp_physicaldriveoffset},
		};

		int i;
		for(i = 0; i < _countof(_comp_proc); i++)
		{
			m_comp_proc[ _comp_proc[i].colid ].colid  = _comp_proc[i].colid;
			m_comp_proc[ _comp_proc[i].colid ].proc   = _comp_proc[i].proc;
		}
	}

	void alloc_comp_proc_table()
	{
		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CFileListPage,CFileInfoItem>[COLUMN_MaxItem];
		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF<CFileListPage,CFileInfoItem>)*COLUMN_MaxItem);
	}

	virtual int CompareItem(CFileInfoItem *pItem1,CFileInfoItem *pItem2,SORT_PARAM<CFileListPage> *op)
	{
		if( m_comp_proc == NULL )
		{
			alloc_comp_proc_table();
			init_compare_proc_def_table();
		}

		if( _PARENT_DIRECTORY(pItem1->pFI->hdr.FileName) && !_PARENT_DIRECTORY(pItem2->pFI->hdr.FileName) )
			return -1;
		if( !_PARENT_DIRECTORY(pItem1->pFI->hdr.FileName) && _PARENT_DIRECTORY(pItem2->pFI->hdr.FileName) )
			return 1;

		int iResult = 0;

		if( op->directory_align )
		{
			if( _IS_DIRECTORY(pItem1->pFI->FileAttributes) && !_IS_DIRECTORY(pItem2->pFI->FileAttributes) )
				iResult = -1;
			if( !_IS_DIRECTORY(pItem1->pFI->FileAttributes) && _IS_DIRECTORY(pItem2->pFI->FileAttributes) )
				iResult = 1;
		}

		if( iResult == 0 && m_comp_proc[op->id].proc != NULL )
		{
			iResult = (this->*m_comp_proc[op->id].proc)(pItem1,pItem2,op);
		}

		iResult *= op->direction;

		return iResult;
	}

	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		CFileInfoItem *pItem1 = (CFileInfoItem *)lParam1;
		CFileInfoItem *pItem2 = (CFileInfoItem *)lParam2;
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)lParamSort;
		return op->pThis->CompareItem(pItem1,pItem2,op);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Text Handler
	//

	virtual HRESULT GetString(int t,LPWSTR psz,int cch)
	{
#if 0
		if( t == VIEW_STR_TITLE )
		{
			PCWSTR pszName;
			if( m_pszCurDir && *m_pszCurDir && !IsRootDirectory_W(m_pszCurDir) )
				pszName = FindFileName_W(m_pszCurDir);
			else
				pszName = m_pszCurDir;

			return StringCchCopy(psz,cch,pszName);
		}
#endif
		return S_FALSE;
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
			case ID_OPEN:
				*puState = ListView_GetSelectedCount(m_hWndList) ?  UPDUI_ENABLED : UPDUI_DISABLED;
				break;
			case ID_UP_DIR:
#if 0
				*puState = UPDUI_ENABLED;
#else
				*puState = NtPathIsRootDirectory(m_pszCurDir) ? UPDUI_DISABLED : UPDUI_ENABLED;
#endif
				break;
			case ID_GOTO_DIRECTORY:
			case ID_VIEW_REFRESH:
				*puState = UPDUI_ENABLED;
				break;
			case ID_OPEN_LOCATION_EXPLORER:
			case ID_OPEN_LOCATION_CMDPROMPT:
			case ID_OPEN_LOCATION_POWERSHELL:
			case ID_OPEN_LOCATION_TERMINAL:
			case ID_OPEN_LOCATION_BASH:
				*puState = UPDUI_ENABLED;
				break;
			case ID_HISTORY_BACKWARD:
			case ID_HISTORY_FORWARD:
				*puState = UPDUI_ENABLED;
				break;
			default:
				return S_FALSE;
		}
		return S_OK;
	}

	VOID OnUpDir()
	{
		LRESULT lResult;
		CStringBuffer szDirPath( _NT_PATH_FULL_LENGTH );

		lResult = SendMessage(m_hWnd,PM_GETWORKINGDIRECTORY,szDirPath.GetBufferSize(),(LPARAM)szDirPath.c_str());

		if( lResult == 1 )
		{
			// NTFS Special File
			UNICODE_STRING usRoot;
			SplitRootRelativePath(szDirPath,&usRoot,NULL);
			HANDLE hFile;
			OpenFile_U(&hFile,NULL,&usRoot,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0);
			LARGE_INTEGER liRoot;
			::GetFileId(hFile,&liRoot);
			CloseHandle(hFile);

			LARGE_INTEGER li;
			SendMessage(m_hWnd,PM_GETWORKINGDIRECTORY,0,(LPARAM)&li);
			if( li.QuadPart != liRoot.QuadPart )
			{
				UNICODE_STRING usPath;
				UNICODE_STRING usFileName;

				RtlInitUnicodeString(&usPath,szDirPath);
				SplitPathFileName_U(&usPath,&usFileName);
				RemoveBackslash_U(&usPath);

				PWSTR ParentPath = AllocateSzFromUnicodeString(&usPath);
				PWSTR ChooseFileName = AllocateSzFromUnicodeString(&usFileName);

				SELECT_ITEM sel = {0};
				sel.ViewType  = GetConsoleId();
				sel.mask = SI_MASK_FILEID;
				sel.Flags = _FLG_NTFS_SPECIALFILE;
				sel.FileId.dwSize = sizeof(FILE_ID_DESCRIPTOR);
				sel.FileId.Type = FileIdType;
				sel.pszPath = ParentPath;
				sel.pszName = ChooseFileName;
				sel.FileId.FileId = li;
				SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);

				FreeMemory(ParentPath);
				FreeMemory(ChooseFileName);
			}
			else
			{
				UNICODE_STRING usPath;
				UNICODE_STRING usFileName;
				RtlInitUnicodeString(&usPath,szDirPath);
				SplitRootRelativePath_U(&usPath,&usRoot,&usFileName);
				PWSTR ParentPath = AllocateSzFromUnicodeString(&usRoot);
				PWSTR ChooseFileName = AllocateSzFromUnicodeString(&usFileName);

				SELECT_ITEM sel = {0};
				sel.ViewType  = GetConsoleId();
				sel.pszPath = ParentPath;
				sel.pszName = ChooseFileName;
				SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);

				FreeMemory(ParentPath);
				FreeMemory(ChooseFileName);
			}

			return ;
		}

		if( IsRootDirectory_W(szDirPath) )
		{
#if 0
			if( 0 )
			{
				// change to root directories page
				UIS_PAGE pg = {0};
				pg.ConsoleTypeId = VOLUME_CONSOLE_FILE_ROOTDIRECTORIES;
				pg.pszPath = szDirPath;
				PWSTR pszBuffer = _MemAllocString(szDirPath);
				if( pszBuffer )
				{
					RemoveBackslash(pszBuffer);
					PWSTR pName = wcsrchr(pszBuffer,L'\\');
					if( pName && (*(++pName) != L'\0') )
						pg.pszFileName = pName;
				}

				SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,MAKEWPARAM(UI_SELECT_PAGE,0),(LPARAM)&pg);

				_SafeMemFree(pszBuffer);
			}
			else
#endif
			{
				// no change, stay page and show root directory of current volume.
				WCHAR *pName = NULL;
				WCHAR szName[MAX_PATH];
				StringCchCopy(szName,MAX_PATH,szDirPath.c_str());
				RemoveBackslash(szName);
				pName = wcsrchr(szName,L'\\');
				if( pName )
					pName++;

				SELECT_ITEM sel = {0};
				sel.Flags = SI_FLAG_ROOT_DIRECTORY;
				sel.ViewType  = GetConsoleId();
				sel.pszPath   = L"";//szDirPath;
				sel.pszCurDir = L"";//szDirPath;
				sel.pszName   = pName;
				SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);
			}
		}
		else
		{
			SELECT_ITEM sel = {0};
			sel.ViewType  = GetConsoleId();
			sel.pszCurDir = szDirPath;
			sel.pszName   = L"..";
			SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);
		}
	}

	VOID OnHistoryBackward()
	{
		HISTORY_ITEM item;
		if( m_history.Back(&item) )
		{
			if( item.pszFileName == NULL )
			{
				SELECT_ITEM sel = {0};
				sel.Flags = SI_FLAG_NOT_ADD_TO_HISTORY;
				sel.ViewType  = GetConsoleId();
				sel.pszPath   = item.pszPath;
				sel.pszName   = NULL;
				SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);
			}
			else
			{
				SELECT_ITEM sel = {0};
				sel.Flags = SI_FLAG_NOT_ADD_TO_HISTORY;
				sel.ViewType  = GetConsoleId();
				sel.pszPath   = item.pszPath;
				sel.pszName   = item.pszFileName;
				SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);
			}
		}
	}
	
	VOID OnHistoryForward()
	{
		HISTORY_ITEM item;
		if( m_history.Forward(&item) )
		{
			SELECT_ITEM sel = {0};
			sel.Flags = SI_FLAG_NOT_ADD_TO_HISTORY;
			sel.ViewType  = GetConsoleId();
			sel.pszPath   = item.pszPath;
			sel.pszName   = NULL;
			SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);
		}
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_UP_DIR:
				OnUpDir();
				break;
			case ID_HISTORY_BACKWARD:
				OnHistoryBackward();
				break;
			case ID_HISTORY_FORWARD:
				OnHistoryForward();
				break;
			case ID_EDIT_COPY:
				OnEditCopyText();
				break;
			case ID_VIEW_REFRESH:
				OnRefresh();
				break;
			case ID_GOTO_DIRECTORY:
				OnGotoDirectory();
				break;
			case ID_OPEN_LOCATION_EXPLORER:
			case ID_OPEN_LOCATION_CMDPROMPT:
			case ID_OPEN_LOCATION_POWERSHELL:
			case ID_OPEN_LOCATION_TERMINAL:
			case ID_OPEN_LOCATION_BASH:
				OnOpenLocation( CmdId );
				break;
			case ID_OPEN:
				OnOpenItem();
				break;
#if _ENABLE_DRAG_AND_DROP_SUPPORT
			case ID_EDIT_PASTE:
				OnPopClipboardFileName();
				break;
#endif
			default:
#if _ENABLE_FILE_TOOLS
				return CFileCommandHandler::InvokeCommand(CmdId);
#else
				return S_FALSE;
#endif
		}
		return S_OK;
	}

	void OnEditCopy()
	{
#if _ENABLE_DRAG_AND_DROP_SUPPORT
		OnPushClipboardFileName(0,0,NULL);
#endif
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

	void OnGotoDirectory()
	{
		PWSTR pszNewPath;
		if( GotoDirectoryOnSameVolumeDialog(m_hWnd,m_pszCurDir,&pszNewPath,0) == S_OK )
		{
			SELECT_ITEM sel = {0};
			sel.ViewType  = GetConsoleId();
			sel.pszPath   = pszNewPath;
			sel.pszCurDir = NULL;
			sel.pszName   = NULL;
			SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);

			WCHAR szVolumeName[MAX_PATH];
			if( NtPathGetVolumeName(pszNewPath,szVolumeName,MAX_PATH) == 0 )
			{
				if( NtPathGetVolumeName(m_pszCurDir,szVolumeName,MAX_PATH) == 0 )
				{
					StringCchCopy(szVolumeName,MAX_PATH,L"Unknown Volume");
				}
			}

			CoTaskMemFree(pszNewPath);
		}
	}

	void OnOpenLocation(UINT uCmdId)
	{
		int op;

		switch( uCmdId )
		{
			case ID_OPEN_LOCATION_EXPLORER:
				op = OpenVolumeLocationWithExplorer;
				break;
			case ID_OPEN_LOCATION_CMDPROMPT:
				op = OpenVolumeLocationWithCommandPrompt;
				break;
			case ID_OPEN_LOCATION_POWERSHELL:
				op = OpenVolumeLocationWithPowerShell;
				break;
			case ID_OPEN_LOCATION_TERMINAL:
				op = OpenVolumeLocationWithTerminal;
				break;
			case ID_OPEN_LOCATION_BASH:
				op = OpenVolumeLocationWithBash;
				break;
			default:
				return;
		}

		if( GetKeyState(VK_SHIFT) < 0 )
			op |= OpenVolumeLocationWithAdmin;

		CStringBuffer dos(32768);
		NtPathToDosPath(m_pszCurDir,dos,dos.GetBufferSize());

		OpenVolumeLocationByShell(GetActiveWindow(),op,dos,NULL);
	}

	void OnOpenItem()
	{
		OpenItem( ListViewEx_GetCurSel(m_hWndList) );
	}

	void OnRefresh()
	{
		SELECT_ITEM sel = {0};

		sel.ViewType  = GetConsoleId();
		sel.pszCurDir = (PWSTR)_MemAllocString(this->m_pszCurDir);
		sel.pszPath   = (PWSTR)_MemAllocString(this->m_pszCurDir);
		sel.pszName   = (PWSTR)NULL;

		FillItems(&sel);

		_SafeMemFree( sel.pszCurDir );
		_SafeMemFree( sel.pszPath );
	}

	//
	// Directory Watch Notify Callback Proc
	//
	// NOTE: This function calls under context of the watch thread.
	//
	VOID CallbackProc(DWORD Action,PWSTR FileName,PWSTR NewFileName)
	{
		switch( Action )
		{
			case FILE_ACTION_MODIFIED:
			{
				HANDLE hDirectory;
				if( OpenFile_W(&hDirectory,NULL,m_pszCurDir,
						GENERIC_READ|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,
						FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT) == STATUS_SUCCESS )
				{
					UNICODE_STRING usFileName;

					RtlInitUnicodeString(&usFileName,FileName);

					FS_FILE_DIRECTORY_INFORMATION fdi;
					if( GetDirectoryFileInformation_U(hDirectory,&usFileName,&fdi,NULL) == STATUS_SUCCESS )
					{
						CFileItem *pFI = new CFileItem(NULL,FileName);

						pFI->FileAttributes = fdi.FileAttributes;
						pFI->LastWriteTime  = fdi.LastWriteTime;
						pFI->CreationTime   = fdi.CreationTime;
						pFI->LastAccessTime = fdi.LastAccessTime;
						pFI->ChangeTime     = fdi.ChangeTime;
						pFI->EndOfFile      = fdi.EndOfFile;
						pFI->AllocationSize = fdi.AllocationSize;
						pFI->EaSize         = fdi.EaSize;
						pFI->FileId         = fdi.FileId;
						memcpy(pFI->ShortName,fdi.ShortName,fdi.ShortNameLength);
						pFI->ShortName[fdi.ShortNameLength/sizeof(WCHAR)] = UNICODE_NULL;

						PostMessage(m_hWnd, PM_UPDATE_FILELIST,MAKEWPARAM(Action,0),(LPARAM)pFI);
					}

					CloseHandle(hDirectory);
				}
				break;
			}
			case FILE_ACTION_ADDED:
			case FILE_ACTION_RENAMED_NEW_NAME:
			{
				CFileItem *pFI = CreateFileItem( FileName );
				if( pFI )
				{
					PostMessage(m_hWnd, PM_UPDATE_FILELIST,MAKEWPARAM(Action,0),(LPARAM)pFI);
				}
				break;
			}
			case FILE_ACTION_REMOVED:
			{
				// NOTE:
				// In Windows 10, renaming causes FILE_ACTION_REMOVED, so I added a workaround.
				// It occurs low frequently with normal renaming, but almost always occurs with case changes.
				BOOLEAN bExists = FALSE;
				PWSTR FullPath = CombinePath(m_pszCurDir,FileName);
				if( FullPath )
				{
					// FILE_ACTION_REMOVED occurred, but the file still existing.
					// In this case, it is determined that the rename operation.
					bExists = PathFileExists_W(FullPath,NULL);
					FreeMemory(FullPath);
				}
				if( bExists )
					break;
				// pass through to the next case block.
			}
			case FILE_ACTION_RENAMED_OLD_NAME:
			{
				PWSTR pszFileName = _MemAllocString(FileName);
				PostMessage(m_hWnd, PM_UPDATE_FILELIST,MAKEWPARAM(Action,0),(LPARAM)pszFileName);
				break;
			}
			case _FILE_ACTION_RENAME:
			{
				SIZE_T cchOldFileName = wcslen(FileName);
				SIZE_T cchNewFileName = wcslen(NewFileName);
				SIZE_T cchLength = (cchOldFileName + 1) + (cchNewFileName + 1) + 1;
				PWSTR pmszFileNameSet = _MemAllocStringBuffer( cchLength );

				ZeroMemory(pmszFileNameSet,cchLength * sizeof(WCHAR));

				// pmszFileNameSet="OldFileName\0NewFileName\0\0"
				memcpy(pmszFileNameSet,FileName,cchOldFileName*sizeof(WCHAR));
				memcpy(&pmszFileNameSet[cchOldFileName+1],NewFileName,cchNewFileName*sizeof(WCHAR));

				PostMessage(m_hWnd, PM_UPDATE_FILELIST,MAKEWPARAM(Action,0),(LPARAM)pmszFileNameSet);
			}
		}
	}

	static HRESULT CALLBACK NotifyCallback(DIRWATCHNOTIFYEVENT *Event)
	{
		WCHAR szFileName[MAX_PATH];
		WCHAR *pszNewFileName = NULL;
		DWORD Action;

		CFileListPage *pThis = (CFileListPage *)Event->Context;

		//
		// NOTE:
		// The order of the first member variables is assumed to be the same each structure.
		//
		FILE_NOTIFY_INFORMATION *pNotify;

		pNotify = Event->pNotifyBuffer;

		while( pNotify != NULL )
		{
			memcpy(szFileName,pNotify->FileName,pNotify->FileNameLength);
			szFileName[ pNotify->FileNameLength / sizeof(WCHAR) ] = 0;

			Action = pNotify->Action;

			if( Action == FILE_ACTION_RENAMED_OLD_NAME && pNotify->NextEntryOffset != 0 )
			{
				FILE_NOTIFY_INFORMATION *pNextNotify = (FILE_NOTIFY_INFORMATION *)((ULONG_PTR)pNotify + pNotify->NextEntryOffset);
				if( pNextNotify->Action == FILE_ACTION_RENAMED_NEW_NAME )
				{
					if( 0 )
					{
						pszNewFileName = (WCHAR*)_MemAllocZero( pNextNotify->FileNameLength + sizeof(WCHAR) );
						if( pszNewFileName )
						{
							memcpy(pszNewFileName,pNextNotify->FileName,pNextNotify->FileNameLength);
							pNotify = pNextNotify;
							Action = _FILE_ACTION_RENAME;
						}
					}
					else if( Event->InformationClass == DirectoryNotifyExtendedInformation )
					{
						FILE_NOTIFY_EXTENDED_INFORMATION *pNextNotifyEx = (FILE_NOTIFY_EXTENDED_INFORMATION *)pNextNotify;
						pszNewFileName = (WCHAR*)_MemAllocZero( pNextNotifyEx->FileNameLength + sizeof(WCHAR) );
						if( pszNewFileName )
						{
							memcpy(pszNewFileName,pNextNotifyEx->FileName,pNextNotifyEx->FileNameLength);
							pNotify = (FILE_NOTIFY_INFORMATION *)pNextNotifyEx;
							Action = _FILE_ACTION_RENAME;
						}
					}
				}
			}

			pThis->CallbackProc(Action,szFileName,pszNewFileName);

			if( pszNewFileName )
			{
				_MemFree(pszNewFileName);
				pszNewFileName = NULL;
			}

			if( pNotify->NextEntryOffset == 0 )
				break;

			pNotify = (FILE_NOTIFY_INFORMATION *)((ULONG_PTR)pNotify + pNotify->NextEntryOffset);
		}
		return S_OK;
	}
};