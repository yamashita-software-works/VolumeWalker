//****************************************************************************
//
//  shellwindow.cpp
//
//  Implements the shell console host window.
//
//  Author:  YAMASHITA Katsuhiro
//
//  History: 2026-06-15 Created.
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
#include "page_shell_recyclebin_volumes.h"
#include "page_shell_recyclebin_files.h"

typedef enum {
	PAGE_SH_UNKNOWN = 0,
	PAGE_SH_RECYCLEBIN = 1,
	PAGE_SH_RECYCLEBIN_FILES,
	MAX_SH_PAGE,
} PAGE_SH_TYPE;

class CShellConsoleWindow : public CBaseWindow
{
	HWND m_hWndCtrlFocus;
	UINT m_ConsoleTypeId;

	CPageWndBase *m_pPage;

	CPageWndBase *m_pPageTable[ MAX_SH_PAGE ];

public:
	DWORD m_dwViewStyle;

	template <class T,PAGE_SH_TYPE N>
	CPageWndBase *__GetOrCreateWndObjct(HWND hWnd,BOOL *pbCreate=nullptr)
	{
		CPageWndBase *pobj = m_pPageTable[(int)N];
		if( pobj == NULL )
		{
			pobj = (CPageWndBase*)new T ;
			pobj->Create(hWnd,(int)N,0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);
			m_pPageTable[(int)N] = pobj;
			if( pbCreate )
				*pbCreate = TRUE;
		}
		else
		{
			if( pbCreate )
				*pbCreate = FALSE;
		}
		return pobj;
	}

public:
	CShellConsoleWindow(UINT ConsoleTypeId)
	{
		m_pPage = NULL;
		m_hWndCtrlFocus = NULL;
		m_ConsoleTypeId = ConsoleTypeId;
		m_dwViewStyle = 0;
		ZeroMemory(m_pPageTable,sizeof(m_pPageTable));
	}

	~CShellConsoleWindow()
	{
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
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
		return (LRESULT)TRUE;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( m_hWndCtrlFocus == NULL )
			m_hWndCtrlFocus = m_pPage->m_hWnd;

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
		if( m_hWndCtrlFocus == m_pPage->m_hWnd )
		{
			if( m_pPage )
				m_pPage->InvokeCommand(uCmdId);
		}
		return 0;
	}

	LRESULT OnQueryCmdState(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		UINT *puState = (UINT *)lParam;
		UINT uCmdId = (UINT)LOWORD(wParam);

		if( m_hWndCtrlFocus == m_pPage->m_hWnd )
		{
			ASSERT( lParam != NULL );
			if( lParam )
			{
				if( m_pPage->QueryCmdState((UINT)LOWORD(wParam),(UINT*)lParam) == S_OK )
					return TRUE;
			}
		}

		return FALSE;
	}

