//****************************************************************************
//
//  fileswindow.cpp
//
//  Implements the directory files view host window.
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
#include "fileswindow.h"
#include "filesview.h"
#include "simplesplitwindow.h"

#define _ENABLE_SUBPANE  0

class CDirectoryFilesWindow : public CSimpleSplitWindow
{
	IViewBaseWindow *m_pView;
	HWND m_hwndSubView;
	HWND m_hWndCtrlFocus;
public:
	CDirectoryFilesWindow()
	{
		m_hWnd = NULL;
		m_pView = NULL;
		m_hwndSubView = NULL;
		m_hWndCtrlFocus = NULL;
		CSimpleSplitWindow::m_xSplitPos = 0;
		CSimpleSplitWindow::m_ratio = 0.5;
	}

	~CDirectoryFilesWindow()
	{
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		CreateFileListBaseObject(GETINSTANCE(m_hWnd),&m_pView);
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

	LRESULT OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		PAINTSTRUCT ps;
		HDC hdc;

		hdc = BeginPaint(hWnd,&ps);

		RECT rc;
		GetClientRect(hWnd,&rc);
		rc.left = m_xSplitPos - 2;
		rc.right = m_xSplitPos + 2;

		HBRUSH hbr = CreateSolidBrush( RGB(255,0,0) );
		FillRect(hdc,&rc,hbr);
		DeleteObject(hbr);

		EndPaint(hWnd,&ps);

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
				case ID_ROOT:
				{
					SELECT_ITEM sel = {0};
					sel.ViewType  = VOLUME_CONSOLE_ROOT;
					sel.pszCurDir = NULL;
					sel.pszPath   = L"Root Directory Viewer";
					sel.pszName   = NULL;
					m_pView->SelectView(&sel);
					break;
				}
				case ID_CONTENTSBROWSER:
				{
					SELECT_ITEM sel = {0};
					sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
					sel.pszCurDir = NULL;
					sel.pszPath   = NULL;
					sel.pszName   = NULL;
					m_pView->SelectView(&sel);
					break;
				}
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

				switch( uCmdId )
				{
					case ID_CONTENTSBROWSER:
					case ID_ROOT:
						*puState = UPDUI_ENABLED;
						return TRUE;
				}
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
		}
		return 0;
	}

	LRESULT OnInitView(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SELECT_ITEM sel = {0};
		sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
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

		if( pszPath && PathFileExists_W(pszPath,NULL) )
		{
			SELECT_ITEM sel = {0};
			sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
			sel.pszCurDir = (PWSTR)pszPath;
			sel.pszPath   = (PWSTR)pszPath;
			sel.pszName   = (PWSTR)NULL;
			m_pView->SelectView(&sel);
		}

		FreeMemory(pszPath);

		return 0;
	}

	LRESULT OnSelectFile(HWND /*hWnd*/,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		PCWSTR pszDirectoryPath = (PCWSTR)lParam;
		PWSTR pszPath;
		if( IsNtDevicePath(pszDirectoryPath) )
			pszPath = DuplicateString(pszDirectoryPath);
		else
			pszPath = DosPathNameToNtPathName(pszDirectoryPath);

		if( PathFileExists_W(pszPath,NULL) )
		{
			InitData(pszPath);

			UNICODE_STRING FileName;
			UNICODE_STRING Path;
			RtlInitUnicodeString(&Path,pszPath);
			SplitPathFileName_U(&Path,&FileName);

			PWSTR pszPath = AllocateSzFromUnicodeString(&Path);
			PWSTR pszFileName = AllocateSzFromUnicodeString(&FileName);

			SELECT_ITEM sel = {0};
			sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
			sel.pszCurDir = (PWSTR)NULL;
			sel.pszPath   = (PWSTR)pszPath;
			sel.pszName   = (PWSTR)pszFileName;
			m_pView->SelectView(&sel);

			FreeMemory(pszPath);
			FreeMemory(pszFileName);
		}

		FreeMemory(pszPath);

		return 0;
	}

