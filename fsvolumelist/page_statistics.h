#pragma once
//***************************************************************************
//*                                                                         *
//*  page_statistics.h                                                      *
//*                                                                         *
//*  Fils System Statistics Page                                            *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-01-22                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
// --------------------------------------------------------------------------
//
//  Remarks:
//
//  The MFT, MFT mirror, root index, user index, bitmap, and MFT bitmap 
//  are counted as metadata files. The log file is not counted as a 
//  metadata file.
//
//  The number of read and write operations measured is the number of 
//  paging operations.
//
//  ex)
//  fsutil fsinfo statistics c:
//
#include "stdafx.h"
#include "string_def.h"
#include "ntobjecthelp.h"

#define _ENABLE_DIFF_COLUMN         0 // reserved
#define _ENABLE_DEBUG_INFORMATION   0 // reserved

#define _INDENT_SIZE  4

typedef struct _STATISTICS_ITEM_DEF
{
	UINT offset;
	UINT size;
	UINT data_id;
} STATISTICS_ITEM_DEF;

typedef struct _STATISTICS_REF_ITEM
{
	STATISTICS_ITEM_DEF def;
} STATISTICS_REF_ITEM;

typedef struct _STATISTICS_LIST_ITEM
{
	STATISTICS_REF_ITEM *tbl;
	STATISTICS_REF_ITEM *tblex;
} STATISTICS_LIST_ITEM;

__inline FILESYSTEM_STATISTICS *GetStatisticsPtr(PVOID pv)
{
	return ((FILESYSTEM_STATISTICS *)pv);
}

#define offsetof(s,m)   (size_t)( (char *)&(((s *)0)->m) - (char *)0 )

#define _DEF_STATISTICS(t,member,di)            { offsetof(FILESYSTEM_STATISTICS,member), sizeof(t), di }
#define _DEF_STATISTICS_EX(t,member,di)         { offsetof(FILESYSTEM_STATISTICS_EX,member), sizeof(t), di }
#define _DEF_STATISTICS_NTFS(t,member,di)       { offsetof(NTFS_STATISTICS,member), sizeof(t), di }
#define _DEF_STATISTICS_NTFS_WIN8(t,member,di)  { offsetof(NTFS_STATISTICS_WIN8,member), sizeof(t), di }
#define _DEF_STATISTICS_NTFS_EX(t,member,di)    { offsetof(NTFS_STATISTICS_EX,member), sizeof(t), di }
#define _DEF_STATISTICS_FAT(t,member,di)        { offsetof(FAT_STATISTICS,member), sizeof(t), di }

class CFileSystemStatisticsPage : public CPageWndBase
{
	HWND m_hWndList;

	HFONT m_hFont;

	SYSTEM_INFO m_si;

	STATISTICS_REF_ITEM *m_tblStatistics;
	STATISTICS_REF_ITEM *m_tblStatisticsEx;

	STATISTICS_REF_ITEM *m_tblMetaData;
	STATISTICS_REF_ITEM *m_tblMetaDataEx;

	PBYTE m_Statistics;
	PBYTE *m_StatisticsPtrArray;

	SIZE_T m_StatisticsExSize;
	FILESYSTEM_STATISTICS_EX *m_StatisticsEx;
#if _ENABLE_DIFF_COLUMN
	FILESYSTEM_STATISTICS_EX *m_StatisticsExDiff;
#endif

	PWSTR m_pszVolumeRoot;
	PWSTR m_pszErrorMessage;

enum {
	ID_GROUP_GENERIC=1,
	ID_GROUP_NTFS,
	ID_GROUP_EXFAT,
	ID_GROUP_FAT,
	ID_GROUP_REFS,
};

public:
	CFileSystemStatisticsPage()
	{
		m_hWndList = NULL;
		m_hFont = NULL;

		m_Statistics = NULL;
		m_StatisticsPtrArray = NULL;

		m_StatisticsEx = NULL;
#if _ENABLE_DIFF_COLUMN
		m_StatisticsExDiff = NULL;
#endif

		m_tblStatistics = NULL;
		m_tblStatisticsEx = NULL;
		m_tblMetaData = NULL;
		m_tblMetaDataEx = NULL;

		m_pszVolumeRoot = NULL;
		m_pszErrorMessage = NULL;

		GetSystemInfo(&m_si);
	}

	~CFileSystemStatisticsPage()
	{
		_SafeMemFree(m_pszErrorMessage);

		_SafeMemFree(m_tblStatistics);
		_SafeMemFree(m_tblStatisticsEx);
		_SafeMemFree(m_tblMetaData);
		_SafeMemFree(m_tblMetaDataEx);

		FreeStatisticsData(m_Statistics,(PFILESYSTEM_STATISTICS *)m_StatisticsPtrArray,m_StatisticsEx);
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | LVS_SINGLESEL, 
                              0,0,0,0,
                              hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		InitList(m_hWndList);
		InitGroup();

		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_hFont )
			DeleteObject( m_hFont );
		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnSetFocus(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SetFocus(m_hWndList);
		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		HMENU hMenu = CreatePopupMenu();

		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");

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
				return OnCustomDraw(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case LVN_ITEMCHANGED:
				return OnItemChanged(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case LVN_ITEMACTIVATE:
				return OnItemActivate(pnmhdr);
			case LVN_KEYDOWN:
				return OnKeyDown(pnmhdr);
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		delete (STATISTICS_LIST_ITEM *)pnmlv->lParam;

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

		if( pnmlvcd->nmcd.hdr.hwndFrom != m_hWndList )
			return CDRF_DODEFAULT;

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			SelectObject(pnmlvcd->nmcd.hdc,m_hFont);
			return CDRF_NEWFONT;
		}

		return CDRF_DODEFAULT;
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

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
		{
			int cxList = cx;
			int cyList = cy;

			SetWindowPos(m_hWndList,NULL,
					0,
					0,
					cxList,
					cyList,
					SWP_NOZORDER);
		}
	}

	int InsertGroup(HWND hWndList,int iGroupId,LPCWSTR pszHeaderText,BOOL fCollapsed=FALSE,int iImage=I_IMAGENONE,LPCWSTR pszSubTitle=NULL)
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
		UINT idGroupTitle;
		int fCollapsed;
		PCWSTR Text;
	} GROUP_ITEM;

