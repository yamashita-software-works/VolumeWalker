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
#include "page_volumefilelist_location.h"

class CVolumeFilesWindow : public CBaseWindow
{
	HWND m_hwndSubView;
	HWND m_hWndCtrlFocus;
	UINT m_ConsoleTypeId;

	CFilesPageHost m_PageHost;
	CFilesPageHost *m_pView;

public:
	DWORD m_dwViewStyle;

public:
	CVolumeFilesWindow(UINT ConsoleTypeId)
	{
		m_hWnd = NULL;
		m_pView = NULL;
		m_hwndSubView = NULL;
		m_hWndCtrlFocus = NULL;
		m_ConsoleTypeId = ConsoleTypeId;
		m_dwViewStyle = 0;
	}

	~CVolumeFilesWindow()
	{
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		m_pView = &m_PageHost;

		m_pView->m_hWnd = m_hWnd;

#if 0
		if( m_ConsoleTypeId == VOLUME_CONSOLE_FILE_MANAGER )
			m_pView->SetState(0,1);
		else if( m_ConsoleTypeId == VOLUME_CONSOLE_FILES || m_ConsoleTypeId == VOLUME_CONSOLE_FILE_LOCATION )
			m_pView->SetState(0,2);
#else
		m_pView->SetState(0,2);
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
		if( m_hWndCtrlFocus == m_pView->GetPageHWND() )
		{
			switch( LOWORD(wParam) )
			{
#if 0
				case ID_FILE_ROOTDIRECTORIES:
				{
					SELECT_ITEM sel = {0};
					sel.ViewType  = VOLUME_CONSOLE_FILE_ROOTDIRECTORIES;
					sel.pszCurDir = NULL;
					sel.pszPath   = NULL;
					sel.pszName   = NULL;
					m_pView->SelectView(&sel);
					break;
				}
#endif
				case ID_FILE_VOLUMEFILES:
				{
					SELECT_ITEM sel = {0};
					sel.ViewType  = VOLUME_CONSOLE_VOLUMEFILELIST;
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
		if( m_hWndCtrlFocus == m_pView->GetPageHWND() )
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
			case UI_NOTIFY_ITEM_ACTIVATED:
				OnItemActivated( (UIS_ITEM_ACTIVATED*)lParam );
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
				return (LRESULT)GetStockObject(NULL_BRUSH);
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
				if( m_pView )
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam); // forward to current page
				return 0;
		}
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

		HDWP hdwp = BeginDeferWindowPos(1);

		if( m_pView && m_pView->GetPageHWND() )
		{
			DeferWindowPos(hdwp,m_pView->GetPageHWND(),NULL,0,0,cx,cy,SWP_NOZORDER);
		}

		EndDeferWindowPos(hdwp);
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

	VOID OnItemActivated(UIS_ITEM_ACTIVATED *pia)
	{
		// Forward to MainFrame
		HWND hwndMainWnd = GetActiveWindow(); // todo:
		SendMessage(hwndMainWnd,WM_NOTIFY_MESSAGE,UI_NOTIFY_ITEM_ACTIVATED,(LPARAM)pia);
	}

	VOID OnItemSelected(SELECT_ITEM* pFile)
	{
		// Forward to MainFrame
		HWND hwndMainWnd = GetActiveWindow(); // todo:
		SendMessage(hwndMainWnd,WM_NOTIFY_MESSAGE,UI_NOTIFY_ITEM_SELECTED,(LPARAM)pFile);
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

		if( ppg->pszPath != NULL && *ppg->pszPath != L'\0' )
		{
			PWSTR pszPath;

			if( FindRootDirectory_W(ppg->pszPath,NULL) != STATUS_SUCCESS )
			{
				//
				// NT volume/drive name without root directory.
				//
#if 0 // ***If Accept Volume Name in Root directory list***
				pszPath = DuplicateString(ppg->pszPath);
#else
				pszPath = CombinePath(ppg->pszPath,L"\\");
#endif

				if( pszPath )
				{
#if 0 // ***If Accept Volume Name in Root directory list***
					sel.ViewType  = ppg->ConsoleTypeId;
					sel.pszPath   = NULL;
					sel.pszName   = NULL;
					sel.pszCurDir = pszPath;
					sel.Flags     = SI_FLAG_ROOT_DIRECTORY;
#else
					sel.ViewType  = ppg->ConsoleTypeId;
					sel.pszPath   = pszPath;
					sel.pszName   = NULL;
					sel.pszCurDir = NULL;
					sel.Flags     = 0;
#endif
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
					m_pView->SelectView(&sel);

					m_ConsoleTypeId = ppg->ConsoleTypeId;
	
					FreeMemory(pszPath);
				}
			}
		}
		else
		{
			//
			// Root Directories
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
				sel.ViewType  = m_ConsoleTypeId;
				sel.pszCurDir = pszPath;
				sel.pszPath   = pszPath;
				sel.pszName   = pszName;
				m_pView->SelectView(&sel);
			}
			_SafeMemFree(pszPath);
			_SafeMemFree(pszName);
		}
		else
		{
			SELECT_ITEM sel = *pFile; // trick! for Specified DOS Path
			PWSTR pszPath = NULL;

			if( pFile->pszPath )
			{
				ULONG PathType = GetPathType(pFile->pszPath);

				if( PATHTYPE_NT_DEVICE != PathType )
				{
					pszPath = DosPathNameToNtPathName_W(pFile->pszPath);
				}
				else
				{
					pszPath = DuplicateString(pFile->pszPath);
				}

				sel.pszPath = pszPath;
			}

			m_pView->SelectView( &sel );

			FreeMemory(pszPath);
		}
		return 0;
	}
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

	HWND hwndView = pWnd->Create(hWndParent,0,L"SimpleVolumeFileList",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

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