#if _ENABLE_SUBPANE
	LRESULT OnSetSubPane(HWND /*hWnd*/,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		m_hwndSubView = (HWND)lParam;

		//
		// reselect item when open sub-view
		//
		FS_SELECTED_FILE path = {0};
		if( SendMessage(m_pView->GetHWND(),PM_GETSELECTEDFILE,0,(LPARAM)&path) )
		{
			SELECT_ITEM sel = {0};
			sel.pszName   = (PWSTR)FindFileName_W(path.pszPath);
			if( wcscmp(sel.pszName,L"..") != 0 )
			{
				sel.pszPath   = path.pszPath;
				sel.pszCurDir = DuplicateString(path.pszPath);
				RemoveFileSpec_W(sel.pszCurDir);

				sel.ViewType  = VIEW_PAGE_FILELIST;
				OnItemSelected( &sel );

				FreeMemory(sel.pszCurDir);
			}
			LocalFree(path.pszPath);
		}

		UpdateLayout();

		return 0;
	}

	LRESULT OnGetSubPane(HWND /*hWnd*/,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		return (LRESULT)m_hwndSubView;
	}
#endif

	LRESULT OnQueryMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case 0:
				break; // todo: avoid C4065
			default:
				if( m_pView )
					SendMessage(m_pView->GetHWND(),uMsg,wParam,lParam);
				break;
		}
		return 0;
	}

	LRESULT OnControlMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_CHANGE_DIRECTORY:
				return OnChangeDirectory( (SELECT_ITEM*)lParam );
			case UI_SET_DIRECTORY:
				return OnSetDirectory(hWnd,0,0,lParam);
			case UI_SELECT_FILE:
				return OnSelectFile(hWnd,0,0,lParam);
			case UI_INIT_VIEW:
				return OnInitView(hWnd,0,0,lParam);
			case UI_SET_TITLE:
				SetWindowText(GetParent(hWnd),(LPCWSTR)lParam);
				break;
			case UI_SET_ICON:
				DestroyIcon((HICON)SendMessage(GetParent(m_hWnd),WM_GETICON,ICON_SMALL,0));
				SendMessage(GetParent(hWnd),WM_SETICON,(WPARAM)ICON_SMALL,lParam);
				break;
#if _ENABLE_SUBPANE
			case UI_SET_SUBPANE:
				return OnSetSubPane(hWnd,0,0,lParam);
			case UI_GET_SUBPANE:
				return OnGetSubPane(hWnd,0,0,lParam);