	void InitGroup()
	{
		GROUP_ITEM Group[] = {
			{ ID_GROUP_GENERIC, 0, 0,L"Statistics" },
			{ ID_GROUP_NTFS,    0, 0,L"NTFS Meta Data Statistics" },
			{ ID_GROUP_EXFAT,   0, 0,L"ExFat Meta Data Statistics"  },
			{ ID_GROUP_FAT,     0, 0,L"FAT Meta Data Statistics" },
			{ ID_GROUP_REFS,    0, 0,L"Refs Meta Data Statistics" },
		};
		int cGroupItem = ARRAYSIZE(Group);

		for(int i = 0; i < cGroupItem; i++)
		{
			InsertGroup(m_hWndList,Group[i].idGroup,Group[i].Text,Group[i].fCollapsed);
		}
	}

	HRESULT InitList(HWND hWndList)
	{
		_EnableVisualThemeStyle(hWndList);

		m_hFont = GetGlobalFont(m_hWnd);
		
		ListView_SetExtendedListViewStyle(hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP|LVS_EX_GRIDLINES);

		HIMAGELIST himl = ImageList_Create(1,18,ILC_COLOR32,1,1);
		ListView_SetImageList(hWndList,himl,LVSIL_SMALL);

		InitColumns(hWndList);

		ListView_EnableGroupView(hWndList,TRUE);
		ListView_SetSelectedColumn(hWndList,0);

		return S_OK;
	}

