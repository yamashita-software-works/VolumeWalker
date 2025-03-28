//****************************************************************************
//
//  volumewindow.cpp
//
//  Implements the volume information window.
//
//  Author: YAMASHITA Katsuhiro
//
//  Create: 2023.03.31
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "resource.h"
#include "fsvolumelist.h"
#include "view.h"

class CVolumeInformationWindow : public CConsoleWindow
{
public:
	CViewBase m_PageHost;
	CViewBase *m_pView;

	CVolumeInformationWindow()
	{
		m_hWnd = NULL;
		m_pView = NULL;
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		m_pView = &m_PageHost;
		m_pView->m_hWnd = m_hWnd;
		return 0;
	}

	LRESULT OnDestroy(HWND,UINT,WPARAM,LPARAM)
	{
		return 0;
	}

	LRESULT OnSize(HWND hWnd,UINT,WPARAM,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy,TRUE);
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		SetFocus(m_pView->GetPageHWND());
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
		switch( LOWORD(wParam) )
		{
			case -1: // todo: dummy for switch~case
				break;
			default:
				m_pView->InvokeCommand(LOWORD(wParam));
				break;
		}
		return 0;
	}

	LRESULT OnNotifyMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_NOTIFY_VOLUME_SELECTED:
				OnUpdateInformationView( (SELECT_ITEM*)lParam );
				break;
		}
		return 0;
	}

	LRESULT OnControlMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_INIT_LAYOUT:
				InitLayout((RECT*)lParam);
				break;
		}
		return 0;
	}

	LRESULT OnQueryMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			// Forward to current view
			case UI_QUERY_CURRENTITEMNAME:
				if( m_pView )
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam);
				break;
		}
		return 0;
	}

	VOID OnUpdateInformationView(SELECT_ITEM* pSel)
	{
		if( SUCCEEDED(m_pView->SelectView(pSel)) )
		{
			m_pView->UpdateData(pSel);

			RECT rc;
			GetClientRect(m_hWnd,&rc);
			UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),FALSE);
			InitLayout(&rc);
		}
	}

	LRESULT OnQueryCmdState(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		ASSERT( lParam != NULL );
		if( lParam )
		{
			if( m_pView->QueryCmdState((UINT)LOWORD(wParam),(UINT*)lParam) == S_OK )
				return TRUE;
		}
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
	        case WM_COMMAND:
				return OnCommand(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		    case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
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
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam); // forward to current view
				return 0;
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	virtual VOID UpdateLayout(int cx,int cy,BOOL absSplitPos=FALSE)
	{
		HDWP hdwp = BeginDeferWindowPos(2);
		DeferWindowPos(hdwp,m_pView->GetPageHWND(),NULL,0,0,cx,cy,SWP_NOZORDER);
		EndDeferWindowPos(hdwp);
	}

	VOID InitData(PCWSTR)
	{
	}

	VOID InitLayout(const RECT *prcDesktopWorkArea)
	{
		m_pView->InitLayout(prcDesktopWorkArea);
	}
};

//////////////////////////////////////////////////////////////////////////////

HWND CreateVolumeInformationWindow(HWND hWndParent)
{
	CVolumeInformationWindow::RegisterClass(GETINSTANCE(hWndParent));

	CVolumeInformationWindow *pView = new CVolumeInformationWindow;

	return pView->Create(hWndParent,0,L"CVolumeInformationWindow",WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);
}
