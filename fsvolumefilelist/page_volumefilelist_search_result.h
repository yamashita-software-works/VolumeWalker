#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_volumefilelist_searchresult.h                                       *
//*                                                                           *
//*  Search file result page                                                  *
//*                                                                           *
//*  Author:  YAMASHITA Katsuhiro                                             *
//*                                                                           *
//*  History: 2024-11-26 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "fsvolumefilelist.h"
#include "page_volumefilelist.h"
#include "ntvolumenames.h"
#include "ntobjecthelp.h"

struct CSearchResultLvItem : public CFileLvItem
{
	CSearchResultLvItem()
	{
	}
};

class CFileSearchResultPage : public CFileListPage
{
	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT; }

public:
	CFileSearchResultPage()
	{
		m_pszCurDir = _MemAllocString( L"" ); // dummy, not use
	}

	virtual ~CFileSearchResultPage()
	{
	}

	virtual HWND CreateHeaderBar(HWND hWnd) { return NULL; }

	virtual DWORD DefListViewStyle() const
	{
		return (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS );
	}

	virtual HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		HRESULT hr = E_FAIL;

		//
		// The Handle is using for FILXxx functions handling file item list.
		// This pointer list is container of pointers to file items that search result.
		//
		HANDLE Handle = (HANDLE)pSel->pszPath;
		if( Handle == NULL )
			return E_INVALIDARG;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		int cItems = FILGetItemCount(Handle);

		for(int i = 0; i < cItems; i++)
		{
			FILEITEMEX *pFI = FILGetItemExPtr(Handle,i);

			ASSERT(pFI != NULL);

			pFI->ItemTypeFlag |= _FLG_COSTRY_DATA;

			Insert(m_hWndList,-1,i,(CFileItemEx*)pFI,0,I_IMAGECALLBACK);
		}

		FILDestroy( Handle );

		SetRedraw(m_hWndList,TRUE);

		return hr;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		SELECT_ITEM *pSel = (SELECT_ITEM *)pFile;
		SendMessage(GetActiveWindow(),PM_UPDATETITLE,0,(LPARAM)L"Search Result"); // todo: title
 		return FillItems(pSel);
	}

	virtual LRESULT OnGetDispImage(int id,NMLVDISPINFO *pdi, CFileLvItem *pItem)
	{
		pdi->item.iImage = GetShellFileImageListIndex(NULL,pItem->pFI->hdr.FileName,pItem->pFI->FileAttributes);
		pdi->item.mask |= LVIF_DI_SETITEM;
		return 0;
	}

	virtual LRESULT OnGetDispText(int id,NMLVDISPINFO *pdi, CFileLvItem *pFileInfoItem)
	{
		CSearchResultLvItem *pItem = (CSearchResultLvItem *)pFileInfoItem;

		switch( id )
		{
			case COLUMN_VolumeRelativePath:
			{
				pdi->item.pszText = pItem->pFI->hdr.Path;

				UNICODE_STRING u;
				SplitVolumeRelativePath(pItem->pFI->hdr.Path,NULL,&u);
				pdi->item.pszText = u.Buffer;
				RemoveBackslash(pdi->item.pszText);
				return 0;
			}
		}

		if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
		{
			return (this->*m_disp_proc[ id ].pfn)(id,pdi);
		}

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
		DeleteItemExPtr(pItem->pFI);
		delete pItem;
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
					case LVN_ITEMACTIVATE:
						return OnItemActivate(pnmhdr);
					case LVN_DELETEITEM:
						return OnDeleteItem(pnmhdr);
				}
				break;
			}
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
		}
		return CFileListPage::WndProc(hWnd,uMsg,wParam,lParam);
	}

	void InitGroup()
	{
		CFileListPage::InitGroup();
	}

	virtual void SetupColumnNameSet()
	{
		CFileListPage::SetupColumnNameSet();
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
			{ COLUMN_VolumeRelativePath,  L"Volume Relative Path", 16, 120, LVCFMT_LEFT },
		};

		m_columns.SetDefaultColumns(def_columns,ARRAYSIZE(def_columns));
	}

	virtual BOOL LoadColumns(HWND hWndList,PCWSTR pszSectionName)
	{
		COLUMN_TABLE *pcoltbl;

		WCHAR buf[] = 
				L"Name=180\0"
				L"VolumeRelativePath=240\0"
				L"FileId=140\0"
				L"Attributes=80\0"
				L"Lcn=100\0"
				L"PhysicalDrive=120\0"
				L"PhysicalOffset=118\0"
				L"AllocationSize=116\0"
				L"EndOfFile=116\0"
				L"EaSize=94\0"
				L"LastWriteTime=180\0"
				L"CreationTime=180\0"
				L"CreationTime=180\0"
				L"LastAccessTime=180\0"
				L"ChangeTime=180\0";

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
	}

	virtual void init_compare_proc_def_table()
	{
		m_comp_proc = new COMPARE_HANDLER_PROC_DEF<CFileListPage,CFileLvItem>[ COLUMN_MaxItem ];
		ASSERT(m_comp_proc != NULL);

		CFileListPage::init_compare_proc_def_table();

		static COMPARE_HANDLER_PROC_DEF<CFileSearchResultPage,CSearchResultLvItem> _comp_proc[] = 
		{
			{COLUMN_VolumeRelativePath,  &CFileSearchResultPage::_comp_volumerelativepath}, 
		};
		int i;
		for(i = 0; i < _countof(_comp_proc); i++)
		{
			m_comp_proc[ _comp_proc[i].colid ].colid  = _comp_proc[i].colid;
			m_comp_proc[ _comp_proc[i].colid ].proc   = (COMPHANDLER)_comp_proc[i].proc;
		}
	}

	int _comp_volumerelativepath(CSearchResultLvItem *pItem1,CSearchResultLvItem *pItem2, const void *p)
	{
		SORT_PARAM<CFileListPage> *op = (SORT_PARAM<CFileListPage> *)p;

		PWSTR p1 = pItem1->pFI->hdr.Path;
		PWSTR p2 = pItem2->pFI->hdr.Path;

		return StrCmpI(p1,p2);
	}

	virtual HRESULT OnInitPage(PVOID,DWORD dwStyle,PVOID)
	{
		CFileSearchResultPage::InitListView();

		if( !LoadColumns(m_hWndList,NULL) )
		{
			InsertDefaultColumns(m_hWndList);
		}

		InitGroup();

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		return S_OK;
	}

	virtual HRESULT SeveConfig()
	{
		return S_OK;
	}

	virtual HRESULT MakeContextMenu(HMENU hMenu)
	{
		AppendMenu(hMenu,MF_STRING,ID_OPEN,L"&Open");
		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		SetMenuDefaultItem(hMenu,ID_OPEN,FALSE);

		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_FILE_SIMPLECHECK,L"Count Selected Files");

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
			case ID_UP_DIR:
			case ID_HISTORY_BACKWARD:
			case ID_HISTORY_FORWARD:
				*puState = UPDUI_DISABLED;
				break;
		}
		return CFileListPage::QueryCmdState(uCmdId,puState);
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		return CFileListPage::InvokeCommand(CmdId);
	}
};