	BOOL InitColumns(HWND hWndList)
	{
		HWND hwndHeader;
		hwndHeader = ListView_GetHeader(hWndList);

		ASSERT(hwndHeader != NULL);

		int cxTitleWidth = 280;
		int cxWidth = 160;
		int iCol = 0;

		HD_ITEM hdi = {0};
		hdi.mask = HDI_LPARAM;

		LVCOLUMN lvc = {0};
		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		lvc.fmt     = 0;

		lvc.pszText = L"Item";
		lvc.cx      = cxTitleWidth;
		ListView_InsertColumn(hWndList,iCol,&lvc);
		hdi.lParam = MAKELPARAM(0,0);
		Header_SetItem(hwndHeader,iCol++,&hdi);

		lvc.pszText = L"Total";
		lvc.cx      = cxWidth;
		lvc.fmt     = LVCFMT_RIGHT;
		ListView_InsertColumn(hWndList,iCol,&lvc);
		hdi.lParam = MAKELPARAM(0,1);
		Header_SetItem(hwndHeader,iCol++,&hdi);

#if _ENABLE_DIFF_COLUMN
		lvc.pszText = L"Diff";
		lvc.cx      = cxWidth;
		lvc.fmt     = LVCFMT_RIGHT;
		ListView_InsertColumn(hWndList,iCol++,&lvc);
		hdi.lParam = MAKELPARAM(1,1);
		Header_SetItem(hwndHeader,2,&hdi);
#endif

		WCHAR szName[8];
		for(DWORD i = 0; i < m_si.dwNumberOfProcessors; i++ )
		{
			StringCchPrintf(szName,_countof(szName),L"CPU#%u",i+1);
			lvc.pszText = szName;
			lvc.cx      = cxWidth;
			lvc.fmt     = LVCFMT_RIGHT;
			ListView_InsertColumn(hWndList,iCol,&lvc);
			hdi.lParam = MAKELPARAM(i,0);
			Header_SetItem(hwndHeader,iCol++,&hdi);
		}

		return TRUE;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	LRESULT OnDisp_StatisticsTotalData(NMLVDISPINFO *pnmlvdi)
	{
		STATISTICS_LIST_ITEM *pLine = (STATISTICS_LIST_ITEM *)pnmlvdi->item.lParam;
		ULONGLONG dt = 0;

		if( pLine->tblex )
		{
			STATISTICS_REF_ITEM *tblex = pLine->tblex;
			memcpy(&dt,((PBYTE)m_StatisticsEx) + tblex->def.offset,tblex->def.size);
			_CommaFormatString(dt,pnmlvdi->item.pszText);
		}

		return 0;
	}

#if _ENABLE_DIFF_COLUMN
	LRESULT OnDisp_StatisticsTotalDataDiff(NMLVDISPINFO *pnmlvdi)
	{
		STATISTICS_LIST_ITEM *pLine = (STATISTICS_LIST_ITEM *)pnmlvdi->item.lParam;
		ULONGLONG dt = 0;

		STATISTICS_REF_ITEM *tblex = pLine->tblex;

		if( tblex )
		{
			memcpy(&dt,((PBYTE)m_StatisticsExDiff) + tblex->def.offset,tblex->def.size);

			if( dt != 0 )
			{
				_CommaFormatString(dt,pnmlvdi->item.pszText);
			}
		}
		return 0;
	}
#endif

	LRESULT OnDisp_StatisticsData(NMLVDISPINFO *pnmlvdi,int cpu)
	{
		STATISTICS_LIST_ITEM *pLine = (STATISTICS_LIST_ITEM *)pnmlvdi->item.lParam;
		ULONGLONG dt = 0;

		*pnmlvdi->item.pszText = L'\0';

		if( pLine->tbl )
		{
			STATISTICS_REF_ITEM *tbl = pLine->tbl;

			if( tbl->def.size != 0 )
			{
				memcpy(&dt,m_StatisticsPtrArray[cpu] + tbl->def.offset,tbl->def.size);

				_CommaFormatString(dt,pnmlvdi->item.pszText);
			}
			else
			{
				wcscpy(pnmlvdi->item.pszText,L"-"); // no item
			}
		}
		else
		{
			wcscpy(pnmlvdi->item.pszText,L""); // Group header
		}

		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		if( pdi->item.mask & LVIF_TEXT )
		{
			if( pdi->item.lParam == 0 )
				return 0;

			HD_ITEM hdi = {0};
			hdi.mask = HDI_LPARAM;
			Header_GetItem(ListView_GetHeader(pnmhdr->hwndFrom),pdi->item.iSubItem,&hdi);

			int total = HIWORD(hdi.lParam);
			int index = LOWORD(hdi.lParam);

			if( total )
			{
				if( index == 0 )
					OnDisp_StatisticsTotalData(pdi);
#if _ENABLE_DIFF_COLUMN
				else if( index == 1 )
					OnDisp_StatisticsTotalDataDiff(pdi);
#endif
			}
			else
			{
				if( index >= 0 )
				{
					OnDisp_StatisticsData(pdi,index);
				}
			}
		}

		return 0;	
	}

	INT GetItemData(STATISTICS_ITEM_DEF **d)
	{
		static STATISTICS_ITEM_DEF def[] = {
#if _ENABLE_DEBUG_INFORMATION
		_DEF_STATISTICS(WORD,  FileSystemType,           diFileSystemType),
		_DEF_STATISTICS(WORD,  Version,                  diStatisticsDataVersion),
		_DEF_STATISTICS(DWORD, SizeOfCompleteStructure,  diSizeOfCompleteStructure),
#endif
		_DEF_STATISTICS(DWORD, UserFileReads,            diUserFileReads),
		_DEF_STATISTICS(DWORD, UserFileReadBytes,        diUserFileReadBytes),
		_DEF_STATISTICS(DWORD, UserDiskReads,            diUserDiskReads),
		_DEF_STATISTICS(DWORD, UserFileWrites,           diUserFileWrites),
		_DEF_STATISTICS(DWORD, UserFileWriteBytes,       diUserFileWriteBytes),
		_DEF_STATISTICS(DWORD, UserDiskWrites,           diUserDiskWrites),
		_DEF_STATISTICS(DWORD, MetaDataReads,            diMetaDataReads),
		_DEF_STATISTICS(DWORD, MetaDataReadBytes,        diMetaDataReadBytes),
		_DEF_STATISTICS(DWORD, MetaDataDiskReads,        diMetaDataDiskReads),
		_DEF_STATISTICS(DWORD, MetaDataWrites,           diMetaDataWrites),
		_DEF_STATISTICS(DWORD, MetaDataWriteBytes,       diMetaDataWriteBytes),
		_DEF_STATISTICS(DWORD, MetaDataDiskWrites,       diMetaDataDiskWrites),
		};
		*d = def;
		return _countof(def);
	}

	INT GetItemDataEx(STATISTICS_ITEM_DEF **d)
	{
		static STATISTICS_ITEM_DEF def[] = {
#if _ENABLE_DEBUG_INFORMATION
		_DEF_STATISTICS_EX(USHORT,    FileSystemType,          diFileSystemType),
		_DEF_STATISTICS_EX(USHORT,    Version,                 diStatisticsDataVersion),
		_DEF_STATISTICS_EX(ULONG,     SizeOfCompleteStructure, diSizeOfCompleteStructure),
#endif
		_DEF_STATISTICS_EX(ULONGLONG, UserFileReads,           diUserFileReads),
		_DEF_STATISTICS_EX(ULONGLONG, UserFileReadBytes,       diUserFileReadBytes),
		_DEF_STATISTICS_EX(ULONGLONG, UserDiskReads,           diUserDiskReads),
		_DEF_STATISTICS_EX(ULONGLONG, UserFileWrites,          diUserFileWrites),
		_DEF_STATISTICS_EX(ULONGLONG, UserFileWriteBytes,      diUserFileWriteBytes),
		_DEF_STATISTICS_EX(ULONGLONG, UserDiskWrites,          diUserDiskWrites),
		_DEF_STATISTICS_EX(ULONGLONG, MetaDataReads,           diMetaDataReads),
		_DEF_STATISTICS_EX(ULONGLONG, MetaDataReadBytes,       diMetaDataReadBytes),
		_DEF_STATISTICS_EX(ULONGLONG, MetaDataDiskReads,       diMetaDataDiskReads),
		_DEF_STATISTICS_EX(ULONGLONG, MetaDataWrites,          diMetaDataWrites),
		_DEF_STATISTICS_EX(ULONGLONG, MetaDataWriteBytes,      diMetaDataWriteBytes),
		_DEF_STATISTICS_EX(ULONGLONG, MetaDataDiskWrites,      diMetaDataDiskWrites),
		};
		*d = def;
		return _countof(def);
	}

	INT GetNTFSItemData(STATISTICS_ITEM_DEF **d)
	{
		STATISTICS_ITEM_DEF def[] = {
		_DEF_STATISTICS_NTFS(DWORD, LogFileFullExceptions,              diLogFileFullExceptions),
		_DEF_STATISTICS_NTFS(DWORD, OtherExceptions,                    diOtherExceptions),
		_DEF_STATISTICS_NTFS(DWORD, MftReads,                           diMftReads),
		_DEF_STATISTICS_NTFS(DWORD, MftReadBytes,                       diMftReadBytes),
		_DEF_STATISTICS_NTFS(DWORD, MftWrites,                          diMftWrites),
		_DEF_STATISTICS_NTFS(DWORD, MftWriteBytes,                      diMftWriteBytes),
		_DEF_STATISTICS_NTFS(WORD,  MftWritesUserLevel.Write,           diMftWritesUserLevel_Write),
		_DEF_STATISTICS_NTFS(WORD,  MftWritesUserLevel.Create,          diMftWritesUserLevel_Create),
		_DEF_STATISTICS_NTFS(WORD,  MftWritesUserLevel.SetInfo,         diMftWritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS(WORD,  MftWritesUserLevel.Flush,           diMftWritesUserLevel_Flush),
		_DEF_STATISTICS_NTFS(WORD,  MftWritesFlushForLogFileFull,       diMftWritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS(WORD,  MftWritesLazyWriter,                diMftWritesLazyWriter),
		_DEF_STATISTICS_NTFS(WORD,  MftWritesUserRequest,               diMftWritesUserRequest),
		_DEF_STATISTICS_NTFS(DWORD, Mft2Writes,                         diMft2Writes),
		_DEF_STATISTICS_NTFS(DWORD, Mft2WriteBytes,                     diMft2WriteBytes),
		_DEF_STATISTICS_NTFS(WORD,  Mft2WritesUserLevel.Write,          diMft2WritesUserLevel_Write),
		_DEF_STATISTICS_NTFS(WORD,  Mft2WritesUserLevel.Create,         diMft2WritesUserLevel_Create),
		_DEF_STATISTICS_NTFS(WORD,  Mft2WritesUserLevel.SetInfo,        diMft2WritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS(WORD,  Mft2WritesUserLevel.Flush,          diMft2WritesUserLevel_Flush),
		_DEF_STATISTICS_NTFS(WORD,  Mft2WritesFlushForLogFileFull,      diMft2WritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS(WORD,  Mft2WritesLazyWriter,               diMft2WritesLazyWriter),
		_DEF_STATISTICS_NTFS(WORD,  Mft2WritesUserRequest,              diMft2WritesUserRequest),
		_DEF_STATISTICS_NTFS(DWORD, RootIndexReads,                     diRootIndexReads),
		_DEF_STATISTICS_NTFS(DWORD, RootIndexReadBytes,                 diRootIndexReadBytes),
		_DEF_STATISTICS_NTFS(DWORD, RootIndexWrites,                    diRootIndexWrites),
		_DEF_STATISTICS_NTFS(DWORD, RootIndexWriteBytes,                diRootIndexWriteBytes),
		_DEF_STATISTICS_NTFS(DWORD, BitmapReads,                        diBitmapReads),
		_DEF_STATISTICS_NTFS(DWORD, BitmapReadBytes,                    diBitmapReadBytes),
		_DEF_STATISTICS_NTFS(DWORD, BitmapWrites,                       diBitmapWrites),
		_DEF_STATISTICS_NTFS(DWORD, BitmapWriteBytes,                   diBitmapWriteBytes),
		_DEF_STATISTICS_NTFS(WORD,  BitmapWritesFlushForLogFileFull,    diBitmapWritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS(WORD,  BitmapWritesLazyWriter,             diBitmapWritesLazyWriter),
		_DEF_STATISTICS_NTFS(WORD,  BitmapWritesUserRequest,            diBitmapWritesUserRequest),
		_DEF_STATISTICS_NTFS(WORD,  BitmapWritesUserLevel.Write,        diBitmapWritesUserLevel_Write),
		_DEF_STATISTICS_NTFS(WORD,  BitmapWritesUserLevel.Create,       diBitmapWritesUserLevel_Create),
		_DEF_STATISTICS_NTFS(WORD,  BitmapWritesUserLevel.SetInfo,      diBitmapWritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS(DWORD, MftBitmapReads,                     diMftBitmapReads),
		_DEF_STATISTICS_NTFS(DWORD, MftBitmapReadBytes,                 diMftBitmapReadBytes),
		_DEF_STATISTICS_NTFS(DWORD, MftBitmapWrites,                    diMftBitmapWrites),
		_DEF_STATISTICS_NTFS(DWORD, MftBitmapWriteBytes,                diMftBitmapWriteBytes),
		_DEF_STATISTICS_NTFS(WORD,  MftBitmapWritesFlushForLogFileFull, diMftBitmapWritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS(WORD,  MftBitmapWritesLazyWriter,          diMftBitmapWritesLazyWriter),
		_DEF_STATISTICS_NTFS(WORD,  MftBitmapWritesUserRequest,         diMftBitmapWritesUserRequest),
		_DEF_STATISTICS_NTFS(WORD,  MftBitmapWritesUserLevel.Write,     diMftBitmapWritesUserLevel_Write),
		_DEF_STATISTICS_NTFS(WORD,  MftBitmapWritesUserLevel.Create,    diMftBitmapWritesUserLevel_Create),
		_DEF_STATISTICS_NTFS(WORD,  MftBitmapWritesUserLevel.SetInfo,   diMftBitmapWritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS(WORD,  MftBitmapWritesUserLevel.Flush,     diMftBitmapWritesUserLevel_Flush),
		_DEF_STATISTICS_NTFS(DWORD, UserIndexReads,                     diUserIndexReads),
		_DEF_STATISTICS_NTFS(DWORD, UserIndexReadBytes,                 diUserIndexReadBytes),
		_DEF_STATISTICS_NTFS(DWORD, UserIndexWrites,                    diUserIndexWrites),
		_DEF_STATISTICS_NTFS(DWORD, UserIndexWriteBytes,                diUserIndexWriteBytes),
		_DEF_STATISTICS_NTFS(DWORD, LogFileReads,                       diLogFileReads),
		_DEF_STATISTICS_NTFS(DWORD, LogFileReadBytes,                   diLogFileReadBytes),
		_DEF_STATISTICS_NTFS(DWORD, LogFileWrites,                      diLogFileWrites),
		_DEF_STATISTICS_NTFS(DWORD, LogFileWriteBytes,                  diLogFileWriteBytes),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.Calls,                     diAllocate_Calls),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.Clusters,                  diAllocate_Clusters),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.Hints,                     diAllocate_Hints),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.RunsReturned,              diAllocate_RunsReturned),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.HintsHonored,              diAllocate_HintsHonored),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.HintsClusters,             diAllocate_HintsClusters),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.Cache,                     diAllocate_Cache),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.CacheClusters,             diAllocate_CacheClusters),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.CacheMiss,                 diAllocate_CacheMiss),
		_DEF_STATISTICS_NTFS(DWORD, Allocate.CacheMissClusters,         diAllocate_CacheMissClusters),
		_DEF_STATISTICS_NTFS_WIN8(DWORD, DiskResourcesExhausted,        diDiskResourcesExhausted),
		};

		*d = (STATISTICS_ITEM_DEF *)_MemAlloc( sizeof(def) );
		CopyMemory(*d,def,sizeof(def));
		return _countof(def);
	};

