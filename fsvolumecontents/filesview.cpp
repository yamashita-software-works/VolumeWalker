//****************************************************************************
//
//  filesview.cpp
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
#define _HEADER_BAR 1
#if _HEADER_BAR
#include "headerbar.h"
#endif

class CFileListBase : 
	public CBaseWindow,
	public IViewBaseWindow
{
	CPageWndBase *m_pPage;
	CPageWndBase *m_pPageTable[VOLUME_CONSOLE_MAX_ID];
	int m_nView;
#if _HEADER_BAR
	CHeaderBarWindow *m_pHeaderBar;
#endif
public:
	CFileListBase()
	{
		m_hWnd = NULL;
		m_pPage = NULL;
#if _HEADER_BAR
		m_pHeaderBar = NULL;
#endif
		memset(m_pPageTable,0,sizeof(m_pPageTable));
		m_nView = -1;
	}

	virtual ~CFileListBase()
	{
	}

	VOID UpdateLayout(int cx,int cy,BOOL absSplitPos=FALSE)
	{
		int cyHeaderBar = 0;
		HDWP hdwp = BeginDeferWindowPos(2);
#if _HEADER_BAR
		if( m_pHeaderBar && IsWindow(m_pHeaderBar->GetHwnd()) && (GetWindowLong(m_pHeaderBar->GetHwnd(),GWL_STYLE) & WS_VISIBLE) )
		{
			cyHeaderBar = m_pHeaderBar->GetHeight();

			DeferWindowPos(hdwp,m_pHeaderBar->GetHwnd(),NULL,0,0,cx,cyHeaderBar,SWP_NOZORDER);
		}
#endif
		if( m_pPage )
		{
			DeferWindowPos(hdwp,m_pPage->GetHwnd(),NULL,0,cyHeaderBar,cx,cy-cyHeaderBar,SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM lParam)
	{
		SetWindowText(hWnd,L"CFileListBase");
#if _HEADER_BAR
		m_pHeaderBar = new CHeaderBarWindow;
		m_pHeaderBar->Create(hWnd,0,L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

		COLORREF crHighlightBack = RGB(247,247,247);
		COLORREF crBack = GetSysColor(COLOR_3DFACE);

		HEADERBARCOLOR ibc;
		ibc.crHighlightText = RGB(0,0,0);
		ibc.crHighlightBack = crHighlightBack;
		ibc.crNormalBack    = crBack;
		ibc.crNormalText    = RGB(55,55,55);
		ibc.crBoxBack       = crHighlightBack;
		ibc.crBoxBorder     = crHighlightBack;
		ibc.crBoxInactiveBack = crBack;
		ibc.crBoxInactiveBorder = crBack;
		SendMessage(m_pHeaderBar->GetHwnd(),HBM_SETCOLOR,2,(LPARAM)&ibc);

		SendMessage(m_pHeaderBar->GetHwnd(),HBM_SETCOLOR,1,0);
#endif
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd,UINT,WPARAM,LPARAM)
	{
#if _HEADER_BAR
		DestroyWindow( m_pHeaderBar->GetHwnd() );
#endif
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

	LRESULT OnChangePath(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		/* lookup current directory */
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
			m_pPage->UpdateData( &sel );
			m_pHeaderBar->SetPath(sz);
			delete[] sz;
		}
		return 0;
	}

#if _HEADER_BAR
	LRESULT OnMakeContextMenu(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HMENU hMenu = (HMENU)lParam;
		UINT id = LOWORD(wParam);
		if( ID_MENU == id )
		{
			AppendMenu(hMenu,MF_STRING,ID_UP_DIR,  L"&Up One Directory\tAlt+UpArrow");
			AppendMenu(hMenu,MF_STRING,ID_GOTO,    L"&GoTo Directory\tCtrl+G");
		}
		return 0;
	}
#endif

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
#if _HEADER_BAR
			case WM_NCACTIVATE:
				SendMessage(m_pHeaderBar->GetHwnd(),HBM_SETCOLOR,wParam?1:0,0);
				break;
#endif
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
			case PM_GETSELECTEDFILE:
			case PM_GETWORKINGDIRECTORY:
				if( m_pPage )
					return SendMessage(m_pPage->GetHwnd(),uMsg,wParam,lParam); // forward to current view
				break;
			// Forward Parent Window
			case WM_CONTROL_MESSAGE:
				return SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,wParam,lParam);
			case WM_NOTIFY_MESSAGE:
				return SendMessage(GetParent(m_hWnd),WM_NOTIFY_MESSAGE,wParam,lParam);
			case WM_QUERY_MESSAGE:
				if( m_pPage )
					return SendMessage(m_pPage->GetHwnd(),uMsg,wParam,lParam); // forward to current view
				break;
#if _HEADER_BAR
			case PM_CHANGEPATH:
				return OnChangePath(hWnd,uMsg,wParam,lParam);
			case PM_MAKECONTEXTMENU:
				return OnMakeContextMenu(hWnd,uMsg,wParam,lParam);
#endif
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
			case VOLUME_CONSOLE_FILES:
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

	CPageWndBase* _SelectPage(SELECT_ITEM *pSel)
	{
		int nView = pSel->ViewType;

		CPageWndBase* pPage = m_pPageTable[ nView ];

		if( pPage == NULL )
		{
			pPage = _CreatePage(nView);

			pPage->OnInitPage( pSel );
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

	void UpdateFileList(SELECT_ITEM *Path)
	{
		HRESULT hr;
		BOOL bValidPath = FALSE;

		if( Path->Flags & _FLG_NTFS_SPECIALFILE )
		{
			bValidPath = TRUE;
		}
		else if( Path->pszPath && *Path->pszPath && PathFileExists_W(Path->pszPath,NULL) )
		{
			bValidPath = TRUE;
		}

		if( Path->pszPath ) // bValidPath )
		{
			if( (hr = UpdateData(Path)) == S_OK )
			{
#if _HEADER_BAR
				m_pHeaderBar->EnableButton(ID_UP_DIR, !IsRootDirectory_W(Path->pszPath) );

				m_pHeaderBar->SetPath( Path->pszPath );
#endif
			}
			else
			{
#if _HEADER_BAR
				m_pHeaderBar->EnableButton(ID_UP_DIR, !IsRootDirectory_W(Path->pszPath) );

				m_pHeaderBar->SetPath( Path->pszPath );
#endif
			}
		}
		else
		{
			m_pHeaderBar->EnableButton(ID_UP_DIR, FALSE);
		}
	}

	void UpdateRootView(SELECT_ITEM *Path)
	{
		UpdateData(Path);
		SetWindowText(m_pHeaderBar->GetHwnd(),Path->pszPath);
		m_pHeaderBar->EnableButton(ID_UP_DIR, FALSE);
		m_pHeaderBar->EnableButton(ID_GOTO, FALSE);
	}

	HRESULT SelectView(SELECT_ITEM *Path) 
	{
		SELECT_ITEM *pSel = (SELECT_ITEM *)Path;
		ASSERT(pSel != NULL);

		CPageWndBase *pPrev=NULL;
		int nPrevView = -1;

		switch( pSel->ViewType )
		{
			case VOLUME_CONSOLE_ROOT:
				pPrev = _SelectPage( pSel );
				UpdateRootView(Path);
				break;
			case VOLUME_CONSOLE_FILES:
				nPrevView = m_nView;
				pPrev = _SelectPage( pSel );
				if( nPrevView != -1 &&  pSel->ViewType != nPrevView )
				{
					PWSTR pszWorkPath = (PWSTR)SendMessage(m_pPage->GetHwnd(),PM_GETWORKINGDIRECTORY,0,0);
					if(pszWorkPath)
					{
						m_pHeaderBar->EnableButton(ID_UP_DIR, !IsRootDirectory_W(pszWorkPath) );
						m_pHeaderBar->SetPath(pszWorkPath);
					}
				}
				else
				{
					UpdateFileList(Path);
				}
				break;
			default:
				ASSERT(FALSE);
				break;
		}

		SetFocus(m_pPage->GetHwnd());	

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
