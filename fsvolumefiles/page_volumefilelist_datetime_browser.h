#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_volumefilelist_datetime_browser.h                                   *
//*                                                                           *
//*  File timestamp browser only for NTFS format partition.                   *
//*                                                                           *
//*  Author:  YAMASHITA Katsuhiro                                             *
//*                                                                           *
//*  History: 2026-09-26 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "page_volumefilelist_filemanager.h"
#include "ntvolumenames.h"
#include "ntobjecthelp.h"
#include "fileitemlist.h"

#define _CLR_DIFFCOLUMN RGB(255,233,233)

class CFileDateTimeItem : public CFileItemEx
{
public:
	CFileDateTimeItem(PCWSTR Path,PCWSTR FileName) : CFileItemEx(Path,FileName)
	{
	}
	~CFileDateTimeItem()
	{
	}
};

struct CFileDateTimeLvItem : public CFileLvItem
{
	CFileDateTimeLvItem(){}
	~CFileDateTimeLvItem(){}
};

typedef struct _FILEITEM_DATETIME_EX
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
} FILEITEM_DATETIME_EX;

template <class T>
class CFileDateTimeBrowserPage : public T
{
	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_VOLUMEFILES_DATETIME; }
	virtual PCWSTR GetWindowFrameTitle() const { return L"File Timestamp Browser"; }

