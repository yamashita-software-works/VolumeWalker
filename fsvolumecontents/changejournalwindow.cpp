//****************************************************************************
//
//  changejournalwindow.cpp
//
//  Implements the change journal view host window.
//
//  Auther: YAMASHITA Katsuhiro
//
//  Create: 2024-04-15
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "resource.h"
#include "fsvolumecontents.h"
#include "changejournalwindow.h"
#include "changejournalview.h"

class CChangeJournalWindow : public CBaseWindow
{
public:
	IViewBaseWindow *m_pView;
	HWND  m_hWndCtrlFocus;

	CChangeJournalWindow()
	{
		m_hWnd = NULL;
		m_pView = NULL;
		m_hWndCtrlFocus = NULL;
	}

	~CChangeJournalWindow()
	{
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		CreateChangeJournalObject(GETINSTANCE(m_hWnd),&m_pView);
		m_pView->Create(hWnd);
		return 0;
	}

	LRESULT OnDestroy(HWND,UINT,WPARAM,LPARAM)
	{
		DestroyIcon((HICON)SendMessage(GetParent(m_hWnd),WM_GETICON,ICON_SMALL,0));
		m_pView->Destroy();
		return 0;
	}

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( m_hWndCtrlFocus == NULL )
			m_hWndCtrlFocus = m_pView->GetHWND();

		SetFocus(m_hWndCtrlFocus);

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
		m_hWndCtrlFocus = pnmhdr->hwndFrom;
		return 0;
	}

	LRESULT OnCommand(HWND,UINT,WPARAM wParam,LPARAM)
	{
		if( m_hWndCtrlFocus == m_pView->GetHWND() )
		{
			switch( LOWORD(wParam) )
			{
				case 0: // avoid warning C4065
					break;
				default:
				{
					if( m_pView )
						m_pView->InvokeCommand(LOWORD(wParam));
					break;
				}
			}
		}
		return 0;
	}

	LRESULT OnQueryCmdState(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		if( m_hWndCtrlFocus == m_pView->GetHWND() )
		{
			ASSERT( lParam != NULL );
			if( lParam )
			{
				UINT *puState = (UINT *)lParam;
				UINT uCmdId = (UINT)LOWORD(wParam);
				if( m_pView->QueryCmdState((UINT)LOWORD(wParam),(UINT*)lParam) == S_OK )
					return TRUE;
			}
		}
		return 0;
	}

	LRESULT OnNotifyMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_NOTIFY_ITEM_SELECTED:
				OnItemSelected( (SELECT_ITEM*)lParam );
				break;
			case UI_NOTIFY_CHANGE_TITLE:
				SetWindowText(GetParent(hWnd),(LPCWSTR)lParam);
				break;
		}
		return 0;
	}

	LRESULT OnControlMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_SET_DIRECTORY:
				return OnSetDirectory(m_hWnd,0,0,lParam);
			case UI_INIT_VIEW:
				return OnInitView(m_hWnd,0,0,lParam);
		}
		return 0;
	}

	LRESULT OnQueryMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
		    case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_CMDSTATE:
				return OnQueryCmdState(hWnd,uMsg,wParam,lParam);
			case WM_CONTROL_MESSAGE:
				return OnControlMessage(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY_MESSAGE:
				return OnNotifyMessage(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_MESSAGE:
				return OnQueryMessage(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
				if( m_pView )
					return SendMessage(m_pView->GetHWND(),uMsg,wParam,lParam); // forward to current view
				return 0;
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy)
	{
		HDWP hdwp = BeginDeferWindowPos(1);

		if( m_pView )
			DeferWindowPos(hdwp,m_pView->GetHWND(),NULL,0,0,cx,cy,SWP_NOZORDER);

		EndDeferWindowPos(hdwp);
	}

	VOID OnUpdateInformationView(SELECT_ITEM* pFile)
	{
		m_pView->SelectView(pFile);
	}

	VOID OnItemSelected(SELECT_ITEM* pFile)
	{
	}

	LRESULT OnInitView(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// First page setting.
		SELECT_ITEM sel = {0};
		sel.ViewType  = VOUUME_CONSOLE_CHANGE_JOURNAL;
		sel.pszCurDir = (PWSTR)0;
		sel.pszPath   = (PWSTR)0;
		sel.pszName   = (PWSTR)NULL;
		m_pView->SelectView(&sel);
		return 0;
	}

	LRESULT OnSetDirectory(HWND /*hWnd*/,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		PCWSTR pszDirectoryPath = (PCWSTR)lParam;

		PWSTR pszPath;
		if( IsNtDevicePath(pszDirectoryPath) )
			pszPath = DuplicateString(pszDirectoryPath);
		else
			pszPath = DosPathNameToNtPathName(pszDirectoryPath);

		if( PathFileExists_W(pszPath,NULL) )
		{
			SELECT_ITEM sel = {0};
			sel.ViewType  = VOUUME_CONSOLE_CHANGE_JOURNAL;
			sel.pszCurDir = (PWSTR)pszPath;
			sel.pszPath   = (PWSTR)pszPath;
			sel.pszName   = (PWSTR)NULL;
			m_pView->SelectView(&sel); // call enum in this call
		}

		FreeMemory(pszPath);

		return 0;
	}

	VOID InitData(PCWSTR pszDirectoryPath)
	{
		m_pView->InitData(NULL,0);

		SELECT_ITEM sel = {0};
		sel.ViewType  = VOUUME_CONSOLE_CHANGE_JOURNAL;
		sel.pszCurDir = (PWSTR)pszDirectoryPath;
		sel.pszPath   = (PWSTR)pszDirectoryPath;
		sel.pszName   = (PWSTR)NULL;
		m_pView->SelectView(&sel);
	}

	VOID InitLayout(const RECT *prcDesktopWorkArea)
	{
		m_pView->InitLayout(NULL);
	}
};

//////////////////////////////////////////////////////////////////////////////

HWND CreateChangeJournalWindow(HWND hWndParent)
{
	CChangeJournalWindow::RegisterClass(GETINSTANCE(hWndParent));

	CChangeJournalWindow *pView = new CChangeJournalWindow;

	HWND hwndView = pView->Create(hWndParent,0,L"ChangeJournalWindow",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);

	return hwndView;
}