#endif
		}
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_PRETRANSLATEMESSAGE:
			{
				MSG *pmsg = (MSG *)lParam;
				if( pmsg->message == WM_KEYDOWN && pmsg->wParam == VK_TAB )
				{
					// Tab order
					if( IsDialogMessage(hWnd,pmsg) )
					{
						return TRUE;
					}
				}
				if( m_pView )
					return SendMessage(m_pView->GetHWND(),uMsg,wParam,lParam); // forward to current view
				return 0;
			}
			case WM_PAINT:
				return OnPaint(hWnd,uMsg,wParam,lParam);
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
			case WM_NCACTIVATE:
				SendMessage(m_pView->GetHWND(),WM_NCACTIVATE,wParam,lParam);
				return 0;
			case PM_FINDITEM:
				if( m_pView )
					return SendMessage(m_pView->GetHWND(),uMsg,wParam,lParam); // forward to current view
				return 0;
		}
		return CSimpleSplitWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx=-1,int cy=-1,BOOL absSplitPos=FALSE)
	{
		if( cx == -1 && cy == -1 )
		{
			RECT rc;
			GetClientRect(m_hWnd,&rc);
			cx = _RECT_WIDTH(rc);
			cy = _RECT_HIGHT(rc);
		}

		BOOL bSubPane = IsWindow(m_hwndSubView);

		int cyInfoBar = 0;
		int xViewL;
		int xViewR;
		int cxViewL;
		int cxViewR;

		if( bSubPane )
		{
			if( absSplitPos )
				cxViewL = m_xSplitPos;
			else
				cxViewL = m_xSplitPos = (int)((double)cx * m_ratio);

			xViewL  = 0;
			cxViewL = cxViewL - 1;
			xViewR  = cxViewL + 1;
			cxViewR = (cx - cxViewL - 1);
		}
		else
		{
			xViewL  = 0;
			cxViewL = cx;
			xViewR  = 0;
			cxViewR = 0;
		}

		int cyHeaderPane = 0;

		HDWP hdwp = BeginDeferWindowPos(3);

		cyInfoBar += cyHeaderPane;

		if( m_pView )
			DeferWindowPos(hdwp,m_pView->GetHWND(),NULL,
				xViewL, cyInfoBar, cxViewL, cy-cyInfoBar,SWP_NOZORDER|SWP_NOREDRAW);

		if( bSubPane )
			DeferWindowPos(hdwp,m_hwndSubView,NULL,
				xViewR, cyInfoBar, cxViewR, cy-cyInfoBar,SWP_NOZORDER|SWP_NOREDRAW);

		EndDeferWindowPos(hdwp);

		InvalidateRect(m_hWnd,NULL,TRUE);
		UpdateWindow(m_hWnd);
	}

	VOID InitData(PCWSTR pszDirectoryPath)
	{
		m_pView->InitData(NULL,0);

		SELECT_ITEM sel = {0};
		sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
		sel.pszCurDir = (PWSTR)pszDirectoryPath;
		sel.pszPath   = (PWSTR)pszDirectoryPath;
		sel.pszName   = (PWSTR)NULL;
		m_pView->SelectView(&sel);
	}

	VOID InitLayout(const RECT *prcDesktopWorkArea)
	{
		m_pView->InitLayout(NULL);
	}

	VOID OnUpdateInformationView(SELECT_ITEM* pFile)
	{
		m_pView->SelectView(pFile);
	}

	VOID OnItemSelected(SELECT_ITEM* pFile)
	{
		if( wcscmp(pFile->pszName,L"..") == 0 )
		{
			PWSTR pszPath = _MemAllocString(pFile->pszCurDir);
			PWSTR pszName = _MemAllocString(PathFindFileName(pszPath));

			RemoveFileSpec(pszPath);

			SELECT_ITEM sel = {0};
			sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
			sel.pszCurDir = pszPath;
			sel.pszPath   = pszPath;
			sel.pszName   = pszName;
			m_pView->SelectView(&sel);

			_SafeMemFree(pszPath);
			_SafeMemFree(pszName);
		}
		else
		{
			HWND hwndMainWnd = GetActiveWindow(); // todo:
			SendMessage(hwndMainWnd,WM_NOTIFY_MESSAGE,UI_NOTIFY_ITEM_SELECTED,(LPARAM)pFile);
		}
	}

	LRESULT OnChangeDirectory(SELECT_ITEM* pFile)
	{
		if( (pFile->mask & SI_MASK_FILEID) && (pFile->Flags &  _FLG_NTFS_SPECIALFILE) )
		{
			m_pView->SelectView(pFile);
			return 0;
		}

		if( pFile->pszName && wcscmp(pFile->pszName,L"..") == 0 )
		{
			PWSTR pszPath = _MemAllocString(pFile->pszCurDir);
			PWSTR pszName = _MemAllocString( FindFileName_W(pszPath) );
			if( pszPath && pszName && *pszPath != L'\0' && *pszName != L'\0' )
			{
				RemoveFileSpec_W(pszPath);

				SELECT_ITEM sel = {0};
				sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
				sel.pszCurDir = pszPath;
				sel.pszPath   = pszPath;
				sel.pszName   = pszName;
				m_pView->SelectView(&sel);

				_SafeMemFree(pszPath);
				_SafeMemFree(pszName);
			}
		}
		else
		{
			PWSTR pszPath;
			ULONG PathType = GetPathType(pFile->pszPath);

			if( PATHTYPE_NT_DEVICE != PathType )
			{
				pszPath = DosPathNameToNtPathName_W(pFile->pszPath);
			}
			else
			{
				pszPath = DuplicateString(pFile->pszPath);
			}

			SELECT_ITEM sel = *pFile; // trick! for Specified DOS Path

			sel.pszPath = pszPath;
			m_pView->SelectView( &sel );

			FreeMemory(pszPath);
		}
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////////////

HWND CreateDirectoryFilesWindow(HWND hWndParent)
{
	CDirectoryFilesWindow::RegisterClass(GETINSTANCE(hWndParent));

	CDirectoryFilesWindow *pView = new CDirectoryFilesWindow;

	HWND hwndView = pView->Create(hWndParent,0,L"DirectoryFiles",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);

	return hwndView;
}
