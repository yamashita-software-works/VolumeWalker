#pragma once
//***************************************************************************
//*                                                                         *
//*  page_changejournal.h                                                   *
//*                                                                         *
//*  Volume Contents - Change Journal Viewr Page                            *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-04-15                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "fsvolumecontents.h"
#include "pagewbdbase.h"
#include "common.h"
#include "column.h"
#include "fsfilelib.h"
#include "changejournalhelp.h"
#include "ntwin32helper.h"

extern int GetImageListIndex(PCWSTR pszPath,PCWSTR pszFileName,DWORD dwFileAttributes);

struct CChangeJournalItem
{
	PUSN_RECORD UsnRecord;
	ULONG State;

	CChangeJournalItem()
	{
		UsnRecord = NULL;
		State = 0;
	}

	~CChangeJournalItem()
	{
	}
};

class CChangeJournalListPage : public CPageWndBase
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CChangeJournalListPage> *m_disp_proc;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	CColumnList m_columns;

	HFONT m_hFont;
	HFONT m_hFontHeader;

	CChangeJournalItem *m_pItemList;

	DWORD m_ErrorStatus;

	IFSJournalBuffer *m_pJournalBuffer;

	PWSTR m_pszVolumeName;
	PWSTR m_pszRawFileName;

public:
	CChangeJournalListPage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction = -1;
		m_disp_proc = NULL;
		m_ErrorStatus = 0;
		m_pItemList = NULL;
		m_pJournalBuffer = NULL;
		m_pszVolumeName = NULL;
		m_pszRawFileName = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
	}

	~CChangeJournalListPage()
	{
		if( m_disp_proc )
			delete[] m_disp_proc;

		_SafeMemFree(m_pszVolumeName);
		_SafeMemFree(m_pszRawFileName);
	}

	virtual HRESULT OnInitPage(PVOID)
	{
		m_columns.SetIniFilePath( GetIniFilePath() );

		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA | LVS_SHAREIMAGELISTS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

		HIMAGELIST himl = GetGlobalShareImageList();
		ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		InitColumnDefinitions();

		if( !LoadColumns(m_hWndList) )
		{
			InsertDefaultColumns(m_hWndList);
		}

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
		if( m_pJournalBuffer )
		{
			DestroyJournalBuffer( m_pJournalBuffer );
			m_pJournalBuffer = NULL;
		}

		if( m_pItemList )
		{
			delete[] m_pItemList;
			m_pItemList = NULL;
		}

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
				CChangeJournalItem *pItem = &m_pItemList[ pnmlvcd->nmcd.dwItemSpec ];
				DWORD dwFileAttributes = _UsnGetItem_FileAttributes(pItem->UsnRecord);

				if( dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED )
					pnmlvcd->clrText = RGB(0,0,180);
				else if( dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED )
					pnmlvcd->clrText = RGB(0,180,60);
				else if( dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL )
					pnmlvcd->clrText = RGB(0,200,48);
				else if( dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE )
					pnmlvcd->clrText = RGB(185,122,87);
				else if( dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
					pnmlvcd->clrText = RGB(120,0,100);

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

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		return 0;
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		DoSort(pnmlv->iSubItem);
		return 0;
	}

	void DoSort(int iSubItem=-1,int iDirection=-1)
	{
		// todo: iSubItem -> read order, no sort. insted current iSubItem
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

		SortItems(id);

		ListViewEx_SetHeaderArrow(hwndLV,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	void SetErrorState(ULONG Status)
	{
		m_ErrorStatus = Status;
	}

	LRESULT OnGetEmptyMarkup(NMHDR *pnmhdr)
	{
		NMLVEMPTYMARKUP *pnmlvem = (NMLVEMPTYMARKUP *)pnmhdr;

		if( m_ErrorStatus != ERROR_SUCCESS )
		{
			pnmlvem->dwFlags = EMF_CENTERED;
			PWSTR pMessage;
			if( WinGetErrorMessage(m_ErrorStatus,&pMessage) > 0 )
			{
				StringCchCopy(pnmlvem->szMarkup,ARRAYSIZE(pnmlvem->szMarkup),pMessage);
				WinFreeErrorMessage(pMessage);
			}
			else
			{
				StringCchPrintf(pnmlvem->szMarkup,ARRAYSIZE(pnmlvem->szMarkup),L"Status=0x%08X",m_ErrorStatus);
			}
			return TRUE;
		}
		return FALSE;
	}

	LRESULT OnDisp_Name(UINT,NMLVDISPINFO *pnmlvdi)
	{
		CChangeJournalItem *pItem = &m_pItemList[ pnmlvdi->item.iItem ];
		_UsnGetItem_GetFileName(pItem->UsnRecord,pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
		return 0;
	}

	LRESULT OnDisp_DateTime(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CChangeJournalItem *pItem = &m_pItemList[ pnmlvdi->item.iItem ];
		int fmt = 0;
		switch( fmt )
		{
			case 0:
				_GetDateTimeStringEx2(_UsnGetItem_TimeStamp64(pItem->UsnRecord),pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,NULL,NULL,0,1);
				break;
			case 1:	
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%016I64X",_UsnGetItem_TimeStamp64(pItem->UsnRecord));
				break;
		}
		return 0;
	}

	LRESULT OnDisp_Attributes(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CChangeJournalItem *pItem = &m_pItemList[ pnmlvdi->item.iItem ];
		GetAttributeString(_UsnGetItem_FileAttributes(pItem->UsnRecord),pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
		return 0;
	}

	LRESULT OnDisp_Usn(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CChangeJournalItem *pItem = &m_pItemList[ pnmlvdi->item.iItem ];
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",_UsnGetItem_Usn(pItem->UsnRecord));
		return 0;
	}

	LRESULT OnDisp_Reason(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CChangeJournalItem *pItem = &m_pItemList[ pnmlvdi->item.iItem ];

		GetJournalReasonAbbreviationsText(_UsnGetItem_Reason(pItem->UsnRecord),pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,0);

		return 0;
	}

	LRESULT OnDisp_Frn(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CChangeJournalItem *pItem = &m_pItemList[ pnmlvdi->item.iItem ];
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%016I64X",_UsnGetItem_FileReferenceNumber(pItem->UsnRecord));
		return 0;
	}

	LRESULT OnDisp_ParentFrn(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CChangeJournalItem *pItem = &m_pItemList[ pnmlvdi->item.iItem ];
		StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%016I64X",_UsnGetItem_ParentFileReferenceNumber(pItem->UsnRecord));
		return 0;
	}

	LRESULT OnDisp_Path(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CChangeJournalListPage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_None,           NULL),
			COL_HANDLER_MAP_DEF(COLUMN_Name,           &CChangeJournalListPage::OnDisp_Name),
			COL_HANDLER_MAP_DEF(COLUMN_Date,           &CChangeJournalListPage::OnDisp_DateTime),
			COL_HANDLER_MAP_DEF(COLUMN_FileAttributes, &CChangeJournalListPage::OnDisp_Attributes),
			COL_HANDLER_MAP_DEF(COLUMN_Reason,         &CChangeJournalListPage::OnDisp_Reason),
			COL_HANDLER_MAP_DEF(COLUMN_Usn,            &CChangeJournalListPage::OnDisp_Usn),
			COL_HANDLER_MAP_DEF(COLUMN_Frn,            &CChangeJournalListPage::OnDisp_Frn),
			COL_HANDLER_MAP_DEF(COLUMN_ParentFrn,      &CChangeJournalListPage::OnDisp_ParentFrn),
			COL_HANDLER_MAP_DEF(COLUMN_Path,           &CChangeJournalListPage::OnDisp_Path),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CChangeJournalListPage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CChangeJournalListPage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;

		int id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( m_disp_proc == NULL )
		{
			InitColumnTable();	
		}

		if( pdi->item.mask & LVIF_IMAGE )
		{
			WCHAR szName[MAX_PATH];
			CChangeJournalItem *pItem = &m_pItemList[ pdi->item.iItem ];
			_UsnGetItem_GetFileName(pItem->UsnRecord,szName,MAX_PATH);
			pdi->item.iImage = GetImageListIndex(NULL,szName,_UsnGetItem_FileAttributes(pItem->UsnRecord));

			if( pdi->item.iImage & 0xff000000 )
			{
				pdi->item.state = INDEXTOOVERLAYMASK(pdi->item.iImage >> 24);
				pdi->item.stateMask = LVIS_OVERLAYMASK;
				pdi->item.mask |= LVIF_STATE;
			}

			pdi->item.iImage = pdi->item.iImage & ~0xFF000000;

			pdi->item.mask |= LVIF_DI_SETITEM;
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
				return OnFindItem(hWnd,uMsg,wParam,lParam);
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

	void InitColumnDefinitions()
	{
		static COLUMN def_columns[] = {
			{ COLUMN_Name,           L"Name",                  1, 240, LVCFMT_LEFT },
			{ COLUMN_FileAttributes, L"Attributes",            2,  80, LVCFMT_LEFT },
			{ COLUMN_Usn,            L"Usn",                   3, 156, LVCFMT_LEFT },
			{ COLUMN_Date,           L"Date",                  4, 156, LVCFMT_LEFT },
			{ COLUMN_Reason,         L"Reason",                5, 100, LVCFMT_LEFT },
			{ COLUMN_Frn,            L"FRN",                   6, 116, LVCFMT_LEFT },
			{ COLUMN_ParentFrn,      L"Parent FRN",            7, 116, LVCFMT_LEFT },
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

	HRESULT FillItems(PCWSTR pszVolumeName,PCWSTR pszFileName)
	{
		CWaitCursor wait;

		ASSERT( !(pszVolumeName == NULL && pszFileName == NULL) );
		ASSERT( !(pszVolumeName != NULL && pszFileName != NULL) );

		if( (pszVolumeName == NULL && pszFileName == NULL) ||
			(pszVolumeName != NULL && pszFileName != NULL) )
		{
			return E_INVALIDARG;
		}

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		if( m_pJournalBuffer )
			DestroyJournalBuffer( m_pJournalBuffer );

		if( m_pItemList ) {
			delete[] m_pItemList;
			m_pItemList = NULL;
		}

		int cchPath =  32768 + 260;
		WCHAR *szPath = new WCHAR[ cchPath ];

		BOOL bSuccess;

		if( pszVolumeName != NULL )
		{
			if( HasPrefix(L"\\??\\",pszVolumeName) )
			{
				// "\??\C:" -> "\\?\C:"
				// "\??\HarddiskVolume1" -> "\\?\HarddiskVolume1"
				StringCchCopy(szPath,cchPath,L"\\\\?\\");
				StringCchCat(szPath,cchPath,&pszVolumeName[4]);
			}
			else if( HasPrefix(L"\\Device\\",pszVolumeName) )
			{
				// "\Device\HarddiskVolume1" -> "\\?\HarddiskVolume1"
				StringCchCopy(szPath,cchPath,L"\\\\?\\");
				StringCchCat(szPath,cchPath,&pszVolumeName[8]);
			}
			else
			{
				StringCchCopy(szPath,cchPath,pszVolumeName);
			}

			RemoveBackslash(szPath);
		
			_SafeMemFree(m_pszVolumeName);
			m_pszVolumeName = _MemAllocString(szPath);

			bSuccess = CreateJournalBuffer(szPath,(void**)&m_pJournalBuffer);

			if( bSuccess )
				_SafeMemFree(m_pszRawFileName);
		}
		else
		{
			bSuccess = ReadChangeJournalDumpFile(pszFileName,(void**)&m_pJournalBuffer);

			if( bSuccess )
				_SafeMemFree(m_pszVolumeName);
		}

		if( bSuccess )
		{
			SIZE_T cRecords = m_pJournalBuffer->GetTotalRecordCount();

			m_pItemList = new CChangeJournalItem[cRecords];
			ZeroMemory(m_pItemList,sizeof(CChangeJournalItem) * cRecords);

			int cItems = m_pJournalBuffer->GetCount();
			int iItem = 0;
			for(int i = 0; i < cItems; i++) {

				PUSN_RECORD pBlock = (PUSN_RECORD)m_pJournalBuffer->GetBuffer(i);
				SIZE_T BufferBytes = m_pJournalBuffer->GetBufferSize(i);

				PUSN_RECORD UsnRecord = (PUSN_RECORD)(((PUCHAR)pBlock) + sizeof(USN));
				BufferBytes -= sizeof(USN);

				int d = 0;
				while( BufferBytes > 0 )
				{
					m_pItemList[iItem++].UsnRecord = UsnRecord;

					BufferBytes -= UsnRecord->RecordLength;

					UsnRecord = (PUSN_RECORD)(((SIZE_T)UsnRecord) + UsnRecord->RecordLength);
					d++;
				}
			}

			ListView_SetItemCount(m_hWndList,cRecords);

			for(int col_index = 0; col_index <= 6; col_index++)
				ListView_SetColumnWidth(m_hWndList,col_index,LVSCW_AUTOSIZE_USEHEADER);
		}
		else
		{
			SetErrorState( GetLastError() );

			ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE_USEHEADER);
		}

		delete szPath;

		SetRedraw(m_hWndList,TRUE);

		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems(((SELECT_ITEM*)pFile)->pszPath,NULL);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Context Menu
	//

	HMENU CreateFileOperationSubMenu()
	{
		HMENU hMenu;
		hMenu = CreatePopupMenu();
		if( ListView_GetItemCount(m_hWndList) > 0 && m_pszRawFileName == NULL )	
		{
			AppendMenu(hMenu,MF_STRING,ID_FILE_EXPORT_CHANGE_JOURNAL_RAW_DATA,L"Export All Journal Records as Jointed RAW Data File");
			AppendMenu(hMenu,0,0,NULL);
		}
		AppendMenu(hMenu,MF_STRING,ID_FILE_LOAD_CHANGE_JOURNAL_RAW_DATA,L"Load Exported Jointed RAW Data File");
		return hMenu;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
		{
			HMENU hMenu = CreateFileOperationSubMenu();

			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			if( pt.x == -1 && pt.y == -1 )
			{
				pt.x = 0;
				pt.y = 0;
			}

			TrackPopupMenuEx(hMenu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_NONOTIFY,pt.x,pt.y,GetActiveWindow(),NULL);

			DestroyMenu(hMenu);

			return 0;
		}

		CChangeJournalItem *pItem = (CChangeJournalItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		AppendMenu(hMenu,0,0,NULL);
		AppendMenu(hMenu,MF_POPUP,(UINT_PTR)CreateFileOperationSubMenu(),L"&Data Save/Load");

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Sort
	//

	typedef struct _SORT_INFO {
		int iSortDirection;
		int iSortOption;
	} SORT_INFO;

	typedef int (__cdecl *COMPAREPROC)(void *context, const void *elem1, const void *elem2);

	void SortItems(UINT id)
	{
		struct {
			UINT id;
			COMPAREPROC comp_proc;
		} si[] = {
			{COLUMN_Name,          &compareFileName},
			{COLUMN_Reason,        &compareReason},
			{COLUMN_Usn,           &compareUsn},
			{COLUMN_Date,          &compareTimeStamp},
			{COLUMN_FileAttributes,&compareFileAttributes},
			{COLUMN_Frn,           &compareFRN},
			{COLUMN_ParentFrn,     &compareParentFRN},
		};

		COMPAREPROC proc = NULL;
		for(int i = 0; i < _countof(si); i++)
		{
			if( si[i].id == id )
			{
				proc = si[i].comp_proc;
			}
		}

		if( proc != NULL )
		{
			SORT_INFO si;
			si.iSortDirection = m_Sort.Direction;
			si.iSortOption = 0;
			int cItems = ListView_GetItemCount(m_hWndList);
			qsort_s(m_pItemList,cItems,sizeof(CChangeJournalItem),proc,&si);
			RedrawWindow(m_hWndList,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
		}
	}

	static int __cdecl compareFRN(void *context, const void *elem1, const void *elem2)
	{
		//
		// note: 128bit ID is not supported
		//
		PUSN_RECORD p1 = *(PUSN_RECORD*)elem1;
		PUSN_RECORD p2 = *(PUSN_RECORD*)elem2;
		SORT_INFO *si = (SORT_INFO *)context;
		int ret;
	
		DWORDLONG f1 = _UsnGetItem_FileReferenceNumber(p1);
		DWORDLONG f2 = _UsnGetItem_FileReferenceNumber(p2);
	
		ret = _COMP(f1,f2);
	
		ret *= si->iSortDirection;
	
		return  ret;
	}
	
	static int __cdecl compareParentFRN(void *context, const void *elem1, const void *elem2)
	{
		//
		// note: 128bit ID is not supported
		//
		PUSN_RECORD p1 = *(PUSN_RECORD*)elem1;
		PUSN_RECORD p2 = *(PUSN_RECORD*)elem2;
		SORT_INFO *si = (SORT_INFO *)context;
		int ret;
	
		DWORDLONG f1 = _UsnGetItem_ParentFileReferenceNumber(p1);
		DWORDLONG f2 = _UsnGetItem_ParentFileReferenceNumber(p2);
	
		ret = _COMP(f1,f2);
	
		ret *= si->iSortDirection;
	
		return  ret;
	}
	
	static int __cdecl compareFileAttributes(void *context, const void *elem1, const void *elem2)
	{
		PUSN_RECORD p1 = *(PUSN_RECORD*)elem1;
		PUSN_RECORD p2 = *(PUSN_RECORD*)elem2;
		SORT_INFO *si = (SORT_INFO *)context;
		int ret;
	
		ULONG a1 = _UsnGetItem_FileAttributes(p1);
		ULONG a2 = _UsnGetItem_FileAttributes(p2);
	
		if( 0 )
		{
			// compare hex binary
			ret = _COMP(a1,a2);
		}
		else
		{
			// compare attribute character
			WCHAR sz1[32+1],sz2[32+1];
			GetAttributeString(a1,sz1,ARRAYSIZE(sz1));
			GetAttributeString(a2,sz2,ARRAYSIZE(sz2));
			ret = wcscmp(sz1,sz2);
		}
	
		ret *= si->iSortDirection;
	
		return  ret;
	}
	
	static int __cdecl compareFileName(void *context, const void *elem1, const void *elem2)
	{
		PUSN_RECORD p1 = *(PUSN_RECORD*)elem1;
		PUSN_RECORD p2 = *(PUSN_RECORD*)elem2;
		SORT_INFO *si = (SORT_INFO *)context;
		int ret;
	
		UNICODE_STRING us1;
		UNICODE_STRING us2;
	
		us1.Length = us1.MaximumLength = _UsnGetItem_FileNameLength(p1);
		us2.Length = us2.MaximumLength = _UsnGetItem_FileNameLength(p2);
	
		us1.Buffer = (PWSTR)_UsnGetItem_FileName(p1);
		us2.Buffer = (PWSTR)_UsnGetItem_FileName(p2);
	
		ret = CompareUnicodeString(&us1,&us2,TRUE);
	
		ret *= si->iSortDirection;
	
		return ret;
	}
	
	static int __cdecl compareReason(void *context, const void *elem1, const void *elem2)
	{
		PUSN_RECORD p1 = *(PUSN_RECORD*)elem1;
		PUSN_RECORD p2 = *(PUSN_RECORD*)elem2;
		SORT_INFO *si = (SORT_INFO *)context;
		int ret;
	
		DWORD Reason1 = _UsnGetItem_Reason(p1);
		DWORD Reason2 = _UsnGetItem_Reason(p2);
	
		// iSortOption
		//  0==Abbreviations
		//  1==FullText
		//  2==Hex(Raw)
		if( si->iSortOption != 2 )
		{
			WCHAR sz1[MAX_PATH];
			WCHAR sz2[MAX_PATH];
			if( si->iSortOption == 1 )
			{
				GetJournalReasonText(Reason1,sz1,_countof(sz1));
				GetJournalReasonText(Reason2,sz2,_countof(sz2));
			}
			else
			{
				GetJournalReasonAbbreviationsText(Reason1,sz1,_countof(sz1),FALSE);
				GetJournalReasonAbbreviationsText(Reason2,sz2,_countof(sz2),FALSE);
			}
			ret = wcscmp(sz1,sz2);
		}
		else
		{
			ret = _COMP(Reason1,Reason2);
		}
	
		ret *= si->iSortDirection;
	
		return ret;
	}
	
	static int __cdecl compareTimeStamp(void *context, const void *elem1, const void *elem2)
	{
		PUSN_RECORD p1 = *(PUSN_RECORD*)elem1;
		PUSN_RECORD p2 = *(PUSN_RECORD*)elem2;
		SORT_INFO *si = (SORT_INFO *)context;
		int ret;
	
		ULONG64 f1 = _UsnGetItem_TimeStamp64(p1);
		ULONG64 f2 = _UsnGetItem_TimeStamp64(p2);
	
		ret = _COMP(f1,f2);
	
		ret *= si->iSortDirection;
	
		return ret;
	}
	
	static int __cdecl compareUsn(void *context, const void *elem1, const void *elem2)
	{
		PUSN_RECORD p1 = *(PUSN_RECORD*)elem1;
		PUSN_RECORD p2 = *(PUSN_RECORD*)elem2;
		SORT_INFO *si = (SORT_INFO *)context;
		int ret;
	
		USN usn1 = _UsnGetItem_Usn(p1);
		USN usn2 = _UsnGetItem_Usn(p2);
	
		ret = _COMP(usn1,usn2);
	
		ret *= si->iSortDirection;
	
		return ret;
	}
	
	//////////////////////////////////////////////////////////////////////////
	//
	// Commnad Handler
	//

	virtual HRESULT QueryCmdState(UINT uCmdId,UINT *puState)
	{
		switch( uCmdId )
		{
			case ID_EDIT_COPY:
				*puState = ListView_GetSelectedCount(m_hWndList) ?  UPDUI_ENABLED : UPDUI_DISABLED;
				break;
			case ID_VIEW_REFRESH:
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
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
			case ID_FILE_EXPORT_CHANGE_JOURNAL_RAW_DATA:
			{
				OPENFILENAME of;
				WCHAR szFile[MAX_PATH];

				memset( &of, 0, sizeof(OPENFILENAME) );

				szFile[0] = 0;

				of.lStructSize  = sizeof(OPENFILENAME);
				of.hwndOwner    = m_hWnd;
				of.hInstance    = _GetResourceInstance();
				of.lpstrFilter  = (LPWSTR)L"journal dump file\0*.cjdump\0\0";
				of.nFilterIndex = 0;
				of.lpstrFile    = (LPWSTR)szFile;
				of.nMaxFile     = (DWORD)sizeof(szFile);
				of.lpstrTitle   = (LPWSTR)L"Export Change Journal Raw Dump Data";
				of.Flags        = OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;
				of.lpstrDefExt  = (LPWSTR)L"cjdump";

				if (GetSaveFileName(&of))
				{
					SaveChangeJournalDumpFile(this->m_pJournalBuffer,szFile);
				}
				break;
			}
			case ID_FILE_LOAD_CHANGE_JOURNAL_RAW_DATA:
			{
               OPENFILENAME of;
               WCHAR szFile[120];

			   memset( &of, 0, sizeof(OPENFILENAME) );

				szFile[0] = 0;

				of.lStructSize  = sizeof(OPENFILENAME);
				of.hwndOwner    = m_hWnd;
				of.hInstance    = _GetResourceInstance();
				of.lpstrFilter  = (LPWSTR)L"journal dump file\0*.cjdump\0Binary File\0*.bin\0All File\0*.*\0\0";
				of.nFilterIndex = 0;
				of.lpstrFile    = (LPWSTR)szFile;
				of.nMaxFile     = (DWORD)256;
				of.lpstrTitle   = (LPWSTR)L"Open Change Jornal Joined Raw Dump File";
				of.Flags        = OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
				of.lpstrDefExt  = (LPWSTR)L"cjdump";

				if( GetOpenFileName( &of ) )
				{
					_SafeMemFree(m_pszRawFileName);
					m_pszRawFileName = _MemAllocString(szFile);

					FillItems(NULL,m_pszRawFileName);

					DoSort(m_Sort.CurrentSubItem,0);

					SendMessage(GetParent(m_hWnd),WM_NOTIFY_MESSAGE,UI_NOTIFY_CHANGE_TITLE,(LPARAM)PathFindFileName(m_pszRawFileName));
				}
				break;
			}
			default:
				return S_FALSE;
		}
		return S_OK;
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
		FillItems(m_pszVolumeName,m_pszRawFileName);
		DoSort(m_Sort.CurrentSubItem,0);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Find Item
	//

	int m_iStartFindItem;
	int m_iFirstMatchItem;
	int m_iLastMatchItem;

	LRESULT OnFindItem(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;
		switch( LOWORD(wParam) )
		{
			case FIND_QUERYOPENDIALOG:
				return 0; // 0:accept 1:prevent

			case FIND_CLOSEDIALOG:
				m_iStartFindItem = -1;
				break;

			case FIND_SEARCH:
				m_iStartFindItem = ListViewEx_GetCurSel(m_hWndList);
				if( m_iStartFindItem == -1 )
					m_iStartFindItem = 0;
				SearchItem(lpfr->lpstrFindWhat,
						(BOOL) (lpfr->Flags & FR_DOWN), 
						(BOOL) (lpfr->Flags & FR_MATCHCASE)); 
				break;
			case FIND_SEARCH_NEXT:
			{
				int cItems = ListView_GetItemCount(m_hWndList);
				m_iStartFindItem = ListViewEx_GetCurSel(m_hWndList);
				if( m_iStartFindItem == -1 )
					m_iStartFindItem = 0;
				else
				{
					m_iStartFindItem = m_iStartFindItem + ((lpfr->Flags & FR_DOWN) ? 1 : -1);
					if( m_iStartFindItem <= 0 )
						m_iStartFindItem = cItems-1;
					else if( m_iStartFindItem >= cItems )
						m_iStartFindItem = 0;
				}
				SearchItem(lpfr->lpstrFindWhat,
						(BOOL) (lpfr->Flags & FR_DOWN), 
						(BOOL) (lpfr->Flags & FR_MATCHCASE)); 
				break;
			}
		}
		return 0;
	}

	VOID SearchItem(PWSTR pszFindText,BOOL Down,BOOL MatchCase)
	{
		int iItem,col,cItems,cColumns;

		const int cchText = MAX_PATH;
		WCHAR szText[cchText];

		cItems = ListView_GetItemCount(m_hWndList);
		cColumns = ListViewEx_GetColumnCount(m_hWndList);

		iItem = m_iStartFindItem;

		for(;;)
		{
			for(col = 0; col < cColumns; col++)
			{
				ListView_GetItemText(m_hWndList,iItem,col,szText,cchText);

				if( StrStrI(szText,pszFindText) != 0 )
				{
					ListViewEx_ClearSelectAll(m_hWndList,TRUE);
					ListView_SetItemState(m_hWndList,iItem,LVNI_SELECTED|LVNI_FOCUSED,LVNI_SELECTED|LVNI_FOCUSED);
					ListView_EnsureVisible(m_hWndList,iItem,FALSE);
					goto __found;
				}
			}

			Down ? iItem++ : iItem--;

			// lap around
			if( iItem >= cItems )
				iItem = 0;
			else if( iItem < 0 )
				iItem = cItems-1;

			if( iItem == m_iStartFindItem )
			{
				MessageBeep(MB_ICONSTOP);
				break; // not found
			}
		}

 __found:
		return;
	}
};
