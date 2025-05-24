#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_volumefilelist_location.h                                           *
//*                                                                           *
//*  NT simple file location viewer page for VolumeWalker                     *
//*                                                                           *
//*  Author:  YAMASHITA Katsuhiro                                             *
//*                                                                           *
//*  History: 2024-11-22 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#define _USE_SHELL_ICON         1
 
#include "page_volumefilelist.h"
#include "runhelp.h"

//*****************************************************************************
//
//  CFileLocationPage
//
//  PURPOSE: File location viewer page class
//
//*****************************************************************************
class CFileLocationPage : public CFileListPage
{
	IApplicationsReader *m_pApps;

protected:
	virtual UINT GetConsoleId() const { return VOLUME_CONSOLE_VOLUMEFILELIST; }

	int AddIcon(HIMAGELIST himl,int id,int cxcy)
	{
		int iIndex = I_IMAGENONE;

		HICON hIcon;
		hIcon = (HICON)LoadImage(_GetResourceInstance(),
							MAKEINTRESOURCE(id),
							IMAGE_ICON,
							cxcy,
							cxcy,
							LR_DEFAULTCOLOR);
		if( hIcon )
		{
			iIndex = ImageList_AddIcon(himl,hIcon);
			DestroyIcon(hIcon);
		}
		return iIndex;
	}

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
#if _USE_SHELL_ICON
			HIMAGELIST himl = GetGlobalShareImageList(0);
#else
			int cxcy = 16;
			HIMAGELIST himl = ImageList_Create(cxcy,cxcy,ILC_COLOR32|ILC_MASK,8, 0);

			AddIcon(himl,IDI_FOLDER,cxcy);
			AddIcon(himl,IDI_UPWARD,cxcy);
#endif
			ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);
		}

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		InitColumnDefinitions();

		CreateApplicationList(&m_pApps);
	}

	virtual LRESULT OnGetDispImage(int id,NMLVDISPINFO *pdi, CFileInfoItem *pItem)
	{
#if _USE_SHELL_ICON
		pdi->item.iImage = GetShellFileImageListIndex(NULL,pItem->pFI->hdr.FileName,pItem->pFI->FileAttributes);
#else
		if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if( wcscmp(pItem->pFI->hdr.FileName,L"..") == 0 )
				pdi->item.iImage = 1;
			else
				pdi->item.iImage = 0;
		}
		else
			pdi->item.iImage = I_IMAGENONE;
#endif
		if( pdi->item.iImage & 0xff000000 )
		{
			pdi->item.state = INDEXTOOVERLAYMASK(pdi->item.iImage >> 24);
			pdi->item.stateMask = LVIS_OVERLAYMASK;
			pdi->item.mask |= LVIF_STATE;
		}

		pdi->item.iImage = pdi->item.iImage & ~0xFF000000;

		pdi->item.mask |= LVIF_DI_SETITEM;

		return 0;
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
		if( ID_OPEN_APP_FIRST <= CmdId && CmdId <= ID_OPEN_APP_LAST )
		{
			return OnOpenFileWithApplication(m_hWnd,CmdId);
		}
		return CFileListPage::InvokeCommand(CmdId);
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_pApps )
		{
			m_pApps->Release();
			m_pApps = NULL;
		}
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
};