	INT GetNTFSItemDataEx(STATISTICS_ITEM_DEF **d)
	{
		STATISTICS_ITEM_DEF def[] = {
		_DEF_STATISTICS_NTFS_EX(DWORD,     LogFileFullExceptions,              diLogFileFullExceptions),
		_DEF_STATISTICS_NTFS_EX(DWORD,     OtherExceptions,                    diOtherExceptions),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftReads,                           diMftReads),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftReadBytes,                       diMftReadBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftWrites,                          diMftWrites),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftWriteBytes,                      diMftWriteBytes),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftWritesUserLevel.Write,           diMftWritesUserLevel_Write),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftWritesUserLevel.Create,          diMftWritesUserLevel_Create),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftWritesUserLevel.SetInfo,         diMftWritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftWritesUserLevel.Flush,           diMftWritesUserLevel_Flush),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftWritesFlushForLogFileFull,       diMftWritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftWritesLazyWriter,                diMftWritesLazyWriter),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftWritesUserRequest,               diMftWritesUserRequest),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, Mft2Writes,                         diMft2Writes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, Mft2WriteBytes,                     diMft2WriteBytes),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Mft2WritesUserLevel.Write,          diMft2WritesUserLevel_Write),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Mft2WritesUserLevel.Create,         diMft2WritesUserLevel_Create),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Mft2WritesUserLevel.SetInfo,        diMft2WritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Mft2WritesUserLevel.Flush,          diMft2WritesUserLevel_Flush),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Mft2WritesFlushForLogFileFull,      diMft2WritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Mft2WritesLazyWriter,               diMft2WritesLazyWriter),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Mft2WritesUserRequest,              diMft2WritesUserRequest),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, RootIndexReads,                     diRootIndexReads),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, RootIndexReadBytes,                 diRootIndexReadBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, RootIndexWrites,                    diRootIndexWrites),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, RootIndexWriteBytes,                diRootIndexWriteBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, BitmapReads,                        diBitmapReads),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, BitmapReadBytes,                    diBitmapReadBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, BitmapWrites,                       diBitmapWrites),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, BitmapWriteBytes,                   diBitmapWriteBytes),
		_DEF_STATISTICS_NTFS_EX(DWORD,     BitmapWritesFlushForLogFileFull,    diBitmapWritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS_EX(DWORD,     BitmapWritesLazyWriter,             diBitmapWritesLazyWriter),
		_DEF_STATISTICS_NTFS_EX(DWORD,     BitmapWritesUserRequest,            diBitmapWritesUserRequest),
		_DEF_STATISTICS_NTFS_EX(DWORD,     BitmapWritesUserLevel.Write,        diBitmapWritesUserLevel_Write),
		_DEF_STATISTICS_NTFS_EX(DWORD,     BitmapWritesUserLevel.Create,       diBitmapWritesUserLevel_Create),
		_DEF_STATISTICS_NTFS_EX(DWORD,     BitmapWritesUserLevel.SetInfo,      diBitmapWritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS_EX(DWORD,     BitmapWritesUserLevel.Flush,        diBitmapWritesUserLevel_Flush),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftBitmapReads,                     diMftBitmapReads),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftBitmapReadBytes,                 diMftBitmapReadBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftBitmapWrites,                    diMftBitmapWrites),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, MftBitmapWriteBytes,                diMftBitmapWriteBytes),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftBitmapWritesFlushForLogFileFull, diMftBitmapWritesFlushForLogFileFull),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftBitmapWritesLazyWriter,          diMftBitmapWritesLazyWriter),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftBitmapWritesUserRequest,         diMftBitmapWritesUserRequest),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftBitmapWritesUserLevel.Write,     diMftBitmapWritesUserLevel_Write),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftBitmapWritesUserLevel.Create,    diMftBitmapWritesUserLevel_Create),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftBitmapWritesUserLevel.SetInfo,   diMftBitmapWritesUserLevel_SetInfo),
		_DEF_STATISTICS_NTFS_EX(DWORD,     MftBitmapWritesUserLevel.Flush,     diMftBitmapWritesUserLevel_Flush),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, UserIndexReads,                     diUserIndexReads),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, UserIndexReadBytes,                 diUserIndexReadBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, UserIndexWrites,                    diUserIndexWrites),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, UserIndexWriteBytes,                diUserIndexWriteBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, LogFileReads,                       diLogFileReads),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, LogFileReadBytes,                   diLogFileReadBytes),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, LogFileWrites,                      diLogFileWrites),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, LogFileWriteBytes,                  diLogFileWriteBytes),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Allocate.Calls,                     diAllocate_Calls),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Allocate.RunsReturned,              diAllocate_RunsReturned),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Allocate.Hints,                     diAllocate_Hints),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Allocate.HintsHonored,              diAllocate_HintsHonored),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Allocate.Cache,                     diAllocate_Cache),
		_DEF_STATISTICS_NTFS_EX(DWORD,     Allocate.CacheMiss,                 diAllocate_CacheMiss),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, Allocate.Clusters,                  diAllocate_Clusters),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, Allocate.HintsClusters,             diAllocate_HintsClusters),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, Allocate.CacheClusters,             diAllocate_CacheClusters),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, Allocate.CacheMissClusters,         diAllocate_CacheMissClusters),
		//
		//  Additions for Windows 8.1
		//
		_DEF_STATISTICS_NTFS_EX(DWORD,     DiskResourcesExhausted,             diDiskResourcesExhausted),
		//
		//  Additions for Windows 10
		//
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, VolumeTrimCount,                    diVolumeTrimCount),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, VolumeTrimTime,                     diVolumeTrimTime),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, VolumeTrimByteCount,                diVolumeTrimByteCount),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, FileLevelTrimCount,                 diFileLevelTrimCount),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, FileLevelTrimTime,                  diFileLevelTrimTime),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, FileLevelTrimByteCount,             diFileLevelTrimByteCount),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, VolumeTrimSkippedCount,             diVolumeTrimSkippedCount),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, VolumeTrimSkippedByteCount,         diVolumeTrimSkippedByteCount),
		//
		//  Additions for NtfsFillStatInfoFromMftRecord
		//
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, NtfsFillStatInfoFromMftRecordCalledCount, diNtfsFillStatInfoFromMftRecordCalledCount),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, NtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount, diNtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount),
		_DEF_STATISTICS_NTFS_EX(DWORDLONG, NtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount, diNtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount),
		};

		*d = (STATISTICS_ITEM_DEF *)_MemAlloc( sizeof(def) );
		CopyMemory(*d,def,sizeof(def));
		return _countof(def);
	}

	INT GetFATItemData(STATISTICS_ITEM_DEF **d)
	{
		STATISTICS_ITEM_DEF def[] = {
		_DEF_STATISTICS_FAT(DWORD, CreateHits,           diCreateHits),
		_DEF_STATISTICS_FAT(DWORD, SuccessfulCreates,    diSuccessfulCreates),
		_DEF_STATISTICS_FAT(DWORD, FailedCreates,        diFailedCreates),
		_DEF_STATISTICS_FAT(DWORD, NonCachedReads,       diNonCachedReads),
		_DEF_STATISTICS_FAT(DWORD, NonCachedReadBytes,   diNonCachedReadBytes),
		_DEF_STATISTICS_FAT(DWORD, NonCachedWrites,      diNonCachedWrites),
		_DEF_STATISTICS_FAT(DWORD, NonCachedWriteBytes,  diNonCachedWriteBytes),
		_DEF_STATISTICS_FAT(DWORD, NonCachedDiskReads,   diNonCachedDiskReads),
		_DEF_STATISTICS_FAT(DWORD, NonCachedDiskWrites,  diNonCachedDiskWrites),
		};

		*d = (STATISTICS_ITEM_DEF *)_MemAlloc( sizeof(STATISTICS_ITEM_DEF) * _countof(def) );
		CopyMemory(*d,def,sizeof(def));
		return _countof(def);
	};

	VOID DataBind()
	{
		INT i,c;
		STATISTICS_ITEM_DEF *p;

		INT cTableItems;
		cTableItems = (diStatisticsMaxId - diStatisticsBase);

		//
		// For Each CPU Column
		//
		c = GetItemData(&p);

		m_tblStatistics = (STATISTICS_REF_ITEM *)_MemAlloc( sizeof(STATISTICS_REF_ITEM) *  cTableItems );

		for(i = 0; i < c; i++)
		{
			ASSERT( (int)(p[i].data_id - diStatisticsBase) < cTableItems );
			m_tblStatistics[ p[i].data_id - diStatisticsBase ].def = p[i];
		}

		//
		// For Total Data Column
		//
		c = GetItemDataEx(&p);

		m_tblStatisticsEx = (STATISTICS_REF_ITEM *)_MemAlloc( sizeof(STATISTICS_REF_ITEM) * cTableItems );

		for(i = 0; i < c; i++)
		{
			ASSERT( (int)(p[i].data_id - diStatisticsBase) < cTableItems );
			m_tblStatisticsEx[ p[i].data_id - diStatisticsBase ].def = p[i];
		}
	}

	VOID DataBindNTFS()
	{
		INT i,c;
		STATISTICS_ITEM_DEF *p;

		//
		// Each CPU
		//
		c = GetNTFSItemData(&p);

		m_tblMetaData = (STATISTICS_REF_ITEM *)_MemAllocZero( sizeof(STATISTICS_REF_ITEM) * (diStatisticsNtfsMaxId - diStatisticsNtfsBase) );

		int cc = (diStatisticsNtfsMaxId - diStatisticsNtfsBase);

		for(i = 0; i < c; i++)
		{
			p[i].offset += sizeof(FILESYSTEM_STATISTICS);

			int index = p[i].data_id - diStatisticsNtfsBase;

			ASSERT( index < cc );

			m_tblMetaData[ index ].def = p[i];
		}

		_SafeMemFree(p);

		//
		// Total Data
		//
		c = GetNTFSItemDataEx(&p);

		m_tblMetaDataEx = (STATISTICS_REF_ITEM *)_MemAllocZero( sizeof(STATISTICS_REF_ITEM) * (diStatisticsNtfsMaxId - diStatisticsNtfsBase) );

		for(i = 0; i < c; i++)
		{
			p[i].offset += sizeof(FILESYSTEM_STATISTICS_EX);

			m_tblMetaDataEx[ p[i].data_id - diStatisticsNtfsBase ].def = p[i];
		}

		_SafeMemFree(p);
	}

	VOID DataBindFAT()
	{
		INT i,c;
		STATISTICS_ITEM_DEF *p;

		//
		// Each CPU
		//
		c = GetFATItemData(&p);

		m_tblMetaData = (STATISTICS_REF_ITEM *)_MemAllocZero( sizeof(STATISTICS_REF_ITEM) * (diStatisticsFatMaxId + 1) );

		for(i = 0; i < c; i++)
		{
			p[i].offset += sizeof(FILESYSTEM_STATISTICS);

			m_tblMetaData[ p[i].data_id - diStatisticsFatBase ].def = p[i];
		}

		_SafeMemFree(p);

		//
		// Total Data
		//
		c = GetFATItemData(&p);

		m_tblMetaDataEx = (STATISTICS_REF_ITEM *)_MemAllocZero( sizeof(STATISTICS_REF_ITEM) * (diStatisticsFatMaxId + 1) );

		for(i = 0; i < c; i++)
		{
			p[i].offset += sizeof(FILESYSTEM_STATISTICS_EX);

			m_tblMetaDataEx[ p[i].data_id - diStatisticsFatBase ].def = p[i];
		}

		_SafeMemFree(p);
	}

	typedef struct _TITLE_NAME
	{
		PWSTR Name;
		UINT DataId;
		UINT GroupId;
	} TITLE_NAME;

	VOID FillTitleCommon()
	{
		TITLE_NAME tn[] =
		{
#if _ENABLE_DEBUG_INFORMATION
			{ L"File System Type",      diFileSystemType,         ID_GROUP_GENERIC },
			{ L"Version",               diStatisticsDataVersion,  ID_GROUP_GENERIC },
			{ L"Structure Size",        diSizeOfCompleteStructure,ID_GROUP_GENERIC },
#endif
			{ L"User File Reads",       diUserFileReads,          ID_GROUP_GENERIC },
			{ L"User File Read Bytes",  diUserFileReadBytes,      ID_GROUP_GENERIC },
			{ L"User Disk Reads",       diUserDiskReads,          ID_GROUP_GENERIC },
			{ L"User File Writes",      diUserFileWrites,         ID_GROUP_GENERIC },
			{ L"User File Write Bytes", diUserFileWriteBytes,     ID_GROUP_GENERIC },
			{ L"User Disk Writes",      diUserDiskWrites,         ID_GROUP_GENERIC },
			{ L"Metadata Reads",        diMetaDataReads,          ID_GROUP_GENERIC },
			{ L"Metadata Read Bytes",   diMetaDataReadBytes,      ID_GROUP_GENERIC },
			{ L"Metadata Disk Reads",   diMetaDataDiskReads,      ID_GROUP_GENERIC },
			{ L"Metadata Writes",       diMetaDataWrites,         ID_GROUP_GENERIC },
			{ L"Metadata Write Bytes",  diMetaDataWriteBytes,     ID_GROUP_GENERIC },
			{ L"Metadata Disk Writes",  diMetaDataDiskWrites,     ID_GROUP_GENERIC },
		};

		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM|LVIF_GROUPID;
		lvi.iImage   = I_IMAGENONE;
		lvi.iIndent  = 1 * _INDENT_SIZE;
		lvi.lParam   = 0;

		for(int i = 0; i < ARRAYSIZE(tn); i++)
		{
			lvi.iItem    = i;
			lvi.iGroupId = 0;
			lvi.pszText  = tn[i].Name;
			lvi.iGroupId = tn[i].GroupId;

			if( tn[i].DataId )
			{
				STATISTICS_LIST_ITEM* pLine = new STATISTICS_LIST_ITEM;
				pLine->tbl = &m_tblStatistics[ tn[i].DataId - diStatisticsBase ];
				pLine->tblex = &m_tblStatisticsEx[ tn[i].DataId - diStatisticsBase ];
				lvi.lParam = (LPARAM)pLine;
			}
			else
			{
				lvi.lParam = 0;
			}

			ListView_InsertItem(m_hWndList,&lvi);

			ListView_SetItemText(m_hWndList,i,1,LPSTR_TEXTCALLBACK);
		}
	}

	int GetTitleText(int iItemType,PWSTR pszText,int cchText)
	{
		PWSTR ref = 0;
		int cch;
		cch = LoadString(_GetResourceInstance(),iItemType,(LPWSTR)&ref,0);
		if( ref && *ref != L' ' )
		{
			memcpy(pszText,ref,cch*sizeof(WCHAR));
			pszText[cch] = 0;
		}
		else
		{
			cch = 0;
		}
		return cch;
	}