public:
	CFileDateTimeBrowserPage()
	{
		m_pszCurDir = _MemAllocString( L"" ); // dummy, not use
	}

	virtual ~CFileDateTimeBrowserPage()
	{
	}

	virtual DWORD DefListViewStyle() const
	{
		return (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS );
	}

	HRESULT GetFileNameDateTime(HANDLE hVolume,LONGLONG FileId,FILEITEM_DATETIME_EX *pFileItemDateTimeEx)
	{
		HRESULT hr = E_FAIL;

		ULONG Result;
		NTFS_FILE_RECORD_OUTPUT_BUFFER *pBuf;
		if( (Result = GetNtfsFileRecordInformation( NULL, hVolume, FileId, (PVOID*)&pBuf, NULL )) == NO_ERROR )
		{
			UCHAR buf[1024];
			ATTRIBUTE_RECORD_HEADER *pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)buf;
			if( (hr = FindNtfsRecordAttribueFromBuffer(pBuf,$FILE_NAME,-1,pAttrRecHdr,1024)) == S_OK )
			{
				if( pAttrRecHdr->FormCode == RESIDENT_FORM )
				{
					FILE_NAME *pfn = (FILE_NAME *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset);

					pFileItemDateTimeEx->LastWriteTime.QuadPart  = pfn->BasicInfo.LastModificationTime;
					pFileItemDateTimeEx->CreationTime.QuadPart   = pfn->BasicInfo.CreationTime;
					pFileItemDateTimeEx->LastAccessTime.QuadPart = pfn->BasicInfo.LastAccessTime;
					pFileItemDateTimeEx->ChangeTime.QuadPart     = pfn->BasicInfo.LastChangeTime;
				}
			}

			FreeNtfsMemory(pBuf);
		}
		else
		{
			hr = E_FAIL;
		}

		return hr;
	}

	virtual BOOL GetCustomDrawColumnColor(int idColumn,COLORREF& crText,COLORREF& crTextBk,LPARAM lParam)
	{
		BOOL bUpdate = FALSE;

		CFileLvItem *pItem = (CFileLvItem *)lParam;

		if( pItem->pFI->lParam == 0 )
			return FALSE;

		FILEITEM_DATETIME_EX *pFileItemDateTimeEx = (FILEITEM_DATETIME_EX *)pItem->pFI->lParam;
		COLORREF crDiffColumn = _CLR_DIFFCOLUMN;
		COLORREF crDiffColumnText = crText; // no change

		switch( idColumn )
		{
			case COLUMN_LastWriteTimeFN:
				if( pFileItemDateTimeEx->LastWriteTime.QuadPart != 0 )
				{
					if( pFileItemDateTimeEx->LastWriteTime.QuadPart != pItem->pFI->LastWriteTime.QuadPart )
					{
						crText   = crDiffColumnText;
						crTextBk = crDiffColumn;
						bUpdate = TRUE;
					}
				}
				break;
			case COLUMN_CreationTimeFN:
				if( pFileItemDateTimeEx->CreationTime.QuadPart != 0 )
				{
					if( pFileItemDateTimeEx->CreationTime.QuadPart != pItem->pFI->CreationTime.QuadPart )
					{
						crText   = crDiffColumnText;
						crTextBk = crDiffColumn;
						bUpdate = TRUE;
					}
				}
				break;
			case COLUMN_LastAccessTimeFN:
				if( pFileItemDateTimeEx->LastAccessTime.QuadPart != 0 )
				{
					if( pFileItemDateTimeEx->LastAccessTime.QuadPart != pItem->pFI->LastAccessTime.QuadPart )
					{
						crText   = crDiffColumnText;
						crTextBk = crDiffColumn;
						bUpdate = TRUE;
					}
				}
				break;
			case COLUMN_ChangeTimeFN:
				if( pFileItemDateTimeEx->ChangeTime.QuadPart != 0 )
				{
					if( pFileItemDateTimeEx->ChangeTime.QuadPart != pItem->pFI->ChangeTime.QuadPart )
					{
						crText   = crDiffColumnText;
						crTextBk = crDiffColumn;
						bUpdate = TRUE;
					}
				}
				break;
		}
		return bUpdate;
	}

	virtual void UpdateFileItemInformation(HANDLE /*hVolume*/,HANDLE /*hFile*/,CFileItemEx *pFI)
	{
		NTSTATUS Status;

		PWSTR RootDirectory=NULL,RootRelativePath=NULL;
		ULONG cchRootDirectory=0,cchRootRelativePath=0;

		if( m_pszCurDir == NULL || *m_pszCurDir == L'\0' )
			SplitRootPath_W(pFI->hdr.Path,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
		else if( pFI->hdr.Path == NULL )
			SplitRootPath_W(m_pszCurDir,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
		else
			return; // invalid case

		InitCostlyFileItemInformation(pFI);

		HANDLE hVolume = NULL;

		Status = NtDosOpenVolume(&hVolume,RootDirectory,FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_SYNCHRONOUS_IO_NONALERT);
		if( Status != NO_ERROR )
		{
			FreeMemory(RootRelativePath);
			RootRelativePath = DuplicateString(m_pszCurDir); // duplicate full-path string
			hVolume = NULL;
		}

		if( hVolume )
		{
			FILEITEM_DATETIME_EX *pFileItemDateTimeEx = new FILEITEM_DATETIME_EX;

			if( pFileItemDateTimeEx != NULL )
			{
				ZeroMemory(pFileItemDateTimeEx,sizeof(FILEITEM_DATETIME_EX));

				GetFileNameDateTime(hVolume,pFI->FileId.QuadPart,pFileItemDateTimeEx);

				pFI->lParam = (LPARAM)pFileItemDateTimeEx;
			}
		}

		FreeMemory(RootRelativePath);
		FreeMemory(RootDirectory);

		if( hVolume )
			CloseHandle(hVolume);

		// We've already tried to get the data.
		pFI->ItemTypeFlag |= _FLG_COSTLY_DATA;
	}

	virtual void GetCostlyFileInformationData(CFileItemEx *pFI)
	{
		UpdateFileItemInformation(NULL,NULL,pFI);
	}

	virtual HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;
		ASSERT(pSel != NULL);
		return CFileListPage::FillItems(pSel);
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return CFileListPage::UpdateData(pFile);
	}

	virtual LRESULT OnGetDispText(int id,NMLVDISPINFO *pdi, CFileLvItem *pFileInfoItem)
	{
		CFileDateTimeLvItem *pItem = (CFileDateTimeLvItem *)pFileInfoItem;

		switch( id )
		{
			case COLUMN_VolumeRelativePath:
			{
				*pdi->item.pszText = NULL;
				if( pItem->pFI->hdr.Path )
				{
					UNICODE_STRING u;
					SplitVolumeRelativePath(pItem->pFI->hdr.Path,NULL,&u);
					if( u.Buffer )
					{
						pdi->item.pszText = u.Buffer;
						RemoveBackslash(pdi->item.pszText);
					}
				}
				return 0;
			}
			case COLUMN_LastWriteTimeFN:
			case COLUMN_CreationTimeFN:
			case COLUMN_LastAccessTimeFN:
			case COLUMN_ChangeTimeFN:
			{
				OnDisp_DateTimeFN(id,pdi);
				return 0;
			}
		}

		if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
		{
			return (this->*m_disp_proc[ id ].pfn)(id,pdi);
		}

		return 0;
	}

	LRESULT OnDisp_DateTimeFN(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		CFileLvItem *pItem = (CFileLvItem *)pnmlvdi->item.lParam;
		LONGLONG dt = 0;

		if(  pItem->pFI->lParam )
		{
			switch( id )
			{
				case COLUMN_CreationTimeFN:
					dt = ((FILEITEM_DATETIME_EX *)(pItem->pFI->lParam))->CreationTime.QuadPart;
					break;
				case COLUMN_LastWriteTimeFN:
					dt = ((FILEITEM_DATETIME_EX *)(pItem->pFI->lParam))->LastWriteTime.QuadPart;
					break;
				case COLUMN_LastAccessTimeFN:
					dt = ((FILEITEM_DATETIME_EX *)(pItem->pFI->lParam))->LastAccessTime.QuadPart;
					break;
				case COLUMN_ChangeTimeFN:
					dt = ((FILEITEM_DATETIME_EX *)(pItem->pFI->lParam))->ChangeTime.QuadPart;
					break;
			}
		}

		UINT fmt = m_columnShowStyleFlags[ id ];

		formatDateTime(pnmlvdi,dt,fmt);

		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;

		if( pnmia->iItem == -1 )
			return 0;

		CFileLvItem *pItem = (CFileLvItem *)ListViewEx_GetItemData(m_hWndList, pnmia->iItem);

		if( pItem == NULL )
			return 0;

		if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			CFileListPage::OpenWithExplorer(pItem);
			return 0;
		}

		return CFileListPage::OnItemActivate(pnmhdr);
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CFileLvItem *pItem = (CFileLvItem *)pnmlv->lParam;

		if( pItem->pFI->lParam )
		{
			delete (FILEITEM_DATETIME_EX *)pItem->pFI->lParam;
		}

		CFileListPage::OnDeleteItem(pnmhdr);

		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_NOTIFY:
			{
				NMHDR *pnmhdr = (NMHDR *)lParam;
				switch(pnmhdr->code)
				{
					case LVN_DELETEITEM:
						return OnDeleteItem(pnmhdr);
				}
				break;
			}
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
		}
		return T::WndProc(hWnd,uMsg,wParam,lParam);
	}

	void InitGroup()
	{
		CFileListPage::InitGroup();
	}

	virtual void SetupColumnNameSet()
	{
		static COLUMN_NAME column_name_map[] = {
			{ COLUMN_Name,                L"Name",                  0},
			{ COLUMN_FileAttributes,      L"Attributes",            0},
			{ COLUMN_LastWriteTime,       L"LastWriteTime",         0},
			{ COLUMN_CreationTime,        L"CreationTime",          0},
			{ COLUMN_LastAccessTime,      L"LastAccessTime",        0},
			{ COLUMN_ChangeTime,          L"ChangeTime",            0},
			{ COLUMN_LastWriteTimeFN,     L"LastWriteTimeFN",       0},
			{ COLUMN_CreationTimeFN,      L"CreationTimeFN",        0},
			{ COLUMN_LastAccessTimeFN,    L"LastAccessTimeFN",      0},
			{ COLUMN_ChangeTimeFN,        L"ChangeTimeFN",          0},
		};
		m_columns.SetColumnNameMap( _countof(column_name_map), column_name_map );
	}

	virtual void InitColumnDefinitions()
	{
		static COLUMN def_columns[] = {
			{ COLUMN_Name,                L"Name",                  1, 0, LVCFMT_LEFT },
			{ COLUMN_FileAttributes,      L"Attributes",            2, 0, LVCFMT_RIGHT|LVCFMT_SPLITBUTTON },
			{ COLUMN_LastWriteTime,       L"Last Write",            3, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_LastWriteTimeFN,     L"Last Write $FN",        4, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_CreationTime,        L"Creation",              5, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_CreationTimeFN,      L"Creation $FN",          6, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_LastAccessTime,      L"Last Access",           7, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_LastAccessTimeFN,    L"Last Access $FN",       8, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_ChangeTime,          L"Change",                9, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
			{ COLUMN_ChangeTimeFN,        L"Change $FN",           10, 0, LVCFMT_LEFT|LVCFMT_SPLITBUTTON },
		};

		m_columns.SetDefaultColumns(def_columns,ARRAYSIZE(def_columns));
	}

	virtual BOOL LoadColumns(HWND hWndList,PCWSTR pszSectionName,PCWSTR pszCurrentSortColumn)
	{
		COLUMN_TABLE *pcoltbl;

		WCHAR buf[] = 
				L"Name=220\0"
				L"Attributes=80\0"
				L"LastWriteTime=180\0"
				L"LastWriteTimeFN=180\0"
				L"CreationTime=180\0"
				L"CreationTimeFN=180\0"
				L"LastAccessTime=180\0"
				L"LastAccessTimeFN=180\0"
				L"ChangeTime=180\0"
				L"ChangeTimeFN=180\0";

		int cb = sizeof(buf);

		if( m_columns.LoadUserDefinitionColumnTableFromText(&pcoltbl,buf,cb) == 0)
			return FALSE;

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

	virtual void init_compare_proc_def_table()
	{
		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CFileListPage,CFileLvItem>[ COLUMN_MaxItem ];
		ASSERT(m_comp_proc != NULL);

		CFileListPage::init_compare_proc_def_table();

		static COMPARE_HANDLER_PROC_DEF<CFileDateTimeBrowserPage,CFileDateTimeLvItem> _comp_proc[] = 
		{
			{COLUMN_LastWriteTimeFN,  &CFileDateTimeBrowserPage::_comp_datetime_fn}, 
			{COLUMN_CreationTimeFN,   &CFileDateTimeBrowserPage::_comp_datetime_fn}, 
			{COLUMN_LastAccessTimeFN, &CFileDateTimeBrowserPage::_comp_datetime_fn}, 
			{COLUMN_ChangeTimeFN,     &CFileDateTimeBrowserPage::_comp_datetime_fn}, 
		};
		int i;
		for(i = 0; i < _countof(_comp_proc); i++)
		{
			m_comp_proc[ _comp_proc[i].colid ].colid  = _comp_proc[i].colid;
			m_comp_proc[ _comp_proc[i].colid ].proc   = (COMPHANDLER)_comp_proc[i].proc;
		}
	}

	int _comp_datetime_fn(CFileDateTimeLvItem *pItem1,CFileDateTimeLvItem *pItem2, const void *p)
	{
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;
		LONGLONG cb1,cb2;

		// If the pItem->pFI->lParam is null, meaning uninitialized by date time data. 
		// Therefore, we get the data it GetCostlyFileInformationData() function call.
		if( pItem1->pFI->lParam == 0 )
		{
			GetCostlyFileInformationData(pItem1->pFI);
		}
		
		if( pItem2->pFI->lParam == 0 )
		{
			GetCostlyFileInformationData(pItem2->pFI);
		}

		ASSERT( pItem1->pFI->lParam != 0 );
		ASSERT( pItem2->pFI->lParam != 0 );

		switch( op->id )
		{
			case COLUMN_LastWriteTimeFN:
				cb1 = ((FILEITEM_DATETIME_EX *)pItem1->pFI->lParam)->LastWriteTime.QuadPart;
				cb2 = ((FILEITEM_DATETIME_EX *)pItem2->pFI->lParam)->LastWriteTime.QuadPart;
				break;
			case COLUMN_CreationTimeFN:
				cb1 = ((FILEITEM_DATETIME_EX *)pItem1->pFI->lParam)->CreationTime.QuadPart;
				cb2 = ((FILEITEM_DATETIME_EX *)pItem2->pFI->lParam)->CreationTime.QuadPart;
				break;
			case COLUMN_LastAccessTimeFN:
				cb1 = ((FILEITEM_DATETIME_EX *)pItem1->pFI->lParam)->LastAccessTime.QuadPart;
				cb2 = ((FILEITEM_DATETIME_EX *)pItem2->pFI->lParam)->LastAccessTime.QuadPart;
				break;
			case COLUMN_ChangeTime:
				cb1 = ((FILEITEM_DATETIME_EX *)pItem1->pFI->lParam)->ChangeTime.QuadPart;
				cb2 = ((FILEITEM_DATETIME_EX *)pItem2->pFI->lParam)->ChangeTime.QuadPart;
				break;
		}

		return _COMP(cb1,cb2);
	}

	virtual HRESULT MakeContextMenu(HMENU hMenu)
	{
		AppendMenu(hMenu,MF_STRING,ID_OPEN,L"&Open");
		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		SetMenuDefaultItem(hMenu,ID_OPEN,FALSE);

		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_FILE_SIMPLECHECK,L"Count Selected Files");

		AppendMenu(hMenu,MF_STRING,0,NULL);
		HMENU hSubMenu;
		hSubMenu = CreatePopupMenu();
		{
			AppendMenu(hSubMenu,MF_STRING,ID_FILE_SIMPLEDUMP,L"Simple Binary &Dump");
			AppendMenu(hSubMenu,MF_STRING,0,0);
			AppendMenu(hSubMenu,MF_STRING,ID_FILE_CLUSTER_INFORMATION,L"&Cluster Information");
			AppendMenu(hSubMenu,MF_STRING,ID_FILE_STREAM_INFORMATION,L"&Stream Information");
			AppendMenu(hSubMenu,MF_STRING,ID_FILE_NTFS_RECORD_INFORMATION,L"&NTFS Record Information");
		}
		AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hSubMenu,L"&Inspect File");

		if( m_pOpenApplications == NULL )
			CreateApplicationList(&m_pOpenApplications);

		if( m_pOpenApplications )
		{
			HMENU hAppMenu = CreatePopupMenu();
			ULONG cApps = 0;
			m_pOpenApplications->GetItemCount( &cApps );
	
			WCHAR szName[MAX_PATH];
	
			for(ULONG i = 0; i < cApps; i++)
			{
				if( m_pOpenApplications->IsSeparator(i) == S_FALSE )
				{
					m_pOpenApplications->GetFriendlyName(i,szName,MAX_PATH);
					AppendMenu(hAppMenu,MF_STRING,ID_OPEN_APP_FIRST+i,szName);
				}
				else
				{
					AppendMenu(hAppMenu,MF_STRING,0,0);
				}
			}	
	
			AppendMenu(hMenu,MF_STRING,0,0);
			AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hAppMenu,L"Open with Application");
		}

		return S_OK;
	}

	virtual HRESULT QueryCmdState(UINT uCmdId,UINT *puState)
	{
		switch( uCmdId )
		{
			case ID_FILE_STREAM_INFORMATION:
			case ID_FILE_NTFS_RECORD_INFORMATION:
				*puState = UPDUI_DISABLED;
				if( ListView_GetSelectedCount(m_hWndList) == 1 )
				{
					CFileLvItem *pItem = (CFileLvItem *)ListViewEx_GetCurItemData(m_hWndList);
					if( pItem->pFI->hdr.Path && (pItem->pFI->FileSystemFlags & FILE_NAMED_STREAMS) )
					{
						*puState = UPDUI_ENABLED;
					}
				}
				return S_OK;
		}
		return T::QueryCmdState(uCmdId,puState);
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		return T::InvokeCommand(CmdId);
	}
};
