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
#include "basewindow.h"
#include "volumeconsoledef.h"
#include "filelistview.h"
#include "page_volumefilelist.h"
#include "page_volumefilelist_search_result.h"
#if _ENABLE_FILELIST_SUB_PANE
#include "panebase_volumes.h"
#include "panebase_property.h"
#include "panebase_information.h"
#include "simplesplitwindow.h"
#endif

inline int _GetTextWidth(HWND hWnd,HFONT hFont,PCWSTR psz)
{
	SIZE size;
	HDC hdc = GetDC(hWnd);
	HGDIOBJ hfontOld = SelectObject(hdc,hFont);
	GetTextExtentPoint32(hdc,psz,lstrlen(psz),&size);
	SelectObject(hdc,hfontOld);
	ReleaseDC(hWnd,hdc);
	return size.cx;
}

#if _ENABLE_FILELIST_SUB_PANE
class CVolumeFilesWindow : 
	public CBaseWindow,
	public CSimpleVSplitWindowEx<CVolumeFilesWindow>,
	public CSimpleHSplitWindowEx<CVolumeFilesWindow>
#else
class CVolumeFilesWindow : public CBaseWindow
#endif
{
	HWND m_hWndCtrlFocus;
	UINT m_ConsoleTypeId;

	CFilesPageHost m_PageHost;
	CFilesPageHost *m_pView;

#if _ENABLE_FILELIST_SUB_PANE
	CPropPaneWindow *m_pPropPane;
	CVolumeSelectPane *m_pVolPane;
	CInfoPaneWindow *m_pInfoPane;
#endif

public:
	DWORD m_dwViewStyle;

public:
	CVolumeFilesWindow(UINT ConsoleTypeId)
	{
		m_pView = NULL;
		m_hWndCtrlFocus = NULL;
		m_ConsoleTypeId = ConsoleTypeId;
		m_dwViewStyle = 0;
#if _ENABLE_FILELIST_SUB_PANE
		m_pPropPane = NULL;
		m_pVolPane = NULL;
		m_pInfoPane = NULL;
		m_xVolumeSplitter = 0;
		m_xPropSplitter = -1;
		m_cxPropPane = 0;
		m_yInfoSplitter = -1;
		m_cyInfoPane = -1;
#endif
	}

	~CVolumeFilesWindow()
	{
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		m_pView = &m_PageHost;

		m_pView->m_hWndHost = m_hWnd;
		m_pView->m_dwFlags = m_dwViewStyle;

#if _ENABLE_FILELIST_SUB_PANE
		CreateVolummePane(FALSE);
		CreatePropPane(FALSE);
		CreateInfoPane(FALSE);
		m_xVolumeSplitter = _DPI_Adjust_X(320);
		m_cxPropPane      = _DPI_Adjust_X(320);
		m_cyInfoPane      = -1;
#endif

		return 0;
	}

	LRESULT OnDestroy(HWND,UINT,WPARAM,LPARAM)
	{
		DestroyIcon((HICON)SendMessage(GetParent(m_hWnd),WM_GETICON,ICON_SMALL,0));
		return 0;
	}

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnEraseBkGnd(HWND hWnd,UINT,WPARAM wParam,LPARAM lParam)
	{
#if _ENABLE_FILELIST_SUB_PANE
		HDC hdcMem;
		HBITMAP hBmp,hOrgBmp;
		RECT rc;

		GetClientRect(hWnd,&rc);
		int cx = _RECT_WIDTH(rc);
		int cy = _RECT_HIGHT(rc);

		HDC hdc = (HDC)wParam;

		hdcMem = CreateCompatibleDC(hdc);
		hBmp = CreateCompatibleBitmap(hdc,cx,cy);

		hOrgBmp = (HBITMAP)SelectObject(hdcMem,hBmp);

		HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
		FillRect(hdcMem,&rc,hbr);
		DeleteObject(hbr);

		HBRUSH hbrSplitter = CreateSolidBrush(RGB(222,222,234));

		rc.left = m_xVolumeSplitter - 1;  // Left Align Pane
		rc.right = rc.left + 1;
		FillRect(hdc,&rc,hbrSplitter);

		rc.left = m_xPropSplitter;        // Right Align Pane
		rc.right = rc.left + 1;
		FillRect(hdc,&rc,hbrSplitter);

		rc.top    = m_yInfoSplitter - 1;  // Bottom Align Pane
		rc.bottom = rc.top + 2;
		rc.left   = 0;
		rc.right  = cx;
		FillRect(hdc,&rc,hbrSplitter);

		DeleteObject(hbrSplitter);

		BitBlt(hdc,
			0,0,
			_RECT_WIDTH(rc),
			_RECT_HIGHT(rc),
			hdcMem,0,0,SRCCOPY);

		SelectObject(hdcMem,hOrgBmp);

		DeleteObject(hBmp);
		DeleteDC(hdcMem);
#endif
		return (LRESULT)TRUE;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( m_hWndCtrlFocus == NULL )
			m_hWndCtrlFocus = m_pView->GetPageHWND();

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

		// Message forward to parent window
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom   = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);

		return 0;
	}

	LRESULT OnCommand(HWND,UINT,WPARAM wParam,LPARAM)
	{
		UINT uCmdId = (UINT)LOWORD(wParam);

#if _ENABLE_FILELIST_SUB_PANE
		switch( uCmdId )
		{
			case ID_VIEW_SELECTVOLUMEPANE:
				OnViewVolumeSelectorPane();
				return 0;
			case ID_VIEW_PROPERTYPANE:
				OnViewPropertyPane();
				return 0;
			case ID_VIEW_INFORMATIONPANE:
				OnViewInformationPane();
				return 0;
		}
#endif

		if( m_hWndCtrlFocus == m_pView->GetPageHWND() )
		{
			if( m_pView )
				m_pView->InvokeCommand(uCmdId);
		}
		return 0;
	}

	LRESULT OnQueryCmdState(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		UINT *puState = (UINT *)lParam;
		UINT uCmdId = (UINT)LOWORD(wParam);

#if _ENABLE_FILELIST_SUB_PANE
		switch( uCmdId )
		{
			case ID_VIEW_SELECTVOLUMEPANE:
				*puState = UPDUI_ENABLED | (IsWindowVisibleEx(m_pVolPane->m_hWnd) ? UPDUI_CHECKED : 0);
				return TRUE;
			case ID_VIEW_PROPERTYPANE:
				*puState = UPDUI_ENABLED | (IsWindowVisibleEx(m_pPropPane->m_hWnd) ? UPDUI_CHECKED : 0);
				return TRUE;
			case ID_VIEW_INFORMATIONPANE:
				*puState = UPDUI_ENABLED | (IsWindowVisibleEx(m_pInfoPane->m_hWnd) ? UPDUI_CHECKED : 0);
				return TRUE;
		}
#endif
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

	LRESULT OnNotifyMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_NOTIFY_ITEM_SELECTED:    // SELECT_ITEM*
			case UI_NOTIFY_VOLUME_SELECTED:  // SELECT_ITEM*
#if _ENABLE_FILELIST_SUB_PANE
			case UI_NOTIFY_UPDATE_SUBPANE:   // SELECT_ITEM*
#endif
				OnNotifyForwardToMainFrame(LOWORD(wParam),(PVOID)lParam);
				break;
			case UI_NOTIFY_ITEM_ACTIVATED:   // UIS_ITEM_ACTIVATED*
				OnNotifyForwardToMainFrame(LOWORD(wParam),(PVOID)lParam);
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
				return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam);
		}
		return 0;
	}

	LRESULT OnControlMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_CHANGE_DIRECTORY:
				return OnChangeDirectory( (SELECT_ITEM*)lParam );
			case UI_SELECT_PAGE:
				return OnSelectPage(hWnd,0,0,lParam);
			case UI_SET_TITLE:
				SetWindowText(GetParent(hWnd),(LPCWSTR)lParam);
				break;
			case UI_SET_ICON:
				DestroyIcon((HICON)SendMessage(GetParent(m_hWnd),WM_GETICON,ICON_SMALL,0));
				SendMessage(GetParent(hWnd),WM_SETICON,(WPARAM)ICON_SMALL,lParam);
				break;
			case UI_INIT_LAYOUT:
				InitLayout(NULL);
				break;
		}
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return OnEraseBkGnd(hWnd,uMsg,wParam,lParam);
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
			case WM_NCACTIVATE:
				SendMessage(m_pView->GetPageHWND(),WM_NCACTIVATE,wParam,lParam);
				return 0;
			case WM_QUERY_CMDSTATE:
				return OnQueryCmdState(hWnd,uMsg,wParam,lParam);
			case WM_CONTROL_MESSAGE:
				return OnControlMessage(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY_MESSAGE:
				return OnNotifyMessage(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_MESSAGE:
				return OnQueryMessage(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
			case PM_GETSELECTEDFILE:
			case PM_GETWORKINGDIRECTORY:
			case PM_SAVECONFIG:
			case WM_PRETRANSLATEMESSAGE:
				if( m_pView )
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam); // forward to current page
				return 0;
			case PPM_NOTIFY:
				SendMessage(m_pView->GetPageHWND(),PM_RESEND_SELECTED_ITEM_DATA,0,0); // todo:
				return 0;
		}
#if _ENABLE_FILELIST_SUB_PANE
		LRESULT lResult;
		if( CSimpleVSplitWindowEx::ProcessWindowMessage(hWnd,uMsg,wParam,lParam,lResult) )
		{
			return lResult;
		}
		if( CSimpleHSplitWindowEx::ProcessWindowMessage(hWnd,uMsg,wParam,lParam,lResult) )
		{
			return lResult;
		}
#endif
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
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

#if _ENABLE_FILELIST_SUB_PANE
		HDWP hdwp = BeginDeferWindowPos(4);

		int cxPropPane = 0;
		int xPropPaneSplitter = 0;
		int yInfoPaneSplitter = 0;
		int cxVolPane  = 0;
		int cyInfoPane = 0;

		if( cx > 0 )
		{
			// Volume Pane
			if( m_pVolPane && IsWindowVisibleEx(m_pVolPane->m_hWnd) )
			{
#if 1
				cxVolPane = m_xVolumeSplitter;
				if( cxVolPane == -1 ) {
					int cxWidth = _GetTextWidth(m_hWnd,m_pVolPane->GetListFont(),L"{88888888-8888-8888-8888-888888888888}");
					cxVolPane = cxWidth + _DPI_Adjust_X(24) + GetSystemMetrics(SM_CXVSCROLL);
				}
#else	
				int cxWidth = _GetTextWidth(m_hWnd,m_pVolPane->GetListFont(),L"{88888888-8888-8888-8888-888888888888}");
				cxVolPane = cxWidth + _DPI_Adjust_X(24) + GetSystemMetrics(SM_CXVSCROLL);
#endif
			}

			// Properties Pane
			if( m_pPropPane && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
			{
				if( absSplitPos )
				{
					if( m_xPropSplitter == -1 )
						m_xPropSplitter = cx - m_cxPropPane;
					cxPropPane = cx - m_xPropSplitter;
					xPropPaneSplitter = m_xPropSplitter;
				}
				else
				{
					if( m_xPropSplitter == -1 )
						m_xPropSplitter = cx - m_cxPropPane;
					m_xPropSplitter = xPropPaneSplitter = cx - m_cxPropPane; // update splitter pos
					cxPropPane = cx - m_xPropSplitter;
				}
			}

			// Information Pane
			if( m_pInfoPane && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
			{
				if( absSplitPos )
				{
					if( m_cyInfoPane == -1 )
						m_cyInfoPane = cy / 2;
					if( m_yInfoSplitter == -1 )
						m_yInfoSplitter = cy - m_cyInfoPane;
					cyInfoPane = cy - m_yInfoSplitter;
					yInfoPaneSplitter = m_yInfoSplitter;
				}
				else
				{
					if( m_cyInfoPane == -1 )
						m_cyInfoPane = cy / 2;
					if( m_yInfoSplitter == -1 )
						m_yInfoSplitter = cy - m_cyInfoPane;
					m_yInfoSplitter = yInfoPaneSplitter = cy - m_cyInfoPane; // update splitter pos
					cyInfoPane = cy - m_yInfoSplitter;
				}
			}
		}

		if( m_pView && m_pView->GetPageHWND() )
		{
			DeferWindowPos(hdwp,m_pView->GetPageHWND(),NULL,cxVolPane,0,cx-cxPropPane-cxVolPane,cy-cyInfoPane-1,SWP_NOZORDER|SWP_NOCOPYBITS);
		}

		if( m_pVolPane && IsWindowVisibleEx(m_pVolPane->m_hWnd) )
		{
			DeferWindowPos(hdwp,m_pVolPane->m_hWnd,NULL,0,0,cxVolPane-1,cy,SWP_NOZORDER|SWP_NOCOPYBITS);
		}

		if( m_pPropPane && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
		{
			DeferWindowPos(hdwp,m_pPropPane->m_hWnd,NULL,xPropPaneSplitter+1,0,cxPropPane-1,cy,SWP_NOZORDER|SWP_NOCOPYBITS);
		}

		if( m_pInfoPane && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
		{
			DeferWindowPos(hdwp,m_pInfoPane->m_hWnd,NULL,cxVolPane,yInfoPaneSplitter+1,cx-cxPropPane-cxVolPane,cyInfoPane-1,SWP_NOZORDER|SWP_NOCOPYBITS);
		}

		EndDeferWindowPos(hdwp);
#else
		if( m_pView && m_pView->GetPageHWND() )
		{
			SetWindowPos(m_pView->GetPageHWND(),NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
#endif
	}

	VOID InitData(PCWSTR pszDirectoryPath)
	{
		SELECT_ITEM sel = {0};
		sel.ViewType  = m_ConsoleTypeId;
		sel.pszCurDir = (PWSTR)pszDirectoryPath;
		sel.pszPath   = (PWSTR)pszDirectoryPath;
		sel.pszName   = (PWSTR)NULL;
		m_pView->SelectView(&sel);
	}

	VOID InitLayout(const RECT *prcDesktopWorkArea)
	{
	}

	VOID OnNotifyForwardToMainFrame(UINT code,PVOID pFile) // SELECT_ITEM* or UIS_ITEM_ACTIVATED *
	{
#if _ENABLE_FILELIST_SUB_PANE
		if( m_pPropPane && UI_NOTIFY_ITEM_SELECTED == code && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
		{
			SELECT_ITEM* p = (SELECT_ITEM*)pFile;
			SendMessage(m_pPropPane->m_hWnd,PPM_SETPATH,(WPARAM)p->ContextPtr,(LPARAM)p->pszPath);
		}
		if( m_pVolPane && UI_NOTIFY_VOLUME_SELECTED == code )
		{
			SELECT_ITEM* p = (SELECT_ITEM*)pFile;
			SendMessage(m_pVolPane->m_hWnd,VNM_SELECTVOLUME,0,(LPARAM)p->pszName);
		}
		if( m_pVolPane && UI_NOTIFY_UPDATE_SUBPANE == code )
		{
			SELECT_ITEM* pSel = (SELECT_ITEM*)pFile;
			return;
		}
		if( m_pInfoPane && UI_NOTIFY_ITEM_SELECTED == code && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
		{
			SELECT_ITEM* p = (SELECT_ITEM*)pFile;
			SendMessage(m_pInfoPane->m_hWnd,PPM_SETPATH,(WPARAM)p->ContextPtr,(LPARAM)p->pszPath);
		}
#endif
		// Forward to MainFrame
		HWND hwndMainWnd = GetActiveWindow(); // todo:
		SendMessage(hwndMainWnd,WM_NOTIFY_MESSAGE,code,(LPARAM)pFile);
	}

	LRESULT OnSelectPage(HWND /*hWnd*/,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		UIS_PAGE *ppg = (UIS_PAGE *)lParam;

		if( ppg == NULL )
			return 0;

		SELECT_ITEM sel = {0};
		sel.ViewType = ppg->ConsoleTypeId;
		sel.Guid     = ppg->ConsoleGuid;
		sel.Flags    = ppg->dwFlags;

		if( ppg->ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT )
		{
			sel.ViewType  = ppg->ConsoleTypeId;
			sel.pszPath   = ppg->pszPath; // Handle of FIL
			sel.pszName   = NULL;
			sel.pszCurDir = NULL;
			sel.Flags     = 0;
			m_pView->SelectView(&sel);

			m_ConsoleTypeId = ppg->ConsoleTypeId;
		}
		else if( ppg->ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILES )
		{
			PWSTR pszPath;

			ASSERT(( ppg->pszPath != NULL && *ppg->pszPath != L'\0' ));		

			if( FindRootDirectory_W(ppg->pszPath,NULL) != STATUS_SUCCESS )
			{
				//
				// NT volume/drive name without root directory.
				//
				// convert volume name to root directory
				// "\Device\HarddiskVolume1" -> "\Device\HarddiskVolume1\"
				//
				if( !IsLastCharacterBackslash(ppg->pszPath) )
					pszPath = CombinePath(ppg->pszPath,L"\\");
				else
					pszPath = DuplicateString(ppg->pszPath);

				if( pszPath )
				{
					sel.ViewType  = ppg->ConsoleTypeId;
					sel.pszPath   = pszPath;
					sel.pszName   = NULL;
					sel.pszCurDir = NULL;
					sel.Flags     = ppg->dwFlags;
					sel.Context   = ppg->Context;

					m_pView->SelectView(&sel);

					m_ConsoleTypeId = ppg->ConsoleTypeId;
	
					FreeMemory(pszPath);
				}

			}
			else
			{
				//
				// NT volume/drive name with directory path.
				//
				pszPath = DuplicateString(ppg->pszPath);

				if( pszPath )
				{
					sel.ViewType  = ppg->ConsoleTypeId;
					sel.pszCurDir = NULL;
					sel.pszPath   = pszPath;
					sel.pszName   = ppg->pszFileName;
					sel.Flags     = ppg->dwFlags;
					sel.Context   = ppg->Context;

					m_pView->SelectView(&sel);

					m_ConsoleTypeId = ppg->ConsoleTypeId;
	
					FreeMemory(pszPath);
				}
			}
		}
		else
		{
			//
			// Other Console
			//
			sel.ViewType  = ppg->ConsoleTypeId;
			sel.pszCurDir = NULL;
			sel.pszPath   = NULL;
			sel.pszName   = ppg->pszFileName;
			sel.Flags     = ppg->dwFlags;
			m_pView->SelectView(&sel);
		}

		CONSOLE_VIEW_ID *pcv = _GET_CONSOLE_VIEW_ID(m_hWnd);
		if( pcv )
			pcv->wndId = ppg->ConsoleTypeId;

		return 0;
	}

	LRESULT OnChangeDirectory(SELECT_ITEM* pSelectItem)
	{
		//
		// Clear Properties
		//
#if _ENABLE_FILELIST_SUB_PANE
		if( m_pPropPane && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
			SendMessage(m_pPropPane->m_hWnd,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
		if( m_pInfoPane && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
			SendMessage(m_pInfoPane->m_hWnd,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
#endif
		if( (pSelectItem->mask & SI_MASK_FILEID) && (pSelectItem->Flags & _FLG_NTFS_SPECIALFILE) )
		{
			m_pView->SelectView(pSelectItem);
			return 0;
		}

		if( pSelectItem->pszName && wcscmp(pSelectItem->pszName,L"..") == 0 )
		{
			PWSTR pszNewCurDir;
			PWSTR pszFileName;
			PWSTR pszRootDir;

			//
			// Check current is rootChecks if current page in the root directory.
			//
			pszRootDir = CombinePath(pSelectItem->pszVolume,L"\\");
			if( wcsicmp(pSelectItem->pszCurDir,pszRootDir) == 0 )
			{
				FreeMemory(pszRootDir);
				return 0;
			}
			FreeMemory(pszRootDir);

			//
			// Get file name for select on the file list when changed to new page.
			//
			pszFileName = (PWSTR)FindFileName_W( pSelectItem->pszCurDir );
			pszFileName = _MemAllocString( pszFileName  );

			//
			// Get directory path to go new page.
			//
			SIZE_T cchNewCurDir = wcslen(pSelectItem->pszCurDir) + 1; // length with a include extra character.
			pszNewCurDir = _MemAllocStringBuffer( cchNewCurDir  );
			StringCchCopy(pszNewCurDir,cchNewCurDir,pSelectItem->pszCurDir);
			RemoveFileSpec_W(pszNewCurDir);
			if( wcsicmp(pszNewCurDir,pSelectItem->pszVolume) == 0 )
			{
				// We are to go the root directory.
				StringCchCat(pszNewCurDir,cchNewCurDir,L"\\");
			}

			SELECT_ITEM sel = {0};
			sel.ViewType  = m_ConsoleTypeId;
			sel.pszPath   = pszNewCurDir; // change to  directory
			sel.pszName   = pszFileName;
			m_pView->SelectView(&sel);

			_SafeMemFree(pszNewCurDir);
			_SafeMemFree(pszFileName);
		}
		else
		{
			SELECT_ITEM sel = *pSelectItem; // trick! for Specified DOS Path
			PWSTR pszPath = NULL;

			if( pSelectItem->pszPath )
			{
				ULONG PathType = GetPathType(pSelectItem->pszPath);

				if( PATHTYPE_NT_DEVICE != PathType )
				{
					pszPath = DosPathNameToNtPathName_W(pSelectItem->pszPath);
				}
				else
				{
					pszPath = DuplicateString(pSelectItem->pszPath);
				}

				sel.pszPath = pszPath;
			}

			m_pView->SelectView( &sel );

			FreeMemory(pszPath);
		}
		return 0;
	}

#if _ENABLE_FILELIST_SUB_PANE
	void CreatePropPane(BOOL bVisible=TRUE)
	{
		m_pPropPane = new CPropPaneWindow();

		DWORD dwStyle = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|(bVisible ? WS_VISIBLE : 0);
		DWORD dwExStyle = WS_EX_CONTROLPARENT;
		m_pPropPane->Create(m_hWnd,0,L"PropPane",dwStyle,dwExStyle);
	}

	void CreateVolummePane(BOOL bVisible=TRUE)
	{
		m_pVolPane = new CVolumeSelectPane();

		DWORD dwStyle = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|(bVisible ? WS_VISIBLE : 0);
		DWORD dwExStyle = WS_EX_CONTROLPARENT;
		m_pVolPane->Create(m_hWnd,0,L"VolumePane",dwStyle,dwExStyle);
	}

	void CreateInfoPane(BOOL bVisible=TRUE)
	{
		m_pInfoPane = new CInfoPaneWindow();

		DWORD dwStyle = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|(bVisible ? WS_VISIBLE : 0);
		DWORD dwExStyle = WS_EX_CONTROLPARENT;
		m_pInfoPane->Create(m_hWnd,0,L"InfoPane",dwStyle,dwExStyle);
	}

	void OnViewVolumeSelectorPane()
	{
		ShowPane(m_pVolPane->m_hWnd);
	}

	void OnViewPropertyPane()
	{
		if( ShowPane(m_pPropPane->m_hWnd) )
		{
			SendMessage(m_pView->GetPageHWND(),PM_RESEND_SELECTED_ITEM_DATA,0,0);
		}
	}

	void OnViewInformationPane()
	{
		if( ShowPane(m_pInfoPane->m_hWnd) )
		{
			SendMessage(m_pView->GetPageHWND(),PM_RESEND_SELECTED_ITEM_DATA,0,0);
		}
	}

	BOOL ShowPane(HWND hwndPane)
	{
		int nCmdShow;
		BOOL bEnable;
		if( IsWindowVisibleEx( hwndPane ) )
		{
			bEnable = FALSE;
			nCmdShow = SW_HIDE;
		}
		else
		{
			bEnable = TRUE;
			nCmdShow = SW_SHOW;
		}
		ShowWindow(hwndPane,nCmdShow);
		EnableWindow(hwndPane,bEnable);
		UpdateLayout();
		return bEnable;
	}

	virtual BOOL IsOverSplitterV(int x,int y)
	{
		int split = checkSplitter(x,y);
		if( split == 1 || split == 2 )
			return TRUE;
		return FALSE;
	}

	virtual BOOL IsOverSplitterH(int x,int y)
	{
		if( checkSplitter(x,y) == 3 )
			return TRUE;
		return FALSE;
	}

	virtual void EnterSplitterDrag()
	{
	}

	virtual void LeaveSplitterDrag()
	{
		RECT rc;
		GetClientRect(m_hWnd,&rc);
		if( CSimpleVSplitWindowEx::GetSplitterValuePtr() == &m_xPropSplitter )
		{
			int cx = _RECT_WIDTH(rc);
			m_cxPropPane = cx - m_xPropSplitter;
		}
		if( CSimpleHSplitWindowEx::GetSplitterValuePtr() == &m_yInfoSplitter )
		{
			int cy = _RECT_HIGHT(rc);
			m_cyInfoPane = cy - m_yInfoSplitter;
		}
		CSimpleVSplitWindowEx::SetSplitterValue(nullptr);
	}
private:	
	int checkSplitter(int x,int y)
	{
		// Volume Pane Splitter		
		if( m_pVolPane && IsWindowVisibleEx(m_pVolPane->m_hWnd) )
		{
			if( ((m_xVolumeSplitter-4) <= x) && (x <= (m_xVolumeSplitter)) )
			{
				CSimpleVSplitWindowEx::SetSplitterValue(&m_xVolumeSplitter);
				return 1;
			}
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		int cx = _RECT_WIDTH(rc);
		int cy = _RECT_HIGHT(rc);

		// Property Pane Splitter		
		if( m_pPropPane && IsWindowVisibleEx(m_pPropPane->m_hWnd) ) 
		{
			int xSplitPos = m_xPropSplitter;
			if( xSplitPos == -1 )
				xSplitPos = cx - m_cxPropPane;
			if( xSplitPos > 0 )
			{
				if( ((xSplitPos-2) <= x) && (x <= (xSplitPos+2)) )
				{
					//m_xPropSplitter = xSplitPos;
					CSimpleVSplitWindowEx::SetSplitterValue(&m_xPropSplitter);
					return 2;
				}
			}
		}

		// Information Pane Splitter		
		if( m_pInfoPane && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
		{
			int ySplitPos = m_yInfoSplitter;
			if( ySplitPos == -1 )
				ySplitPos = cy / 2;
			if( ySplitPos > 0 )
			{
				int left = 0;
				int right = cx;
	
				if( m_pVolPane && IsWindowVisibleEx(m_pVolPane->m_hWnd) )
					left = m_xVolumeSplitter;
				if( m_pPropPane && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
					right = m_xPropSplitter;
				if( left < x && x < right )
				{
					if( ((ySplitPos-2) <= y) && (y <= (ySplitPos+2)) )
					{
						CSimpleHSplitWindowEx::SetSplitterValue(&m_yInfoSplitter);
						return 3;
					}
				}
			}
		}
		return 0;
	}

	int m_xVolumeSplitter;
	int m_cxPropPane;
	int m_xPropSplitter;
	int m_yInfoSplitter;
	int m_cyInfoPane;

#endif
};

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  _CreateVolumeFileListWindow()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
HWND _CreateVolumeFileListWindow(HWND hWndParent,UINT ConsoleType,DWORD dwOptionFlags,LPARAM lParam)
{
	CVolumeFilesWindow::RegisterClass(GETINSTANCE(hWndParent));

	CVolumeFilesWindow *pWnd = new CVolumeFilesWindow(ConsoleType);

	pWnd->m_dwViewStyle = dwOptionFlags;

	HWND hwndView = pWnd->Create(hWndParent,0,L"SimpleVolumeFileList",
							WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

	return hwndView;
}

//////////////////////////////////////////////////////////////////////////////

EXTERN_C
HWND
WINAPI
CreateVolumeFileList(
	HWND hwnd,
	UINT ConsoleId,
	DWORD dwOptionFlags,
	LPARAM lParam
	)
{
	return _CreateVolumeFileListWindow(hwnd,ConsoleId,dwOptionFlags,lParam);
}
