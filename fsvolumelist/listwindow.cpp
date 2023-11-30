//****************************************************************************
//
//  listwindow.cpp
//
//  Implements the common list window base.
//
//  Auther: YAMASHITA Katsuhiro
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
	IViewBaseWindow *m_pView;

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
		m_hWnd = hWnd;

		ViewBase_CreateObject(GETINSTANCE(m_hWnd),&m_pView);
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
		UpdateLayout(cx,cy,TRUE);
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		m_hWndCtrlFocus = m_pView->GetHWND();

		SetFocus(m_pView->GetHWND());

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
			case CTRL_INIT_LAYOUT:
				InitLayout((RECT*)lParam);
				break;
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
			case WM_PRETRANSLATEMESSAGE:
				return 0;
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
			case WM_QUERY_CMDSTATE:
				return OnQueryCmdState(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy,BOOL absSplitPos=FALSE)
	{
		HDWP hdwp = BeginDeferWindowPos(1);

		DeferWindowPos(hdwp,m_pView->GetHWND(),NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE);

		EndDeferWindowPos(hdwp);
	}

	LRESULT OnNotifyMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case NOTIFY_VOLUME_SELECTED:
				OnUpdateInformationView( (SELECT_ITEM*)lParam );
				break;
		}
		return 0;
	}

	VOID OnUpdateInformationView(SELECT_ITEM* pVolume)
	{
		SELECT_ITEM sel = {0};
		sel.ViewType = -1;
		m_pView->UpdateData(&sel);
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
		m_hWndCtrlFocus = m_pView->GetHWND();
	}
};

//////////////////////////////////////////////////////////////////////////////

static HWND _CreateViewWindow(HWND hWndParent,UINT ConsoleId,PCWSTR pszTitle,HICON hIcon)
{
	CListWindow::RegisterClass(GETINSTANCE(hWndParent));

	CListWindow *pView = new CListWindow(ConsoleId);

	HWND hwnd = pView->Create(hWndParent,0,NULL,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);

	if( hIcon != NULL )
		SetFrameIcon(hwnd,hIcon);

	if( pszTitle != NULL )
		SetFrameTitle(hwnd,pszTitle);

	return hwnd;
}

HWND CreateVolumeListWindow(HWND hWndParent)
{
	HICON hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_VOLUMELIST,L"Volumes",hIcon);
}

HWND CreatePhysicalDriveListWindow(HWND hWndParent)
{
	HICON hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_PHYSICALDRIVELIST,L"Physical Drives",hIcon);
}

HWND CreateStorageDeviceWindow(HWND hWndParent)
{
	HICON hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_STORAGEDEVICE,L"Storage Devices",hIcon);
}

HWND CreateMountedDeviceWindow(HWND hWndParent)
{
	HICON hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_MOUNTEDDEVICE,L"Mounted Devices",hIcon);
}

HWND CreateShadowCopyListWindow(HWND hWndParent)
{
	HICON hIcon = GetDeviceClassIcon(0,&GUID_DEVCLASS_VOLUMESNAPSHOT);
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_SHADOWCOPYLIST,L"Shadow Copy Volumes",hIcon);
}

HWND CreateDosDriveWindow(HWND hWndParent)
{
	HICON hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
	return _CreateViewWindow(hWndParent,VOLUME_CONSOLE_DOSDRIVELIST,L"Dos Drives",hIcon);
}
