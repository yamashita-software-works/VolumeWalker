#pragma once
//****************************************************************************
//
//  filesview.h
//
//  Implements the file list view base window.
//
//  Author: YAMASHITA Katsuhiro
//
//  Create: 2023.06.29
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "pagewbdbase.h"
#include "page_volumefilelist.h"
#include "page_volumefilelist_search_result.h"
#include "interface.h"

class CFilesPageHost
{
	CPageWndBase *m_pPage;
	CPageWndBase *m_pPageTable[VOLUME_CONSOLE_MAX_ID];
	int m_nView;
public:
	HWND m_hWnd;
	HWND GetPageHWND()
	{
		if( m_pPage == NULL )
			return NULL;
		return m_pPage->GetHwnd();
	}

public:
	CFilesPageHost()
	{
		m_hWnd = NULL;
		m_pPage = NULL;
		m_nView = -1;
		memset(m_pPageTable,0,sizeof(m_pPageTable));
	}

	virtual ~CFilesPageHost()
	{
	}

	HRESULT UpdateLayout(int cx,int cy,BOOL absSplitPos=FALSE)
	{
		int cyHeaderBar = 0;
		HDWP hdwp = BeginDeferWindowPos(2);
		if( m_pPage )
		{
			DeferWindowPos(hdwp,m_pPage->GetHwnd(),NULL,0,cyHeaderBar,cx,cy-cyHeaderBar,SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);
		return S_OK;
	}

public:
	HRESULT UpdateData(SELECT_ITEM *pFile)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->UpdateData(pFile);
	}

	HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->QueryCmdState(CmdId,State);
	}

	HRESULT InvokeCommand(UINT CmdId)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->InvokeCommand(CmdId);
	}

	HRESULT SetState(int idx,ULONG ul)
	{
		return E_NOTIMPL;
	}

	HRESULT GetString(int t,LPWSTR pszString,int cch)
	{
		if( m_pPage )
			return m_pPage->GetString(t,pszString,cch);
		return E_FAIL;
	}

	template <class T>
	CPageWndBase *GetOrAllocWndObjct(int wndId)
	{
		CPageWndBase *pobj;
		if( m_pPageTable[ wndId ] == NULL )
		{
			pobj = (CPageWndBase*)new T ;
			m_pPageTable[ wndId ] = pobj;
			pobj->Create(m_hWnd,wndId,0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);
		}
		else
		{
			pobj = m_pPageTable[ wndId ];
		}
		return pobj;
	}

	CPageWndBase* _CreatePage(int nView)
	{
		CPageWndBase* pNew = NULL;

		switch( nView )
		{
			case VOLUME_CONSOLE_VOLUMEFILELIST:
			{
				pNew = GetOrAllocWndObjct<CFileListPage>(nView);
				ASSERT(pNew != NULL);
				break;
			}
			case VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT:
			{
				pNew = GetOrAllocWndObjct<CFileSearchResultPage>(nView);
				ASSERT(pNew != NULL);
				break;
			}
			default:
			{
				ASSERT(FALSE);
				break;
			}
		}
		return pNew;
	}

	CPageWndBase* _SelectPage(SELECT_ITEM *pSel,BOOL& bCreate)
	{
		int nView = pSel->ViewType;

		CPageWndBase* pPage = m_pPageTable[ nView ];

		if( pPage == NULL )
		{
			pPage = _CreatePage(nView);

			pPage->OnInitPage( pSel, 0, 0 );

			bCreate = TRUE;
		}
		else
		{
			bCreate = FALSE;
		}

		if( m_pPage == pPage )
		{
			return NULL; // no change page
		}

		CPageWndBase* pPrev = m_pPage;

		m_pPage = pPage;
		m_nView = nView;

		return pPrev;
	}

	void ChangePage(CPageWndBase*pNew,CPageWndBase*pPrev)
	{
		EnableWindow(pNew->m_hWnd,TRUE);
		ShowWindow(pNew->m_hWnd,SW_SHOWNA);

		if( pPrev )
		{
			ShowWindow(pPrev->m_hWnd,SW_HIDE);
			EnableWindow(pPrev->m_hWnd,FALSE);
		}
	}

	void UpdateFileList(SELECT_ITEM *Path)
	{
		UpdateData(Path);
	}

	void UpdateRootDirectories(SELECT_ITEM *Path)
	{
		UpdateData(Path);
	}

	HRESULT SelectView(SELECT_ITEM *Path) 
	{
		ASSERT(Path  != NULL);

		CPageWndBase *pPrev=NULL;
		int nPrevView = -1;

		// todo: check range of Path->ViewType
		BOOL bCreate = ( m_pPageTable[ Path->ViewType ] == NULL ) ? TRUE : FALSE;

		switch( Path->ViewType )
		{
			case VOLUME_CONSOLE_VOLUMEFILELIST:
				nPrevView = m_nView;
				pPrev = _SelectPage( Path, bCreate );
				if( bCreate || ((Path->Flags & 0x8)== 0) )
				{
					UpdateFileList(Path);
				}
				break;
			case VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT:
				nPrevView = m_nView;
				pPrev = _SelectPage( Path, bCreate );
				if( bCreate || ((Path->Flags & 0x8)== 0) )
				{
					UpdateFileList(Path);
				}
				break;
			default:
				ASSERT(FALSE);
				break;
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),TRUE);

		ChangePage(m_pPage,pPrev);

		return S_OK;
	}
};
