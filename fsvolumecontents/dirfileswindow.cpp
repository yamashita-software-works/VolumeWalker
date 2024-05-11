//****************************************************************************
//
//  dirfileswindow.cpp
//
//  Implements the directory files view host window.
//
//  Auther: YAMASHITA Katsuhiro
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
#include "dirfileswindow.h"
#include "dirfilesview.h"

class CDirectoryFilesWindow : public CBaseWindow
{
public:
	IViewBaseWindow *m_pView;
	HWND  m_hWndCtrlFocus;

	CDirectoryFilesWindow()
	{
		m_hWnd = NULL;
		m_pView = NULL;
		m_hWndCtrlFocus = NULL;
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
				case ID_UP_DIR:
					OnUpDir();
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
				return OnSetDirectory(m_hWnd,0,0,lParam);
			case UI_SELECT_FILE:
				return OnSelectFile(m_hWnd,0,0,lParam);
			case UI_INIT_VIEW:
				return OnInitView(m_hWnd,0,0,lParam);
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

	LRESULT OnQueryMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_PRETRANSLATEMESSAGE:
				if( m_pView )
					return SendMessage(m_pView->GetHWND(),uMsg,wParam,lParam); // forward to current view
				return 0;
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

	VOID OnUpDir()
	{
		LRESULT lResult;
		WCHAR szDirPath[MAX_PATH];
		lResult = SendMessage(m_pView->GetHWND(),PM_GETCURDIR,MAX_PATH,(LPARAM)szDirPath);

		if( lResult == 1 )
		{
			// NTFS Special File
			UNICODE_STRING usRoot;
			SplitRootRelativePath(szDirPath,&usRoot,NULL);
			HANDLE hFile;
			OpenFile_U(&hFile,NULL,&usRoot,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0);
			LARGE_INTEGER liRoot;
			GetFileId(hFile,&liRoot);
			CloseHandle(hFile);

			LARGE_INTEGER li;
			SendMessage(m_pView->GetHWND(),PM_GETCURDIR,0,(LPARAM)&li);
			if( li.QuadPart != liRoot.QuadPart )
			{
				UNICODE_STRING usPath;
				UNICODE_STRING usFileName;

				RtlInitUnicodeString(&usPath,szDirPath);
				SplitPathFileName_U(&usPath,&usFileName);
				RemoveBackslash_U(&usPath);

				PWSTR ParentPath = AllocateSzFromUnicodeString(&usPath);
				PWSTR ChooseFileName = AllocateSzFromUnicodeString(&usFileName);

				SELECT_ITEM sel = {0};
				sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
				sel.mask = SI_MASK_FILEID;
				sel.Flags = _FLG_NTFS_SPECIALFILE;
				sel.FileId.dwSize = sizeof(FILE_ID_DESCRIPTOR);
				sel.FileId.Type = FileIdType;
				sel.pszPath = ParentPath;
				sel.pszName = ChooseFileName;
				sel.FileId.FileId = li;
				SendMessage(m_hWnd,WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);

				FreeMemory(ParentPath);
				FreeMemory(ChooseFileName);
			}
			else
			{
				UNICODE_STRING usPath;
				UNICODE_STRING usFileName;
				RtlInitUnicodeString(&usPath,szDirPath);
				SplitRootRelativePath_U(&usPath,&usRoot,&usFileName);
				PWSTR ParentPath = AllocateSzFromUnicodeString(&usRoot);
				PWSTR ChooseFileName = AllocateSzFromUnicodeString(&usFileName);

				SELECT_ITEM sel = {0};
				sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
				sel.pszPath = ParentPath;
				sel.pszName = ChooseFileName;
				SendMessage(m_hWnd,WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);

				FreeMemory(ParentPath);
				FreeMemory(ChooseFileName);
			}

			return ;
		}

		if( IsRootDirectory_W(szDirPath) )
		{
			return ;
		}

		SELECT_ITEM sel = {0};
		sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
		sel.pszCurDir = szDirPath;
		sel.pszName   = L"..";
		SendMessage(m_hWnd,WM_CONTROL_MESSAGE,UI_CHANGE_DIRECTORY,(LPARAM)&sel);
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

		if( PathFileExists_W(pszPath,NULL) )
		{
			InitData(NULL);

			SELECT_ITEM sel = {0};
			sel.ViewType  = VOLUME_CONSOLE_CONTENT_FILES;
			sel.pszCurDir = (PWSTR)pszPath;
			sel.pszPath   = (PWSTR)pszPath;
			sel.pszName   = (PWSTR)NULL;
			m_pView->SelectView(&sel); // call enum in this call
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
};

//////////////////////////////////////////////////////////////////////////////

HWND CreateDirectoryFilesWindow(HWND hWndParent)
{
	CDirectoryFilesWindow::RegisterClass(GETINSTANCE(hWndParent));

	CDirectoryFilesWindow *pView = new CDirectoryFilesWindow;

	HWND hwndView = pView->Create(hWndParent,0,L"DirectoryFiles",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);

	return hwndView;
}
