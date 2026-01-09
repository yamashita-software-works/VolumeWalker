#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_filelist_manager.h                                                  *
//*                                                                           *
//*  File Manager                                                             *
//*                                                                           *
//*  Author: YAMASHITA Katsuhiro                                              *
//*                                                                           *
//*  History: 2024-11-29 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "page_volumefilelist.h"
#if _ENABLE_FILELIST_DRAGFILE || _ENABLE_FILELIST_DROPFILE
#include "page_volumefilelist_drag_and_drop.h"
#endif
#if _ENABLE_FILE_MANAGER
#include "page_volumefilelist_commandhandler.h"
#endif

//*****************************************************************************
//
//  CFileManagerPage
//
//  PURPOSE: File manager page class
//
//*****************************************************************************
class CFileManagerPage :
	public CFileListPage
#if _ENABLE_FILE_MANAGER
	, public CFileCommandHandler<CFileManagerPage>
#endif
#if _ENABLE_FILELIST_DRAGFILE
	, public CDragHandler<CFileManagerPage>
#endif
#if _ENABLE_FILELIST_DROPFILE
	, public CDropHandler<CFileManagerPage>
	, public CClipboardFileList<CFileManagerPage>
#endif
{
protected:
	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_VOLUMEFILEMANAGER; }

	virtual BOOL LoadColumns(HWND hWndList,PCWSTR pszColumnLayout,PCWSTR pszCurrentSortColumn)
	{
		COLUMN_TABLE *pcoltbl;

		PWSTR ptr;

		int cb = 0;
		if( pszColumnLayout != NULL && *pszColumnLayout )
		{
			ptr = ColumnList_ConvertAllocSzToMszColumnString(pszColumnLayout);
			cb = ColumnList_GetMszColumnStringSizeCb(ptr);
		}
		else
		{
			return FALSE;
		}

		int iRet = m_columns.LoadUserDefinitionColumnTableFromText(&pcoltbl,ptr,cb);

		if( pszColumnLayout != NULL && *pszColumnLayout != L'\0' )
			ColumnList_FreeString(ptr);

		if( iRet == 0 )
		{
			return FALSE;
		}

		InsertColumns(hWndList,pcoltbl);

		m_columns.FreeUserDefinitionColumnTable(pcoltbl);

		LARGE_INTEGER li;
		PCWSTR pszSortColumn;
		if( pszCurrentSortColumn && *pszCurrentSortColumn )
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

public:
	CFileManagerPage()
	{
		CFileListPage::m_bFillCostlyData = TRUE;
	}

	virtual HRESULT QueryCmdState(UINT uCmdId,UINT *puState)
	{
#if _ENABLE_FILE_MANAGER
		if( CFileCommandHandler::QueryCmdState(uCmdId,puState) == S_OK )
			return S_OK;
#endif
		return CFileListPage::QueryCmdState(uCmdId,puState);
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
#if _ENABLE_FILE_MANAGER
		if( CFileCommandHandler::InvokeCommand(CmdId) == S_OK )
			return S_OK;
#endif
		return CFileListPage::InvokeCommand(CmdId);
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		//
		// Initialize File Command Handler
		//
#if _ENABLE_FILE_MANAGER
		CFileCommandHandler::Init();
#endif
		//
		// Create drop target.
		//
		if( GetConsoleId() ==  VOLUME_CONSOLE_VOLUMEFILEMANAGER )
		{
#if _ENABLE_FILELIST_DROPFILE
			m_pDropTarget = new CDropTarget;
			m_pDropTarget->AddRef();
			m_pDropTarget->RegisterDropWindow( m_hWnd, this );
			m_pDropTarget->EnableDrop( TRUE );
#endif
		}

		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CFileListPage::OnDestroy(hWnd,uMsg,wParam,lParam);
#if _ENABLE_FILE_MANAGER
		CFileCommandHandler::Destroy();
#endif
#if _ENABLE_FILELIST_DROPFILE
		if( m_pDropTarget != NULL )
		{
			m_pDropTarget->RevokeDragDrop();
			m_pDropTarget->Release();
			m_pDropTarget = NULL;
		}
#endif
		return 0;
	}

	virtual HRESULT MakeContextMenu(HMENU hMenu)
	{
		CFileListPage::MakeContextMenu(hMenu);
#if _ENABLE_FILE_MANAGER
		AppendMenu(hMenu,MF_STRING,0,NULL);
		CFileCommandHandler::MakeContextMenu(hMenu);
#endif
		return S_OK;
	}

public:
	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
#if _ENABLE_FILELIST_DRAGFILE || _ENABLE_FILELIST_DROPFILE
			case WM_NOTIFY:
			{
				NMHDR *pnmhdr = (NMHDR *)lParam;

				switch( pnmhdr->code )
				{
					case LVN_BEGINDRAG:
					case LVN_BEGINRDRAG:
						return CDragHandler::OnBeginDrag(pnmhdr,m_hWndList,GetPath());
				}
				break;
			}
#endif
			case WM_CREATE:
			{
				CFileListPage::OnCreate(hWnd,uMsg,wParam,lParam);
				return OnCreate(hWnd,uMsg,wParam,lParam);
			}
			case WM_DESTROY:
			{
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			}
			case WM_PRETRANSLATEMESSAGE:
			{
				MSG *pmsg = (MSG *)lParam;
#if _ENABLE_FILE_MANAGER
				if( pmsg )
				{
					return CFileCommandHandler::PreTranslateMessage(pmsg);
				}
#endif
				return 0;
			}
			default:
			{
#if _ENABLE_FILE_MANAGER
				LRESULT lResult;
				BOOL bResult = FALSE;
				lResult = CFileCommandHandler::MessageHandler(hWnd,uMsg,wParam,lParam,&bResult);
				if( bResult )
					return lResult;
#endif
				break;
			}
		}
		return CFileListPage::WndProc(hWnd,uMsg,wParam,lParam);
	}
};
