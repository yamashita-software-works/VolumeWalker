//****************************************************************************
//
//  filelistwindow.cpp
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
#if _ENABLE_FILELIST_SUBPANE
#include "simplesplitwindow.h"
#endif
#if _ENABLE_SUBPANE_NAVIGATION
#include "panebase_volumes.h"
#endif
#if _ENABLE_SUBPANE_PROPERTY
#include "panebase_property.h"
#endif
#if _ENABLE_SUBPANE_INFORMATION
#include "panebase_information.h"
#endif

#if _ENABLE_FILELIST_SUBPANE
class CVolumeFilesWindow : 
	public CBaseWindow,
	public CSimpleVSplitWindowEx<CVolumeFilesWindow>,
	public CSimpleHSplitWindowEx<CVolumeFilesWindow>
#else
class CVolumeFilesWindow : public CBaseWindow
#endif
{
	UINT m_HostConsoleTypeId;

	HWND m_hWndCtrlFocus;

	CFilesPageHost m_PageHost;
	CFilesPageHost *m_pView;

#if _ENABLE_SUBPANE_NAVIGATION
	CNavigationPane *m_pNaviPane;
#endif
#if _ENABLE_SUBPANE_PROPERTY
	CPropPaneWindow *m_pPropPane;
#endif
#if _ENABLE_SUBPANE_INFORMATION
	CInfoPaneWindow *m_pInfoPane;
#endif

	BOOL m_bInpacePageChange;
	BOOL m_bSettingsSaved;

	PWSTR m_pszCurDir;

	DWORD m_dwViewStyle;
public:
	VOID SetViewStyle(DWORD dwViewStyle) { m_dwViewStyle = dwViewStyle; }
	DWORD GetViewStyle() const { return m_dwViewStyle; }

public:
	CVolumeFilesWindow(UINT ConsoleTypeId)
	{
		ASSERT( (ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILES) || (ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT) );

		m_HostConsoleTypeId = ConsoleTypeId;
		m_pView = NULL;
		m_hWndCtrlFocus = NULL;
		m_dwViewStyle = 0;
		m_pszCurDir = NULL;
#if _ENABLE_SUBPANE_NAVIGATION
		m_pNaviPane = NULL;
		m_xVolumeSplitter = -1;
#endif
#if _ENABLE_SUBPANE_PROPERTY
		m_pPropPane = NULL;
		m_xPropSplitter = -1;
		m_cxPropPane = -1;
#endif
#if _ENABLE_SUBPANE_INFORMATION
		m_pInfoPane = NULL;
		m_yInfoSplitter = -1;
		m_cyInfoPane = -1;
#endif
		m_bInpacePageChange = FALSE;
		m_bSettingsSaved = FALSE;
	}

	~CVolumeFilesWindow()
	{
		_SafeMemFree(m_pszCurDir);
	}

	void _MakeSectionName(UINT ConsoleTypeId,PWSTR pszSectionName,int cchSectionName)
	{
		CONSOLE_VIEW_ID *pcv = (CONSOLE_VIEW_ID *)GetProp(m_hWnd,_PROP_CONSOLE_VIEW_ID);
		GUID Guid = pcv->wndGuid;
		if( ConsoleTypeId != 0 )
			Guid.Data1 = ConsoleTypeId;
		StringFromGUID(&Guid,pszSectionName,cchSectionName);
		GUIDStringRemoveBrackets(pszSectionName);
	}

	void _SetCurPath(PCWSTR pszPath)
	{
		_SafeMemFree(m_pszCurDir);
		if( pszPath ) {
			m_pszCurDir = _MemAllocString(pszPath);
		}
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		m_pView = &m_PageHost;

		m_pView->m_hWndHost = m_hWnd;
		m_pView->m_dwFlags = m_dwViewStyle;

#if _ENABLE_SUBPANE_NAVIGATION
		m_xVolumeSplitter = -1;
#endif
#if _ENABLE_SUBPANE_PROPERTY
		CreatePropPane(FALSE);
		m_cxPropPane      = _DPI_Adjust_X(320);
#endif
#if _ENABLE_SUBPANE_INFORMATION
		CreateInfoPane(FALSE);
		m_cyInfoPane      = -1;
#endif
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		DestroyIcon((HICON)SendMessage(GetParent(m_hWnd),WM_GETICON,ICON_SMALL,0));
		return 0;
	}

	LRESULT OnMDIChildFrameClose(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		// Reserved:
		// In this handler, The _PROP_CONSOLE_VIEW_ID (pointer to CONSOLE_VIEW_ID structure) 
		// property is available.
		// It cannot be available in the WM_DESTROY handler.
		// CONSOLE_VIEW_ID *pcv = (CONSOLE_VIEW_ID *)GetProp(m_hWnd,_PROP_CONSOLE_VIEW_ID);
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
#if _ENABLE_FILELIST_SUBPANE
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

#if _ENABLE_SUBPANE_NAVIGATION
		rc.top = 0;
		rc.bottom = cy;
		rc.left = m_xVolumeSplitter - 1;  // Left Align Pane
		rc.right = rc.left + 1;
		FillRect(hdcMem,&rc,hbrSplitter);
#endif
#if _ENABLE_SUBPANE_PROPERTY
		rc.top = 0;
		rc.bottom = cy;
		rc.left = m_xPropSplitter;        // Right Align Pane
		rc.right = rc.left + 1;
		FillRect(hdcMem,&rc,hbrSplitter);
#endif
#if _ENABLE_SUBPANE_INFORMATION
		rc.top    = m_yInfoSplitter - 1;  // Bottom Align Pane
		rc.bottom = rc.top + 2;
		rc.left   = 0;
		rc.right  = cx;
		FillRect(hdcMem,&rc,hbrSplitter);
#endif
		DeleteObject(hbrSplitter);

		BitBlt(hdc,
			0,0,
			cx,
			cy,
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

#if _ENABLE_FILELIST_SUBPANE
		switch( uCmdId )
		{
#if _ENABLE_SUBPANE_NAVIGATION
			case ID_VIEW_NAVIGATION_NONE:
			case ID_VIEW_NAVIGATION_VOLUMES:
			case ID_VIEW_NAVIGATION_DIRTRAVERSER:
				OnViewNavigationPane(uCmdId);
				return 0;
#endif
#if _ENABLE_SUBPANE_PROPERTY
			case ID_VIEW_PROPERTYPANE:
				OnViewPropertyPane();
				return 0;
#endif
#if _ENABLE_SUBPANE_INFORMATION
			case ID_VIEW_INFORMATIONPANE:
				OnViewInformationPane();
				return 0;
#endif
		}
#endif
		switch( uCmdId )
		{
			case ID_VIEW_FILELIST:
				OnFileListView();
				return 0;
			case ID_VIEW_FILEDATETIME:
				OnFileDateTimeView();
				return 0;
		}

		if( m_hWndCtrlFocus == m_pView->GetPageHWND() )
		{
			if( m_pView )
				m_pView->InvokeCommand(uCmdId);
		}
#if _ENABLE_SUBPANE_PROPERTY
		else if( this->m_hWndCtrlFocus == this->m_pPropPane->m_hWnd )
		{
			m_pPropPane->OnCommand(m_pPropPane->m_hWnd,WM_COMMAND,(WPARAM)uCmdId,0);
		}
#endif
		return 0;
	}

	LRESULT OnQueryCmdState(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		UINT *puState = (UINT *)lParam;
		UINT uCmdId = (UINT)LOWORD(wParam);

#if _ENABLE_SUBPANE_PROPERTY || _ENABLE_SUBPANE_INFORMATION
		switch( uCmdId )
		{
#if _ENABLE_SUBPANE_PROPERTY
			case ID_VIEW_PROPERTYPANE:
				*puState = UPDUI_ENABLED | (IsWindowVisibleEx(m_pPropPane->m_hWnd) ? UPDUI_CHECKED : 0);
				return TRUE;
#endif
#if _ENABLE_SUBPANE_INFORMATION
			case ID_VIEW_INFORMATIONPANE:
				*puState = UPDUI_ENABLED | (IsWindowVisibleEx(m_pInfoPane->m_hWnd) ? UPDUI_CHECKED : 0);
				return TRUE;
#endif
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
		else
		{
#if _ENABLE_SUBPANE_PROPERTY
			if( m_pPropPane && IsWindowVisibleEx( m_pPropPane->m_hWnd ) )
			{
				if( m_pPropPane->QueryCmdState((UINT)LOWORD(wParam),(UINT*)lParam) == S_OK )
					return TRUE;
			}
#endif
		}

		return FALSE;
	}

	LRESULT OnQueryCmdStateEx(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		// wParam - LOWORD : Command ID
		//          HIWORD : 0
		// lParam - Pointer to COMMAND_STATE structure.
		COMMAND_STATE *pcs = (COMMAND_STATE *)lParam;
		if( pcs )
		{
			switch( LOWORD(wParam) )
			{
				case ID_VIEW_FILELIST:
					pcs->State   = UPDUI_ENABLED|((VOLUME_CONSOLE_VOLUMEFILES == m_pView->GetCurConsoleId()) ? UPDUI_RADIO : 0);
					pcs->IdFirst = ID_VIEW_FILELIST;
					pcs->IdLast  = ID_VIEW_FILEDATETIME;
					return TRUE;
				case ID_VIEW_FILEDATETIME:
					pcs->State   = UPDUI_ENABLED|((VOLUME_CONSOLE_VOLUMEFILES_DATETIME == m_pView->GetCurConsoleId()) ? UPDUI_RADIO : 0);
					pcs->IdFirst = ID_VIEW_FILELIST;
					pcs->IdLast  = ID_VIEW_FILEDATETIME;
					return TRUE;
#if _ENABLE_SUBPANE_NAVIGATION
				case ID_VIEW_NAVIGATION_NONE:
				case ID_VIEW_NAVIGATION_VOLUMES:
				case ID_VIEW_NAVIGATION_DIRTRAVERSER:
					pcs->IdFirst = ID_VIEW_NAVIGATION_NONE;
					pcs->IdLast  = ID_VIEW_NAVIGATION_DIRTRAVERSER;
					pcs->State   = UPDUI_ENABLED;
					switch( LOWORD(wParam) )
					{
						case ID_VIEW_NAVIGATION_NONE:
							if( m_pNaviPane == NULL || !IsWindowVisibleEx(m_pNaviPane->m_hWnd) || (m_pNaviPane->GetCurPaneId()==0) )
								pcs->State = UPDUI_ENABLED|UPDUI_RADIO;
							break;
						case ID_VIEW_NAVIGATION_VOLUMES:
							if( m_pNaviPane && IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
								pcs->State |= ((m_pNaviPane && m_pNaviPane->GetCurPaneId()==NAVI_VOLUMES) ? UPDUI_RADIO : 0);
							break;
						case ID_VIEW_NAVIGATION_DIRTRAVERSER:
							if( m_pNaviPane && IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
								pcs->State |= ((m_pNaviPane && m_pNaviPane->GetCurPaneId()==NAVI_DIRTRAVERSER) ? UPDUI_RADIO : 0);
							break;
					}
					return TRUE;
#endif
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
#if _ENABLE_FILELIST_SUBPANE
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
			case UI_INIT_LAYOUT_EX:
				InitLayoutEx((PAGE_CONTEXT *)lParam);
				break;
		}
		return 0;
	}

	LRESULT OnSaveConfig(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ISaveViewConfig *psvc = (ISaveViewConfig *)lParam;

		if( psvc == NULL )
		{
			return ERROR_INVALID_PARAMETER;
		}

		if( m_pView )
		{
#if _ENABLE_FILELIST_SUBPANE
#if _ENABLE_SUBPANE_NAVIGATION
			if( m_xVolumeSplitter != -1 )
			{
				psvc->WriteValueInt(m_hWnd,L"Pane.Navigation",m_xVolumeSplitter);
				if( m_pNaviPane )
					psvc->WriteValueInt(m_hWnd,L"Pane.Navigation.Type",m_pNaviPane->GetCurPaneId());
			}
			else
			{
				psvc->WriteValue(m_hWnd,L"Pane.Navigation",NULL); // clear and reset pos
				psvc->WriteValue(m_hWnd,L"Pane.Navigation.Type",NULL); 
			}
#endif
#if _ENABLE_SUBPANE_PROPERTY
			if( this->m_xPropSplitter != -1 )
				psvc->WriteValueInt(m_hWnd,L"Pane.Properties",this->m_xPropSplitter);
			else
				psvc->WriteValue(m_hWnd,L"Pane.Properties",NULL); // clear and reset pos
#endif
#if _ENABLE_SUBPANE_INFORMATION
			if( this->m_yInfoSplitter != -1 )
				psvc->WriteValueInt(m_hWnd,L"Pane.Information",this->m_yInfoSplitter);
			else
				psvc->WriteValue(m_hWnd,L"Pane.Information",NULL); // clear and reset pos
#endif
#endif
			//
			// forward PM_SAVECONFIG message to current page
			//
			if( m_HostConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILES )
			{
				//
				// Current working directory
				//
				SaveCurrentDirectoryConfig(psvc);

				//
				// Each view configuration
				//
				CPageWndBase *fl = m_pView->GetPage(VOLUME_CONSOLE_VOLUMEFILES);
				if( fl )
					SendMessage(fl->GetHwnd(),PM_SAVECONFIG,0,lParam);
				CPageWndBase *st = m_pView->GetPage(VOLUME_CONSOLE_VOLUMEFILES_STREAMS);
				if( st )
					SendMessage(st->GetHwnd(),PM_SAVECONFIG,0,lParam);
				CPageWndBase *dt = m_pView->GetPage(VOLUME_CONSOLE_VOLUMEFILES_DATETIME);
				if( dt )
					SendMessage(dt->GetHwnd(),PM_SAVECONFIG,0,lParam);

				// Save configuration in host console (VOLUME_CONSOLE_VOLUMEFILES)
				PWSTR pszCurrentPageGuid = (PWSTR)m_pView->GetSectionName();

				ASSERT( pszCurrentPageGuid != NULL );

				if( pszCurrentPageGuid )
				{
					WCHAR szSectionName[40];
					_MakeSectionName(VOLUME_CONSOLE_VOLUMEFILES,szSectionName,_countof(szSectionName));
					psvc->WriteStringToSection(szSectionName,L"CurPage",pszCurrentPageGuid);
				}
			}
			else
			{
				// VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT
				SendMessage(m_pView->GetPageHWND(),PM_SAVECONFIG,wParam,lParam);
			}
		}

		m_bSettingsSaved = TRUE;

		return 0;
	}

	void SaveCurrentDirectoryConfig(ISaveViewConfig *psvc)
	{
		if( m_pszCurDir && *m_pszCurDir )
		{
			// ToDo:
			// Currently, not yet supported ultra deep path
			// (over MAX_PATH like as 32767 characters path).
			UNICODE_STRING usVolume;
			UNICODE_STRING usRelativePath;
			SplitVolumeRelativePath(m_pszCurDir,&usVolume,&usRelativePath);
	
			PWSTR pszVolumeName         = AllocateSzFromUnicodeString(&usVolume);
			PWSTR pszVolumeRelativePath = AllocateSzFromUnicodeString(&usRelativePath);
	
			if( pszVolumeName && pszVolumeRelativePath )
			{
				psvc->WriteValue( m_hWnd, C_KEY_VOLUME, pszVolumeName );
				psvc->WriteValue( m_hWnd, C_KEY_RELATIVEPATH, pszVolumeRelativePath );
			}
	
			FreeMemory(pszVolumeName);
			FreeMemory(pszVolumeRelativePath);
		}
		else
		{
			// Root Directories
			psvc->WriteValue( m_hWnd, C_KEY_VOLUME, L"\\" );
		}
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
			case WM_QUERY_CMDSTATE_EX:
				return OnQueryCmdStateEx(hWnd,uMsg,wParam,lParam);
			case WM_CONTROL_MESSAGE:
				return OnControlMessage(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY_MESSAGE:
				return OnNotifyMessage(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_MESSAGE:
				return OnQueryMessage(hWnd,uMsg,wParam,lParam);
			case WM_MDI_CHILDFRAME_CLOSE:
				return OnMDIChildFrameClose(hWnd,uMsg,wParam,lParam);
			case PM_SAVECONFIG:
				return OnSaveConfig(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
			case PM_GETSELECTEDFILE:
			case PM_GETWORKINGDIRECTORY:
				if( m_pView )
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam); // forward to current page
				return 0;
			case WM_PRETRANSLATEMESSAGE:
#if _ENABLE_SUBPANE_PROPERTY
				if( m_pPropPane )
					if( SendMessage(m_pPropPane->GetHwnd(),uMsg,wParam,lParam) )
						return TRUE;
#endif
				if( m_pView )
					return SendMessage(m_pView->GetPageHWND(),uMsg,wParam,lParam); // forward to current page
				return 0;
#if _ENABLE_FILELIST_SUBPANE
			case PM_RESEND_SELECTED_ITEM_DATA:
				SendMessage(m_pView->GetPageHWND(),PM_RESEND_SELECTED_ITEM_DATA,0,0);
				return 0;
#endif
		}
#if _ENABLE_FILELIST_SUBPANE
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

#if _ENABLE_FILELIST_SUBPANE
		HDWP hdwp = BeginDeferWindowPos(5);

		int cxPropPane = 0;
		int xPropPaneSplitter = 0;
		int yInfoPaneSplitter = 0;
		int cxVolPane  = 0;
		int cyInfoPane = 0;

		if( cx > 0 )
		{
#if _ENABLE_SUBPANE_NAVIGATION
			// Volume Pane
			if( m_pNaviPane && IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
			{
				if( m_xVolumeSplitter == -1 )
					m_xVolumeSplitter = _DPI_Adjust_X(320);
				cxVolPane = m_xVolumeSplitter;
				if( cxVolPane == -1 ) {
					int cxWidth = _GetTextWidth(m_hWnd,m_pNaviPane->GetListFont(),L"{88888888-8888-8888-8888-888888888888}");
					cxVolPane = cxWidth + _DPI_Adjust_X(24) + GetSystemMetrics(SM_CXVSCROLL);
				}
			}
#endif
#if _ENABLE_SUBPANE_PROPERTY
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
#endif
#if _ENABLE_SUBPANE_INFORMATION
			// Information Pane
			if( m_pInfoPane && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
			{
				if( absSplitPos )
				{
					if( m_cyInfoPane == -1 )
						m_cyInfoPane = (int)((double)cy * 0.44);
					if( m_yInfoSplitter == -1 )
						m_yInfoSplitter = cy - m_cyInfoPane;
					cyInfoPane = cy - m_yInfoSplitter;
					yInfoPaneSplitter = m_yInfoSplitter;
				}
				else
				{
					if( m_cyInfoPane == -1 )
						m_cyInfoPane = (int)((double)cy * 0.44);
					if( m_yInfoSplitter == -1 )
						m_yInfoSplitter = cy - m_cyInfoPane;
					m_yInfoSplitter = yInfoPaneSplitter = cy - m_cyInfoPane; // update splitter pos
					cyInfoPane = cy - m_yInfoSplitter;
				}
			}
#endif
		}

		int cxView = cx - cxPropPane - cxVolPane;
		int cyView = cy - cyInfoPane;
		int cyHeader = 0;
		if( m_pView && m_pView->GetPageHWND() )
		{
			DeferWindowPos(hdwp,m_pView->GetPageHWND(),NULL,cxVolPane,cyHeader,cxView,cyView-cyHeader,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
#if _ENABLE_SUBPANE_NAVIGATION
		if( m_pNaviPane && IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
		{
			DeferWindowPos(hdwp,m_pNaviPane->m_hWnd,NULL,0,0,cxVolPane-1,cy,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
#endif
#if _ENABLE_SUBPANE_PROPERTY
		if( m_pPropPane && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
		{
			DeferWindowPos(hdwp,m_pPropPane->m_hWnd,NULL,xPropPaneSplitter+1,cyHeader,cxPropPane-1,cy-cyHeader,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
#endif
#if _ENABLE_SUBPANE_INFORMATION
		if( m_pInfoPane && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
		{
			DeferWindowPos(hdwp,m_pInfoPane->m_hWnd,NULL,cxVolPane,yInfoPaneSplitter+1,cxView,cyInfoPane-1,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
#endif
		EndDeferWindowPos(hdwp);
#else
		if( m_pView && m_pView->GetPageHWND() )
		{
			SetWindowPos(m_pView->GetPageHWND(),NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOCOPYBITS);
		}
#endif
	}

	VOID InitLayoutEx(PAGE_CONTEXT *pParam)
	{
		ASSERT(pParam);
		ASSERT(pParam->MainApp);
		if( pParam == NULL )
			return ;

#if _ENABLE_SUBPANE_NAVIGATION
		if( m_pView && m_pView->GetCurConsoleId() == VOLUME_CONSOLE_VOLUMEFILES_STREAMS )
		{
			m_xVolumeSplitter = 260; // always showing volume navi pane with streams view.
		}
#endif

#if _ENABLE_FILELIST_SUBPANE
#if _ENABLE_SUBPANE_NAVIGATION
		int iNavigationPaneType = 0;
#endif
		ILoadViewConfig *pLoadConfig;
		if( pParam->MainApp->QueryInterface( I_LOADCONFIG, (void **)&pLoadConfig ) == S_OK )
		{
#if _ENABLE_SUBPANE_NAVIGATION
			m_xVolumeSplitter = pLoadConfig->ReadValueInt(m_hWnd,L"Pane.Navigation",m_xVolumeSplitter);
			iNavigationPaneType = pLoadConfig->ReadValueInt(m_hWnd,L"Pane.Navigation.Type",iNavigationPaneType);
#endif
#if _ENABLE_SUBPANE_PROPERTY
			m_xPropSplitter = pLoadConfig->ReadValueInt(m_hWnd,L"Pane.Properties",m_xPropSplitter);
#endif
#if _ENABLE_SUBPANE_INFORMATION
			m_yInfoSplitter = pLoadConfig->ReadValueInt(m_hWnd,L"Pane.Information",m_yInfoSplitter);
#endif
			pLoadConfig->Release();
		}
#if _ENABLE_SUBPANE_NAVIGATION
		if( m_xVolumeSplitter != -1 )
		{
			switch( iNavigationPaneType )
			{
				case 1:
				case 2:
					SendMessage(m_hWnd,WM_COMMAND,ID_VIEW_NAVIGATION_NONE+iNavigationPaneType,0);
					break;
			}
		}
#endif
#if _ENABLE_SUBPANE_PROPERTY
		if( m_xPropSplitter != -1 )
			SendMessage(m_hWnd,WM_COMMAND,ID_VIEW_PROPERTYPANE,0);
#endif
#if _ENABLE_SUBPANE_INFORMATION
		if( m_yInfoSplitter != -1 )
			SendMessage(m_hWnd,WM_COMMAND,ID_VIEW_INFORMATIONPANE,0);
#endif
#endif
	}

	VOID OnNotifyForwardToMainFrame(UINT code,PVOID pFile) // SELECT_ITEM* or UIS_ITEM_ACTIVATED *
	{
#if _ENABLE_FILELIST_SUBPANE
#if _ENABLE_SUBPANE_NAVIGATION
		if( m_pNaviPane && UI_NOTIFY_VOLUME_SELECTED == code )
		{
			SELECT_ITEM* p = (SELECT_ITEM*)pFile;
			SendMessage(m_pNaviPane->m_hWnd,PPM_SELECTVOLUME,0,(LPARAM)p->pszName);
		}
		if( m_pNaviPane && UI_NOTIFY_UPDATE_SUBPANE == code )
		{
			SELECT_ITEM* pSel = (SELECT_ITEM*)pFile;
			SendMessage(m_pNaviPane->m_hWnd,PPM_CHDIR,0,(LPARAM)pSel->pszPath);
			return;
		}
#endif
#if _ENABLE_SUBPANE_PROPERTY
		if( m_pPropPane && UI_NOTIFY_ITEM_SELECTED == code && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
		{
			SELECT_ITEM* p = (SELECT_ITEM*)pFile;
			SendMessage(m_pPropPane->m_hWnd,PPM_SETPATH,(WPARAM)p->ContextPtr,(LPARAM)p->pszPath);
		}
#endif
#if _ENABLE_SUBPANE_INFORMATION
		if( m_pInfoPane && UI_NOTIFY_ITEM_SELECTED == code && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
		{
			SELECT_ITEM* p = (SELECT_ITEM*)pFile;
			SendMessage(m_pInfoPane->m_hWnd,PPM_SETPATH,(WPARAM)p->ContextPtr,(LPARAM)p->pszPath);
		}
#endif
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

		IApplication *pApp = NULL;
		SendMessage(GetActiveWindow(),PM_GETAPPINFORMATION,0,(LPARAM)&pApp);

		if( pApp && (ppg->dwFlags & SI_FLAG_CREATE_PAGE) )
		{
			ILoadViewConfig *pLoadConfig;
			if( pApp->QueryInterface( I_LOADCONFIG, (void **)&pLoadConfig ) == S_OK )
			{
				WCHAR szConsoleGuid[48];
				if( pLoadConfig->ReadValue(m_hWnd,L"CurPage",&szConsoleGuid[1],ARRAYSIZE(szConsoleGuid)-1) > 0 )
				{
					// make a '{xxx}' style GUID string.
					GUID Guid;
					szConsoleGuid[0]  = L'{';
					szConsoleGuid[37] = L'}';
					szConsoleGuid[38] = L'\0';
					GUIDFromString(szConsoleGuid,&Guid);

					if( Guid.Data1 != VOLUME_CONSOLE_VOLUMEFILES )
						ppg->ConsoleTypeId = Guid.Data1;
				}
				pLoadConfig->Release();
			}
		}

		SELECT_ITEM sel = {0};
		sel.ViewType   = ppg->ConsoleTypeId;
		sel.Guid       = ppg->ConsoleGuid;
		sel.Flags      = ppg->dwFlags;
		sel.ContextPtr = pApp;
		sel.pszPath    = ppg->pszPath; // If type VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT, Handle of FIL.
		sel.pszName    = ppg->pszFileName;

		_SetCurPath( ppg->pszPath );

		if( ppg->ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT )
		{
			sel.pszName   = NULL;
			sel.pszCurDir = NULL;
			sel.Flags     = 0;
			m_pView->SelectView(&sel);
		}
		else if( ppg->ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILES || ppg->ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILES_DATETIME || ppg->ConsoleTypeId == VOLUME_CONSOLE_VOLUMEFILES_STREAMS )
		{
			PWSTR pszPath;

			ASSERT(( ppg->pszPath != NULL && *ppg->pszPath != L'\0' ));		

#if _ENABLE_SUBPANE_NAVIGATION
			if( m_bInpacePageChange )
			{
				if( m_pNaviPane )
					m_pNaviPane->PreventUpdate(TRUE);
			}
#endif

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
#if _ENABLE_SUBPANE_PROPERTY
					if( m_pPropPane )
						SendMessage(m_pPropPane->m_hWnd,PPM_SETPATH,(WPARAM)0,(LPARAM)0); // clear prop pane
#endif
					sel.ViewType  = ppg->ConsoleTypeId;
					sel.pszPath   = pszPath;
					sel.pszName   = NULL;
					sel.pszCurDir = NULL;

					m_pView->SelectView(&sel);

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
#if _ENABLE_SUBPANE_PROPERTY
					if( m_pPropPane )
						SendMessage(m_pPropPane->m_hWnd,PPM_SETPATH,(WPARAM)0,(LPARAM)0); // clear prop pane
#endif
					sel.ViewType  = ppg->ConsoleTypeId;
					sel.pszCurDir = NULL;
					sel.pszPath   = pszPath;

					m_pView->SelectView(&sel);

					_SetCurPath(pszPath);	

					FreeMemory(pszPath);
				}
			}
		}
		else
		{
			//
			// Other Console
			//
#if _ENABLE_SUBPANE_PROPERTY
			if( m_pPropPane )
				SendMessage(m_pPropPane->m_hWnd,PPM_SETPATH,(WPARAM)0,(LPARAM)0); // clear prop pane
#endif
			sel.pszCurDir = NULL;
			sel.pszPath   = NULL;
			m_pView->SelectView(&sel);
		}

#if _ENABLE_SUBPANE_NAVIGATION
		if( m_bInpacePageChange )
		{
			if( m_pNaviPane )
				m_pNaviPane->PreventUpdate(FALSE);
		}
#endif

		CONSOLE_VIEW_ID *pcv = _GET_CONSOLE_VIEW_ID(m_hWnd);
		if( pcv )
		{
			pcv->wndId = ppg->ConsoleTypeId;
		}

		return 0;
	}

	LRESULT OnChangeDirectory(SELECT_ITEM* pSelectItem)
	{
		pSelectItem->ViewType = m_pView->GetCurConsoleId();

#if _ENABLE_FILELIST_SUBPANE
#if _ENABLE_SUBPANE_PROPERTY
		if( m_pPropPane && IsWindowVisibleEx(m_pPropPane->m_hWnd) )
		{
			SendMessage(m_pPropPane->m_hWnd,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
		}
#endif
#if _ENABLE_SUBPANE_INFORMATION
		if( m_pInfoPane && IsWindowVisibleEx(m_pInfoPane->m_hWnd) )
		{
			SendMessage(m_pInfoPane->m_hWnd,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
			SendMessage(m_pInfoPane->m_hWnd,PPM_CHDIR,(WPARAM)0,(LPARAM)pSelectItem->pszPath);
		}
#endif
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
			sel.ViewType = pSelectItem->ViewType;
			sel.pszPath  = pszNewCurDir; // change to  directory
			sel.pszName  = pszFileName;
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

				if( PATHTYPE_NT_DEVICE != PathType && !(pSelectItem->pszPath[0] == L'\\' && pSelectItem->pszPath[1] == L'\0') )
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

#if _ENABLE_SUBPANE_NAVIGATION
		if( m_pNaviPane && IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
		{
			PWSTR psz = (PWSTR)SendMessage(m_pView->GetPageHWND(),PM_GETWORKINGDIRECTORY,0,0);
			SendMessage(m_pNaviPane->m_hWnd,PPM_CHDIR,(WPARAM)0,(LPARAM)psz);
		}
#endif

		PWSTR psz = (PWSTR)SendMessage(m_pView->GetPageHWND(),PM_GETWORKINGDIRECTORY,0,0);
		if( psz )
		{
			_SafeMemFree(m_pszCurDir);
			m_pszCurDir = _MemAllocString(psz);
		}

		return 0;
	}

	void OnFileDateTimeView()
	{
		ChangePage(VOLUME_CONSOLE_VOLUMEFILES_DATETIME);
	}

	void OnFileListView()
	{
		ChangePage(VOLUME_CONSOLE_VOLUMEFILES);
	}

	void ChangePage(UINT ConsoleTypeId)
	{
		m_bInpacePageChange = true;
		UIS_PAGE pg = {0};
		pg.ConsoleTypeId = ConsoleTypeId;
		pg.pszPath       = _MemAllocString(m_pszCurDir);
		pg.pszFileName   = NULL;
		SendMessage(m_hWnd,WM_CONTROL_MESSAGE,UI_SELECT_PAGE,(LPARAM)&pg);
		_SafeMemFree(pg.pszPath);
		SetFocus(this->m_pView->GetPageHWND());
		m_bInpacePageChange = false;
	}

#if _ENABLE_FILELIST_SUBPANE

#if _ENABLE_SUBPANE_PROPERTY
	void CreatePropPane(BOOL bVisible=TRUE)
	{
		m_pPropPane = new CPropPaneWindow();

		DWORD dwStyle = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|(bVisible ? WS_VISIBLE : 0);
		DWORD dwExStyle = WS_EX_CONTROLPARENT;
		m_pPropPane->Create(m_hWnd,0,L"PropPane",dwStyle,dwExStyle);
	}
#endif
#if _ENABLE_SUBPANE_NAVIGATION
	void CreateVolummePane(BOOL bVisible=FALSE)
	{
		m_pNaviPane = new CNavigationPane();
	
		DWORD dwStyle = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|(bVisible ? WS_VISIBLE : 0);
		DWORD dwExStyle = WS_EX_CONTROLPARENT;
		m_pNaviPane->Create(m_hWnd,0,L"VolumePane",dwStyle,dwExStyle);
	}
#endif
#if _ENABLE_SUBPANE_INFORMATION
	void CreateInfoPane(BOOL bVisible=TRUE)
	{
		m_pInfoPane = new CInfoPaneWindow();

		DWORD dwStyle = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|(bVisible ? WS_VISIBLE : 0);
		DWORD dwExStyle = WS_EX_CONTROLPARENT;
		m_pInfoPane->Create(m_hWnd,0,L"InfoPane",dwStyle,dwExStyle);
	}
#endif
#if _ENABLE_SUBPANE_NAVIGATION
	void OnViewNavigationPane(UINT uCmdId)
	{
		WCHAR szBuf[MAX_PATH];

		switch( uCmdId )
		{
			case ID_VIEW_NAVIGATION_NONE:
			{
				if( IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
				{
					if( ShowPane(m_pNaviPane->m_hWnd) == 0 )
					{
						m_xVolumeSplitter = -1;
					}
				}
				break;
			}
			case ID_VIEW_NAVIGATION_VOLUMES:
			{
				if( m_pNaviPane == NULL )
					CreateVolummePane();
				if( !IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
					ShowPane(m_pNaviPane->m_hWnd);
				if( m_pNaviPane->ChangePane(NAVI_VOLUMES) )
				{
					if( NtPathGetVolumeName(m_pszCurDir,szBuf,MAX_PATH) == 0 )
						StringCchCopy(szBuf,_countof(szBuf),L"\\??\\C:");
					SendMessage(m_pNaviPane->m_hWnd,PPM_SELECTVOLUME,0,(LPARAM)szBuf);
				}
				break;
			}
			case ID_VIEW_NAVIGATION_DIRTRAVERSER:
			{
				if( m_pNaviPane == NULL )
					CreateVolummePane();
				if( !IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
					ShowPane(m_pNaviPane->m_hWnd);
				if( m_pNaviPane->ChangePane(NAVI_DIRTRAVERSER) )
				{
					SendMessage(m_pNaviPane->m_hWnd,PPM_CHDIR,0,(LPARAM)m_pszCurDir);
				}
				break;
			}
		}
	}
#endif
#if _ENABLE_SUBPANE_PROPERTY
	void OnViewPropertyPane()
	{
		if( ShowPane(m_pPropPane->m_hWnd) )
		{
			SendMessage(m_pView->GetPageHWND(),PM_RESEND_SELECTED_ITEM_DATA,0,0);
		}
		else
		{
			m_xPropSplitter = -1;
		}
	}
#endif
#if _ENABLE_SUBPANE_INFORMATION
	void OnViewInformationPane()
	{
		if( ShowPane(m_pInfoPane->m_hWnd) )
		{
			// Determines the initial display of the window by the selected items file system.
			PWSTR psz = (PWSTR)SendMessage(m_pView->GetPageHWND(),PM_GETWORKINGDIRECTORY,0,0);
			SendMessage(m_pInfoPane->m_hWnd,PPM_INITLAYOUT,0,(LPARAM)psz);

			// Retrieve selected file information again.
			SendMessage(m_pView->GetPageHWND(),PM_RESEND_SELECTED_ITEM_DATA,0,0);
		}
		else
		{
			m_yInfoSplitter = -1;
		}
	}
#endif

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
		if( m_pNaviPane && IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
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

#if _ENABLE_SUBPANE_PROPERTY
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
					CSimpleVSplitWindowEx::SetSplitterValue(&m_xPropSplitter);
					return 2;
				}
			}
		}
#endif
#if _ENABLE_SUBPANE_INFORMATION
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
	
				if( m_pNaviPane && IsWindowVisibleEx(m_pNaviPane->m_hWnd) )
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
#endif
		return 0;
	}

private:
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

	pWnd->SetViewStyle( dwOptionFlags );

	HWND hwndView = pWnd->Create(hWndParent,0,L"VolumeFileExplorer_ConsoleWindow",
							WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

	return hwndView;
}