typedef struct _TITLE {
	USHORT Type;
	USHORT Indent;
	UINT  DataId;
	PWSTR pszTitle;
} TITLE;

#define TITLE_DEF(name)            { 0, 0, name, L#name, }
#define TITLE_DEF_HDR(name)        { 1, 0, name, L#name, }
#define TITLE_DEF_HDR_STR(name)    { 1, 0, 0, L#name, }
#define TITLE_DEF_END()            { 2, 0, 0, NULL, }

	TITLE *GetTitleSetNTFS(int *pnCount)
	{
		static TITLE ntfs[] =
		{
			TITLE_DEF(diLogFileFullExceptions),
			TITLE_DEF(diOtherExceptions),
	
			TITLE_DEF_HDR_STR( MFT ),
				TITLE_DEF(diMftReads),
				TITLE_DEF(diMftReadBytes),
				TITLE_DEF(diMftWrites),
				TITLE_DEF(diMftWriteBytes),
				TITLE_DEF_HDR(diMftWritesUserLevel),
					TITLE_DEF(diMftWritesUserLevel_Write),
					TITLE_DEF(diMftWritesUserLevel_Create),
					TITLE_DEF(diMftWritesUserLevel_SetInfo),
					TITLE_DEF(diMftWritesUserLevel_Flush),
				TITLE_DEF_END(),
				TITLE_DEF(diMftWritesFlushForLogFileFull),
				TITLE_DEF(diMftWritesLazyWriter),
				TITLE_DEF(diMftWritesUserRequest),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( MFT2 ),
				TITLE_DEF(diMft2Writes),
				TITLE_DEF(diMft2WriteBytes),
				TITLE_DEF_HDR(diMft2WritesUserLevel),
					TITLE_DEF(diMft2WritesUserLevel_Write),
					TITLE_DEF(diMft2WritesUserLevel_Create),
					TITLE_DEF(diMft2WritesUserLevel_SetInfo),
					TITLE_DEF(diMft2WritesUserLevel_Flush),
				TITLE_DEF_END(),
				TITLE_DEF(diMft2WritesFlushForLogFileFull),
				TITLE_DEF(diMft2WritesLazyWriter),
				TITLE_DEF(diMft2WritesUserRequest),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( RootIndex ),
				TITLE_DEF(diRootIndexReads),
				TITLE_DEF(diRootIndexReadBytes),
				TITLE_DEF(diRootIndexWrites),
				TITLE_DEF(diRootIndexWriteBytes),
			TITLE_DEF_END(),
			
			TITLE_DEF_HDR_STR( Bitmap ),
				TITLE_DEF(diBitmapReads),
				TITLE_DEF(diBitmapReadBytes),
				TITLE_DEF(diBitmapWrites),
				TITLE_DEF(diBitmapWriteBytes),
				TITLE_DEF(diBitmapWritesFlushForLogFileFull),
				TITLE_DEF(diBitmapWritesLazyWriter),
				TITLE_DEF(diBitmapWritesUserRequest),
				TITLE_DEF(diBitmapWritesUserLevel_Write),
				TITLE_DEF(diBitmapWritesUserLevel_Create),
				TITLE_DEF(diBitmapWritesUserLevel_SetInfo),
				TITLE_DEF(diBitmapWritesUserLevel_Flush),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( MFT Bitmap ),
				TITLE_DEF(diMftBitmapReads),
				TITLE_DEF(diMftBitmapReadBytes),
				TITLE_DEF(diMftBitmapWrites),
				TITLE_DEF(diMftBitmapWriteBytes),
				TITLE_DEF(diMftBitmapWritesFlushForLogFileFull),
				TITLE_DEF(diMftBitmapWritesLazyWriter),
				TITLE_DEF(diMftBitmapWritesUserRequest),
				TITLE_DEF_HDR_STR( MFT Bitmap Writes User Level ),
					TITLE_DEF(diMftBitmapWritesUserLevel_Write),
					TITLE_DEF(diMftBitmapWritesUserLevel_Create),
					TITLE_DEF(diMftBitmapWritesUserLevel_SetInfo),
					TITLE_DEF(diMftBitmapWritesUserLevel_Flush),
				TITLE_DEF_END(),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( User Index ),
				TITLE_DEF(diUserIndexReads),
				TITLE_DEF(diUserIndexReadBytes),
				TITLE_DEF(diUserIndexWrites),
				TITLE_DEF(diUserIndexWriteBytes),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( Log File ),
				TITLE_DEF(diLogFileReads),
				TITLE_DEF(diLogFileReadBytes),
				TITLE_DEF(diLogFileWrites),
				TITLE_DEF(diLogFileWriteBytes),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( Allocate ),
				TITLE_DEF(diAllocate_Calls),
				TITLE_DEF(diAllocate_RunsReturned),
				TITLE_DEF(diAllocate_Hints),
				TITLE_DEF(diAllocate_HintsHonored),
				TITLE_DEF(diAllocate_Cache),
				TITLE_DEF(diAllocate_CacheMiss),
				TITLE_DEF(diAllocate_Clusters),
				TITLE_DEF(diAllocate_HintsClusters),
				TITLE_DEF(diAllocate_CacheClusters),
				TITLE_DEF(diAllocate_CacheMissClusters),
			TITLE_DEF_END(),
		
			TITLE_DEF(diDiskResourcesExhausted),
		
			TITLE_DEF_HDR_STR( Volume Level Trim ),
				TITLE_DEF(diVolumeTrimCount),
				TITLE_DEF(diVolumeTrimTime),
				TITLE_DEF(diVolumeTrimByteCount),
				TITLE_DEF(diVolumeTrimSkippedCount),
				TITLE_DEF(diVolumeTrimSkippedByteCount),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( File Level Trim ),
				TITLE_DEF(diFileLevelTrimCount),
				TITLE_DEF(diFileLevelTrimTime),
				TITLE_DEF(diFileLevelTrimByteCount),
			TITLE_DEF_END(),
		
			TITLE_DEF_HDR_STR( Fill StatInfo From MFT Record ),
				TITLE_DEF(diNtfsFillStatInfoFromMftRecordCalledCount),
				TITLE_DEF(diNtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount),
				TITLE_DEF(diNtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount),
			TITLE_DEF_END(),
		};
		*pnCount = _countof(ntfs);
		return ntfs;
	}

	TITLE *GetTitleSetFAT( int *pnCount )
	{
		static TITLE fat[] =
		{
			TITLE_DEF(diCreateHits),
			TITLE_DEF(diSuccessfulCreates),
			TITLE_DEF(diFailedCreates),
			TITLE_DEF(diNonCachedReads),
			TITLE_DEF(diNonCachedReadBytes),
			TITLE_DEF(diNonCachedWrites),
			TITLE_DEF(diNonCachedWriteBytes),
			TITLE_DEF(diNonCachedDiskReads),
			TITLE_DEF(diNonCachedDiskWrites),
		};
		*pnCount = _countof(fat);
		return fat;
	}

	VOID FillTitle(TITLE *ntfs,int cTitles,int BaseDataId,int iGroupId)
	{
		WCHAR sz[260];
		STATISTICS_LIST_ITEM* pLine;

		int iItem = 0;
		int IndentWidth = 4 * _INDENT_SIZE;

		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM|LVIF_GROUPID;
		lvi.iImage   = I_IMAGENONE;
		lvi.iIndent  = 1 * _INDENT_SIZE;
		lvi.lParam   = 0;
		lvi.iGroupId = iGroupId;

		for(int i = 0; i < cTitles; i++)
		{
			if( ntfs[i].Type == 2 )
			{
				lvi.iIndent -= IndentWidth;
				continue;
			}

			lvi.iItem = iItem;

			if( GetTitleText(ntfs[i].DataId,sz,_countof(sz)) )
			{
				lvi.pszText = sz;
			}
			else
			{
				lvi.pszText = ntfs[i].pszTitle; // for DEBUG only
			}

			pLine = new STATISTICS_LIST_ITEM;

			if( pLine )
			{
				ZeroMemory(pLine,sizeof(STATISTICS_LIST_ITEM));

				if( ntfs[i].Type == 0 )
				{
					if( ntfs[i].DataId )
					{
						int index = ntfs[i].DataId - BaseDataId;

						if( index < BaseDataId )
						{
							pLine->tbl   = &m_tblMetaData[ index ];
							pLine->tblex = &m_tblMetaDataEx[ index ];
						}
					}
				}

				lvi.lParam   = (LPARAM)pLine;

				iItem = ListView_InsertItem(m_hWndList,&lvi);
			}

			if( ntfs[i].Type == 1 )
			{
				lvi.iIndent += IndentWidth;
			}

			iItem++;
		}
	}

	HRESULT FillItems(SELECT_ITEM *pFile)
	{
		HRESULT hr;
		PBYTE Buffer=NULL;
		SIZE_T BufferSize=0;
		PFILESYSTEM_STATISTICS *PtrArray=NULL;
		ULONG PtrArrayCount=0;
		FILESYSTEM_STATISTICS_EX *StatisticsEx=NULL;
		SIZE_T cbStatisticsExSize= 0;
		
		if( m_pszVolumeRoot == NULL )
		{
			ASSERT(pFile != NULL);
			m_pszVolumeRoot = _MemAllocString(pFile->pszName);
		}

		SetRedraw(m_hWndList,FALSE);

		hr = GetStatisticsData( m_pszVolumeRoot, 
					&Buffer, &BufferSize,
					&PtrArray, NULL , &PtrArrayCount,
					&StatisticsEx, &cbStatisticsExSize );

		if( hr == S_OK )
		{
#if _ENABLE_DIFF_COLUMN
			if( m_StatisticsExDiff == NULL )
			{
				// This buffer is allocation once.
				m_StatisticsExDiff = (FILESYSTEM_STATISTICS_EX *)_MemAllocZero(cbStatisticsExSize);
			}

			if( m_StatisticsExDiff && m_StatisticsEx && m_Statistics )
			{
				if( GetStatisticsPtr(Buffer)->FileSystemType == FILESYSTEM_STATISTICS_TYPE_NTFS ||
					GetStatisticsPtr(Buffer)->FileSystemType == FILESYSTEM_STATISTICS_TYPE_REFS )
				{			
					CalcStatisticsDiffEx( 
						m_StatisticsExDiff, 
						m_StatisticsEx,
						StatisticsEx
						);

					CalcStatisticsDiffNtfsEx( 
						GetNTFSStatisticsEx(m_StatisticsExDiff), 
						GetNTFSStatisticsEx(m_StatisticsEx),
						GetNTFSStatisticsEx(StatisticsEx)
						);
				}
			}
#endif
			FreeStatisticsData(m_Statistics,(PFILESYSTEM_STATISTICS *)m_StatisticsPtrArray,m_StatisticsEx);

			m_Statistics = (PBYTE)Buffer;
			m_StatisticsPtrArray = (PBYTE*)PtrArray;
			m_StatisticsEx = StatisticsEx;

			if( ListView_GetItemCount(m_hWndList) == 0 )
			{
				//
				// Initialize ListView All items at First call only.
				//
				DataBind();
				FillTitleCommon();
	
				TITLE *pTitle;
				int cTitle;
	
				PFILESYSTEM_STATISTICS psHdr = GetStatisticsPtr(Buffer);
	
				if( psHdr->FileSystemType == FILESYSTEM_STATISTICS_TYPE_NTFS ||
					psHdr->FileSystemType == FILESYSTEM_STATISTICS_TYPE_REFS )
				{ 
					DataBindNTFS();
	
					pTitle = GetTitleSetNTFS(&cTitle);
					FillTitle(pTitle,cTitle,diStatisticsNtfsBase,
						psHdr->FileSystemType == FILESYSTEM_STATISTICS_TYPE_NTFS ? ID_GROUP_NTFS : ID_GROUP_REFS);
				}
				else if( psHdr->FileSystemType == FILESYSTEM_STATISTICS_TYPE_EXFAT ||
						psHdr->FileSystemType == FILESYSTEM_STATISTICS_TYPE_FAT )
				{
					DataBindFAT();
	
					pTitle = GetTitleSetFAT(&cTitle);
					FillTitle(pTitle,cTitle,diStatisticsFatBase,
						psHdr->FileSystemType == FILESYSTEM_STATISTICS_TYPE_EXFAT ? ID_GROUP_EXFAT : ID_GROUP_FAT);
				}
			}
		}
		else
		{
			PWSTR pMessage;
			if( WinGetErrorMessage(hr,&pMessage) > 0 )
			{
				_SafeMemFree(m_pszErrorMessage);
				m_pszErrorMessage = _MemAllocString(pMessage);
				WinFreeErrorMessage(pMessage);
			}
		}

		RedrawWindow(m_hWndList,NULL,NULL,RDW_UPDATENOW|RDW_INVALIDATE);
		SetRedraw(m_hWndList,TRUE);

		return hr;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
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
		}
		return S_FALSE;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnEditCopy();
				break;
			case ID_VIEW_REFRESH:
				OnViewRefresh();
				break;
		}
		return S_OK;
	}

	void OnEditCopy()
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

	void OnViewRefresh()
	{
		UpdateData(NULL);
	}
};
