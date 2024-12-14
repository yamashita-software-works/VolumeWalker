//****************************************************************************
//
//  listwindow.cpp
//
//  Implements the common list window base.
//
//  Author: YAMASHITA Katsuhiro
//
//  Create: 2023.10.27
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

class CListWindow : public CConsoleWindow
{
public:
	CViewBase m_PageHost;
	CViewBase *m_pView;

	int  m_ConsoleTypeId;

	CListWindow(UINT ConsoleTypeId)
	{
		m_hWnd = NULL;
		m_pView = NULL;
		m_hWndCtrlFocus = NULL;
		m_ConsoleTypeId = ConsoleTypeId;
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

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy,TRUE);
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		m_hWndCtrlFocus = m_pView->GetPageHWND();

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
		return 0;
	}

	LRESULT OnCommand(HWND,UINT,WPARAM wParam,LPARAM)
	{
		switch( LOWORD(wParam) )
		{
			case 0:
				break; // todo: avoid C4065
			default:
				if( m_pView )
					m_pView->InvokeCommand(LOWORD(wParam));
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
			case 0:
				break; // todo: avoid C4065
			default:
				if( m_pView )
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam);
				break;
		}
		return 0;
	}

	LRESULT OnQueryCmdState(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		if( m_hWndCtrlFocus == m_pView->GetPageHWND() )
		{
			ASSERT( lParam != NULL );
			if( lParam )
			{
				if( m_pView->QueryCmdState((UINT)LOWORD(wParam),(UINT*)lParam) == S_OK )
					return TRUE;
			}
		}
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
//			case WM_PRETRANSLATEMESSAGE:
//				if( m_pView )
//					m_pView->PreTranslateMessage((MSG*)lParam);
//				return 0;
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
			case WM_NOTIFY_MESSAGE:
				return OnNotifyMessage(hWnd,uMsg,wParam,lParam);
			case WM_CONTROL_MESSAGE:
				return OnControlMessage(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_MESSAGE:
				return OnQueryMessage(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_CMDSTATE:
				return OnQueryCmdState(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
				if( m_pView )
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam); // forward to current view
				return 0;
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy,BOOL absSplitPos=FALSE)
	{
		HDWP hdwp = BeginDeferWindowPos(1);

		DeferWindowPos(hdwp,m_pView->GetPageHWND(),NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE);

		EndDeferWindowPos(hdwp);
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

	VOID OnUpdateInformationView(SELECT_ITEM* pVolume)
	{
		m_pView->UpdateData(pVolume);
	}

	VOID InitData(PCWSTR pszParam)
	{
		SELECT_ITEM sel = {0};
		sel.ViewType = m_ConsoleTypeId;
		sel.pszPath = (PWSTR)pszParam;
		m_pView->SelectView(&sel);
	}

	VOID InitLayout(const RECT *prcDesktopWorkArea)
	{
		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),FALSE);
		m_pView->InitLayout(NULL);
		m_hWndCtrlFocus = m_pView->GetPageHWND();
	}
};

//////////////////////////////////////////////////////////////////////////////

static HWND _CreateViewWindow(HWND hWndParent,UINT ConsoleId)
{
	CListWindow::RegisterClass(GETINSTANCE(hWndParent));

	CListWindow *pView = new CListWindow(ConsoleId);

	HWND hwnd = pView->Create(hWndParent,0,NULL,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);

	return hwnd;
}

HWND CreateVolumeListWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_VOLUMELIST);
}

HWND CreatePhysicalDriveListWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_PHYSICALDRIVELIST);
}

HWND CreateStorageDeviceWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_STORAGEDEVICE);
}

HWND CreateMountedDeviceWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_MOUNTEDDEVICE);
}

HWND CreateShadowCopyListWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_SHADOWCOPYLIST);
}

HWND CreateDosDriveWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_DOSDRIVELIST);
}

HWND CreateFileSystemStatisticsWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_FILESYSTEMSTATISTICS);
}

HWND CreateSimpleHexDumpWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_SIMPLEHEXDUMP);
}

HWND CreateFilterDriverWindow(HWND hWndParent)
{
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_FILTERDRIVER);
}
