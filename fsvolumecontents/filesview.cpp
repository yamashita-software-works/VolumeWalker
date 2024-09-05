//****************************************************************************
//
//  dirfilesview.cpp
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
#include "stdafx.h"
#include "resource.h"
#include "fsvolumecontents.h"
#include "filesview.h"

class CFileListBase : 
	public CBaseWindow,
	public IViewBaseWindow
{
	CPageWndBase *m_pPage;
	CPageWndBase *m_pPageTable[VOLUME_CONSOLE_MAX_ID];
public:
	CFileListBase()
	{
		m_hWnd = NULL;
		m_pPage = NULL;
		memset(m_pPageTable,0,sizeof(m_pPageTable));
	}

	virtual ~CFileListBase()
	{
	}

	VOID UpdateLayout(int cx,int cy,BOOL absSplitPos=FALSE)
	{
		HDWP hdwp = BeginDeferWindowPos(1);
		if( m_pPage )
			DeferWindowPos(hdwp,m_pPage->GetHwnd(),NULL,0,0,cx,cy,SWP_NOZORDER);
		EndDeferWindowPos(hdwp);
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM lParam)
	{
		SetWindowText(hWnd,L"CFileListBase");
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		return 0;
	}

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( lParam != 0 )
		{
			int cx = GET_X_LPARAM(lParam);
			int cy = GET_Y_LPARAM(lParam);

			UpdateLayout(cx,cy,TRUE);
		}
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( m_pPage )
			SetFocus(m_pPage->GetHwnd());
		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;
		switch( pnmhdr->code )
		{
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_PRETRANSLATEMESSAGE:
				if( m_pPage )
					return SendMessage(m_pPage->GetHwnd(),uMsg,wParam,lParam); // forward to current page
				return 0;
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case PM_GETCURDIR:
				if( wParam != 0 )
				{
					StringCchCopy((PWSTR)lParam,(SIZE_T)wParam,(reinterpret_cast<CFileListPage*>(m_pPage))->GetPath());
					return (LRESULT)(reinterpret_cast<CFileListPage*>(m_pPage))->IsNtfsSpecialDirectory();
				}
				else
				{
					*((LARGE_INTEGER *)lParam) = (reinterpret_cast<CFileListPage*>(m_pPage))->GetFileId();
				}
				return 0;
			case PM_FINDITEM:
				if( m_pPage )
					return SendMessage(m_pPage->GetHwnd(),uMsg,wParam,lParam); // forward to current view
				break;
			// Forward Parent Window
			case WM_CONTROL_MESSAGE:
				return SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,wParam,lParam);
			case WM_NOTIFY_MESSAGE:
				return SendMessage(GetParent(m_hWnd),WM_NOTIFY_MESSAGE,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	//
	// IViewBaseWindow
	//
public:
	virtual HWND GetHWND() const
	{
		return GetHwnd();
	}

	virtual HRESULT Create(HWND hWnd,HWND *phWndFileList=NULL)
	{
		HWND hwnd = CBaseWindow::Create(hWnd,0,0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);
		if( phWndFileList )
			*phWndFileList = hwnd;

		return S_OK;
	}

	virtual HRESULT Destroy()
	{
		return E_NOTIMPL;
	}

	virtual HRESULT InitData(PVOID,LPARAM)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT InitLayout(const RECT *prc)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT UpdateData(SELECT_ITEM *pFile)
	{
		ASSERT( m_pPage != NULL );

		return m_pPage->UpdateData(pFile);
	}

	virtual HRESULT GetState(int,ULONG *pul)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->QueryCmdState(CmdId,State);
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->InvokeCommand(CmdId);
	}

	virtual HRESULT PreTranslateMessage(MSG *pmsg)
	{
		return E_NOTIMPL;
	}

	template <class T>
	CPageWndBase *GetOrAllocWndObjct(int wndId)
	{
		CPageWndBase *pobj;
		if( m_pPageTable[ wndId ] == NULL )
		{
			pobj = (CPageWndBase*)new T ;
			m_pPageTable[ wndId ] = pobj;
			pobj->Create(m_hWnd,wndId,0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);
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
			case VOLUME_CONSOLE_ROOT:
			{
				pNew = GetOrAllocWndObjct<CRootView>(nView);
				ASSERT(pNew != NULL);
				break;
			}
			case VOLUME_CONSOLE_CONTENT_FILES:
			{
				pNew = GetOrAllocWndObjct<CFileListPage>(nView);
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

	CPageWndBase* _SelectPage(int nView)
	{
		CPageWndBase* pPage = m_pPageTable[ nView ];

		if( pPage == NULL )
		{
			pPage = _CreatePage(nView);

			pPage->OnInitPage(0);
		}

		if( m_pPage == pPage )
		{
			return NULL; // no change page
		}

		CPageWndBase* pPrev = m_pPage;

		m_pPage = pPage;

		return pPrev;
	}

	void _ChangePage(CPageWndBase*pNew,CPageWndBase*pPrev)
	{
		EnableWindow(pNew->m_hWnd,TRUE);
		ShowWindow(pNew->m_hWnd,SW_SHOWNA);

		if( pPrev )
		{
			ShowWindow(pPrev->m_hWnd,SW_HIDE);
			EnableWindow(pPrev->m_hWnd,FALSE);
		}
	}

	HRESULT SelectView(SELECT_ITEM *Path) 
	{
		SELECT_ITEM *pSel = (SELECT_ITEM *)Path;
		ASSERT(pSel != NULL);

		CPageWndBase *pPrev=NULL;

		switch( pSel->ViewType )
		{
			case VOLUME_CONSOLE_ROOT:
			case VOLUME_CONSOLE_CONTENT_FILES:
				pPrev = _SelectPage( pSel->ViewType );
				break;
			default:
				ASSERT(FALSE);
				break;
		}

		if( Path->pszPath || Path->Flags )
		{
			UpdateData(Path);
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),TRUE);

		_ChangePage(m_pPage,pPrev);

		return S_OK;
	}
};

//////////////////////////////////////////////////////////////////////////////

HRESULT CreateFileListBaseObject(HINSTANCE hInstance,IViewBaseWindow **pObject)
{
	CFileListBase *pWnd = new CFileListBase;

	CFileListBase::RegisterClass(hInstance);

	*pObject = static_cast<IViewBaseWindow *>(pWnd);

	return S_OK;
}