	LRESULT OnNotifyMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_NOTIFY_ITEM_SELECTED:    // SELECT_ITEM*
			case UI_NOTIFY_VOLUME_SELECTED:  // SELECT_ITEM*
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
				return SendMessage(m_pPage->m_hWnd,uMsg,wParam,lParam);
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
			case UI_INIT_LAYOUT_EX:
				InitLayoutEx((PAGE_CONTEXT *)lParam);
				break;
		}
		return 0;
	}

	LRESULT OnSaveConfig(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ISaveViewConfig *psvc = (ISaveViewConfig *)lParam;
		if( psvc != NULL )
		{
			// todo:
			return SendMessage(m_pPage->m_hWnd,uMsg,wParam,lParam); // forward to current page
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
			case WM_QUERY_CMDSTATE:
				return OnQueryCmdState(hWnd,uMsg,wParam,lParam);
			case WM_CONTROL_MESSAGE:
				return OnControlMessage(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY_MESSAGE:
				return OnNotifyMessage(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_MESSAGE:
				return OnQueryMessage(hWnd,uMsg,wParam,lParam);
			case PM_SAVECONFIG:
				return OnSaveConfig(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
			case WM_PRETRANSLATEMESSAGE:
				if( m_pPage )
					return SendMessage(m_pPage->m_hWnd,uMsg,wParam,lParam); // forward to current page
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

		if( m_pPage && m_pPage->m_hWnd )
		{
			SetWindowPos(m_pPage->m_hWnd,NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
	}

	VOID InitLayoutEx(PAGE_CONTEXT *pParam)
	{
		ASSERT(pParam);
		ASSERT(pParam->MainApp);
		if( pParam == NULL )
			return ;
		if( m_pPage )
			m_pPage->OnInitLayout(NULL);
	}

	VOID OnNotifyForwardToMainFrame(UINT code,PVOID pFile) // SELECT_ITEM* or UIS_ITEM_ACTIVATED *
	{
		// Forward to MainFrame
		HWND hwndMainWnd = GetActiveWindow(); // todo:
		SendMessage(hwndMainWnd,WM_NOTIFY_MESSAGE,code,(LPARAM)pFile);
	}

	LRESULT OnSelectPage(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		CPageWndBase *pPage;

		UIS_PAGE *ppg = (UIS_PAGE *)lParam;

		if( ppg == NULL )
			return 0;

		BOOL bCreate = FALSE;

		SELECT_ITEM sel = {0};
		sel.ViewType = ppg->ConsoleTypeId;
		sel.Guid     = ppg->ConsoleGuid;
		sel.Flags    = ppg->dwFlags;

		if( ppg->ConsoleTypeId == VOLUME_CONSOLE_SHELL_RECYCLEBIN )
		{
			pPage = __GetOrCreateWndObjct<CRecycleBinVolumesPage,PAGE_SH_RECYCLEBIN>(hWnd,&bCreate);
			if( bCreate ) {
			sel.ViewType  = ppg->ConsoleTypeId;
			sel.pszCurDir = NULL;
			sel.pszPath   = NULL;
			sel.pszName   = ppg->pszFileName;
			sel.Flags     = ppg->dwFlags;
			sel.Context   = ppg->Context;
			pPage->OnInitPage(&sel,0,nullptr);
			}
		}
		else if( ppg->ConsoleTypeId == VOLUME_CONSOLE_SHELL_RECYCLEBIN_FILES )
		{
			pPage = __GetOrCreateWndObjct<CRecycleBinFilesPage,PAGE_SH_RECYCLEBIN_FILES>(hWnd,&bCreate);
			if( bCreate ) {
			sel.ViewType  = ppg->ConsoleTypeId;
			sel.pszPath   = ppg->pszPath;
			sel.pszName   = NULL;
			sel.pszCurDir = NULL;
			sel.Flags     = 0;
			pPage->OnInitPage(&sel,0,nullptr);
			}
		}
		else
		{
			ASSERT(FALSE);
		}

		m_pPage = pPage;

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
//		if( (pSelectItem->mask & SI_MASK_FILEID) && (pSelectItem->Flags & _FLG_NTFS_SPECIALFILE) )
//		{
//			m_pPage->SelectView(pSelectItem);
//			return 0;
//		}

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
//			m_pPage->SelectView(&sel);

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

//			m_pPage->SelectView( &sel );

			FreeMemory(pszPath);
		}
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  _CreateShellConsoleWindow()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
HWND _CreateShellConsoleWindow(HWND hWndParent,UINT ConsoleType,DWORD dwOptionFlags,LPARAM lParam)
{
	CShellConsoleWindow::RegisterClass(GETINSTANCE(hWndParent));

	CShellConsoleWindow *pWnd = new CShellConsoleWindow(ConsoleType);

	pWnd->m_dwViewStyle = dwOptionFlags;

	HWND hwndView = pWnd->Create(hWndParent,0,L"Shell_ConsoleWindow",
							WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

	return hwndView;
}
