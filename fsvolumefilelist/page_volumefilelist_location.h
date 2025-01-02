#pragma once
//*****************************************************************************
//
//  CFileLocationPage
//
//  PURPOSE: File location viewer page class
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2024-11-22 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#define _ENABLE_OPEN_APPLICATIONS  1

#include "page_volumefilelist.h"
#include "runhelp.h"

class CFileLocationPage : public CFileListPage
{
	IApplicationsReader *m_pApps;

protected:
	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_SIMPLEVOLUMEFILELIST; }

	void InitListView()
	{
		SetupColumnNameSet();

		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              DefListViewStyle(),
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif
		if( m_bEnableIconImage )
		{
			HIMAGELIST himl = GetGlobalShareImageList(0);
			ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);
		}

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		InitColumnDefinitions();

#if _ENABLE_OPEN_APPLICATIONS
		CreateApplicationList(&m_pApps);
#endif
	}

	virtual BOOL LoadColumns(HWND hWndList,PCWSTR pszSectionName)
	{
		COLUMN_TABLE *pcoltbl;

		WCHAR buf[] = 
				L"Name=240\0"
				L"Extension=80\0"
				L"FileId=140\0"
				L"Attributes=80\0"
				L"Lcn=100\0"
				L"PhysicalDrive=120\0"
				L"PhysicalOffset=118\0"
				L"AllocationSize=116\0"
				L"EndOfFile=116\0"
				L"EaSize=94\0"
				L"LastWriteTime=180\0"
				L"CreationTime=180\0"
				L"LastAccessTime=180\0"
				L"ChangeTime=180\0"
				L"CreationTime=180\0";
		int cb = sizeof(buf);

		if( m_columns.LoadUserDefinitionColumnTableFromText(&pcoltbl,buf,cb) == 0)
			return FALSE;

		InsertColumns(hWndList,pcoltbl);

		m_columns.FreeUserDefinitionColumnTable(pcoltbl);

		LARGE_INTEGER li;
		PWSTR pszSortColumn = L"Name,1";
		li = m_columns.GetColumnSortInfoFromText(pszSortColumn);
		if( li.QuadPart != 0 )
		{
			m_Sort.CurrentSubItem = FindSubItemById( li.LowPart );
			m_Sort.Direction = li.HighPart;
		}

		return TRUE;
	}

	virtual HRESULT MakeContextMenu(HMENU hMenu)
	{
		AppendMenu(hMenu,MF_STRING,ID_OPEN,L"&Open");
		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		SetMenuDefaultItem(hMenu,ID_OPEN,FALSE);

#if _ENABLE_OPEN_APPLICATIONS
		if( m_pApps )
		{
			HMENU hAppMenu = CreatePopupMenu();
			ULONG cApps = 0;
			m_pApps->GetItemCount( &cApps );
	
			WCHAR szName[MAX_PATH];
	
			for(ULONG i = 0; i < cApps; i++)
			{
				if( m_pApps->IsSeparator(i) == S_FALSE )
				{
					m_pApps->GetFriendlyName(i,szName,MAX_PATH);
					AppendMenu(hAppMenu,MF_STRING,ID_OPEN_APP_FIRST+i,szName);
				}
				else
				{
					AppendMenu(hAppMenu,MF_STRING,0,0);
				}
			}	
	
			AppendMenu(hMenu,MF_STRING,0,0);
			AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hAppMenu,L"Open with Application");
		}
#endif
		return S_OK;
	}

	virtual HRESULT QueryCmdState(UINT uCmdId,UINT *puState)
	{
		if( ID_OPEN_APP_FIRST <= uCmdId && uCmdId <= ID_OPEN_APP_LAST )
		{
			*puState = UPDUI_ENABLED;
			return S_OK;
		}

		return CFileListPage::QueryCmdState(uCmdId,puState);
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
#if _ENABLE_OPEN_APPLICATIONS
		if( ID_OPEN_APP_FIRST <= CmdId && CmdId <= ID_OPEN_APP_LAST )
		{
			return OnOpenFileWithApplication(m_hWnd,CmdId);
		}
#endif
		return CFileListPage::InvokeCommand(CmdId);
	}

	HRESULT OnOpenFileWithApplication(HWND hWnd,UINT uCmdId)
	{
		HRESULT hr;
		ULONG Index = uCmdId - ID_OPEN_APP_FIRST;

		ASSERT( m_pApps != NULL );

		WCHAR szCmdLine[MAX_PATH];
		WCHAR szStartupPath[MAX_PATH];
		WCHAR szAppendPath[MAX_PATH];
		WCHAR szExecPath[MAX_PATH];

		ZeroMemory(szCmdLine,sizeof(szCmdLine));
		ZeroMemory(szStartupPath,sizeof(szStartupPath));
		ZeroMemory(szAppendPath,sizeof(szAppendPath));
		ZeroMemory(szExecPath,sizeof(szExecPath));

		m_pApps->GetCommandLine(Index,szCmdLine,MAX_PATH);
		m_pApps->GetStartupDirectory(Index,szStartupPath,MAX_PATH);
		m_pApps->GetAppendPath(Index,szAppendPath,MAX_PATH);
		m_pApps->GetAppendPath(Index,szExecPath,MAX_PATH);

		FS_SELECTED_FILE FilePath = {0};
		if( (hr = GetSelectedFile(hWnd,&FilePath)) == S_OK )
		{
			CRunHelp run;
			CStringBuffer dosPath(32768);

			NtPathToDosPath(FilePath.pszPath,dosPath,dosPath.GetBufferSize());

			CRunHelp::RUN_COMMAND_PARAM cf = {0};

			cf.mask                = RHCF_MASK_PATH|RHCF_MASK_DOSPATH|RHCF_MASK_PARAM;
			cf.pszPath             = FilePath.pszPath; // open file nt-path
			cf.pszDosPath          = dosPath;          // open file dos-path
			cf.pszExecPath         = szCmdLine;        // app command line (with "%1")
			cf.pszAppendPath       = szAppendPath;
			cf.pszStartupDirectory = szStartupPath;

			run.RunCommand(0,&cf);

			FreeSelectedFile(&FilePath);
		}
		return hr;
	}

	HRESULT GetSelectedFile(HWND hWnd,FS_SELECTED_FILE *FilePath)
	{
		if( hWnd )
		{
			FS_SELECTED_FILE path = {0};
			if( SendMessage(hWnd,PM_GETSELECTEDFILE,0,(LPARAM)&path) )
			{
				FilePath->FileAttributes = path.FileAttributes;
				FilePath->pszPath = _MemAllocString(path.pszPath);
				LocalFree(path.pszPath);
				return S_OK;
			}
		}
		return S_FALSE;
	}

	HRESULT FreeSelectedFile(FS_SELECTED_FILE *FilePath)
	{
		_SafeMemFree(FilePath->pszPath);
		return S_OK;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
#if _ENABLE_OPEN_APPLICATIONS
		if( m_pApps )
		{
			m_pApps->Release();
			m_pApps = NULL;
		}
#endif
		return CFileListPage::OnDestroy(hWnd,uMsg,wParam,lParam);
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		}
		return CFileListPage::WndProc(hWnd,uMsg,wParam,lParam);
	}

public:
	CFileLocationPage()
	{
		m_pApps = NULL;
		m_bEnumShadowCopyVolumes = FALSE;
	}

	virtual ~CFileLocationPage()
	{
	}
};
